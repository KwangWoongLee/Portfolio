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

private:
    friend class ZoneTaskRunner;

    // Owning zone worker only. Lifecycle decisions must not use cached counts.
    bool IsEmpty() const { return _actors.empty(); }

    void Update() override;
    void Leave(ActorId const entityId) override;

private:
    InstanceId _instanceId{};
    uint32_t _dungeonId{};
};
