#pragma once
#include "CmsTypes.h"
#include <string>

struct SiegeWarTag {};
using SiegeWarType = StrongId<SiegeWarTag>;
using SiegeWarTypeHash = StrongIdHash<SiegeWarTag>;

struct SiegeWarData final
{
    SiegeWarType _type;
    std::string _name;
    int32_t _prepDurationSec{};
    int32_t _attackDurationSec{};
    int32_t _swapSideDurationSec{};
};
