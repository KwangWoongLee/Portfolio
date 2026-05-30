#pragma once
#include "CorePch.h"
#include "Logger.h"

struct MetricsSample final
{
    std::chrono::system_clock::time_point _timestamp;
    uint32_t _ccu{};
    uint32_t _sendPacketsPerSecond{};
    uint32_t _recvPacketsPerSecond{};
    uint32_t _sendBytesPerSecond{};
    uint32_t _recvBytesPerSecond{};
    uint32_t _cpuPercent{};
    uint32_t _taskQueueSize{};
    uint64_t _pendingSendBytesTotal{};
    uint64_t _sendOverflowCount{};
};

class MetricsLogger final
    : public Logger
{
public:
    using Singleton = Singleton<MetricsLogger>;

    bool Start(std::string const& filePath);

    void Enqueue(MetricsSample const& sample);
};
