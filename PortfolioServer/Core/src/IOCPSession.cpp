#include "CorePch.h"
#include "IOCPSession.h"

#include "IOCPSessionManager.h"
#include "SocketUtil.h"

IOCPSession::~IOCPSession()
{
    if (auto const handle = GetHandle(); INVALID_HANDLE_VALUE != handle)
    {
        SocketUtil::Singleton::GetInstance().CloseSocket(reinterpret_cast<SOCKET>(handle));
    }
}

void IOCPSession::Dispatch(Overlapped const* iocpEvent, uint32_t const numOfBytes)
{
    switch (iocpEvent->GetIOType())
    {
    case EIOType::Connect:
        {
    		OnConnectCompleted();
        } break;
    case EIOType::Disconnect:
        {
            OnDisconnectCompleted();
        } break;
    case EIOType::Send:
        {
            OnSendCompleted(numOfBytes);
        } break;
    case EIOType::Recv:
        {
            OnRecvCompleted(numOfBytes);
        } break;
    default:
        {} break;
    }
}

void IOCPSession::Disconnect(EDisconnectReason const reason)
{
    if (_state != EIOCPSessionState::Connected)
    {
        return;
    }

    AsyncDisconnect();
}

void IOCPSession::Send(char const* buffer, uint32_t const contentSize)
{
    if (_state != EIOCPSessionState::Connected)
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

/*void IOCPSession::SendPacket(uint16_t const packetId, google::protobuf::MessageLite& packet)
{
    if (_state != EIOCPSessionState::Connected)
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
}*/

void IOCPSession::OnAcceptCompleted()
{
    auto const socket = reinterpret_cast<SOCKET>(GetHandle());

    std::ignore = SocketUtil::Singleton::GetInstance().SetKeepAlive(socket, true);
    std::ignore = SocketUtil::Singleton::GetInstance().SetTcpNoDelay(socket, true);

    SetConnected();
}

void IOCPSession::OnConnectCompleted()
{
    auto const socket = reinterpret_cast<SOCKET>(GetHandle());

    std::ignore = SocketUtil::Singleton::GetInstance().SetKeepAlive(socket, true);
    std::ignore = SocketUtil::Singleton::GetInstance().SetTcpNoDelay(socket, true);

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
        Disconnect(EDisconnectReason::InvalidOperation);
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
        /*on_recv_packet(packetHeader.id, payload, packetHeader.size);*/
    }

    _recvBuffer.Remove(sizeof(StreamHeader) + streamHeader->size);
    return true;
}

void IOCPSession::OnRecvCompleted(uint32_t const transferred)
{
    if (0 == transferred)
    {
        Disconnect(EDisconnectReason::RecvZero);
        return;
    }

    if (_recvBuffer.GetFreeSpaceSize() < transferred)
    {
        Disconnect(EDisconnectReason::RecvOverflow);
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
        Disconnect(EDisconnectReason::SendZero);
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

void IOCPSession::on_recv_packet(uint16_t const packetId, void const* payload, uint32_t const size)
{
    if (_packetHandler)
    {
        _packetHandler(packetId, payload, size);
    }
    else
    {
        // TODO: log
    }
} 

void IOCPSession::SetConnected()
{
    _state = EIOCPSessionState::Connected;
    OnConnected();
    AsyncRecv();
}

void IOCPSession::SetDisconnected()
{
    if (_state == EIOCPSessionState::Disconnected)
    {
        return;
    }

    _state = EIOCPSessionState::Disconnected;
    OnDisconnected();
    IOCPSessionManager::Singleton::GetInstance().ReleaseSession(_sessionId);
}

void IOCPSession::AsyncDisconnect()
{
    auto const disconnectIoEvent = Overlapped::GetObjectPoolIOEvent(EIOType::Disconnect, shared_from_this());

    if (not fnDisconnectEx(reinterpret_cast<SOCKET>(GetHandle()), disconnectIoEvent, TF_REUSE_SOCKET, 0))
    {
        if (auto const error = WSAGetLastError(); error != WSA_IO_PENDING)
        {
            ObjectPool<Overlapped>::Singleton::GetInstance().Release(disconnectIoEvent);
            ::closesocket(reinterpret_cast<SOCKET>(GetHandle()));
        }
    }
}

void IOCPSession::AsyncRecv()
{
    if (_state != EIOCPSessionState::Connected)
    {
        return;
    }

    auto const recvIoEvent = Overlapped::GetObjectPoolIOEvent(EIOType::Recv, shared_from_this());

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
            ObjectPool<Overlapped>::Singleton::GetInstance().Release(recvIoEvent);
            HandleError(err);
        }
    }
}

void IOCPSession::AsyncSend()
{
    if (_state != EIOCPSessionState::Connected)
    {
        return;
    }

    std::scoped_lock lock(_sendMutex);

    if (_sendBuffer.GetContiguousBytes() == 0)
    {
        return;
    }

    auto const sendIoEvent = Overlapped::GetObjectPoolIOEvent(EIOType::Send, shared_from_this());

    WSABUF wsaBuf;
    wsaBuf.buf = reinterpret_cast<CHAR*>(_sendBuffer.GetBufferStart());
    wsaBuf.len = static_cast<ULONG>(_sendBuffer.GetContiguousBytes());

    DWORD sendBytes = 0;
    DWORD flags = 0;

    if (SOCKET_ERROR == WSASend(reinterpret_cast<SOCKET>(GetHandle()), &wsaBuf, 1, &sendBytes, flags, sendIoEvent, NULL))
    {
        if (auto const err = WSAGetLastError(); err != WSA_IO_PENDING)
        {
            ObjectPool<Overlapped>::Singleton::GetInstance().Release(sendIoEvent);
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
            Disconnect(EDisconnectReason::HandleError);
            break;
        }
    default:
        {
            //TODO: log - HandleError 발생, errorCode 출력
            break;
        }
    }
}