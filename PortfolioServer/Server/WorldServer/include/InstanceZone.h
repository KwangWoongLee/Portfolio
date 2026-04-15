#pragma once
#include "IZone.h"

class InstanceZone final
    : public IZone
{
public:
    InstanceZone(ZoneId const zoneId, InstanceId const instanceId, uint32_t const dungeonId)
        : IZone(zoneId, EZoneType::Instance)
        , _instanceId(instanceId)
        , _dungeonId(dungeonId)
    {
    }

    InstanceId GetInstanceId() const { return _instanceId; }
    uint32_t GetDungeonId() const { return _dungeonId; }
    bool IsEmpty() const { return _players.empty(); }

    void Update() override;
    void Leave(EntityId const entityId) override;

private:
    InstanceId _instanceId{};
    uint32_t _dungeonId{};
};
