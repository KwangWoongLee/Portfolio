#pragma once
#include <cstdint>
#include <functional>

template <typename Tag>
struct StrongId final
{
    int32_t _value{};

    bool operator==(StrongId const&) const = default;
    bool operator<(StrongId const& other) const { return _value < other._value; }
};

template <typename Tag>
struct StrongIdHash final
{
    size_t operator()(StrongId<Tag> const& id) const
    {
        return std::hash<int32_t>{}(id._value);
    }
};

