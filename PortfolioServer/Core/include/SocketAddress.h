#pragma once

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

    void Init(uint32_t const ipAddr, uint16_t const port)
    {
        ::ZeroMemory(&_sockAddr, sizeof(_sockAddr));
        _sockAddr.sin_family = AF_INET;
        _sockAddr.sin_addr.s_addr = ipAddr;
        _sockAddr.sin_port = ::htons(port);
    }

    void SetFro_sockAddr(SOCKADDR const* addr)
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
    SOCKADDR_IN _sockAddr;
};
