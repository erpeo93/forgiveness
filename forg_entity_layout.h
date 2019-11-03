#pragma once
struct InventorySlot
{
    u16 type;
    EntityID ID;
};

struct EquipmentComponent
{
    InventorySlot slots[Count_equipmentSlot];
};

struct UsingComponent
{
    InventorySlot slots[Count_usingSlot];
};

#define MAX_CONTAINER_OBJECT 64
#define MAX_USING_OBJECT 8
struct ContainerComponent
{
    EntityID openedBy;
    
    InventorySlot storedObjects[MAX_CONTAINER_OBJECT];
    InventorySlot usingObjects[MAX_USING_OBJECT];
};

struct ObjectMapping
{
    r32 distanceFromMouseSq;
    Rect2 projOnScreen;
    u64 slotHash;
    u64 pieceHash;
    InventorySlot object;
};

struct EquipmentMappingComponent
{
    ObjectMapping mappings[Count_equipmentSlot];
};

struct UsingMappingComponent
{
    ObjectMapping mappings[Count_usingSlot];
};



struct ContainerMappingComponent
{
    EntityID openedBy;
    r32 zoomCoeff;
    Vec2 desiredOpenedDim;
    Vec2 desiredUsingDim;
    
    ObjectMapping storedMappings[MAX_CONTAINER_OBJECT];
    ObjectMapping usingMappings[MAX_USING_OBJECT];
};

enum EntityFlags
{
    EntityFlag_notInWorld = (1 << 0),
    EntityFlag_occluding = (1 << 1),
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
