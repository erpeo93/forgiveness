#pragma once
struct AliveComponent
{
    U32 physicalHealth;
    U32 maxPhysicalHealth;
    
    U32 mentalHealth;
    U32 maxMentalHealth;
};

struct MiscComponent
{
    R32 attackDistance;
    R32 attackContinueCoeff;
    R32 lightRadious;
};

enum EntityFlags
{
    EntityFlag_notInWorld = (1 << 0),
    EntityFlag_occluding = (1 << 1),
    EntityFlag_deleted = (1 << 2),
    EntityFlag_locked = (1 << 3),
    EntityFlag_canGoIntoWater = (1 << 4),
    EntityFlag_ghost = (1 << 5),
    EntityFlag_fearsLight = (1 << 6),
};
