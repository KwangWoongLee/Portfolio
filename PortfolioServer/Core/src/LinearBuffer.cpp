#include "CorePch.h"
#include "LinearBuffer.h"

LinearBuffer::LinearBuffer(uint32_t const capacity)
    : _capacity(capacity)
{
    _buffer = new uint8_t[_capacity];
}

LinearBuffer::~LinearBuffer()
{
    delete[] _buffer;
}

LinearBuffer::LinearBuffer(LinearBuffer&& other) noexcept
    : _buffer(other._buffer), _capacity(other._capacity)
    , _readOffset(other._readOffset), _writeOffset(other._writeOffset)
{
    other._buffer = nullptr;
    other._capacity = 0;
    other._readOffset = 0;
    other._writeOffset = 0;
}

LinearBuffer& LinearBuffer::operator=(LinearBuffer&& other) noexcept
{
    if (this != &other)
    {
        delete[] _buffer;
        _buffer = other._buffer;
        _capacity = other._capacity;
        _readOffset = other._readOffset;
        _writeOffset = other._writeOffset;
        other._buffer = nullptr;
        other._capacity = 0;
        other._readOffset = 0;
        other._writeOffset = 0;
    }
    return *this;
}

bool LinearBuffer::EnsureWritable(uint32_t const size)
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

void LinearBuffer::CommitRead(uint32_t const size)
{
    _readOffset += size;

    if (_readOffset == _writeOffset)
    {
        _readOffset = 0;
        _writeOffset = 0;
    }
}
