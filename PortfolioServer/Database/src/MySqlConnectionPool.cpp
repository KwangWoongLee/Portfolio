#include "CorePch.h"
#include <utility>
#include "MySqlConnectionPool.h"
#include "MySqlConnection.h"

MySqlConnectionLease::MySqlConnectionLease(
    MySqlConnectionPool* const pool,
    MySqlConnection* const connection)
    : _pool(pool)
    , _connection(connection)
{
}

MySqlConnectionLease::~MySqlConnectionLease()
{
    Reset();
}

MySqlConnectionLease::MySqlConnectionLease(MySqlConnectionLease&& other) noexcept
    : _pool(std::exchange(other._pool, nullptr))
    , _connection(std::exchange(other._connection, nullptr))
{
}

MySqlConnectionLease& MySqlConnectionLease::operator=(MySqlConnectionLease&& other) noexcept
{
    if (this == &other)
    {
        return *this;
    }

    Reset();
    _pool = std::exchange(other._pool, nullptr);
    _connection = std::exchange(other._connection, nullptr);
    return *this;
}

MySqlConnection& MySqlConnectionLease::GetConnection() const
{
    return *_connection;
}

void MySqlConnectionLease::Reset()
{
    if (nullptr == _pool || nullptr == _connection)
    {
        return;
    }

    _pool->Release(_connection);
    _pool = nullptr;
    _connection = nullptr;
}

bool MySqlConnectionPool::Initialize(DbConfig const& config)
{
    Shutdown();

    if (not config.IsValid())
    {
        _lastError = "invalid database configuration";
        return false;
    }

    if (not config._enabled)
    {
        _lastError.clear();
        return true;
    }

    std::vector<std::unique_ptr<MySqlConnection>> connections;
    connections.reserve(config._connectionCount);

    for (uint32_t index = 0; index < config._connectionCount; ++index)
    {
        auto connection = std::make_unique<MySqlConnection>();
        if (not connection->Connect(config))
        {
            _lastError = std::format(
                "database connection {} failed: {}",
                index,
                connection->GetLastError());
            return false;
        }

        connections.emplace_back(std::move(connection));
    }

    {
        std::scoped_lock lock(_mutex);
        _connections = std::move(connections);
        for (auto const& connection : _connections)
        {
            _availableConnections.emplace_back(connection.get());
        }
        _accepting = true;
    }

    _lastError.clear();
    return true;
}

void MySqlConnectionPool::Shutdown()
{
    std::unique_lock lock(_mutex);
    _accepting = false;
    _stopping = true;
    _condition.notify_all();
    _condition.wait(lock, [this]() { return 0 == _activeLeaseCount; });

    _availableConnections.clear();
    _connections.clear();
    _stopping = false;
}

std::optional<MySqlConnectionLease> MySqlConnectionPool::Acquire(
    std::chrono::milliseconds const timeout)
{
    std::unique_lock lock(_mutex);
    if (not _accepting || _stopping)
    {
        return std::nullopt;
    }

    bool const ready = _condition.wait_for(lock, timeout, [this]()
        {
            return _stopping || not _accepting || not _availableConnections.empty();
        });
    if (not ready || _stopping || not _accepting || _availableConnections.empty())
    {
        return std::nullopt;
    }

    auto const connection = _availableConnections.front();
    _availableConnections.pop_front();
    ++_activeLeaseCount;
    return MySqlConnectionLease(this, connection);
}

bool MySqlConnectionPool::IsInitialized() const
{
    std::scoped_lock lock(_mutex);
    return _accepting && not _connections.empty();
}

size_t MySqlConnectionPool::GetConnectionCount() const
{
    std::scoped_lock lock(_mutex);
    return _connections.size();
}

void MySqlConnectionPool::Release(MySqlConnection* const connection)
{
    std::scoped_lock lock(_mutex);
    if (0 < _activeLeaseCount)
    {
        --_activeLeaseCount;
    }

    if (_accepting && not _stopping && nullptr != connection)
    {
        _availableConnections.emplace_back(connection);
    }

    _condition.notify_one();
}