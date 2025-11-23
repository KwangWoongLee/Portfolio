#pragma once
#include "CorePch.h"

extern LPFN_CONNECTEX fnConnectEx;
extern LPFN_DISCONNECTEX fnDisconnectEx;
extern LPFN_ACCEPTEX fnAcceptEx;

class SocketAddress;

class SocketUtil
{
public:
    using Singleton = Singleton<SocketUtil>;

public:
	SocketUtil() = default;
	virtual ~SocketUtil() {
		::WSACleanup();
	};

public:
    static bool Init();

    static SOCKET CreateSocket();
    static void CloseSocket(SOCKET const socket);
    static bool Bind(SOCKET const socket, SocketAddress const socketAddress);
    static bool Listen(SOCKET const socket, int32_t const backlog = SOMAXCONN);

    static bool SetLinger(SOCKET const socket, uint16_t const onoff, uint16_t const linger);
    static bool SetReuseAddress(SOCKET const socket, bool const flag);
    static bool SetRecvBufferSize(SOCKET const socket, int32_t const size);
    static bool SetSendBufferSize(SOCKET const socket, int32_t const size);
    static bool SetKeepAlive(SOCKET const socket, bool const flag);
    static bool SetTcpNoDelay(SOCKET const socket, bool const flag);
    static bool SetUpdateAcceptSocket(SOCKET const socket, SOCKET const listenSocket);

private:
    static bool	SetExFunction();

	template<typename T>
	static bool SetSockOpt(SOCKET const socket, int32_t const level, int32_t const optName, T const& optVal)
	{
		return SOCKET_ERROR != ::setsockopt(socket, level, optName, reinterpret_cast<char const*>(&optVal), sizeof(T));
	}
};