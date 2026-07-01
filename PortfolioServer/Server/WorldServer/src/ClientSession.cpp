#include "CorePch.h"
#include "ClientSession.h"
#include "PlayerManager.h"
#include "PlayerPost.h"
#include "WorldTypes.h"
#include "ZoneManager.h"
#include "ZonePost.h"
#include "PacketId.h"
#include "WorldPackets.h"

void ClientSession::HandlePacket(uint16_t const packetId, void const* const payload, uint32_t const size)
{
    auto const actorId = GetActorId();
    if (INVALID_ACTOR_ID == actorId)
    {
        // TODO: handle packets that arrive before character binding is completed.
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
            SendToPlayer(actorId, PlayerMsg::MoveRequest{ pkt._x, pkt._z });
        } break;

    case EPacketId::C2WAttack:
        {
            C2WAttack pkt;
            if (not pkt.ReadFromBytes(payload, size))
            {
                return;
            }
            SendToPlayer(actorId, PlayerMsg::AttackRequest{ pkt._targetActorId });
        } break;

    default:
        break;
    }
}

void ClientSession::OnConnected()
{
    SetActorId(INVALID_ACTOR_ID);
    _disconnected.store(false, std::memory_order_relaxed);

    auto const self = std::static_pointer_cast<ClientSession>(shared_from_this());
    auto const enqueued = PlayerManager::Singleton::GetInstance().CreateCharacterAsync(
        DEFAULT_WORLD_ID,
        [weakSelf = std::weak_ptr<ClientSession>{ self }](CharacterLoadResult result)
        {
            auto self = weakSelf.lock();
            if (not self || self->_disconnected.load(std::memory_order_relaxed))
            {
                return;
            }

            if (not result.Succeeded() || not result._profile.IsValid())
            {
                std::cerr << "[CharacterRepository] CreateCharacter failed: "
                    << result._message << std::endl;
                self->Disconnect(EDisconnectReason::InvalidOperation);
                return;
            }

            auto const session = std::static_pointer_cast<IOCPSession>(self);
            auto const actorId = PlayerManager::Singleton::GetInstance().CreateLoaded(
                session,
                result._profile);
            if (INVALID_ACTOR_ID == actorId)
            {
                self->Disconnect(EDisconnectReason::InvalidOperation);
                return;
            }

            self->SetActorId(actorId);
            if (self->_disconnected.load(std::memory_order_relaxed))
            {
                self->SetActorId(INVALID_ACTOR_ID);
                PlayerTaskRunner::PostDisconnect(actorId);
                return;
            }

            std::cout << "[ClientSession:" << self->GetSessionId()
                << "] Connected (actor=" << actorId << ")" << std::endl;

            // RequestMovePlayer reserves enter permission, then Zone enter sets Player current zone.
            ZoneManager::Singleton::GetInstance().RequestMovePlayer(actorId, 1);
        });

    if (not enqueued)
    {
        Disconnect(EDisconnectReason::InvalidOperation);
    }
}

void ClientSession::OnDisconnected()
{
    _disconnected.store(true, std::memory_order_relaxed);

    auto const actorId = GetActorId();
    if (INVALID_ACTOR_ID == actorId)
    {
        return;
    }

    SetActorId(INVALID_ACTOR_ID);
    PlayerTaskRunner::PostDisconnect(actorId);

    std::cout << "[ClientSession:" << GetSessionId() << " actor:" << actorId << "] Disconnected" << std::endl;
}
