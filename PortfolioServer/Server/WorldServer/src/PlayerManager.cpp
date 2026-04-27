#include "CorePch.h"
#include "PlayerManager.h"

std::shared_ptr<Player> PlayerManager::Create(std::shared_ptr<IOCPSession> const& session)
{
    auto const actorId = ActorIdGenerator::Generate();
    auto player = std::make_shared<Player>(actorId, session);

    std::unique_lock lock(_mutex);
    _players.emplace(actorId, player);

    return player;
}

void PlayerManager::Remove(ActorId const actorId)
{
    std::unique_lock lock(_mutex);
    _players.erase(actorId);
}

std::shared_ptr<Player> PlayerManager::Find(ActorId const actorId) const
{
    std::shared_lock lock(_mutex);

    auto const iter = _players.find(actorId);
    if (_players.end() == iter)
    {
        return nullptr;
    }

    return iter->second;
}
