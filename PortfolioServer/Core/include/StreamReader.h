#pragma once
#include "StreamHeader.h"

class StreamReader
{
public:
    StreamReader() = default;

    void Init(uint8_t* buffer, uint32_t size)
    {
        _buffer = buffer;
        _size = size;
        _offset = 0;
    }

    bool ReadPacket(PacketHeader& outHeader, const void*& outPayload)
    {
        if (_offset + sizeof(PacketHeader) > _size)
            return false;

        memcpy(&outHeader, _buffer + _offset, sizeof(PacketHeader));
        _offset += sizeof(PacketHeader);

        if (_offset + outHeader.size > _size)
            return false;

        outPayload = _buffer + _offset;
        _offset += outHeader.size;
        return true;
    }

private:
    uint8_t* _buffer = nullptr;
    uint32_t _size = 0;
    uint32_t _offset = 0;
};
