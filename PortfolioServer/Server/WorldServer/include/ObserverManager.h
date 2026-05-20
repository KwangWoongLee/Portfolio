#pragma once
#include "CorePch.h"

class ObserverSession;

class ObserverManager final
{
public:
    using Singleton = Singleton<ObserverManager>;
    using Clock = std::chrono::steady_clock;

    void Register(std::shared_ptr<ObserverSession> const& session);
    void Unregister(SessionId const sessionId);

    void PushSnapshot();

private:
    std::shared_mutex _mutex;
    std::unordered_map<SessionId, std::weak_ptr<ObserverSession>> _observers;

    Clock::time_point _lastSampleTime{ Clock::now() };
    Clock::time_point _lastCsvLogTime{ Clock::now() };
    uint64_t _lastRecvPacketCount{};
    uint64_t _lastSendPacketCount{};
};
