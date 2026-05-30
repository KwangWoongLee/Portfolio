#pragma once
#include "CorePch.h"
#include "Logger.h"

struct ClientMetricsSample final
{
    std::chrono::system_clock::time_point _timestamp;
    uint32_t _processId{};
    uint32_t _botCount{};
    uint32_t _recvPacketsPerSecond{};
    uint32_t _recvBytesPerSecond{};
    uint32_t _packetQueueSize{};
    uint32_t _cpuPercent{};
    uint64_t _disconnectCount{};
};

class ClientMetricsLogger final
    : public Logger
{
public:
    using Singleton = Singleton<ClientMetricsLogger>;

    bool Start(std::string const& filePath);

    void Enqueue(ClientMetricsSample const& sample);
};
