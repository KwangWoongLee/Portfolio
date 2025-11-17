#pragma once
#include "stdafx.h"

template<typename T_SESSION, typename T_SESSION_MANAGER>
class PacketSendTask
	: public ITask
{
public:
    PacketSendTask(int64_t const key, uint16_t const pktId, google::protobuf::MessageLite const& pkt)
	: ITask(ETaskType::BASIC, key)
	, _key(key)
	, _pktId(pktId)
	, _pkt(pkt)
    {
    }

    void Execute() override
    {
	    if (auto const session = SessionManagerT::Singleton::Instance().Find(_key))
        {
            session->SendPacket(_pktId, _pkt);
        }
    }

private:
    int64_t const _key{};
    uint16_t const _pktId{};
    google::protobuf::MessageLite const& _pkt;
};