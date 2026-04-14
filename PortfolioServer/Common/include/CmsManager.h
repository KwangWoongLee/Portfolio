#pragma once
#include "CorePch.h"
#include "SiegeWarData.h"
#include "SiegeScheduleData.h"

class CmsManager final
{
public:
    using Singleton = Singleton<CmsManager>;

    bool LoadAll(std::string const& dataPath);

    SiegeWarData const* FindSiegeWar(SiegeWarType const type) const;
    SiegeScheduleData const* FindSiegeSchedule(SiegeScheduleType const type) const;

    std::vector<SiegeScheduleData const*> GetAllSiegeSchedules() const;

private:
    bool LoadSiegeWar(std::string const& dataPath);
    bool LoadSiegeSchedule(std::string const& dataPath);

    std::unordered_map<SiegeWarType, SiegeWarData, SiegeWarTypeHash> _siegeWars;
    std::unordered_map<SiegeScheduleType, SiegeScheduleData, SiegeScheduleTypeHash> _siegeSchedules;
};
