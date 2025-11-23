#pragma once

#pragma warning(disable: 4251)

#include <iostream>

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
#include <functional>
#include <optional>
#include <string>
#include <string_view>

#include <iostream>
#include <vector>
#include <unordered_map>
#include <atomic>
#include <queue>
#include <condition_variable>

#include <shared_mutex>

#include "RAII.h"
#include "Singleton.h"

#include "ObjectPool.h"
#include "LockQueue.h"
#include "CircularBuffer.h"

#include "Task.h"
#include "BaseSessionManager.h"
