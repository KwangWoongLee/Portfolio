#pragma once
#include "CorePch.h"

#include "BaseSession.h"
#include "IOCPObject.h"
#include "IOEvent.h"
#include "StreamWriter.h"
#include "StreamReader.h"


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
    SendBufferOverflow,
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
    void Send(char const* buffer, uint32_t const contentSize);
    //void SendPacket(uint16_t packetId, google::protobuf::MessageLite& packet);

    void OnAcceptCompleted();

protected:
    using PacketHandler = std::function<void(uint16_t const packetId, void const* payload, uint32_t const size)>;
    void SetPacketHandler(PacketHandler const& handler)
    {
        _packetHandler = handler;
    }

    SessionId _sessionId{};

private:
    virtual void OnConnected()
    {
    }
    virtual void OnDisconnected()
    {
    }

    void SetConnected();
    void SetDisconnected();

    void AsyncDisconnect();
    void AsyncRecv();
    void AsyncSend();
    void HandleError(int32_t const errorCode);

    bool TryProcessPacket();

    void OnConnectCompleted();
    void OnDisconnectCompleted();
    void OnRecvCompleted(uint32_t const transferred);
    void OnSendCompleted(uint32_t const transferred);

    void on_recv_packet(uint16_t const packetId, void const* payload, uint32_t const size);

private:
    char _acceptBuf[64]{};
    std::atomic<EIOCPSessionState> _state{EIOCPSessionState::None};

    CircularBuffer _recvBuffer{ 0x10000 };
    CircularBuffer _sendBuffer{ 0x10000 };

    std::mutex _sendMutex;
    bool _isSendPending{};

    StreamWriter _streamWriter;
    StreamReader _streamReader;

    PacketHandler _packetHandler;
};
