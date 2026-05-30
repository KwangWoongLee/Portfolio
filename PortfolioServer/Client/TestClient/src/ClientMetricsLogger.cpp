#include "CorePch.h"
#include "ClientMetricsLogger.h"
#include <iomanip>
#include <sstream>

namespace
{
    char const* const CLIENT_METRICS_CSV_HEADER =
        "timestamp,processId,botCount,recvPps,recvBps,packetQueueSize,cpuPercent,disconnectCount";
}

bool ClientMetricsLogger::Start(std::string const& filePath)
{
    return Logger::Start(filePath, CLIENT_METRICS_CSV_HEADER);
}

void ClientMetricsLogger::Enqueue(ClientMetricsSample const& sample)
{
    auto const epochTime = std::chrono::system_clock::to_time_t(sample._timestamp);
    std::tm utcTime{};
    ::gmtime_s(&utcTime, &epochTime);

    std::ostringstream oss;
    oss << std::put_time(&utcTime, "%Y-%m-%dT%H:%M:%SZ") << ','
        << sample._processId << ','
        << sample._botCount << ','
        << sample._recvPacketsPerSecond << ','
        << sample._recvBytesPerSecond << ','
        << sample._packetQueueSize << ','
        << sample._cpuPercent << ','
        << sample._disconnectCount;

    EnqueueLine(oss.str());
}
