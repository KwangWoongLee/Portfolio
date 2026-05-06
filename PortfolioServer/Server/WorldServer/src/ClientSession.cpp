#include "CorePch.h"
#include "ClientSession.h"
#include "PlayerPost.h"
#include "ZoneManager.h"
#include "PacketId.h"
#include "WorldPackets.h"

void ClientSession::HandlePacket(uint16_t const packetId, void const* const payload, uint32_t const size)
{
    if (INVALID_ACTOR_ID == _actorId)
    {
        // TODO: 로그인 전 패킷 처리 (인증/캐릭터 선택 등)
        return;
    }

    switch (static_cast<EPacketId>(packetId))
    {
    case EPacketId::C2WMove:
        {
            C2WMove pkt;
            if (not pkt.ReadFromBytes(payload, size))
            {
                return;
            }
            SendToPlayer(_actorId, PlayerMsg::MoveRequest{ pkt._x, pkt._z });
        } break;

    case EPacketId::C2WAttack:
        {
            C2WAttack pkt;
            if (not pkt.ReadFromBytes(payload, size))
            {
                return;
            }
            SendToPlayer(_actorId, PlayerMsg::AttackRequest{ pkt._targetActorId });
        } break;

    default:
        break;
    }
}

void ClientSession::OnConnected()
{
    auto const session = std::static_pointer_cast<IOCPSession>(shared_from_this());
    auto const player = PlayerManager::Singleton::GetInstance().Create(session);
    _actorId = player->GetActorId();

    std::cout << "[ClientSession:" << GetSessionId() << "] Connected (actor=" << _actorId << ")" << std::endl;

    ZoneManager::Singleton::GetInstance().MovePlayer(player, 1);
}

void ClientSession::OnDisconnected()
{
    if (INVALID_ACTOR_ID == _actorId)
    {
        return;
    }

    auto const player = PlayerManager::Singleton::GetInstance().Find(_actorId);
    if (player)
    {
        auto const zoneId = player->GetCurrentZoneId();
        if (INVALID_ZONE_ID != zoneId)
        {
            if (auto const zone = ZoneManager::Singleton::GetInstance().FindZone(zoneId))
            {
                zone->Leave(_actorId);
            }
        }
    }

    PlayerManager::Singleton::GetInstance().Remove(_actorId);

    std::cout << "[ClientSession:" << GetSessionId() << " actor:" << _actorId << "] Disconnected" << std::endl;

    _actorId = INVALID_ACTOR_ID;
}
