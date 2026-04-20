#pragma once
#include "CorePch.h"

struct SiegeWarTag {};
using SiegeWarType = StrongId<SiegeWarTag>;
using SiegeWarTypeHash = StrongIdHash<SiegeWarTag>;

struct SiegeScheduleTag {};
using SiegeScheduleType = StrongId<SiegeScheduleTag>;
using SiegeScheduleTypeHash = StrongIdHash<SiegeScheduleTag>;
