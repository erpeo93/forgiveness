internal void UpdateSeasonTimer(ServerState* server, r32 elapsedTime)
{
    // NOTE(Leonardo): advance server "status"
    server->seasonTime += elapsedTime;
    if(server->seasonTime >= SEASON_DURATION)
    {
        server->seasonTime = 0;
        server->season = (WorldSeason) ((server->season == Season_Count - 1) ? 0 : server->season + 1);
    }
    server->seasonLerp = Clamp01MapToRange(0.5f * SEASON_DURATION, server->seasonTime, SEASON_DURATION);
}

internal EntityID AddEntity_(ServerState* server, UniversePos P, AssetID definitionID, u32 seed, PlayerComponent* player = 0)
{
    Assert(IsValid(definitionID));
    EntityDefinition* definition = GetData(server->assets, EntityDefinition, definitionID);
    EntityID result = {};
    ServerEntityInitParams params = definition->server;
    params.P = P;
    params.definitionID = EntityReference(definitionID);
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

#define GameProp(property, value) GameProp_(Property_##property, value)
internal GameProperty GameProp_(u16 property, u16 value)
{
    GameProperty result;
    result.property = property;
    result.value = value;
    
    return result;
}

internal EntityRef EntityReference(ServerState* server, char* kind, char* type)
{
    char type_[128];
    FormatString(type_, sizeof(type_), "%s.dat", type);
    EntityRef result;
    result.subtypeHashIndex = GetAssetSubtype(server->assets, AssetType_EntityDefinition, kind);
    result.index = GetAssetIndex(server->assets, AssetType_EntityDefinition, result.subtypeHashIndex, Tokenize(type_));
    return result;
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

internal void DispatchCommand(ServerState* server, EntityID ID, GameCommand* command)
{
    
    switch(command->action)
    {
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
                    
                    EntityRef ref = EntityReference(server, "default", "sword");
                    EntityRef ref2 = EntityReference(server, "default", "bag");
                    
                    if(!IsSet(targetPhysic->flags, EntityFlag_notInWorld) && 
                       targetPhysic->P.chunkZ == physic->P.chunkZ)
                    {
                        if(AreEqual(targetPhysic->definitionID, ref))
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
                        else if(AreEqual(targetPhysic->definitionID, ref2))
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
            EquipmentComponent* equipment = GetComponent(server, ID, EquipmentComponent);
            UsingComponent* equipped = GetComponent(server, ID, UsingComponent);
            if(equipment && equipped)
            {
                for(u32 equipIndex = 0; equipIndex < ArrayCount(equipment->IDs); ++equipIndex)
                {
                    if(AreEqual(equipment->IDs[equipIndex], targetID))
                    {
                        if(Use(server, ID, targetID))
                        {
                            equipment->IDs[equipIndex] = {};
                            break;
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
                b32 removed = false;
                EquipmentComponent* equipment = GetComponent(server, ID, EquipmentComponent);
                if(equipment)
                {
                    for(u32 slotIndex = 0; slotIndex < ArrayCount(equipment->IDs); ++slotIndex)
                    {
                        EntityID equipmentID = equipment->IDs[slotIndex];
                        if(AreEqual(equipmentID, targetID))
                        {
                            removed = true;
                            equipment->IDs[slotIndex] = {};
                            break;
                        }
                        else
                        {
                            if(Remove(server, GetComponent(server, equipmentID, ContainerComponent), targetID))
                            {
                                removed = true;
                            }
                        }
                    }
                }
                
                if(!removed)
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
                                removed = true;
                                equipped->IDs[usingIndex] = {};
                                break;
                            }
                        }
                    }
                }
                
                if(removed)
                {
                    MakeTangible(server, targetID);
                }
            }
        } break;
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

internal void SendEffectDispatch(ServerState* server, EntityID ID)
{
    PhysicComponent* physic = GetComponent(server, ID, PhysicComponent);
    
    r32 maxDistance = 3.0f * CHUNK_DIM;
    r32 maxDistanceSq = Square(maxDistance);
    
    SpatialPartitionQuery playerQuery = QuerySpatialPartitionAtPoint(&server->playerPartition, physic->P);
    for(EntityID playerID = GetCurrent(&playerQuery); IsValid(&playerQuery); playerID = Advance(&playerQuery))
    {
        PlayerComponent* player = GetComponent(server, playerID, PlayerComponent);
        PhysicComponent* playerPhysic = GetComponent(server, playerID, PhysicComponent);
        
        if(physic->P.chunkZ == playerPhysic->P.chunkZ)
        {
            Vec3 distance = SubtractOnSameZChunk(physic->P, playerPhysic->P);
            if(LengthSq(distance) < maxDistanceSq)
            {
                QueueEffectDispatch(player, ID);
            }
        }
    }
}

internal void DispatchGameEffect(ServerState* server, EntityID ID, UniversePos P, GameEffect* effect)
{
    switch(effect->effectType.value)
    {
        case spawnEntity:
        {
            SendEffectDispatch(server, ID);
            //AddEntity(server, P, &server->entropy, effect->spawnType);
        } break;
        
        case moveOnZSlice:
        {
            if(HasComponent(ID, PhysicComponent))
            {
                PhysicComponent* physic = GetComponent(server, ID, PhysicComponent);
                if(++physic->P.chunkZ == (i32) server->maxDeepness)
                {
                    physic->P.chunkZ = 0;
                }
            }
        } break;
    }
}

internal void DispatchEntityEffects(ServerState* server, EntityID ID, r32 elapsedTime)
{
    PhysicComponent* physic = GetComponent(server, ID, PhysicComponent);
    EffectComponent* effects = GetComponent(server, ID, EffectComponent);
    if(physic && effects)
    {
        for(u32 effectIndex = 0; effectIndex < effects->effectCount; ++effectIndex)
        {
            GameEffect* effect = effects->effects + effectIndex;
            effects->timers[effectIndex] += elapsedTime;
            if(effects->timers[effectIndex] >= effect->timer)
            {
                effects->timers[effectIndex] = 0;
                DispatchGameEffect(server, ID, physic->P, effect);
            }
        }
    }
}

STANDARD_ECS_JOB_SERVER(DispatchEquipmentEffects)
{
    EquipmentComponent* equipment = GetComponent(server, ID, EquipmentComponent);
    if(equipment)
    {
        for(u32 equipmentIndex = 0; equipmentIndex < ArrayCount(equipment->IDs); ++equipmentIndex)
        {
            EntityID equipID = equipment->IDs[equipmentIndex];
            if(IsValidID(equipID))
            {
                DispatchEntityEffects(server, equipID, elapsedTime);
                
                ContainerComponent* container = GetComponent(server, equipID, ContainerComponent);
                for(u32 usingIndex = 0; usingIndex < container->maxUsingCount; ++usingIndex)
                {
                    EntityID usingID = container->usingIDs[usingIndex];
                    if(IsValidID(usingID))
                    {
                        DispatchEntityEffects(server, usingID, elapsedTime);
                    }
                }
            }
        }
    }
    
    UsingComponent* equipped = GetComponent(server, ID, UsingComponent);
    if(equipped)
    {
        for(u32 equipmentIndex = 0; equipmentIndex < ArrayCount(equipped->IDs); ++equipmentIndex)
        {
            EntityID usingID = equipped->IDs[equipmentIndex];
            if(IsValidID(usingID))
            {
                DispatchEntityEffects(server, usingID, elapsedTime);
            }
        }
    }
}

internal void DispatchCollisitonEffects(ServerState* server, UniversePos P, EntityID actor, EntityID trigger)
{
    CollisionEffectsComponent* effects = GetComponent(server, trigger, CollisionEffectsComponent);
    if(effects)
    {
        for(u32 effectIndex = 0; effectIndex < effects->effectCount; ++effectIndex)
        {
            GameEffect* effect = effects->effects + effectIndex;
            DispatchGameEffect(server, actor, P, effect);
        }
    }
}

internal void DispatchOverlappingEffects(ServerState* server, UniversePos P, EntityID actor, EntityID overlap)
{
    OverlappingEffectsComponent* effects = GetComponent(server, overlap, OverlappingEffectsComponent);
    if(effects)
    {
        for(u32 effectIndex = 0; effectIndex < effects->effectCount; ++effectIndex)
        {
            GameEffect* effect = effects->effects + effectIndex;
            DispatchGameEffect(server, actor, P, effect);
        }
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

internal void UpdateEntity(ServerState* server, EntityID ID, r32 elapsedTime)
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

internal void SendEntityUpdate(ServerState* server, EntityID ID, r32 elapsedTime)
{
    PhysicComponent* physic = GetComponent(server, ID, PhysicComponent);
    unsigned char buff_[KiloBytes(2)];
    u16 totalSize = PrepareEntityUpdate(server, physic, buff_);
    
    EquipmentComponent* equipment = GetComponent(server, ID, EquipmentComponent);
    UsingComponent* equipped = GetComponent(server, ID, UsingComponent);
    ContainerComponent* container = GetComponent(server, ID, ContainerComponent);
    
    r32 maxDistance = 3.0f * CHUNK_DIM;
    r32 maxDistanceSq = Square(maxDistance);
    
    SpatialPartitionQuery playerQuery = QuerySpatialPartitionAtPoint(&server->playerPartition, physic->P);
    for(EntityID playerID = GetCurrent(&playerQuery); IsValid(&playerQuery); playerID = Advance(&playerQuery))
    {
        PlayerComponent* player = GetComponent(server, playerID, PlayerComponent);
        PhysicComponent* playerPhysic = GetComponent(server, playerID, PhysicComponent);
        
        if(physic->P.chunkZ == playerPhysic->P.chunkZ)
        {
            Vec3 distance = SubtractOnSameZChunk(physic->P, playerPhysic->P);
            if(LengthSq(distance) < maxDistanceSq)
            {
                QueueEntityHeader(player, ID);
                u8* writeHere = ForgReserveSpace(player, GuaranteedDelivery_None, 0, totalSize, ID);
                Assert(writeHere);
                if(writeHere)
                {
                    Copy(totalSize, writeHere, buff_);
                }
                
                if(equipment)
                {
                    for(u16 slotIndex = 0; slotIndex < Count_equipmentSlot; ++slotIndex)
                    {
                        EntityID equipmentID = equipment->IDs[slotIndex];
                        //if(IsValid(equipmentID))
                        {
                            QueueEquipmentID(player, ID, slotIndex, equipmentID);
                        }
                    }
                }
                
                if(equipped)
                {
                    for(u16 slotIndex = 0; slotIndex < Count_usingSlot; ++slotIndex)
                    {
                        EntityID usingID = equipped->IDs[slotIndex];
                        //if(IsValid(usingID))
                        {
                            QueueUsingID(player, ID, slotIndex, usingID);
                            if(IsValidID(usingID))
                            {
                                Assert(!HasComponent(usingID, ContainerComponent));
                            }
                        }
                    }
                }
                
                if(container)
                {
                    QueueOpenedByID(player, ID, container->openedBy);
                    for(u16 objectIndex = 0; objectIndex < container->maxStoredCount; ++objectIndex)
                    {
                        EntityID objectID = container->storedIDs[objectIndex];
                        //if(IsValid(usingID))
                        {
                            QueueContainerStoredID(player, ID, objectIndex, objectID);
                        }
                    }
                    
                    for(u16 objectIndex = 0; objectIndex < container->maxUsingCount; ++objectIndex)
                    {
                        EntityID objectID = container->usingIDs[objectIndex];
                        //if(IsValid(usingID))
                        {
                            QueueContainerUsingID(player, ID, objectIndex, objectID);
                        }
                    }
                }
            }
        }
    }
}

internal UniversePos BuildPFromSpawnerGrid(r32 cellDim, u32 cellX, u32 cellY, i32 chunkZ)
{
    UniversePos result = {};
    result.chunkZ = chunkZ;
    result.chunkOffset.x = cellDim * cellX;
    result.chunkOffset.y = cellDim * cellY;
    
    result = NormalizePosition(result);
    return result;
}

struct PoissonP
{
    UniversePos P;
    PoissonP* next;
};

internal b32 Valid(PoissonP* positions, UniversePos P, r32 maxDelta)
{
    b32 result = true;
    
    r32 maxDistanceSq = Square(maxDelta);
    for(PoissonP* poisson = positions; poisson; poisson = poisson->next)
    {
        Vec3 delta = SubtractOnSameZChunk(P, poisson->P);
        if(LengthSq(delta) < maxDistanceSq)
        {
            result = false;
            break;
        }
    }
    
    return result;
}

internal void AddToPoission(PoissonP** positions, MemoryPool* pool, UniversePos P)
{
    PoissonP* newP = PushStruct(pool, PoissonP);
    newP->P = P;
    newP->next = *positions;
    *positions = newP;
}

internal void TriggerSpawner(ServerState* server, Spawner* spawner, UniversePos referenceP, RandomSequence* seq)
{
    if(spawner->optionCount)
    {
        r32 totalWeight = 0;
        for(u32 optionIndex = 0; optionIndex < spawner->optionCount; ++optionIndex)
        {
            totalWeight += spawner->options[optionIndex].weight;
        }
        r32 weight = totalWeight * RandomUni(seq);
        
        
        SpawnerOption* option = 0;
        
        r32 runningWeight = 0;
        for(u32 optionIndex = 0; optionIndex < spawner->optionCount; ++optionIndex)
        {
            SpawnerOption* test = spawner->options + optionIndex;
            runningWeight += test->weight;
            if(weight <= runningWeight)
            {
                option = test;
                break;
            }
        }
        
        PoissonP* entities = 0;
        PoissonP* clusters = 0;
        
        MemoryPool tempPool = {};
        for(u32 entityIndex = 0; entityIndex < option->entityCount; ++entityIndex)
        {
            SpawnerEntity* spawn = option->entities + entityIndex;
            i32 clusterCount = spawn->clusterCount + RoundReal32ToI32(spawn->clusterCountV * RandomBil(seq));
            
            for(i32 clusterIndex = 0; clusterIndex < clusterCount; ++clusterIndex)
            {
                UniversePos P = referenceP;
                Vec3 maxClusterOffset = V3(VOXEL_SIZE * spawn->maxClusterOffset, 0);
                
                b32 valid = false;
                u32 tries = 0;
                while(!valid && tries++ < 100)
                {
                    P = referenceP;
                    P.chunkOffset += Hadamart(maxClusterOffset, RandomBilV3(seq));
                    P = NormalizePosition(P);
                    if(Valid(clusters, P, spawn->minClusterDistance))
                    {
                        valid = true;
                        AddToPoission(&clusters, &tempPool, P);
                    }
                }
                
                if(valid)
                {
                    UniversePos clusterP = P;
                    i32 count = spawn->count + RoundReal32ToI32(spawn->countV * RandomBil(seq));
                    for(i32 index = 0; index < count; ++index)
                    {
                        Vec3 maxOffset = V3(VOXEL_SIZE * spawn->maxOffset, 0);
                        
                        valid = false;
                        tries = 0;
                        while(!valid && tries++ < 100)
                        {
                            P = clusterP;
                            P.chunkOffset += Hadamart(maxOffset, RandomBilV3(seq));
                            P = NormalizePosition(P);
                            
                            if(Valid(entities, P, spawn->minEntityDistance))
                            {
                                AddToPoission(&entities, &tempPool, P);
                                valid = true;
                            }
                        }
                        
                        
                        if(valid)
                        {
                            AddEntity(server, P, seq, spawn->type, 0);
                        }
                    }
                }
            }
        }
        
        Clear(&tempPool);
    }
}


internal void SpawnEntities(ServerState* server, r32 elapsedTime)
{
    MemoryPool tempPool = {};
    u16 spawnerCount;
    AssetID* spawners = GetAllDataAsset(&tempPool, server->assets, Spawner, "default", 0, &spawnerCount);
    for(u32 spawnerIndex = 0; spawnerIndex < spawnerCount; ++spawnerIndex)
    {
        AssetID sID = spawners[spawnerIndex];
        Spawner* spawner = GetData(server->assets, Spawner, sID);
        if(spawner->targetTime > 0)
        {
            spawner->time += elapsedTime;
            if(spawner->time >= spawner->targetTime)
            {
                spawner->time = 0;
                r32 cellDim = VOXEL_SIZE * spawner->cellDim;
                u32 cellCount = TruncateReal32ToU32(WORLD_SIDE / cellDim);
                
                u32 cellX = RandomChoice(&server->entropy, cellCount);
                u32 cellY = RandomChoice(&server->entropy, cellCount);
                i32 Z = 0;
                UniversePos P = BuildPFromSpawnerGrid(cellDim, cellX, cellY, Z);
                TriggerSpawner(server, spawner, P, &server->entropy);
            }
        }
    }
    
    Clear(&tempPool);
}

internal void BuildWorld(ServerState* server, b32 spawnEntities)
{
    
    RandomSequence generatorSeq = Seed(server->worldSeed);
    GameProperties properties = {};
    AssetID ID = QueryDataFiles(server->assets, world_generator, "default", &generatorSeq, &properties);
    if(IsValid(ID))
    {
        world_generator* generator = GetData(server->assets, world_generator, ID);
        server->nullTile = NullTile(generator);
        
        server->maxDeepness = generator->maxDeepness;
        
        Clear(&server->chunkPool);
        server->chunks = PushArray(&server->chunkPool, WorldChunk, generator->maxDeepness * WORLD_CHUNK_SPAN * WORLD_CHUNK_SPAN);
        WorldChunk* chunk = server->chunks;
        for(u32 chunkZ = 0; chunkZ < generator->maxDeepness; ++chunkZ)
        {
            for(u32 chunkY = 0; chunkY < WORLD_CHUNK_SPAN; ++chunkY)
            {
                for(u32 chunkX = 0; chunkX < WORLD_CHUNK_SPAN; ++chunkX)
                {
                    BuildChunk(server->assets, &server->chunkPool, generator,
                               chunk, chunkX, chunkY, chunkZ, server->worldSeed, 0);
                    ++chunk;
                }
            }
        }
    }
    else
    {
        InvalidCodePath;
    }
    
    RandomSequence seq = Seed(server->worldSeed);
    if(spawnEntities)
    {
        MemoryPool tempPool = {};
        u16 spawnerCount;
        AssetID* spawners = GetAllDataAsset(&tempPool, server->assets, Spawner, "default", 0, &spawnerCount);
        
        for(u32 spawnerIndex = 0; spawnerIndex < spawnerCount; ++spawnerIndex)
        {
            AssetID sID = spawners[spawnerIndex];
            Spawner* spawner = GetData(server->assets, Spawner, sID);
            r32 cellDim = VOXEL_SIZE * spawner->cellDim;
            u32 cellCount = TruncateReal32ToU32(WORLD_SIDE / cellDim);
            for(i32 chunkZ = 0; chunkZ < (i32) server->maxDeepness; ++chunkZ)
            {
                for(u32 Y = 0; Y < cellCount; ++Y)
                {
                    for(u32 X = 0; X < cellCount; ++X)
                    {
                        if(RandomUni(&seq) <= spawner->percentageOfStartingCells)
                        {
                            UniversePos P = BuildPFromSpawnerGrid(cellDim, X, Y, chunkZ);
                            TriggerSpawner(server, spawner, P, &seq);
                        }
                    }
                }
            }
        }
        
        Clear(&tempPool);
    }
}

STANDARD_ECS_JOB_SERVER(DeleteAllEntities)
{
    FreeArchetype(server, &ID);
}
