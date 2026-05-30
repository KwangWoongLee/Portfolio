#include "CorePch.h"
#include "IOCP.h"
#include "Engine.h"
#include "Connector.h"
#include "IOCPSessionManager.h"
#include "TaskDispatcher.h"
#include "TimerManager.h"
#include "Metrics.h"
#include "ProcessInfo.h"
#include "DisconnectLogger.h"
#include "ClientMetricsLogger.h"
#include "TestClientSession.h"
#include <iomanip>
#include <sstream>

namespace
{
    auto constexpr TARGET_CONNECTIONS = 500;
    auto constexpr SPAWN_INTERVAL = std::chrono::milliseconds(1);

    auto constexpr NETWORK_IO_THREADS = 8;
    auto constexpr TIMER_THREADS = 2;

    auto constexpr METRICS_SAMPLE_INTERVAL = std::chrono::seconds(1);

    std::string BuildClientMetricsPath(uint32_t const processId)
    {
        auto const now = std::chrono::system_clock::now();
        auto const epochTime = std::chrono::system_clock::to_time_t(now);
        std::tm localTime{};
        ::localtime_s(&localTime, &epochTime);

        std::ostringstream pathStream;
        pathStream << "metrics/client_metrics_"
            << processId << '_'
            << std::put_time(&localTime, "%Y%m%d_%H%M%S")
            << ".csv";
        return pathStream.str();
    }

    std::string BuildDisconnectLogPath(uint32_t const processId)
    {
        auto const now = std::chrono::system_clock::now();
        auto const epochTime = std::chrono::system_clock::to_time_t(now);
        std::tm localTime{};
        ::localtime_s(&localTime, &epochTime);

        std::ostringstream pathStream;
        pathStream << "logs/client_"
            << processId << '_'
            << std::put_time(&localTime, "%Y%m%d")
            << ".log";
        return pathStream.str();
    }
}

int main()
{
    auto const iocp = std::make_shared<IOCP>();
    Engine engine(iocp);

    IOCPSessionManager::Singleton::GetInstance().Init(iocp);

    auto& dispatcher = TaskDispatcher::Singleton::GetInstance();
    dispatcher.AddExecutor(ETaskType::NetworkIO, NETWORK_IO_THREADS);
    dispatcher.AddExecutor(ETaskType::Timer, TIMER_THREADS);

    TimerManager::Singleton::GetInstance().Start();

    auto const processId = static_cast<uint32_t>(::GetCurrentProcessId());

    if (not ClientMetricsLogger::Singleton::GetInstance().Start(BuildClientMetricsPath(processId)))
    {
        std::cout << "[TestClient] Failed to start ClientMetricsLogger" << std::endl;
        return 1;
    }

    if (not DisconnectLogger::Singleton::GetInstance().Start(BuildDisconnectLogPath(processId)))
    {
        std::cout << "[TestClient] Failed to start DisconnectLogger" << std::endl;
        return 1;
    }

    Connector connector("127.0.0.1", 9000,
        []() -> std::shared_ptr<IOCPSession>
        {
            return IOCPSessionManager::Singleton::GetInstance().CreateSession<TestClientSession>();
        }
    );

    std::cout << "[Main] Spawning " << TARGET_CONNECTIONS << " connections..." << std::endl;

    std::thread spawner(
        [&connector]()
        {
            for (int32_t i = 0; i < TARGET_CONNECTIONS; ++i)
            {
                connector.AsyncConnect();
                std::this_thread::sleep_for(SPAWN_INTERVAL);
            }
            std::cout << "[Main] All connect requests issued." << std::endl;
        });
    spawner.detach();

    struct SamplerState
    {
        std::chrono::steady_clock::time_point _lastSampleTime{ std::chrono::steady_clock::now() };
        uint64_t _lastRecvPacketCount{};
        uint64_t _lastRecvByteCount{};
        uint64_t _lastProcessCpuTime100ns{};
    };
    auto const samplerState = std::make_shared<SamplerState>();

    TimerManager::Singleton::GetInstance().AddRepeatTimer(
        std::chrono::duration_cast<std::chrono::milliseconds>(METRICS_SAMPLE_INTERVAL),
        0,
        [samplerState, processId]()
        {
            auto const currentTime = std::chrono::steady_clock::now();
            auto const currentRecvPacketCount = Metrics::g_recvPacketCount.load(std::memory_order_relaxed);
            auto const currentRecvByteCount = Metrics::g_recvByteCount.load(std::memory_order_relaxed);
            auto const currentCpuTime100ns = ProcessInfo::GetProcessCpuTime100ns();

            auto const elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                currentTime - samplerState->_lastSampleTime).count();
            auto const safeElapsedMs = (elapsedMs <= 0) ? 1 : elapsedMs;

            ClientMetricsSample sample;
            sample._timestamp = std::chrono::system_clock::now();
            sample._processId = processId;
            sample._botCount = TARGET_CONNECTIONS;
            sample._recvPacketsPerSecond = static_cast<uint32_t>(
                (currentRecvPacketCount - samplerState->_lastRecvPacketCount) * 1000 / safeElapsedMs);
            sample._recvBytesPerSecond = static_cast<uint32_t>(
                (currentRecvByteCount - samplerState->_lastRecvByteCount) * 1000 / safeElapsedMs);
            sample._packetQueueSize = static_cast<uint32_t>(
                TaskDispatcher::Singleton::GetConstInstance().GetTotalQueueSize());
            sample._cpuPercent = ProcessInfo::ComputeCpuPercent(
                samplerState->_lastProcessCpuTime100ns, currentCpuTime100ns, safeElapsedMs);
            sample._disconnectCount = Metrics::g_disconnectCount.load(std::memory_order_relaxed);

            samplerState->_lastSampleTime = currentTime;
            samplerState->_lastRecvPacketCount = currentRecvPacketCount;
            samplerState->_lastRecvByteCount = currentRecvByteCount;
            samplerState->_lastProcessCpuTime100ns = currentCpuTime100ns;

            ClientMetricsLogger::Singleton::GetInstance().Enqueue(sample);
        });

    engine.Run();

    DisconnectLogger::Singleton::GetInstance().Stop();
    ClientMetricsLogger::Singleton::GetInstance().Stop();
    return 0;
}
