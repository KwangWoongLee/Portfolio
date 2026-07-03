#include <gtest/gtest.h>
#include "Siege/SiegeWar.h"

namespace
{
    SiegeWarData MakeSiegeWarData(
        int32_t const prepDurationSec,
        int32_t const attackDurationSec,
        int32_t const swapSideDurationSec)
    {
        return SiegeWarData{
            SiegeWarType{ 1 },
            "TestSiege",
            prepDurationSec,
            attackDurationSec,
            swapSideDurationSec,
            100,
        };
    }

    void EnterInProgress(SiegeWar& war, SiegeWar::Clock::time_point const now)
    {
        auto const scheduled = war.Tick(now);
        ASSERT_TRUE(scheduled.Succeeded());
        ASSERT_EQ(ESiegeWarState::Prepare, war.GetState());

        auto const prepared = war.Tick(now);
        ASSERT_TRUE(prepared.Succeeded());
        ASSERT_EQ(ESiegeWarState::InProgress, war.GetState());
        ASSERT_EQ(ESiegeWarProgressStep::AttackWindow, war.GetProgressStep());
    }

    void ExpectNextWakeUp(
        SiegeWar const& war,
        SiegeWar::Clock::time_point const expected)
    {
        auto const wakeUp = war.GetNextWakeUpTime();
        ASSERT_TRUE(wakeUp);
        EXPECT_EQ(expected, *wakeUp);
    }
}

TEST(SiegeWarTests, OccupationDoesNotExtendFinalDeadline)
{
    auto const startedAt = SiegeWar::Clock::now();
    SiegeWar war(
        DEFAULT_WORLD_ID,
        SiegeWarId{ 1 },
        MakeSiegeWarData(0, 10, 5),
        startedAt);
    EnterInProgress(war, startedAt);
    ExpectNextWakeUp(war, startedAt + std::chrono::seconds(10));

    EXPECT_TRUE(war.OnOccupied(GuildId{ 100 }, startedAt + std::chrono::seconds(1)));
    EXPECT_EQ(GuildId{ 100 }, war.GetDefenderGuildId());
    EXPECT_EQ(ESiegeWarProgressStep::OccupationGrace, war.GetProgressStep());
    ExpectNextWakeUp(war, startedAt + std::chrono::seconds(6));

    EXPECT_TRUE(war.OnOccupied(GuildId{ 200 }, startedAt + std::chrono::seconds(2)));
    EXPECT_EQ(GuildId{ 200 }, war.GetDefenderGuildId());
    EXPECT_EQ(ESiegeWarProgressStep::AttackWindow, war.GetProgressStep());
    ExpectNextWakeUp(war, startedAt + std::chrono::seconds(10));

    auto const beforeDeadline = war.Tick(startedAt + std::chrono::seconds(9));
    EXPECT_FALSE(beforeDeadline.Succeeded());
    EXPECT_EQ(ESiegeWarState::InProgress, war.GetState());

    auto const deadline = war.Tick(startedAt + std::chrono::seconds(10));
    EXPECT_TRUE(deadline.Succeeded());
    EXPECT_EQ(ESiegeWarState::Finished, war.GetState());
    EXPECT_FALSE(war.GetNextWakeUpTime().has_value());

    auto const snapshot = war.CreateSnapshot();
    EXPECT_EQ(ESiegeWarEndReason::DefenderVictory, snapshot._endReason);
    EXPECT_EQ(GuildId{ 200 }, snapshot._winnerGuildId);
}

TEST(SiegeWarTests, SameDefenderDoesNotRefreshOccupationGrace)
{
    auto const startedAt = SiegeWar::Clock::now();
    SiegeWar war(
        DEFAULT_WORLD_ID,
        SiegeWarId{ 1 },
        MakeSiegeWarData(0, 20, 5),
        startedAt);
    EnterInProgress(war, startedAt);

    EXPECT_TRUE(war.OnOccupied(GuildId{ 100 }, startedAt + std::chrono::seconds(1)));
    ExpectNextWakeUp(war, startedAt + std::chrono::seconds(6));

    EXPECT_FALSE(war.OnOccupied(GuildId{ 100 }, startedAt + std::chrono::seconds(4)));
    EXPECT_EQ(ESiegeWarProgressStep::OccupationGrace, war.GetProgressStep());
    ExpectNextWakeUp(war, startedAt + std::chrono::seconds(6));

    auto const graceEnded = war.Tick(startedAt + std::chrono::seconds(6));
    EXPECT_TRUE(graceEnded.Succeeded());
    EXPECT_EQ(ESiegeWarState::Finished, war.GetState());
    EXPECT_FALSE(war.GetNextWakeUpTime().has_value());

    auto const snapshot = war.CreateSnapshot();
    EXPECT_EQ(ESiegeWarEndReason::DefenderVictory, snapshot._endReason);
    EXPECT_EQ(GuildId{ 100 }, snapshot._winnerGuildId);
}

TEST(SiegeWarTests, OccupationAtFinalDeadlineDoesNotCreateNewDefender)
{
    auto const startedAt = SiegeWar::Clock::now();
    SiegeWar war(
        DEFAULT_WORLD_ID,
        SiegeWarId{ 1 },
        MakeSiegeWarData(0, 10, 5),
        startedAt);
    EnterInProgress(war, startedAt);
    ExpectNextWakeUp(war, startedAt + std::chrono::seconds(10));

    EXPECT_TRUE(war.OnOccupied(GuildId{ 100 }, startedAt + std::chrono::seconds(10)));
    EXPECT_EQ(ESiegeWarState::Finished, war.GetState());
    EXPECT_FALSE(war.GetNextWakeUpTime().has_value());

    auto const snapshot = war.CreateSnapshot();
    EXPECT_EQ(ESiegeWarEndReason::DrawNoOwner, snapshot._endReason);
    EXPECT_EQ(INVALID_GUILD_ID, snapshot._winnerGuildId);
}