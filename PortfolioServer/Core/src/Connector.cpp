#include "CorePch.h"
#include "Connector.h"
#include "SocketUtil.h"
#include "IOCPSessionManager.h"
#include "IOCPSession.h"
#include "ObjectPool.h"
#include "SocketAddress.h"

void Connector::AsyncConnect() const
{
    auto const session = _funcCreateSession();
    if (not session)
    {
        return;
    }

    auto const socket = reinterpret_cast<SOCKET>(session->GetHandle());

    if (not SocketUtil::Singleton::GetInstance().SetReuseAddress(socket, true)
        || not SocketUtil::Singleton::GetInstance().SetLinger(socket, 0, 0)
        || not SocketUtil::Singleton::GetInstance().SetTcpNoDelay(socket, true))
    {
        IOCPSessionManager::Singleton::GetInstance().ReleaseSession(session->GetSessionId());
        return;
    }

    SocketAddress bindAddr(static_cast<uint16_t>(0));
    if (not SocketUtil::Singleton::GetInstance().Bind(socket, bindAddr))
    {
        IOCPSessionManager::Singleton::GetInstance().ReleaseSession(session->GetSessionId());
        return;
    }

    auto* const ioEvent = Overlapped::GetObjectPoolIOEvent(EIOType::Connect, session);

    SocketAddress serverAddress(_ip, _port);

    DWORD numOfBytes = 0;
    if (not fnConnectEx(
        socket,
        serverAddress.GetAsSockAddr(),
        static_cast<int>(serverAddress.GetSize()),
        nullptr, 0, &numOfBytes,
        reinterpret_cast<LPOVERLAPPED>(&(*ioEvent))))
    {
        if (WSA_IO_PENDING != WSAGetLastError())
        {
            ObjectPool<Overlapped>::Singleton::GetInstance().Release(ioEvent);
            IOCPSessionManager::Singleton::GetInstance().ReleaseSession(session->GetSessionId());
            return;
        }
    }
}
