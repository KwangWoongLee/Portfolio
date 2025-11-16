#include "stdafx.h"
#include "Engine.h"
#include "IOCP.h"

Engine::Engine(IOCPRef iocp, uint16_t maxSessionCount, SessionFactory sessionFactory)
	:mIOCP(iocp),
	mMaxSessionCount(maxSessionCount),
	mSessionFactory(sessionFactory)
{
}

Engine::~Engine()
{
	mIOCP = nullptr;
}

bool Engine::Init()
{
	// Init �۾��� �����ϸ�, ���α׷��� ������ ���̹Ƿ� �׳� �����ϱ�� ����.

	if (SocketUtil::GetInstance().Init() == false)
		return false;

	if (mIOCP->Init() == false)
		return false;

	return true;
}

void Engine::Run(uint32_t timeout)
{
	mIOCP->Run(timeout);
}

void Engine::Stop()
{
}

SessionRef Engine::CreateSession()
{
	SessionRef session = mSessionFactory();
	session->SetEngine(shared_from_this());
	
	{
		WRITE_LOCK;

		mSessions.push_back(session);

		if (mIOCP->RegistForCompletionPort(session) == false)
			return nullptr;
	}


	return session;
}

void Engine::DisConnectSession(SessionRef session)
{
	{
		WRITE_LOCK;
		auto iter = std::find(mSessions.begin(), mSessions.end(), session);
		if (iter != mSessions.end())
		{
			std::iter_swap(iter, mSessions.end() - 1);

			mSessions.pop_back();
		}
		
	}
}

bool Engine::RegistForCompletionPort(IOCPObjectRef iocpObject)
{
	return mIOCP->RegistForCompletionPort(iocpObject);
}
