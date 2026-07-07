#include "CorePch.h"
#include "ObserverManager.h"
#include "ObserverSession.h"
#include "ObserverPackets.h"
#include "ZoneManager.h"
#include "PlayerManager.h"
#include "Metrics.h"
#include "MetricsLogger.h"
#include "TaskDispatcher.h"
#include "ProcessInfo.h"

namespace
{
    auto constexpr CSV_LOG_INTERVAL = std::chrono::milliseconds(10000);
}

void ObserverManager::Register(std::shared_ptr<ObserverSession> const& session)
{
    std::unique_lock lock(_mutex);
    _observers.emplace(session->GetSessionId(), session);
}

void ObserverManager::Unregister(SessionId const sessionId)
{
    std::unique_lock lock(_mutex);
    _observers.erase(sessionId);
}

void ObserverManager::PushSnapshot()
{
    auto const currentTime = Clock::now();
    auto const currentRecvPacketCount = Metrics::g_recvPacketCount.load(std::memory_order_relaxed);
    auto const currentSendPacketCount = Metrics::g_sendPacketCount.load(std::memory_order_relaxed);
    auto const currentRecvByteCount = Metrics::g_recvByteCount.load(std::memory_order_relaxed);
    auto const currentSendByteCount = Metrics::g_sendByteCount.load(std::memory_order_relaxed);
    auto const currentProcessCpuTime100ns = ProcessInfo::GetProcessCpuTime100ns();

    auto const elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - _lastSampleTime).count();
    auto const safeElapsedMs = (elapsedMs <= 0) ? 1 : elapsedMs;

    auto const recvPps = static_cast<uint32_t>((currentRecvPacketCount - _lastRecvPacketCount) * 1000 / safeElapsedMs);
    auto const sendPps = static_cast<uint32_t>((currentSendPacketCount - _lastSendPacketCount) * 1000 / safeElapsedMs);
    auto const recvBytesPerSecond = static_cast<uint32_t>((currentRecvByteCount - _lastRecvByteCount) * 1000 / safeElapsedMs);
    auto const sendBytesPerSecond = static_cast<uint32_t>((currentSendByteCount - _lastSendByteCount) * 1000 / safeElapsedMs);

    auto const cpuPercent = ProcessInfo::ComputeCpuPercent(_lastProcessCpuTime100ns, currentProcessCpuTime100ns, safeElapsedMs);

    _lastSampleTime = currentTime;
    _lastRecvPacketCount = currentRecvPacketCount;
    _lastSendPacketCount = currentSendPacketCount;
    _lastRecvByteCount = currentRecvByteCount;
    _lastSendByteCount = currentSendByteCount;
    _lastProcessCpuTime100ns = currentProcessCpuTime100ns;

    auto const queueSnapshot = TaskDispatcher::Singleton::GetConstInstance().GetQueueSnapshot();

    W2OSnapshot pkt;
    pkt._ccu = static_cast<uint32_t>(PlayerManager::Singleton::GetConstInstance().GetCount());
    pkt._sendPps = sendPps;
    pkt._recvPps = recvPps;
    pkt._sendBytesPerSecond = sendBytesPerSecond;
    pkt._recvBytesPerSecond = recvBytesPerSecond;
    pkt._cpuPercent = cpuPercent;
    pkt._queueLen = static_cast<uint32_t>(queueSnapshot.GetTotalQueueSize());

    auto const sinceLastCsvLogMs = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - _lastCsvLogTime);
    if (CSV_LOG_INTERVAL <= sinceLastCsvLogMs)
    {
        MetricsSample sample;
        sample._timestamp = std::chrono::system_clock::now();
        sample._ccu = pkt._ccu;
        sample._sendPacketsPerSecond = pkt._sendPps;
        sample._recvPacketsPerSecond = pkt._recvPps;
        sample._sendBytesPerSecond = pkt._sendBytesPerSecond;
        sample._recvBytesPerSecond = pkt._recvBytesPerSecond;
        sample._cpuPercent = pkt._cpuPercent;
        sample._taskQueueSize = queueSnapshot.GetTotalQueueSize();
        sample._taskQueueSizeByType = queueSnapshot._queueSizeByTaskType;
        sample._taskWorkerQueueSizesByType = queueSnapshot._workerQueueSizesByTaskType;
        sample._playerMoveRequestCount = Metrics::g_playerMoveRequestCount.load(std::memory_order_relaxed);
        sample._playerAttackRequestCount = Metrics::g_playerAttackRequestCount.load(std::memory_order_relaxed);
        sample._playerAttackedCount = Metrics::g_playerAttackedCount.load(std::memory_order_relaxed);
        sample._zoneActorMovedCount = Metrics::g_zoneActorMovedCount.load(std::memory_order_relaxed);
        sample._zonePlayerEnteredCount = Metrics::g_zonePlayerEnteredCount.load(std::memory_order_relaxed);
        sample._zoneHpChangedCount = Metrics::g_zoneHpChangedCount.load(std::memory_order_relaxed);
        sample._zoneActorDiedCount = Metrics::g_zoneActorDiedCount.load(std::memory_order_relaxed);
        sample._zoneMoveSightDiffCount = Metrics::g_zoneMoveSightDiffCount.load(std::memory_order_relaxed);
        sample._zoneMoveSightEnteredCount = Metrics::g_zoneMoveSightEnteredCount.load(std::memory_order_relaxed);
        sample._zoneMoveSightLeftCount = Metrics::g_zoneMoveSightLeftCount.load(std::memory_order_relaxed);
        sample._zoneBroadcastInSightCount = Metrics::g_zoneBroadcastInSightCount.load(std::memory_order_relaxed);
        sample._zoneBroadcastRecipientCount = Metrics::g_zoneBroadcastRecipientCount.load(std::memory_order_relaxed);
        sample._gridNearbyQueryCount = Metrics::g_gridNearbyQueryCount.load(std::memory_order_relaxed);
        sample._gridNearbyResultCount = Metrics::g_gridNearbyResultCount.load(std::memory_order_relaxed);
        sample._pendingSendBytesTotal = Metrics::g_pendingSendBytesTotal.load(std::memory_order_relaxed);
        sample._sendOverflowCount = Metrics::g_sendOverflowCount.load(std::memory_order_relaxed);
        MetricsLogger::Singleton::GetInstance().Enqueue(sample);

        _lastCsvLogTime = currentTime;
    }

    ZoneManager::Singleton::GetConstInstance().CollectAllSnapshots(pkt._actors);

    std::shared_lock lock(_mutex);
    for (auto const& [sessionId, weakSession] : _observers)
    {
        if (auto const session = weakSession.lock())
        {
            session->SendPacket(pkt);
            session->FlushPacketStream();
        }
    }
}
