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
        std::unique_ptr<sql::PreparedStatement> insertStatement(
            connection.prepareStatement(
                "INSERT IGNORE INTO characters(id, name, gold) VALUES (?, ?, ?)"));
        insertStatement->setInt64(1, static_cast<int64_t>(id));
        insertStatement->setString(2, std::string(name));
        insertStatement->setInt64(3, initialGold);
        (void)insertStatement->executeUpdate();

        std::unique_ptr<sql::PreparedStatement> selectStatement(
            connection.prepareStatement(
                "SELECT id, name, gold FROM characters WHERE id = ?"));
        selectStatement->setInt64(1, static_cast<int64_t>(id));

        std::optional<CharacterProfile> profile;
        std::unique_ptr<sql::ResultSet> result(selectStatement->executeQuery());
        if (result && result->next())
        {
            profile = CharacterProfile{
                CharacterId{ result->getInt64("id") },
                result->getString("name").asStdString(),
                result->getInt64("gold"),
            };
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
            connection.prepareStatement("UPDATE characters SET gold = ? WHERE id = ?"));
        statement->setInt64(1, gold);
        statement->setInt64(2, static_cast<int64_t>(id));
        (void)statement->executeUpdate();
    }
    catch (sql::SQLException const& exception)
    {
        return ToQueryFailed(exception);
    }

    return {};
}
