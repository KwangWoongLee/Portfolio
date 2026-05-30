#include "CorePch.h"
#include "MetricsLogger.h"
#include <iomanip>
#include <sstream>

namespace
{
    char const* const METRICS_CSV_HEADER =
        "timestamp,ccu,sendPps,recvPps,sendBps,recvBps,cpuPercent,queueSize,pendingSendBytesTotal,sendOverflowCount";
}

bool MetricsLogger::Start(std::string const& filePath)
{
    return Logger::Start(filePath, METRICS_CSV_HEADER);
}

void MetricsLogger::Enqueue(MetricsSample const& sample)
{
    auto const epochTime = std::chrono::system_clock::to_time_t(sample._timestamp);
    std::tm utcTime{};
    ::gmtime_s(&utcTime, &epochTime);

    std::ostringstream oss;
    oss << std::put_time(&utcTime, "%Y-%m-%dT%H:%M:%SZ") << ','
        << sample._ccu << ','
        << sample._sendPacketsPerSecond << ','
        << sample._recvPacketsPerSecond << ','
        << sample._sendBytesPerSecond << ','
        << sample._recvBytesPerSecond << ','
        << sample._cpuPercent << ','
        << sample._taskQueueSize << ','
        << sample._pendingSendBytesTotal << ','
        << sample._sendOverflowCount;

    EnqueueLine(oss.str());
}
