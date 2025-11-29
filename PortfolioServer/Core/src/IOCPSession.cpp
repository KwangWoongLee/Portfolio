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

void IOCPSession::FlushPacketStream()
{
    if (not _hasPendingStream)
    {
        return;
    }

    auto const totalSize = _pendingStream.GetTotalSize();
    if (totalSize <= sizeof(StreamHeader))
    {
        _hasPendingStream = false;
        return;
    }

    Send(reinterpret_cast<char const*>(_pendingStream.GetData()), totalSize);

    _hasPendingStream = false;
}

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

void IOCPSession::OnRecvCompleted(uint32_t const transferred)
{
    if (0 == transferred)
    {
        Disconnect(EDisconnectReason::RecvZero);
        return;
    }

    if (_recvBuffer.GetWritableSize() < transferred)
    {
        Disconnect(EDisconnectReason::RecvOverflow);
        return;
    }

    _recvBuffer.CommitWrite(transferred);

    Stream stream;
    if (Stream::TryReadFromRecvBuffer(_recvBuffer, stream))
    {
        AsyncRecv();
        return;
    }

    // TODO:: enqueue packet

    AsyncRecv();
}

void IOCPSession::OnSendCompleted(uint32_t const transferred)
{
    if (0 == transferred)
    {
        Disconnect(EDisconnectReason::SendZero);
        return;
    }

    std::scoped_lock lock(_sendMutex);

    _sendBuffer.CommitRead(transferred);
    
    _isSendPending = false;
    if (0 < _sendBuffer.GetReadableSize())
    {
        _isSendPending = true;
        AsyncSend();
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
    if (EIOCPSessionState::Connected != _state)
    {
        return;
    }

    auto const recvIoEvent = Overlapped::GetObjectPoolIOEvent(EIOType::Recv, shared_from_this());

    if (not _recvBuffer.EnsureWritable(0x1000))
    {
        return;
    }

    auto const writableSize = _recvBuffer.GetWritableSize();
    if (0 == writableSize)
    {
        return;
    }


    WSABUF wsaBuf;
    wsaBuf.buf = reinterpret_cast<char*>(_recvBuffer.GetWritePtr());
    wsaBuf.len = static_cast<ULONG>(writableSize);

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
    if (EIOCPSessionState::Connected != _state)
    {
        return;
    }

    std::scoped_lock lock(_sendMutex);

    if (0 == _sendBuffer.GetReadableSize())
    {
        return;
    }

    auto const sendIoEvent = Overlapped::GetObjectPoolIOEvent(EIOType::Send, shared_from_this());

    WSABUF wsaBuf;
    wsaBuf.buf = reinterpret_cast<CHAR*>(_sendBuffer.GetReadPtr());
    wsaBuf.len = static_cast<ULONG>(_sendBuffer.GetReadableSize());

    DWORD sendBytes{};
    DWORD constexpr flags{};
    if (SOCKET_ERROR == WSASend(reinterpret_cast<SOCKET>(GetHandle()), &wsaBuf, 1, &sendBytes, flags, sendIoEvent, nullptr))
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
            //TODO: log
            break;
        }
    }
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

        if (_sendBuffer.GetWritableSize() < contentSize)
        {
            Disconnect(EDisconnectReason::SendOverflow);
            return;
        }

        memcpy(_sendBuffer.GetWritePtr(), buffer, contentSize);
        _sendBuffer.CommitWrite(contentSize);

        if (not _isSendPending)
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
