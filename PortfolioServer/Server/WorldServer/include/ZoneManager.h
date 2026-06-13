#pragma once
#include "CorePch.h"
#include "IZone.h"
#include "InstanceZone.h"

class ZoneTaskRunner;

class ZoneManager final
{
public:
    using Singleton = Singleton<ZoneManager>;

    void CreateField(ZoneId const zoneId);
    InstanceId CreateInstanceDungeon(uint32_t const dungeonId);

    void RequestMovePlayer(ActorId const actorId, ZoneId const toZoneId);
    bool RequestEnterInstanceDungeon(ActorId const actorId, InstanceId const instanceId);

    void CleanupEmptyInstances();

    void CollectAllSnapshots(std::vector<struct ActorSnapshot>& outSnapshots) const;

private:
    enum class EZoneLifecycleState : uint8_t
    {
        Open,
        Closing,
    };

    struct ZoneRegistryEntry final
    {
        std::shared_ptr<IZone> _zone;
        EZoneLifecycleState _state{ EZoneLifecycleState::Open };
        uint64_t _generation{};
        uint32_t _pendingEnterCount{};
    };

    struct ZoneEnterPermit final
    {
        ZoneId _zoneId{};
        uint64_t _generation{};
    };

    friend class PlayerTaskRunner;
    friend class ZoneTaskRunner;

    std::shared_ptr<IZone> FindZoneInternal(ZoneId const zoneId) const;
    std::shared_ptr<IZone> FindZoneByPermit(ZoneEnterPermit const& permit) const;
    std::shared_ptr<InstanceZone> FindInstanceDungeonInternal(InstanceId const instanceId) const;

    std::optional<ZoneEnterPermit> TryReserveZoneEnter(ZoneId const zoneId);
    void ReleaseZoneEnter(ZoneEnterPermit const& permit);
    bool TryMarkClosingIfDestroyable(InstanceId const instanceId, ZoneId const zoneId);
    void DestroyInstanceDungeon(InstanceId const instanceId);

    void StartZoneTick(std::shared_ptr<IZone> const& zone);
    ZoneId AllocateZoneId();
    uint64_t AllocateZoneGeneration();

    mutable std::shared_mutex _mutex;

    std::unordered_map<ZoneId, ZoneRegistryEntry> _zones;
    std::unordered_map<InstanceId, std::shared_ptr<InstanceZone>> _instances;
    std::unordered_map<ZoneId, TimerId> _zoneTimers;

    ZoneId _nextZoneId{ 1 };
    InstanceId _nextInstanceId{ 1 };
    uint64_t _nextZoneGeneration{ 1 };
};
