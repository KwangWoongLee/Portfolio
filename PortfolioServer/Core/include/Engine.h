#pragma once

using SessionFactory = std::function<SessionRef()>;

class Engine
{
public:


public:
	Engine(IOCPRef iocp, uint16_t maxSessionCount, SessionFactory sessionFactory);
	virtual ~Engine();

	virtual bool Init();
	virtual void Run(uint32_t timeout = INFINITE);
	virtual void Stop();

	SessionRef CreateSession();
	void		DisConnectSession(SessionRef session);
	bool RegistForCompletionPort(IOCPObjectRef iocpObject);

public:
	uint16_t GetMaxSessionCount() const { return mMaxSessionCount; }


protected:
	USE_LOCK;

	IOCPRef					mIOCP = nullptr;
	uint16_t					mMaxSessionCount = 0;
	SessionFactory			mSessionFactory;
	std::vector<SessionRef>	mSessions;
};



