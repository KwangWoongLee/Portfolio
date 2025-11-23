#pragma once
#include "CorePch.h"

#include "BaseSession.h"

class BaseSessionManager
{
public:
    virtual ~BaseSessionManager() = default;

    void ForEach(std::function<void(std::shared_ptr<BaseSession> const&)> const& fn) const
    {
        std::scoped_lock lock(_mutex);
        for (auto const& [id, session] : _sessions)
        {
            fn(session);
        }
    }

protected:
    bool Add(std::shared_ptr<BaseSession> const& session)
    {
        std::scoped_lock lock(_mutex);
        auto const [_, isSuccess] = _sessions.try_emplace(_nextSessionId, session);
        if (not isSuccess)
        {
            return false;
        }

        session->SetSessionId(_nextSessionId);

        ++_nextSessionId;
        return true;
    }

    std::shared_ptr<BaseSession> Remove(SessionId const id)
    {
        std::scoped_lock lock(_mutex);
        auto const iter = _sessions.find(id);
        if (iter == _sessions.end())
        {
            return nullptr;
		}

		std::shared_ptr<BaseSession> ret = iter->second;
        _sessions.erase(iter);

        return ret;
    }

private:
    mutable std::shared_mutex _mutex;

    SessionId _nextSessionId{ 1 };
    std::unordered_map<SessionId, std::shared_ptr<BaseSession>> _sessions;
};
