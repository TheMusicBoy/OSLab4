#include <common/logging.h>

#include <nlohmann/json.hpp>

#include <config.h>
#include <fstream>

namespace NConfig {

////////////////////////////////////////////////////////////////////////////////

TConfigPtr TConfig::LoadFromFile(const std::string& filePath) {
    auto config = NCommon::New<TConfig>();

    try {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            THROW("Failed to open config file: {}", filePath);
        }

        nlohmann::json configJson;
        file >> configJson;

        // Load values with defaults from config instance
        config->serial_port = configJson.value("serial_port", config->serial_port);
        config->baud_rate = configJson.value("baud_rate", config->baud_rate);
        config->mesure_delay = configJson.value("mesure_delay", config->mesure_delay);
        
        config->TemperaturePath = configJson.value("temperature_path", config->TemperaturePath.string());
        config->TemperatureHourPath = configJson.value("hourly_path", config->TemperatureHourPath.string());
        config->TemperatureDayPath = configJson.value("daily_path", config->TemperatureDayPath.string());

        return config;
    }
    catch (const nlohmann::json::exception& e) {
        LOG_ERROR("JSON parsing error: {}", e.what());
        RETHROW(e, "Invalid config file format");
    }
    catch (const std::exception& e) {
        RETHROW(e, "Config loading failed");
    }
}

////////////////////////////////////////////////////////////////////////////////

} // namespace NConfig
