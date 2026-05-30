#include "CorePch.h"
#include "Logger.h"
#include <filesystem>

bool Logger::Start(std::string const& filePath, std::string const& header)
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
        std::cerr << "[Logger] Failed to open " << filePath << std::endl;
        return false;
    }

    if (not fileExists && not header.empty())
    {
        _file << header << '\n';
        _file.flush();
    }

    _stop.store(false);
    _thread = std::thread([this]() { ThreadMain(); });

    std::cout << "[Logger] Started -> " << filePath << std::endl;
    return true;
}

void Logger::Stop()
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

void Logger::EnqueueLine(std::string line)
{
    {
        std::scoped_lock lock(_mutex);
        _pending.emplace_back(std::move(line));
    }
    _cv.notify_one();
}

void Logger::ThreadMain()
{
    while (true)
    {
        std::vector<std::string> drained;
        {
            std::unique_lock lock(_mutex);
            _cv.wait(lock, [this]() { return _stop.load() || not _pending.empty(); });

            if (_stop.load() && _pending.empty())
            {
                return;
            }

            drained = std::move(_pending);
            _pending.clear();
        }

        for (auto const& line : drained)
        {
            _file << line << '\n';
        }
        _file.flush();
    }
}
