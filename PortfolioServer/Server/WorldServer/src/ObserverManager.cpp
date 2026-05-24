#include "CorePch.h"
#include "ObserverManager.h"
#include "ObserverSession.h"
#include "ObserverPackets.h"
#include "ZoneManager.h"
#include "PlayerManager.h"
#include "Metrics.h"
#include "MetricsLogger.h"
#include "TaskDispatcher.h"

namespace
{
    auto constexpr CSV_LOG_INTERVAL = std::chrono::milliseconds(10000);

    uint64_t FileTimeToUInt64(FILETIME const& ft)
    {
        return (static_cast<uint64_t>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
    }

    uint64_t GetCurrentProcessCpuTime100ns()
    {
        FILETIME creationTime{};
        FILETIME exitTime{};
        FILETIME kernelTime{};
        FILETIME userTime{};
        ::GetProcessTimes(::GetCurrentProcess(), &creationTime, &exitTime, &kernelTime, &userTime);
        return FileTimeToUInt64(kernelTime) + FileTimeToUInt64(userTime);
    }

    uint32_t GetProcessorCount()
    {
        SYSTEM_INFO sysInfo{};
        ::GetSystemInfo(&sysInfo);
        return sysInfo.dwNumberOfProcessors;
    }
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
    auto const currentProcessCpuTime100ns = GetCurrentProcessCpuTime100ns();

    auto const elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - _lastSampleTime).count();
    auto const safeElapsedMs = (elapsedMs <= 0) ? 1 : elapsedMs;

    auto const recvPps = static_cast<uint32_t>((currentRecvPacketCount - _lastRecvPacketCount) * 1000 / safeElapsedMs);
    auto const sendPps = static_cast<uint32_t>((currentSendPacketCount - _lastSendPacketCount) * 1000 / safeElapsedMs);
    auto const recvBytesPerSecond = static_cast<uint32_t>((currentRecvByteCount - _lastRecvByteCount) * 1000 / safeElapsedMs);
    auto const sendBytesPerSecond = static_cast<uint32_t>((currentSendByteCount - _lastSendByteCount) * 1000 / safeElapsedMs);

    auto const cpuElapsed100ns = currentProcessCpuTime100ns - _lastProcessCpuTime100ns;
    auto const realElapsed100ns = static_cast<uint64_t>(safeElapsedMs) * 10000;
    static auto const processorCount = GetProcessorCount();
    auto const cpuPercent = (0 != _lastProcessCpuTime100ns && 0 != realElapsed100ns)
        ? static_cast<uint32_t>((cpuElapsed100ns * 100) / (realElapsed100ns * processorCount))
        : 0;

    _lastSampleTime = currentTime;
    _lastRecvPacketCount = currentRecvPacketCount;
    _lastSendPacketCount = currentSendPacketCount;
    _lastRecvByteCount = currentRecvByteCount;
    _lastSendByteCount = currentSendByteCount;
    _lastProcessCpuTime100ns = currentProcessCpuTime100ns;

    W2OSnapshot pkt;
    pkt._ccu = static_cast<uint32_t>(PlayerManager::Singleton::GetConstInstance().GetCount());
    pkt._sendPps = sendPps;
    pkt._recvPps = recvPps;
    pkt._sendBytesPerSecond = sendBytesPerSecond;
    pkt._recvBytesPerSecond = recvBytesPerSecond;
    pkt._cpuPercent = cpuPercent;
    pkt._queueLen = static_cast<uint32_t>(TaskDispatcher::Singleton::GetConstInstance().GetTotalQueueSize());

    ZoneManager::Singleton::GetConstInstance().CollectAllSnapshots(pkt._actors);

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
        sample._taskQueueSize = pkt._queueLen;
        MetricsLogger::Singleton::GetInstance().Enqueue(std::move(sample));

        _lastCsvLogTime = currentTime;
    }

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
