#pragma once
#include "CorePch.h"
#include <fstream>

struct MetricsSample final
{
    std::chrono::system_clock::time_point _timestamp;
    uint32_t _ccu{};
    uint32_t _sendPacketsPerSecond{};
    uint32_t _recvPacketsPerSecond{};
    uint32_t _taskQueueSize{};
};

class MetricsLogger final
{
public:
    using Singleton = Singleton<MetricsLogger>;

    bool Start(std::string const& filePath);
    void Stop();

    void Enqueue(MetricsSample sample);

private:
    void ThreadMain();
    void WriteSample(MetricsSample const& sample);

    std::ofstream _file;
    std::atomic<bool> _stop{ false };
    std::thread _thread;

    std::mutex _mutex;
    std::condition_variable _cv;
    std::vector<MetricsSample> _pendingSamples;
};
