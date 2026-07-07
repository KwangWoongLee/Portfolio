#include "CorePch.h"
#include "ClientSession.h"
#include "DbCompletionTarget.h"
#include "PlayerManager.h"
#include "PlayerPost.h"
#include "WorldTypes.h"
#include "ZoneManager.h"
#include "ZonePost.h"
#include "PacketId.h"
#include "WorldPackets.h"

namespace
{
    auto constexpr LOAD_TEST_FIELD_ZONE_IDS = std::array<ZoneId, 4>{ 1, 2, 3, 4 };
    std::atomic<uint64_t> g_nextLoadTestZoneIndex{ 0 };

    ZoneId SelectInitialFieldZoneForLoadTest()
    {
        auto const index = static_cast<size_t>(
            g_nextLoadTestZoneIndex.fetch_add(1, std::memory_order_relaxed)
            % LOAD_TEST_FIELD_ZONE_IDS.size());

        return LOAD_TEST_FIELD_ZONE_IDS[index];
    }
}

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
        DbCompletionTarget::NetworkSession(GetSessionId()),
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

            ZoneManager::Singleton::GetInstance().RequestMovePlayer(actorId, SelectInitialFieldZoneForLoadTest());
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
