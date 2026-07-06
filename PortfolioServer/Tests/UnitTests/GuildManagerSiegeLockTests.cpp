#include <gtest/gtest.h>
#include "Guild/GuildManager.h"

class GuildManagerTestAccess final
{
public:
    static GuildOperationResult CreateGuild(
        GuildManager& manager,
        ActorId const leaderActorId,
        std::string name)
    {
        return manager.CreateGuild(leaderActorId, std::move(name));
    }

    static GuildOperationResult JoinGuild(
        GuildManager& manager,
        ActorId const actorId,
        GuildId const guildId)
    {
        return manager.JoinGuild(actorId, guildId);
    }

    static GuildOperationResult LeaveGuild(
        GuildManager& manager,
        ActorId const actorId,
        std::optional<ActorId> const successorActorId = std::nullopt)
    {
        return manager.LeaveGuild(actorId, successorActorId);
    }

    static GuildOperationResult TransferLeader(
        GuildManager& manager,
        ActorId const leaderActorId,
        ActorId const successorActorId)
    {
        return manager.TransferLeader(leaderActorId, successorActorId);
    }

    static SiegeWarId RegisterSiegeWar(
        GuildManager& manager,
        SiegeWarData data,
        SiegeWar::Clock::time_point const scheduledAt)
    {
        return manager.RegisterSiegeWar(DEFAULT_WORLD_ID, std::move(data), scheduledAt);
    }

    static std::optional<SiegeDeclarationPayment> TryReserveSiegeDeclaration(
        GuildManager& manager,
        ActorId const requesterActorId,
        SiegeWarType const siegeWarType)
    {
        return manager.TryReserveSiegeDeclaration(requesterActorId, siegeWarType);
    }

    static ESiegeDeclarationCompletion CompleteSiegeDeclaration(
        GuildManager& manager,
        SiegeDeclarationId const declarationId,
        ActorId const requesterActorId,
        bool const paid)
    {
        return manager.CompleteSiegeDeclaration(declarationId, requesterActorId, paid);
    }

    static bool ApplySiegeWarSnapshot(GuildManager& manager, SiegeWarSnapshot snapshot)
    {
        return manager.ApplySiegeWarSnapshot(std::move(snapshot));
    }
};

namespace
{
    SiegeWarData MakeSiegeWarData(SiegeWarType const siegeWarType)
    {
        return SiegeWarData{
            siegeWarType,
            "LockTestSiege",
            10,
            20,
            5,
            100,
        };
    }
}

TEST(GuildManagerSiegeLockTests, DeclarationReservationLocksGuildBeforePayment)
{
    GuildManager manager(DEFAULT_WORLD_ID);
    auto const leaderActorId = ActorId{ 1001 };
    auto const siegeWarType = SiegeWarType{ 11 };

    auto const guild = GuildManagerTestAccess::CreateGuild(
        manager,
        leaderActorId,
        "DeclarationLockGuild");
    ASSERT_TRUE(guild.Succeeded());

    auto const siegeWarId = GuildManagerTestAccess::RegisterSiegeWar(
        manager,
        MakeSiegeWarData(siegeWarType),
        SiegeWar::Clock::now());
    ASSERT_NE(INVALID_SIEGE_WAR_ID, siegeWarId);

    auto const payment = GuildManagerTestAccess::TryReserveSiegeDeclaration(
        manager,
        leaderActorId,
        siegeWarType);
    ASSERT_TRUE(payment);
    EXPECT_EQ(guild._guildId, payment->_guildId);

    auto const lockedLeave = GuildManagerTestAccess::LeaveGuild(manager, leaderActorId);
    EXPECT_EQ(EGuildOperationError::SiegeParticipationLocked, lockedLeave._error);
    EXPECT_TRUE(manager.GetGuildSnapshot(guild._guildId));

    auto const canceled = GuildManagerTestAccess::CompleteSiegeDeclaration(
        manager,
        payment->_declarationId,
        leaderActorId,
        false);
    EXPECT_EQ(ESiegeDeclarationCompletion::Canceled, canceled);

    auto const leaveAfterPaymentFailure = GuildManagerTestAccess::LeaveGuild(manager, leaderActorId);
    EXPECT_TRUE(leaveAfterPaymentFailure.Succeeded());
    EXPECT_FALSE(manager.GetGuildSnapshot(guild._guildId));
}

TEST(GuildManagerSiegeLockTests, PaidDeclarationStaysLockedUntilFinishedSnapshot)
{
    GuildManager manager(DEFAULT_WORLD_ID);
    auto const leaderActorId = ActorId{ 2001 };
    auto const successorActorId = ActorId{ 2002 };
    auto const siegeWarType = SiegeWarType{ 12 };

    auto const guild = GuildManagerTestAccess::CreateGuild(
        manager,
        leaderActorId,
        "PaidDeclarationLockGuild");
    ASSERT_TRUE(guild.Succeeded());
    ASSERT_TRUE(GuildManagerTestAccess::JoinGuild(manager, successorActorId, guild._guildId).Succeeded());

    auto const siegeWarId = GuildManagerTestAccess::RegisterSiegeWar(
        manager,
        MakeSiegeWarData(siegeWarType),
        SiegeWar::Clock::now());
    ASSERT_NE(INVALID_SIEGE_WAR_ID, siegeWarId);

    auto const payment = GuildManagerTestAccess::TryReserveSiegeDeclaration(
        manager,
        leaderActorId,
        siegeWarType);
    ASSERT_TRUE(payment);

    auto const completed = GuildManagerTestAccess::CompleteSiegeDeclaration(
        manager,
        payment->_declarationId,
        leaderActorId,
        true);
    EXPECT_EQ(ESiegeDeclarationCompletion::Completed, completed);

    auto const lockedTransfer = GuildManagerTestAccess::TransferLeader(
        manager,
        leaderActorId,
        successorActorId);
    EXPECT_EQ(EGuildOperationError::SiegeParticipationLocked, lockedTransfer._error);

    auto snapshot = manager.GetSiegeWarSnapshot(siegeWarId);
    ASSERT_TRUE(snapshot);
    ++snapshot->_revision;
    snapshot->_state = ESiegeWarState::Finished;
    snapshot->_progressStep = ESiegeWarProgressStep::None;
    snapshot->_endReason = ESiegeWarEndReason::DefenderVictory;
    snapshot->_defenderGuildId = guild._guildId;
    snapshot->_winnerGuildId = guild._guildId;

    EXPECT_TRUE(GuildManagerTestAccess::ApplySiegeWarSnapshot(manager, *snapshot));

    auto const transferAfterFinished = GuildManagerTestAccess::TransferLeader(
        manager,
        leaderActorId,
        successorActorId);
    EXPECT_TRUE(transferAfterFinished.Succeeded());
}