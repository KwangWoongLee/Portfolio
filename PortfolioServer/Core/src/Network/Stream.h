#pragma once
#pragma pack(push, 1)
struct StreamHeader final
{
	uint16_t _id;
	uint32_t _bodySize;
};
#pragma pack(pop)

static auto constexpr SIZE_OF_STREAM_HEADER = sizeof(StreamHeader);