#include "CorePch.h"
#include "IOCPSession.h"

#include "IOCPSessionManager.h"
#include "PacketTask.h"
#include "SocketUtil.h"
#include "TaskDispatcher.h"

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
    auto expected = EIOCPSessionState::Connected;
    if (not _state.compare_exchange_strong(expected, EIOCPSessionState::Disconnecting))
    {
        return;
    }

    AsyncDisconnect();
}

void IOCPSession::FlushPacketStream()
{
    std::scoped_lock lock(_sendMutex);
    FlushPendingStreamLocked();
}

void IOCPSession::FlushPendingStreamLocked()
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

    SendLocked(reinterpret_cast<char const*>(_pendingStream.GetData()), totalSize);

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
    while (true)
    {
	    if (auto const [isSuccess, disConnectReasonOpt] = Stream::TryReadFromRecvBuffer(_recvBuffer, stream); not isSuccess)
        {
            if (disConnectReasonOpt.has_value())
            {
                Disconnect(disConnectReasonOpt.value());
			}

            break;
        }

		HandleStream(stream);
        stream.Reset();
    }

    AsyncRecv();
}

void IOCPSession::OnSendCompleted(uint32_t const transferred)
{
    if (0 == transferred)
    {
        Disconnect(EDisconnectReason::SendZero);
        return;
    }

    bool isNeedToSend{ false };
    {
        std::scoped_lock lock(_sendMutex);

        _sendBuffer.CommitRead(transferred);

        _isSendPending = false;
        if (0 < _sendBuffer.GetReadableSize())
        {
            _isSendPending = true;
            isNeedToSend = true;
        }
    }

    if (isNeedToSend)
    {
        AsyncSend();
	}
}

void IOCPSession::HandleStream(Stream& outStream)
{
    auto const* data = outStream.GetData();

    auto const* streamHeader = reinterpret_cast<StreamHeader const*>(data);
    uint32_t const totalSize = static_cast<uint32_t>(sizeof(StreamHeader)) + streamHeader->_bodySize;

    uint32_t offset = sizeof(StreamHeader);

    while (offset + sizeof(PacketHeader) <= totalSize)
    {
        auto const* pktHeader = reinterpret_cast<PacketHeader const*>(data + offset);

        uint16_t const packetId = pktHeader->_id;
        uint16_t const packetBodySize = pktHeader->_size;

        uint32_t const packetBodyStart = offset + static_cast<uint32_t>(sizeof(PacketHeader));
        uint32_t const packetBodyEnd = packetBodyStart + packetBodySize;

        if (packetBodyEnd > totalSize)
        {
            Disconnect(EDisconnectReason::InvalidOperation);
            return;
        }

        void const* payload = data + packetBodyStart;

        std::vector<uint8_t> payloadCopy;
        payloadCopy.resize(packetBodySize);

        if (packetBodySize > 0)
        {
            ::memcpy(payloadCopy.data(), payload, packetBodySize); // TODO: optimize
        }

        auto task = std::make_shared<PacketRecvTask>(_sessionId, packetId, std::move(payloadCopy));
        TaskDispatcher::Singleton::GetInstance().Dispatch(task);

        offset = packetBodyEnd;
    }

    if (offset != totalSize)
    {
        Disconnect(EDisconnectReason::InvalidOperation);
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
        if (auto const err = WSAGetLastError(); err != WSA_IO_PENDING)
        {
            ObjectPool<Overlapped>::Singleton::GetInstance().Release(disconnectIoEvent);
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
		ObjectPool<Overlapped>::Singleton::GetInstance().Release(recvIoEvent);
        Disconnect(EDisconnectReason::RecvOverflow);
    	return;
    }

    auto const writableSize = _recvBuffer.GetWritableSize();
    if (0 == writableSize)
    {
        ObjectPool<Overlapped>::Singleton::GetInstance().Release(recvIoEvent);
        return;
    }


    WSABUF wsaBuf;
    wsaBuf.buf = reinterpret_cast<char*>(_recvBuffer.GetWritePtr());
    wsaBuf.len = static_cast<ULONG>(writableSize);

    DWORD flags = 0;
    DWORD recvBytes = 0;

    if (SOCKET_ERROR == WSARecv(reinterpret_cast<SOCKET>(GetHandle()), &wsaBuf, 1, &recvBytes, &flags, recvIoEvent, nullptr))
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

void IOCPSession::SendLocked(char const* buffer, uint32_t const contentSize)
{
    if (_state != EIOCPSessionState::Connected)
    {
        return;
    }

    if (_sendBuffer.GetWritableSize() < contentSize)
    {
        Disconnect(EDisconnectReason::SendOverflow);
        return;
    }

    ::memcpy(_sendBuffer.GetWritePtr(), buffer, contentSize);
    _sendBuffer.CommitWrite(contentSize);

    if (not _isSendPending)
    {
        _isSendPending = true;
        AsyncSend();
    }
}
