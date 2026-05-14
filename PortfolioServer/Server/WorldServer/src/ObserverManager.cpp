#include "CorePch.h"
#include "ObserverManager.h"
#include "ObserverSession.h"
#include "ObserverPackets.h"
#include "ZoneManager.h"
#include "PlayerManager.h"
#include "Metrics.h"
#include "TaskDispatcher.h"

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

    auto const elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - _lastSampleTime).count();
    auto const safeElapsedMs = (elapsedMs <= 0) ? 1 : elapsedMs;

    auto const recvPps = static_cast<uint32_t>((currentRecvPacketCount - _lastRecvPacketCount) * 1000 / safeElapsedMs);
    auto const sendPps = static_cast<uint32_t>((currentSendPacketCount - _lastSendPacketCount) * 1000 / safeElapsedMs);

    _lastSampleTime = currentTime;
    _lastRecvPacketCount = currentRecvPacketCount;
    _lastSendPacketCount = currentSendPacketCount;

    W2OSnapshot pkt;
    pkt._ccu = static_cast<uint32_t>(PlayerManager::Singleton::GetConstInstance().GetCount());
    pkt._sendPps = sendPps;
    pkt._recvPps = recvPps;
    pkt._queueLen = static_cast<uint32_t>(TaskDispatcher::Singleton::GetConstInstance().GetTotalQueueSize());

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
