#pragma once
#include "CorePch.h"
#include "Player.h"

class PlayerManager final
{
public:
    using Singleton = Singleton<PlayerManager>;

    std::shared_ptr<Player> Create(std::shared_ptr<IOCPSession> const& session);
    void Remove(ActorId const actorId);
    std::shared_ptr<Player> Find(ActorId const actorId) const;

private:
    mutable std::shared_mutex _mutex;
    std::unordered_map<ActorId, std::shared_ptr<Player>> _players;
};
