#include "CorePch.h"
#include "IOCP.h"
#include "IOCPObject.h"
#include "IOEvent.h"

IOCP::IOCP()
{
	_completionPort = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);

	if (not _completionPort)
	{
		assert(false);
	}
}

IOCP::~IOCP()
{
	if (INVALID_HANDLE_VALUE != _completionPort && nullptr != _completionPort)
	{
		::CloseHandle(_completionPort);
		_completionPort = INVALID_HANDLE_VALUE;
	}
}

bool IOCP::RegisterForCompletionPort(HANDLE const handle) const
{
    if (not CreateIoCompletionPort(handle, _completionPort, 0, 0))
	{
		return false;
	}

	return true;
}

void IOCP::Run(uint32_t const timeout) const
{
	_runningWorkerCount.store(1);
	IOWorkerFunc(timeout);
	_runningWorkerCount.store(0);
}

void IOCP::RunWorkerPool(uint32_t const workerCount, uint32_t const timeout) const
{
	auto const actualWorkerCount = (0 == workerCount) ? 1 : workerCount;
	_runningWorkerCount.store(actualWorkerCount);

	std::vector<std::thread> workers;
	workers.reserve(actualWorkerCount);

	for (uint32_t i{}; i < actualWorkerCount; ++i)
	{
		workers.emplace_back([this, timeout]()
		{
			IOWorkerFunc(timeout);
		});
	}

	for (auto& worker : workers)
	{
		if (worker.joinable())
		{
			worker.join();
		}
	}

	_runningWorkerCount.store(0);
}

void IOCP::Stop() const
{	
	auto const workerCount = _runningWorkerCount.load();
	auto const shutdownCount = (0 == workerCount) ? 1 : workerCount;
	for (uint32_t i{}; i < shutdownCount; ++i)
	{
		::PostQueuedCompletionStatus(_completionPort, 0, SHUTDOWN_KEY, nullptr);
	}
}

void IOCP::IOWorkerFunc(uint32_t const timeout) const
{
	while (true)
	{
		Overlapped* ioEvent{ nullptr };
		RAII _([&ioEvent]() {
			if (ioEvent)
			{
				if (auto const funcRelease = ioEvent->GetReleaseFunction())
				{
					funcRelease(ioEvent);
				}
				else
				{
					ObjectPool<Overlapped>::Singleton::GetInstance().Release(ioEvent);
				}
			}
			});

		ULONG_PTR key{ 0 };
		DWORD dwTransferred{ 0 };

		auto const result = ::GetQueuedCompletionStatus(_completionPort, &dwTransferred, &key, reinterpret_cast<LPOVERLAPPED*>(&ioEvent), timeout);
		if (SHUTDOWN_KEY == key)
		{
			return;
		}

		if (not ioEvent)
		{
			continue;
		}

		if (result)
		{
			auto const iocpObject = ioEvent->GetIOCPObject();
			if (not iocpObject)
			{
				//TODO:: log
				continue;
			}

			iocpObject->Dispatch(ioEvent, dwTransferred);
		}
		else
		{
			switch (GetLastError())
			{
			case ERROR_NETNAME_DELETED:
			case ERROR_CONNECTION_ABORTED:
				{
					//TODO:: log
					break;
				}
			default:
				{} break;
			}

			auto const iocpObject = ioEvent->GetIOCPObject();
			if (not iocpObject)
			{
				//TODO:: log
				continue;
			}

			iocpObject->Dispatch(ioEvent, dwTransferred);
		}
	}
}

