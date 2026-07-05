#include <gtest/gtest.h>
#include "Reward/SiegeRewardPlanner.h"

namespace
{
    SiegeWarSnapshot MakeFinishedSnapshot(
        SiegeWarId const siegeWarId,
        ESiegeWarEndReason const endReason,
        GuildId const winnerGuildId)
    {
        return SiegeWarSnapshot{
            DEFAULT_WORLD_ID,
            siegeWarId,
            SiegeWarType{ 1 },
            10,
            ESiegeWarState::Finished,
            ESiegeWarProgressStep::None,
            endReason,
            winnerGuildId,
            winnerGuildId,
        };
    }

    GuildSnapshot MakeGuildSnapshot(GuildId const guildId)
    {
        return GuildSnapshot{
            guildId,
            "Winner",
            ActorId{ 300 },
            {
                GuildMemberSnapshot{ ActorId{ 300 }, EGuildRole::Leader },
                GuildMemberSnapshot{ ActorId{ 100 }, EGuildRole::Member },
            },
        };
    }
}

TEST(SiegeRewardPlannerTests, CreatesReadyClaimsForWinnerMembersOnce)
{
    SiegeRewardPlanner planner(DEFAULT_WORLD_ID);
    auto const snapshot = MakeFinishedSnapshot(
        SiegeWarId{ 101 },
        ESiegeWarEndReason::DefenderVictory,
        GuildId{ 10 });
    auto const guild = MakeGuildSnapshot(GuildId{ 10 });

    auto job = planner.CreateFinishedSiegeJob(snapshot, guild);

    ASSERT_TRUE(job);
    EXPECT_EQ(ESiegeRewardJobState::ClaimsPrepared, job->_state);
    EXPECT_EQ(snapshot._siegeWarId, job->_siegeWarId);
    EXPECT_EQ(GuildId{ 10 }, job->_winnerGuildId);
    ASSERT_EQ(2, job->_claims.size());

    EXPECT_EQ(ActorId{ 100 }, job->_claims[0]._actorId);
    EXPECT_EQ(ActorId{ 300 }, job->_claims[1]._actorId);
    for (RewardClaim const& claim : job->_claims)
    {
        EXPECT_NE(INVALID_REWARD_CLAIM_ID, claim._claimId);
        EXPECT_EQ(snapshot._siegeWarId, claim._eventId);
        EXPECT_EQ(GuildId{ 10 }, claim._guildId);
        EXPECT_EQ(ERewardType::SiegeWinnerGold, claim._rewardType);
        EXPECT_EQ(ERewardClaimState::ReadyToClaim, claim._state);
        EXPECT_GT(claim._goldAmount, 0);
    }

    auto duplicate = planner.CreateFinishedSiegeJob(snapshot, guild);
    EXPECT_FALSE(duplicate);
}

TEST(SiegeRewardPlannerTests, SkipsClaimsWhenSiegeHasNoWinner)
{
    SiegeRewardPlanner planner(DEFAULT_WORLD_ID);
    auto const snapshot = MakeFinishedSnapshot(
        SiegeWarId{ 102 },
        ESiegeWarEndReason::DrawNoOwner,
        INVALID_GUILD_ID);

    auto job = planner.CreateFinishedSiegeJob(snapshot, std::nullopt);

    ASSERT_TRUE(job);
    EXPECT_EQ(ESiegeRewardJobState::SkippedNoWinner, job->_state);
    EXPECT_EQ(INVALID_GUILD_ID, job->_winnerGuildId);
    EXPECT_TRUE(job->_claims.empty());
}
