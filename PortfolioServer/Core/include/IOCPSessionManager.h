#pragma once
#include "CorePch.h"

#include "IOCP.h"
#include "IOCPSession.h"
#include "SocketUtil.h"

class IOCPSessionManager
{
public:
    using Singleton = Singleton<IOCPSessionManager>;

public:
    void Init(std::shared_ptr<IOCP> const& iocp);

    void ReleaseSession(SessionId const sessionId);

    std::shared_ptr<IOCPSession> Find(SessionId const sessionId) const;


    template <typename T_SESSION>
    std::shared_ptr<T_SESSION> CreateSession()
    {
        static_assert(std::is_base_of_v<IOCPSession, T_SESSION>);

        if (not _iocp)
        {
            return nullptr;
        }

        auto const session = ObjectPool<T_SESSION>::Singleton::GetInstance().AcquireShared();
        session->SetHandle(reinterpret_cast<HANDLE>(SocketUtil::Singleton::GetInstance().CreateSocket()));

        if (not _iocp->RegisterForCompletionPort(session->GetHandle()))
        {
            SocketUtil::Singleton::GetInstance().CloseSocket(reinterpret_cast<SOCKET>(session->GetHandle()));
			session->SetHandle(INVALID_HANDLE_VALUE);

            return nullptr;
        }

        {
            std::unique_lock lock(_mutex);

            auto const newId = _nextSessionId++;
            session->SetSessionId(newId);
            if (auto const [_, isSuccess] = _sessions.try_emplace(newId, session); not isSuccess)
            {
                //TODO:: CompletionPort Remove ?
                return nullptr;
            }
        }

        return session;
    }

    template <typename Fn>
    void ForEach(Fn const& fn) const
    {
        std::unique_lock lock(_mutex);
        for (auto const& [id, session] : _sessions)
        {
            fn(session);
        }
    }

private:
    mutable std::shared_mutex _mutex;

    SessionId _nextSessionId{ 1 };
    std::unordered_map<SessionId, std::shared_ptr<IOCPSession>> _sessions;

    std::shared_ptr<IOCP> _iocp;
};
