#pragma once
#include "CorePch.h"

namespace ProcessInfo
{
    inline uint64_t FileTimeToUInt64(FILETIME const& ft)
    {
        return (static_cast<uint64_t>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
    }

    inline uint64_t GetProcessCpuTime100ns()
    {
        FILETIME creationTime{};
        FILETIME exitTime{};
        FILETIME kernelTime{};
        FILETIME userTime{};
        ::GetProcessTimes(::GetCurrentProcess(), &creationTime, &exitTime, &kernelTime, &userTime);
        return FileTimeToUInt64(kernelTime) + FileTimeToUInt64(userTime);
    }

    inline uint32_t GetProcessorCount()
    {
        SYSTEM_INFO sysInfo{};
        ::GetSystemInfo(&sysInfo);
        return sysInfo.dwNumberOfProcessors;
    }

    inline uint32_t ComputeCpuPercent(uint64_t const previousCpuTime100ns, uint64_t const currentCpuTime100ns, int64_t const elapsedMs)
    {
        if (0 == previousCpuTime100ns || 0 >= elapsedMs)
        {
            return 0;
        }
        static auto const processorCount = GetProcessorCount();
        auto const cpuElapsed100ns = currentCpuTime100ns - previousCpuTime100ns;
        auto const realElapsed100ns = static_cast<uint64_t>(elapsedMs) * 10000;
        return static_cast<uint32_t>((cpuElapsed100ns * 100) / (realElapsed100ns * processorCount));
    }
}
