#include "CorePch.h"
#include "MySqlCharacterRepository.h"
#include "MySqlConnectionPool.h"
#include <mysql/jdbc.h>

namespace
{
    CharacterRepositoryResult ToQueryFailed(sql::SQLException const& exception)
    {
        return {
            ECharacterRepositoryError::QueryFailed,
            std::format(
                "code={}, state={}, message={}",
                exception.getErrorCode(),
                exception.getSQLState(),
                exception.what()),
        };
    }
}

MySqlCharacterRepository::MySqlCharacterRepository(
    std::shared_ptr<MySqlConnectionPool> connectionPool)
    : _connectionPool(std::move(connectionPool))
{
}

CharacterLoadResult MySqlCharacterRepository::GetOrCreateCharacter(
    CharacterId const id,
    std::string_view const name,
    int64_t const initialGold)
{
    if (INVALID_CHARACTER_ID == id || name.empty() || initialGold < 0)
    {
        return {
            ECharacterRepositoryError::InvalidArgument,
            "invalid character load argument",
        };
    }

    auto lease = _connectionPool->Acquire();
    if (not lease)
    {
        return {
            ECharacterRepositoryError::ConnectionUnavailable,
            "database connection unavailable",
        };
    }

    try
    {
        auto& connection = lease->GetConnection().GetNativeConnection();
        std::unique_ptr<sql::PreparedStatement> statement(
            connection.prepareStatement("CALL get_or_create_character(?, ?, ?)"));
        statement->setInt64(1, static_cast<int64_t>(id));
        statement->setString(2, std::string(name));
        statement->setInt64(3, initialGold);

        std::optional<CharacterProfile> profile;
        bool hasResultSet{ statement->execute() };
        while (true)
        {
            if (hasResultSet)
            {
                std::unique_ptr<sql::ResultSet> result(statement->getResultSet());
                while (result && result->next())
                {
                    if (not profile)
                    {
                        profile = CharacterProfile{
                            CharacterId{ result->getInt64("id") },
                            result->getString("name").asStdString(),
                            result->getInt64("gold"),
                        };
                    }
                }
            }
            else if (-1 == statement->getUpdateCount())
            {
                break;
            }

            hasResultSet = statement->getMoreResults();
        }

        if (not profile || not profile->IsValid())
        {
            return {
                ECharacterRepositoryError::QueryFailed,
                "character profile was not returned",
            };
        }

        return { ECharacterRepositoryError::None, {}, std::move(*profile) };
    }
    catch (sql::SQLException const& exception)
    {
        auto result = ToQueryFailed(exception);
        return {
            result._error,
            std::move(result._message),
        };
    }
}

CharacterRepositoryResult MySqlCharacterRepository::UpdateGold(
    CharacterId const id,
    int64_t const gold)
{
    auto lease = _connectionPool->Acquire();
    if (not lease)
    {
        return {
            ECharacterRepositoryError::ConnectionUnavailable,
            "database connection unavailable",
        };
    }

    try
    {
        auto& connection = lease->GetConnection().GetNativeConnection();
        std::unique_ptr<sql::PreparedStatement> statement(
            connection.prepareStatement("CALL update_character_gold(?, ?)"));
        statement->setInt64(1, static_cast<int64_t>(id));
        statement->setInt64(2, gold);

        bool hasResultSet{ statement->execute() };
        while (true)
        {
            if (hasResultSet)
            {
                std::unique_ptr<sql::ResultSet> result(statement->getResultSet());
                while (result && result->next())
                {
                }
            }
            else if (-1 == statement->getUpdateCount())
            {
                break;
            }

            hasResultSet = statement->getMoreResults();
        }
    }
    catch (sql::SQLException const& exception)
    {
        return ToQueryFailed(exception);
    }

    return {};
}
