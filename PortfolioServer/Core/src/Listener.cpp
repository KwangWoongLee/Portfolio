#include "CorePch.h"
#include "Listener.h"
#include "IOCPSessionManager.h"
#include "SocketUtil.h"

uint16_t constexpr MAX_SESSION_COUNT = 1500;

Listener::Listener(uint16_t const port, std::function<bool(HANDLE const)> const&& funcRegisterForCompletionPort)
	:_socketAddress(port)
{
	auto isSuccess{ false };
	RAII _([&isSuccess]()
	{
		if (not isSuccess)
		{
			assert(false);
			//TODO: log
		}
	});

	auto const listenSocket = SocketUtil::Singleton::GetInstance().CreateSocket();
	if (not SocketUtil::Singleton::GetInstance().SetReuseAddress(listenSocket, true))
	{
		return;
	}

	if (not SocketUtil::Singleton::GetInstance().SetLinger(listenSocket, 0, 0))
	{
		return;
	}

	if (not SocketUtil::Singleton::GetInstance().Bind(listenSocket, _socketAddress))
	{
		return;
	}

	if (not SocketUtil::Singleton::GetInstance().Listen(listenSocket))
	{
		return;
	}

	SetHandle(reinterpret_cast<HANDLE const>(listenSocket));

	if (not funcRegisterForCompletionPort(GetHandle()))
	{
		return;
	}

	PrepareAccepts();

	isSuccess = true;
}

void Listener::Dispatch(Overlapped const* ioEvent, uint32_t const numOfBytes)
{
	if (EIOType::Accept != ioEvent->GetIOType())
	{
		return;
	}

	auto const iocpObject = ioEvent->GetIOCPObject();
	auto const iocpSession = std::dynamic_pointer_cast<IOCPSession>(iocpObject);
	if (not iocpSession)
	{
		AsyncAccept();
		return;
	}

	auto const acceptedSocket = reinterpret_cast<SOCKET>(iocpSession->GetHandle());
	auto const listenSocket = reinterpret_cast<SOCKET>(GetHandle());

	if (not SocketUtil::Singleton::GetInstance().SetUpdateAcceptSocket(acceptedSocket, listenSocket))
	{
		AsyncAccept();
		return;
	}

	iocpSession->OnAcceptCompleted();

	AsyncAccept();
}

void Listener::PrepareAccepts() const
{
	for (uint16_t i{}; i < MAX_SESSION_COUNT; ++i)
	{
		AsyncAccept();
	}
}

void Listener::AsyncAccept() const
{
	auto* const acceptIOEvent = ObjectPool<OverlappedAccept>::Singleton::GetInstance().Acquire();
	acceptIOEvent->Init();
	acceptIOEvent->SetIOType(EIOType::Accept);

	auto const iocpSession = IOCPSessionManager::Singleton::GetInstance().CreateSession();
	acceptIOEvent->SetIOCPObject(iocpSession);

	DWORD bytesReceived = 0;
	if (not fnAcceptEx(reinterpret_cast<SOCKET>(GetHandle()), reinterpret_cast<SOCKET>(iocpSession->GetHandle()), acceptIOEvent->GetBuffer(), 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, &bytesReceived, static_cast<LPOVERLAPPED>(&(*acceptIOEvent))))
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			ObjectPool<Overlapped>::Singleton::GetInstance().Release(acceptIOEvent);
		}
	}
}
