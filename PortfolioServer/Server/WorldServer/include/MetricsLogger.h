#pragma once
#include "CorePch.h"
#include "Logger.h"
#include "Task.h"

struct MetricsSample final
{
    std::chrono::system_clock::time_point _timestamp;
    uint32_t _ccu{};
    uint32_t _sendPacketsPerSecond{};
    uint32_t _recvPacketsPerSecond{};
    uint32_t _sendBytesPerSecond{};
    uint32_t _recvBytesPerSecond{};
    uint32_t _cpuPercent{};
    size_t _taskQueueSize{};
    std::array<size_t, TASK_TYPE_MAX> _taskQueueSizeByType{};
    std::array<std::vector<size_t>, TASK_TYPE_MAX> _taskWorkerQueueSizesByType;
    uint64_t _playerMoveRequestCount{};
    uint64_t _playerAttackRequestCount{};
    uint64_t _playerAttackedCount{};
    uint64_t _zoneActorMovedCount{};
    uint64_t _zonePlayerEnteredCount{};
    uint64_t _zoneHpChangedCount{};
    uint64_t _zoneActorDiedCount{};
    uint64_t _zoneMoveSightDiffCount{};
    uint64_t _zoneMoveSightEnteredCount{};
    uint64_t _zoneMoveSightLeftCount{};
    uint64_t _zoneBroadcastInSightCount{};
    uint64_t _zoneBroadcastRecipientCount{};
    uint64_t _gridNearbyQueryCount{};
    uint64_t _gridNearbyResultCount{};
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
