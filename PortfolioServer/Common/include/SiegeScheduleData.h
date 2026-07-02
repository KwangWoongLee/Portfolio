#pragma once
#include "CmsTypes.h"
#include <vector>

struct SiegeScheduleData final
{
    SiegeScheduleType _type;
    std::vector<SiegeWarType> _siegeWarTypes;
    int32_t _dayOfWeek{};
    int32_t _startHour{};
    int32_t _startMinute{};
};
