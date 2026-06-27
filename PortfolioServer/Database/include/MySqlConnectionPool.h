#pragma once
#include "CorePch.h"
#include "DbConfig.h"
#include "MySqlConnection.h"

class MySqlConnectionPool;

class MySqlConnectionLease final
{
public:
    ~MySqlConnectionLease();

    MySqlConnectionLease(MySqlConnectionLease const&) = delete;
    MySqlConnectionLease& operator=(MySqlConnectionLease const&) = delete;
    MySqlConnectionLease(MySqlConnectionLease&& other) noexcept;
    MySqlConnectionLease& operator=(MySqlConnectionLease&& other) noexcept;

    MySqlConnection& GetConnection() const;

private:
    friend class MySqlConnectionPool;

    MySqlConnectionLease(MySqlConnectionPool* pool, MySqlConnection* connection);
    void Reset();

    MySqlConnectionPool* _pool{};
    MySqlConnection* _connection{};
};

class MySqlConnectionPool final
{
public:
    bool Initialize(DbConfig const& config);
    void Shutdown();

    std::optional<MySqlConnectionLease> Acquire(
        std::chrono::milliseconds timeout = std::chrono::seconds(1));

    bool IsInitialized() const;
    size_t GetConnectionCount() const;
    std::string const& GetLastError() const { return _lastError; }

private:
    friend class MySqlConnectionLease;

    void Release(MySqlConnection* connection);

    mutable std::mutex _mutex;
    std::condition_variable _condition;
    std::vector<std::unique_ptr<MySqlConnection>> _connections;
    std::deque<MySqlConnection*> _availableConnections;
    size_t _activeLeaseCount{};
    bool _accepting{};
    bool _stopping{};
    std::string _lastError;
};