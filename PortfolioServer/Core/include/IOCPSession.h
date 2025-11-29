#pragma once
#include "CorePch.h"

#include "BaseSession.h"
#include "IOCPObject.h"
#include "IOEvent.h"
#include "LinearBuffer.h"
#include "Stream.h"


class OverlappedAccept final
    : public Overlapped
{
public:
    OverlappedAccept()
    {
        ZeroMemory(_acceptBuf, sizeof(_acceptBuf));
    }

    char* GetBuffer()
    {
        return _acceptBuf;
    }

private:
    char _acceptBuf[sizeof(SOCKADDR_IN) * 2 + 32]{};
};

enum class EIOCPSessionState : uint8_t
{
    None,
    Connecting,
    Connected,
    Disconnecting,
    Disconnected,
};

enum class EDisconnectReason
{
    None,
    ExplicitCall,
    RecvZero,
    SendZero,
    RecvOverflow,
    HandleError,
    InvalidState,
    SendOverflow,
    InvalidOperation,
};

class IOCPSession
	: public BaseSession
	, public IIOCPObject
{
public:
    ~IOCPSession() override;

    void Dispatch(Overlapped const* iocpEvent, uint32_t const numOfBytes = 0) override;

    void Disconnect(EDisconnectReason const reason);
    void FlushPacketStream();

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

    void OnAcceptCompleted();

protected:
    using PacketHandler = std::function<void(uint16_t const packetId, void const* payload, uint32_t const size)>;
    void SetPacketHandler(PacketHandler const& handler)
    {
        _packetHandler = handler;
    }

    SessionId _sessionId{};

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

private:
    char _acceptBuf[64]{};
    std::atomic<EIOCPSessionState> _state{EIOCPSessionState::None};

    static uint16_t constexpr RECV_BUFFER_SIZE = 65535;
    LinearBuffer _recvBuffer{ RECV_BUFFER_SIZE };

    static uint16_t constexpr SEND_BUFFER_SIZE = 65535;
    LinearBuffer _sendBuffer{ SEND_BUFFER_SIZE };
    
	Stream _pendingStream;
    bool _hasPendingStream{};
    
	std::mutex _sendMutex;
    bool _isSendPending{};

    PacketHandler _packetHandler;
};
