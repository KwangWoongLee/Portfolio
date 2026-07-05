#pragma once
#include "GameObject.h"
#include "IOCPSession.h"
#include "CharacterRepository.h"
#include "PlayerMessages.h"

class Player final
    : public GameObject
{
public:
    Player(
        ActorId const actorId,
        CharacterId const characterId,
        int64_t const gold,
        std::shared_ptr<IOCPSession> const& session,
        std::shared_ptr<ICharacterRepository> characterRepository)
        : GameObject(actorId)
        , _characterId(characterId)
        , _session(session)
        , _sessionId(session->GetSessionId())
        , _characterRepository(std::move(characterRepository))
        , _hp(MAX_HP)
        , _gold(gold)
    {
    }

    SessionId GetSessionId() const { return _sessionId; }
    void OnPacket(uint16_t const packetId, std::vector<uint8_t> const& payload);

private:
    friend class PlayerTaskRunner;

    std::shared_ptr<IOCPSession> GetSession() const { return _session.lock(); }

    ZoneId GetCurrentZoneId() const { return _currentZoneId; }
    void SetCurrentZoneId(ZoneId const zoneId) { _currentZoneId = zoneId; }

    ZoneId GetPendingZoneId() const { return _pendingZoneId; }
    void SetPendingZoneId(ZoneId const zoneId) { _pendingZoneId = zoneId; }

    int32_t GetHp() const { return _hp; }
    bool IsDead() const { return _hp <= 0; }

    void OnMessage(PlayerMsg::MoveRequest const& msg);
    void OnMessage(PlayerMsg::AttackRequest const& msg);
    void OnMessage(PlayerMsg::Attacked const& msg);
    void OnMessage(PlayerMsg::Healed const& msg);
    void OnMessage(PlayerMsg::Respawn const& msg);
    void OnMessage(PlayerMsg::SiegeDeclarationPaymentRequested const& msg);
    void OnMessage(PlayerMsg::SiegeDeclarationRefundRequested const& msg);

    static int32_t constexpr MAX_HP = 5000;

    CharacterId const _characterId;
    std::weak_ptr<IOCPSession> const _session;
    SessionId const _sessionId;
    std::shared_ptr<ICharacterRepository> const _characterRepository;
    ZoneId _currentZoneId{ INVALID_ZONE_ID };
    ZoneId _pendingZoneId{ INVALID_ZONE_ID };
    int32_t _hp{};
    int64_t _gold{};
    std::unordered_set<SiegeDeclarationId> _paidSiegeDeclarationIds;
};
