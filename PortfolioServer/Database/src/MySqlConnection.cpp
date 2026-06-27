#include "CorePch.h"
#include "MySqlConnection.h"
#include <mysql/jdbc.h>

MySqlConnection::MySqlConnection() = default;

MySqlConnection::~MySqlConnection()
{
    Disconnect();
}

bool MySqlConnection::Connect(DbConfig const& config)
{
    Disconnect();

    try
    {
        auto* driver = sql::mysql::get_driver_instance();
        _connection.reset(driver->connect(
            std::format("tcp://{}:{}", config._host, config._port),
            config._user,
            config._password));
        _connection->setSchema(config._schema);
    }
    catch (sql::SQLException const& exception)
    {
        _lastError = std::format(
            "connect failed: code={}, state={}, message={}",
            exception.getErrorCode(),
            exception.getSQLState(),
            exception.what());
        Disconnect();
        return false;
    }

    _lastError.clear();
    return true;
}

void MySqlConnection::Disconnect()
{
    _connection.reset();
}


sql::Connection& MySqlConnection::GetNativeConnection()
{
    return *_connection;
}
