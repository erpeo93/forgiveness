internal void AddEntity_(ServerState* server, UniversePos P, AssetID definitionID, u32 seed, AddEntityParams params)
{
    NewEntity* newEntity;
    FREELIST_ALLOC(newEntity, server->firstFreeNewEntity, PushStruct(&server->gamePool, NewEntity));
    newEntity->P = P;
    newEntity->definitionID = definitionID;
    newEntity->seed = seed;
    newEntity->params = params;
    FREELIST_INSERT(newEntity, server->firstNewEntity);
}

internal void AddEntity(ServerState* server, UniversePos P, RandomSequence* seq, GameProperties* definitionProperties, AddEntityParams params = DefaultAddEntityParams())
{
    AssetID definitionID = QueryDataFiles(server->assets, EntityDefinition, "default", seq, definitionProperties);
    
    u32 seed = GetNextUInt32(seq);
    AddEntity_(server, P, definitionID, seed, params);
}

internal void AddEntity(ServerState* server, UniversePos P, RandomSequence* seq, EntityRef type, AddEntityParams params = DefaultAddEntityParams())
{
    AssetID definitionID;
    definitionID.type = AssetType_EntityDefinition;
    definitionID.subtypeHashIndex = type.subtypeHashIndex;
    definitionID.index = type.index;
    
    u32 seed = GetNextUInt32(seq);
    AddEntity_(server, P, definitionID, seed, params);
}

internal void SpawnPlayer(ServerState* server, UniversePos P, AddEntityParams params)
{
    EntityRef type = EntityReference(server->assets, "default", "human");
    AddEntity(server, P, &server->entropy, type, params);
}

internal void SpawnPlayerGhost(ServerState* server, UniversePos P, AddEntityParams params)
{
    EntityRef type = EntityReference(server->assets, "default", "placeholder");
    AddEntity(server, P, &server->entropy, type, params);
}

internal void MakeIntangible(ServerState* server, EntityID ID)
{
    DefaultComponent* def = GetComponent(server, ID, DefaultComponent);
    AddEntityFlags(def, EntityFlag_notInWorld);
}

internal void MakeTangible(ServerState* server, EntityID ID)
{
    DefaultComponent* def = GetComponent(server, ID, DefaultComponent);
    ClearEntityFlags(def, EntityFlag_notInWorld);
}

internal void DeleteEntity(ServerState* server, EntityID ID, DeleteEntityReasonType type)
{
    DefaultComponent* def = GetComponent(server, ID, DefaultComponent);
    Assert(!EntityHasFlags(def, EntityFlag_locked));
    AddEntityFlags(def, EntityFlag_deleted);
    DeletedEntity* deleted;
    FREELIST_ALLOC(deleted, server->firstFreeDeletedEntity, PushStruct(&server->gamePool, DeletedEntity));
    deleted->ID = ID;
    deleted->type = type;
    FREELIST_INSERT(deleted, server->firstDeletedEntity);
}

internal b32 Find(InventorySlot* slots, u32 slotCount, EntityID ID)
{
    b32 result = false;
    for(u32 slotIndex = 0; slotIndex < slotCount; ++slotIndex)
    {
        InventorySlot* slot = slots + slotIndex;
        if(AreEqual(slot, ID))
        {
            result = true;
            break;
        }
    }
    
    return result;
}

internal b32 UsingEquipOptionApplicable(UsingEquipOption* option, InventorySlot* slots, u32 slotCount, EntityID targetID)
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
            
            Assert(slot < slotCount);
            EntityID currentID = GetBoundedID(slots + slot);
            if(!AreEqual(currentID, targetID) && IsValidID(currentID))
            {
                result = false;
                break;
            }
        }
    }
    
    return result;
}

internal void ApplyOption(UsingEquipOption* option, InventorySlot* slots, u32 slotCount, EntityID targetID)
{
    for(u16 slotIndex = 0; slotIndex < ArrayCount(option->slots); ++slotIndex)
    {
        u16 slot = option->slots[slotIndex];
        if(slot == 0xffff)
        {
            break;
        }
        
        Assert(slot < slotCount);
        SetBoundedID(slots + slot, targetID);
    }
}

internal UsingEquipOption* ValidOption(UsingEquipOption* options, u32 optionCount, InventorySlot* slots, u32 slotCount, EntityID targetID)
{
    UsingEquipOption* result = 0;
    for(u32 optionIndex = 0; optionIndex < optionCount; ++optionIndex)
    {
        UsingEquipOption* option = options + optionIndex;
        if(UsingEquipOptionApplicable(option, slots, slotCount, targetID))
        {
            result = option;
            break;
        }
    }
    
    return result;
}

internal UsingEquipOption* CanUse(ServerState* server, EntityID ID, EntityID targetID)
{
    UsingEquipOption* result = 0;
    
    UsingComponent* usingComponent = GetComponent(server, ID, UsingComponent);
    InteractionComponent* interaction = GetComponent(server, targetID, InteractionComponent);
    if(usingComponent && interaction)
    {
        result = ValidOption(interaction->usingConfigurations, interaction->usingConfigurationCount,
                             usingComponent->slots, ArrayCount(usingComponent->slots), targetID);
    }
    
    return result;
}

internal UsingEquipOption* CanEquip(ServerState* server, EntityID ID, EntityID targetID)
{
    UsingEquipOption* result = 0;
    EquipmentComponent* equipComponent = GetComponent(server, ID, EquipmentComponent);
    InteractionComponent* interaction = GetComponent(server, targetID, InteractionComponent);
    
    if(equipComponent && interaction)
    {
        result = ValidOption(interaction->equipConfigurations, interaction->equipConfigurationCount,
                             equipComponent->slots, ArrayCount(equipComponent->slots), targetID);
    }
    
    return result;
}

internal void Use_(ServerState* server, UsingEquipOption* option, EntityID ID, EntityID targetID)
{
    UsingComponent* equipped = GetComponent(server, ID, UsingComponent);
    ApplyOption(option, equipped->slots, ArrayCount(equipped->slots), targetID);
    MakeIntangible(server, targetID);
}


internal void Equip_(ServerState* server, UsingEquipOption* option, EntityID ID, EntityID targetID)
{
    EquipmentComponent* equipment = GetComponent(server, ID, EquipmentComponent);
    ApplyOption(option, equipment->slots, ArrayCount(equipment->slots), targetID);
    MakeIntangible(server, targetID);
}

internal b32 Use(ServerState* server, EntityID ID, EntityID targetID)
{
    b32 result = false;
    
    UsingEquipOption* option = CanUse(server, ID, targetID);
    if(option)
    {
        Use_(server, option, ID, targetID);
        result = true;
    }
    
    return result;
}

internal b32 Equip(ServerState* server, EntityID ID, EntityID targetID)
{
    b32 result = false;
    UsingEquipOption* option = CanEquip(server, ID, targetID);
    if(option)
    {
        Equip_(server, option, ID, targetID);
        result = true;
    }
    
    return result;
}

internal b32 UseOptionIndex(ServerState* server, EntityID ID, EntityID targetID, u16 optionIndex)
{
    b32 result = false;
    
    UsingComponent* usingComponent = GetComponent(server, ID, UsingComponent);
    InteractionComponent* interaction = GetComponent(server, targetID, InteractionComponent);
    
    if(usingComponent && interaction)
    {
        if(optionIndex < interaction->usingConfigurationCount)
        {
            UsingEquipOption* option = interaction->usingConfigurations + optionIndex;
            if(UsingEquipOptionApplicable(option, usingComponent->slots, ArrayCount(usingComponent->slots), targetID))
            {
                Use_(server, option, ID, targetID);
                result = true;
            }
        }
    }
    
    return result;
}

internal b32 EquipOptionIndex(ServerState* server, EntityID ID, EntityID targetID, u16 optionIndex)
{
    b32 result = false;
    
    EquipmentComponent* equipComponent = GetComponent(server, ID, EquipmentComponent);
    InteractionComponent* interaction = GetComponent(server, targetID, InteractionComponent);
    
    if(equipComponent && interaction)
    {
        if(optionIndex < interaction->equipConfigurationCount)
        {
            UsingEquipOption* option = interaction->equipConfigurations + optionIndex;
            if(UsingEquipOptionApplicable(option, equipComponent->slots, ArrayCount(equipComponent->slots), targetID))
            {
                Equip_(server, option, ID, targetID);
                result = true;
            }
        }
    }
    
    return result;
}

internal b32 RemoveFromContainer(ServerState* server, ContainerComponent* container, EntityID ID)
{
    b32 result = false;
    if(container)
    {
        for(u32 objectIndex = 0; objectIndex < ArrayCount(container->storedObjects); ++objectIndex)
        {
            InventorySlot* slot = container->storedObjects + objectIndex;
            if(slot->type == InventorySlot_Invalid)
            {
                break;
            }
            
            if(AreEqual(slot, ID))
            {
                SetBoundedID(slot, {});
                result = true;
                break;
            }
        }
        
        if(!result)
        {
            for(u32 objectIndex = 0; objectIndex < ArrayCount(container->usingObjects); ++objectIndex)
            {
                InventorySlot* slot = container->usingObjects + objectIndex;
                
                if(slot->type == InventorySlot_Invalid)
                {
                    break;
                }
                
                if(AreEqual(slot, ID))
                {
                    SetBoundedID(slot, {});
                    result = true;
                    break;
                }
            }
            
        }
    }
    
    if(result)
    {
        MakeTangible(server, ID);
    }
    return result;
}

internal b32 RemoveFromEntity(ServerState* server, EntityID ID, EntityID targetID)
{
    b32 result = false;
    
    EquipmentComponent* equipment = GetComponent(server, ID, EquipmentComponent);
    if(equipment)
    {
        for(u32 slotIndex = 0; slotIndex < ArrayCount(equipment->slots); ++slotIndex)
        {
            InventorySlot* slot = equipment->slots + slotIndex;
            if(AreEqual(slot, targetID))
            {
                result = true;
                SetBoundedID(slot, {});
                break;
            }
            else
            {
                if(RemoveFromContainer(server, GetComponent(server, GetBoundedID(slot), ContainerComponent), targetID))
                {
                    result= true;
                }
            }
        }
    }
    
    if(!result)
    {
        UsingComponent* equipped = GetComponent(server, ID, UsingComponent);
        if(equipped)
        {
            if(AreEqual(equipped->draggingID, targetID))
            {
                result = true;
                SetBoundedID(&equipped->draggingID, {});
            }
            else
            {
                for(u32 usingIndex = 0; usingIndex < ArrayCount(equipped->slots); ++usingIndex)
                {
                    InventorySlot* slot = equipped->slots + usingIndex;
                    //Assert(!HasComponent(slot->ID, ContainerComponent));
                    if(AreEqual(slot, targetID))
                    {
                        result = true;
                        SetBoundedID(slot, {});
                        break;
                    }
                }
            }
        }
    }
    
    if(result)
    {
        MakeTangible(server, targetID);
    }
    
    return result;
}

internal b32 RemoveAccordingToCommand(ServerState* server, EntityID ID, GameCommand* command)
{
    b32 removed = false;
    if(AreEqual(command->containerID, ID))
    {
        if(RemoveFromEntity(server, ID, command->targetID))
        {
            removed = true;
        }
    }
    else
    {
        ContainerComponent* sourceContainer = GetComponent(server, command->containerID, ContainerComponent);
        if(RemoveFromContainer(server, sourceContainer, command->targetID))
        {
            removed = true;
        }
    }
    
    return removed;
}

internal b32 Store_(ServerState* server, InventorySlot* slots, u32 slotCount, u16 index, EntityID ID)
{
    b32 result = false;
    
    Assert(index < slotCount);
    InventorySlot* slot = slots + index;
    
    InteractionComponent* interaction = GetComponent(server, ID, InteractionComponent);
    
    if(!IsValidID(GetBoundedID(slot)) &&
       interaction && CompatibleSlot(interaction, slot))
    {
        SetBoundedID(slot, ID);
        MakeIntangible(server, ID);
        result = true;
    }
    
    return result;
    
}

internal b32 Store(ServerState* server, EntityID containerID, EntityID ID, u16 objectIndex)
{
    ContainerComponent* container = GetComponent(server, containerID, ContainerComponent);
    b32 result = Store_(server, container->storedObjects, ArrayCount(container->storedObjects), 
                        objectIndex, ID);
    return result;
}

internal b32 StoreInUsingSlots(ServerState* server, EntityID containerID, EntityID ID, u16 objectIndex)
{
    ContainerComponent* container = GetComponent(server, containerID, ContainerComponent);
    b32 result = Store_(server, container->usingObjects, ArrayCount(container->usingObjects), 
                        objectIndex, ID);
    return result;
}

internal b32 StoreInContainer(ServerState* server, EntityID containerID, EntityID ID)
{
    b32 result = false;
    
    ContainerComponent* container = GetComponent(server, containerID, ContainerComponent);
    for(u16 slotIndex = 0; slotIndex < ArrayCount(container->usingObjects); ++slotIndex)
    {
        if(StoreInUsingSlots(server, containerID, ID, slotIndex))
        {
            result = true;
            break;
        }
    }
    
    if(!result)
    {
        for(u16 slotIndex = 0; slotIndex < ArrayCount(container->storedObjects); ++slotIndex)
        {
            if(Store(server, containerID, ID, slotIndex))
            {
                result = true;
                break;
            }
        }
    }
    
    return result;
}

internal void SignalCompletedCommand(ServerState* server, EntityID ID, GameCommand* command)
{
    BrainComponent* brain = GetComponent(server, ID, BrainComponent);
    Assert(brain);
	brain->commandCompleted = true;
	if(brain->type == Brain_Player)
	{
		QueueCompletedCommand(server, ID, command);
	}
}

internal void DispatchCommand(ServerState* server, EntityID ID, GameCommand* command, CommandParameters* parameters, r32 elapsedTime, b32 updateAction)
{
    DefaultComponent* def = GetComponent(server, ID, DefaultComponent);
    PhysicComponent* physic = GetComponent(server, ID, PhysicComponent);
    ActionComponent* action = GetComponent(server, ID, ActionComponent);
    MiscComponent* misc = GetComponent(server, ID, MiscComponent);
    
    EntityID targetID = command->targetID;
    DefaultComponent* targetDef = GetComponent(server, command->targetID, DefaultComponent);
    InteractionComponent* interaction = GetComponent(server, command->targetID, InteractionComponent);
    DefaultComponent* usingDef = GetComponent(server, command->usingID, DefaultComponent);
    
    r32 distanceSq = R32_MAX;
    if(targetDef)
    {
        distanceSq = LengthSq(SubtractOnSameZChunk(targetDef->P, def->P));
    }
    
    u16 oldAction = GetU16(action->action);
    u16 newAction = command->action;
    if(updateAction)
    {
        if(newAction == action->action)
        {
            action->time += elapsedTime;
        }
        else
        {
            action->time = 0;
            SetU16(def, &action->action, newAction);
            
            if(physic)
            {
                physic->acc = V3(0, 0, 0);
            }
        }
    }
    
    b32 resetAction = false;
    b32 resetActionTime = false;
    switch(GetU16(action->action))
    {
        case none:
        case idle:
        case move:
        {
            if(physic)
            {
                physic->acc = parameters->acceleration;
            }
        } break;
        
        case attack:
        {
            r32 targetTime;
            if(ActionIsPossibleAtDistance(interaction, attack, oldAction, distanceSq, &targetTime, misc))
            {
                if(action->time >= targetTime)
                {
                    DamageEntityPhysically(server, targetID, 1);
                    resetActionTime = true;
                }
            }
            else
            {
                //resetAction = true;
            }
        } break;
        
        case cast:
        {
            SkillComponent* skills = GetComponent(server, ID, SkillComponent);
            if(skills)
            {
                if(command->skillIndex < ArrayCount(skills->activeSkills))
                {
                    Skill* active = skills->activeSkills + command->skillIndex;
                    if(action->time >= active->targetTime)
                    {
                        resetActionTime = true;
                        UniversePos targetP = Offset(def->P, parameters->targetOffset);
                        DispatchGameEffect(server, ID, targetP, &active->effect, command->targetID);
                    }
                }
                else
                {
                    resetAction = true;
                }
            }
            else
            {
                resetAction = true;
            }
        } break;
        
        case pick:
        {
            if(IsValidID(targetID))
            {
                r32 targetTime;
                if(ActionIsPossibleAtDistance(interaction, drag, oldAction, distanceSq, &targetTime, misc))
                {
                    if(action->time >= targetTime)
                    {
                        if(!EntityHasFlags(targetDef, EntityFlag_notInWorld) && 
                           targetDef->P.chunkZ == def->P.chunkZ)
                        {
                            if(!Use(server, ID, targetID))
                            {
                                if(!Equip(server, ID, targetID))
                                {
                                    EquipmentComponent* equipment = GetComponent(server, ID, EquipmentComponent);
                                    
                                    if(equipment)
                                    {
                                        for(u32 equipIndex = 0; equipIndex < ArrayCount(equipment->slots); ++equipIndex)
                                        {
                                            EntityID equipID = GetBoundedID(equipment->slots + equipIndex);
                                            if(IsValidID(equipID) && HasComponent(equipID, ContainerComponent))
                                            {
                                                if(StoreInContainer(server, equipID, targetID))
                                                {
                                                    break;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        resetAction = true;
                        SignalCompletedCommand(server, ID, command);
                    }
                }
                else
                {
                    resetAction = true;
                }
            }
            
        } break;
        
        case drag:
        {
            UsingComponent* equipped = GetComponent(server, ID, UsingComponent);
            if(equipped)
            {
                r32 targetTime;
                if(ActionIsPossibleAtDistance(interaction, drag, oldAction, distanceSq, &targetTime, misc))
                {
                    if(action->time >= targetTime)
                    {
                        if(!IsValidID(equipped->draggingID))
                        {
                            MakeIntangible(server, command->targetID);
                            SetBoundedID(&equipped->draggingID, command->targetID);
                        }
                        
                        SignalCompletedCommand(server, ID, command);
                        resetAction = true;
                    }
                }
                else
                {
                    resetAction = true;
                }
            }
        } break;
        
        case setOnFire:
        {
            if(IsValidID(command->usingID))
            {
                r32 targetTime;
                if(ActionIsPossibleAtDistance(interaction, setOnFire, oldAction, distanceSq, &targetTime, misc, true, usingDef->definitionID))
                {
                    if(action->time >= targetTime)
                    {
                        DeleteEntity(server, command->usingID);
                        SignalCompletedCommand(server, ID, command);
                        resetAction = true;
                    }
                }
                else
                {
                    resetAction = true;
                }
            }
        } break;
        
        
        case open:
        {
            r32 targetTime;
            if(ActionIsPossibleAtDistance(interaction, open, oldAction, distanceSq, &targetTime, misc))
            {
                if(action->time >= targetTime)
                {
                    b32 canOpen = true;
                    EquipmentComponent* equipment = GetComponent(server, ID, EquipmentComponent);
                    if(equipment && Find(equipment->slots, ArrayCount(equipment->slots), targetID))
                    {
                        canOpen = false;
                    }
                    UsingComponent* equipped = GetComponent(server, ID, UsingComponent);
                    if(equipped && Find(equipped->slots, ArrayCount(equipped->slots), targetID))
                    {
                        canOpen = false;
                    }
                    
                    if(canOpen)
                    {
                        ContainerComponent* container = GetComponent(server, targetID, ContainerComponent);
                        if(container)
                        {
                            if(!IsValidID(container->openedBy))
                            {
                                SetBoundedID(&container->openedBy, ID);
                                SignalCompletedCommand(server, ID, command);
                            }
                        }
                    }
                    
                    resetAction = true;
                }
            }
            else
            {
                resetAction = true;
            }
        } break;
        
        case use:
        {
            if(RemoveAccordingToCommand(server, ID, command))
            {
                UseOptionIndex(server, ID, command->targetID, command->optionIndex);
            }
        } break;
        
        case equip:
        {
            if(RemoveAccordingToCommand(server, ID, command))
            {
                EquipOptionIndex(server, ID, command->targetID, command->optionIndex);
            }
        } break;
        
        case disequip:
        {
            RemoveFromEntity(server, ID, command->targetID);
        } break;
        
        case drop:
        {
            RemoveAccordingToCommand(server, ID, command);
        } break;
        
        case storeInventory:
        {
            if(RemoveAccordingToCommand(server, ID, command))
            {
                Store(server, command->targetContainerID, targetID, command->targetObjectIndex);
            }
        } break;
        
        case useInventory:
        {
            if(IsValidID(targetID))
            {
                if(RemoveAccordingToCommand(server, ID, command))
                {
                    if(AreEqual(command->targetContainerID, ID))
                    {
                        UseOptionIndex(server, ID, targetID, command->optionIndex);
                    }
                    else
                    {
                        StoreInUsingSlots(server, command->targetContainerID, targetID, command->targetObjectIndex);
                    }
                }
            }
        } break;
        
        InvalidDefaultCase;
    }
    
    if(resetAction)
    {
        action->time = 0;
        SetU16(def, &action->action, idle);
    }
    
    if(resetActionTime)
    {
        action->time = 0;
    }
}

STANDARD_ECS_JOB_SERVER(HandleOpenedContainers)
{
    ContainerComponent* container = GetComponent(server, ID, ContainerComponent);
    if(IsValidID(container->openedBy))
    {
        InteractionComponent* interaction = GetComponent(server, ID, InteractionComponent);
        
        DefaultComponent* opened = GetComponent(server, ID, DefaultComponent);
        DefaultComponent* opener = GetComponent(server, GetBoundedID(container->openedBy), DefaultComponent);
        r32 distanceSq = LengthSq(SubtractOnSameZChunk(opened->P, opener->P));
        r32 targetTime;
        
        MiscComponent* misc = 0;
        if(!ActionIsPossibleAtDistance(interaction, open, open, distanceSq, &targetTime, misc))
        {
            SetBoundedID(&container->openedBy, {});
        }
    }
}

STANDARD_ECS_JOB_SERVER(FillPlayerSpacePartition)
{
    DefaultComponent* def = GetComponent(server, ID, DefaultComponent);
    PlayerComponent* player = GetComponent(server, ID, PlayerComponent);
    if(player)
    {
        PhysicComponent* physic = GetComponent(server, ID, PhysicComponent);
        Rect3 bounds = AddRadius(physic->bounds, UPDATE_DISTANCE * V3(1, 1, 1));
        AddToSpatialPartition(server->frameByFramePool, &server->playerPartition, def->P, bounds, ID);
    }
}

global_variable r32 g_maxDelta = 1.0f;
STANDARD_ECS_JOB_SERVER(FillCollisionSpatialPartition)
{
    DefaultComponent* def = GetComponent(server, ID, DefaultComponent);
    PhysicComponent* physic = GetComponent(server, ID, PhysicComponent);
    
    if(!EntityHasFlags(def, EntityFlag_notInWorld))
    {
        Rect3 bounds = AddRadius(physic->bounds, V3(g_maxDelta, g_maxDelta, g_maxDelta));
        AddToSpatialPartition(server->frameByFramePool, &server->standardPartition, def->P, bounds, ID);
    }
}

internal void HandleEntityMovement(ServerState* server, DefaultComponent* def, PhysicComponent* physic, EntityID ID, r32 elapsedTime)
{
    UniversePos oldP = def->P;
    Vec3 acceleration = Normalize(physic->acc) *physic->accelerationCoeff;
    Vec3 velocity = physic->speed;
    r32 dt = elapsedTime;
    acceleration.xy += physic->drag * velocity.xy;
    Vec3 deltaP = 0.5f * acceleration * Square(dt) + velocity * dt;
    
    Assert(Abs(deltaP.x) <= g_maxDelta);
    Assert(Abs(deltaP.y) <= g_maxDelta);
    Assert(Abs(deltaP.z) <= g_maxDelta);
    
    physic->speed += acceleration * dt;
    
    r32 tRemaining = 1.0f;
    for( u32 iteration = 0; (iteration < 2) && tRemaining > 0; iteration++)
    {
        Vec3 wallNormalMin = {};
        r32 tStop = tRemaining;
        
        Rect3 bounds = AddRadius(physic->bounds, deltaP);
        SpatialPartitionQuery collisionQuery = QuerySpatialPartition(&server->standardPartition, def->P, bounds);
        
        EntityID collisionTriggerID = {};
        UniversePos collisionP = {};
        
        for(EntityID testID = GetCurrent(&collisionQuery); IsValid(&collisionQuery); testID = Advance(&collisionQuery))
        {
            DefaultComponent* testDef = GetComponent(server, testID, DefaultComponent);
            PhysicComponent* testPhysic = GetComponent(server, testID, PhysicComponent);
            if(testDef->P.chunkZ == def->P.chunkZ)
            {
                Vec3 testP = SubtractOnSameZChunk(testDef->P, def->P);
                
                b32 overlappingEntity = HasComponent(testID, OverlappingEffectsComponent);
                
                b32 shouldCollide = overlappingEntity ? false : true;
                b32 shouldOverlap = overlappingEntity ? true : false;
                
                r32 oldT = tStop;
                if(HandleVolumeCollision(physic->bounds, deltaP, testP, testPhysic->bounds, &tStop, &wallNormalMin, shouldCollide))
                {
                    if(shouldOverlap)
                    {
                        DispatchOverlappingEffects(server, testDef->P, ID, testID);
                    }
                }
                
                
                if(tStop < oldT)
                {
                    collisionP = testDef->P;
                    collisionTriggerID = testID;
                }
            }
        }
        
        if(IsValidID(collisionTriggerID))
        {
            DispatchCollisitonEffects(server, collisionP, ID, collisionTriggerID);
            DispatchCollisitonEffects(server, collisionP, collisionTriggerID, ID);
        }
        
        Vec3 wallNormal = wallNormalMin;
        def->P.chunkOffset += tStop * deltaP;
        
        physic->speed -= Dot(physic->speed, wallNormal) * wallNormal;
        deltaP -= Dot(deltaP, wallNormal) * wallNormal;
        tRemaining -= tStop;
    }
    
    def->P = NormalizePosition(def->P);
    if(!PositionInsideWorld(&def->P))
    {
        def->P = oldP;
    }
    
    def->updateSent = false;
    if(LengthSq(SubtractOnSameZChunk(def->P, oldP)) > 0)
    {
        AddChangedFlags(def, &def->basicPropertiesChanged, EntityBasics_Position);
    }
    
    if(physic->speed != velocity)
    {
        AddChangedFlags(def, &def->basicPropertiesChanged, EntityBasics_Velocity);
    }
}

internal void UpdateObjectPosition(ServerState* server, BoundedEntityID* ID, UniversePos P)
{
    DefaultComponent* equipmentDef = GetComponent(server, GetBoundedID(*ID), DefaultComponent);
    if(EntityHasFlags(equipmentDef, EntityFlag_deleted))
    {
        SetBoundedID(ID, {});
    }
    else
    {
        equipmentDef->P = P;
        AddEntityFlags(equipmentDef, EntityFlag_notInWorld);
    }
}
internal void UpdateObjectPositions(ServerState* server, UniversePos P, InventorySlot* slots, u32 slotCount)
{
    for(u16 slotIndex = 0; slotIndex < slotCount; ++slotIndex)
    {
        InventorySlot* slot = slots + slotIndex;
        if(IsValidID(GetBoundedID(slot)))
        {
            UpdateObjectPosition(server, &slot->ID_, P);
        }
    }
}

STANDARD_ECS_JOB_SERVER(UpdateEntity)
{
    DefaultComponent* def = GetComponent(server, ID, DefaultComponent);
    PhysicComponent* physic = GetComponent(server, ID, PhysicComponent);
    HandleEntityMovement(server, def, physic, ID, elapsedTime);
    
    
    
    
    UsingComponent* equipped = GetComponent(server, ID, UsingComponent);
    if(equipped)
    {
        if(IsValidID(equipped->draggingID))
        {
            UpdateObjectPosition(server, &equipped->draggingID, def->P);
        }
        UpdateObjectPositions(server, def->P, equipped->slots, Count_usingSlot);
        for(u32 usingIndex = 0; usingIndex < Count_usingSlot; ++usingIndex)
        {
            InventorySlot* slot = equipped->slots + usingIndex;
            EntityID slotID = GetBoundedID(slot);
            if(IsValidID(slotID))
            {
                Assert(!HasComponent(slotID, ContainerComponent));
            }
        }
    }
    
    EquipmentComponent* equipment = GetComponent(server, ID, EquipmentComponent);
    if(equipment)
    {
        UpdateObjectPositions(server, def->P, equipment->slots, Count_equipmentSlot);
        for(u32 equipIndex = 0; equipIndex < Count_equipmentSlot; ++equipIndex)
        {
            InventorySlot* slot = equipment->slots + equipIndex;
            EntityID slotID = GetBoundedID(slot);
            if(IsValidID(slotID))
            {
                ContainerComponent* container = GetComponent(server, slotID, ContainerComponent);
                if(container)
                {
                    UpdateObjectPositions(server, def->P, container->storedObjects, ArrayCount(container->storedObjects));
                    UpdateObjectPositions(server, def->P, container->usingObjects, ArrayCount(container->usingObjects));
                }
            }
        }
    }
    
    AliveComponent* alive = GetComponent(server, ID, AliveComponent);
    if(alive)
    {
        if(GetU32(alive->physicalHealth) <= 0 || GetU32(alive->mentalHealth) <= 0)
        {
            DeleteEntity(server, ID);
        }
    }
}

STANDARD_ECS_JOB_SERVER(UpdateTempEntity)
{
    TempEntityComponent* temp = GetComponent(server, ID, TempEntityComponent);
    temp->time += elapsedTime;
    if(temp->time >= temp->targetTime)
    {
        DeleteEntity(server, ID);
    }
}

STANDARD_ECS_JOB_SERVER(DeleteAllEntities)
{
    FreeArchetype(server, ID);
}

internal void DeleteDeletedEntityList(ServerState* server)
{
}
