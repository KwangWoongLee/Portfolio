#pragma once
#include "CorePch.h"

class LinearBuffer final
{
public:
    explicit LinearBuffer(uint32_t const capacity);
    ~LinearBuffer();

    LinearBuffer(LinearBuffer const&) = delete;
    LinearBuffer& operator=(LinearBuffer const&) = delete;

    LinearBuffer(LinearBuffer&& other) noexcept;
    LinearBuffer& operator=(LinearBuffer&& other) noexcept;

    bool EnsureWritable(uint32_t const size);

    uint8_t* GetWritePtr() { return _buffer + _writeOffset; }
    uint32_t GetWritableSize() const { return _capacity - _writeOffset; }
    void CommitWrite(uint32_t const size) { _writeOffset += size; }

    uint8_t* GetReadPtr() const { return _buffer + _readOffset; }
    uint32_t GetReadableSize() const { return _writeOffset - _readOffset; }
    void CommitRead(uint32_t const size);

    void Clear() { _readOffset = 0; _writeOffset = 0; }
    uint32_t GetCapacity() const { return _capacity; }

private:
    uint8_t* _buffer = nullptr;
    uint32_t _capacity = 0;
    uint32_t _readOffset = 0;
    uint32_t _writeOffset = 0;
};
