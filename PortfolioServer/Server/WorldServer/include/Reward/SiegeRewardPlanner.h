#pragma once
#include "Reward/RewardTypes.h"

class SiegeRewardPlanner final
{
public:
    explicit SiegeRewardPlanner(WorldId worldId);

    std::optional<SiegeRewardJob> CreateFinishedSiegeJob(
        SiegeWarSnapshot const& snapshot,
        std::optional<GuildSnapshot> const& winnerGuildSnapshot);

private:
    RewardClaimId GenerateClaimId() const;

    WorldId const _worldId;
    std::unordered_set<SiegeWarId> _settledSiegeWarIds;
};
