#pragma once
#include "CorePch.h"

using ActorId = StrongId<struct ActorIdTag, int64_t>;

inline ActorId constexpr INVALID_ACTOR_ID{ 0 };

class Actor
{
public:
    explicit Actor(ActorId const actorId) : _actorId(actorId) {}
    virtual ~Actor() = default;

    Actor(Actor const&) = delete;
    Actor& operator=(Actor const&) = delete;
    Actor(Actor&&) = delete;
    Actor& operator=(Actor&&) = delete;

    ActorId GetActorId() const { return _actorId; }

private:
    ActorId const _actorId;
};

class ActorIdGenerator final
{
public:
    static ActorId Generate()
    {
        static std::atomic<int64_t> nextId{ 1 };
        return ActorId{ nextId.fetch_add(1, std::memory_order_relaxed) };
    }
};
