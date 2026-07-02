#include "CorePch.h"
#include "Guild/Guild.h"

Guild::Guild(GuildId const guildId, std::string name, ActorId const leaderActorId)
    : _guildId(guildId)
    , _name(std::move(name))
    , _leaderActorId(leaderActorId)
{
    _members.insert(leaderActorId);
}

GuildSnapshot Guild::CreateSnapshot() const
{
    GuildSnapshot snapshot{
        _guildId,
        _name,
        _leaderActorId,
    };
    snapshot._members.reserve(_members.size());

    for (ActorId const actorId : _members)
    {
        snapshot._members.emplace_back(GuildMemberSnapshot{
            actorId,
            actorId == _leaderActorId ? EGuildRole::Leader : EGuildRole::Member,
        });
    }

    return snapshot;
}

std::string Guild::GetName() const
{
    return _name;
}

bool Guild::IsLeader(ActorId const actorId) const
{
    return _leaderActorId == actorId;
}

bool Guild::Contains(ActorId const actorId) const
{
    return _members.contains(actorId);
}

EGuildOperationError Guild::AddMember(ActorId const actorId)
{
    if (_members.contains(actorId))
    {
        return EGuildOperationError::AlreadyInGuild;
    }

    _members.insert(actorId);
    return EGuildOperationError::None;
}

EGuildOperationError Guild::RemoveMember(
    ActorId const actorId,
    std::optional<ActorId> const successorActorId,
    bool& disbanded)
{
    disbanded = false;

    if (not _members.contains(actorId))
    {
        return EGuildOperationError::NotGuildMember;
    }

    if (actorId != _leaderActorId)
    {
        _members.erase(actorId);
        return EGuildOperationError::None;
    }

    if (_members.size() == 1)
    {
        _members.erase(actorId);
        _leaderActorId = INVALID_ACTOR_ID;
        disbanded = true;
        return EGuildOperationError::None;
    }

    if (not successorActorId)
    {
        return EGuildOperationError::SuccessorRequired;
    }

    if (*successorActorId == actorId || not _members.contains(*successorActorId))
    {
        return EGuildOperationError::SuccessorNotMember;
    }

    _leaderActorId = *successorActorId;
    _members.erase(actorId);
    return EGuildOperationError::None;
}

EGuildOperationError Guild::TransferLeader(
    ActorId const leaderActorId,
    ActorId const successorActorId)
{
    if (_leaderActorId != leaderActorId)
    {
        return EGuildOperationError::NotGuildLeader;
    }

    if (leaderActorId == successorActorId || not _members.contains(successorActorId))
    {
        return EGuildOperationError::SuccessorNotMember;
    }

    _leaderActorId = successorActorId;
    return EGuildOperationError::None;
}
