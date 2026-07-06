#include "CorePch.h"
#include "Reward/SiegeRewardPlanner.h"
#include "UniqueIdGenerator.h"
#include <algorithm>

namespace
{
    auto constexpr SIEGE_WINNER_GOLD_AMOUNT = 10'000;
}

char const* ToString(ERewardType const rewardType)
{
    switch (rewardType)
    {
    case ERewardType::SiegeWinnerGold:
        return "SiegeWinnerGold";
    default:
        return "Unknown";
    }
}

char const* ToString(ERewardClaimState const state)
{
    switch (state)
    {
    case ERewardClaimState::ReadyToClaim:
        return "ReadyToClaim";
    case ERewardClaimState::Claimed:
        return "Claimed";
    case ERewardClaimState::Failed:
        return "Failed";
    default:
        return "Unknown";
    }
}

char const* ToString(ESiegeRewardJobState const state)
{
    switch (state)
    {
    case ESiegeRewardJobState::ClaimsPrepared:
        return "ClaimsPrepared";
    case ESiegeRewardJobState::SkippedNoWinner:
        return "SkippedNoWinner";
    default:
        return "Unknown";
    }
}

SiegeRewardPlanner::SiegeRewardPlanner(WorldId const worldId)
    : _worldId(worldId)
{
}

std::optional<SiegeRewardJob> SiegeRewardPlanner::CreateFinishedSiegeJob(
    SiegeWarSnapshot const& snapshot,
    std::optional<GuildSnapshot> const& winnerGuildSnapshot)
{
    if (snapshot._worldId != _worldId ||
        snapshot._state != ESiegeWarState::Finished ||
        _settledSiegeWarIds.contains(snapshot._siegeWarId))
    {
        return std::nullopt;
    }

    SiegeRewardJob job{
        snapshot._siegeWarId,
        snapshot._worldId,
        snapshot._siegeWarType,
        snapshot._winnerGuildId,
        snapshot._endReason,
    };

    if (snapshot._endReason != ESiegeWarEndReason::DefenderVictory ||
        snapshot._winnerGuildId == INVALID_GUILD_ID ||
        not winnerGuildSnapshot ||
        winnerGuildSnapshot->_guildId != snapshot._winnerGuildId)
    {
        job._state = ESiegeRewardJobState::SkippedNoWinner;
        _settledSiegeWarIds.insert(snapshot._siegeWarId);
        return job;
    }

    auto members = winnerGuildSnapshot->_members;
    std::sort(members.begin(), members.end(),
        [](GuildMemberSnapshot const& lhs, GuildMemberSnapshot const& rhs)
        {
            return lhs._actorId < rhs._actorId;
        });

    job._state = ESiegeRewardJobState::ClaimsPrepared;
    job._claims.reserve(members.size());

    for (GuildMemberSnapshot const& member : members)
    {
        if (member._actorId == INVALID_ACTOR_ID)
        {
            continue;
        }

        auto const id = GenerateClaimId();
        if (id == INVALID_REWARD_CLAIM_ID)
        {
            return std::nullopt;
        }

        job._claims.emplace_back(RewardClaim{
            id,
            snapshot._siegeWarId,
            CharacterId{ static_cast<int64_t>(member._actorId) },
            snapshot._winnerGuildId,
            ERewardType::SiegeWinnerGold,
            ERewardClaimState::ReadyToClaim,
            SIEGE_WINNER_GOLD_AMOUNT,
        });
    }

    _settledSiegeWarIds.insert(snapshot._siegeWarId);
    return job;
}

RewardClaimId SiegeRewardPlanner::GenerateClaimId() const
{
    auto const value = UniqueIdGenerator::Generate(
        EUniqueIdKind::RewardClaim,
        static_cast<int64_t>(_worldId));
    if (not value)
    {
        return INVALID_REWARD_CLAIM_ID;
    }

    return RewardClaimId{ *value };
}
