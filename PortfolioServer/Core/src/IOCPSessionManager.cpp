#include "CorePch.h"
#include "IOCPSessionManager.h"

#include "IOCP.h"
#include "SocketUtil.h"

void IOCPSessionManager::Init(std::shared_ptr<IOCP> const& iocp)
{
	_iocp = iocp;
}

std::shared_ptr<IOCPSession> IOCPSessionManager::CreateSession()
{
    if (not _iocp)
    {
		//TODO: log
        return nullptr;
    }

    auto const iocpSession = ObjectPool<IOCPSession>::Singleton::GetInstance().AcquireShared();
    iocpSession->SetHandle(reinterpret_cast<HANDLE>(SocketUtil::Singleton::GetInstance().CreateSocket()));

    if (not Add(iocpSession))
    {
        return nullptr;
    }

    if (not _iocp->RegisterForCompletionPort(iocpSession->GetHandle()))
    {
        Remove(iocpSession->GetSessionId());
        return nullptr;
    }

    return iocpSession;
}

void IOCPSessionManager::ReleaseSession(SessionId const sessionId)
{
    if (not _iocp)
    {
        //TODO: log
    }

    if (auto const baseSession = Remove(sessionId))
    {
        if (auto const iocpSession = std::dynamic_pointer_cast<IOCPSession>(baseSession))
        {
            SocketUtil::Singleton::GetInstance().CloseSocket(reinterpret_cast<SOCKET>(iocpSession->GetHandle()));
            iocpSession->SetHandle(nullptr);
        }
    }
}
