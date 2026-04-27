#pragma once
#include "Actor.h"
#include "WorldTypes.h"

class GameObject
    : public Actor
{
public:
    explicit GameObject(ActorId const actorId)
        : Actor(actorId)
    {
    }

    Position const& GetPosition() const { return _position; }
    void SetPosition(Position const& pos) { _position = pos; }

protected:
    Position _position{};
};
