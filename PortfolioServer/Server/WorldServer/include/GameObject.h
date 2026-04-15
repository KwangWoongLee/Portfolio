#pragma once
#include "WorldTypes.h"

class GameObject
{
public:
    explicit GameObject(EntityId const entityId)
        : _entityId(entityId)
    {
    }

    virtual ~GameObject() = default;

    EntityId GetEntityId() const { return _entityId; }

    Position const& GetPosition() const { return _position; }
    void SetPosition(Position const& pos) { _position = pos; }

protected:
    EntityId _entityId{};
    Position _position{};
};
