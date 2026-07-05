#pragma once
#include "CorePch.h"
#include "DbCompletionTarget.h"
#include "Player.h"
#include "WorldTypes.h"

class PlayerManager final
{
public:
    using Singleton = Singleton<PlayerManager>;
    using CharacterLoadCompleted = std::function<void(CharacterLoadResult)>;

    bool Initialize(std::shared_ptr<ICharacterRepository> characterRepository);
    void Shutdown();

    bool CreateCharacterAsync(
        WorldId worldId,
        DbCompletionTarget completionTarget,
        CharacterLoadCompleted completed);
    ActorId CreateLoaded(
        std::shared_ptr<IOCPSession> const& session,
        CharacterProfile const& profile);
    size_t GetCount() const;

private:
    friend class PlayerTaskRunner;

    void RemoveInternal(ActorId const actorId);
    std::shared_ptr<Player> FindInternal(ActorId const actorId) const;

    mutable std::shared_mutex _mutex;
    std::unordered_map<ActorId, std::shared_ptr<Player>> _players;
    std::shared_ptr<ICharacterRepository> _characterRepository;
};
