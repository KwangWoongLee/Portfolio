#pragma once
#include "IZone.h"

class FieldZone final
    : public IZone
{
public:
    explicit FieldZone(ZoneId const zoneId)
        : IZone(zoneId, EZoneType::Field)
    {
    }

    void Update() override;
};
