#include "CorePch.h"
#include "Player.h"

void Player::OnPacket(uint16_t const packetId, std::vector<uint8_t> const& payload)
{
    std::cout << "[Player:" << GetActorId() << "] OnPacket id=" << packetId
        << " size=" << payload.size() << std::endl;

    // TODO: 패킷 ID별로 deserialize → 해당 PlayerMsg 생성 → SendToPlayer 또는 OnMessage 직접 호출
}

void Player::OnMessage(PlayerMsg::Attacked const& msg)
{
    if (IsDead())
    {
        return;
    }

    _hp -= msg._damage;

    std::cout << "[Player:" << GetActorId() << "] Attacked by " << msg._attackerId
        << " dmg=" << msg._damage << " hp=" << _hp << std::endl;

    if (IsDead())
    {
        std::cout << "[Player:" << GetActorId() << "] DIED" << std::endl;
        // TODO: 사망 처리 (Zone broadcast, 부활 타이머 등)
    }
}

void Player::OnMessage(PlayerMsg::Healed const& msg)
{
    if (IsDead())
    {
        return;
    }

    _hp = std::min(_hp + msg._amount, MAX_HP);

    std::cout << "[Player:" << GetActorId() << "] Healed by " << msg._healerId
        << " amount=" << msg._amount << " hp=" << _hp << std::endl;
}
