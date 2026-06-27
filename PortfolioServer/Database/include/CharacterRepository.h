#pragma once
#include "CorePch.h"
#include "CharacterTypes.h"

enum class ECharacterRepositoryError : uint8_t
{
    None,
    ConnectionUnavailable,
    QueryFailed,
};

struct CharacterRepositoryResult final
{
    ECharacterRepositoryError _error{ ECharacterRepositoryError::None };
    std::string _message;

    bool Succeeded() const { return ECharacterRepositoryError::None == _error; }
};

class ICharacterRepository
{
public:
    virtual ~ICharacterRepository() = default;

    virtual CharacterRepositoryResult UpdateGold(CharacterId id, int64_t gold) = 0;
};
