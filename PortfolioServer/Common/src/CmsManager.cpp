#include "CorePch.h"
#include "CmsManager.h"
#include "CsvParser.h"

bool CmsManager::LoadAll(std::string const& dataPath)
{
    if (not LoadSiegeWar(dataPath))
    {
        return false;
    }

    if (not LoadSiegeSchedule(dataPath))
    {
        return false;
    }

    std::cout << "[CmsManager] All CMS data loaded" << std::endl;
    return true;
}

SiegeWarData const* CmsManager::FindSiegeWar(SiegeWarType const type) const
{
    auto const iter = _siegeWars.find(type);
    if (_siegeWars.end() == iter)
    {
        return nullptr;
    }
    return &iter->second;
}

SiegeScheduleData const* CmsManager::FindSiegeSchedule(SiegeScheduleType const type) const
{
    auto const iter = _siegeSchedules.find(type);
    if (_siegeSchedules.end() == iter)
    {
        return nullptr;
    }
    return &iter->second;
}

std::vector<SiegeScheduleData const*> CmsManager::GetAllSiegeSchedules() const
{
    std::vector<SiegeScheduleData const*> result;
    result.reserve(_siegeSchedules.size());

    for (auto const& [type, data] : _siegeSchedules)
    {
        result.push_back(&data);
    }

    return result;
}

bool CmsManager::LoadSiegeWar(std::string const& dataPath)
{
    CsvParser csv;
    if (not csv.Load(dataPath + "/SiegeWar.csv"))
    {
        return false;
    }

    for (auto const& row : csv.GetRows())
    {
        SiegeWarData data;
        data._type = SiegeWarType{ row.GetInt32(0) };
        data._name = row.GetString(1);
        data._prepDurationSec = row.GetInt32(2);
        data._attackDurationSec = row.GetInt32(3);
        data._swapSideDurationSec = row.GetInt32(4);

        auto const type = data._type;
        _siegeWars.emplace(type, std::move(data));
    }

    std::cout << "[CmsManager] SiegeWar loaded: " << _siegeWars.size() << " entries" << std::endl;
    return true;
}

bool CmsManager::LoadSiegeSchedule(std::string const& dataPath)
{
    CsvParser csv;
    if (not csv.Load(dataPath + "/SiegeSchedule.csv"))
    {
        return false;
    }

    for (auto const& row : csv.GetRows())
    {
        SiegeScheduleData data;
        data._type = SiegeScheduleType{ row.GetInt32(0) };
        data._siegeWarType = SiegeWarType{ row.GetInt32(1) };
        data._dayOfWeek = row.GetInt32(2);
        data._startHour = row.GetInt32(3);
        data._startMinute = row.GetInt32(4);

        auto const type = data._type;
        _siegeSchedules.emplace(type, std::move(data));
    }

    std::cout << "[CmsManager] SiegeSchedule loaded: " << _siegeSchedules.size() << " entries" << std::endl;
    return true;
}
