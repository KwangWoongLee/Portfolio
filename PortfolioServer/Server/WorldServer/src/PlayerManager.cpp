#include "CorePch.h"
#include <random>
#include "PlayerManager.h"

namespace
{
    auto constexpr SPAWN_AREA_HALF_EXTENT = 1000.0f;
}

bool PlayerManager::Initialize(std::shared_ptr<ICharacterRepository> characterRepository)
{
    if (not characterRepository)
    {
        return false;
    }

    std::unique_lock lock(_mutex);
    _characterRepository = std::move(characterRepository);
    return true;
}

void PlayerManager::Shutdown()
{
    std::unique_lock lock(_mutex);
    _players.clear();
    _characterRepository.reset();
}

ActorId PlayerManager::Create(
    std::shared_ptr<IOCPSession> const& session,
    CharacterId const characterId)
{
    static thread_local std::mt19937 rng(std::random_device{}());
    static thread_local std::uniform_real_distribution<float> dist(
        -SPAWN_AREA_HALF_EXTENT, SPAWN_AREA_HALF_EXTENT);

    std::shared_ptr<ICharacterRepository> characterRepository;
    {
        std::shared_lock lock(_mutex);
        characterRepository = _characterRepository;
    }
    if (not characterRepository || INVALID_CHARACTER_ID == characterId)
    {
        return INVALID_ACTOR_ID;
    }

    auto const actorId = ActorIdGenerator::Generate();
    auto player = std::make_shared<Player>(
        actorId,
        characterId,
        session,
        std::move(characterRepository));
    player->SetPosition({ dist(rng), dist(rng) });

    std::unique_lock lock(_mutex);
    _players.emplace(actorId, player);

    return actorId;
}

void PlayerManager::RemoveInternal(ActorId const actorId)
{
    std::unique_lock lock(_mutex);
    _players.erase(actorId);
}

std::shared_ptr<Player> PlayerManager::FindInternal(ActorId const actorId) const
{
    std::shared_lock lock(_mutex);

    auto const iter = _players.find(actorId);
    if (_players.end() == iter)
    {
        return nullptr;
    }

    return iter->second;
}

size_t PlayerManager::GetCount() const
{
    std::shared_lock lock(_mutex);
    return _players.size();
}