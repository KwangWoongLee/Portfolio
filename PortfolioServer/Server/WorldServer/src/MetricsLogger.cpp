#include "CorePch.h"
#include "MetricsLogger.h"
#include <iomanip>
#include <sstream>

namespace
{
    char const* const METRICS_CSV_HEADER =
        "timestamp,ccu,sendPps,recvPps,sendBps,recvBps,cpuPercent,queueSize,"
        "gameLogicQueueSize,networkIoQueueSize,dbQueueSize,timerQueueSize,"
        "gameLogicWorkerQueues,networkIoWorkerQueues,dbWorkerQueues,timerWorkerQueues,"
        "playerMoveRequests,playerAttackRequests,playerAttacked,"
        "zoneActorMoved,zonePlayerEntered,zoneHpChanged,zoneActorDied,"
        "zoneMoveSightDiffs,zoneMoveSightEntered,zoneMoveSightLeft,"
        "zoneBroadcastInSight,zoneBroadcastRecipients,"
        "gridNearbyQueries,gridNearbyResults,"
        "pendingSendBytesTotal,sendOverflowCount";

    std::string JoinQueueSizes(std::vector<size_t> const& queueSizes)
    {
        std::ostringstream oss;
        for (size_t i = 0; i < queueSizes.size(); ++i)
        {
            if (0 != i)
            {
                oss << '|';
            }
            oss << queueSizes[i];
        }
        return oss.str();
    }
}

bool MetricsLogger::Start(std::string const& filePath)
{
    return Logger::Start(filePath, METRICS_CSV_HEADER);
}

void MetricsLogger::Enqueue(MetricsSample const& sample)
{
    auto const epochTime = std::chrono::system_clock::to_time_t(sample._timestamp);
    std::tm utcTime{};
    ::gmtime_s(&utcTime, &epochTime);

    std::ostringstream oss;
    oss << std::put_time(&utcTime, "%Y-%m-%dT%H:%M:%SZ") << ','
        << sample._ccu << ','
        << sample._sendPacketsPerSecond << ','
        << sample._recvPacketsPerSecond << ','
        << sample._sendBytesPerSecond << ','
        << sample._recvBytesPerSecond << ','
        << sample._cpuPercent << ','
        << sample._taskQueueSize << ','
        << sample._taskQueueSizeByType[static_cast<size_t>(ETaskType::GameLogic)] << ','
        << sample._taskQueueSizeByType[static_cast<size_t>(ETaskType::NetworkIO)] << ','
        << sample._taskQueueSizeByType[static_cast<size_t>(ETaskType::DB)] << ','
        << sample._taskQueueSizeByType[static_cast<size_t>(ETaskType::Timer)] << ','
        << JoinQueueSizes(sample._taskWorkerQueueSizesByType[static_cast<size_t>(ETaskType::GameLogic)]) << ','
        << JoinQueueSizes(sample._taskWorkerQueueSizesByType[static_cast<size_t>(ETaskType::NetworkIO)]) << ','
        << JoinQueueSizes(sample._taskWorkerQueueSizesByType[static_cast<size_t>(ETaskType::DB)]) << ','
        << JoinQueueSizes(sample._taskWorkerQueueSizesByType[static_cast<size_t>(ETaskType::Timer)]) << ','
        << sample._playerMoveRequestCount << ','
        << sample._playerAttackRequestCount << ','
        << sample._playerAttackedCount << ','
        << sample._zoneActorMovedCount << ','
        << sample._zonePlayerEnteredCount << ','
        << sample._zoneHpChangedCount << ','
        << sample._zoneActorDiedCount << ','
        << sample._zoneMoveSightDiffCount << ','
        << sample._zoneMoveSightEnteredCount << ','
        << sample._zoneMoveSightLeftCount << ','
        << sample._zoneBroadcastInSightCount << ','
        << sample._zoneBroadcastRecipientCount << ','
        << sample._gridNearbyQueryCount << ','
        << sample._gridNearbyResultCount << ','
        << sample._pendingSendBytesTotal << ','
        << sample._sendOverflowCount;

    EnqueueLine(oss.str());
}
