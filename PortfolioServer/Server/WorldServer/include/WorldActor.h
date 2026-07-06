#pragma once
#include "Guild/GuildManager.h"
#include "Reward/SiegeRewardPlanner.h"
#include "WorldMessages.h"

class ISiegeRewardClaimRepository;
class SiegeWarTaskRunner;
class WorldTaskRunner;
struct SiegeScheduleData;

class WorldActor final
{
public:
    WorldActor(
        WorldId worldId,
        std::shared_ptr<ISiegeRewardClaimRepository> siegeRewardClaimRepository);
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
    void OnMessage(WorldMsg::SiegeScheduleTriggered const& msg);
    void OnMessage(WorldMsg::SiegeWarSnapshotUpdated const& msg);
    void OnMessage(WorldMsg::SiegeRewardClaimsPersisted const& msg);
    void OnMessage(WorldMsg::StartSiegeDemo const& msg);

    std::shared_ptr<SiegeWar> FindSiegeWarInternal(SiegeWarId siegeWarId) const;
    void CancelSiegeDeclarationPaymentTimer(SiegeDeclarationId declarationId);
    void CancelSiegeWarWakeUpTimer(SiegeWarId siegeWarId);
    void CreateSiegeRewardJob(SiegeWarSnapshot const& snapshot);
    bool ScheduleSiegeWarWakeUp(SiegeWarId siegeWarId);
    bool ScheduleNextSiege(SiegeScheduleData const& schedule);

    WorldId const _worldId;
    GuildManager _guildManager;
    SiegeRewardPlanner _siegeRewardPlanner;
    std::shared_ptr<ISiegeRewardClaimRepository> _siegeRewardClaimRepository;
    std::unordered_map<SiegeScheduleType, TimerId, SiegeScheduleTypeHash> _siegeScheduleTimerIds;
    std::unordered_map<SiegeWarId, TimerId> _siegeWarWakeUpTimerIds;
    std::unordered_map<SiegeDeclarationId, TimerId> _siegeDeclarationPaymentTimerIds;
};
