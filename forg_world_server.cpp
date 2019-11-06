internal EntityID AddEntitySync(NewEntity* newEntity)
{
}

internal void SpawnNewEntities(ServerState* server)
{
}

internal void AddEntity_(ServerState* server, UniversePos P, AssetID definitionID, u32 seed, u32 playerIndex = 0)
{
    NewEntity* newEntity;
    FREELIST_ALLOC(newEntity, server->firstFreeNewEntity, PushStruct(&server->gamePool, NewEntity));
    newEntity->P = P;
    newEntity->definitionID = definitionID;
    newEntity->seed = seed;
    newEntity->playerIndex = playerIndex;
    
    FREELIST_INSERT(newEntity, server->firstNewEntity);
}

internal void AddEntity(ServerState* server, UniversePos P, RandomSequence* seq, GameProperties* definitionProperties, u32 playerIndex = 0)
{
    AssetID definitionID = QueryDataFiles(server->assets, EntityDefinition, "default", seq, definitionProperties);
    
    u32 seed = GetNextUInt32(seq);
    AddEntity_(server, P, definitionID, seed, playerIndex);
}

internal void AddEntity(ServerState* server, UniversePos P, RandomSequence* seq, EntityRef type, u32 playerIndex = 0)
{
    AssetID definitionID;
    definitionID.type = AssetType_EntityDefinition;
    definitionID.subtypeHashIndex = type.subtypeHashIndex;
    definitionID.index = type.index;
    
    u32 seed = GetNextUInt32(seq);
    AddEntity_(server, P, definitionID, seed, playerIndex);
}

internal void SpawnPlayer(ServerState* server, u32 playerIndex, UniversePos P)
{
    EntityRef type = EntityReference(server->assets, "default", "human");
    AddEntity(server, P, &server->entropy, type, playerIndex);
}

internal void MakeIntangible(ServerState* server, EntityID ID)
{
    PhysicComponent* physic = GetComponent(server, ID, PhysicComponent);
    physic->flags = AddFlags(physic->flags, EntityFlag_notInWorld);
}

internal void MakeTangible(ServerState* server, EntityID ID)
{
    PhysicComponent* physic = GetComponent(server, ID, PhysicComponent);
    physic->flags = ClearFlags(physic->flags, EntityFlag_notInWorld);
}

internal b32 Find(InventorySlot* slots, u32 slotCount, EntityID ID)
{
    b32 result = false;
    for(u32 slotIndex = 0; slotIndex < slotCount; ++slotIndex)
    {
        InventorySlot* slot = slots + slotIndex;
        if(AreEqual(slot->ID, ID))
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
            EntityID currentID = slots[slot].ID;
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
        slots[slot].ID = targetID;
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
            
            if(AreEqual(slot->ID, ID))
            {
                slot->ID = {};
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
                
                if(AreEqual(slot->ID, ID))
                {
                    slot->ID = {};
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
    
    PhysicComponent* physic = GetComponent(server, ID, PhysicComponent);
    if(AreEqual(targetID, physic->draggingID))
    {
        result = true;
        MakeTangible(server, physic->draggingID);
        physic->draggingID = {};
    }
    else
    {
        EquipmentComponent* equipment = GetComponent(server, ID, EquipmentComponent);
        if(equipment)
        {
            for(u32 slotIndex = 0; slotIndex < ArrayCount(equipment->slots); ++slotIndex)
            {
                InventorySlot* slot = equipment->slots + slotIndex;
                if(AreEqual(slot->ID, targetID))
                {
                    result = true;
                    slot->ID = {};
                    break;
                }
                else
                {
                    if(RemoveFromContainer(server, GetComponent(server, slot->ID, ContainerComponent), targetID))
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
                for(u32 usingIndex = 0; usingIndex < ArrayCount(equipped->slots); ++usingIndex)
                {
                    InventorySlot* slot = equipped->slots + usingIndex;
                    Assert(!HasComponent(slot->ID, ContainerComponent));
                    if(AreEqual(slot->ID, targetID))
                    {
                        result = true;
                        slot->ID = {};
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
    
    if(!IsValidID(slot->ID) &&
       interaction && CompatibleSlot(interaction, slot))
    {
        slot->ID = ID;
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

internal void DispatchCommand(ServerState* server, EntityID ID, GameCommand* command, CommandParameters* parameters, r32 elapsedTime, b32 updateAction)
{
    Assert(HasComponent(ID, PhysicComponent));
    PhysicComponent* physic = GetComponent(server, ID, PhysicComponent);
    GameProperty newAction = GameProp(action, command->action);
    
    switch(command->action)
    {
        case none:
        case idle:
        case move:
        {
            physic->acc = parameters->acceleration;
            
#if 0            
            physic->action = {};
            if(LengthSq(physic->speed) > 0.1f)
            {
                physic->action = GameProp(action, move);
            }
            else
            {
                physic->action = GameProp(action, idle);
            }
#endif
        } break;
        
        case cast:
        {
            physic->acc = {};
            SkillComponent* skills = GetComponent(server, ID, SkillComponent);
            if(skills)
            {
                if(command->skillIndex < ArrayCount(skills->activeSkills))
                {
                    Skill* active = skills->activeSkills + command->skillIndex;
                    if(physic->actionTime >= active->targetTime)
                    {
                        physic->actionTime = 0;
                        UniversePos targetP = Offset(physic->P, parameters->targetOffset);
                        DispatchGameEffect(server, ID, targetP, &active->effect, command->targetID);
                    }
                }
            }
        } break;
        
        case pick:
        {
            physic->acc = {};
            EntityID targetID = command->targetID;
            if(IsValidID(targetID) && HasComponent(targetID, PhysicComponent))
            {
                PhysicComponent* targetPhysic = GetComponent(server, targetID, PhysicComponent);
                if(!IsSet(targetPhysic->flags, EntityFlag_notInWorld) && 
                   targetPhysic->P.chunkZ == physic->P.chunkZ)
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
                                    EntityID equipID = equipment->slots[equipIndex].ID;
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
            }
        } break;
        
        case drag:
        {
            if(true)
            {
                if(!IsValidID(physic->draggingID))
                {
                    MakeIntangible(server, command->targetID);
                    physic->draggingID = command->targetID;
                }
                else
                {
                    newAction = GameProp(action, idle);
                }
            }
            else
            {
                newAction = GameProp(action, idle);
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
        
        case open:
        {
            EntityID targetID = command->targetID;
            
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
                        container->openedBy = ID;
                    }
                }
            }
        } break;
        
        case drop:
        {
            RemoveAccordingToCommand(server, ID, command);
        } break;
        
        case storeInventory:
        {
            EntityID targetID = command->targetID;
            if(RemoveAccordingToCommand(server, ID, command))
            {
                Store(server, command->targetContainerID, targetID, command->targetObjectIndex);
            }
        } break;
        
        case useInventory:
        {
            EntityID targetID = command->targetID;
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
    
    if(updateAction)
    {
        if(AreEqual(newAction, physic->action))
        {
            physic->actionTime += elapsedTime;
        }
        else
        {
            physic->actionTime = 0;
            physic->action = newAction;
        }
    }
}

STANDARD_ECS_JOB_SERVER(HandleOpenedContainers)
{
    ContainerComponent* container = GetComponent(server, ID, ContainerComponent);
    if(IsValidID(container->openedBy))
    {
        PhysicComponent* opened = GetComponent(server, ID, PhysicComponent);
        PhysicComponent* opener = GetComponent(server, container->openedBy, PhysicComponent);
        
        UniversePos P1 = opened->P;
        UniversePos P2 = opener->P;
        
        if(P1.chunkZ == P2.chunkZ)
        {
            r32 maxDistanceSq = Square(1.0f);
            Vec3 delta = SubtractOnSameZChunk(P1, P2);
            if(LengthSq(delta) > maxDistanceSq)
            {
                container->openedBy = {};
            }
        }
        else
        {
            container->openedBy = {};
        }
    }
}

STANDARD_ECS_JOB_SERVER(HandlePlayerCommands)
{
    PlayerComponent* player = GetComponent(server, ID, PlayerComponent);
    if(player)
    {
        DispatchCommand(server, ID, &player->requestCommand, &player->commandParameters, elapsedTime, true);
        if(player->inventoryCommandValid)
        {
            DispatchCommand(server, ID, &player->inventoryCommand, &player->commandParameters, elapsedTime, false);
            player->inventoryCommandValid = false;
        }
    }
}

STANDARD_ECS_JOB_SERVER(FillPlayerSpacePartition)
{
    PlayerComponent* player = GetComponent(server, ID, PlayerComponent);
    if(player)
    {
        PhysicComponent* physic = GetComponent(server, ID, PhysicComponent);
        r32 maxDistance = 3.0f * CHUNK_DIM;
        Rect3 bounds = AddRadius(physic->bounds, V3(maxDistance, maxDistance, maxDistance));
        AddToSpatialPartition(server->frameByFramePool, &server->playerPartition, physic->P, bounds, ID);
    }
}

global_variable r32 g_maxDelta = 1.0f;
STANDARD_ECS_JOB_SERVER(FillCollisionSpatialPartition)
{
    PhysicComponent* physic = GetComponent(server, ID, PhysicComponent);
    
    if(!(physic->flags & EntityFlag_notInWorld))
    {
        Rect3 bounds = AddRadius(physic->bounds, V3(g_maxDelta, g_maxDelta, g_maxDelta));
        AddToSpatialPartition(server->frameByFramePool, &server->collisionPartition, physic->P, bounds, ID);
    }
}

internal void HandleEntityMovement(ServerState* server, PhysicComponent* physic, EntityID ID, r32 elapsedTime)
{
    UniversePos oldP = physic->P;
    
    r32 accelerationCoeff = 27.0f;
    r32 drag = -7.8f;
    
    Vec3 acceleration = Normalize(physic->acc) *accelerationCoeff;
    Vec3 velocity = physic->speed;
    r32 dt = elapsedTime;
    acceleration.xy += drag * velocity.xy;
    
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
        SpatialPartitionQuery collisionQuery = QuerySpatialPartition(&server->collisionPartition, physic->P, bounds);
        
        EntityID collisionTriggerID = {};
        UniversePos collisionP = {};
        
        for(EntityID testID = GetCurrent(&collisionQuery); IsValid(&collisionQuery); testID = Advance(&collisionQuery))
        {
            PhysicComponent* testPhysic = GetComponent(server, testID, PhysicComponent);
            if(testPhysic->P.chunkZ == physic->P.chunkZ)
            {
                Vec3 testP = SubtractOnSameZChunk(testPhysic->P, physic->P);
                
                b32 overlappingEntity = HasComponent(testID, OverlappingEffectsComponent);
                
                b32 shouldCollide = overlappingEntity ? false : true;
                b32 shouldOverlap = overlappingEntity ? true : false;
                
                r32 oldT = tStop;
                if(HandleVolumeCollision(physic->bounds, deltaP, testP, testPhysic->bounds, &tStop, &wallNormalMin, shouldCollide))
                {
                    if(shouldOverlap)
                    {
                        DispatchOverlappingEffects(server, testPhysic->P, ID, testID);
                    }
                }
                
                
                if(tStop < oldT)
                {
                    collisionP = testPhysic->P;
                    collisionTriggerID = testID;
                }
            }
        }
        
        if(IsValidID(collisionTriggerID))
        {
            DispatchCollisitonEffects(server, collisionP, ID, collisionTriggerID);
        }
        
        Vec3 wallNormal = wallNormalMin;
        physic->P.chunkOffset += tStop * deltaP;
        
        physic->speed -= Dot(physic->speed, wallNormal) * wallNormal;
        deltaP -= Dot(deltaP, wallNormal) * wallNormal;
        tRemaining -= tStop;
    }
    
    physic->P = NormalizePosition(physic->P);
    if(!PositionInsideWorld(&physic->P))
    {
        physic->P = oldP;
    }
}

internal void UpdateObjectPositions(ServerState* server, UniversePos P, InventorySlot* slots, u32 slotCount)
{
    for(u16 slotIndex = 0; slotIndex < slotCount; ++slotIndex)
    {
        InventorySlot* slot = slots + slotIndex;
        if(IsValidID(slot->ID))
        {
            PhysicComponent* equipmentPhysic = GetComponent(server, slot->ID, PhysicComponent);
            equipmentPhysic->P = P;
            equipmentPhysic->flags |= EntityFlag_notInWorld;
        }
    }
}

STANDARD_ECS_JOB_SERVER(UpdateEntity)
{
    PhysicComponent* physic = GetComponent(server, ID, PhysicComponent);
    HandleEntityMovement(server, physic, ID, elapsedTime);
    
    UsingComponent* equipped = GetComponent(server, ID, UsingComponent);
    if(equipped)
    {
        UpdateObjectPositions(server, physic->P, equipped->slots, Count_usingSlot);
        for(u32 usingIndex = 0; usingIndex < Count_usingSlot; ++usingIndex)
        {
            InventorySlot* slot = equipped->slots + usingIndex;
            if(IsValidID(slot->ID))
            {
                Assert(!HasComponent(slot->ID, ContainerComponent));
            }
        }
    }
    
    
    EquipmentComponent* equipment = GetComponent(server, ID, EquipmentComponent);
    if(equipment)
    {
        UpdateObjectPositions(server, physic->P, equipment->slots, Count_equipmentSlot);
        for(u32 equipIndex = 0; equipIndex < Count_equipmentSlot; ++equipIndex)
        {
            InventorySlot* slot = equipment->slots + equipIndex;
            ContainerComponent* container = GetComponent(server, slot->ID, ContainerComponent);
            if(container)
            {
                UpdateObjectPositions(server, physic->P, container->storedObjects, ArrayCount(container->storedObjects));
                UpdateObjectPositions(server, physic->P, container->usingObjects, ArrayCount(container->usingObjects));
            }
        }
    }
}

STANDARD_ECS_JOB_SERVER(DeleteAllEntities)
{
    FreeArchetype(server, &ID);
}

