#include "CorePch.h"
#include "SocketUtil.h"

#include "SocketAddress.h"

LPFN_CONNECTEX fnConnectEx = nullptr;
LPFN_DISCONNECTEX fnDisconnectEx = nullptr;
LPFN_ACCEPTEX fnAcceptEx = nullptr;

bool SocketUtil::Init()
{
	WSADATA WSAData;

	if (WSAStartup(MAKEWORD(2, 2), &WSAData) != 0)
		return false;

	if (SetExFunction() == false)
		return false;

	return true;
}

SOCKET SocketUtil::CreateSocket()
{
	return ::WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
}

void SocketUtil::CloseSocket(SOCKET const socket)
{
	::closesocket(socket);
}

bool SocketUtil::Bind(SOCKET const socket, SocketAddress const socketAddress)
{
	if (SOCKET_ERROR == ::bind(socket, socketAddress.GetAsSockAddr(), static_cast<int>(socketAddress.GetSize())))
	{
		return false;
	}

	return true;
}

bool SocketUtil::Listen(SOCKET const socket, int32_t const backlog)
{
	if (::listen(socket, backlog) == SOCKET_ERROR)
	{
		return false;
	}

	return true;
}


bool SocketUtil::SetLinger(SOCKET const socket, uint16_t const onOff, uint16_t const linger)
{
	LINGER option;
	option.l_onoff = onOff;
	option.l_linger = linger;
	return SetSockOpt(socket, SOL_SOCKET, SO_LINGER, option);
}

bool SocketUtil::SetReuseAddress(SOCKET const socket, bool const flag)
{
	return SetSockOpt(socket, SOL_SOCKET, SO_REUSEADDR, flag);
}

bool SocketUtil::SetRecvBufferSize(SOCKET const socket, int32_t const size)
{
	return SetSockOpt(socket, SOL_SOCKET, SO_RCVBUF, size);
}

bool SocketUtil::SetSendBufferSize(SOCKET const socket, int32_t const size)
{
	return SetSockOpt(socket, SOL_SOCKET, SO_SNDBUF, size);
}

bool SocketUtil::SetKeepAlive(SOCKET const socket, bool const flag)
{
	return SetSockOpt(socket, SOL_SOCKET, SO_KEEPALIVE, flag);
}

bool SocketUtil::SetTcpNoDelay(SOCKET const socket, bool const flag)
{
	return SetSockOpt(socket, SOL_SOCKET, TCP_NODELAY, flag);
}

bool SocketUtil::SetUpdateAcceptSocket(SOCKET const socket, SOCKET const listenSocket)
{
	return SetSockOpt(socket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, listenSocket);
}

bool SocketUtil::SetExFunction()
{
	DWORD bytes = 0;
	auto const tmpSocket = CreateSocket();

	GUID guidAcceptEx = WSAID_ACCEPTEX;
	if (SOCKET_ERROR == WSAIoctl(tmpSocket, SIO_GET_EXTENSION_FUNCTION_POINTER,
		&guidAcceptEx, sizeof(GUID), &fnAcceptEx, sizeof(LPFN_ACCEPTEX), &bytes, nullptr, nullptr))
		return false;

	GUID guidConnectEx = WSAID_CONNECTEX;
	if (SOCKET_ERROR == WSAIoctl(tmpSocket, SIO_GET_EXTENSION_FUNCTION_POINTER,
		&guidConnectEx, sizeof(GUID), &fnConnectEx, sizeof(LPFN_CONNECTEX), &bytes, nullptr, nullptr))
		return false;

	GUID guidDisconnectEx = WSAID_DISCONNECTEX;
	if (SOCKET_ERROR == WSAIoctl(tmpSocket, SIO_GET_EXTENSION_FUNCTION_POINTER,
		&guidDisconnectEx, sizeof(GUID), &fnDisconnectEx, sizeof(LPFN_DISCONNECTEX), &bytes, nullptr, nullptr))
		return false;

	::closesocket(tmpSocket);

	return true;
}
