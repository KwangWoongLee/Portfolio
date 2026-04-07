#pragma once
#include "CorePch.h"

class IOCPSession;

class Connector final
{
public:
    using FuncCreateSession = std::function<std::shared_ptr<IOCPSession>()>;

    Connector(std::string const& ip, uint16_t const port, FuncCreateSession&& funcCreateSession)
        : _ip(ip), _port(port), _funcCreateSession(std::move(funcCreateSession))
    {
    }

    void AsyncConnect() const;

private:
    std::string _ip;
    uint16_t _port;
    FuncCreateSession _funcCreateSession;
};
