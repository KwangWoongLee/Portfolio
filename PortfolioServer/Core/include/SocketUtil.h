#pragma once
#include "stdafx.h"

class SocketAddress final
{
public:
    SocketAddress() = default;

    explicit SocketAddress(uint16_t const port)
    {
        Init(INADDR_ANY, port);
    }

    SocketAddress(std::string const& ip, uint16_t const port)
    {
        IN_ADDR addr;
        if (::InetPtonA(AF_INET, ip.c_str(), &addr) != 1)
        {
            // TODO: log invalid IP
            ::ZeroMemory(&_sockAddr, sizeof(_sockAddr));
            return;
        }

        Init(addr.S_un.S_addr, port);
    }

    void Init(uint32_t ipAddrNBO, uint16_t port)
    {
        ::ZeroMemory(&_sockAddr, sizeof(_sockAddr));
        _sockAddr.sin_family = AF_INET;
        _sockAddr.sin_addr.s_addr = ipAddrNBO;
        _sockAddr.sin_port = ::htons(port);
    }

    void SetFromSockAddr(SOCKADDR const* addr)
    {
        ::memcpy(&_sockAddr, addr, sizeof(SOCKADDR_IN));
    }

    SOCKADDR const* GetAsSockAddr() const
    {
        return reinterpret_cast<SOCKADDR const*>(&_sockAddr);
    }

    SOCKADDR* GetAsSockAddr()
    {
        return reinterpret_cast<SOCKADDR*>(&_sockAddr);
    }

    size_t GetSize() const
    {
        return sizeof(SOCKADDR_IN);
    }

    std::string ToString() const
    {
        char ipStr[INET_ADDRSTRLEN]{};
        ::InetNtopA(AF_INET, const_cast<IN_ADDR*>(&_sockAddr.sin_addr), ipStr, INET_ADDRSTRLEN);
        return std::format("{}:{}", ipStr, ::ntohs(_sockAddr.sin_port));
    }

    SOCKADDR_IN const& GetRaw() const
    {
        return _sockAddr;
    }
    SOCKADDR_IN& GetRaw()
    {
        return _sockAddr;
    }

private:
    SOCKADDR_IN _sockAddr{};
};


extern LPFN_CONNECTEX		FnConnectEx;
extern LPFN_DISCONNECTEX	FnDisconnectEx;
extern LPFN_ACCEPTEX		FnAcceptEx;

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
	bool Init();

	SOCKET CreateSocket() const;
	void CloseSocket(SOCKET const& socket) const;
	bool Bind(SOCKET const& socket) const;
	bool Listen(SOCKET const& socket, int32_t const backlog = SOMAXCONN) const;

	bool SetLinger(SOCKET const& socket, uint16_t const onoff, uint16_t const linger) const;
	bool SetReuseAddress(SOCKET const& socket, bool const flag) const;
	bool SetRecvBufferSize(SOCKET const& socket, int32_t const size) const;
	bool SetSendBufferSize(SOCKET const& socket, int32_t const size) const;
	bool SetKeepAlive(SOCKET const& socket, bool const flag) const;
	bool SetTcpNoDelay(SOCKET const& socket, bool const flag) const;
	bool SetUpdateAcceptSocket(SOCKET const& socket, SOCKET const listenSocket) const;

private:
	bool	setExFunction();

	template<typename T>
	static bool SetSockOpt(SOCKET const& socket, int32_t const level, int32_t const optName, T const& optVal)
	{
		return SOCKET_ERROR != ::setsockopt(socket, level, optName, reinterpret_cast<char const*>(&optVal), sizeof(T));
	}
};