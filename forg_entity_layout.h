#pragma once
struct EquipmentComponent
{
    EntityID IDs[Count_equipmentSlot];
};

struct UsingComponent
{
    EntityID IDs[Count_usingSlot];
};

#define MAX_CONTAINER_OBJECT 64
#define MAX_USING_OBJECT 8
struct ContainerComponent
{
    EntityID openedBy;
    
    u32 maxStoredCount;
    EntityID storedIDs[MAX_CONTAINER_OBJECT];
    
    u32 maxUsingCount;
    EntityID usingIDs[MAX_USING_OBJECT];
};

struct ObjectMapping
{
    Rect2 projOnScreen;
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



struct ContainerMappingComponent
{
    EntityID openedBy;
    r32 zoomCoeff;
    Vec2 desiredOpenedDim;
    
    ObjectMapping storedMappings[MAX_CONTAINER_OBJECT];
    ObjectMapping usingMappings[MAX_USING_OBJECT];
};

enum EntityFlags
{
    EntityFlag_notInWorld = (1 << 0),
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