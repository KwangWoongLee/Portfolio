#pragma once
#include "CorePch.h"
#include "Packet.h"
#include "WorldTypes.h"
#include "Grid.h"

class Player;

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

    virtual bool Enter(std::shared_ptr<Player> const& player);
    virtual void Leave(ActorId const actorId);

    void OnActorMove(ActorId const actorId, Position const& oldPos, Position const& newPos);

    void Broadcast(Packet const& packet);
    void BroadcastInSight(Position const& center, Packet const& packet, ActorId const excludeActorId = INVALID_ACTOR_ID);

protected:
    std::shared_ptr<Player> FindPlayer(ActorId const actorId) const;

    ZoneId _zoneId{};
    EZoneType _zoneType{};

    Grid _grid;
    std::unordered_map<ActorId, std::shared_ptr<Player>> _players;
};
