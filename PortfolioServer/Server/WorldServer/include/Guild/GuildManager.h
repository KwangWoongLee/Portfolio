#pragma once
#include "Guild/Guild.h"
#include "Siege/SiegeManager.h"

class WorldActor;
class SiegeWarTaskRunner;

class GuildManager final
{
public:
    explicit GuildManager(WorldId worldId);

    GuildManager(GuildManager const&) = delete;
    GuildManager& operator=(GuildManager const&) = delete;
    GuildManager(GuildManager&&) = delete;
    GuildManager& operator=(GuildManager&&) = delete;

    std::optional<GuildSnapshot> GetGuildSnapshot(GuildId guildId) const;
    std::optional<GuildSnapshot> GetGuildSnapshotByMember(ActorId actorId) const;
    std::optional<SiegeWarSnapshot> GetSiegeWarSnapshot(SiegeWarId siegeWarId) const;

private:
    friend class WorldActor;
    friend class SiegeWarTaskRunner;

    GuildOperationResult CreateGuild(ActorId leaderActorId, std::string name);
    GuildOperationResult JoinGuild(ActorId actorId, GuildId guildId);
    GuildOperationResult LeaveGuild(ActorId actorId, std::optional<ActorId> successorActorId);
    GuildOperationResult TransferLeader(ActorId leaderActorId, ActorId successorActorId);
    std::optional<SiegeDeclarationPayment> TryReserveSiegeDeclaration(
        ActorId requesterActorId,
        SiegeWarType siegeWarType);
    ESiegeDeclarationCompletion CompleteSiegeDeclaration(
        SiegeDeclarationId declarationId,
        ActorId requesterActorId,
        bool paid);

    SiegeWarId RegisterSiegeWar(
        WorldId worldId,
        SiegeWarData data,
        SiegeWar::Clock::time_point scheduledAt,
        GuildId initialDefenderGuildId);
    bool ApplySiegeWarSnapshot(SiegeWarSnapshot snapshot);
    std::shared_ptr<SiegeWar> FindSiegeWarInternal(SiegeWarId siegeWarId) const;

    WorldId const _worldId;
    mutable std::shared_mutex _mutex;
    std::unordered_map<GuildId, std::shared_ptr<Guild>> _guilds;
    std::unordered_map<ActorId, GuildId> _guildIdsByMember;
    std::unordered_map<std::string, GuildId> _guildIdsByName;
    SiegeManager _siegeManager;
};
