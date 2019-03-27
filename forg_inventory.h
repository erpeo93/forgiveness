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
    u64 recipeIndex;
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
    u32 slotCount;
    SlotName slots[4];
};

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