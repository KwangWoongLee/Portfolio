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

class Stream final
{
    friend class StreamWriter;

public:
    static uint32_t constexpr MAX_SIZE = 0x10000;

    Stream() { Reset(); }

    void Reset()
    {
        _header = reinterpret_cast<StreamHeader*>(_buffer.data());
        _header->_bodySize = 0;
        _writeOffset = static_cast<uint32_t>(sizeof(StreamHeader));
    }

    uint8_t const* GetData() const { return _buffer.data(); }
    uint32_t GetTotalSize() const { return _writeOffset; }
    uint32_t GetBodySize() const { return _header ? _header->_bodySize : 0; }
    StreamHeader const* GetStreamHeader() const { return _header; }

    static std::pair<bool, std::optional<EDisconnectReason>> TryReadFromRecvBuffer(LinearBuffer& recvBuffer, Stream& outStream);

private:
    std::array<uint8_t, MAX_SIZE> _buffer{};
    StreamHeader* _header{};
    uint32_t _writeOffset{};
};

class StreamWriter final
{
public:
    explicit StreamWriter(Stream& stream) : _stream(stream) {}

    StreamWriter(StreamWriter const&) = delete;
    StreamWriter& operator=(StreamWriter const&) = delete;
    StreamWriter(StreamWriter&&) = delete;
    StreamWriter& operator=(StreamWriter&&) = delete;

    bool WriteBytes(void const* data, uint32_t const size);

    template <typename T>
    bool Write(T const& value)
    {
        return WriteBytes(std::addressof(value), static_cast<uint32_t>(sizeof(T)));
    }

    bool WriteField(void const* data, uint32_t const size) { return WriteBytes(data, size); }

    template <typename T>
    bool WriteField(T const& value) { return Write(value); }

    template <typename T>
    bool WriteField(std::vector<T> const& vec)
    {
        auto const count = static_cast<uint32_t>(vec.size());
        if (not Write(count))
        {
            return false;
        }
        for (auto const& item : vec)
        {
            if (not WriteField(item))
            {
                return false;
            }
        }
        return true;
    }

    template <typename... ARGS>
    static bool WritePacket(StreamWriter& writer, uint16_t const packetId, ARGS const&... args)
    {
        auto& s = writer._stream;
        auto const headerOffset = s._writeOffset;

        PacketHeader pktHeader;
        pktHeader._id = packetId;
        pktHeader._size = 0;

        if (not writer.WriteBytes(&pktHeader, sizeof(PacketHeader)))
        {
            return false;
        }

        auto const bodyStart = s._writeOffset;

        if (not (writer.WriteField(args) && ...))
        {
            s._writeOffset = headerOffset;
            s._header->_bodySize = headerOffset - static_cast<uint32_t>(sizeof(StreamHeader));
            return false;
        }

        auto* writtenHeader = reinterpret_cast<PacketHeader*>(s._buffer.data() + headerOffset);
        writtenHeader->_size = static_cast<uint16_t>(s._writeOffset - bodyStart);

        s._header->_bodySize = s._writeOffset - static_cast<uint32_t>(sizeof(StreamHeader));
        return true;
    }

private:
    Stream& _stream;
};

class StreamReader final
{
public:
    StreamReader(void const* const data, uint32_t const size)
        : _data(static_cast<uint8_t const*>(data))
        , _size(size)
    {
    }

    explicit StreamReader(Stream const& stream)
        : _data(stream.GetData())
        , _size(stream.GetTotalSize())
    {
    }

    StreamReader(StreamReader const&) = delete;
    StreamReader& operator=(StreamReader const&) = delete;
    StreamReader(StreamReader&&) = delete;
    StreamReader& operator=(StreamReader&&) = delete;

    uint32_t GetReadableSize() const { return _size - _readOffset; }
    bool CanRead(uint32_t const size) const { return size <= GetReadableSize(); }

    bool ReadBytes(void* outData, uint32_t const size);

    template <typename T>
    bool Read(T& out)
    {
        return ReadBytes(std::addressof(out), static_cast<uint32_t>(sizeof(T)));
    }

    template <typename T>
    bool ReadField(T& out) { return Read(out); }

    template <typename T>
    bool ReadField(std::vector<T>& vec)
    {
        uint32_t count{};
        if (not Read(count))
        {
            return false;
        }
        if (Stream::MAX_SIZE < count)
        {
            return false;
        }
        vec.resize(count);
        for (auto& item : vec)
        {
            if (not ReadField(item))
            {
                return false;
            }
        }
        return true;
    }

    template <typename... ARGS>
    bool ReadFields(ARGS&... args)
    {
        return (ReadField(args) && ...);
    }

private:
    uint8_t const* _data;
    uint32_t _size;
    uint32_t _readOffset{};
};
