#pragma once
#include "Actor.h"
#include "CorePch.h"

using GuildId = StrongId<struct GuildIdTag, int64_t>;

inline GuildId constexpr INVALID_GUILD_ID{ 0 };

enum class EGuildRole : uint8_t
{
    Member,
    Leader,
};

enum class EGuildOperationError : uint8_t
{
    None,
    InvalidArgument,
    GuildNotFound,
    AlreadyInGuild,
    NotGuildMember,
    NotGuildLeader,
    SuccessorRequired,
    SuccessorNotMember,
    GuildNameAlreadyExists,
    SiegeParticipationLocked,
    SiegeAlreadyDeclared,
    SiegeDeclarationNotFound,
};

struct GuildOperationResult final
{
    EGuildOperationError _error{ EGuildOperationError::None };
    GuildId _guildId{ INVALID_GUILD_ID };

    bool Succeeded() const { return _error == EGuildOperationError::None; }
};

struct GuildMemberSnapshot final
{
    ActorId _actorId{ INVALID_ACTOR_ID };
    EGuildRole _role{ EGuildRole::Member };
};

struct GuildSnapshot final
{
    GuildId _guildId{ INVALID_GUILD_ID };
    std::string _name;
    ActorId _leaderActorId{ INVALID_ACTOR_ID };
    std::vector<GuildMemberSnapshot> _members;
};
