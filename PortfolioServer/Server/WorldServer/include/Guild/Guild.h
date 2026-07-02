#pragma once
#include "Guild/GuildTypes.h"

class Guild final
{
public:
    Guild(GuildId guildId, std::string name, ActorId leaderActorId);

    Guild(Guild const&) = delete;
    Guild& operator=(Guild const&) = delete;
    Guild(Guild&&) = delete;
    Guild& operator=(Guild&&) = delete;

    GuildSnapshot CreateSnapshot() const;
    std::string GetName() const;
    bool IsLeader(ActorId actorId) const;
    bool Contains(ActorId actorId) const;

private:
    friend class GuildManager;

    EGuildOperationError AddMember(ActorId actorId);
    EGuildOperationError RemoveMember(
        ActorId actorId,
        std::optional<ActorId> successorActorId,
        bool& disbanded);
    EGuildOperationError TransferLeader(ActorId leaderActorId, ActorId successorActorId);

    GuildId const _guildId;
    std::string const _name;

    ActorId _leaderActorId{ INVALID_ACTOR_ID };
    std::unordered_set<ActorId> _members;
};
