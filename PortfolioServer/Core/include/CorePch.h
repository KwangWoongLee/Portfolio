#pragma once

#pragma warning(disable: 4251)

#include <winsock2.h>
#include <mswsock.h>
#include <ws2tcpip.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")

#include <cassert>

// std
#include <iostream>
#include <stdexcept>
#include <format>
#include <memory>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <array>
#include <vector>
#include <unordered_map>
#include <atomic>
#include <queue>
#include <condition_variable>
#include <chrono>
#include <set>
#include <unordered_set>

#include "RAII.h"
#include "Singleton.h"
#include "StrongId.h"

#include "ObjectPool.h"
#include "LockQueue.h"

#include "Task.h"

using SessionId = StrongId<struct SessionIdTag, int64_t>;
using TimerId = uint64_t;