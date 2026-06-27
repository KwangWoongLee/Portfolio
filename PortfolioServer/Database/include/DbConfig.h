#pragma once
#include "CorePch.h"

struct DbConfig final
{
    static DbConfig FromEnvironment();

    bool IsValid() const;

    bool _enabled{ false };
    std::string _host{ "127.0.0.1" };
    uint16_t _port{ 3306 };
    std::string _user{ "portfolio_server" };
    std::string _password;
    std::string _schema{ "portfolio" };
    uint32_t _connectionCount{ 2 };
};
