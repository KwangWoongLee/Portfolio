#pragma once
#include "IOCPSession.h"
#include "ObjectPool.h"
#include "Singleton.h"

class IOCPSessionManager
{
public:
    using Singleton = Singleton<IOCPSessionManager>;

public:
    std::shared_ptr<IOCPSession> CreateSession()
    {
        auto const iocpSession = ObjectPool<IOCPSession>::Singleton::Instance().AcquireShared();
        iocpSession->SetHandle(reinterpret_cast<HANDLE>(SocketUtil::Singleton::Instance().CreateSocket()));

        if (not _iocp->RegistForCompletionPort(iocpSession))
        {
            return nullptr;
        }

        iocpSession->SetSessionId(NEXT_SESSION_ID);
        _sessions.insert({NEXT_SESSION_ID++, iocpSession});
        
        return iocpSession;
    }

    void ReleaseSession(SessionId const sessionId)
    {
        auto const iter = _sessions.find(sessionId);
        if (_sessions.end() == iter)
        {
            //TODO: log
            return;
        }

        auto const iocpSession = iter->second;

        SocketUtil::Singleton::Instance().CloseSocket(reinterpret_cast<SOCKET>(iocpSession->GetHandle()));
        iocpSession->SetHandle(nullptr);

        _sessions.erase(iter);
    }

private:
    std::shared_ptr<IOCP> _iocp;

    int64_t NEXT_SESSION_ID{ 1 };
    std::unordered_map<SessionId, std::shared_ptr<IOCPSession>> _sessions;
};
