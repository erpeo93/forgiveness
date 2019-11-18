internal Vec2 HandleKeyboardInteraction(GameUIContext* UI, ClientPlayer* player, PlatformInput* input)
{
    GameCommand* command = &UI->standardCommand;
    CommandParameters* parameters = &UI->commandParameters;
    
    command->action = idle;
    parameters->acceleration = {};
    command->targetID = {};
    
    Vec2 cameraOffset = {};
    
    r32 cameraOffsetMagnitudo = 0.6f;
    if(IsDown(&input->moveLeft))
    {
        parameters->acceleration.x = -1.0f;
        cameraOffset.x = -cameraOffsetMagnitudo;
    }
    if(IsDown(&input->moveRight))
    {
        parameters->acceleration.x = 1.0f;
        cameraOffset.x = cameraOffsetMagnitudo;
    }
    if(IsDown(&input->moveDown))
    {
        parameters->acceleration.y = -1.0f;
        cameraOffset.y = -cameraOffsetMagnitudo;
    }
    if(IsDown(&input->moveUp))
    {
        parameters->acceleration.y = 1.0f;
        cameraOffset.y = cameraOffsetMagnitudo;
    }
    
    if(LengthSq(parameters->acceleration) > 0)
    {
        command->action = move;
    }
    
    return cameraOffset;
}

internal void HandleMouseInteraction(GameUIContext* UI, GameModeWorld* worldMode, ClientPlayer* player, PlatformInput* input)
{
    if(Pressed(&input->mouseCenter))
    {
        if(IsValidID(UI->lootingIDServer))
        {
            if(UI->lootingMode)
            {
                UI->lootingMode = false;
                UI->inventoryMode = true;
            }
            else
            {
                UI->lootingMode = true;
                UI->inventoryMode = false;
            }
        }
        else
        {
            UI->inventoryMode = !UI->inventoryMode;
        }
    }
}

internal GameCommand ComputeFinalCommand(GameUIContext* UI, GameModeWorld* worldMode, ClientPlayer* myPlayer)
{
    BaseComponent* player = GetComponent(worldMode, myPlayer->clientID, BaseComponent);
    
    GameCommand result = UI->standardCommand;
    
    switch(UI->lockedInteractionType)
    {
        case LockedInteraction_None:
        {
        } break;
        
        case LockedInteraction_SkillTarget:
        case LockedInteraction_SkillOffset:
        {
            result = UI->lockedCommand;
        } break;
        
        case LockedInteraction_ReachTarget:
        {
            GameCommand lockedCommand = UI->lockedCommand;
            result = lockedCommand;
            
            EntityID lockedIDClient = GetClientIDMapping(worldMode, lockedCommand.targetID);
            
            InteractionComponent* interaction = GetComponent(worldMode, lockedIDClient, InteractionComponent);
            
            BaseComponent* lockedBase = GetComponent(worldMode, lockedIDClient, BaseComponent);
            
            Vec3 toTarget = SubtractOnSameZChunk(lockedBase->universeP, player->universeP);
            u16 action = lockedCommand.action;
            r32 distanceSq = LengthSq(toTarget);
            
            b32 usingValid = IsValidID(lockedCommand.usingID);
            BaseComponent* usingBase = GetComponent(worldMode, GetClientIDMapping(worldMode, lockedCommand.usingID), BaseComponent);
            EntityRef usingRef = {};
            if(usingBase)
            {
                usingRef = usingBase->definitionID;
            }
            
            r32 targetTime;
            if(!ActionIsPossible(interaction, action, distanceSq, &targetTime, usingValid, usingRef))
            {
                GameCommand command = {};
                command.action = move;
                result = command;
            }
        } break;
    }
    
    return result;
}

internal void ResetInteractions(GameUIContext* UI)
{
    UI->hotCount = 0;
    UI->anyValidInteraction = false;
}

internal b32 LeftMouseAction(u16 action)
{
    b32 result = true;
    if(action == drag)
    {
        result = false;
    }
    
    return result;
}

internal EntityHotInteraction* AddPossibleInteraction_(GameModeWorld* worldMode, GameUIContext* UI, InteractionType type, PossibleActionList* list, EntityID entityID, EntityID containerID = {}, u16 objectIndex = 0, InventorySlot* slot = 0, u16 optionIndex = 0)
{
    EntityHotInteraction* result = 0;
    
    if(UI->hotCount < ArrayCount(UI->hotInteractions))
    {
        EntityHotInteraction* dest = UI->hotInteractions;
        if(UI->hotCount == 1 && !UI->anyValidInteraction)
        {
        }
        else
        {
            dest = UI->hotInteractions + UI->hotCount++;
        }
        
        dest->type = type;
        dest->actionCount = 0;
        dest->actions[0] = 0;
        dest->usingIDServer = {};
        
        if(list)
        {
            for(u32 actionIndex = 0; actionIndex < list->actionCount; ++actionIndex)
            {
                if(dest->actionCount < ArrayCount(dest->actions))
                {
                    PossibleAction* action = list->actions + actionIndex;
                    
                    b32 valid = true;
                    if(type == Interaction_Ground && IsValidID(UI->draggingIDServer))
                    {
                        EntityID draggingIDClient = GetClientIDMapping(worldMode, UI->draggingIDServer);
                        
                        BaseComponent* draggingBase = GetComponent(worldMode, draggingIDClient, BaseComponent);
                        valid = false;
                        if(draggingBase && 
                           AreEqual(action->requiredUsingType, draggingBase->definitionID))
                        {
                            valid = true;
                        }
                    }
                    
                    if(valid && LeftMouseAction(action->action))
                    {
                        dest->actions[dest->actionCount++] = action->action;
                    }
                }
            }
        }
        dest->containerIDServer = containerID;
        dest->objectIndex = objectIndex;
        dest->slot = slot;
        dest->entityIDServer = entityID;
        dest->optionIndex = optionIndex;
        
        result = dest;
    }
    
    return result;
}


internal EntityHotInteraction* AddPossibleInteraction(GameModeWorld* worldMode, GameUIContext* UI, InteractionType type, PossibleActionList* list, EntityID entityID, EntityID containerID = {}, u16 objectIndex = 0, InventorySlot* slot = 0, u16 optionIndex = 0)
{
    EntityHotInteraction* result = AddPossibleInteraction_(worldMode, UI, type, list, entityID, containerID, objectIndex, slot, optionIndex);
    UI->anyValidInteraction = true;
    return result;
}

internal void AddContainerObjectsInteractions(GameUIContext* UI, GameModeWorld* worldMode, ObjectMapping* mappings, u16  mappingCount, EntityID containerID, InteractionType interactionType)
{
	for(u16 mappingIndex = 0; mappingIndex < mappingCount; ++mappingIndex)
	{
        ObjectMapping* mapping = mappings + mappingIndex;
        EntityID ID = mapping->object.ID;
        if(IsValidInventoryMapping(UI, mapping))
        {
            InteractionComponent* interaction = GetComponent(worldMode, ID, InteractionComponent);
            BaseComponent* base = GetComponent(worldMode, ID, BaseComponent);
            if(interaction)
            {
                if(PointInRect(mapping->projOnScreen, worldMode->relativeMouseP))
                {
                    mapping->hot = true;
                    ResetInteractions(UI);
                    AddPossibleInteraction(worldMode, UI, interactionType, interaction->actions + interactionType, ID, containerID, mappingIndex, &mapping->object);
                }
                else if((interactionType == Interaction_Equipment) && PointInRect(base->projectedOnScreen, worldMode->relativeMouseP))
                {
                    mapping->hot = true;
                    AddPossibleInteraction(worldMode, UI, interactionType, interaction->actions + interactionType, ID, containerID, mappingIndex, &mapping->object);
                }
            }
        }
        else
        {
            if(PointInRect(mapping->projOnScreen, worldMode->relativeMouseP))
            {
                if(IsValidID(UI->draggingIDServer))
                {
                    InteractionComponent* interaction = GetComponent(worldMode, GetClientIDMapping(worldMode, UI->draggingIDServer), InteractionComponent);
                    
                    if(CompatibleSlot(interaction, &mapping->object))
                    {
                        mapping->hot = true;
                        ResetInteractions(UI);
                        AddPossibleInteraction(worldMode, UI, interactionType, 0, {}, containerID, mappingIndex, &mapping->object);
                    }
                }
            }
        }
    }
}

internal void HandleEquipmentInteraction(GameUIContext* UI, GameModeWorld* worldMode, EntityID ID)
{
    EquipmentMappingComponent* equipment = GetComponent(worldMode, ID, EquipmentMappingComponent);
    if(equipment)
    {
        AddContainerObjectsInteractions(UI, worldMode, equipment->mappings, ArrayCount(equipment->mappings), ID, Interaction_Equipment);
    }
    
    if(!IsValidID(UI->draggingIDServer))
    {
        UsingMappingComponent* equipped = GetComponent(worldMode, ID, UsingMappingComponent);
        if(equipped)
        {
            AddContainerObjectsInteractions(UI, worldMode, equipped->mappings, ArrayCount(equipped->mappings), ID, Interaction_Equipment);
        }
    }
}

internal void HandleContainerInteraction(GameUIContext* UI, GameModeWorld* worldMode)
{
    if(IsValidID(UI->openIDRight))
    {
        ContainerMappingComponent* container = GetComponent(worldMode, UI->openIDRight, ContainerMappingComponent);
        if(container)
        {
			AddContainerObjectsInteractions(UI, worldMode, container->storedMappings, ArrayCount(container->storedMappings), UI->openIDRight, Interaction_Container);
            
            AddContainerObjectsInteractions(UI, worldMode, container->usingMappings, ArrayCount(container->usingMappings), UI->openIDRight, Interaction_Equipped);
        }
    }
    
    if(IsValidID(UI->openIDLeft))
    {
        ContainerMappingComponent* container = GetComponent(worldMode, UI->openIDLeft, ContainerMappingComponent);
        if(container)
        {
            AddContainerObjectsInteractions(UI, worldMode, container->storedMappings, ArrayCount(container->storedMappings), UI->openIDLeft, Interaction_Container);
            
            AddContainerObjectsInteractions(UI, worldMode, container->usingMappings, ArrayCount(container->usingMappings), UI->openIDLeft, Interaction_Equipped);
        }
    }
}

internal void HandleOverlayObjectsInteraction(GameUIContext* UI, GameModeWorld* worldMode, b32 onlyUsingSlots)
{
    
    UsingMappingComponent* equipped = GetComponent(worldMode, worldMode->player.clientID, UsingMappingComponent);
    if(equipped)
    {
        AddContainerObjectsInteractions(UI, worldMode, equipped->mappings, ArrayCount(equipped->mappings), worldMode->player.serverID, Interaction_Equipped);
    }
    
    if(!onlyUsingSlots)
    {
        for(u32 equipmentIndex = 0; equipmentIndex < ArrayCount(UI->equipmentOnScreen); ++equipmentIndex)
        {
            if(PointInRect(UI->equipmentOnScreen[equipmentIndex], worldMode->relativeMouseP))
            {
                ResetInteractions(UI);
                AddPossibleInteraction(worldMode, UI, Interaction_MoveContainerOnScreen, 0, {}, {}, SafeTruncateToU16(equipmentIndex), 0);
            }
        }
        
        EquipmentMappingComponent* equipment = GetComponent(worldMode, worldMode->player.clientID, EquipmentMappingComponent);
        if(equipment)
        {
            for(u32 equipIndex = 0; equipIndex < ArrayCount(equipment->mappings); ++equipIndex)
            {
                EntityID ID = equipment->mappings[equipIndex].object.ID;
                ContainerMappingComponent* container = GetComponent(worldMode, ID, ContainerMappingComponent);
                if(container)
                {
                    AddContainerObjectsInteractions(UI, worldMode, container->usingMappings, ArrayCount(container->usingMappings), ID, Interaction_Equipped);
                }
            }
        }
    }
}

internal void OverdrawLayout(GameModeWorld* worldMode, RenderGroup* group, EntityID ID, Rect2 rect, LayoutContainerDrawMode drawMode, r32 zBias = 2.0f)
{
    BaseComponent* base = GetComponent(worldMode, ID, BaseComponent);
    Vec4 color = GetTint(worldMode, ID);
    
    LayoutComponent* layout = GetComponent(worldMode, ID, LayoutComponent);
    LayoutContainer container = {};
    container.container = GetComponent(worldMode, ID, ContainerMappingComponent);
    container.drawMode = drawMode;
    
    ObjectTransform transform = FlatTransform(zBias);
    transform.angle = layout->rootAngle;
    transform.scale = layout->rootScale;
    transform.modulationPercentage = GetModulationPercentage(worldMode, ID); 
    RenderLayoutInRect(worldMode, group, rect, transform, color, layout, base->seed, {}, &container);
}

internal void OverdrawVisibleStuff(GameUIContext* UI, GameModeWorld* worldMode, RenderGroup* group, b32 onlyUsingSlots)
{
    Vec2 minP = -0.5f * group->screenDim + V2(30, 10);
    Vec2 dim = V2(100, 100);
    r32 margin = 20.0f;
    
    ObjectTransform overdrawTransform = FlatTransform(2.0f);
    
    UsingMappingComponent* usingMappings = GetComponent(worldMode, worldMode->player.clientID, UsingMappingComponent);
    if(usingMappings)
    {
        for(u16 slotIndex = 0; slotIndex < ArrayCount(usingMappings->mappings); ++slotIndex)
        {
            ObjectMapping* mapping = usingMappings->mappings + slotIndex;
            EntityID ID = mapping->object.ID;
            
            Rect2 rect = RectMinDim(minP, dim);
            mapping->projOnScreen = rect;
            
            b32 hot = PointInRect(rect, worldMode->relativeMouseP);
            if(hot)
            {
                PushRectOutline(group, overdrawTransform, rect, V4(0, 0, 0, 0.4f), 2.0f);
            }
            if(IsValidUsingMapping(UI, mapping, slotIndex))
            {
                BaseComponent* usingBase = GetComponent(worldMode, ID, BaseComponent);
                OverdrawLayout(worldMode, group, ID, rect, LayoutContainerDraw_Standard);
            }
            
            minP.x += dim.x + margin;
        }
    }
    
    if(!onlyUsingSlots)
    {
        EquipmentMappingComponent* equipment = GetComponent(worldMode, worldMode->player.clientID, EquipmentMappingComponent);
        if(equipment)
        {
            for(u16 slotIndex = 0; slotIndex < ArrayCount(equipment->mappings); ++slotIndex)
            {
                ObjectMapping* mapping = equipment->mappings + slotIndex;
                if(IsValidEquipmentMapping(UI, mapping, slotIndex))
                {
                    EntityID mappingID = mapping->object.ID;
                    ContainerMappingComponent* container = GetComponent(worldMode, mappingID, ContainerMappingComponent);
                    Vec2 equipmentP = UI->equipmentPositions[slotIndex];
                    if(!equipmentP.x && !equipmentP.y)
                    {
                        UI->equipmentPositions[slotIndex].y = 100.0f;
                        UI->equipmentPositions[slotIndex].x = 100 + 200.0f * slotIndex;
                        UI->equipmentPositions[slotIndex] -= 0.5f * group->screenDim;
                        
                        equipmentP = UI->equipmentPositions[slotIndex];
                    }
                    Vec2 equipmentDim = container ? container->desiredUsingDim : V2(200, 200);
                    Rect2 rect = RectMinDim(equipmentP, equipmentDim);
                    
                    b32 hot = PointInRect(rect, worldMode->relativeMouseP);
                    if(hot)
                    {
                        PushRectOutline(group, overdrawTransform, rect, V4(0, 0, 0, 0.4f), 2.0f);
                    }
                    
                    UI->equipmentOnScreen[slotIndex] = rect;
                    OverdrawLayout(worldMode, group, mappingID, rect, LayoutContainerDraw_Using);
                }
                minP.x += dim.x + margin;
            }
        }
    }
}

internal void OverdrawSkillSlots(GameUIContext* UI, GameModeWorld* worldMode, RenderGroup* group)
{
    Vec2 P = V2(100, -400);
    Vec2 skillDim = V2(80, 80);
    r32 margin = 10;
    
    for(u32 skillIndex = 0; skillIndex < MAX_ACTIVE_SKILLS; ++skillIndex)
    {
        UI->skillRects[skillIndex] = InvertedInfinityRect2();
    }
    
    SkillMappingComponent* skills = GetComponent(worldMode, worldMode->player.clientID, SkillMappingComponent);
    if(skills)
    {
        for(u32 skillIndex = 0; skillIndex < MAX_ACTIVE_SKILLS; ++skillIndex)
        {
            SkillMapping* skill = skills->mappings + skillIndex;
            Vec4 color = skill->color;
            if(skillIndex != UI->hotSkillIndex && skillIndex != UI->selectedSkillIndex)
            {
                color.a *= 0.7f;
            }
            
            Rect2 skillRect = RectMinDim(P, skillDim);
            UI->skillRects[skillIndex] = skillRect;
            PushRect(group, FlatTransform(2.0f), skillRect, color);
            
            if(skillIndex == UI->selectedSkillIndex)
            {
                PushRectOutline(group, FlatTransform(2.0f), skillRect, V4(0, 0, 0, 1), 2.0f);
            }
            
            P.x += (margin + skillDim.x);
        }
    }
}

internal void RenderUIOverlay(GameModeWorld* worldMode, RenderGroup* group)
{
    GameUIContext* UI = &worldMode->gameUI;
    
    switch(UI->lockedInteractionType)
    {
        case LockedInteraction_SkillOffset:
        {
            PushRect(group, FlatTransform(0.01f), UI->commandParameters.targetOffset, V2(0.2f, 0.2f), V4(1, 0, 0, 1));
        } break;
    }
    
    Vec2 mouseP = worldMode->relativeMouseP;
    if(UI->lootingMode)
    {
        OverdrawVisibleStuff(UI, worldMode, group, false);
    }
    else if(UI->inventoryMode)
    {
        Vec2 rightP = V2(400, 0);
        Vec2 leftP = V2(-400, 0);
        Vec2 defaultDim = V2(400, 400);
        if(IsValidID(UI->openIDRight))
        {
            ContainerMappingComponent* container = GetComponent(worldMode, UI->openIDRight, ContainerMappingComponent);
            Vec2 dim = container ? container->desiredOpenedDim : defaultDim;
            Rect2 rect = RectCenterDim(rightP, dim);
            OverdrawLayout(worldMode, group, UI->openIDRight, rect, LayoutContainerDraw_Open);
        }
        
        if(IsValidID(UI->openIDLeft))
        {
            ContainerMappingComponent* container = GetComponent(worldMode, UI->openIDLeft, ContainerMappingComponent);
            Vec2 dim = container ? container->desiredOpenedDim : defaultDim;
            Rect2 rect = RectCenterDim(leftP, dim);
            OverdrawLayout(worldMode, group, UI->openIDLeft, rect, 
                           LayoutContainerDraw_Open);
        }
        
        OverdrawVisibleStuff(UI, worldMode, group, true);
    }
    else
    {
        OverdrawVisibleStuff(UI, worldMode, group, false);
        OverdrawSkillSlots(UI, worldMode, group);
    }
    
    EntityID draggingID = GetClientIDMapping(worldMode, UI->draggingIDServer);
    if(IsValidID(draggingID))
    {
        if(UI->testingDraggingOnEquipment)
		{
			UI->testingDraggingOnEquipment = false;
			UI->draggingTestUsingOption = 0;
			UI->draggingTestEquipOption = 0;
		}
		else
		{
			Rect2 dragRect = RectCenterDim(worldMode->relativeMouseP, V2(100, 100));
			OverdrawLayout(worldMode, group, draggingID, dragRect, LayoutContainerDraw_Standard, 10.0f);
		}
    }
    
    FontId fontID = QueryFonts(group->assets, "game", 0, 0);
    if(IsValid(fontID))
    {
        Vec3 tooltipP = V3(mouseP, 0);
        Vec4 color = UI->multipleActions ? V4(1, 0, 0, 1) : V4(1, 1, 1, 1);
        PushText(group, fontID, UI->tooltip, tooltipP, 0.7f, color, false, true, 10.0f);
    }
}

INTERACTION_ECS_JOB_CLIENT(HandleEntityInteraction)
{
    GameUIContext* UI = &worldMode->gameUI;
    
    InteractionComponent* interaction = GetComponent(worldMode, ID, InteractionComponent);
    interaction->isOnFocus = false;
    
    BaseComponent* base = GetComponent(worldMode, ID, BaseComponent);
    base->worldBounds = GetEntityBound(worldMode, base);
    
    if(ShouldBeRendered(worldMode, base))
    {
        r32 cameraZ;
        base->projectedOnScreen = ProjectOnScreen(group, base->worldBounds, &cameraZ);
        
        if(!AreEqual(ID, worldMode->player.clientID) && !AreEqual(ID, GetClientIDMapping(worldMode, worldMode->gameUI.lootingIDServer)))
        {
            if(PointInRect(base->projectedOnScreen, worldMode->relativeMouseP))
            {
				if(IsValidID(UI->draggingIDServer))
				{
					PossibleActionList* list = interaction->actions + InteractionList_Dragging;
					EntityHotInteraction* addedInteraction = AddPossibleInteraction(worldMode, UI, Interaction_Ground, interaction->actions + InteractionList_Dragging, ID);
					addedInteraction->usingIDServer = UI->draggingIDServer;
				}
				else
				{
					AddPossibleInteraction(worldMode, UI, Interaction_Ground, interaction->actions + InteractionList_Ground, ID);
				}
            }
        }
    }
}

internal b32 MouseInsidePlayerRectProjectedOnScreen(GameModeWorld* worldMode)
{
    BaseComponent* playerBase = GetComponent(worldMode, worldMode->player.clientID, BaseComponent);
    b32 result = (PointInRect(playerBase->projectedOnScreen, worldMode->relativeMouseP));
    return result;
}

internal void SetTooltip(GameUIContext* UI, u32 actionCount, char* tooltip)
{
    UI->multipleActions = (actionCount > 1);
    FormatString(UI->tooltip, sizeof(UI->tooltip), "%s", tooltip);
}

internal void AddObjectRemovedPrediction(GameModeWorld* worldMode, GameUIContext* UI, EntityID containerID, EntityID ID)
{
    UI->ignoreDraggingMappingTimer = 0.2f;
    if(IsValidID(containerID))
    {
        Assert(IsValidID(ID));
        UI->predictionTime = 0.2f;
        UI->predictionContainerID = GetClientIDMapping(worldMode, containerID);
        UI->predictionObjectID = GetClientIDMapping(worldMode, ID);
    }
}

internal b32 UsingEquipOptionApplicable(UsingEquipOption* option, ObjectMapping* mappings, u32 mappingCount, EntityID targetID)
{
    b32 result = false;
    
    if(option->slots[0] != 0xffff)
    {
        result = true;
        
        for(u32 slotIndex = 0; slotIndex < ArrayCount(option->slots); ++slotIndex)
        {
            u16 slot = option->slots[slotIndex];
            
            if(slot == 0xffff)
            {
                break;
            }
            
            Assert(slot < mappingCount);
            EntityID currentID = mappings[slot].object.ID;
            if(!AreEqual(currentID, targetID) && IsValidID(currentID))
            {
                result = false;
                break;
            }
        }
    }
    
    return result;
}

internal UsingEquipOption* ExistValidUseOptionThatContains(GameModeWorld* worldMode, EntityID ID, EntityID targetID, u16 slot, u16* optionIndexPtr)
{
    UsingEquipOption* result = 0;
    
    UsingMappingComponent* usingComponent = GetComponent(worldMode, GetClientIDMapping(worldMode, ID), UsingMappingComponent);
    InteractionComponent* interaction = GetComponent(worldMode, GetClientIDMapping(worldMode, targetID), InteractionComponent);
    if(usingComponent && interaction)
    {
        for(u16 optionIndex = 0; optionIndex < interaction->usingConfigurationCount; ++optionIndex)
        {
            UsingEquipOption* option = interaction->usingConfigurations + optionIndex;
            b32 valid = false;
            for(u32 slotIndex = 0; slotIndex < ArrayCount(option->slots); ++slotIndex)
            {
                if(option->slots[slotIndex] == slot)
                {
                    valid = true;
                    break;
                }
            }
            
            if(valid)
            {
                if(UsingEquipOptionApplicable(option, 
                                              usingComponent->mappings, ArrayCount(usingComponent->mappings), 
                                              targetID))
                {
                    result = option;
                    *optionIndexPtr = optionIndex;
                    break;
                }
            }
        }
    }
    
    return result;
}


internal UsingEquipOption* FindNearestCompatibleOption(GameModeWorld* worldMode, ObjectMapping* mappings, u32 mappingCount, UsingEquipOption* options, u32 optionCount, u16* optionIndexPtr, EntityID targetID, b32 useBaseComponentDistance)
{
    UsingEquipOption* result = 0;
    
    r32 minDistanceFromOptionSlotSq = R32_MAX;
    for(u16 optionIndex = 0; optionIndex < optionCount; ++optionIndex)
    {
        UsingEquipOption* option = options + optionIndex;
        if(UsingEquipOptionApplicable(option, 
                                      mappings, mappingCount, 
                                      targetID))
        {
            r32 nearestOptionDistance = R32_MAX;
            for(u32 slotIndex = 0; slotIndex < ArrayCount(option->slots); ++slotIndex)
            {
                u16 slot = option->slots[slotIndex];
                if(slot == 0xffff)
                {
                    break;
                }
                
                ObjectMapping* mapping = mappings + slot;
                
                r32 distanceSq = R32_MAX;
                if(false && useBaseComponentDistance)
                {
                    if(IsValidID(mapping->object.ID))
                    {
                        BaseComponent* base = GetComponent(worldMode, GetClientIDMapping(worldMode, mapping->object.ID), BaseComponent);
                        
                        distanceSq = LengthSq(GetCenter(base->projectedOnScreen) - worldMode->relativeMouseP);
                    }
                }
                else
                {
                    distanceSq = mapping->distanceFromMouseSq;
                }
                
                if(distanceSq < nearestOptionDistance)
                {
                    nearestOptionDistance = distanceSq;
                }
            }
            if(nearestOptionDistance < minDistanceFromOptionSlotSq)
            {
                minDistanceFromOptionSlotSq = nearestOptionDistance;
                result = option;
                *optionIndexPtr = optionIndex;
            }
        }
    }
    
    return result;
}

internal UsingEquipOption* CanUse(GameModeWorld* worldMode, EntityID ID, EntityID targetID, u16* optionIndexPtr)
{
    r32 minDistanceFromOptionSlotSq = R32_MAX;
    UsingEquipOption* result = 0;
    
    UsingMappingComponent* usingComponent = GetComponent(worldMode, ID, UsingMappingComponent);
    InteractionComponent* interaction = GetComponent(worldMode, targetID, InteractionComponent);
    if(usingComponent && interaction)
    {
        result = FindNearestCompatibleOption(worldMode, usingComponent->mappings, ArrayCount(usingComponent->mappings),
                                             interaction->usingConfigurations, interaction->usingConfigurationCount, optionIndexPtr, targetID, false);
    }
    
    return result;
}

internal UsingEquipOption* CanEquip(GameModeWorld* worldMode, EntityID ID, EntityID targetID, u16* optionIndexPtr)
{
    r32 minDistanceFromOptionSlotSq = R32_MAX;
    UsingEquipOption* result = 0;
    
    EquipmentMappingComponent* equipComponent = GetComponent(worldMode, ID, EquipmentMappingComponent);
    InteractionComponent* interaction = GetComponent(worldMode, targetID, InteractionComponent);
    if(equipComponent && interaction)
    {
        result = FindNearestCompatibleOption(worldMode, equipComponent->mappings, ArrayCount(equipComponent->mappings),
                                             interaction->equipConfigurations, interaction->equipConfigurationCount, optionIndexPtr, targetID, true);
    }
    
    return result;
}


internal void FakeUse(GameUIContext* UI, GameModeWorld* worldMode, EntityID ID, EntityID targetIDServer, u16 optionIndex)
{
    EntityID targetID = GetClientIDMapping(worldMode, targetIDServer);
    
    InteractionComponent* interaction = GetComponent(worldMode, targetID, InteractionComponent);
    Assert(optionIndex < interaction->usingConfigurationCount);
    UsingEquipOption* option = interaction->usingConfigurations + optionIndex;
    Assert(option);
    UsingMappingComponent* equipped = GetComponent(worldMode, ID, UsingMappingComponent);
    UI->draggingTestUsingOption = option;
    
    if(interaction && equipped)
    {
        for(u32 slotIndex = 0; slotIndex < ArrayCount(option->slots); ++slotIndex)
        {
            u16 slot = option->slots[slotIndex];
            if(slot == 0xffff)
            {
                break;
            }
            StoreObjectMapping(worldMode, equipped->mappings, ArrayCount(equipped->mappings), slot, 0, targetIDServer, StringHash(MetaTable_usingSlot[slot]));
        }
    }
}


internal void FakeEquip(GameUIContext* UI, GameModeWorld* worldMode, EntityID ID, EntityID targetIDServer, u16 optionIndex)
{
    EntityID targetID = GetClientIDMapping(worldMode, targetIDServer);
    InteractionComponent* interaction = GetComponent(worldMode, targetID, InteractionComponent);
    Assert(optionIndex < interaction->equipConfigurationCount);
    UsingEquipOption* option = interaction->equipConfigurations + optionIndex;
    Assert(option);
    UI->draggingTestEquipOption = option;
    
    EquipmentMappingComponent* equipment = GetComponent(worldMode, ID, EquipmentMappingComponent);
    if(interaction && equipment)
    {
        for(u32 slotIndex = 0; slotIndex < ArrayCount(option->slots); ++slotIndex)
        {
            u16 slot = option->slots[slotIndex];
            if(slot == 0xffff)
            {
                break;
            }
            StoreObjectMapping(worldMode, equipment->mappings, ArrayCount(equipment->mappings), slot, 0, targetIDServer, StringHash(MetaTable_equipmentSlot[slot]));
        }
    }
}

internal void HandleSkillsInteraction(GameUIContext* UI, GameModeWorld* worldMode)
{
    for(u32 skillIndex = 0; skillIndex < MAX_ACTIVE_SKILLS; ++skillIndex)
    {
        if(PointInRect(UI->skillRects[skillIndex], worldMode->relativeMouseP))
        {
            ResetInteractions(UI);
            EntityHotInteraction* interaction = AddPossibleInteraction(worldMode, UI, Interaction_SkillSelection, 0, {}, {}, 0, 0, 0);
            interaction->skillIndex = SafeTruncateToU16(skillIndex);
        }
    }
}

internal void HandleUIInteraction(GameModeWorld* worldMode, RenderGroup* group, ClientPlayer* myPlayer, PlatformInput* input)
{
    GameUIContext* UI = &worldMode->gameUI;
    
    if(!UI->initialized)
    {
        UI->initialized = true;
        UI->hotSkillIndex = 0xffff;
    }
    BaseComponent* player = GetComponent(worldMode, myPlayer->clientID, BaseComponent);
    player->flags = ClearFlags(player->flags, EntityFlag_occluding);
    ResetInteractions(UI);
    UI->tooltip[0] = 0;
    
    if(UI->ignoreDraggingMappingTimer > 0)
    {
        UI->ignoreDraggingMappingTimer -= input->timeToAdvance;
    }
    
    AddPossibleInteraction_(worldMode, &worldMode->gameUI, Interaction_Ground, 0, {});
    
    Vec2 cameraOffset = {};
    if(!UI->keyboardInteractionDisabled)
    {
        cameraOffset = HandleKeyboardInteraction(UI, myPlayer, input);
    }
    HandleMouseInteraction(UI, worldMode, myPlayer, input);
    
    EXECUTE_INTERACTION_JOB(worldMode, group, input, HandleEntityInteraction, ArchetypeHas(BaseComponent) && ArchetypeHas(InteractionComponent), input->timeToAdvance);
    
    MoveCameraTowards(worldMode, player, 0.5f, cameraOffset, V2(0, 0), worldMode->defaultZoomCoeff);
    
    
    b32 standardInteractionAllowed = true;
    
    GameCommand lockedCommand = UI->lockedCommand;
    EntityID lockedIDClient = GetClientIDMapping(worldMode, lockedCommand.targetID);
    switch(UI->lockedInteractionType)
    {
        case LockedInteraction_SkillTarget:
        {
            InteractionComponent* lockedInteraction = GetComponent(worldMode, lockedIDClient, InteractionComponent);
            if(lockedInteraction)
            {
                lockedInteraction->isOnFocus = true;
            }
            BaseComponent* lockedBase = GetComponent(worldMode, lockedIDClient, BaseComponent);
            if(lockedBase)
            {
                if(Released(&input->mouseRight) || !ShouldBeRendered(worldMode, lockedBase))
                {
                    UI->lockedInteractionType = LockedInteraction_None;
                }
            }
        } break;
        
        case LockedInteraction_SkillOffset:
        {
            CommandParameters* parameters = &UI->commandParameters;
            parameters->targetOffset = worldMode->groundMouseP;
            if(Released(&input->mouseRight))
            {
                UI->lockedInteractionType = LockedInteraction_None;
            }
        } break;
        
        case LockedInteraction_ReachTarget:
        {
            CommandParameters* parameters = &UI->commandParameters;
            BaseComponent* targetBase = GetComponent(worldMode, lockedIDClient, BaseComponent);
            Vec3 toTarget = SubtractOnSameZChunk(targetBase->universeP, player->universeP);
            parameters->acceleration += Normalize(toTarget);
            
            InteractionComponent* lockedInteraction = GetComponent(worldMode, lockedIDClient, InteractionComponent);
            
            lockedInteraction->isOnFocus = true;
            
            if(Released(&input->mouseLeft))
            {
                UI->lockedInteractionType = LockedInteraction_None;
            }
        } break;
        
        case LockedInteraction_Completed:
        {
            CommandParameters* parameters = &UI->commandParameters;
            parameters->acceleration = {};
            standardInteractionAllowed = true;
            
            if(Released(&input->mouseLeft) || Released(&input->mouseRight))
            {
                UI->lockedInteractionType = LockedInteraction_None;
                UI->keyboardInteractionDisabled = false;
            }
        } break;
    }
    
    
    if(standardInteractionAllowed)
    {
        if(UI->lootingMode)
        {
            HandleEquipmentInteraction(UI, worldMode, myPlayer->clientID);
            Assert(IsValidID(UI->lootingIDServer));
            EntityID lootingID = GetClientIDMapping(worldMode, UI->lootingIDServer);
            ContainerMappingComponent* container = GetComponent(worldMode, lootingID, ContainerMappingComponent);
            BaseComponent* lootingBase = GetComponent(worldMode, lootingID, BaseComponent);
            
            Vec3 deltaP = SubtractOnSameZChunk(player->universeP, lootingBase->universeP);
            if(deltaP.y <= 0)
            {
                if(LengthSq(player->velocity) < 0.1f && Abs(deltaP.x) < 0.5f * GetWidth(lootingBase->bounds))
                {
                    player->flags = AddFlags(player->flags, EntityFlag_occluding);
                }
            }
            
            if(container && lootingBase)
            {
                AddContainerObjectsInteractions(UI, worldMode, container->storedMappings, ArrayCount(container->storedMappings), UI->lootingIDServer, Interaction_Container);
                MoveCameraTowards(worldMode, lootingBase, 5.0f, V2(0, 0), V2(0, 0), container->zoomCoeff);
            }
            
            HandleOverlayObjectsInteraction(UI, worldMode, false);
        }
        else if(UI->inventoryMode)
        {
            HandleEquipmentInteraction(UI, worldMode, myPlayer->clientID);
            HandleContainerInteraction(UI, worldMode);
            MoveCameraTowards(worldMode, player, 2.0f, V2(0, 0), V2(0, 0), 2.0f);
            HandleOverlayObjectsInteraction(UI, worldMode, true);
        }
        else
        {
            HandleOverlayObjectsInteraction(UI, worldMode, false);
            HandleSkillsInteraction(UI, worldMode);
        }
        
        EntityID draggingID = GetClientIDMapping(worldMode, UI->draggingIDServer);
        if(IsValidID(draggingID) && MouseInsidePlayerRectProjectedOnScreen(worldMode))
        {
            
            u16 optionIndex;
            
            if(CanUse(worldMode, myPlayer->clientID, draggingID, &optionIndex))
            {
                EntityHotInteraction* interaction = AddPossibleInteraction(worldMode, UI, Interaction_Dragging, 0, {}, {}, 0, 0, optionIndex);
                if(interaction)
                {
                    interaction->actionCount = 1;
                    interaction->actions[0] = use;
                }
            }
            else if(CanEquip(worldMode, myPlayer->clientID, draggingID, &optionIndex))
            {
                EntityHotInteraction* interaction = AddPossibleInteraction(worldMode, UI, Interaction_Dragging, 0, {}, {}, 0, 0, optionIndex);
                if(interaction)
                {
                    interaction->actionCount = 1;
                    interaction->actions[0] = equip;
                    
                }
            }
        }
        
        if(UI->hotCount > 0)
        {
            b32 interactionChanged = true;
            for(u32 hotIndex = 0; hotIndex < UI->hotCount; ++hotIndex)
            {
                EntityHotInteraction hotInteraction = UI->hotInteractions[hotIndex];
                
                if(AreEqual(hotInteraction, UI->lastFrameHotInteraction))
                {
                    UI->currentHotIndex = (i32) hotIndex;
                    interactionChanged = false;
                    break;
                }
            }
            if(interactionChanged)
            {
                UI->currentActionIndex = 0;
            }
            
            
            if(Pressed(&input->switchButton))
            {
                ++UI->currentHotIndex;
            }
            
            UI->currentHotIndex = Wrap(0, UI->currentHotIndex, (i32) UI->hotCount);
            
            EntityHotInteraction hotInteraction = UI->hotInteractions[UI->currentHotIndex];
            
            UI->currentActionIndex += input->mouseWheelOffset;
            UI->currentActionIndex = Wrap(0, UI->currentActionIndex, (i32) hotInteraction.actionCount);
            
            
            UI->lastFrameHotInteraction = hotInteraction;
            EntityID hotID = hotInteraction.entityIDServer;
            u16 hotAction = hotInteraction.actions[UI->currentActionIndex];
            
            b32 validInteraction = true;
            switch(hotInteraction.type)
            {
                case Interaction_Ground:
                {
                    validInteraction = false;
                    if(Pressed(&input->mouseRight) && !IsValidID(UI->draggingIDServer))
                    {
                        b32 draggingInteraction = false;
                        if(IsValidID(hotID))
                        {
                            EntityID hotIDClient = GetClientIDMapping(worldMode, hotID);
                            InteractionComponent* interaction = GetComponent(worldMode, hotIDClient, InteractionComponent);
                            BaseComponent* hotBase = GetComponent(worldMode, hotIDClient, BaseComponent);
                            r32 distanceSq = LengthSq(SubtractOnSameZChunk(hotBase->universeP, player->universeP));
                            
                            r32 targetTime;
                            if(ActionIsPossible(interaction, drag, distanceSq, &targetTime))
                            {
                                GameCommand command = {};
                                command.action = drag;
                                command.targetID = hotID;
                                SendInventoryCommand(command); 
                                draggingInteraction = true;
                                validInteraction = true;
                                
                                UI->draggingIDServer = hotID;
                                UI->draggingContainerIDServer = myPlayer->serverID;
                            }
                        }
                        
                        if(!draggingInteraction)
                        {
                            GameCommand command = {};
                            command.action = cast;
                            command.skillIndex = SafeTruncateToU16(UI->selectedSkillIndex);
                            SkillMappingComponent* skills = GetComponent(worldMode, myPlayer->clientID, SkillMappingComponent);
                            
                            if(skills)
                            {
                                Assert(command.skillIndex < ArrayCount(skills->mappings));
                                SkillMapping* skill = skills->mappings + command.skillIndex;
                                if(skill->targetSkill)
                                {
                                    if(IsValidID(hotID))
                                    {
                                        UI->lockedInteractionType = LockedInteraction_SkillTarget;
                                        command.targetID = hotID;
                                        validInteraction = true;
                                    }
                                    else
                                    {
                                        validInteraction = false;
                                    }
                                }
                                else
                                {
                                    UI->lockedInteractionType = LockedInteraction_SkillOffset;
                                    validInteraction = true;
                                }
                                
                                if(validInteraction)
                                {
                                    UI->lockedCommand = command;
                                }
                            }
                        }
                    }
                    else if(IsValidID(hotID))
                    {
                        if(Pressed(&input->mouseLeft))
                        {
                            EntityID hotIDClient = GetClientIDMapping(worldMode, hotID);
                            InteractionComponent* interaction = GetComponent(worldMode, hotIDClient, InteractionComponent);
                            BaseComponent* hotBase = GetComponent(worldMode, hotIDClient, BaseComponent);
                            r32 distanceSq = LengthSq(SubtractOnSameZChunk(hotBase->universeP, player->universeP));
                            
                            GameCommand command = {};
                            command.targetID = hotID;
                            command.action = hotAction;
                            command.usingID = hotInteraction.usingIDServer;
                            
                            UI->lockedInteractionType = LockedInteraction_ReachTarget;
                            UI->lockedCommand = command;
                        }
                        validInteraction = true;
                    }
                } break;
                
                case Interaction_Container:
                case Interaction_Equipped:
                {
                    if(IsValidID(UI->draggingIDServer))
                    {
                        validInteraction = false;
                        if(IsValidID(hotID))
                        {
                            //switchRequest(hotInteraction.containerID);
                        }
                        else
                        {
                            b32 valid = false;
                            u16 action;
                            u16 optionIndex = 0;
                            
                            if(hotInteraction.type == Interaction_Container)
                            {
                                SetTooltip(UI, 1, "move");
                                validInteraction = true;
                                action = storeInventory;
                            }
                            else
                            {
                                if(AreEqual(hotInteraction.containerIDServer, myPlayer->serverID))
                                {
                                    if(ExistValidUseOptionThatContains(worldMode, myPlayer->serverID, UI->draggingIDServer, hotInteraction.objectIndex, &optionIndex))
                                    {
                                        UI->testingDraggingOnEquipment = true;
                                        InteractionComponent* draggingInteraction = GetComponent(worldMode, GetClientIDMapping(worldMode, UI->draggingIDServer), InteractionComponent);
                                        draggingInteraction->isOnFocus = true;
                                        FakeUse(UI, worldMode, myPlayer->clientID, UI->draggingIDServer, optionIndex);
                                        SetTooltip(UI, 1, "use");
                                        validInteraction = true;
                                    }
                                }
                                else
                                {
                                    SetTooltip(UI, 1, "move");
                                    validInteraction = true;
                                }
                                
                                action = useInventory;
                            }
                            
                            if(validInteraction)
                            {
                                if(Pressed(&input->mouseLeft))
                                {
                                    GameCommand command = {};
                                    command.action = action;
                                    command.containerID = UI->draggingContainerIDServer;
                                    command.targetID = UI->draggingIDServer;
                                    command.targetContainerID = hotInteraction.containerIDServer;
                                    command.targetObjectIndex = hotInteraction.objectIndex;
                                    command.optionIndex = optionIndex;
                                    SendInventoryCommand(command); 
                                    UI->draggingIDServer = {};
                                    AddObjectRemovedPrediction(worldMode, UI, command.containerID, command.targetID);
                                }
                            }
                        }
                    }
                    else
                    {
                        if(IsValidID(hotID))
                        {
                            if(Pressed(&input->mouseLeft))
                            {
                                GameCommand* command = &UI->standardCommand;
                                command->targetID = hotID;
                                command->action = hotAction;
                            }
                            
                            if(Pressed(&input->mouseRight))
                            {
                                UI->draggingIDServer = hotID;
                                UI->draggingContainerIDServer = hotInteraction.containerIDServer;
                                
                                if(AreEqual(UI->draggingIDServer, UI->openIDLeft))
                                {
                                    UI->openIDLeft = {};
                                }
                                
                                if(AreEqual(UI->draggingIDServer, UI->openIDRight))
                                {
                                    UI->openIDRight = {};
                                }
                            }
                        }
                    }
                } break;
                
                case Interaction_Equipment:
                {
                    if(IsValidID(UI->draggingIDServer))
                    {
                        validInteraction = (hotInteraction.actionCount == 1 && hotInteraction.actions[0] == open);
                    }
                    else
                    {
                        if(Pressed(&input->mouseRight))
                        {
                            if(!IsValidID(UI->draggingIDServer))
                            {
                                UI->draggingIDServer = hotID;
                                UI->draggingContainerIDServer = hotInteraction.containerIDServer;
                                
                                if(AreEqual(UI->draggingIDServer, UI->openIDLeft))
                                {
                                    UI->openIDLeft = {};
                                }
                                
                                if(AreEqual(UI->draggingIDServer, UI->openIDRight))
                                {
                                    UI->openIDRight = {};
                                }
                            }
                        }
                    }
                    
                    if(validInteraction && Pressed(&input->mouseLeft))
                    {
                        Assert(IsValidID(hotID));
                        GameCommand command = {};
                        command.action = hotAction;
                        command.targetID = hotID;
                        SendInventoryCommand(command);
                        
                        switch(command.action)
                        {
                            case open:
                            {
                                if(AreEqual(hotID, UI->openIDLeft))
                                {
                                    UI->openIDLeft = {};
                                }
                                else if(AreEqual(hotID, UI->openIDRight))
                                {
                                    UI->openIDRight = {};
                                }
                                else
                                {
                                    if(!IsValidID(UI->openIDLeft))
                                    {
                                        UI->openIDLeft = hotID;
                                    }
                                    else if(!IsValidID(UI->openIDRight))
                                    {
                                        UI->openIDRight = hotID;
                                    }
                                }
                            } break;
                        }
                    }
                    
                } break;
                
                case Interaction_Dragging:
                {
                    UI->testingDraggingOnEquipment = true;
                    SetTooltip(UI, 1, "equip");
                    
                    
                    
                    InteractionComponent* draggingInteraction = GetComponent(worldMode, GetClientIDMapping(worldMode, UI->draggingIDServer), InteractionComponent);
                    draggingInteraction->isOnFocus = true;
                    
                    
                    if(hotInteraction.actions[0] == use)
                    {
                        FakeUse(UI, worldMode, myPlayer->clientID, UI->draggingIDServer, hotInteraction.optionIndex);
                    }
                    else
                    {
                        Assert(hotInteraction.actions[0] == equip);
                        FakeEquip(UI, worldMode, myPlayer->clientID, UI->draggingIDServer, hotInteraction.optionIndex);
                    }
                    
                    if(Pressed(&input->mouseLeft))
                    {
                        GameCommand command = {};
                        command.action = hotInteraction.actions[0];
                        command.targetID = UI->draggingIDServer;
                        command.containerID = UI->draggingContainerIDServer;
                        command.optionIndex = hotInteraction.optionIndex;
                        SendInventoryCommand(command);
                        UI->draggingIDServer = {};
                        
                        AddObjectRemovedPrediction(worldMode, UI, command.containerID, command.targetID);
                    }
                } break;
                
                case Interaction_MoveContainerOnScreen:
                {
                    if(IsDown(&input->mouseRight))
                    {
                        UI->equipmentPositions[hotInteraction.objectIndex] += worldMode->deltaMouseP;
                    }
                } break;
                
                case Interaction_SkillSelection:
                {
                    UI->hotSkillIndex = hotInteraction.skillIndex;
                    if(Pressed(&input->mouseLeft))
                    {
                        UI->selectedSkillIndex = UI->hotSkillIndex;
                    }
                } break;
                
                InvalidDefaultCase;
            }
            
            if(validInteraction)
            {
                InteractionComponent* interaction = GetComponent(worldMode, GetClientIDMapping(worldMode, hotID), InteractionComponent);
                if(interaction)
                {
                    interaction->isOnFocus = true;
                }
                
                if(hotAction)
                {
                    SetTooltip(UI, hotInteraction.actionCount, MetaTable_action[hotAction]);
                }
            }
            else
            {
                if(IsValidID(UI->draggingIDServer))
                {
                    UI->multipleActions = false;
                    SetTooltip(UI, false, "drop");
                    
                    if(Pressed(&input->mouseLeft) || Pressed(&input->mouseRight))
                    {
                        GameCommand command = {};
                        command.action = drop;
                        command.targetID = UI->draggingIDServer;
                        command.containerID = UI->draggingContainerIDServer;
                        SendInventoryCommand(command);
                        UI->draggingIDServer = {};
                        
                        AddObjectRemovedPrediction(worldMode, UI, command.containerID, command.targetID);
                    }
                }
            }
        }
    }
    
    if(UI->predictionTime > 0)
    {
        UI->predictionTime -= input->timeToAdvance;
        if(AreEqual(UI->predictionContainerID, worldMode->player.clientID))
        {
            EquipmentMappingComponent* equipment = GetComponent(worldMode, UI->predictionContainerID, EquipmentMappingComponent);
            if(equipment)
            {
                RemoveObjectMapping(equipment->mappings, ArrayCount(equipment->mappings), UI->predictionObjectID);
            }
            
            UsingMappingComponent* equipped = GetComponent(worldMode, UI->predictionContainerID, UsingMappingComponent);
            if(equipped)
            {
                RemoveObjectMapping(equipped->mappings, ArrayCount(equipped->mappings), UI->predictionObjectID);
            }
        }
        else
        {
            ContainerMappingComponent* container = GetComponent(worldMode, UI->predictionContainerID, ContainerMappingComponent);
            RemoveObjectMapping(container->storedMappings, ArrayCount(container->storedMappings), UI->predictionObjectID);
            RemoveObjectMapping(container->usingMappings, ArrayCount(container->usingMappings), UI->predictionObjectID);
        }
    }
}