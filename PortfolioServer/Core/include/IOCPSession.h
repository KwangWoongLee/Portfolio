#pragma once
#include "CorePch.h"

#include "IOCPObject.h"
#include "IOEvent.h"
#include "LinearBuffer.h"
#include "Stream.h"


class OverlappedAccept final
    : public Overlapped
{
public:
	explicit OverlappedAccept(std::shared_ptr<IOCPSession> const& acceptedSession)
		: _acceptedSession(acceptedSession)
    {
        ZeroMemory(_acceptBuf, sizeof(_acceptBuf));
    }

    char* GetBuffer()
    {
        return _acceptBuf;
    }

private:
    char _acceptBuf[sizeof(SOCKADDR_IN) * 2 + 32]{};
    std::shared_ptr<IOCPSession> _acceptedSession;
};

enum class EIOCPSessionState : uint8_t
{
    None,
    Connecting,
    Connected,
    Disconnecting,
    Disconnected,
};

class IOCPSession
	: public IIOCPObject
{
public:
    ~IOCPSession() override;

    virtual void HandlePacket(uint16_t packetId, void const* payload, uint32_t size) = 0;

    SessionId GetSessionId() const { return _sessionId; }

    void SetSessionId(SessionId const sessionId) { _sessionId = sessionId; }

    void Dispatch(Overlapped const* iocpEvent, uint32_t const numOfBytes = 0) override;

    void Disconnect(EDisconnectReason const reason);
    void FlushPacketStream();

    void OnAcceptCompleted();

    template <typename T>
    void SendPacket(T const& packet)
    {
        if (EIOCPSessionState::Connected != _state)
        {
            return;
        }

        if (not _hasPendingStream)
        {
            _pendingStream.Reset();
            _hasPendingStream = true;
        }

        if (not packet.WriteToStream(_pendingStream))
        {
            FlushPacketStream();

            _pendingStream.Reset();
            _hasPendingStream = true;

            if (not packet.WriteToStream(_pendingStream))
            {
                Disconnect(EDisconnectReason::SendOverflow);
                return;
            }
        }
    }
private:
    virtual void OnConnected() {}
    virtual void OnDisconnected() {}

    void SetConnected();
    void SetDisconnected();

    void AsyncDisconnect();
    void AsyncRecv();
    void AsyncSend();
    void HandleError(int32_t const errorCode);

    void Send(char const* buffer, uint32_t const contentSize);

    void OnConnectCompleted();
    void OnDisconnectCompleted();
    void OnRecvCompleted(uint32_t const transferred);
    void OnSendCompleted(uint32_t const transferred);

    void HandleStream(Stream& outStream);

private:
    SessionId _sessionId{};

    char _acceptBuf[64]{};
    std::atomic<EIOCPSessionState> _state{EIOCPSessionState::None};

    LinearBuffer _recvBuffer{Stream::MAX_SIZE };
    LinearBuffer _sendBuffer{ Stream::MAX_SIZE };
    
	Stream _pendingStream;
    bool _hasPendingStream{};
    
	std::mutex _sendMutex;
    bool _isSendPending{};
};
