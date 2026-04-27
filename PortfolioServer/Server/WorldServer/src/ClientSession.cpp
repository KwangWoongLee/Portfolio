#include "CorePch.h"
#include "ClientSession.h"
#include "PlayerPost.h"
#include "ZoneManager.h"

void ClientSession::HandlePacket(uint16_t const packetId, void const* const payload, uint32_t const size)
{
    if (INVALID_ACTOR_ID == _actorId)
    {
        // TODO: 로그인 전 패킷 처리 (인증/캐릭터 선택 등)
        return;
    }

    auto const* const bytes = static_cast<uint8_t const*>(payload);
    std::vector<uint8_t> data(bytes, bytes + size);

    PostToPlayer(_actorId,
        [packetId, data = std::move(data)](Player& player)
        {
            player.OnPacket(packetId, data);
        });
}

void ClientSession::OnConnected()
{
    auto const self = IOCPSessionManager::Singleton::GetInstance().Find(GetSessionId());
    if (not self)
    {
        return;
    }

    auto const player = PlayerManager::Singleton::GetInstance().Create(self);
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
