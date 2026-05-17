#include "CorePch.h"
#include "Player.h"
#include "PlayerPost.h"
#include "TimerManager.h"
#include "ZoneManager.h"
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

    auto const oldPos = _position;
    _position = { msg._x, msg._z };

    auto const zone = ZoneManager::Singleton::GetInstance().FindZone(_currentZoneId);
    if (not zone)
    {
        return;
    }

    zone->OnActorMove(GetActorId(), oldPos, _position);

    W2CMoveBroadcast packet;
    packet._actorId = GetActorId();
    packet._x = _position._x;
    packet._z = _position._z;
    zone->BroadcastInSight(_position, packet, GetActorId());
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

    auto const zone = ZoneManager::Singleton::GetInstance().FindZone(_currentZoneId);
    if (zone)
    {
        W2CHpUpdate hpPacket;
        hpPacket._actorId = GetActorId();
        hpPacket._hp = _hp;
        zone->BroadcastInSight(_position, hpPacket);
    }

    if (IsDead())
    {
        if (zone)
        {
            W2CDeath deathPacket;
            deathPacket._actorId = GetActorId();
            zone->BroadcastInSight(_position, deathPacket);
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

    auto const zone = ZoneManager::Singleton::GetInstance().FindZone(_currentZoneId);
    if (zone)
    {
        W2CHpUpdate hpPacket;
        hpPacket._actorId = GetActorId();
        hpPacket._hp = _hp;
        zone->BroadcastInSight(_position, hpPacket);
    }
}

void Player::OnMessage(PlayerMsg::Respawn const& msg)
{
    (void)msg;

    _hp = MAX_HP;

    auto const zone = ZoneManager::Singleton::GetInstance().FindZone(_currentZoneId);
    if (zone)
    {
        W2CHpUpdate hpPacket;
        hpPacket._actorId = GetActorId();
        hpPacket._hp = _hp;
        zone->BroadcastInSight(_position, hpPacket);
    }
}
