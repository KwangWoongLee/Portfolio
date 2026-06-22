#pragma once
#include "CmsTypes.h"
#include "Guild/GuildTypes.h"
#include "WorldTypes.h"

using SiegeWarId = StrongId<struct SiegeWarIdTag, int64_t>;
using SiegeDeclarationId = StrongId<struct SiegeDeclarationIdTag, int64_t>;

inline SiegeWarId constexpr INVALID_SIEGE_WAR_ID{ 0 };
inline SiegeDeclarationId constexpr INVALID_SIEGE_DECLARATION_ID{ 0 };

enum class ESiegeWarState : uint8_t
{
    Scheduled,
    Prepare,
    InProgress,
    Finished,
    Canceled,
};

enum class ESiegeWarProgressStep : uint8_t
{
    None,
    AttackWindow,
    OccupationGrace,
};

enum class ESiegeWarEndReason : uint8_t
{
    None,
    DefenderVictory,
    DrawNoOwner,
    Canceled,
};

struct SiegeWarSnapshot final
{
    WorldId _worldId{ INVALID_WORLD_ID };
    SiegeWarId _siegeWarId{ INVALID_SIEGE_WAR_ID };
    SiegeWarType _siegeWarType;
    uint64_t _revision{};

    ESiegeWarState _state{ ESiegeWarState::Scheduled };
    ESiegeWarProgressStep _progressStep{ ESiegeWarProgressStep::None };
    ESiegeWarEndReason _endReason{ ESiegeWarEndReason::None };

    GuildId _defenderGuildId{ INVALID_GUILD_ID };
    GuildId _winnerGuildId{ INVALID_GUILD_ID };
};

enum class ESiegeDeclarationState : uint8_t
{
    PendingPayment,
    Declared,
    Canceled,
};

enum class ESiegeDeclarationCompletion : uint8_t
{
    Completed,
    Canceled,
    AlreadyCompleted,
    NotFound,
    RequesterMismatch,
};

struct SiegeDeclarationPayment final
{
    SiegeDeclarationId _declarationId{ INVALID_SIEGE_DECLARATION_ID };
    ActorId _requesterActorId{ INVALID_ACTOR_ID };
    GuildId _guildId{ INVALID_GUILD_ID };
    int64_t _costGold{};
};
