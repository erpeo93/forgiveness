#pragma once
struct BoundedEntityID
{
    EntityID ID;
    b32 dirty;
};

inline void SetBoundedID(BoundedEntityID* bounded, EntityID ID)
{
    //bounded->dirty = (!AreEqual(ID, bounded->ID));
    bounded->dirty = true;
    bounded->ID = ID;
}

inline EntityID GetBoundedID(BoundedEntityID bounded)
{
    EntityID result = bounded.ID;
    return result;
}

inline b32 IsValidID(BoundedEntityID ID)
{
    b32 result = IsValidID(ID.ID);
    return result;
}

inline b32 AreEqual(BoundedEntityID i1, BoundedEntityID i2)
{
    b32 result = (i1.ID.archetype_archetypeIndex == i2.ID.archetype_archetypeIndex);
    return result;
}

inline b32 AreEqual(BoundedEntityID i1, EntityID i2)
{
    b32 result = (i1.ID.archetype_archetypeIndex == i2.archetype_archetypeIndex);
    return result;
}

inline b32 IsDirty(BoundedEntityID i)
{
    b32 result = i.dirty;
    return result;
}

inline void ResetDirty(BoundedEntityID* i)
{
    i->dirty = false;
}

enum InventorySlotFlags
{
    InventorySlot_Locked = (1 << 16),
};

#ifdef FORG_SERVER
struct InventorySlot
{
    u32 flags_type;
    BoundedEntityID ID_;
};

inline EntityID GetBoundedID(InventorySlot* slot)
{
    EntityID result = GetBoundedID(slot->ID_);
    return result;
}

inline void SetBoundedID(InventorySlot* slot, EntityID ID)
{
    SetBoundedID(&slot->ID_, ID);
}

inline b32 AreEqual(InventorySlot* slot, EntityID ID)
{
    b32 result = AreEqual(slot->ID_, ID);
    return result;
}

inline b32 IsDirty(InventorySlot* slot)
{
    b32 result = IsDirty(slot->ID_);
    return result;
}

inline void ResetDirty(InventorySlot* slot)
{
    ResetDirty(&slot->ID_);
}

#else
struct InventorySlot
{
    u32 flags_type;
    EntityID ID;
};
#endif

struct EquipmentComponent
{
    InventorySlot slots[Count_equipmentSlot];
};

struct UsingComponent
{
    BoundedEntityID draggingID;
    InventorySlot slots[Count_usingSlot];
};

#define MAX_CONTAINER_OBJECT 64
#define MAX_USING_OBJECT 8
struct ContainerComponent
{
    BoundedEntityID openedBy;
    
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
    
    b32 hot;
    r32 zoomCoeff;
    r32 zoomSpeed;
    r32 maxZoomCoeff;
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
    b32 displayInStandardMode;
    Vec2 desiredOpenedDim;
    Vec2 desiredUsingDim;
    
    ObjectMapping storedMappings[MAX_CONTAINER_OBJECT];
    ObjectMapping usingMappings[MAX_USING_OBJECT];
};

#define MAX_RECIPE_ESSENCES 1
struct RecipeEssenceComponent
{
    Rect2 projectedOnScreen[MAX_RECIPE_ESSENCES];
    u16 essences[MAX_RECIPE_ESSENCES];
};