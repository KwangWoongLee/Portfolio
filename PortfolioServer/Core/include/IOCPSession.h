#pragma once
#include "stdafx.h"
#include "SocketUtil.h"
#include "IOCP.h"
#include "IOEvent.h"
#include "StreamWriter.h"
#include "StreamReader.h"
#include "CircularBuffer.h" // 새 버퍼

namespace
{
    enum class EIOCPSessionState : uint8_t
    {
        NONE,
        CONNECTING,
        CONNECTED,
        DISCONNECTING,
        DISCONNECTED,
    };

    enum class EDisconnectReason
    {
        NONE,
        EXPLICIT_CALL,
        RECV_ZERO,
        SEND_ZERO,
        RECV_OVERFLOW,
        HANDLE_ERROR,
        INVALID_STATE,
        SEND_BUFFER_OVERFLOW,
        INVALID_OPERATION,
    };

    char const* ToString(EDisconnectReason const reason);
}

class IOCPSession
	: public IIOCPObject
{
public:
    void Dispatch(Overlapped const* iocpEvent, uint32_t const numOfBytes = 0) override;

    bool SetSockAddr();
    void SetSessionId(SessionId const sessionId)
    {
        _sessionId = sessionId;
    }

    void Disconnect(EDisconnectReason const reason);
    void Send(const char* buffer, uint32_t const contentSize);
    void SendPacket(uint16_t const packetId, google::protobuf::MessageLite& packet);

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

    void OnRecvPacket(uint16_t const packetId, const void* payload, uint32_t const size);

private:
    char _acceptBuf[64] = {};
    SocketAddress _sockAddress;
    std::atomic<EIOCPSessionState> _state{EIOCPSessionState::NONE};

    CircularBuffer _recvBuffer{0x10000};
    CircularBuffer _sendBuffer{0x10000};

    std::mutex _sendMutex;
    bool _isSendPending{};

    StreamWriter _streamWriter;
    StreamReader _streamReader;

    PacketHandler _packetHandler;
};
