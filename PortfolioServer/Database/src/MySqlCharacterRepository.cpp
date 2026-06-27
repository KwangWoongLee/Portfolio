#include "CorePch.h"
#include "MySqlCharacterRepository.h"
#include "MySqlConnectionPool.h"
#include <mysql/jdbc.h>

MySqlCharacterRepository::MySqlCharacterRepository(
    std::shared_ptr<MySqlConnectionPool> connectionPool)
    : _connectionPool(std::move(connectionPool))
{
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
        return {
            ECharacterRepositoryError::QueryFailed,
            std::format(
                "code={}, state={}, message={}",
                exception.getErrorCode(),
                exception.getSQLState(),
                exception.what()),
        };
    }

    return {};
}
