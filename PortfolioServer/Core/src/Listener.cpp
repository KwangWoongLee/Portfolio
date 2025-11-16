#include "stdafx.h"
#include "Listener.h"
#include "IOCP.h"
#include "IOCPSessionManager.h"

void Listener::Dispatch(Overlapped const* ioEvent, uint32_t const numOfBytes)
{
	if (EIOType::ACCEPT != ioEvent->GetIOType())
	{
		return;
	}

	auto const iocpObject = ioEvent->GetIOCPObject();
	auto const iocpSession = std::dynamic_pointer_cast<IOCPSession>(iocpObject);
	if (not iocpSession)
	{
		asyncAccept();
		return;
	}

	auto const acceptedSocket = reinterpret_cast<SOCKET>(iocpSession->GetHandle());
	auto const listenSocket = reinterpret_cast<SOCKET>(GetHandle());

	if (not SocketUtil::Singleton::Instance().SetUpdateAcceptSocket(acceptedSocket, listenSocket))
	{
		asyncAccept();
		return;
	}

	if (not iocpSession->SetSockAddr())
	{
		asyncAccept();
		return;
	}

	iocpSession->OnAcceptCompleted(); // 연결 완료 처리

	asyncAccept(); // 다음 AcceptEx 등록 (새 Overlapped, 새 소켓)
}

bool Listener::Init()
{
	auto const listenSocket = SocketUtil::Singleton::Instance().CreateSocket();
	if (not SocketUtil::Singleton::Instance().SetReuseAddress(listenSocket, true))
	{
		return false;
	}

	if (not SocketUtil::Singleton::Instance().SetLinger(listenSocket, 0, 0))
	{
		return false;
	}

	if (not SocketUtil::Singleton::Instance().Bind(listenSocket))
	{
		return false;
	}
	
	if (not SocketUtil::Singleton::Instance().Listen(listenSocket))
	{
		return false;
	}

	SetHandle(reinterpret_cast<HANDLE>(listenSocket));

	if (not _iocp->RegistForCompletionPort(shared_from_this()))
	{
		return false;
	}

	prepareAccepts();

	return true;
}


void Listener::prepareAccepts()
{
	auto const maxSessionCount = 10; // TODO: config
	for (uint16_t i{}; i < maxSessionCount; ++i)
	{
		asyncAccept();
	}
}

void Listener::asyncAccept()
{
	auto* const acceptIOEvent = ObjectPool<OverlappedAccept>::Singleton::Instance().Acquire();
	acceptIOEvent->Init();
	acceptIOEvent->SetIOType(EIOType::ACCEPT);

	auto const iocpSession = IOCPSessionManager::Singleton::Instance().CreateSession();
	acceptIOEvent->SetIOCPObject(iocpSession);

	DWORD bytesReceived = 0;
	if (not FnAcceptEx(reinterpret_cast<SOCKET>(GetHandle()), reinterpret_cast<SOCKET>(iocpSession->GetHandle()), acceptIOEvent->GetBuffer(), 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, &bytesReceived, static_cast<LPOVERLAPPED>(&(*acceptIOEvent))))
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			ObjectPool<Overlapped>::Singleton::Instance().Release(acceptIOEvent); // 이런 경우 GetQueuedCompletionStatus 에 감지되지 않아 메모리 반납 필요
		}
	}
}
