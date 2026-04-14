#include "CorePch.h"
#include "IOCPSessionManager.h"

void IOCPSessionManager::Init(std::shared_ptr<IOCP> const& iocp)
{
    if (not iocp)
    {
        assert(false);
    }

    _iocp = iocp;
}

void IOCPSessionManager::ReleaseSession(SessionId const sessionId)
{
    std::unique_lock lock(_mutex);
    _sessions.erase(sessionId);
}

std::shared_ptr<IOCPSession> IOCPSessionManager::Find(SessionId const sessionId) const
{
    std::shared_lock lock(_mutex);

    auto const iter = _sessions.find(sessionId);
    if (_sessions.end() == iter)
    {
        return nullptr;
    }

    return iter->second;
}
