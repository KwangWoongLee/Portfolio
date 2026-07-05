#include "CorePch.h"
#include <random>
#include "DbDispatcher.h"
#include "DbCompletionTarget.h"
#include "PlayerManager.h"
#include "TaskDispatcher.h"
#include "UniqueIdGenerator.h"

namespace
{
    auto constexpr SPAWN_AREA_HALF_EXTENT = 1000.0f;
    auto constexpr DEFAULT_CHARACTER_GOLD = 1'000'000;

    std::string BuildCharacterName(CharacterId const characterId)
    {
        return std::format("Player{}", static_cast<int64_t>(characterId));
    }

    class CharacterLoadCompletedTask final
        : public ITask
    {
    public:
        CharacterLoadCompletedTask(
            SessionId const sessionId,
            PlayerManager::CharacterLoadCompleted completed,
            CharacterLoadResult result)
            : ITask(ETaskType::NetworkIO, static_cast<int64_t>(sessionId))
            , _completed(std::move(completed))
            , _result(std::move(result))
        {
        }

        void Execute() override
        {
            if (_completed)
            {
                _completed(std::move(_result));
            }
        }

    private:
        PlayerManager::CharacterLoadCompleted _completed;
        CharacterLoadResult _result;
    };

    class LoadOrCreateCharacterCommand final
        : public IDbCommand
    {
    public:
        LoadOrCreateCharacterCommand(
            std::shared_ptr<ICharacterRepository> characterRepository,
            CharacterId const characterId,
            std::string characterName,
            DbCompletionTarget completionTarget,
            PlayerManager::CharacterLoadCompleted completed)
            : _characterRepository(std::move(characterRepository))
            , _characterId(characterId)
            , _characterName(std::move(characterName))
            , _completionTarget(completionTarget)
            , _completed(std::move(completed))
        {
        }

        EDbCommandResult Execute() override
        {
            if (not _characterRepository)
            {
                _result = CharacterLoadResult{
                    ECharacterRepositoryError::ConnectionUnavailable,
                    "character repository unavailable",
                };
                return EDbCommandResult::Failed;
            }

            _result = _characterRepository->GetOrCreateCharacter(
                _characterId,
                _characterName,
                DEFAULT_CHARACTER_GOLD);
            return _result.Succeeded() ? EDbCommandResult::Succeeded : EDbCommandResult::Failed;
        }

        void OnCompleted(EDbCommandResult const result) override
        {
            (void)result;
            if (EDbCompletionTargetType::NetworkSession != _completionTarget._type)
            {
                return;
            }

            auto task = std::make_shared<CharacterLoadCompletedTask>(
                SessionId{ _completionTarget._id },
                std::move(_completed),
                std::move(_result));
            (void)TaskDispatcher::Singleton::GetInstance().Dispatch(std::move(task));
        }

    private:
        std::shared_ptr<ICharacterRepository> _characterRepository;
        CharacterId _characterId{ INVALID_CHARACTER_ID };
        std::string _characterName;
        DbCompletionTarget _completionTarget;
        PlayerManager::CharacterLoadCompleted _completed;
        CharacterLoadResult _result{
            ECharacterRepositoryError::QueryFailed,
            "character load was not executed",
        };
    };
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

bool PlayerManager::CreateCharacterAsync(
    WorldId const worldId,
    DbCompletionTarget const completionTarget,
    CharacterLoadCompleted completed)
{
    std::shared_ptr<ICharacterRepository> characterRepository;
    {
        std::shared_lock lock(_mutex);
        characterRepository = _characterRepository;
    }

    if (not characterRepository ||
        INVALID_WORLD_ID == worldId ||
        not completionTarget.IsValid() ||
        not completed)
    {
        return false;
    }

    auto const characterIdValue = UniqueIdGenerator::Generate(
        EUniqueIdKind::Character,
        static_cast<int64_t>(worldId));
    if (not characterIdValue)
    {
        return false;
    }

    auto const characterId = CharacterId{ *characterIdValue };
    auto characterName = BuildCharacterName(characterId);

    return DbDispatcher::Singleton::GetInstance().Enqueue(
        static_cast<int64_t>(characterId),
        std::make_unique<LoadOrCreateCharacterCommand>(
            std::move(characterRepository),
            characterId,
            std::move(characterName),
            completionTarget,
            std::move(completed)));
}

ActorId PlayerManager::CreateLoaded(
    std::shared_ptr<IOCPSession> const& session,
    CharacterProfile const& profile)
{
    static thread_local std::mt19937 rng(std::random_device{}());
    static thread_local std::uniform_real_distribution<float> dist(
        -SPAWN_AREA_HALF_EXTENT, SPAWN_AREA_HALF_EXTENT);

    std::shared_ptr<ICharacterRepository> characterRepository;
    {
        std::shared_lock lock(_mutex);
        characterRepository = _characterRepository;
    }

    if (not session || not characterRepository || not profile.IsValid() || profile._gold < 0)
    {
        return INVALID_ACTOR_ID;
    }

    auto const actorId = ActorId{ static_cast<int64_t>(profile._id) };
    auto player = std::make_shared<Player>(
        actorId,
        profile._id,
        profile._gold,
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
