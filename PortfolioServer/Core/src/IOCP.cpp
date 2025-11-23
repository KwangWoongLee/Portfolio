#include "CorePch.h"
#include "IOCP.h"
#include "IOCPObject.h"
#include "IOEvent.h"

bool IOCP::RegisterForCompletionPort(HANDLE const handle) const
{
    if (CreateIoCompletionPort(handle, _completionPort, 0, 0))
	{
		return false;
	}

	return true;
}

void IOCP::Run(uint32_t const timeout) const
{
	IOWorkerFunc(timeout);
}

void IOCP::Stop() const
{	
	::PostQueuedCompletionStatus(_completionPort, 0, SHUTDOWN_KEY, nullptr);
}

void IOCP::IOWorkerFunc(uint32_t const timeout) const
{
	Overlapped* ioEvent = nullptr;
	RAII _([&ioEvent]() {
			if (ioEvent)
			{
				ObjectPool<Overlapped>::Singleton::GetInstance().Release(ioEvent);
			}
		});

	ULONG_PTR key = 0;
	DWORD dwTransferred = 0;

	auto const result = ::GetQueuedCompletionStatus(_completionPort, &dwTransferred, &key, reinterpret_cast<LPOVERLAPPED*>(&ioEvent), timeout);
	if (SHUTDOWN_KEY == key)
	{
		return;
	}

	if (not ioEvent)
	{
		return;
	}

	if (result)
	{ // success
		auto const iocpObject = ioEvent->GetIOCPObject();
		if (not iocpObject)
		{
			return;
		}

		iocpObject->Dispatch(ioEvent, dwTransferred);
	}
	else 
	{
		switch (auto const err = WSAGetLastError())
		{
		case ERROR_NETNAME_DELETED:
		case ERROR_CONNECTION_ABORTED:
			{
				return;
			}
        default: 
			{} break;
        }

		auto const iocpObject = ioEvent->GetIOCPObject();
		if (not iocpObject)
		{
			return;
		}

		iocpObject->Dispatch(ioEvent, dwTransferred);
	}
}

