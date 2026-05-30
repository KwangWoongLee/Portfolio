#pragma once
#include "CorePch.h"
#include <fstream>

class Logger
{
public:
    bool Start(std::string const& filePath, std::string const& header);
    void Stop();

protected:
    void EnqueueLine(std::string line);

private:
    void ThreadMain();

    std::ofstream _file;
    std::atomic<bool> _stop{ false };
    std::thread _thread;

    std::mutex _mutex;
    std::condition_variable _cv;
    std::vector<std::string> _pending;
};
