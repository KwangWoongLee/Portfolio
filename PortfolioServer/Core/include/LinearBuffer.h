#pragma once
#include "CorePch.h"

class LinearBuffer final
{
public:
    explicit LinearBuffer(uint32_t const capacity)
        : _capacity(capacity)
    {
        _buffer = new uint8_t[_capacity];
    }

    ~LinearBuffer()
    {
        delete[] _buffer;
    }

    bool EnsureWritable(uint32_t const size)
    {
        auto const readable = GetReadableSize();
        if (size > _capacity - readable)
        {
            return false;
        }

        if (_capacity - _writeOffset >= size)
        {
            return true;
        }

        if (0 < readable)
        {
            ::memmove(_buffer, _buffer + _readOffset, readable);
        }

        _readOffset = 0;
        _writeOffset = readable;
        return true;
    }

    uint8_t* GetWritePtr()
    {
        return _buffer + _writeOffset;
    }

    uint32_t GetWritableSize() const
    {
        return _capacity - _writeOffset;
    }

    void CommitWrite(uint32_t const size)
    {
        _writeOffset += size;
    }

    uint8_t* GetReadPtr() const
    {
        return _buffer + _readOffset;
    }

    uint32_t GetReadableSize() const
    {
        return _writeOffset - _readOffset;
    }

    void CommitRead(uint32_t const size)
    {
        _readOffset += size;

        if (_readOffset == _writeOffset)
        {
            _readOffset = 0;
            _writeOffset = 0;
        }
    }

    void Clear()
    {
        _readOffset = 0;
        _writeOffset = 0;
    }

    uint32_t GetCapacity() const
    {
        return _capacity;
    }

private:
    uint8_t* _buffer = nullptr;
    uint32_t _capacity = 0;
    uint32_t _readOffset = 0;
    uint32_t _writeOffset = 0;
};