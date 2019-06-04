#pragma once

enum RarityType
{
    Rarity_Common,
    Rarity_UnCommon,
    Rarity_Rare,
    Rarity_VeryRare,
    Rarity_Legendary,
    Rarity_Unique,
    
    Rarity_Count,
};

struct Object
{
    u32 taxonomy;
    GenerationData gen;
    u16 quantity;
    i16 status;
};

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

struct EquipInfo
{
    SlotName slot;
};

inline b32 IsValid(EquipInfo info)
{
    b32 result = (info.slot != Slot_None);
    return result;
}

inline b32 AreEqual(EquipInfo i1, EquipInfo i2)
{
    b32 result = (i1.slot == i2.slot);
    return result;
}

inline SlotName GetMainSlot(EquipInfo info)
{
    SlotName result = info.slot;
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

inline b32 IsRecipe(Object* object)
{
    b32 result = (object->quantity == 0xffff);
    return result;
}

