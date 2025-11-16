#include "stdafx.h"
#include "IOCPSession.h"

#include "IOCPSessionManager.h"

char const* ToString(EDisconnectReason const reason)
{
    switch (reason)
    {
    case EDisconnectReason::EXPLICIT_CALL: return "Explicit Disconnect";
    case EDisconnectReason::RECV_ZERO: return "Client Closed Connection (Recv 0)";
    case EDisconnectReason::SEND_ZERO: return "Send Completed with 0 Bytes";
    case EDisconnectReason::RECV_OVERFLOW: return "Recv Buffer Overwrite Attempted";
    case EDisconnectReason::HANDLE_ERROR: return "Socket Handle Error";
    case EDisconnectReason::INVALID_STATE: return "Invalid State for Operation";
    default: return "Unknown Reason";
    }
}

void IOCPSession::Dispatch(Overlapped const* iocpEvent, uint32_t const numOfBytes)
{
    switch (iocpEvent->GetIOType())
    {
    case EIOType::CONNECT:
        {
    		OnConnectCompleted();
        } break;
    case EIOType::DISCONNECT:
        {
            OnDisconnectCompleted();
        } break;
    case EIOType::SEND:
        {
            OnSendCompleted(numOfBytes);
        } break;
    case EIOType::RECV:
        {
            OnRecvCompleted(numOfBytes);
        } break;
    default:
        {} break;
    }
}

bool IOCPSession::SetSockAddr()
{
    SocketAddress addr;
    auto len = static_cast<int>(addr.GetSize());
    if (::getpeername(reinterpret_cast<SOCKET>(GetHandle()), addr.GetAsSockAddr(), &len) != 0)
    {
        return false;
    }

    _sockAddress = addr;
    return true;
}

void IOCPSession::Disconnect(EDisconnectReason const reason)
{
    if (_state != EIOCPSessionState::CONNECTED)
    {
        return;
    }

    AsyncDisconnect();
}

void IOCPSession::Send(const char* buffer, uint32_t const contentSize)
{
    if (_state != EIOCPSessionState::CONNECTED)
    {
        return;
    }

    bool registerSend = false;

    {
        std::scoped_lock lock(_sendMutex);

        if (_sendBuffer.GetFreeSpaceSize() < contentSize)
        {
            return;
        }

        memcpy(_sendBuffer.GetBuffer(), buffer, contentSize);
        _sendBuffer.Commit(contentSize);

        if (!_isSendPending)
        {
            _isSendPending = true;
            registerSend = true;
        }
    }

    if (registerSend)
    {
        AsyncSend();
    }
}

void IOCPSession::SendPacket(uint16_t const packetId, google::protobuf::MessageLite& packet)
{
    if (_state != EIOCPSessionState::CONNECTED)
    {
        return;
    }

    uint8_t tempBuffer[0x10000];
    _streamWriter.Init(tempBuffer, sizeof(tempBuffer));

    if (_streamWriter.WritePacket(packetId, packet))
    {
        _streamWriter.Finalize();
        Send(reinterpret_cast<const char*>(tempBuffer), _streamWriter.GetSize());
    }
}

void IOCPSession::OnAcceptCompleted()
{
    auto const socket = reinterpret_cast<SOCKET>(GetHandle());

    std::ignore = SocketUtil::Singleton::Instance().SetKeepAlive(socket, true);
    std::ignore = SocketUtil::Singleton::Instance().SetTcpNoDelay(socket, true);

    SetSockAddr();
    SetConnected();
}

void IOCPSession::OnConnectCompleted()
{
    auto const socket = reinterpret_cast<SOCKET>(GetHandle());

    std::ignore = SocketUtil::Singleton::Instance().SetKeepAlive(socket, true);
    std::ignore = SocketUtil::Singleton::Instance().SetTcpNoDelay(socket, true);

    SetSockAddr();
    SetConnected();
}

void IOCPSession::OnDisconnectCompleted()
{
    SetDisconnected();
}

bool IOCPSession::TryProcessPacket()
{
    if (_recvBuffer.GetContiguousBytes() < sizeof(StreamHeader))
    {
        return false;
    }

    auto const streamHeader = reinterpret_cast<StreamHeader const*>(_recvBuffer.GetBufferStart());

    if (0 == streamHeader->size || streamHeader->size > _recvBuffer.GetCapacity())
    {
        Disconnect(EDisconnectReason::INVALID_OPERATION);
        return false;
    }

    if (_recvBuffer.GetContiguousBytes() < sizeof(StreamHeader) + streamHeader->size)
    {
        return false;
    }

    _streamReader.Init(
        _recvBuffer.GetBufferStart() + sizeof(StreamHeader),
        streamHeader->size);

    PacketHeader packetHeader;
    void const* payload = nullptr;

    while (_streamReader.ReadPacket(packetHeader, payload))
    {
        OnRecvPacket(packetHeader.id, payload, packetHeader.size);
    }

    _recvBuffer.Remove(sizeof(StreamHeader) + streamHeader->size);
    return true;
}

void IOCPSession::OnRecvCompleted(uint32_t const transferred)
{
    if (0 == transferred)
    {
        Disconnect(EDisconnectReason::RECV_ZERO);
        return;
    }

    if (_recvBuffer.GetFreeSpaceSize() < transferred)
    {
        Disconnect(EDisconnectReason::RECV_OVERFLOW);
        return;
    }

    _recvBuffer.Commit(transferred);

    while (TryProcessPacket())
    {
    }

    AsyncRecv();
}

void IOCPSession::OnSendCompleted(uint32_t const transferred)
{
    if (transferred == 0)
    {
        Disconnect(EDisconnectReason::SEND_ZERO);
        return;
    }

    std::scoped_lock lock(_sendMutex);

    _sendBuffer.Remove(transferred);
    _isSendPending = false;

    if (_sendBuffer.GetContiguousBytes() > 0)
    {
        _isSendPending = true;
        AsyncSend();
    }
}

void IOCPSession::OnRecvPacket(uint16_t const packetId, const void* payload, uint32_t const size)
{
    if (_packetHandler)
    {
        _packetHandler(packetId, payload, size);
    }
    else
    {
        // TODO: log - 핸들러 없음
    }
}

void IOCPSession::SetConnected()
{
    _state = EIOCPSessionState::CONNECTED;
    OnConnected();
    AsyncRecv();
}

void IOCPSession::SetDisconnected()
{
    if (_state == EIOCPSessionState::DISCONNECTED)
    {
        return;
    }

    _state = EIOCPSessionState::DISCONNECTED;
    OnDisconnected();
    IOCPSessionManager::Singleton::Instance().ReleaseSession(_sessionId);
}

void IOCPSession::AsyncDisconnect()
{
    auto const disconnectIoEvent = Overlapped::GetObjectPoolIOEvent(EIOType::DISCONNECT, shared_from_this());

    if (not FnDisconnectEx(reinterpret_cast<SOCKET>(GetHandle()), disconnectIoEvent, TF_REUSE_SOCKET, 0))
    {
        if (auto const error = WSAGetLastError(); error != WSA_IO_PENDING)
        {
            ObjectPool<Overlapped>::Singleton::Instance().Release(disconnectIoEvent);
            ::closesocket(reinterpret_cast<SOCKET>(GetHandle()));
        }
    }
}

void IOCPSession::AsyncRecv()
{
    if (_state != EIOCPSessionState::CONNECTED)
    {
        return;
    }

    auto const recvIoEvent = Overlapped::GetObjectPoolIOEvent(EIOType::RECV, shared_from_this());

    uint32_t freeSpace = _recvBuffer.GetFreeSpaceSize();
    if (freeSpace == 0)
    {
        //TODO: log - No free space in RecvBuffer
        return;
    }

    WSABUF wsaBuf;
    wsaBuf.buf = reinterpret_cast<char*>(_recvBuffer.GetBuffer());
    wsaBuf.len = static_cast<ULONG>(freeSpace);

    DWORD flags = 0;
    DWORD recvBytes = 0;

    if (SOCKET_ERROR == WSARecv(reinterpret_cast<SOCKET>(GetHandle()), &wsaBuf, 1, &recvBytes, &flags, recvIoEvent, NULL))
    {
        if (auto const err = WSAGetLastError(); err != WSA_IO_PENDING)
        {
            ObjectPool<Overlapped>::Singleton::Instance().Release(recvIoEvent);
            HandleError(err);
        }
    }
}

void IOCPSession::AsyncSend()
{
    if (_state != EIOCPSessionState::CONNECTED)
    {
        return;
    }

    std::scoped_lock lock(_sendMutex);

    if (_sendBuffer.GetContiguousBytes() == 0)
    {
        return;
    }

    auto const sendIoEvent = Overlapped::GetObjectPoolIOEvent(EIOType::SEND, shared_from_this());

    WSABUF wsaBuf;
    wsaBuf.buf = reinterpret_cast<CHAR*>(_sendBuffer.GetBufferStart());
    wsaBuf.len = static_cast<ULONG>(_sendBuffer.GetContiguousBytes());

    DWORD sendBytes = 0;
    DWORD flags = 0;

    if (SOCKET_ERROR == WSASend(reinterpret_cast<SOCKET>(GetHandle()), &wsaBuf, 1, &sendBytes, flags, sendIoEvent, NULL))
    {
        if (auto const err = WSAGetLastError(); err != WSA_IO_PENDING)
        {
            ObjectPool<Overlapped>::Singleton::Instance().Release(sendIoEvent);
            HandleError(err);
        }
    }
}

void IOCPSession::HandleError(int32_t const errorCode)
{
    switch (errorCode)
    {
    case WSAECONNRESET:
    case WSAECONNABORTED:
    case WSAENETRESET:
    case WSAETIMEDOUT:
        {
            Disconnect(EDisconnectReason::HANDLE_ERROR);
            break;
        }
    default:
        {
            //TODO: log - HandleError 발생, errorCode 출력
            break;
        }
    }
}