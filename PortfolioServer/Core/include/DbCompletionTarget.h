#pragma once
#include "CorePch.h"

enum class EDbCompletionTargetType : uint8_t
{
    None = 0,
    NetworkSession,
    Actor,
};

struct DbCompletionTarget final
{
    EDbCompletionTargetType _type{ EDbCompletionTargetType::None };
    int64_t _id{ 0 };

    static DbCompletionTarget NetworkSession(SessionId const sessionId)
    {
        return Raw(EDbCompletionTargetType::NetworkSession, static_cast<int64_t>(sessionId));
    }

    static DbCompletionTarget Actor(int64_t const actorId)
    {
        return Raw(EDbCompletionTargetType::Actor, actorId);
    }

    static DbCompletionTarget Raw(EDbCompletionTargetType const type, int64_t const id)
    {
        DbCompletionTarget target;
        target._type = type;
        target._id = id;
        return target;
    }

    bool IsValid() const
    {
        return EDbCompletionTargetType::None != _type && _id > 0;
    }
};