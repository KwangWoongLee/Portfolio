#include "CorePch.h"
#include "Reward/MySqlSiegeRewardClaimRepository.h"
#include "MySqlConnectionPool.h"
#include <mysql/jdbc.h>

namespace
{
    SiegeRewardClaimRepositoryResult MakeError(
        ESiegeRewardClaimRepositoryError const error,
        std::string message,
        size_t const requestedCount = 0,
        size_t const persistedCount = 0)
    {
        return {
            error,
            std::move(message),
            requestedCount,
            persistedCount,
        };
    }

    SiegeRewardClaimRepositoryResult ToQueryFailed(
        sql::SQLException const& exception,
        size_t const requestedCount,
        size_t const persistedCount)
    {
        return MakeError(
            ESiegeRewardClaimRepositoryError::QueryFailed,
            std::format(
                "code={}, state={}, message={}",
                exception.getErrorCode(),
                exception.getSQLState(),
                exception.what()),
            requestedCount,
            persistedCount);
    }

    bool IsValid(SiegeRewardJob const& job)
    {
        if (job._siegeWarId == INVALID_SIEGE_WAR_ID ||
            job._worldId == INVALID_WORLD_ID ||
            job._siegeWarType == SiegeWarType{} ||
            job._claims.empty())
        {
            return false;
        }

        for (RewardClaim const& claim : job._claims)
        {
            if (claim._id == INVALID_REWARD_CLAIM_ID ||
                claim._eventId != job._siegeWarId ||
                claim._characterId == INVALID_CHARACTER_ID ||
                claim._guildId == INVALID_GUILD_ID ||
                claim._goldAmount < 0)
            {
                return false;
            }
        }

        return true;
    }
}

MySqlSiegeRewardClaimRepository::MySqlSiegeRewardClaimRepository(
    std::shared_ptr<MySqlConnectionPool> connectionPool)
    : _connectionPool(std::move(connectionPool))
{
}

SiegeRewardClaimRepositoryResult MySqlSiegeRewardClaimRepository::PrepareClaims(
    SiegeRewardJob const& job)
{
    auto const requestedCount = job._claims.size();
    if (not _connectionPool || not IsValid(job))
    {
        return MakeError(
            ESiegeRewardClaimRepositoryError::InvalidArgument,
            "invalid siege reward claim job",
            requestedCount);
    }

    auto lease = _connectionPool->Acquire();
    if (not lease)
    {
        return MakeError(
            ESiegeRewardClaimRepositoryError::ConnectionUnavailable,
            "database connection unavailable",
            requestedCount);
    }

    size_t persistedCount{ 0 };
    sql::Connection* connection{ nullptr };
    bool transactionStarted{ false };
    try
    {
        connection = &lease->GetConnection().GetNativeConnection();
        connection->setAutoCommit(false);
        transactionStarted = true;

        std::unique_ptr<sql::PreparedStatement> statement(
            connection->prepareStatement(
                "INSERT INTO siege_reward_claim "
                "(id, siege_war_id, world_id, siege_war_type, character_id, guild_id, reward_type, claim_state, gold_amount) "
                "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?) "
                "ON DUPLICATE KEY UPDATE updated_ut = updated_ut"));

        for (RewardClaim const& claim : job._claims)
        {
            statement->setInt64(1, static_cast<int64_t>(claim._id));
            statement->setInt64(2, static_cast<int64_t>(job._siegeWarId));
            statement->setInt64(3, static_cast<int64_t>(job._worldId));
            statement->setInt64(4, static_cast<int64_t>(job._siegeWarType));
            statement->setInt64(5, static_cast<int64_t>(claim._characterId));
            statement->setInt64(6, static_cast<int64_t>(claim._guildId));
            statement->setString(7, ToString(claim._rewardType));
            statement->setString(8, ToString(claim._state));
            statement->setInt64(9, claim._goldAmount);
            (void)statement->executeUpdate();
            ++persistedCount;
        }

        connection->commit();
        transactionStarted = false;
        connection->setAutoCommit(true);
    }
    catch (sql::SQLException const& exception)
    {
        if (nullptr != connection)
        {
            try
            {
                if (transactionStarted)
                {
                    connection->rollback();
                }
                connection->setAutoCommit(true);
            }
            catch (sql::SQLException const&)
            {
            }
        }

        return ToQueryFailed(exception, requestedCount, 0);
    }

    return { ESiegeRewardClaimRepositoryError::None, {}, requestedCount, persistedCount };
}
