#include "CorePch.h"
#include "MetricsLogger.h"
#include <filesystem>
#include <iomanip>

bool MetricsLogger::Start(std::string const& filePath)
{
    auto const path = std::filesystem::path(filePath);
    auto const parentDirectory = path.parent_path();
    if (not parentDirectory.empty())
    {
        std::error_code errorCode;
        std::filesystem::create_directories(parentDirectory, errorCode);
    }

    auto const fileExists = std::filesystem::exists(path);

    _file.open(filePath, std::ios::out | std::ios::app);
    if (not _file.is_open())
    {
        std::cerr << "[MetricsLogger] Failed to open " << filePath << std::endl;
        return false;
    }

    if (not fileExists)
    {
        _file << "timestamp,ccu,sendPps,recvPps,sendBps,recvBps,cpuPercent,queueSize\n";
        _file.flush();
    }

    _stop.store(false);
    _thread = std::thread([this]() { ThreadMain(); });

    std::cout << "[MetricsLogger] Started -> " << filePath << std::endl;
    return true;
}

void MetricsLogger::Stop()
{
    _stop.store(true);
    _cv.notify_one();

    if (_thread.joinable())
    {
        _thread.join();
    }

    if (_file.is_open())
    {
        _file.flush();
        _file.close();
    }
}

void MetricsLogger::Enqueue(MetricsSample sample)
{
    {
        std::scoped_lock lock(_mutex);
        _pendingSamples.emplace_back(std::move(sample));
    }
    _cv.notify_one();
}

void MetricsLogger::ThreadMain()
{
    while (true)
    {
        std::vector<MetricsSample> drainedSamples;
        {
            std::unique_lock lock(_mutex);
            _cv.wait(lock, [this]() { return _stop.load() || not _pendingSamples.empty(); });

            if (_stop.load() && _pendingSamples.empty())
            {
                return;
            }

            drainedSamples = std::move(_pendingSamples);
            _pendingSamples.clear();
        }

        for (auto const& sample : drainedSamples)
        {
            WriteSample(sample);
        }
        _file.flush();
    }
}

void MetricsLogger::WriteSample(MetricsSample const& sample)
{
    auto const epochTime = std::chrono::system_clock::to_time_t(sample._timestamp);
    std::tm utcTime{};
    ::gmtime_s(&utcTime, &epochTime);

    _file << std::put_time(&utcTime, "%Y-%m-%dT%H:%M:%SZ") << ','
        << sample._ccu << ','
        << sample._sendPacketsPerSecond << ','
        << sample._recvPacketsPerSecond << ','
        << sample._sendBytesPerSecond << ','
        << sample._recvBytesPerSecond << ','
        << sample._cpuPercent << ','
        << sample._taskQueueSize << '\n';
}
