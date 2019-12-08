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
};

enum EntityFlags
{
    EntityFlag_notInWorld = (1 << 0),
    EntityFlag_occluding = (1 << 1),
    EntityFlag_deleted = (1 << 2),
    EntityFlag_locked = (1 << 3),
};

inline b32 IsSet(u32 flags, u32 flag)
{
    b32 result = ((flags & flag) == flag);
    return result;
}

inline u32 AddFlags(u32 flags, u32 flag)
{
    u32 result = (flags | flag);
    return result;
}

inline u32 ClearFlags(u32 flags, u32 flag)
{
    u32 result = (flags & ~flag);
    return result;
}

