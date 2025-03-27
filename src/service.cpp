#include <service.h>
#include <common/logging.h>
#include <common/periodic_executor.h>
#include <common/weak_ptr.h>
#include <common/threadpool.h>
#include <ipc/serial_port.h>

#include <chrono>
#include <deque>
#include <mutex>
#include <limits>
#include <iomanip>
#include <ctime>

////////////////////////////////////////////////////////////////////////////////

namespace {

////////////////////////////////////////////////////////////////////////////////

#ifdef _WIN32
time_t timegm(struct tm* tm) {
    _tzset();
    return _mkgmtime(tm);
}
#endif

std::string ReadingToString(TReading reading) {
    auto time = std::chrono::system_clock::to_time_t(reading.timestamp);
    std::ostringstream oss;
    oss << std::put_time(gmtime(&time), "%Y-%m-%dT%H:%M:%SZ") << " "
        << std::fixed << std::setprecision(std::numeric_limits<double>::digits10 + 1)
        << reading.temperature;
    return oss.str();
}

TReading StringToReading(const std::string& str) {
    std::istringstream iss(str);
    std::tm tm = {};
    std::string datetime;
    double temp;
    iss >> datetime >> temp;
    
    std::istringstream dtstream(datetime);
    dtstream >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    tm.tm_isdst = 0;
    time_t timestamp = timegm(&tm);
    
    return {
        std::chrono::system_clock::from_time_t(timestamp),
        temp
    };
}

std::deque<TReading> ReadingsFromFile(const std::filesystem::path& file) {
    std::deque<TReading> data;
    try {
        std::fstream fin(file, std::ios::in);
        std::string str;
        while (std::getline(fin, str)) {
            data.emplace_front(StringToReading(str));
        }
    } catch (std::exception& ex) {
        LOG_WARNING("Failed to read file with readings (File: {}, Exception: {})", file, ex);
    }
    LOG_INFO("Got {} from file {}", data.size(), file);
    return data;
}

void ReadingsToFile(const std::filesystem::path& file, const std::deque<TReading>& data) {
    try {
        std::filesystem::create_directory(file.parent_path());
        std::fstream fout(file, std::ios::out);
        for (const auto& reading : data) {
            fout << ReadingToString(reading) << '\n';
        }
    } catch (std::exception& ex) {
        LOG_WARNING("Failed to write readings to file (File: {}, Exception: {})", file, ex);
    }
}

////////////////////////////////////////////////////////////////////////////////

} // namespace

////////////////////////////////////////////////////////////////////////////////

TService::TService(NConfig::TConfigPtr config, std::function<std::optional<TReading>(double)> processor)
    : Config_(std::move(config)),
      Port_(NCommon::New<NIpc::TComPort>(Config_->serial_port, Config_->baud_rate)),
      Processor_(processor),
      ThreadPool_(NCommon::New<NCommon::TThreadPool>(2)),
      Invoker_(NCommon::New<NCommon::TInvoker>(ThreadPool_)),
      Cache_(NCommon::New<TCache>())
{
    Cache_->rawReadings = ReadingsFromFile(Config_->TemperaturePath);
    Cache_->hourlyAverages = ReadingsFromFile(Config_->TemperatureHourPath);
    Cache_->dailyAverages = ReadingsFromFile(Config_->TemperatureDayPath);
}

TService::~TService() {
    MesurePeriodicExecutor_->Stop();
}

void TService::Start() {
    LOG_INFO("Starting service with measurement interval {} milliseconds", 
            Config_->mesure_delay);

    MesurePeriodicExecutor_ = NCommon::New<NCommon::TPeriodicExecutor>(
        NCommon::Bind(&TService::MesureTemperature, MakeWeak(this)),
        Invoker_,
        duration_cast<std::chrono::milliseconds>(std::chrono::milliseconds(Config_->mesure_delay))
    );

    MesurePeriodicExecutor_->Start();
}

void TService::MesureTemperature() {
    try {
        std::string response = Port_->ReadLine();
        
        if (response.empty()) {
            return;
        }

        LOG_INFO("Reading: {}", response);

        if (auto reading = Processor_(std::stod(response))) {
            Invoker_->Run(NCommon::Bind(
                &TService::ProcessTemperature,
                MakeWeak(this),
                *reading
            ));
        }
    } catch (const NCommon::TException& ex) {
        LOG_ERROR("Temperature measurement failed: {}", ex.what());
    }
}

void TService::ProcessTemperature(TReading reading) {
    std::lock_guard<std::mutex> lock(Cache_->dataMutex);
    Cache_->rawReadings.push_back(reading);
    
    const auto hour_ago = reading.timestamp - std::chrono::hours(1);
    const auto day_ago = reading.timestamp - std::chrono::days(1);
    const auto month_ago = reading.timestamp - std::chrono::days(30);
    const auto year_ago = reading.timestamp - std::chrono::days(360);

    while (!Cache_->rawReadings.empty() && Cache_->rawReadings.front().timestamp < day_ago) {
        Cache_->rawReadings.pop_front();
    }
    
    while (!Cache_->hourlyAverages.empty() && Cache_->hourlyAverages.front().timestamp < month_ago) {
        Cache_->hourlyAverages.pop_front();
    }

    while (!Cache_->dailyAverages.empty() && Cache_->dailyAverages.front().timestamp < year_ago) {
        Cache_->dailyAverages.pop_front();
    }

    ReadingsToFile(Config_->TemperaturePath, Cache_->rawReadings);

// Create hourly average temperature
    bool hourlyUpdate = Cache_->hourlyAverages.empty() && Cache_->rawReadings.front().timestamp < hour_ago
        || !Cache_->hourlyAverages.empty() && Cache_->hourlyAverages.back().timestamp < hour_ago;

    if (!hourlyUpdate) {
        return;
    }

    double sum = 0;
    size_t count = 0;
    for (auto iter = Cache_->rawReadings.rbegin(); iter != Cache_->rawReadings.rend() && iter->timestamp >= hour_ago; iter++) {
        count++;
        sum += iter->temperature;
    }
    Cache_->hourlyAverages.emplace_back(reading.timestamp, sum / count);

    ReadingsToFile(Config_->TemperatureHourPath, Cache_->hourlyAverages);
    
// Create daily average temperature
    bool dailyUpdate = Cache_->dailyAverages.empty() && Cache_->hourlyAverages.front().timestamp < day_ago
        || !Cache_->dailyAverages.empty() && Cache_->dailyAverages.back().timestamp < day_ago;

    if (!dailyUpdate) {
        return;
    }

    sum = 0;
    count = 0;
    for (auto iter = Cache_->hourlyAverages.rbegin(); iter != Cache_->hourlyAverages.rend() && iter->timestamp >= day_ago; iter++) {
        count++;
        sum += iter->temperature;
    }
    Cache_->dailyAverages.emplace_back(reading.timestamp, sum / count);

    ReadingsToFile(Config_->TemperatureDayPath, Cache_->dailyAverages);
}
