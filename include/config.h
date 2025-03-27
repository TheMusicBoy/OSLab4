#pragma once

#include <common/exception.h>
#include <common/refcounted.h>
#include <common/intrusive_ptr.h>

#include <filesystem>
#include <string>

namespace NConfig {

struct TConfig
    : public NRefCounted::TRefCountedBase
{
    std::string serial_port =
#ifdef _WIN32
        "COM3";
#else 
        "/dev/ttyUSB0";
#endif
    unsigned baud_rate = 9600;
    unsigned mesure_delay = 100;
    
    std::filesystem::path TemperaturePath = "/var/log/temperature/current.log";
    std::filesystem::path TemperatureHourPath = "/var/log/temperature/hourly.log";
    std::filesystem::path TemperatureDayPath = "/var/log/temperature/daily.log";

    static NCommon::TIntrusivePtr<TConfig> LoadFromFile(const std::string& filePath);
};

DECLARE_REFCOUNTED(TConfig);

} // namespace NConfig
