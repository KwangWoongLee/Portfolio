#include "CorePch.h"
#include "ClientSession.h"
#include "ZoneManager.h"

void ClientSession::HandlePacket(uint16_t const packetId, void const* const payload, uint32_t const size)
{
    std::cout << "[ClientSession:" << GetSessionId() << " entity:" << _entityId << "] "
        << "PacketId=" << packetId << " Size=" << size << std::endl;

    // TODO: 패킷 ID별 핸들러 분기
}

void ClientSession::OnConnected()
{
    _entityId = EntityIdGenerator::Generate();

    std::cout << "[ClientSession:" << GetSessionId() << "] Connected (entity=" << _entityId << ")" << std::endl;

    auto self = std::dynamic_pointer_cast<ClientSession>(
        IOCPSessionManager::Singleton::GetInstance().Find(GetSessionId()));

    if (self)
    {
        ZoneManager::Singleton::GetInstance().MovePlayer(self, 1);
    }
}

void ClientSession::OnDisconnected()
{
    auto const zoneId = _currentZoneId;
    if (INVALID_ZONE_ID != zoneId)
    {
        if (auto const zone = ZoneManager::Singleton::GetInstance().FindZone(zoneId))
        {
            zone->Leave(_entityId);
        }
    }

    std::cout << "[ClientSession:" << GetSessionId() << " entity:" << _entityId << "] Disconnected" << std::endl;

    _entityId = INVALID_ENTITY_ID;
}
