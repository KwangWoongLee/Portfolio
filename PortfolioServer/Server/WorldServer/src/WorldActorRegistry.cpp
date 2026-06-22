#include "CorePch.h"
#include "WorldActorRegistry.h"

bool WorldActorRegistry::Create(WorldId const worldId)
{
    if (worldId == INVALID_WORLD_ID)
    {
        return false;
    }

    std::unique_lock lock(_mutex);
    if (_worldActors.contains(worldId))
    {
        return false;
    }

    _worldActors.emplace(worldId, std::make_shared<WorldActor>(worldId));
    return true;
}

bool WorldActorRegistry::Remove(WorldId const worldId)
{
    std::unique_lock lock(_mutex);
    return 0 != _worldActors.erase(worldId);
}

size_t WorldActorRegistry::GetCount() const
{
    std::shared_lock lock(_mutex);
    return _worldActors.size();
}

std::optional<GuildSnapshot> WorldActorRegistry::GetGuildSnapshot(
    WorldId const worldId,
    GuildId const guildId) const
{
    auto const worldActor = FindInternal(worldId);
    if (not worldActor)
    {
        return std::nullopt;
    }

    return worldActor->GetGuildSnapshot(guildId);
}

std::optional<GuildSnapshot> WorldActorRegistry::GetGuildSnapshotByMember(
    WorldId const worldId,
    ActorId const actorId) const
{
    auto const worldActor = FindInternal(worldId);
    if (not worldActor)
    {
        return std::nullopt;
    }

    return worldActor->GetGuildSnapshotByMember(actorId);
}

std::optional<SiegeWarSnapshot> WorldActorRegistry::GetSiegeWarSnapshot(
    WorldId const worldId,
    SiegeWarId const siegeWarId) const
{
    auto const worldActor = FindInternal(worldId);
    if (not worldActor)
    {
        return std::nullopt;
    }

    return worldActor->GetSiegeWarSnapshot(siegeWarId);
}

std::shared_ptr<WorldActor> WorldActorRegistry::FindInternal(WorldId const worldId) const
{
    std::shared_lock lock(_mutex);
    auto const iter = _worldActors.find(worldId);
    if (iter == _worldActors.end())
    {
        return nullptr;
    }

    return iter->second;
}
