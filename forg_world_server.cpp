internal EntityID AddEntity_(ServerState* server, UniversePos P, AssetID definitionID, u32 seed, PlayerComponent* player = 0)
{
    Assert(IsValid(definitionID));
    EntityDefinition* definition = GetData(server->assets, EntityDefinition, definitionID);
    EntityID result = {};
    ServerEntityInitParams params = definition->server;
    params.P = P;
    definition->common.definitionID = EntityReference(definitionID);
    params.seed = seed;
    
    u8 archetype = SafeTruncateToU8(ConvertEnumerator(EntityArchetype, definition->archetype));
    AcquireArchetype(server, archetype, (&result));
    InitEntity(server, result, &definition->common, &params, 0); 
    if(HasComponent(result, PlayerComponent))
    {
        SetComponent(server, result, PlayerComponent, player);
    }
    return result;
}

internal EntityID AddEntity(ServerState* server, UniversePos P, RandomSequence* seq, GameProperties* definitionProperties, PlayerComponent* player = 0)
{
    AssetID definitionID = QueryDataFiles(server->assets, EntityDefinition, "default", seq, definitionProperties);
    
    u32 seed = GetNextUInt32(seq);
    EntityID result = AddEntity_(server, P, definitionID, seed, player);
    return result;
}

internal EntityID AddEntity(ServerState* server, UniversePos P, RandomSequence* seq, EntityRef type, PlayerComponent* player = 0)
{
    AssetID definitionID;
    definitionID.type = AssetType_EntityDefinition;
    definitionID.subtypeHashIndex = type.subtypeHashIndex;
    definitionID.index = type.index;
    
    u32 seed = GetNextUInt32(seq);
    EntityID result = AddEntity_(server, P, definitionID, seed, player);
    return result;
}

#define SpawnPlayer(server, player, P) SpawnPlayer_(server, player, P, false)
#define RespawnPlayer(server, player, P) SpawnPlayer_(server, player, P, true)
internal void SpawnPlayer_(ServerState* server, PlayerComponent* player, UniversePos P, b32 deleteEntities)
{
    EntityRef type = EntityReference(server->assets, "default", "human");
    EntityID ID = AddEntity(server, P, &server->entropy, type, player);
    
    ResetQueue(player->queues + GuaranteedDelivery_None);
    QueueGameAccessConfirm(player, server->worldSeed, ID, deleteEntities);
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

internal b32 Find(EntityID* IDs, u32 idCount, EntityID ID)
{
    b32 result = false;
    for(u32 idIndex = 0; idIndex < idCount; ++idIndex)
    {
        if(AreEqual(IDs[idIndex], ID))
        {
            result = true;
            break;
        }
    }
    
    return result;
}

internal b32 FindPlace(EntityID* IDs, u32 idCount, EntityID ID)
{
    b32 result = false;
    for(u32 idIndex = 0; idIndex < idCount; ++idIndex)
    {
        if(!IsValidID(IDs[idIndex]))
        {
            IDs[idIndex] = ID;
            result = true;
            break;
        }
    }
    
    return result;
}

internal b32 Store(ServerState* server, ContainerComponent* container, EntityID ID)
{
    b32 result = false;
    if(container)
    {
        if(FindPlace(container->usingIDs, container->maxUsingCount, ID))
        {
            result = true;
        }
        else
        {
            if(FindPlace(container->storedIDs, container->maxStoredCount, ID))
            {
                result = true;
            }
        }
    }
    
    return result;
}

internal b32 Use(ServerState* server, EntityID ID, EntityID targetID)
{
    b32 result = false;
    UsingComponent* equipped = GetComponent(server, ID, UsingComponent);
    if(equipped)
    {
        if(FindPlace(equipped->IDs, ArrayCount(equipped->IDs), targetID))
        {
            MakeIntangible(server, targetID);
            result = true;
        }
    }
    
    if(!result)
    {
        EquipmentComponent* equipment = GetComponent(server, ID, EquipmentComponent);
        if(equipment)
        {
            for(u32 equipIndex = 0; equipIndex < ArrayCount(equipment->IDs); ++equipIndex)
            {
                EntityID equipmentID = equipment->IDs[equipIndex];
                if(IsValidID(equipmentID))
                {
                    ContainerComponent* container = GetComponent(server, equipmentID, ContainerComponent);
                    if(container && FindPlace(container->usingIDs, container->maxUsingCount, targetID))
                    {
                        MakeIntangible(server, targetID);
                        result = true;
                        break;
                    }
                }
            }
        }
    }
    
    return result;
}

internal b32 Remove(ServerState* server, ContainerComponent* container, EntityID ID)
{
    b32 result = false;
    if(container)
    {
        for(u32 objectIndex = 0; objectIndex < container->maxStoredCount; ++objectIndex)
        {
            if(AreEqual(container->storedIDs[objectIndex], ID))
            {
                container->storedIDs[objectIndex] = {};
                result = true;
                break;
            }
        }
        
        if(!result)
        {
            for(u32 objectIndex = 0; objectIndex < container->maxUsingCount; ++objectIndex)
            {
                if(AreEqual(container->usingIDs[objectIndex], ID))
                {
                    container->usingIDs[objectIndex] = {};
                    result = true;
                    break;
                }
            }
            
        }
    }
    return result;
}

internal b32 RemoveFromEntity(ServerState* server, EntityID ID, EntityID targetID)
{
    b32 result = false;
    EquipmentComponent* equipment = GetComponent(server, ID, EquipmentComponent);
    if(equipment)
    {
        for(u32 slotIndex = 0; slotIndex < ArrayCount(equipment->IDs); ++slotIndex)
        {
            EntityID equipmentID = equipment->IDs[slotIndex];
            if(AreEqual(equipmentID, targetID))
            {
                result = true;
                equipment->IDs[slotIndex] = {};
                break;
            }
            else
            {
                if(Remove(server, GetComponent(server, equipmentID, ContainerComponent), targetID))
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
            for(u32 usingIndex = 0; usingIndex < ArrayCount(equipped->IDs); ++usingIndex)
            {
                EntityID usingID = equipped->IDs[usingIndex];
                Assert(!HasComponent(usingID, ContainerComponent));
                if(AreEqual(equipped->IDs[usingIndex], targetID))
                {
                    result = true;
                    equipped->IDs[usingIndex] = {};
                    break;
                }
            }
        }
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
        if(Remove(server, sourceContainer, command->targetID))
        {
            removed = true;
        }
    }
    
    return removed;
}

internal void DispatchCommand(ServerState* server, EntityID ID, GameCommand* command)
{
    switch(command->action)
    {
        case none:
        case idle:
        case move:
        {
            if(HasComponent(ID, PhysicComponent))
            {
                PhysicComponent* physic = GetComponent(server, ID, PhysicComponent);
                physic->acc = command->acceleration;
            }
        } break;
        
        case pick:
        {
            if(HasComponent(ID, PhysicComponent))
            {
                PhysicComponent* physic = GetComponent(server, ID, PhysicComponent);
                physic->acc = {};
                
                EntityID targetID = command->targetID;
                if(IsValidID(targetID) && HasComponent(targetID, PhysicComponent))
                {
                    PhysicComponent* targetPhysic = GetComponent(server, targetID, PhysicComponent);
                    
                    if(!IsSet(targetPhysic->flags, EntityFlag_notInWorld) && 
                       targetPhysic->P.chunkZ == physic->P.chunkZ)
                    {
                        if(CanUse(server->assets, targetPhysic->definitionID))
                        {
                            if(!Use(server, ID, targetID))
                            {
                                EquipmentComponent* equipment = GetComponent(server, ID, EquipmentComponent);
                                if(equipment)
                                {
                                    for(u32 equipmentIndex = 0; equipmentIndex < ArrayCount(equipment->IDs); equipmentIndex++)
                                    {
                                        EntityID equipmentID = equipment->IDs[equipmentIndex];
                                        
                                        ContainerComponent* container = GetComponent(server, equipmentID, ContainerComponent);
                                        if(Store(server, container, targetID))
                                        {
                                            MakeIntangible(server, targetID);
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                        else if(CanEquip(server->assets, targetPhysic->definitionID))
                        {
                            EquipmentComponent* equipment = GetComponent(server, ID, EquipmentComponent);
                            if(equipment)
                            {
                                if(FindPlace(equipment->IDs, ArrayCount(equipment->IDs), targetID))
                                {
                                    MakeIntangible(server, targetID);
                                }
                            }
                        }
                    }
                }
            }
        } break;
        
        case use:
        {
            EntityID targetID = command->targetID;
            PhysicComponent* targetPhysic = GetComponent(server, targetID, PhysicComponent);
            
            if(IsValidID(command->containerID))
            {
                ContainerComponent* container = GetComponent(server, command->containerID, ContainerComponent);
                if(Remove(server, container, targetID))
                {
                    Use(server, ID, targetID);
                }
            }
            else
            {
                EquipmentComponent* equipment = GetComponent(server, ID, EquipmentComponent);
                UsingComponent* equipped = GetComponent(server, ID, UsingComponent);
                if(equipment && equipped)
                {
                    for(u32 equipIndex = 0; equipIndex < ArrayCount(equipment->IDs); ++equipIndex)
                    {
                        if(AreEqual(equipment->IDs[equipIndex], targetID))
                        {
                            if(CanUse(server->assets, targetPhysic->definitionID))
                            {
                                if(Use(server, ID, targetID))
                                {
                                    equipment->IDs[equipIndex] = {};
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        } break;
        
        case disequip:
        {
            EntityID targetID = command->targetID;
            EquipmentComponent* equipment = GetComponent(server, ID, EquipmentComponent);
            UsingComponent* equipped = GetComponent(server, ID, UsingComponent);
            
            if(equipment && equipped)
            {
                for(u32 equipIndex = 0; equipIndex < ArrayCount(equipment->IDs); ++equipIndex)
                {
                    if(AreEqual(equipment->IDs[equipIndex], targetID))
                    {
                        MakeTangible(server, targetID);
                        equipment->IDs[equipIndex] = {};
                        break;
                    }
                }
            }
        } break;
        
        case open:
        {
            EntityID targetID = command->targetID;
            
            b32 canOpen = true;
            EquipmentComponent* equipment = GetComponent(server, ID, EquipmentComponent);
            if(equipment && Find(equipment->IDs, ArrayCount(equipment->IDs), targetID))
            {
                canOpen = false;
            }
            UsingComponent* equipped = GetComponent(server, ID, UsingComponent);
            if(equipped && Find(equipped->IDs, ArrayCount(equipped->IDs), targetID))
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
            EntityID targetID = command->targetID;
            if(IsValidID(targetID))
            {
                if(RemoveAccordingToCommand(server, ID, command))
                {
                    MakeTangible(server, targetID);
                }
            }
        } break;
        
        case storeInventory:
        {
            EntityID targetID = command->targetID;
            if(IsValidID(targetID))
            {
                if(RemoveAccordingToCommand(server, ID, command))
                {
                    ContainerComponent* destContainer = GetComponent(server, command->targetContainerID, ContainerComponent);
                    if(destContainer &&
                       (command->targetObjectIndex < destContainer->maxStoredCount) &&!IsValidID(destContainer->storedIDs[command->targetObjectIndex]))
                    {
                        destContainer->storedIDs[command->targetObjectIndex] = targetID;
                    }
                    else
                    {
                        MakeTangible(server, targetID);
                    }
                }
            }
        } break;
        
        
        case useInventory:
        {
            EntityID targetID = command->targetID;
            PhysicComponent* targetPhysic = GetComponent(server, targetID, PhysicComponent);
            
            if(CanUse(server->assets, targetPhysic->definitionID))
            {
                if(IsValidID(targetID))
                {
                    if(RemoveAccordingToCommand(server, ID, command))
                    {
                        b32 added = false;
                        if(AreEqual(command->targetContainerID, ID))
                        {
                            UsingComponent* equipped = GetComponent(server, ID, UsingComponent);
                            if(equipped &&
                               (command->targetObjectIndex < ArrayCount(equipped->IDs)) &&!IsValidID(equipped->IDs[command->targetObjectIndex]))
                            {
                                equipped->IDs[command->targetObjectIndex] = targetID;
                                added = true;
                            }
                        }
                        else
                        {
                            ContainerComponent* destContainer = GetComponent(server, command->targetContainerID, ContainerComponent);
                            if(destContainer &&
                               (command->targetObjectIndex < destContainer->maxUsingCount) &&!IsValidID(destContainer->usingIDs[command->targetObjectIndex]))
                            {
                                destContainer->usingIDs[command->targetObjectIndex] = targetID;
                                added = true;
                            }
                        }
                        
                        
                        if(!added)
                        {
                            MakeTangible(server, targetID);
                        }
                    }
                }
            }
        } break;
        
        InvalidDefaultCase;
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
        DispatchCommand(server, ID, &player->requestCommand);
        if(player->inventoryCommandValid)
        {
            DispatchCommand(server, ID, &player->inventoryCommand);
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

internal void UpdateObjectPositions(ServerState* server, UniversePos P, EntityID* IDs, u32 IDCount)
{
    for(u16 IDIndex = 0; IDIndex < IDCount; ++IDIndex)
    {
        EntityID ID = IDs[IDIndex];
        if(IsValidID(ID))
        {
            PhysicComponent* equipmentPhysic = GetComponent(server, ID, PhysicComponent);
            equipmentPhysic->P = P;
            equipmentPhysic->flags |= EntityFlag_notInWorld;
        }
    }
}

STANDARD_ECS_JOB_SERVER(UpdateEntity)
{
    PhysicComponent* physic = GetComponent(server, ID, PhysicComponent);
    HandleEntityMovement(server, physic, ID, elapsedTime);
    
    physic->action = {};
    if(LengthSq(physic->speed) > 0.1f)
    {
        physic->action = GameProp(action, move);
    }
    else
    {
        physic->action = GameProp(action, idle);
    }
    
    UsingComponent* equipped = GetComponent(server, ID, UsingComponent);
    if(equipped)
    {
        UpdateObjectPositions(server, physic->P, equipped->IDs, Count_usingSlot);
        for(u32 usingIndex = 0; usingIndex < Count_usingSlot; ++usingIndex)
        {
            EntityID usingID = equipped->IDs[usingIndex];
            if(IsValidID(usingID))
            {
                Assert(!HasComponent(usingID, ContainerComponent));
            }
        }
    }
    
    
    EquipmentComponent* equipment = GetComponent(server, ID, EquipmentComponent);
    if(equipment)
    {
        UpdateObjectPositions(server, physic->P, equipment->IDs, Count_equipmentSlot);
        for(u32 equipIndex = 0; equipIndex < Count_equipmentSlot; ++equipIndex)
        {
            EntityID equipID = equipment->IDs[equipIndex];
            ContainerComponent* container = GetComponent(server, equipID, ContainerComponent);
            if(container)
            {
                UpdateObjectPositions(server, physic->P, container->storedIDs, container->maxStoredCount);
                UpdateObjectPositions(server, physic->P, container->usingIDs, container->maxUsingCount);
            }
        }
    }
}

STANDARD_ECS_JOB_SERVER(DeleteAllEntities)
{
    FreeArchetype(server, &ID);
}

