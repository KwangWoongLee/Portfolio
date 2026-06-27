#pragma once
#include "CharacterRepository.h"

class MySqlConnectionPool;

class MySqlCharacterRepository final
    : public ICharacterRepository
{
public:
    explicit MySqlCharacterRepository(std::shared_ptr<MySqlConnectionPool> connectionPool);

    CharacterRepositoryResult UpdateGold(CharacterId id, int64_t gold) override;

private:
    std::shared_ptr<MySqlConnectionPool> _connectionPool;
};
