#include <service.h>

#include <common/logging.h>
#include <common/exception.h>
#include <common/getopts.h>

#include <iostream>
#include <service.h>

int main(int argc, const char* argv[]) {
    NCommon::GetOpts opts;
    opts.AddOption('h', "help", "Show help message");
    opts.AddOption('v', "version", "Show version information");
    opts.AddOption('c', "config", "Path to config", true);
    opts.AddOption('a', "accelerate", "Acceleration of time", true);

    try {
        opts.Parse(argc, argv);

        if (opts.Has('h') || opts.Has("help")) {
            std::cerr << opts.Help();
            return 0;
        }

        if (opts.Has('v') || opts.Has("version")) {
            std::cerr << "Lab3 Service v2.0\n";
            return 0;
        }

        ASSERT(opts.Has('c') || opts.Has("config"), "Config is required");
        auto config = NConfig::TConfig::LoadFromFile(opts.Get("config"));

        std::function<std::optional<TReading>(double)> processor;

        if (opts.Has('a') || opts.Has("accelerate")) {
            double boost = 1;

            if (opts.Has('a')) boost = std::stod(opts.Get('a'));
            if (opts.Has("accelerate")) boost = std::stod(opts.Get("accelerate"));

            auto startTime = std::chrono::system_clock::now();

            processor = [startTime, boost](double value) -> std::optional<TReading> {
                if (value < -100 || value > 100) {
                    return {};
                }

                auto difference = std::chrono::system_clock::now() - startTime;
                difference *= boost;
                LOG_INFO("Reading: {}", value);
                return TReading(startTime + difference, value);
            };
        } else {
            processor = [](double value) -> std::optional<TReading> {
                if (value < -100 || value > 100) {
                    return {};
                }

                return TReading(std::chrono::system_clock::now(), value);
            };
        }

        auto service = NCommon::New<TService>(config, processor);

        service->Start();

        LOG_INFO("Service started. Press Ctrl+C to exit.");
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    } catch (const NCommon::TException& ex) {
        LOG_ERROR("Error: {}", ex.what());
        return 1;
    }
    
    return 0;
}
