#include <ipc/serial_port.h>
#include <common/logging.h>

#include <chrono>
#include <thread>
#include <cmath>
#include <random>

class TSimulator {
public:
    TSimulator(const std::string& port, unsigned baud, double multiplier)
        : PortName(port),
          BaudRate(baud),
          Multiplier(multiplier),
          BaseTemp(20.0),
          Amplitude(15.0),
          Period(std::chrono::seconds(60)) 
    {}

    void Run() {
        auto port = NCommon::New<NIpc::TComPort>(PortName, BaudRate);
        port->Open();
        
        LOG_INFO("Temperature simulator started on {}", PortName);

        try {
            while (true) {
                auto now = std::chrono::system_clock::now();
                double temp = CalculateSimulatedTemp(now);
                std::string response = std::to_string(temp) + "\n";
                
                port->Write(response);
                LOG_INFO("Sent temperature: {}C", temp);
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        } catch (...) {
            port->Close();
            throw;
        }
    }

private:
    double CalculateSimulatedTemp(std::chrono::system_clock::time_point now) {
        // Get duration in seconds with multiplier
        auto real_duration = now.time_since_epoch();
        double sec = duration_cast<std::chrono::duration<double>>(real_duration).count();
        double simulated_time = sec * Multiplier;

        // Calculate seasonal variation (1 year period)
        const double yearly_period = 365.2425 * 24 * 3600; 
        double season_factor = std::sin((2 * M_PI * simulated_time) / yearly_period);
        
        // Calculate daily variation (24-hour period)
        const double daily_period = 24 * 3600;
        double daily_phase = std::fmod(simulated_time, daily_period);
        double daily_factor = std::sin((2 * M_PI * daily_phase) / daily_period);
        
        // Calculate current simulated day
        int current_day = static_cast<int>(simulated_time / daily_period);
        if(current_day != last_simulated_day_) {
            current_daily_offset_ = daily_offset_dist_(gen_);
            last_simulated_day_ = current_day;
        }

        // Combine components
        double temp = (BaseTemp + 10 * season_factor)
                    + (Amplitude * daily_factor)
                    + current_daily_offset_
                    + noise_dist_(gen_);
        
        return temp;
    }


    std::mt19937 gen_;
    std::uniform_real_distribution<> daily_offset_dist_{-5.0, 5.0};
    std::uniform_real_distribution<> noise_dist_{-0.5, 0.5};
    double current_daily_offset_ = 0;
    int last_simulated_day_ = -1;

    std::chrono::system_clock::time_point StartTime;
    std::string PortName;
    unsigned BaudRate;
    double Multiplier;
    double BaseTemp;
    double Amplitude;
    std::chrono::nanoseconds Period;
};

int main(int argc, char* argv[]) {
    if (argc < 3 || argc > 4) {
        std::cerr << "Usage: " << argv[0] << " <PORT> <BAUD_RATE> [MULTIPLIER=1.0]\n";
        return 1;
    }

    double multiplier = 1.0;
    if (argc == 4) {
        multiplier = std::stod(argv[3]);
    }

    try {
        TSimulator simulator(argv[1], std::stoi(argv[2]), multiplier);
        simulator.Run();
    } catch (const std::exception& ex) {
        LOG_ERROR("Simulator error: {}", ex.what());
        return 2;
    }
    
    return 0;
}
