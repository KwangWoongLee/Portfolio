#pragma once
#include "CorePch.h"
#include "DbConfig.h"

namespace sql
{
    class Connection;
}

class MySqlConnection final
{
public:
    MySqlConnection();
    ~MySqlConnection();

    MySqlConnection(MySqlConnection const&) = delete;
    MySqlConnection& operator=(MySqlConnection const&) = delete;
    MySqlConnection(MySqlConnection&&) = delete;
    MySqlConnection& operator=(MySqlConnection&&) = delete;

    bool Connect(DbConfig const& config);
    void Disconnect();

    sql::Connection& GetNativeConnection();
    bool IsConnected() const { return nullptr != _connection; }
    std::string const& GetLastError() const { return _lastError; }

private:
    std::unique_ptr<sql::Connection> _connection;
    std::string _lastError;
};
