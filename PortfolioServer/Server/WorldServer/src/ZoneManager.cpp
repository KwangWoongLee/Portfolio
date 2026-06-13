#include "CorePch.h"
#include "ZoneManager.h"
#include "FieldZone.h"
#include "InstanceZone.h"
#include "PlayerTaskRunner.h"
#include "TimerManager.h"
#include "ZonePost.h"

void ZoneManager::CreateField(ZoneId const zoneId)
{
    std::shared_ptr<FieldZone> field;

    {
        std::unique_lock lock(_mutex);
        if (_zones.contains(zoneId))
        {
            return;
        }

        field = std::make_shared<FieldZone>(zoneId);
        ZoneRegistryEntry entry;
        entry._zone = field;
        entry._generation = AllocateZoneGeneration();
        _zones.emplace(zoneId, std::move(entry));

        if (_nextZoneId <= zoneId)
        {
            _nextZoneId = zoneId + 1;
        }
    }

    StartZoneTick(field);

    std::cout << "[ZoneManager] Field zone " << zoneId << " created" << std::endl;
}

InstanceId ZoneManager::CreateInstanceDungeon(uint32_t const dungeonId)
{
    InstanceId instanceId{};
    ZoneId zoneId{};
    std::shared_ptr<InstanceZone> instance;

    {
        std::unique_lock lock(_mutex);
        instanceId = _nextInstanceId++;
        zoneId = AllocateZoneId();

        instance = std::make_shared<InstanceZone>(zoneId, instanceId, dungeonId);
        ZoneRegistryEntry entry;
        entry._zone = instance;
        entry._generation = AllocateZoneGeneration();
        _zones.emplace(zoneId, std::move(entry));
        _instances.emplace(instanceId, instance);
    }

    StartZoneTick(instance);

    std::cout << "[ZoneManager] InstanceDungeon " << instanceId
        << " (dungeon=" << dungeonId << ", zone=" << zoneId << ") created" << std::endl;

    return instanceId;
}

void ZoneManager::DestroyInstanceDungeon(InstanceId const instanceId)
{
    ZoneId zoneId{};
    TimerId timerId{};

    {
        std::unique_lock lock(_mutex);

        auto const iter = _instances.find(instanceId);
        if (_instances.end() == iter)
        {
            return;
        }

        zoneId = iter->second->GetZoneId();

        if (auto const timerIter = _zoneTimers.find(zoneId); _zoneTimers.end() != timerIter)
        {
            timerId = timerIter->second;
            _zoneTimers.erase(timerIter);
        }

        _zones.erase(zoneId);
        _instances.erase(iter);
    }

    if (0 != timerId)
    {
        TimerManager::Singleton::GetInstance().CancelTimer(timerId);
    }

    std::cout << "[ZoneManager] InstanceDungeon " << instanceId << " destroyed" << std::endl;
}

std::shared_ptr<IZone> ZoneManager::FindZoneInternal(ZoneId const zoneId) const
{
    std::shared_lock lock(_mutex);

    auto const iter = _zones.find(zoneId);
    if (_zones.end() == iter)
    {
        return nullptr;
    }
    return iter->second._zone;
}

std::shared_ptr<IZone> ZoneManager::FindZoneByPermit(ZoneEnterPermit const& permit) const
{
    std::shared_lock lock(_mutex);

    auto const iter = _zones.find(permit._zoneId);
    if (_zones.end() == iter || iter->second._generation != permit._generation)
    {
        return nullptr;
    }

    return iter->second._zone;
}

std::shared_ptr<InstanceZone> ZoneManager::FindInstanceDungeonInternal(InstanceId const instanceId) const
{
    std::shared_lock lock(_mutex);

    auto const iter = _instances.find(instanceId);
    if (_instances.end() == iter)
    {
        return nullptr;
    }
    return iter->second;
}

std::optional<ZoneManager::ZoneEnterPermit> ZoneManager::TryReserveZoneEnter(ZoneId const zoneId)
{
    std::unique_lock lock(_mutex);

    auto iter = _zones.find(zoneId);
    if (_zones.end() == iter || EZoneLifecycleState::Open != iter->second._state)
    {
        return std::nullopt;
    }

    ++iter->second._pendingEnterCount;
    return ZoneEnterPermit{ zoneId, iter->second._generation };
}

void ZoneManager::ReleaseZoneEnter(ZoneEnterPermit const& permit)
{
    std::unique_lock lock(_mutex);

    auto iter = _zones.find(permit._zoneId);
    if (_zones.end() == iter || iter->second._generation != permit._generation)
    {
        return;
    }

    if (0 < iter->second._pendingEnterCount)
    {
        --iter->second._pendingEnterCount;
    }
}

bool ZoneManager::TryMarkClosingIfDestroyable(InstanceId const instanceId, ZoneId const zoneId)
{
    std::unique_lock lock(_mutex);

    auto const instanceIter = _instances.find(instanceId);
    if (_instances.end() == instanceIter || instanceIter->second->GetZoneId() != zoneId)
    {
        return false;
    }

    auto zoneIter = _zones.find(zoneId);
    if (_zones.end() == zoneIter || 0 != zoneIter->second._pendingEnterCount)
    {
        return false;
    }

    zoneIter->second._state = EZoneLifecycleState::Closing;
    return true;
}

void ZoneManager::RequestMovePlayer(ActorId const actorId, ZoneId const toZoneId)
{
    PlayerTaskRunner::PostMoveToZone(actorId, toZoneId);
}

bool ZoneManager::RequestEnterInstanceDungeon(ActorId const actorId, InstanceId const instanceId)
{
    auto const instance = FindInstanceDungeonInternal(instanceId);
    if (not instance)
    {
        return false;
    }

    RequestMovePlayer(actorId, instance->GetZoneId());
    return true;
}

void ZoneManager::CollectAllSnapshots(std::vector<ActorSnapshot>& outSnapshots) const
{
    std::vector<std::shared_ptr<IZone>> zones;

    {
        std::shared_lock lock(_mutex);
        zones.reserve(_zones.size());

        for (auto const& [id, zone] : _zones)
        {
            zones.emplace_back(zone._zone);
        }
    }

    for (auto const& zone : zones)
    {
        if (auto const cached = zone->GetCachedSnapshot())
        {
            outSnapshots.insert(outSnapshots.end(), cached->begin(), cached->end());
        }
    }
}

void ZoneManager::CleanupEmptyInstances()
{
    std::vector<std::pair<InstanceId, ZoneId>> candidates;

    {
        std::shared_lock lock(_mutex);
        candidates.reserve(_instances.size());

        for (auto const& [instanceId, instance] : _instances)
        {
            candidates.emplace_back(instanceId, instance->GetZoneId());
        }
    }

    for (auto const [instanceId, zoneId] : candidates)
    {
        ZoneTaskRunner::PostDestroyIfEmpty(instanceId, zoneId);
    }
}

void ZoneManager::StartZoneTick(std::shared_ptr<IZone> const& zone)
{
    auto const zoneId = zone->GetZoneId();

    std::weak_ptr<IZone> weakZone = zone;
    auto const timerId = TimerManager::Singleton::GetInstance().AddRepeatTimer(
        std::chrono::milliseconds(50),
        static_cast<int64_t>(zoneId),
        [zoneId, weakZone]()
        {
            if (weakZone.expired())
            {
                return;
            }

            ZoneTaskRunner::PostUpdate(zoneId);
        }
    );

    bool registered{ false };

    {
        std::unique_lock lock(_mutex);
        auto const iter = _zones.find(zoneId);
        if (_zones.end() != iter && iter->second._zone == zone)
        {
            auto const [timerIter, inserted] = _zoneTimers.emplace(zoneId, timerId);
            (void)timerIter;
            registered = inserted;
        }
    }

    if (not registered)
    {
        TimerManager::Singleton::GetInstance().CancelTimer(timerId);
    }
}

ZoneId ZoneManager::AllocateZoneId()
{
    return _nextZoneId++;
}

uint64_t ZoneManager::AllocateZoneGeneration()
{
    return _nextZoneGeneration++;
}
