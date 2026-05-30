#include "CorePch.h"
#include "DisconnectLogger.h"
#include <iomanip>
#include <sstream>

namespace
{
    char const* const DISCONNECT_LOG_HEADER = "timestamp,sessionId,reason";

    char const* DisconnectReasonToCString(EDisconnectReason const reason)
    {
        switch (reason)
        {
            case EDisconnectReason::None: return "None";
            case EDisconnectReason::ExplicitCall: return "ExplicitCall";
            case EDisconnectReason::RecvZero: return "RecvZero";
            case EDisconnectReason::SendZero: return "SendZero";
            case EDisconnectReason::RecvOverflow: return "RecvOverflow";
            case EDisconnectReason::HandleError: return "HandleError";
            case EDisconnectReason::InvalidState: return "InvalidState";
            case EDisconnectReason::SendOverflow: return "SendOverflow";
            case EDisconnectReason::InvalidOperation: return "InvalidOperation";
        }
        return "Unknown";
    }
}

bool DisconnectLogger::Start(std::string const& filePath)
{
    return Logger::Start(filePath, DISCONNECT_LOG_HEADER);
}

void DisconnectLogger::Log(SessionId const sessionId, EDisconnectReason const reason)
{
    auto const now = std::chrono::system_clock::now();
    auto const epochTime = std::chrono::system_clock::to_time_t(now);
    std::tm utcTime{};
    ::gmtime_s(&utcTime, &epochTime);

    std::ostringstream oss;
    oss << std::put_time(&utcTime, "%Y-%m-%dT%H:%M:%SZ") << ','
        << static_cast<int64_t>(sessionId) << ','
        << DisconnectReasonToCString(reason);

    EnqueueLine(oss.str());
}
