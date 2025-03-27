#pragma once

#include <config.h>

#include <common/periodic_executor.h>
#include <common/threadpool.h>
#include <ipc/serial_port.h>

////////////////////////////////////////////////////////////////////////////////

struct TReading {
    std::chrono::system_clock::time_point timestamp;
    double temperature;
};

////////////////////////////////////////////////////////////////////////////////

struct TCache
    : NRefCounted::TRefCountedBase
{
    
    std::deque<TReading> rawReadings;
    std::deque<TReading> hourlyAverages;
    std::deque<TReading> dailyAverages;
    std::mutex dataMutex;
};


DECLARE_REFCOUNTED(TCache);

////////////////////////////////////////////////////////////////////////////////

class TService
    : public NRefCounted::TRefCountedBase
{
private:
    NConfig::TConfigPtr Config_;
    NIpc::TComPortPtr Port_;

    NCommon::TThreadPoolPtr ThreadPool_;
    NCommon::TInvokerPtr Invoker_;

    NCommon::TPeriodicExecutorPtr MesurePeriodicExecutor_;

    std::function<std::optional<TReading>(double)> Processor_;
    TCachePtr Cache_;

    void MesureTemperature();

    void ProcessTemperature(TReading reading);

public:
    TService(NConfig::TConfigPtr config, std::function<std::optional<TReading>(double)> processor);
    ~TService();

    void Start();

};

DECLARE_REFCOUNTED(TService);
