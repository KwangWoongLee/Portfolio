#pragma once
#include <cstdint>
#include <functional>
#include <ostream>

template <typename T_TAG, typename T_VALUE = int32_t>
struct StrongId final
{
    T_VALUE _value{};

    bool operator==(StrongId const&) const = default;
    bool operator<(StrongId const& other) const { return _value < other._value; }

    template <typename T_OTHER_TAG, typename T_OTHER_VALUE>
    bool operator==(StrongId<T_OTHER_TAG, T_OTHER_VALUE> const&) const = delete;

    constexpr operator T_VALUE() const { return _value; }

    friend std::ostream& operator<<(std::ostream& os, StrongId const& id)
    {
        return os << id._value;
    }
};

template <typename T_TAG, typename T_VALUE = int32_t>
struct StrongIdHash final
{
    size_t operator()(StrongId<T_TAG, T_VALUE> const& id) const
    {
        return std::hash<T_VALUE>{}(id._value);
    }
};

namespace std
{
    template <typename T_TAG, typename T_VALUE>
    struct hash<StrongId<T_TAG, T_VALUE>>
    {
        size_t operator()(StrongId<T_TAG, T_VALUE> const& id) const
        {
            return std::hash<T_VALUE>{}(id._value);
        }
    };
}
