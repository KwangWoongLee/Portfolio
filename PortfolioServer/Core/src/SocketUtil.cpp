#include "stdafx.h"
#include "SocketUtil.h"

LPFN_CONNECTEX		FnConnectEx = nullptr;
LPFN_DISCONNECTEX	FnDisconnectEx = nullptr;
LPFN_ACCEPTEX		FnAcceptEx = nullptr;

bool SocketUtil::Init()
{
	WSADATA WSAData;

	if (WSAStartup(MAKEWORD(2, 2), &WSAData) != 0)
		return false;

	if (setExFunction() == false)
		return false;

	return true;
}

SOCKET SocketUtil::CreateSocket() const
{
	return ::WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
}

void SocketUtil::CloseSocket(SOCKET const& socket) const
{
	::closesocket(socket);
}

bool SocketUtil::Bind(SOCKET const& socket) const
{
	SocketAddress address(7777); // TODO: config

	if (SOCKET_ERROR == ::bind(socket, address.GetAsSockAddr(), static_cast<int>(address.GetSize())))
	{
		return false;
	}

	return true;
}

bool SocketUtil::Listen(SOCKET const& socket, int32_t const backlog) const
{
	if (::listen(socket, backlog) == SOCKET_ERROR)
	{
		return false;
	}

	return true;
}


bool SocketUtil::SetLinger(SOCKET const& socket, uint16_t const onoff, uint16_t const linger) const
{
	LINGER option;
	option.l_onoff = onoff;
	option.l_linger = linger;
	return SetSockOpt(socket, SOL_SOCKET, SO_LINGER, option);
}

bool SocketUtil::SetReuseAddress(SOCKET const& socket, bool const flag) const
{
	return SetSockOpt(socket, SOL_SOCKET, SO_REUSEADDR, flag);
}

bool SocketUtil::SetRecvBufferSize(SOCKET const& socket, int32_t const size) const
{
	return SetSockOpt(socket, SOL_SOCKET, SO_RCVBUF, size);
}

bool SocketUtil::SetSendBufferSize(SOCKET const& socket, int32_t const size) const
{
	return SetSockOpt(socket, SOL_SOCKET, SO_SNDBUF, size);
}

bool SocketUtil::SetKeepAlive(SOCKET const& socket, bool const flag) const
{
	return SetSockOpt(socket, SOL_SOCKET, SO_KEEPALIVE, flag);
}

bool SocketUtil::SetTcpNoDelay(SOCKET const& socket, bool const flag) const
{
	return SetSockOpt(socket, SOL_SOCKET, TCP_NODELAY, flag);
}

bool SocketUtil::SetUpdateAcceptSocket(SOCKET const& socket, SOCKET const listenSocket) const
{
	return SetSockOpt(socket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, listenSocket);
}

bool SocketUtil::setExFunction()
{
	DWORD bytes = 0;
	SOCKET tmpSocket = this->CreateSocket();

	GUID guidAcceptEx = WSAID_ACCEPTEX;
	if (SOCKET_ERROR == WSAIoctl(tmpSocket, SIO_GET_EXTENSION_FUNCTION_POINTER,
		&guidAcceptEx, sizeof(GUID), &FnAcceptEx, sizeof(LPFN_ACCEPTEX), &bytes, NULL, NULL))
		return false;

	GUID guidConnectEx = WSAID_CONNECTEX;
	if (SOCKET_ERROR == WSAIoctl(tmpSocket, SIO_GET_EXTENSION_FUNCTION_POINTER,
		&guidConnectEx, sizeof(GUID), &FnConnectEx, sizeof(LPFN_CONNECTEX), &bytes, NULL, NULL))
		return false;

	GUID guidDisconnectEx = WSAID_DISCONNECTEX;
	if (SOCKET_ERROR == WSAIoctl(tmpSocket, SIO_GET_EXTENSION_FUNCTION_POINTER,
		&guidDisconnectEx, sizeof(GUID), &FnDisconnectEx, sizeof(LPFN_DISCONNECTEX), &bytes, NULL, NULL))
		return false;

	::closesocket(tmpSocket);
	tmpSocket = INVALID_SOCKET;

	return true;
}
