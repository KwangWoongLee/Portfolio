#pragma once
#include "CorePch.h"
#include "StreamTypes.h"
#include "LinearBuffer.h"

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

class IOCPSession;

class Stream final
{
	friend class IOCPSession;
public:
    static uint32_t constexpr MAX_SIZE = 0x10000;

    Stream() { Reset(); }

    void Reset()
    {
        _header = reinterpret_cast<StreamHeader*>(_buffer.data());
        _header->_bodySize = 0;
        _writeOffset = static_cast<uint32_t>(sizeof(StreamHeader));
    }

    uint8_t* GetData() { return _buffer.data(); }
    uint8_t const* GetData() const { return _buffer.data(); }

    StreamHeader* GetStreamHeader() { return _header; }
    StreamHeader const* GetStreamHeader() const { return _header; }

    uint32_t GetBodySize() const { return _header ? _header->_bodySize : 0; }
    uint32_t GetTotalSize() const { return _writeOffset; }
    uint32_t GetWritableSize() const { return MAX_SIZE - _writeOffset; }
    bool CanWrite(uint32_t const size) const { return size <= GetWritableSize(); }

    bool WriteBytes(void const* data, uint32_t const size);

    template <typename T>
    bool Write(T const& value)
    {
        return WriteBytes(std::addressof(value), static_cast<uint32_t>(sizeof(T)));
    }

    bool WriteField(void const* data, uint32_t const size) { return WriteBytes(data, size); }

    template <typename T>
    bool WriteField(T const& value) { return Write(value); }

    template <typename... ARGS>
    static bool WritePacket(Stream& stream, uint16_t const packetId, ARGS const&... args)
    {
        auto const headerOffset = stream._writeOffset;

        PacketHeader pktHeader;
        pktHeader._id = packetId;
        pktHeader._size = 0;

        if (not stream.WriteBytes(&pktHeader, sizeof(PacketHeader)))
        {
            return false;
        }

        auto const bodyStart = stream._writeOffset;

        if (not (stream.WriteField(args) && ...))
        {
            stream._writeOffset = headerOffset;
            stream._header->_bodySize = headerOffset - static_cast<uint32_t>(sizeof(StreamHeader));
            return false;
        }

        auto* writtenHeader = reinterpret_cast<PacketHeader*>(stream._buffer.data() + headerOffset);
        writtenHeader->_size = static_cast<uint16_t>(stream._writeOffset - bodyStart);

        stream._header->_bodySize = stream._writeOffset - static_cast<uint32_t>(sizeof(StreamHeader));
        return true;
    }

private:
    static std::pair<bool, std::optional<EDisconnectReason>> TryReadFromRecvBuffer(LinearBuffer& recvBuffer, Stream& outStream);

private:
    std::array<uint8_t, MAX_SIZE> _buffer{};
    StreamHeader* _header{};
    uint32_t _writeOffset{};
};
