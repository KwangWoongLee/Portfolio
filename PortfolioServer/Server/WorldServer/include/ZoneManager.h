#pragma once
#include "CorePch.h"
#include "IZone.h"
#include "InstanceZone.h"

class ZoneManager final
{
public:
    using Singleton = Singleton<ZoneManager>;

    void CreateField(ZoneId const zoneId);
    InstanceId CreateInstanceDungeon(uint32_t const dungeonId);
    void DestroyInstanceDungeon(InstanceId const instanceId);

    std::shared_ptr<IZone> FindZone(ZoneId const zoneId) const;
    std::shared_ptr<InstanceZone> FindInstanceDungeon(InstanceId const instanceId) const;

    bool MovePlayer(std::shared_ptr<ClientSession> const& session, ZoneId const toZoneId);
    bool EnterInstanceDungeon(std::shared_ptr<ClientSession> const& session, InstanceId const instanceId);

    void CleanupEmptyInstances();

private:
    void StartZoneTick(std::shared_ptr<IZone> const& zone);
    ZoneId AllocateZoneId();

    std::unordered_map<ZoneId, std::shared_ptr<IZone>> _zones;
    std::unordered_map<InstanceId, std::shared_ptr<InstanceZone>> _instances;
    std::unordered_map<ZoneId, TimerId> _zoneTimers;

    ZoneId _nextZoneId{ 1 };
    InstanceId _nextInstanceId{ 1 };
};
