#include <service/service.h>

#include <service/file_storage.h>

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


namespace NService {

////////////////////////////////////////////////////////////////////////////////

namespace {

////////////////////////////////////////////////////////////////////////////////

#ifdef _WIN32
time_t timegm(struct tm* tm) {
    _tzset();
    return _mkgmtime(tm);
}
#endif

////////////////////////////////////////////////////////////////////////////////

} // namespace

////////////////////////////////////////////////////////////////////////////////

TService::TService(NConfig::TConfigPtr config, std::function<std::optional<TReading>(double)> processor)
    : Config_(std::move(config)),
      Port_(NCommon::New<NIpc::TComPort>(Config_->SerialConfig)),
      Processor_(processor),
      ThreadPool_(NCommon::New<NCommon::TThreadPool>(2)),
      Invoker_(NCommon::New<NCommon::TInvoker>(ThreadPool_))
{
    Storage_ = std::make_unique<TFileStorage>(Config_->StorageConfig->FileStorageConfig);
}

TService::~TService() {
    MesurePeriodicExecutor_->Stop();
}

void TService::Start() {
    LOG_INFO("Starting service with measurement interval {} milliseconds", 
            Config_->MesureDelay);

    MesurePeriodicExecutor_ = NCommon::New<NCommon::TPeriodicExecutor>(
        NCommon::Bind(&TService::MesureTemperature, MakeWeak(this)),
        Invoker_,
        duration_cast<std::chrono::milliseconds>(std::chrono::milliseconds(Config_->MesureDelay))
    );

    MesurePeriodicExecutor_->Start();
}

void TService::MesureTemperature() {
    try {
        std::string response = Port_->ReadLine();
        
        if (response.empty()) {
            return;
        }

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
    Storage_->ProcessTemperature(reading);
}

////////////////////////////////////////////////////////////////////////////////

} // namespace NService
