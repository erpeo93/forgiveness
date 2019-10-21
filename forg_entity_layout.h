#pragma once
struct EquipmentComponent
{
    EntityID IDs[Count_equipmentSlot];
};

struct UsingComponent
{
    EntityID IDs[Count_usingSlot];
};

struct ObjectMapping
{
    u64 slotHash;
    u64 pieceHash;
    EntityID ID;
};

struct EquipmentMappingComponent
{
    ObjectMapping mappings[Count_equipmentSlot];
};

struct UsingMappingComponent
{
    ObjectMapping mappings[Count_usingSlot];
};

enum EntityFlags
{
    EntityFlag_equipment = (1 << 0),
};

inline b32 IsSet(u32 flags, u32 flag)
{
    b32 result = ((flags & flag) == flag);
    return result;
}