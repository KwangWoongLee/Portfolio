#pragma once
#include "GameObject.h"
#include "IOCPSession.h"
#include "PlayerMessages.h"

class Player final
    : public GameObject
{
public:
    Player(ActorId const actorId, std::shared_ptr<IOCPSession> const& session)
        : GameObject(actorId)
        , _session(session)
        , _sessionId(session->GetSessionId())
        , _hp(MAX_HP)
    {
    }

    SessionId GetSessionId() const { return _sessionId; }
    std::shared_ptr<IOCPSession> GetSession() const { return _session.lock(); }

    ZoneId GetCurrentZoneId() const { return _currentZoneId; }
    void SetCurrentZoneId(ZoneId const zoneId) { _currentZoneId = zoneId; }

    int32_t GetHp() const { return _hp; }
    bool IsDead() const { return _hp <= 0; }

    void OnPacket(uint16_t const packetId, std::vector<uint8_t> const& payload);

    void OnMessage(PlayerMsg::Attacked const& msg);
    void OnMessage(PlayerMsg::Healed const& msg);

private:
    static int32_t constexpr MAX_HP = 100;

    std::weak_ptr<IOCPSession> const _session;
    SessionId const _sessionId;
    ZoneId _currentZoneId{ INVALID_ZONE_ID };
    int32_t _hp{};
};
