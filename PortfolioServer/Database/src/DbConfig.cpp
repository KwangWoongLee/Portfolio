#include "CorePch.h"
#include "DbConfig.h"
#include <cstdlib>
#include <limits>

namespace
{
    std::optional<std::string> ReadEnvironment(char const* const name)
    {
        char* value{ nullptr };
        size_t length{ 0 };
        if (0 != ::_dupenv_s(&value, &length, name) || nullptr == value)
        {
            return std::nullopt;
        }

        auto holder = std::unique_ptr<char, decltype(&std::free)>{ value, &std::free };
        return std::string(value);
    }

    bool ReadEnabled(std::optional<std::string> const& value)
    {
        return value and (*value == "1" || *value == "true" || *value == "TRUE");
    }

    template <typename T_VALUE>
    void ReadUnsigned(std::optional<std::string> const& value, T_VALUE& output)
    {
        if (not value)
        {
            return;
        }

        try
        {
            auto const parsed = std::stoull(*value);
            if (parsed <= (std::numeric_limits<T_VALUE>::max)())
            {
                output = static_cast<T_VALUE>(parsed);
            }
        }
        catch (std::exception const&)
        {
        }
    }
}

DbConfig DbConfig::FromEnvironment()
{
    DbConfig config;
    config._enabled = ReadEnabled(ReadEnvironment("PORTFOLIO_DB_ENABLED"));

    if (auto value = ReadEnvironment("PORTFOLIO_DB_HOST"))
    {
        config._host = std::move(*value);
    }
    if (auto value = ReadEnvironment("PORTFOLIO_DB_USER"))
    {
        config._user = std::move(*value);
    }
    if (auto value = ReadEnvironment("PORTFOLIO_DB_PASSWORD"))
    {
        config._password = std::move(*value);
    }
    if (auto value = ReadEnvironment("PORTFOLIO_DB_SCHEMA"))
    {
        config._schema = std::move(*value);
    }

    ReadUnsigned(ReadEnvironment("PORTFOLIO_DB_PORT"), config._port);
    ReadUnsigned(ReadEnvironment("PORTFOLIO_DB_CONNECTIONS"), config._connectionCount);
    return config;
}

bool DbConfig::IsValid() const
{
    if (not _enabled)
    {
        return true;
    }

    return not _host.empty()
        && 0 != _port
        && not _user.empty()
        && not _schema.empty()
        && 0 != _connectionCount;
}
