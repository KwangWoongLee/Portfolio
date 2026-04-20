#pragma once
#include "CmsTypes.h"

struct SiegeScheduleData final
{
    SiegeScheduleType _type;
    SiegeWarType _siegeWarType;
    int32_t _dayOfWeek{};
    int32_t _startHour{};
    int32_t _startMinute{};
};
