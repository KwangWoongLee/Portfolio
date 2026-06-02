#pragma once
#include "CorePch.h"
#include "Packet.h"
#include "WorldTypes.h"
#include "Grid.h"
#include "WorldPackets.h"
#include "ZoneMessages.h"

class IOCPSession;

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
    size_t GetPlayerCount() const { return _actors.size(); }

    virtual void Update() = 0;

    virtual bool Enter(ZoneMsg::PlayerEntered const& msg);
    virtual void Leave(ActorId const actorId);

    void OnActorMove(ActorId const actorId, Position const& oldPos, Position const& newPos);

    void OnMessage(ZoneMsg::ActorMoved const& msg);
    void OnMessage(ZoneMsg::PlayerEntered const& msg);
    void OnMessage(ZoneMsg::PlayerLeft const& msg);
    void OnMessage(ZoneMsg::BroadcastInSightRequest const& msg);
    void OnMessage(ZoneMsg::HpChanged const& msg);
    void OnMessage(ZoneMsg::ActorDied const& msg);

    void GetSightSnapshot(ActorId const selfActorId, std::vector<ActorSnapshot>& outSnapshots) const;
    void CollectAllSnapshots(std::vector<ActorSnapshot>& outSnapshots) const;

    // Zone tick에서 갱신, 외부 thread는 atomic load로 race-free read.
    void UpdateSnapshotCache();
    std::shared_ptr<std::vector<ActorSnapshot> const> GetCachedSnapshot() const { return _cachedSnapshot.load(std::memory_order_acquire); }

    void Broadcast(Packet const& packet);
    void BroadcastInSight(Position const& center, Packet const& packet, ActorId const excludeActorId = INVALID_ACTOR_ID);

protected:
    struct ZoneActorState final
    {
        ActorId _actorId{};
        std::weak_ptr<IOCPSession> _session;
        Position _position;
        int32_t _hp{};
    };

    ZoneActorState* FindActor(ActorId const actorId);
    ZoneActorState const* FindActor(ActorId const actorId) const;

    ZoneId _zoneId{};
    EZoneType _zoneType{};

    Grid _grid;
    std::unordered_map<ActorId, ZoneActorState> _actors;

    std::atomic<std::shared_ptr<std::vector<ActorSnapshot> const>> _cachedSnapshot;
};
