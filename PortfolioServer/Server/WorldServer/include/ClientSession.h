#pragma once
#include "CorePch.h"
#include "IOCPSession.h"
#include "WorldTypes.h"

class ClientSession final
    : public IOCPSession
{
public:
    void HandlePacket(uint16_t const packetId, void const* const payload, uint32_t const size) override;

    EntityId GetEntityId() const { return _entityId; }
    void SetEntityId(EntityId const entityId) { _entityId = entityId; }

    ZoneId GetCurrentZoneId() const { return _currentZoneId; }
    void SetCurrentZoneId(ZoneId const zoneId) { _currentZoneId = zoneId; }

    Position const& GetPosition() const { return _position; }
    void SetPosition(Position const& pos) { _position = pos; }

private:
    void OnConnected() override;
    void OnDisconnected() override;

    EntityId _entityId{ INVALID_ENTITY_ID };
    ZoneId _currentZoneId{ INVALID_ZONE_ID };
    Position _position{};
};
