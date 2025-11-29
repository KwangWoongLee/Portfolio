#include "CorePch.h"
#include "Listener.h"
#include "IOCPSessionManager.h"
#include "SocketUtil.h"

uint16_t constexpr MAX_SESSION_COUNT = 1500;

Listener::Listener(uint16_t const port,
	std::function<bool(HANDLE const)> const& funcRegisterForCompletionPort,
	FuncCreateSession&& funcCreateSession)
	:_socketAddress(port)
	, _funcCreateSession(std::move(funcCreateSession))
{
	auto isSuccess{ false };
	auto const listenSocket = SocketUtil::Singleton::GetInstance().CreateSocket();

	RAII _([&isSuccess, &listenSocket]()
	{
		if (not isSuccess)
		{
			SocketUtil::Singleton::GetInstance().CloseSocket(listenSocket);

			assert(false);
			//TODO: log
		}
	});

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

	auto const a= std::dynamic_pointer_cast<OverlappedAccept*>(ioEvent);
	if ()

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

void Listener::PrepareAccepts()
{
	auto const coreCnt = std::thread::hardware_concurrency();
	for (uint16_t i{}; i < coreCnt * 2; ++i)
	{
		AsyncAccept();
	}
}

void Listener::AsyncAccept()
{
	auto const acceptedSession = _funcCreateSession();
	if (not acceptedSession)
	{
		return;
	}

	auto* const acceptIOEvent = ObjectPool<OverlappedAccept>::Singleton::GetInstance().Acquire(acceptedSession);
	acceptIOEvent->Init(
		[](Overlapped* overlappedAccept)
		{
			ObjectPool<OverlappedAccept>::Singleton::GetInstance().Release(static_cast<OverlappedAccept*>(overlappedAccept));
		});
	acceptIOEvent->SetIOType(EIOType::Accept);
	acceptIOEvent->SetIOCPObject(shared_from_this());

	DWORD bytesReceived = 0;
	if (not fnAcceptEx(reinterpret_cast<SOCKET>(GetHandle()), reinterpret_cast<SOCKET>(acceptedSession->GetHandle()), acceptIOEvent->GetBuffer(), 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, &bytesReceived, static_cast<LPOVERLAPPED>(&(*acceptIOEvent))))
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			IOCPSessionManager::Singleton::GetInstance().ReleaseSession(acceptedSession->GetSessionId());
			ObjectPool<OverlappedAccept>::Singleton::GetInstance().Release(acceptIOEvent);
		}
	}
}
