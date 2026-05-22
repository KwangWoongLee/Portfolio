#include "CorePch.h"
#include "Player.h"
#include "PlayerPost.h"
#include "TimerManager.h"
#include "ZoneManager.h"
#include "ZonePost.h"
#include "WorldPackets.h"

namespace
{
    auto constexpr DEFAULT_ATTACK_DAMAGE = 25;
    auto constexpr RESPAWN_DELAY = std::chrono::seconds(5);
}

void Player::OnPacket(uint16_t const packetId, std::vector<uint8_t> const& payload)
{
    std::cout << "[Player:" << GetActorId() << "] OnPacket id=" << packetId
        << " size=" << payload.size() << std::endl;
}

void Player::OnMessage(PlayerMsg::MoveRequest const& msg)
{
    if (IsDead())
    {
        return;
    }

    auto const oldPosition = _position;
    _position = { msg._x, msg._z };

    if (INVALID_ZONE_ID == _currentZoneId)
    {
        return;
    }

    SendToZone(_currentZoneId, ZoneMsg::ActorMoved{ GetActorId(), oldPosition, _position });
}

void Player::OnMessage(PlayerMsg::AttackRequest const& msg)
{
    if (IsDead())
    {
        return;
    }

    SendToPlayer(msg._targetActorId, PlayerMsg::Attacked{ GetActorId(), DEFAULT_ATTACK_DAMAGE });
}

void Player::OnMessage(PlayerMsg::Attacked const& msg)
{
    if (IsDead())
    {
        return;
    }

    _hp -= msg._damage;

    if (INVALID_ZONE_ID != _currentZoneId)
    {
        auto hpPacket = std::make_shared<W2CHpUpdate>();
        hpPacket->_actorId = GetActorId();
        hpPacket->_hp = _hp;
        SendToZone(_currentZoneId, ZoneMsg::BroadcastInSightRequest{ hpPacket, _position, INVALID_ACTOR_ID });
    }

    if (IsDead())
    {
        if (INVALID_ZONE_ID != _currentZoneId)
        {
            auto deathPacket = std::make_shared<W2CDeath>();
            deathPacket->_actorId = GetActorId();
            SendToZone(_currentZoneId, ZoneMsg::BroadcastInSightRequest{ deathPacket, _position, INVALID_ACTOR_ID });
        }

        auto const selfActorId = GetActorId();
        TimerManager::Singleton::GetInstance().AddTimer(
            std::chrono::duration_cast<std::chrono::milliseconds>(RESPAWN_DELAY),
            static_cast<int64_t>(selfActorId),
            [selfActorId]()
            {
                SendToPlayer(selfActorId, PlayerMsg::Respawn{});
            });
    }
}

void Player::OnMessage(PlayerMsg::Healed const& msg)
{
    if (IsDead())
    {
        return;
    }

    _hp += msg._amount;
    if (MAX_HP < _hp)
    {
        _hp = MAX_HP;
    }

    if (INVALID_ZONE_ID != _currentZoneId)
    {
        auto hpPacket = std::make_shared<W2CHpUpdate>();
        hpPacket->_actorId = GetActorId();
        hpPacket->_hp = _hp;
        SendToZone(_currentZoneId, ZoneMsg::BroadcastInSightRequest{ hpPacket, _position, INVALID_ACTOR_ID });
    }
}

void Player::OnMessage(PlayerMsg::Respawn const& msg)
{
    (void)msg;

    _hp = MAX_HP;

    if (INVALID_ZONE_ID != _currentZoneId)
    {
        auto hpPacket = std::make_shared<W2CHpUpdate>();
        hpPacket->_actorId = GetActorId();
        hpPacket->_hp = _hp;
        SendToZone(_currentZoneId, ZoneMsg::BroadcastInSightRequest{ hpPacket, _position, INVALID_ACTOR_ID });
    }
}
