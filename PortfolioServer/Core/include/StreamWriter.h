#pragma once
#include "StreamHeader.h"
//#include <google/protobuf/message_lite.h>

class StreamWriter
{
public:
    StreamWriter() = default;

    void Init(uint8_t* buffer, uint32_t maxSize)
    {
        _buffer = buffer;
        _capacity = maxSize;
        _offset = sizeof(StreamHeader);
        _packetCount = 0;
    }

    /*bool WritePacket(uint16_t packetId, google::protobuf::MessageLite& packet)
    {
        auto const  packetSize = static_cast<uint32_t>(packet.ByteSizeLong());
        auto const requiredSize = sizeof(PacketHeader) + packetSize;

        if (_offset + requiredSize > _capacity)
            return false;

        PacketHeader header{packetId, packetSize};
        memcpy(_buffer + _offset, &header, sizeof(PacketHeader));
        _offset += sizeof(PacketHeader);

        if (!packet.SerializeToArray(_buffer + _offset, packetSize))
            return false;

        _offset += packetSize;
        ++_packetCount;
        return true;
    }*/

    void Finalize()
    {
        auto* streamHeader = reinterpret_cast<StreamHeader*>(_buffer);
        streamHeader->size = _offset - sizeof(StreamHeader);
    }

    uint32_t GetSize() const
    {
        return _offset;
    }
    uint32_t GetPacketCount() const
    {
        return _packetCount;
    }

private:
    uint8_t* _buffer = nullptr;
    uint32_t _capacity = 0;
    uint32_t _offset = 0;
    uint32_t _packetCount = 0;
};
