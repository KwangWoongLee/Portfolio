#pragma once
#include "CmsTypes.h"

struct SiegeScheduleTag {};
using SiegeScheduleType = StrongId<SiegeScheduleTag>;
using SiegeScheduleTypeHash = StrongIdHash<SiegeScheduleTag>;

struct SiegeScheduleData final
{
    SiegeScheduleType _type;
    SiegeWarType _siegeWarType;
    int32_t _dayOfWeek{};
    int32_t _startHour{};
    int32_t _startMinute{};
};
