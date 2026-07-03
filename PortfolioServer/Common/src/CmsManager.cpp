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
        result.emplace_back(&data);
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
        data._declarationCostGold = row.GetInt64(5);

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
        auto const scheduleType = SiegeScheduleType{ row.GetInt32(0) };
        auto const siegeWarType = SiegeWarType{ row.GetInt32(1) };
        auto const dayOfWeek = row.GetInt32(2);
        auto const startHour = row.GetInt32(3);
        auto const startMinute = row.GetInt32(4);

        if (dayOfWeek < 0 || dayOfWeek > 6 ||
            startHour < 0 || startHour > 23 ||
            startMinute < 0 || startMinute > 59)
        {
            return false;
        }

        if (not _siegeWars.contains(siegeWarType))
        {
            return false;
        }

        auto [iter, inserted] = _siegeSchedules.try_emplace(scheduleType);
        auto& schedule = iter->second;
        if (inserted)
        {
            schedule._type = scheduleType;
            schedule._dayOfWeek = dayOfWeek;
            schedule._startHour = startHour;
            schedule._startMinute = startMinute;
        }
        else if (schedule._dayOfWeek != dayOfWeek ||
                 schedule._startHour != startHour ||
                 schedule._startMinute != startMinute)
        {
            return false;
        }

        if (std::ranges::find(schedule._siegeWarTypes, siegeWarType) !=
            schedule._siegeWarTypes.end())
        {
            return false;
        }

        schedule._siegeWarTypes.emplace_back(siegeWarType);
    }

    std::cout << "[CmsManager] SiegeSchedule loaded: " << _siegeSchedules.size() << " entries" << std::endl;
    return true;
}
