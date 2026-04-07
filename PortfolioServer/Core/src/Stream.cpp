#include "CorePch.h"
#include "Stream.h"

bool Stream::WriteBytes(void const* data, uint32_t const size)
{
    if (not CanWrite(size))
    {
        return false;
    }

    ::memcpy(_buffer.data() + _writeOffset, data, size);
    _writeOffset += size;

    _header->_bodySize = _writeOffset - static_cast<uint32_t>(sizeof(StreamHeader));
    return true;
}

std::pair<bool, std::optional<EDisconnectReason>> Stream::TryReadFromRecvBuffer(LinearBuffer& recvBuffer, Stream& outStream)
{
    if (recvBuffer.GetReadableSize() < sizeof(StreamHeader))
    {
        return { false, std::nullopt };
    }

    auto const* header = reinterpret_cast<StreamHeader const*>(recvBuffer.GetReadPtr());

    auto const bodySize = header->_bodySize;
    if (0 == bodySize)
    {
        return { false, EDisconnectReason::InvalidOperation };
    }

    uint32_t const totalSize = static_cast<uint32_t>(sizeof(StreamHeader)) + bodySize;
    if (MAX_SIZE < totalSize)
    {
        return { false, EDisconnectReason::InvalidOperation };
    }

    if (recvBuffer.GetReadableSize() < totalSize)
    {
        return { false, std::nullopt };
    }

    outStream.Reset();
    ::memcpy(outStream._buffer.data(), recvBuffer.GetReadPtr(), totalSize);
    outStream._header = reinterpret_cast<StreamHeader*>(outStream._buffer.data());
    outStream._writeOffset = totalSize;

    recvBuffer.CommitRead(totalSize);

    return { true, std::nullopt };
}
