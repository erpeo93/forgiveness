#pragma once
struct PossibleActionList
{
    u16 actionCount;
    u16 actions[8];
};

enum InteractionType
{
    Interaction_Ground,
    Interaction_Equipment,
    Interaction_Container,
    Interaction_Equipped,
    Interaction_Dragging,
    Interaction_MoveContainerOnScreen,
    
    Interaction_Count
};

struct InteractionComponent
{
    b32 isOnFocus;
    PossibleActionList actions[Interaction_Count];
};

struct EntityHotInteraction
{
    InteractionType type;
    u16 actionCount;
    u16 actions[8];
    
    u16 objectIndex;
    EntityID containerIDServer;
    EntityID entityIDServer;
};

inline b32 AreEqual(EntityHotInteraction i1, EntityHotInteraction i2)
{
    b32 result = (i1.type == i2.type && 
                  AreEqual(i1.containerIDServer, i2.containerIDServer) &&
                  AreEqual(i1.entityIDServer, i2.entityIDServer));
    return result;
}

struct GameUIContext
{
    char tooltip[128];
    b32 multipleActions;
    
    b32 inventoryMode;
    b32 lootingMode;
    b32 testingDraggingOnEquipment;
    EntityID draggingIDServer;
    EntityID draggingContainerIDServer;
    
    EntityID lootingIDServer;
    EntityID openIDLeft;
    EntityID openIDRight;
    
    EntityID predictionContainerID;
    EntityID predictionObjectID;
    r32 predictionTime;
    
    i32 currentHotIndex;
    i32 currentActionIndex;
    
    u32 hotCount;
    EntityHotInteraction hotInteractions[8];
    EntityHotInteraction lastFrameHotInteraction;
    
    Vec2 equipmentPositions[Count_equipmentSlot];
    Rect2 equipmentOnScreen[Count_equipmentSlot];
};