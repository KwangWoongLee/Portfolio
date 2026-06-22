#pragma once
#include "Guild/GuildManager.h"
#include "WorldMessages.h"

class SiegeWarTaskRunner;
class WorldTaskRunner;

class WorldActor final
{
public:
    explicit WorldActor(WorldId worldId) : _worldId(worldId) {}
    ~WorldActor();

    WorldActor(WorldActor const&) = delete;
    WorldActor& operator=(WorldActor const&) = delete;
    WorldActor(WorldActor&&) = delete;
    WorldActor& operator=(WorldActor&&) = delete;

    WorldId GetWorldId() const { return _worldId; }
    std::optional<GuildSnapshot> GetGuildSnapshot(GuildId guildId) const;
    std::optional<GuildSnapshot> GetGuildSnapshotByMember(ActorId actorId) const;
    std::optional<SiegeWarSnapshot> GetSiegeWarSnapshot(SiegeWarId siegeWarId) const;

private:
    friend class SiegeWarTaskRunner;
    friend class WorldTaskRunner;

    void OnMessage(WorldMsg::CreateGuild const& msg);
    void OnMessage(WorldMsg::JoinGuild const& msg);
    void OnMessage(WorldMsg::LeaveGuild const& msg);
    void OnMessage(WorldMsg::TransferGuildLeader const& msg);
    void OnMessage(WorldMsg::DeclareSiege const& msg);
    void OnMessage(WorldMsg::SiegeDeclarationPaymentCompleted const& msg);
    void OnMessage(WorldMsg::SiegeDeclarationPaymentTimedOut const& msg);
    void OnMessage(WorldMsg::RegisterSiegeWar const& msg);
    void OnMessage(WorldMsg::SiegeWarSnapshotUpdated const& msg);

    std::shared_ptr<SiegeWar> FindSiegeWarInternal(SiegeWarId siegeWarId) const;
    void CancelSiegeDeclarationPaymentTimer(SiegeDeclarationId declarationId);

    WorldId const _worldId;
    GuildManager _guildManager;
    std::unordered_map<SiegeWarId, TimerId> _siegeWarTimerIds;
    std::unordered_map<SiegeDeclarationId, TimerId> _siegeDeclarationPaymentTimerIds;
};
