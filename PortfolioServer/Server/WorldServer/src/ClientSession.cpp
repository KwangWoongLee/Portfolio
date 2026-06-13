#include "CorePch.h"
#include "ClientSession.h"
#include "PlayerPost.h"
#include "ZoneManager.h"
#include "ZonePost.h"
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
    _actorId = PlayerManager::Singleton::GetInstance().Create(session);

    std::cout << "[ClientSession:" << GetSessionId() << "] Connected (actor=" << _actorId << ")" << std::endl;

    // RequestMovePlayer가 enter permit을 예약하고, Zone enter 성공 후 Player current zone을 확정.
    ZoneManager::Singleton::GetInstance().RequestMovePlayer(_actorId, 1);
}

void ClientSession::OnDisconnected()
{
    if (INVALID_ACTOR_ID == _actorId)
    {
        return;
    }

    PlayerTaskRunner::PostDisconnect(_actorId);

    std::cout << "[ClientSession:" << GetSessionId() << " actor:" << _actorId << "] Disconnected" << std::endl;

    _actorId = INVALID_ACTOR_ID;
}
