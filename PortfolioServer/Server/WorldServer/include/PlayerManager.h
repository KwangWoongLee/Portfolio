#pragma once
#include "CorePch.h"
#include "Player.h"

class PlayerManager final
{
public:
    using Singleton = Singleton<PlayerManager>;

    ActorId Create(std::shared_ptr<IOCPSession> const& session);
    size_t GetCount() const;

private:
    friend class PlayerTaskRunner;

    void RemoveInternal(ActorId const actorId);
    std::shared_ptr<Player> FindInternal(ActorId const actorId) const;

    mutable std::shared_mutex _mutex;
    std::unordered_map<ActorId, std::shared_ptr<Player>> _players;
};
