#pragma once

enum ActionDistanceType
{
    ActionDistance_Standard,
    ActionDistance_Special,
};

struct PossibleActionDistance
{
    u16 type;
    union
    {
        struct
        {
            r32 startDistance;
            r32 continueCoeff;
        };
        
        u16 propertyIndex;
    };
};
struct PossibleAction
{
    u16 action;
    PossibleActionDistance distance;
    r32 time;
    EntityType requiredUsingType;
    EntityType requiredEquippedType;
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
    Interaction_SelectRecipeEssence,
    
    Interaction_Count
};

enum InteractionListType
{
    InteractionList_Ground,
    InteractionList_Equipment,
    InteractionList_Container,
    InteractionList_Equipped,
    InteractionList_Dragging,
    InteractionList_Count,
};

enum UsingEquipOptionType
{
    UsingEquip_SlotIndex,
    UsingEquip_SlotType,
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

inline b32 IsValid(EntityType type);
inline b32 AreEqual(EntityType r1, EntityType r2);
internal PossibleAction* ActionIsPossible(InteractionComponent* interaction, u16 action, EntityType* equipped, u32 equippedCount, EntityType usingType = {})
{
    PossibleAction* result = 0;
    if(interaction)
    {
        if(IsValid(usingType))
        {
            PossibleActionList* list = interaction->actions + InteractionList_Dragging;
            for(u32 actionIndex = 0; actionIndex < list->actionCount; ++actionIndex)
            {
                PossibleAction* possibleAction = list->actions + actionIndex;
                if(possibleAction->action == action && AreEqual(usingType, possibleAction->requiredUsingType))
                {
                    result = possibleAction;
                    break;
                }
            }
        }
        else
        {
            PossibleActionList* list = interaction->actions + InteractionList_Ground;
            for(u32 actionIndex = 0; actionIndex < list->actionCount; ++actionIndex)
            {
                PossibleAction* possibleAction = list->actions + actionIndex;
                b32 valid = false;
                if(possibleAction->action == action)
                {
                    if(IsValid(possibleAction->requiredEquippedType))
                    {
                        for(u32 equippedIndex = 0; equippedIndex < equippedCount; ++equippedIndex)
                        {
                            if(AreEqual(equipped[equippedIndex], possibleAction->requiredEquippedType))
                            {
                                valid = true;
                                break;
                            }
                        }
                    }
                    else
                    {
                        valid = true;
                    }
                }
                
                if(valid)
                {
                    result = possibleAction;
                    break;
                }
            }
        }
    }
    
    return result;
}

inline r32 GetMaxDistanceSq(PossibleAction* action, b32 continuing, CombatComponent* combat)
{
    r32 result = 0;
    
    PossibleActionDistance* distance = &action->distance;
    switch(distance->type)
    {
        case ActionDistance_Standard:
        {
            if(continuing)
            {
                result = distance->startDistance * distance->continueCoeff;
            }
            else
            {
                result = distance->startDistance;
            }
        } break;
        
        case ActionDistance_Special:
        {
            switch(distance->propertyIndex)
            {
                case Special_AttackDistance:
                {
                    if(combat)
                    {
                        if(continuing)
                        {
                            result = combat->attackDistance * combat->attackContinueCoeff;
                        }
                        else
                        {
                            result = GetR32(combat->attackDistance);
                        }
                    }
                } break;
            }
        } break;
    }
    
    result = Square(result);
    return result;
}

internal b32 ActionIsPossibleAtDistance(InteractionComponent* interaction, u16 action, u16 oldAction, r32 distanceSq, r32* targetTime,CombatComponent* combat, EntityType* equipped, u32 equippedCount, EntityType usingType = {})
{
    b32 result = false;
    PossibleAction* possible = ActionIsPossible(interaction, action, equipped, equippedCount, usingType);
    if(possible)
    {
        b32 continuing = (action == oldAction);
        r32 maxDistanceSq = GetMaxDistanceSq(possible, continuing, combat);
        if(distanceSq <= maxDistanceSq)
        {
            result = true;
            *targetTime = possible->time;
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
    EntityID usingIDServer;
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
    b32 predictionValid;
    
    i32 currentHotIndex;
    i32 currentActionIndex;
    
    b32 anyValidInteraction;
    u32 hotCount;
    EntityHotInteraction hotInteractions[8];
    EntityHotInteraction lastFrameHotInteraction;
    
    Vec2 equipmentPositions[Count_equipmentSlot];
    Rect2 equipmentOnScreen[Count_equipmentSlot];
    
    
    u32 selectedSkillIndex;
    
    GameCommand standardCommand;
    
    LockedInteractionType lockedInteractionType;
    GameCommand lockedCommand;
    b32 keyboardInteractionDisabled;
    
    
    CommandParameters commandParameters;
    
};