#pragma once


printTable(noPrefix) enum SlotName
{
    Slot_None,
    Slot_Belly,
    Slot_BellySideLeft,
    Slot_BellySideRight,
    Slot_Leg,
    Slot_Shoulder,
    Slot_Back,
    Slot_RightHand,
    Slot_LeftHand,
    Slot_Count,
};

struct EquipmentSlot
{
    u64 ID;
};

inline b32 IsValid(SlotName info)
{
    b32 result = (info != Slot_None);
    return result;
}

struct ContainedObjects
{
    u8 objectCount;
    u8 maxObjectCount;
    Object objects[16];
};

struct ObjectReference
{
    u64 containerID;
    u8 objectIndex;
};


struct EquipmentAss
{
    u64 stringHashID;
    u8 index;
    
    u32 assIndex;
    
    Vec2 assOffset;
    r32 zOffset;
    r32 angle;
    Vec2 scale;
    
    union
    {
        EquipmentAss* next;
        EquipmentAss* nextFree;
    };
};

struct EquipmentLayout
{
    u64 layoutHashID;
    SlotName slot;
    
    EquipmentAss* firstEquipmentAss;
    
    union
    {
        EquipmentLayout* next;
        EquipmentLayout* nextFree;
    };
};

struct EquipmentMapping
{
    EquipmentLayout* firstEquipmentLayout;
    
    union
    {
        EquipmentMapping* next;
        EquipmentMapping* nextFree;
    };
};