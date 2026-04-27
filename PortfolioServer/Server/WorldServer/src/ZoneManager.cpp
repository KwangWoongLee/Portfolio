#include "CorePch.h"
#include "ZoneManager.h"
#include "FieldZone.h"
#include "InstanceZone.h"
#include "Player.h"
#include "TimerManager.h"

void ZoneManager::CreateField(ZoneId const zoneId)
{
    if (_zones.contains(zoneId))
    {
        return;
    }

    auto field = std::make_shared<FieldZone>(zoneId);
    _zones.emplace(zoneId, field);

    StartZoneTick(field);

    std::cout << "[ZoneManager] Field zone " << zoneId << " created" << std::endl;
}

InstanceId ZoneManager::CreateInstanceDungeon(uint32_t const dungeonId)
{
    auto const instanceId = _nextInstanceId++;
    auto const zoneId = AllocateZoneId();

    auto instance = std::make_shared<InstanceZone>(zoneId, instanceId, dungeonId);
    _zones.emplace(zoneId, instance);
    _instances.emplace(instanceId, instance);

    StartZoneTick(instance);

    std::cout << "[ZoneManager] InstanceDungeon " << instanceId
        << " (dungeon=" << dungeonId << ", zone=" << zoneId << ") created" << std::endl;

    return instanceId;
}

void ZoneManager::DestroyInstanceDungeon(InstanceId const instanceId)
{
    auto const iter = _instances.find(instanceId);
    if (_instances.end() == iter)
    {
        return;
    }

    auto const zoneId = iter->second->GetZoneId();

    if (auto const timerIter = _zoneTimers.find(zoneId); _zoneTimers.end() != timerIter)
    {
        TimerManager::Singleton::GetInstance().CancelTimer(timerIter->second);
        _zoneTimers.erase(timerIter);
    }

    std::cout << "[ZoneManager] InstanceDungeon " << instanceId << " destroyed" << std::endl;

    _zones.erase(zoneId);
    _instances.erase(iter);
}

std::shared_ptr<IZone> ZoneManager::FindZone(ZoneId const zoneId) const
{
    auto const iter = _zones.find(zoneId);
    if (_zones.end() == iter)
    {
        return nullptr;
    }
    return iter->second;
}

std::shared_ptr<InstanceZone> ZoneManager::FindInstanceDungeon(InstanceId const instanceId) const
{
    auto const iter = _instances.find(instanceId);
    if (_instances.end() == iter)
    {
        return nullptr;
    }
    return iter->second;
}

bool ZoneManager::MovePlayer(std::shared_ptr<Player> const& player, ZoneId const toZoneId)
{
    auto const toZone = FindZone(toZoneId);
    if (not toZone)
    {
        return false;
    }

    auto const fromZoneId = player->GetCurrentZoneId();
    if (INVALID_ZONE_ID != fromZoneId)
    {
        if (auto const fromZone = FindZone(fromZoneId))
        {
            fromZone->Leave(player->GetActorId());
        }
    }

    return toZone->Enter(player);
}

bool ZoneManager::EnterInstanceDungeon(std::shared_ptr<Player> const& player, InstanceId const instanceId)
{
    auto const instance = FindInstanceDungeon(instanceId);
    if (not instance)
    {
        return false;
    }

    return MovePlayer(player, instance->GetZoneId());
}

void ZoneManager::CleanupEmptyInstances()
{
    std::vector<InstanceId> toRemove;

    for (auto const& [id, instance] : _instances)
    {
        if (instance->IsEmpty())
        {
            toRemove.push_back(id);
        }
    }

    for (auto const instanceId : toRemove)
    {
        DestroyInstanceDungeon(instanceId);
    }
}

void ZoneManager::StartZoneTick(std::shared_ptr<IZone> const& zone)
{
    auto const zoneId = zone->GetZoneId();

    std::weak_ptr<IZone> weakZone = zone;
    auto const timerId = TimerManager::Singleton::GetInstance().AddRepeatTimer(
        std::chrono::milliseconds(50),
        static_cast<int64_t>(zoneId),
        [weakZone]()
        {
            if (auto const zone = weakZone.lock())
            {
                zone->Update();
            }
        }
    );

    _zoneTimers.emplace(zoneId, timerId);
}

ZoneId ZoneManager::AllocateZoneId()
{
    return _nextZoneId++;
}
