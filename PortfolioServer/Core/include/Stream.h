#pragma once
#include "CorePch.h"
#include "StreamTypes.h"
#include "LinearBuffer.h"

class Stream final
{
public:
    static uint32_t constexpr MAX_SIZE = 0x10000;

    Stream()
    {
        Reset();
    }

    void Reset()
    {
        _header = reinterpret_cast<StreamHeader*>(_buffer.data());
        _header->_size = 0;

        _writeOffset = static_cast<uint32_t>(sizeof(StreamHeader));
    }

    uint8_t* GetData()
    {
        return _buffer.data();
    }

    uint8_t const* GetData() const
    {
        return _buffer.data();
    }

    StreamHeader* GetStreamHeader()
    {
        return _header;
    }

    StreamHeader const* GetStreamHeader() const
    {
        return _header;
    }

    uint32_t GetBodySize() const
    {
        return _header ? _header->_size : 0;
    }

    uint32_t GetTotalSize() const
    {
        return _writeOffset;
    }

    uint32_t GetWritableSize() const
    {
        return MAX_SIZE - _writeOffset;
    }

    bool CanWrite(uint32_t const size) const
    {
        return size <= GetWritableSize();
    }

    bool WriteBytes(void const* data, uint32_t const size)
    {
        if (not CanWrite(size))
        {
            return false;
        }

        ::memcpy(_buffer.data() + _writeOffset, data, size);
        _writeOffset += size;

        _header->_size = _writeOffset - static_cast<uint32_t>(sizeof(StreamHeader));
        return true;
    }

    template <typename T>
    bool Write(T const& value)
    {
        return WriteBytes(std::addressof(value), static_cast<uint32_t>(sizeof(T)));
    }

    bool WriteField(void const* data, uint32_t const size)
    {
        return WriteBytes(data, size);
    }

    template <typename T>
    bool WriteField(T const& value)
    {
        return Write(value);
    }

    void IncreaseBodySize(uint32_t const size)
    {
        if (!CanWrite(size))
        {
            throw std::out_of_range("Stream::IncreaseBodySize");
        }

        _header->_size += size;
        _writeOffset += size;
    }

    static bool TryReadFromRecvBuffer(LinearBuffer& recvBuffer, Stream& outStream)
    {
        if (recvBuffer.GetReadableSize() < sizeof(StreamHeader))
        {
            return false;
        }

        auto const* header = reinterpret_cast<StreamHeader const*>(recvBuffer.GetReadPtr());

        auto const bodySize = header->_size;
        if (0 == bodySize)
        {
            return false;
        }

        uint32_t const totalSize = static_cast<uint32_t>(sizeof(StreamHeader)) + bodySize;
        if (MAX_SIZE < totalSize)
        {
            return false;
        }

        if (recvBuffer.GetReadableSize() < totalSize)
        {
            return false;
        }

        outStream.Reset();
        ::memcpy(outStream._buffer.data(), recvBuffer.GetReadPtr(), totalSize);
        outStream._header = reinterpret_cast<StreamHeader*>(outStream._buffer.data());
    	outStream._writeOffset = totalSize;

        recvBuffer.CommitRead(totalSize);

        return true;
    }

private:
    std::array<uint8_t, MAX_SIZE> _buffer{};
    StreamHeader* _header{};
    uint32_t _writeOffset{};
};
