#pragma once
struct PossibleAction
{
    u16 action;
    r32 distanceSq;
};

struct PossibleActionList
{
    u16 actionCount;
    PossibleAction actions[8];
};

enum InteractionType
{
    Interaction_Ground,
    Interaction_Equipment,
    Interaction_Container,
    Interaction_Equipped,
    Interaction_Dragging,
    Interaction_MoveContainerOnScreen,
    Interaction_SkillSelection,
    
    Interaction_Count
};

enum InteractionListType
{
    InteractionList_Ground,
    InteractionList_Equipment,
    InteractionList_Container,
    InteractionList_Equipped,
    InteractionList_Count,
};

struct UsingEquipOption
{
    u16 slots[2];
};

struct InteractionComponent
{
    b32 isOnFocus;
    PossibleActionList actions[InteractionList_Count];
    
    u32 usingConfigurationCount;
    UsingEquipOption usingConfigurations[4];
    
    u32 equipConfigurationCount;
    UsingEquipOption equipConfigurations[4];
    
    u16 inventorySlotType;
};

internal b32 ActionIsPossible(InteractionComponent* interaction, u16 action, r32 distanceSq)
{
    b32 result = false;
    if(interaction)
    {
        PossibleActionList* list = interaction->actions + Interaction_Ground;
        for(u32 actionIndex = 0; actionIndex < list->actionCount; ++actionIndex)
        {
            PossibleAction* possibleAction = list->actions + actionIndex;
            if(possibleAction->action == action)
            {
                if(distanceSq <= possibleAction->distanceSq)
                {
                    result = true;
                }
                break;
            }
        }
    }
    return result;
}

struct EntityHotInteraction
{
    InteractionType type;
    u16 actionCount;
    u16 actions[8];
    
    u16 objectIndex;
    InventorySlot* slot;
    EntityID containerIDServer;
    EntityID entityIDServer;
    u16 optionIndex;
    u16 skillIndex;
};

inline b32 AreEqual(EntityHotInteraction i1, EntityHotInteraction i2)
{
    b32 result = (i1.type == i2.type && 
                  AreEqual(i1.containerIDServer, i2.containerIDServer) &&
                  AreEqual(i1.entityIDServer, i2.entityIDServer));
    return result;
}

enum LockedInteractionType
{
    LockedInteraction_None,
    LockedInteraction_SkillTarget,
    LockedInteraction_SkillOffset,
    LockedInteraction_ReachTarget,
    LockedInteraction_Completed,
};

struct GameUIContext
{
    b32 initialized;
    
    char tooltip[128];
    b32 multipleActions;
    
    b32 inventoryMode;
    b32 lootingMode;
    
    b32 testingDraggingOnEquipment;
    UsingEquipOption* draggingTestUsingOption;
    UsingEquipOption* draggingTestEquipOption;
    
    EntityID draggingIDServer;
    EntityID draggingContainerIDServer;
    
    EntityID lootingIDServer;
    EntityID openIDLeft;
    EntityID openIDRight;
    
    EntityID predictionContainerID;
    EntityID predictionObjectID;
    r32 predictionTime;
    r32 ignoreDraggingMappingTimer;
    
    i32 currentHotIndex;
    i32 currentActionIndex;
    
    b32 anyValidInteraction;
    u32 hotCount;
    EntityHotInteraction hotInteractions[8];
    EntityHotInteraction lastFrameHotInteraction;
    
    Vec2 equipmentPositions[Count_equipmentSlot];
    Rect2 equipmentOnScreen[Count_equipmentSlot];
    
    
    u32 selectedSkillIndex;
    u32 hotSkillIndex;
    Rect2 skillRects[MAX_ACTIVE_SKILLS];
    
    GameCommand standardCommand;
    
    LockedInteractionType lockedInteractionType;
    GameCommand lockedCommand;
    b32 keyboardInteractionDisabled;
    
    
    CommandParameters commandParameters;
};