#pragma once
#include "CorePch.h"
#include "Packet.h"
#include "WorldTypes.h"
#include "Grid.h"

class ClientSession;
class GameObject;

enum class EZoneType : uint8_t
{
    Field,
    Instance,
};

class IZone
{
public:
    explicit IZone(ZoneId const zoneId, EZoneType const zoneType)
        : _zoneId(zoneId), _zoneType(zoneType)
    {
    }

    virtual ~IZone() = default;

    ZoneId GetZoneId() const { return _zoneId; }
    EZoneType GetZoneType() const { return _zoneType; }
    size_t GetPlayerCount() const { return _players.size(); }

    virtual void Update() = 0;

    virtual bool Enter(std::shared_ptr<ClientSession> const& session);
    virtual void Leave(EntityId const entityId);

    void OnEntityMove(EntityId const entityId, Position const& oldPos, Position const& newPos);

    void Broadcast(Packet const& packet);
    void BroadcastInSight(Position const& center, Packet const& packet, EntityId const excludeEntityId = INVALID_ENTITY_ID);

protected:
    std::shared_ptr<ClientSession> FindPlayer(EntityId const entityId) const;

    ZoneId _zoneId{};
    EZoneType _zoneType{};

    Grid _grid;
    std::unordered_map<EntityId, std::shared_ptr<ClientSession>> _players;
};
