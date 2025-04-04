#include "config.h"

#include <common/logging.h>

namespace NConfig {

////////////////////////////////////////////////////////////////////////////////

void TSimulatorConfig::Load(const nlohmann::json& data) {
    SerialConfig = TConfigBase::LoadRequired<NIpc::TSerialConfig>(data, "serial");
    TimeMultiplier = TConfigBase::Load<double>(data, "time_multiplier", 1);
    DelayMs = TConfigBase::Load<uint32_t>(data, "delay_ms", 100);
}

////////////////////////////////////////////////////////////////////////////////

void TFileStorageConfig::Load(const nlohmann::json& data) {
    TemperaturePath = TConfigBase::LoadRequired<std::string>(data, "temperature");
    TemperatureHourPath = TConfigBase::LoadRequired<std::string>(data, "hourly");
    TemperatureDayPath = TConfigBase::LoadRequired<std::string>(data, "daily");
}

////////////////////////////////////////////////////////////////////////////////

void TStorageConfig::Load(const nlohmann::json& data) {
    FileStorageConfig = TConfigBase::LoadRequired<NConfig::TFileStorageConfig>(data, "file_system");
}

////////////////////////////////////////////////////////////////////////////////

void TLogDestinationConfig::Load(const nlohmann::json& data) {
    Path = TConfigBase::LoadRequired<std::string>(data, "path");
    
    std::string levelStr = TConfigBase::Load<std::string>(data, "level", "Info");
    if (levelStr == "Debug") Level = NLogging::ELevel::Debug;
    else if (levelStr == "Info") Level = NLogging::ELevel::Info;
    else if (levelStr == "Warning") Level = NLogging::ELevel::Warning;
    else if (levelStr == "Error") Level = NLogging::ELevel::Error;
    else if (levelStr == "Fatal") Level = NLogging::ELevel::Fatal;
}

////////////////////////////////////////////////////////////////////////////////

void TConfig::Load(const nlohmann::json& data) {
    MesureDelay = TConfigBase::Load<unsigned>(data, "mesure_delay", MesureDelay);

    if (data.contains("logging") && data["logging"].is_array()) {
        for (const auto& dest : data["logging"]) {
            auto logConfig = NCommon::New<TLogDestinationConfig>();
            logConfig->Load(dest);
            LogDestinations.push_back(logConfig);
        }
    }

    SerialConfig = TConfigBase::LoadRequired<NIpc::TSerialConfig>(data, "serial");
    StorageConfig = TConfigBase::LoadRequired<TStorageConfig>(data, "storage");
}

} // namespace NConfig
