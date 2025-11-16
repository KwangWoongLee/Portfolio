#pragma once

#include "stdafx.h"
#include "SocketUtil.h"
#include "IOCPSessionManager.h"
#include "IOCPSession.h"
#include "ObjectPool.h"

class Connector final
{
public:
    Connector(std::string const& ip, uint16_t const port)
        : _ip(ip), _port(port)
    {
    }

    void AsyncConnect() const
    {
        auto const session = IOCPSessionManager::Singleton::Instance().CreateSession();

        auto* const ioEvent = Overlapped::GetObjectPoolIOEvent(EIOType::CONNECT, session);
        auto const socket = reinterpret_cast<SOCKET>(session->GetHandle());

        if (not SocketUtil::Singleton::Instance().SetReuseAddress(socket, true))
        {
            return;
        }
        if (not SocketUtil::Singleton::Instance().SetLinger(socket, 0, 0))
        {
            return;
        }
        if (not SocketUtil::Singleton::Instance().SetTcpNoDelay(socket, true))
        {
            return;
        }

        SocketAddress serverAddress(_ip, _port);

        DWORD numOfBytes = 0;
        if (not FnConnectEx(
            socket,
            serverAddress.GetAsSockAddr(),
            static_cast<int>(serverAddress.GetSize()),
            nullptr, 0, &numOfBytes,
            reinterpret_cast<LPOVERLAPPED>(&(*ioEvent))))
        {
            if (WSAGetLastError() != WSA_IO_PENDING)
            {
                ObjectPool<Overlapped>::Singleton::Instance().Release(ioEvent);
                return;
            }
        }
    }

private:
    std::string _ip;
    uint16_t _port;
};
