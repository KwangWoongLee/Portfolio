#pragma once

using SessionId = int64_t;

class BaseSession
{
public:
	virtual ~BaseSession() = default;
	SessionId GetSessionId() const
	{
		return _sessionId;
	}

	void SetSessionId(SessionId const sessionId)
	{
		_sessionId = sessionId;
	}

private:
	SessionId _sessionId{};
};