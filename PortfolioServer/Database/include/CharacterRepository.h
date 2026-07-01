#pragma once
#include "CorePch.h"
#include "CharacterTypes.h"

enum class ECharacterRepositoryError : uint8_t
{
    None,
    InvalidArgument,
    ConnectionUnavailable,
    QueryFailed,
};

struct CharacterProfile final
{
    CharacterId _id{ INVALID_CHARACTER_ID };
    std::string _name;
    int64_t _gold{};

    bool IsValid() const { return INVALID_CHARACTER_ID != _id; }
};

struct CharacterRepositoryResult final
{
    ECharacterRepositoryError _error{ ECharacterRepositoryError::None };
    std::string _message;

    bool Succeeded() const { return ECharacterRepositoryError::None == _error; }
};

struct CharacterLoadResult final
{
    ECharacterRepositoryError _error{ ECharacterRepositoryError::None };
    std::string _message;
    CharacterProfile _profile;

    bool Succeeded() const { return ECharacterRepositoryError::None == _error; }
};

class ICharacterRepository
{
public:
    virtual ~ICharacterRepository() = default;

    virtual CharacterLoadResult GetOrCreateCharacter(
        CharacterId id,
        std::string_view name,
        int64_t initialGold) = 0;
    virtual CharacterRepositoryResult UpdateGold(CharacterId id, int64_t gold) = 0;
};
