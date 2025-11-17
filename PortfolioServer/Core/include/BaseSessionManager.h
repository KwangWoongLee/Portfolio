#pragma once
#include "stdafx.h"

template <typename T_SESSION>
class BaseSessionManager
{
public:
    using SessionId = int64_t;

    virtual ~BaseSessionManager() = default;

    void Add(SessionId const id, std::shared_ptr<T_SESSION> const& session)
    {
        std::scoped_lock lock(_mutex);
        _sessions.emplace(id, session);
    }

    void Remove(SessionId const id)
    {
        std::scoped_lock lock(_mutex);
        _sessions.erase(id);
    }

    std::shared_ptr<T_SESSION> Find(SessionId const id) const
    {
        std::scoped_lock lock(_mutex);
        auto it = _sessions.find(id);
        return (it != _sessions.end()) ? it->second : nullptr;
    }

    void ForEach(std::function<void(std::shared_ptr<T_SESSION> const&)> const& fn) const
    {
        std::scoped_lock lock(_mutex);
        for (auto const& [id, session] : _sessions)
        {
            fn(session);
        }
    }

protected:
    mutable std::shared_mutex _mutex;
    std::unordered_map<SessionId, std::shared_ptr<T_SESSION>> _sessions;
};
