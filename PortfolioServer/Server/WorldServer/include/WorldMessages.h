#pragma once
#include "Guild/GuildTypes.h"
#include "Siege/SiegeWar.h"

namespace WorldMsg
{
    struct CreateGuild final
    {
        ActorId _leaderActorId{ INVALID_ACTOR_ID };
        std::string _name;
    };

    struct JoinGuild final
    {
        ActorId _actorId{ INVALID_ACTOR_ID };
        GuildId _guildId{ INVALID_GUILD_ID };
    };

    struct LeaveGuild final
    {
        ActorId _actorId{ INVALID_ACTOR_ID };
        std::optional<ActorId> _successorActorId;
    };

    struct TransferGuildLeader final
    {
        ActorId _leaderActorId{ INVALID_ACTOR_ID };
        ActorId _successorActorId{ INVALID_ACTOR_ID };
    };

    struct DeclareSiege final
    {
        ActorId _requesterActorId{ INVALID_ACTOR_ID };
        SiegeWarType _siegeWarType;
    };

    struct SiegeDeclarationPaymentCompleted final
    {
        ActorId _requesterActorId{ INVALID_ACTOR_ID };
        SiegeDeclarationId _declarationId{ INVALID_SIEGE_DECLARATION_ID };
        int64_t _costGold{};
        bool _paid{};
    };

    struct SiegeDeclarationPaymentTimedOut final
    {
        ActorId _requesterActorId{ INVALID_ACTOR_ID };
        SiegeDeclarationId _declarationId{ INVALID_SIEGE_DECLARATION_ID };
    };

    struct RegisterSiegeWar final
    {
        SiegeWarData _data;
        SiegeWar::Clock::time_point _scheduledAt;
        GuildId _initialDefenderGuildId{ INVALID_GUILD_ID };
    };

    struct SiegeWarSnapshotUpdated final
    {
        SiegeWarSnapshot _snapshot;
    };
}
