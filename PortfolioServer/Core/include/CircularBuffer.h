#pragma once
#include "stdafx.h"

class CircularBuffer final
{
public:
    CircularBuffer(uint32_t const capacity)
        : _capacity(capacity)
    {
        _buffer = new uint8_t[_capacity];
    }

    ~CircularBuffer()
    {
        delete[] _buffer;
    }

    uint8_t* GetBuffer()
    {
        if (_writeOffset >= _readOffset)
        {
            return _buffer + _writeOffset;
        }

    	if (_writeOffset == 0)
        {
	        return _buffer;
        }

    	return _buffer + _writeOffset;
    }

    uint8_t* GetBufferStart()
    {
        return _buffer + _readOffset;
    }

    uint32_t GetFreeSpaceSize() const
    {
        if (_writeOffset >= _readOffset)
        {
            return _capacity - (_writeOffset - _readOffset) - 1;
        }
        else
        {
            return (_readOffset - _writeOffset) - 1;
        }
    }

    uint32_t GetContiguousBytes() const
    {
        if (_readOffset <= _writeOffset)
        {
            return _writeOffset - _readOffset;
        }
        else
        {
            return _capacity - _readOffset;
        }
    }

    void Commit(uint32_t const size)
    {
        _writeOffset = (_writeOffset + size) % _capacity;
    }

    void Remove(uint32_t const size)
    {
        _readOffset = (_readOffset + size) % _capacity;
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
