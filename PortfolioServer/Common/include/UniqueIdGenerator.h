#pragma once
#include "CorePch.h"

enum class EUniqueIdKind : uint8_t
{
    None = 0,
    Character = 1,
    Guild = 2,
    SiegeWar = 3,
    SiegeDeclaration = 4,
    ItemInstance = 5,
    RewardClaim = 6,
};

struct UniqueIdParts final
{
    int64_t _unixTimestampMs{};
    int64_t _scopeId{};
    EUniqueIdKind _kind{ EUniqueIdKind::None };
    int64_t _sequence{};
};

class UniqueIdGenerator final
{
public:
    static std::optional<int64_t> Generate(
        EUniqueIdKind const kind,
        int64_t const scopeId)
    {
        auto const kindValue = static_cast<int64_t>(kind);
        if (kindValue <= 0 || MAX_KIND < kindValue || scopeId <= 0 || MAX_SCOPE_ID < scopeId)
        {
            return std::nullopt;
        }

        std::unique_lock lock(GetMutex());

        int64_t timestampMs{ GetRelativeTimestampMs() };
        if (timestampMs < 0)
        {
            return std::nullopt;
        }

        int64_t& lastTimestampMs = GetLastTimestampMs();
        int64_t& sequence = GetSequence();

        if (timestampMs < lastTimestampMs)
        {
            timestampMs = lastTimestampMs;
        }

        if (timestampMs == lastTimestampMs)
        {
            if (MAX_SEQUENCE <= sequence)
            {
                timestampMs = WaitNextTimestampMs(lastTimestampMs);
                sequence = 0;
            }
            else
            {
                ++sequence;
            }
        }
        else
        {
            sequence = 0;
        }

        if (MAX_TIMESTAMP < timestampMs)
        {
            return std::nullopt;
        }

        lastTimestampMs = timestampMs;

        return (timestampMs << TIMESTAMP_SHIFT)
            | (scopeId << SCOPE_SHIFT)
            | (kindValue << KIND_SHIFT)
            | sequence;
    }

    static std::optional<UniqueIdParts> Decode(int64_t const id)
    {
        if (id <= 0)
        {
            return std::nullopt;
        }

        auto const sequence = id & MAX_SEQUENCE;
        auto const kindValue = (id >> KIND_SHIFT) & MAX_KIND;
        auto const scopeId = (id >> SCOPE_SHIFT) & MAX_SCOPE_ID;
        auto const timestampMs = (id >> TIMESTAMP_SHIFT) & MAX_TIMESTAMP;

        if (kindValue <= 0 || scopeId <= 0)
        {
            return std::nullopt;
        }

        return UniqueIdParts{
            timestampMs + CUSTOM_EPOCH_UNIX_MS,
            scopeId,
            static_cast<EUniqueIdKind>(kindValue),
            sequence,
        };
    }

    static int64_t GetCustomEpochUnixMs()
    {
        return CUSTOM_EPOCH_UNIX_MS;
    }

private:
    UniqueIdGenerator() = delete;

    static int64_t GetRelativeTimestampMs()
    {
        auto const now = std::chrono::system_clock::now();
        auto const unixMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count();
        return unixMs - CUSTOM_EPOCH_UNIX_MS;
    }

    static int64_t WaitNextTimestampMs(int64_t const lastTimestampMs)
    {
        int64_t timestampMs{ GetRelativeTimestampMs() };
        while (timestampMs <= lastTimestampMs)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            timestampMs = GetRelativeTimestampMs();
        }
        return timestampMs;
    }

    static std::mutex& GetMutex()
    {
        static std::mutex mutex;
        return mutex;
    }

    static int64_t& GetLastTimestampMs()
    {
        static int64_t lastTimestampMs{ -1 };
        return lastTimestampMs;
    }

    static int64_t& GetSequence()
    {
        static int64_t sequence{ 0 };
        return sequence;
    }

    static int64_t constexpr CUSTOM_EPOCH_UNIX_MS = 1'767'225'600'000LL; // 2026-01-01 00:00:00 UTC

    static int64_t constexpr SEQUENCE_BITS = 8;
    static int64_t constexpr KIND_BITS = 4;
    static int64_t constexpr SCOPE_BITS = 10;
    static int64_t constexpr TIMESTAMP_BITS = 41;

    static int64_t constexpr MAX_SEQUENCE = (1LL << SEQUENCE_BITS) - 1;
    static int64_t constexpr MAX_KIND = (1LL << KIND_BITS) - 1;
    static int64_t constexpr MAX_SCOPE_ID = (1LL << SCOPE_BITS) - 1;
    static int64_t constexpr MAX_TIMESTAMP = (1LL << TIMESTAMP_BITS) - 1;

    static int64_t constexpr KIND_SHIFT = SEQUENCE_BITS;
    static int64_t constexpr SCOPE_SHIFT = KIND_SHIFT + KIND_BITS;
    static int64_t constexpr TIMESTAMP_SHIFT = SCOPE_SHIFT + SCOPE_BITS;
};
