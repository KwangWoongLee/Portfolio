#pragma once
#include "WorldActor.h"

class WorldTaskRunner;
class SiegeWarTaskRunner;

class WorldActorRegistry final
{
public:
    using Singleton = Singleton<WorldActorRegistry>;

    bool Create(WorldId worldId);
    bool Remove(WorldId worldId);
    size_t GetCount() const;
    std::optional<GuildSnapshot> GetGuildSnapshot(WorldId worldId, GuildId guildId) const;
    std::optional<GuildSnapshot> GetGuildSnapshotByMember(WorldId worldId, ActorId actorId) const;
    std::optional<SiegeWarSnapshot> GetSiegeWarSnapshot(
        WorldId worldId,
        SiegeWarId siegeWarId) const;

private:
    friend class SiegeWarTaskRunner;
    friend class WorldTaskRunner;

    std::shared_ptr<WorldActor> FindInternal(WorldId worldId) const;

    mutable std::shared_mutex _mutex;
    std::unordered_map<WorldId, std::shared_ptr<WorldActor>> _worldActors;
};
