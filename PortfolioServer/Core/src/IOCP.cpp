#include "stdafx.h"
#include "IOCP.h"
#include "IOEvent.h"

bool IOCP::RegistForCompletionPort(std::shared_ptr<IIOCPObject> const& iocpObject) const
{
    if (auto const handle = ::CreateIoCompletionPort(iocpObject->GetHandle(), _completionPort, 0, 0); 
		not handle)
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
	auto const _ = RAII([&ioEvent]() {
		if (ioEvent)
		{
			ObjectPool<Overlapped>::Singleton::Instance().Release(ioEvent);
		}
		});

	ULONG_PTR key = 0;
	DWORD dwTransferred = 0;

	auto const result = ::GetQueuedCompletionStatus(_completionPort, &dwTransferred, &key, reinterpret_cast<LPOVERLAPPED*>(&ioEvent), timeout);
	if (key == SHUTDOWN_KEY)
	{
		return;
	}

	if (not ioEvent)
	{
		return;
	}

	if (result)
	{// 정상적인 io event 발생
		auto const iocpObject = ioEvent->GetIOCPObject();
		if (not iocpObject)
		{
			return;
		}

		iocpObject->Dispatch(ioEvent, dwTransferred);
	}
	else 
	{
		auto const err = WSAGetLastError();
		switch (err)
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

