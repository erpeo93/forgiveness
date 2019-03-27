internal b32 MustBeUpdated(ServerState* server, SimRegion* region)
{
    b32 result = false;
    return true;
    
    if(region->playerCount > 0)
    {
        result = true;
    }
    else
    {
        r32 serverFrameMS = (5.0f * server->elapsedMS5x);
        if(1000.0f * region->timeToUpdate >= 2.0f * serverFrameMS)
        {
            result = true;
        }
    }
    
    return result;
}


internal b32 AllNeighBorsFinished(ServerState* server, SimRegion* region)
{
    i32 X = region->regionX;
    i32 Y = region->regionY;
    b32 allNeighBorsFinished = true;
    
    for(i32 testY = Y - 1; 
        (testY <= Y + 1) && allNeighBorsFinished;
        testY++ )
    {
        for(i32 testX = X - 1; 
            (testX <= X + 1) && allNeighBorsFinished; 
            testX++)
        {
            if(testX != X || testY != Y)
            {
                if(testX >= -1 && 
                   testY >= -1 && 
                   testX < SERVER_REGION_SPAN + 1 &&
                   testY < SERVER_REGION_SPAN + 1)
                {
                    SimRegion* test = GetServerRegion(server, testX, testY);
                    if(test->simulating)
                    {
                        allNeighBorsFinished = false;
                    }
                }
            }
        }
    }
    
    return allNeighBorsFinished;
}

inline u32 AddEntity(SimRegion* region, MemoryPool* pool)
{
    u32 result = 0;
    
    SimEntityBlock* block = region->firstEntityBlock;
    while(block)
    {
        if(block->entityCount < ArrayCount(block->entities))
        {
            result += block->entityCount++;
            if(block->entityCount == ArrayCount(block->entities))
            {
                SimEntityBlock* newBlock = PushStruct(pool, SimEntityBlock, NoClear());
                newBlock->entityCount = 0;
                newBlock->next = 0;
                block->next = newBlock;
            }
            break;
        }
        
        block = block->next;
        result += ArrayCount(block->entities);
    }
    
    return result;
}

inline void AddToIDHash( SimRegion* region, u64 id, u32 regionIndex)
{
    u32 index = (u32) id & (ArrayCount(region->IDHash) - 1);
    
}

inline void AddToIDHash( SimRegion* region, MemoryPool* pool, u64 id, u32 regionIndex )
{
    
    u32 index = ( u32 ) id & ( region->idHashCount - 1 );
    RegionIDHash* newHash = PushStruct( pool, RegionIDHash );
    
    newHash->ID = id;
    newHash->regionIndex = regionIndex;
    
    newHash->nextInHash = region->IDHash[index];
    region->IDHash[index] = newHash;
}

inline void InitSpacePartition(SpacePartition* partition, MemoryPool* pool, r32 gridDim, u32 partitionSideCount)
{
    partition->partitionSurfaceDim = partitionSideCount;
    partition->partitionSurfaces = PushArray(pool, RegionPartitionSurface, Squarei(partition->partitionSurfaceDim));
    r32 surfaceDim = gridDim / partition->partitionSurfaceDim;
    partition->oneOverSurfaceDim = 1.0f / surfaceDim;
}

internal void BeginSim(SimRegion* region, MemoryPool* tempPool)
{
    TIMED_FUNCTION();
    
    ServerState* server = region->server;
    region->taxTable = server->activeTable;
    
    i32 halfChunkSpan = SIM_REGION_CHUNK_SPAN / 2;
    
    
    i32 safetyChunkMargin = (region->border == Border_Mirror) ? 0 : 2;
    i32 totalHalfSpan = halfChunkSpan + safetyChunkMargin;
    Assert( region->origin.chunkOffset.z == 0 );
    
    region->gridSide = 2 * totalHalfSpan * CHUNK_DIM;
    u32 maxTileCount = Squarei(region->gridSide);
    //region->tiles = PushArray(tempPool, RegionTile, maxTileCount);
    
    region->fluidHashCount = 1024;
    //region->fluidHash = PushArray( tempPool, RegionFluidHash*, region->fluidHashCount );
    
    region->idHashCount = 1024;
    region->IDHash = PushArray(tempPool, RegionIDHash*, region->idHashCount);
    
    r32 regionSideDim = region->gridSide * VOXEL_SIZE;
    region->halfGridDim = 0.5f * regionSideDim; 
    
    
    InitSpacePartition(&region->collisionPartition, tempPool, regionSideDim, 32);
    InitSpacePartition(&region->playerPartition, tempPool, regionSideDim, 16);
    
    
    r32 chunkSide = VOXEL_SIZE * server->chunkDim;
    region->updateBounds = RectCenterDim(V2( 0, 0), chunkSide * V2(SIM_REGION_CHUNK_SPAN, SIM_REGION_CHUNK_SPAN));
    
    for(i32 Y = (i32) region->origin.chunkY - totalHalfSpan; Y < (i32 ) region->origin.chunkY + totalHalfSpan; Y++ )
    {
        for(i32 X = (i32) region->origin.chunkX - totalHalfSpan; X < ( i32 ) region->origin.chunkX + totalHalfSpan; X++)
        {
            if(ChunkValid(server->lateralChunkSpan, X, Y))
            {
                b32 mirrorChunk = ChunkOutsideWorld(server->lateralChunkSpan, X, Y);
                Vec3 chunkOffset = chunkSide * V3i(X - region->origin.chunkX, Y - region->origin.chunkY, 0);
                UniversePos doubleCheck = GetUniverseP(region, chunkOffset);
                Assert(doubleCheck.chunkX == X);
                Assert(doubleCheck.chunkY == Y);
                
                WorldChunk* chunk = GetChunk(server->chunks, ArrayCount( server->chunks ), X, Y, &server->worldPool);
                Assert(chunk->initialized);
                
#if 0                
                for( u32 tileY = 0; tileY < CHUNK_DIM; ++tileY )
                {
                    for( u32 tileX = 0; tileX < CHUNK_DIM; ++tileX )
                    {
                        Vec3 tileOffset = VOXEL_SIZE * V3( tileX + 0.5f, tileY + 0.5f, 0 ); 
                        Vec3 tileP = chunkOffset + tileOffset;
                        RegionTile* tile = GetRegionTile( region, tileP.xy );
                        tile->Y = ( Y * CHUNK_DIM ) + tileY; 
                        tile->X = ( X * CHUNK_DIM ) + tileX; 
                        
                        Assert( !tile->valid );
                        tile->valid = true;
                        tile->P = tileP;
                        
                        
                        r32 height = chunk->heights[tileY][tileX];
                        tile->waterAmount = chunk->waterAmount[tileY][tileX];
                        tile->P.z = height;
                    }
                }
#endif
                
                EntityBlock* block = chunk->entities;
                while(block)
                {
                    EntityBlock* nextBlock = block->next;
                    u8* entityRaw = block->data;
                    for(u32 entityIndex = 0; entityIndex < block->countEntity; ++entityIndex)
                    {
                        SimEntity* source = (SimEntity*) entityRaw;
                        entityRaw += sizeof(SimEntity);
                        
                        source->P += chunkOffset;
                        
                        Assert(source->P.x > -region->halfGridDim);
                        Assert(source->P.y > -region->halfGridDim);
                        Assert(source->P.x < region->halfGridDim);
                        Assert(source->P.y < region->halfGridDim);
                        
                        UniversePos P = GetUniverseP(region, source->P);
                        Assert(P.chunkX == X);
                        Assert(P.chunkY == Y);
                        
                        ClearFlags(source, Flag_insideRegion);
                        Assert(!IsSet( source, Flag_deleted));
                        Assert(source->taxonomy);
                        Assert(source->identifier);
                        
                        u32 index = AddEntity(region, tempPool);
                        AddToIDHash(region, tempPool, source->identifier, index);
                        
                        SimEntity* dest = GetRegionEntity(region, index);
						*dest = *source;
                        
                        if(PointInRect(region->updateBounds, dest->P.xy))
                        {
                            TaxonomySlot* slot = GetSlotForTaxonomy(region->taxTable, dest->taxonomy);
                            Assert(slot);
                            
                            AddFlags(dest, Flag_insideRegion);
                            if(region->border != Border_Mirror)
                            {
                                Assert(!ChunkOutsideWorld(region->server->lateralChunkSpan, P.chunkX, P.chunkY));
                            }
                        }
                        
#if 0                        
                        if(dest->fluid.type)
                        {
                            RegionTile* tile = GetRegionTile(region, dest->P.xy);
                            AddToFluidHash( region, tempPool, dest->fluid.source1, dest->fluid.sourceSegmentIndex1, dest->fluid.source2, dest->fluid.sourceSegmentIndex2, index, tile, ( i32 ) dest->P.z );
                        }
#endif
                        Rect3 bounds = dest->bounds;
                        Vec3 maxDelta = GetMaxDelta(region);
                        bounds = AddRadius(bounds, maxDelta);
                        
                        CollisionData collider;
                        collider.entityIndex = index;
                        collider.radiousSq = CalculateCollisionRadiousSq(bounds);
                        collider.P = dest->P;
                        
                        AddToSpacePartition(region, &region->collisionPartition, tempPool, dest->P, bounds, collider);
                    }
                    block->countEntity = 0;
                    block->usedData = 0;
                    
                    FREELIST_DEALLOC( block, region->context->firstFreeBlock );
                    block = nextBlock;
                }
                chunk->entities = 0;
            }
        }
    }
}

internal void EndSim(SimRegion* region)
{
    TIMED_FUNCTION();
    
    region->playerCount = 0;
    ServerState* server = region->server;
    
#if 0    
    for(u32 tileY = 0; tileY < region->gridSide; ++tileY )
    {
        for( u32 tileX = 0; tileX < region->gridSide; ++tileX )
        {
            RegionTile* tile = region->tiles + ( tileY * region->gridSide ) + tileX;
            if( tile->dirty )
            {
                u32 tX = tile->X % CHUNK_DIM;
                u32 tY = tile->Y % CHUNK_DIM;
                
                UniversePos tileP = GetUniverseP( region, tile->P );
                
                for(u32 blockIndex = 0; blockIndex < region->entityCount; ++blockIndex )
                {
                    SimEntity* entity = region->entities + blockIndex;
                    if( entity->playerID )
                    {
                        ServerPlayer* player = region->server->players + entity->playerID;
                        //SendTileUpdate( player, tileP.chunkX, tileP.chunkY, tX, tY, tile->waterAmount );
                    }
                }
                
                Assert( ChunkValid( server->lateralChunkSpan, tileP.chunkX, tileP.chunkY ) );
                WorldChunk* chunk = GetChunk( server->chunks, ArrayCount( server->chunks ), tileP.chunkX, tileP.chunkY, 0 );
                chunk->waterAmount[tY][tX] = tile->waterAmount;
            }
        }
    }
#endif
    
    for(u32 effectTriggerIndex = 0; effectTriggerIndex < region->effectTriggeredCount; ++effectTriggerIndex)
    {
        EffectTriggeredToSend* toSend = region->effectToSend + effectTriggerIndex;
        PartitionSurfaceEntityBlock* playerSurfaceBlock = QuerySpacePartitionPoint(region, &region->playerPartition, toSend->P);
        while(playerSurfaceBlock)
        {
            for( u32 playerIndex = 0; playerIndex < playerSurfaceBlock->entityCount; ++playerIndex )
            {
                CollisionData* collider = playerSurfaceBlock->colliders + playerIndex;
                SimEntity* entityToSend = GetRegionEntity(region, collider->entityIndex);
                Assert(entityToSend->playerID);
                ServerPlayer* player = server->players + entityToSend->playerID;
                
                SendEffectTriggered(player, toSend);
            }
            
            playerSurfaceBlock = playerSurfaceBlock->next;
        }
    }
    region->effectTriggeredCount = 0;
    
    
    for(SimEntityBlock* entityBlock = region->firstEntityBlock; entityBlock; entityBlock = entityBlock->next)
    {
        for(u32 blockIndex = 0; blockIndex < entityBlock->entityCount; blockIndex++)
        {
            b32 deleted = false;
            SimEntity* entity = entityBlock->entities + blockIndex;
            if(!IsSet(entity, Flag_deleted))
            {
                if(region->border != Border_Mirror &&
                   IsSet(entity, Flag_insideRegion) && 
                   entity->playerID)
                {
                    ++region->playerCount;
                    CreatureComponent* creature = Creature(region, entity);
                    ServerPlayer* player = server->players + entity->playerID;
                    if(player->connectionClosed)
                    {
                        deleted = true;
                    }
                    SendGameAccessConfirm(player, server->universeX, server->universeY, entity->identifier, creature->openedContainerID, server->elapsedMS5x);
                }
                
                SendEntityUpdate(region, entity);
            }
            else
            {
                deleted = true;
            }
            
            
            if(deleted)
            {
                SendDeleteMessage(region, entity);
                if(region->border != Border_Mirror &&
                   IsSet(entity, Flag_insideRegion))
                {
                    if(entity->IDs[Component_Creature])
                    {
                        CreatureComponent* creature = Creature(region, entity);
                        
                        for(u32 slotIndex = 0; slotIndex < Slot_Count; ++slotIndex)
                        {
                            u64 equipmentID = creature->equipment[slotIndex].ID;
                            if(equipmentID)
                            {
                                SimEntity* equipment = GetRegionEntityByID(region, equipmentID);
                                ClearFlags(equipment, Flag_Equipped);
                            }
                        }
                        
                        if(creature->draggingEntity)
                        {
                            SimEntity* dragging = creature->draggingEntity;
                            if(dragging && dragging->taxonomy)
                            {
                                if(entity->playerID)
                                {
                                    if(entity->taxonomy == region->taxTable->recipeTaxonomy)
                                    {
                                        AddEntity(region, entity->P, region->taxTable->recipeTaxonomy, 0, DroppedRecipeObject(dragging->taxonomy, dragging->recipeIndex, (i16) dragging->status));
                                    }
                                    else
                                    {
                                        ObjectComponent* object = Object(region, dragging);
                                        
                                        AddEntity(region, entity->P, dragging->taxonomy, dragging->recipeIndex, Dropped((u16) dragging->quantity, (i16) dragging->status, &object->objects));
                                    }
                                    
                                    ServerPlayer* player = region->server->players + entity->playerID;
                                    player->draggingEntity.taxonomy = 0;
                                }
                                else
                                {
                                    InvalidCodePath;
                                }
                                
                                creature->draggingEntity = 0;
                            }
                        }
                    }
                    
                    DeleteEntityComponents(region, entity);
                    SendUpdateToAdiacentRegions(region, entity->identifier);
                }
            }
            else
            {
                PackEntityIntoChunk(region, entity);
            }
        }
    }
    
    region->firstEntityBlock = 0;
}

inline void DispatchMirrorUpdate(SimRegion* region, SimEntity* entity, HashEntityUpdate* update)
{
    *entity = update->entity;
    UniversePos P = GetUniverseP(region, entity->P);
    if(!ChunkValid(region->server->lateralChunkSpan, P.chunkX, P.chunkY))
    {
        AddFlags(entity, Flag_deleted);
    }
    
    if(UNIVERSE_DIM > 1)
    {
        for(u32 componentIndex = 0; componentIndex < Component_Count; ++componentIndex)
        {
            entity->IDs[componentIndex] = 0;
        }
    }
    
    update->valid = false;
}

internal void DispatchRegionUpdate(SimRegion* region, MemoryPool* pool)
{
    TIMED_FUNCTION();
    Assert(region->updateHash);
    
    for(SimEntityBlock* entityBlock = region->firstEntityBlock; entityBlock; entityBlock = entityBlock->next)
    {
        for(u32 blockIndex = 0; blockIndex < entityBlock->entityCount; ++blockIndex)
        {
            SimEntity* entity = entityBlock->entities + blockIndex;
            HashEntityUpdate* update = GetHashUpdate(region, entity->identifier);
            if(update->valid)
            {
                DispatchMirrorUpdate(region, entity, update);
            }
        }
    }
    
    for(u32 updateIndex = 0; updateIndex < HASH_UPDATE_COUNT; ++updateIndex)
    {
        HashEntityUpdate* update = region->updateHash + updateIndex;
        if(update->valid)
        {
            u32 index = AddEntity(region, pool);
            SimEntity* entity = GetRegionEntity(region, index);
            DispatchMirrorUpdate(region, entity, update);
        }
    }
}


enum Season
{
    Season_Winter,
    Season_Spring,
    Season_Summer,
    Season_Autumn,
};

internal void UpdateRegionTiles( SimRegion* region )
{
    
#if 0    
    for( i32 tileY = 0; tileY < ( i32 ) region->gridSide; ++tileY )
    {
        for( i32 tileX = 0; tileX < ( i32 ) region->gridSide; ++tileX )
        {
            RegionTile* tile = region->tiles + ( tileY * region->gridSide ) + tileX;
            if( tile->waterAmount )
            {
                r32 myHeight = tile->P.z + tile->waterAmount;
                
                for( i32 testY = tileY - 1; testY <= tileY + 1; ++testY )
                {
                    for( i32 testX = tileX - 1; testX <= tileX + 1; ++testX )
                    {
                        if( testY > 0 && testY < ( i32 ) region->gridSide &&
                           testX > 0 && testX < ( i32 ) region->gridSide )
                        {
                            RegionTile* test = region->tiles + ( testY * region->gridSide ) + testX;
                            r32 hisHeight = test->P.z + test->waterAmount;
                            
                            if( hisHeight < myHeight )
                            {
                                r32 delta = myHeight - hisHeight;
                                
                                r32 waterMovement = delta / 0.5f * region->timeToUpdate;
                                test->waterAmount += waterMovement;
                                tile->waterAmount -= waterMovement;
                                test->dirty = true;
                            }
                        }
                    }
                }
            }
        }
    }
#endif
    
}

struct MarkedObject
{
    b32 isEssence;
    u16 quantity;
    union
    {
        ObjectReference objectReference;
        u32 essenceIndex;
    };
};

internal void HandlePlayerRequest(SimRegion* region, SimEntity* entity, PlayerRequest* requestData)
{
    ServerState* server = region->server;
    ServerPlayer* player = server->players + entity->playerID;
    
    CreatureComponent* creature = Creature(region, entity);
    
    unsigned char* data = requestData->data;
    ForgNetworkHeader header;
    data = ForgUnpackHeader(data, &header);
    switch(header.packetType)
    {
        case Type_ActionRequest:
        {
            Assert(entity->playerID);
            
            ActionRequest request;
            unpack(data, "LVLQQ", &request.sequenceNumber, &request.acceleration, &request.desiredAction, &request.targetEntityID, &request.overlappingEntityID);
            
            b32 valid = false;
            if(!Ignored(player, (EntityAction)request.desiredAction))
            {
                player->ignoredActionCount = 0;
                
                if(request.desiredAction < Action_Attack)
                {
                    valid = true;
                }
                else if(request.desiredAction == Action_Attack)
                {
                    if(request.acceleration == V3(0, 0, 0))
                    {
                        valid = true;
                    }
                }
                else
                {
                    valid = true;
                }
            }
            
            player->overlappingEntityID = request.overlappingEntityID;
            if(valid)
            {
                entity->acceleration = request.acceleration;
                entity->targetID = request.targetEntityID;
                
                if(request.desiredAction < Action_Attack)
                {
                    entity->action = (EntityAction)request.desiredAction;
                    if(LengthSq(request.acceleration) > 0)
                    {
                        entity->action = Action_Move;
                    }
                }
                else
                {
                    SimEntity* destEntity = GetRegionEntityByID(region, request.targetEntityID);
                    if(destEntity && !IsSet(destEntity, Flag_Equipped))
                    {
                        Vec3 ToP = destEntity->P - entity->P;
                        if(PlayerCanDoAction(region, entity, destEntity, (EntityAction)request.desiredAction, true))
                        {
                            entity->acceleration = {};
                            entity->action = (EntityAction) request.desiredAction;
                        }
                        else
                        {
                            entity->acceleration = ToP;
                            entity->action = Action_Move;
                        }
                    }
                }
                
                if(entity->action && creature->openedContainerID)
                {
                    SimEntity* container = GetRegionEntityByID(region, creature->openedContainerID);
                    Assert(container);
                    creature->openedContainerID = 0;
                }
            }
        } break;
        
        case Type_EquipRequest:
        {
            EquipRequest equip_;
            unpack( data, "QC", &equip_.sourceContainerID, &equip_.sourceObjectIndex);
            EquipRequest* equip = &equip_;
            SimEntity* container = GetRegionEntityByID(region, equip->sourceContainerID);
            if(container)
            {
                if(Owned(container, entity->identifier))
                {
                    ObjectComponent* objectComp = Object(region, container);
                    ContainedObjects* objects = &objectComp->objects;
                    Object* object = objects->objects + equip->sourceObjectIndex;
                    
                    u32 objectTaxonomy = GetObjectTaxonomy(region->taxTable, object);
                    EquipInfo info = PossibleToEquip(region, entity, object);
                    if(IsValid(info))
                    {
                        u64 ID = AddEntity(region, entity->P, objectTaxonomy, object->recipeIndex, EquippedBy(entity->identifier, object->quantity, object->status, 0));
                        
                        MarkAllSlotsAsOccupied(creature->equipment, info, ID);
                        
                        RemoveFromContainer(region, entity->identifier, container, equip->sourceObjectIndex);
                        
                        u8 maxObjectCount = GetMaxObjectCount(region->taxTable, objectTaxonomy);
                        if(maxObjectCount)
                        {
                            SendContainerInfo(player, ID, maxObjectCount);
                        }
                    }
                    else
                    {
                        InvalidCodePath;
                    }
                }
            }
        } break;
        
        case Type_EquipDraggingRequest:
        {
            EquipDraggingRequest equipDragging_;
            unpack(data, "L", &equipDragging_.slotIndex);
            
            EquipDraggingRequest* equipDragging = &equipDragging_;
            SimEntity* dragging = creature->draggingEntity;
            ObjectComponent* object = Object(region, dragging);
            ContainedObjects* objects = &object->objects;
            if(dragging->status >= 0 && dragging->taxonomy)
            {
                EquipmentMapping slotPresent = InventorySlotPresent(region->taxTable, entity->taxonomy, dragging->taxonomy);
                SlotName desired = (SlotName) equipDragging->slotIndex;
                Assert(desired > Slot_None);
                
                
                for(EquipmentLayout* layout = slotPresent.firstEquipmentLayout; layout; layout = layout->next)
                {
                    if(desired == layout->slot.slot)
                    {
                        EquipmentSlot* equipmentSlot = creature->equipment + desired;
                        if(!equipmentSlot->ID)
                        {
                            u64 ID = AddEntity(region, entity->P, dragging->taxonomy, dragging->recipeIndex, EquippedBy(entity->identifier, (u16) dragging->quantity, (u16) dragging->status, objects));
                            
                            MarkAllSlotsAsOccupied(creature->equipment, layout->slot, ID);
                            creature->draggingEntity = 0;
                            player->draggingEntity.taxonomy = 0;
                            
                            SendCompleteContainerInfoIdentifier(region, player, equipmentSlot->ID, dragging);
                        }
                        else
                        {
                            InvalidCodePath;
                        }
                    }
                }
            }
            else
            {
                InvalidCodePath;
            }
            
            DeleteEntityComponents(region, dragging);
        } break;
        
        case Type_DisequipRequest:
        {
            DisequipRequest disequip_;
            unpack( data, "LQC", &disequip_.slotIndex, &disequip_.destContainerID, &disequip_.destObjectIndex);
            
            DisequipRequest* disequip = &disequip_;
            EquipmentSlot* slot = creature->equipment + disequip->slotIndex;
            u64 toDisequipID = slot->ID;
            if(toDisequipID != disequip->destContainerID)
            {
                SimEntity* toDisequip = GetRegionEntityByID(region, toDisequipID);
                Assert(toDisequip);
                
                if(Owned(toDisequip, entity->identifier))
                {
                    MarkAsNullAllSlots(creature->equipment, toDisequipID);
                    
                    if(!disequip->destContainerID)
                    {
                        
                        ObjectComponent* object = Object(region, toDisequip);
                        ContainedObjects* objects = &object->objects;
                        
                        AddEntityAdditionalParams params = Dropped((u16) 1, (u16) toDisequip->status, objects);
                        AddEntity(region, entity->P, toDisequip->taxonomy, toDisequip->recipeIndex, params);
                    }
                    else
                    {
                        SimEntity* destContainer = GetRegionEntityByID(region, disequip->destContainerID);
                        ObjectComponent* object = Object(region, destContainer);
                        if(destContainer)
                        {
                            Object* destObject = object->objects.objects + disequip->destObjectIndex;
                            if(!destObject->taxonomy)
                            {
                                Object dest;
                                EntityToObject(toDisequip, &dest);
                                AddToContainer(region, entity->identifier, destContainer, disequip->destObjectIndex, dest);
                            }
                            else
                            {
                                InvalidCodePath;
                            }
                        }
                        else
                        {
                            InvalidCodePath;
                        }
                    }
                }
            }
            else
            {
                InvalidCodePath;
            }
        } break;
        
        case Type_DropRequest:
        {
            DropRequest drop_;
            unpack( data, "QC", &drop_.sourceContainerID, &drop_.sourceObjectIndex);
            
            DropRequest* drop = &drop_;
            if(!drop->sourceContainerID)
            {
                if(creature->draggingEntity && creature->draggingEntity->taxonomy)
                {
                    ObjectComponent* object = Object(region, creature->draggingEntity);
                    ContainedObjects* objects = &object->objects;
                    AddEntityAdditionalParams params = Dropped((u16) creature->draggingEntity->quantity, (u16) creature->draggingEntity->status, objects);
                    if(entity->taxonomy == region->taxTable->recipeTaxonomy)
                    {
                        params = DroppedRecipeObject(entity->recipeTaxonomy, entity->recipeIndex, (i16) entity->status);
                    }
                    AddEntity(region, entity->P, creature->draggingEntity->taxonomy, creature->draggingEntity->recipeIndex, params);
                    creature->draggingEntity = 0;
                    player->draggingEntity.taxonomy = 0;
                }
                else
                {
                    InvalidCodePath;
                }
            }
            else
            {
                SimEntity* container = GetRegionEntityByID(region, drop->sourceContainerID);
                if(container)
                {
                    if(Owned(container, entity->identifier))
                    {
                        ObjectComponent* objectComp = Object(region, container);
                        ContainedObjects* objects = &objectComp->objects;
                        Object* object = objects->objects + drop->sourceObjectIndex;
                        Assert(object->taxonomy);
                        
                        if(IsRecipe(object))
                        {
                            AddEntity(region, entity->P, region->taxTable->recipeTaxonomy, 0, DroppedRecipeObject(object->taxonomy, object->recipeIndex, object->status));
                        }
                        else
                        {
                            AddEntity(region, entity->P, object->taxonomy, object->recipeIndex, Dropped(object->quantity, object->status, 0));
                        }
                        RemoveFromContainer(region, entity->identifier, container, drop->sourceObjectIndex);
                    }
                }
            }
        } break;
        
        
        case Type_MoveRequest:
        {
            MoveRequest move_;
            unpack( data, "QCQC", &move_.sourceContainerID, &move_.sourceObjectIndex, &move_.destContainerID, &move_.destObjectIndex);
            
            MoveRequest* move = &move_;
            
            SimEntity* sourceContainer = GetRegionEntityByID(region, move->sourceContainerID);
            SimEntity* destContainer = GetRegionEntityByID(region, move->destContainerID);
            if(sourceContainer && destContainer)
            {
                if(Owned(sourceContainer, entity->identifier) && Owned(destContainer, entity->identifier))
                {
                    ObjectComponent* source = Object(region, sourceContainer);
                    ObjectComponent* dest = Object(region, destContainer);
                    ContainedObjects* sourceObjects = &source->objects;
                    ContainedObjects* destObjects = &dest->objects;
                    
                    if(move->sourceObjectIndex < sourceObjects->maxObjectCount &&
                       move->destObjectIndex < destObjects->maxObjectCount)
                    {
                        Object* sourceObject = sourceObjects->objects + move->sourceObjectIndex;
                        Assert(sourceObject->taxonomy);
                        
                        AddToContainer(region, entity->identifier, destContainer, move->destObjectIndex, *sourceObject);
                        RemoveFromContainer(region, entity->identifier, sourceContainer, move->sourceObjectIndex);
                    }
                    else
                    {
                        InvalidCodePath;
                    }
                }
                else
                {
                    InvalidCodePath;
                }
            }
        } break;
        
        case Type_SwapRequest:
        {
            SwapRequest swap_;
            unpack( data, "QC", &swap_.sourceContainerID, &swap_.sourceObjectIndex);
            
            SwapRequest* swap = &swap_;
            SimEntity* sourceContainer = GetRegionEntityByID(region, swap->sourceContainerID);
            if(sourceContainer)
            {
                if(Owned(sourceContainer, entity->identifier))
                {
                    ObjectComponent* objectComp = Object(region, sourceContainer);
                    ContainedObjects* sourceObjects = &objectComp->objects;
                    if(swap->sourceObjectIndex < sourceObjects->maxObjectCount)
                    {
                        Object* sourceObject = sourceObjects->objects + swap->sourceObjectIndex;
                        Object newDragging = *sourceObject;
                        
                        if(creature->draggingEntity && creature->draggingEntity->taxonomy)
                        {
                            ObjectComponent* dragObject = Object(region, creature->draggingEntity);
                            if(!dragObject->objects.objectCount)
                            {
                                if(sourceObject->taxonomy)
                                {
                                    RemoveFromContainer(region, entity->identifier, sourceContainer, swap->sourceObjectIndex);
                                }
                                Object object;
                                EntityToObject(creature->draggingEntity, &object);
                                AddToContainer(region, entity->identifier, sourceContainer, swap->sourceObjectIndex, object);
                            }
                            else
                            {
                                InvalidCodePath;
                            }
                        }
                        else
                        {
                            if(sourceObject->taxonomy)
                            {
                                RemoveFromContainer(region, entity->identifier, sourceContainer, swap->sourceObjectIndex);
                            }
                        }
                        
                        ObjectToEntity(region->taxTable, &newDragging, &player->draggingEntity);
                        creature->draggingEntity = &player->draggingEntity;
                    }
                    else
                    {
                        InvalidCodePath;
                    }
                }
                else
                {
                    InvalidCodePath;
                }
            }
        } break;
        
        case Type_DragEquipmentRequest:
        {
            DragEquipmentRequest drag_;
            unpack( data, "L", &drag_.slotIndex);
            
            DragEquipmentRequest* drag = &drag_;
            if(drag->slotIndex < Slot_Count)
            {
                EquipmentSlot* slot = creature->equipment + drag->slotIndex;
                u64 pieceID = slot->ID;
                SimEntity* equipmentPiece = GetRegionEntityByID(region, pieceID);
                if(equipmentPiece)
                {
                    if(Owned(equipmentPiece, entity->identifier))
                    {
                        Assert(IsSet(equipmentPiece, Flag_Equipped));
                        Assert(!player->draggingEntity.taxonomy);
                        
                        player->draggingEntity = *equipmentPiece;
                        creature->draggingEntity = &player->draggingEntity;
                        
                        AddFlags(equipmentPiece, Flag_deleted);
                        for(u32 componentIndex = 0; componentIndex < Component_Count; ++componentIndex)
                        {
                            equipmentPiece->IDs[componentIndex] = 0;
                        }
                        
                        MarkAsNullAllSlots(creature->equipment, pieceID);
                    }
                    else
                    {
                        InvalidCodePath;
                    }
                }
                else
                {
                    InvalidCodePath;
                }
            }
            else
            {
                InvalidCodePath;
            }
        } break;
        
        case Type_CraftRequest:
        {
            CraftRequest craft_;
            unpack(data, "LQ", &craft_.taxonomy, &craft_.recipeIndex);
            
            CraftRequest* craft = &craft_;
            
            b32 ownsRecipe = false;
            for(u32 recipeIndex = 0; recipeIndex < player->recipeCount; ++recipeIndex)
            {
                Recipe* recipe = player->recipes + recipeIndex;
                if(recipe->taxonomy == craft->taxonomy && recipe->recipeIndex == craft->recipeIndex)
                {
                    ownsRecipe = true;
                    break;
                }
            }
            
            if(ownsRecipe)
            {
                u32 toolCount = 0;
                ObjectReference tools[4];
                
                b32 hasAllTools = true;
                TaxonomySlot* recipeTaxSlot = GetSlotForTaxonomy(region->taxTable, craft->taxonomy);
                for(u32 toolIndex = 0; toolIndex < ArrayCount(recipeTaxSlot->neededToolTaxonomies); ++toolIndex)
                {
                    u32 toolTaxonomy = recipeTaxSlot->neededToolTaxonomies[toolIndex];
                    if(toolTaxonomy)
                    {
                        ObjectReference reference = HasObjectOfKind(region, entity, toolTaxonomy);
                        if(!reference.containerID)
                        {
                            hasAllTools = false;
                            break;
                        }
                        else
                        {
                            Assert(toolCount < ArrayCount(tools));
                            tools[toolCount++] = reference;
                        }
                    }
                }
                
                if(hasAllTools)
                {
                    RecipeIngredients ingredients;
                    GetRecipeIngredients(&ingredients, region->taxTable, craft->taxonomy, craft->recipeIndex);
                    
                    u32 markCount = 0;
                    MarkedObject marks[32];
                    
                    for(u32 slotIndex = 0; slotIndex < Slot_Count; ++slotIndex)
                    {
                        EquipmentSlot* slot = creature->equipment + slotIndex;
                        u64 ID = slot->ID;
                        
                        if(ID)
                        {
                            SimEntity* container = GetRegionEntityByID(region, ID);
                            ObjectComponent* objectComp = Object(region, container);
                            ContainedObjects* objects = &objectComp->objects;
                            
                            for(u32 objectIndex = 0; objectIndex < objects->maxObjectCount; ++objectIndex)
                            {
                                Object* object = objects->objects + objectIndex;
                                u32 taxonomy = object->taxonomy;
                                if(taxonomy && !IsRecipe(object))
                                {
                                    u16 quantity = object->quantity;
                                    Assert(quantity);
                                    
                                    for(u32 ingredientIndex = 0; ingredientIndex < ingredients.count; ++ingredientIndex)
                                    {
                                        if(taxonomy == ingredients.taxonomies[ingredientIndex])
                                        {
                                            if(ingredients.quantities[ingredientIndex] > 0)
                                            {
                                                u8 toUse = SafeTruncateToU8(quantity);
                                                if(toUse > ingredients.quantities[ingredientIndex])
                                                {
                                                    toUse = SafeTruncateToU8(ingredients.quantities[ingredientIndex]);
                                                }
                                                
                                                Assert(markCount < ArrayCount(marks));
                                                MarkedObject* mark = marks + markCount++;
                                                mark->isEssence = false;
                                                mark->objectReference.containerID = ID;
                                                mark->objectReference.objectIndex = SafeTruncateToU8(objectIndex);
                                                mark->quantity = toUse;
                                                
                                                ingredients.quantities[ingredientIndex] -= toUse;
                                            }
                                            
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                    }
                    
                    for(u32 essenceIndex = 0; essenceIndex < MAX_DIFFERENT_ESSENCES; ++essenceIndex)
                    {
                        EssenceSlot* slot = creature->essences + essenceIndex;
                        for(u32 ingredientIndex = 0; ingredientIndex < ingredients.count; ++ingredientIndex)
                        {
                            if(slot->taxonomy == ingredients.taxonomies[ingredientIndex])
                            {
                                u8 toUse = SafeTruncateToU8(slot->quantity);
                                if(toUse > ingredients.quantities[ingredientIndex])
                                {
                                    toUse = SafeTruncateToU8(ingredients.quantities[ingredientIndex]);
                                }
                                
                                Assert(markCount < ArrayCount(marks));
                                MarkedObject* mark = marks + markCount++;
                                mark->isEssence = true;
                                mark->essenceIndex = essenceIndex;
                                mark->quantity = toUse;
                                
                                ingredients.quantities[ingredientIndex] -= toUse;
                            }
                        }
                    }
                    
                    b32 canCraft = true;
                    for(u32 ingredientIndex = 0; ingredientIndex < ingredients.count; ++ingredientIndex)
                    {
                        if(ingredients.quantities[ingredientIndex] > 0)
                        {
                            canCraft = false;
                            break;
                        }
                    }
                    
                    if(canCraft)
                    {
                        for(u32 markIndex = 0; markIndex < markCount; ++markIndex)
                        {
                            MarkedObject* mark = marks + markIndex;
                            if(mark->isEssence)
                            {
                                EssenceSlot* slot = creature->essences + mark->essenceIndex;
                                EssenceDelta(region, entity, slot->taxonomy, -(i16)mark->quantity);
                            }
                            else
                            {
                                SimEntity* container = GetRegionEntityByID(region, mark->objectReference.containerID);
                                Object* object = GetObject(region, container, mark->objectReference.objectIndex);
                                
                                Assert(object->quantity);
                                
                                if(object->quantity > mark->quantity)
                                {
                                    object->quantity -= mark->quantity;
                                }
                                else
                                {
                                    RemoveFromContainer(region, entity->identifier, container, mark->objectReference.objectIndex);
                                }
                            }
                        }
                        
                        for(u32 toolIndex = 0; toolIndex < toolCount; ++toolIndex)
                        {
                            ObjectReference* tool = tools + toolIndex;
                            SimEntity* toolContainer = GetRegionEntityByID(region, tool->containerID);
                            Object* object = GetObject(region, toolContainer, tool->objectIndex);
                            
                            i16 X = 200;
                            if(object->status <= X)
                            {
                                RemoveFromContainer(region, entity->identifier, toolContainer, tool->objectIndex);
                            }
                            else
                            {
                                object->status -= X;
                            }
                        }
                        
                        i16 status = -10;
                        u64 identifier = AddEntity(region, entity->P, craft->taxonomy, craft->recipeIndex, Incomplete(entity->identifier, 0, status));
                        
                        IgnoreAction(region, entity, Action_None);
                        entity->action = Action_Craft;
                        entity->targetID = identifier;
                    }
                    else
                    {
                        InvalidCodePath;
                    }
                }
                else
                {
                    InvalidCodePath;
                }
            }
            else
            {
                InvalidCodePath;
            }
            
        } break;
        
        case Type_CraftFromInventoryRequest:
        {
            CraftFromInventoryRequest craft_;
            unpack(data, "QL", &craft_.containerID, &craft_.objectIndex);
            
            CraftFromInventoryRequest* craftFromInventory = &craft_;
            SimEntity* container = GetRegionEntityByID(region, craftFromInventory->containerID);
            
            if(container)
            {
                Object* toCraft = GetObjectIfOwned(region, entity, container, craftFromInventory->objectIndex);
                if(toCraft)
                {
                    
                    u64 identifier = AddEntity(region, entity->P, toCraft->taxonomy, toCraft->recipeIndex, Incomplete(entity->identifier, toCraft->quantity, toCraft->status));
                    RemoveFromContainer(region, entity->identifier, container, SafeTruncateToU8(craftFromInventory->objectIndex));
                    
                    IgnoreAction(region, entity, Action_None);
                    entity->action = Action_Craft;
                    entity->targetID = identifier;
                    
                }
                else
                {
                    InvalidCodePath;
                }
            }
            else
            {
                InvalidCodePath;
            }
            
        } break;
        
        case Type_LearnRequest:
        {
            LearnRequest learn_;
            unpack(data, "QL", &learn_.containerID, &learn_.objectIndex);
            
            LearnRequest* learn = &learn_;
            
            SimEntity* container = GetRegionEntityByID(region, learn->containerID);
            Object* recipeOjbect = GetObjectIfOwned(region, entity, container, learn->objectIndex);
            if(IsRecipe(recipeOjbect))
            {
                Assert(player->recipeCount < ArrayCount(player->recipes));
                Recipe* recipe = player->recipes + player->recipeCount++;
                recipe->taxonomy = recipeOjbect->taxonomy;
                recipe->recipeIndex = recipeOjbect->recipeIndex;
                SendNewRecipeMessage(player, recipe);
                RemoveFromContainer(region, entity->identifier, container, SafeTruncateToU8(learn->objectIndex));
            }
            else
            {
                InvalidCodePath;
            }
        } break;
        
        case Type_ConsumeRequest:
        {
            ConsumeRequest consume_;
            unpack(data, "QL", &consume_.containerID, &consume_.objectIndex);
            
            ConsumeRequest* consume = &consume_;
            SimEntity* container = GetRegionEntityByID(region, consume->containerID);
            Object* object = GetObjectIfOwned(region, entity, container, consume->objectIndex);
            if(object)
            {
                u32 taxonomy = GetObjectTaxonomy(region->taxTable, object);
                EntityAction action = CanConsume(region->taxTable, entity->taxonomy, taxonomy);
                
                if(action)
                {
                    u64 entityID = AddEntity(region, entity->P, taxonomy, 0, EquippedBy(entity->identifier, object->quantity, object->status, 0));
                    entity->action = Action_Eat;
                    entity->targetID = entityID;
                    
                    IgnoreAction(region, entity, Action_None);
                    
                    RemoveFromContainer(region, entity->identifier, container, SafeTruncateToU8(consume->objectIndex));
                }
                else
                {
                    InvalidCodePath;
                }
            }
            else
            {
                InvalidCodePath;
            }
        } break;
        
        case Type_ActiveSkillRequest:
        {
            SetActiveRequest active_;
            unpack(data, "L", &active_.taxonomy);
            
            SetActiveRequest* setActive = &active_;
            
            creature->activeSkillIndex = -1;
            for(u32 skillIndex = 0; skillIndex < creature->skillCount; ++skillIndex)
            {
                SkillSlot* skill = creature->skills + skillIndex;
                if(skill->taxonomy == setActive->taxonomy)
                {
                    creature->activeSkillIndex = skillIndex;
                    break;
                }
            }
        } break;
        
        case Type_PassiveSkillRequest:
        {
            PassiveSkillRequest passive;
            b32 activate;
            unpack(data, "Ll", &passive.taxonomy, &activate);
            
            b32 hasPassiveSkill = false;
            for(u32 skillIndex = 0; skillIndex < creature->skillCount; ++skillIndex)
            {
                SkillSlot* skill = creature->skills + skillIndex;
                if(skill->taxonomy == passive.taxonomy)
                {
                    hasPassiveSkill = true;
                    break;
                }
            }
            
            if(hasPassiveSkill)
            {
                for(u32 passiveSlotIndex = 0; passiveSlotIndex < MAX_PASSIVE_SKILLS_ACTIVE; ++passiveSlotIndex)
                {
                    SkillSlot* slot = creature->passiveSkills + passiveSlotIndex;
                    if(slot->taxonomy == passive.taxonomy)
                    {
                        slot->taxonomy = 0;
                        PassiveSkillEffects* effects = creature->passiveSkillEffects + passiveSlotIndex;
                        effects->effectCount = 0;
                    }
                }
                
                if(activate)
                {
                    for(u32 passiveSlotIndex = 0; passiveSlotIndex < MAX_PASSIVE_SKILLS_ACTIVE; ++passiveSlotIndex)
                    {
                        SkillSlot* skill = creature->passiveSkills + passiveSlotIndex;
                        if(!skill->taxonomy)
                        {
                            skill->taxonomy = passive.taxonomy;
                            PassiveSkillEffects* effects = creature->passiveSkillEffects + passiveSlotIndex;
                            
                            TaxonomySlot* slot = GetSlotForTaxonomy(region->taxTable, skill->taxonomy);
                            
                            for(TaxonomyEffect* taxEffect = slot->firstEffect; taxEffect; taxEffect = taxEffect->next)
                            {
                                Assert(effects->effectCount < ArrayCount(effects->effects));
                                effects->effects[effects->effectCount++] = taxEffect->effect;
                            }
                            break;
                        }
                    }
                }
            }
        } break;
        
        case Type_UnlockSkillCategoryRequest:
        {
            UnlockCategoryRequest unlock_;
            unpack(data, "L", &unlock_.taxonomy);
            
            UnlockCategoryRequest* unlock = &unlock_;
            TaxonomySlot* categorySlot = GetSlotForTaxonomy(region->taxTable, unlock->taxonomy);
            if(HasEssences(creature->essences, categorySlot->essences))
            {
                RemoveEssences(region, entity, categorySlot->essences);
                Assert(player->unlockedCategoryCount < ArrayCount(player->unlockedSkillCategories));
                player->unlockedSkillCategories[player->unlockedCategoryCount++] = unlock->taxonomy;
                SendUnlockedSkillCatConfirm(player, unlock->taxonomy);
            }
        } break;
        
        case Type_SkillLevelUpRequest:
        {
            LevelUpRequest levelUp_;
            unpack(data, "L", &levelUp_.taxonomy);
            
            LevelUpRequest* levelUp = &levelUp_;
            TaxonomySlot* skillSlot = GetSlotForTaxonomy(region->taxTable, levelUp->taxonomy);
            
            if(HasEssences(creature->essences, skillSlot->essences))
            {
                RemoveEssences(region, entity, skillSlot->essences);
                SkillSlot* toSend = 0;
                b32 found = false;
                for(u32 skillIndex = 0; skillIndex < creature->skillCount; ++skillIndex)
                {
                    SkillSlot* slot = creature->skills + skillIndex;
                    if(slot->taxonomy == levelUp->taxonomy)
                    {
                        toSend = slot;
                        slot->level += 1;
                        found = true;
                        break;
                    }
                }
                
                if(!found)
                {
                    Assert(creature->skillCount < ArrayCount(creature->skills));
                    toSend = creature->skills + creature->skillCount++;
                    toSend->level = 1;
                    toSend->taxonomy = levelUp->taxonomy;
                }
                SendSkillLevelUp(player, toSend, skillSlot->isPassiveSkill);
            }
        } break;
        
        case Type_InstantiateTaxonomy:
        {
            InstantiateRequest request;
            unpack(data, "LV", &request.taxonomy, &request.offset);
            RandomSequence* seq = &server->instantiateSequence;
            
            AddEntityAdditionalParams params = DefaultAddEntityParams();
            
            TaxonomySlot* slot = GetSlotForTaxonomy(server->activeTable, request.taxonomy);
            
            if(slot->firstDefaultLayout)
            {
                u64 recipeIndex = GetNextUInt32(&server->instantiateSequence);
                params = Crafting(recipeIndex);
            }
            AddRandomEntity(region, seq, entity->P + request.offset, request.taxonomy, params);
        } break;
        
        case Type_InstantiateRecipe:
        {
            InstantiateRecipeRequest request;
            unpack(data, "LQV", &request.taxonomy, &request.recipeIndex, &request.offset);
            
            if(request.taxonomy)
            {
                RandomSequence* seq = &server->instantiateSequence;
                u32 recipeTaxonomy = GetRandomChild(server->activeTable, seq, request.taxonomy);
                
                AddEntityAdditionalParams params = RecipeObject(recipeTaxonomy, request.recipeIndex);
                
                u32 taxonomy = server->activeTable->recipeTaxonomy;
                AddRandomEntity(region, seq, entity->P + request.offset, taxonomy, params);
            }
        } break;
        
        case Type_DeleteEntity:
        {
            u64 identifier;
            unpack(data, "Q", &identifier);
            SimEntity* toDelete = GetRegionEntityByID(region, identifier);
            if(toDelete)
            {
                AddFlags(toDelete, Flag_deleted);
            }
        } break;
        
        case Type_ImpersonateEntity:
        {
            u64 identifier;
            unpack(data, "Q", &identifier);
            SimEntity* target = GetRegionEntityByID(region, identifier);
            
            target->playerID = entity->playerID;
            entity->playerID = 0;
            
            player->requestCount = 0;
            
            SendGameAccessConfirm(player, server->universeX, server->universeY, identifier, 0, server->elapsedMS5x);
            InitPlayerEntity(region, player, entity);
            
        } break;
    }
}

internal void UpdatePlant(SimRegion* region, SimEntity* entity)
{
    PlantComponent* plant = Plant(region, entity);
    
    TaxonomySlot* slot = GetSlotForTaxonomy( region->taxTable, entity->taxonomy );
    
    plant->plantStatusPercentage += ( region->timeToUpdate / slot->plantStatusDuration[plant->plantStatus] );
    if(plant->plantStatusPercentage >= 1.0f )
    {
        plant->plantStatusPercentage = 0;
        plant->plantStatus = slot->nextStatus[plant->plantStatus];
    }
    
    if( plant->plantStatus == PlantLife_NewBranches )
    {
        plant->plantTotalAge += region->timeToUpdate;
    }
}

internal void UpdateEssence(SimRegion* region, SimEntity* entity)
{
    SimEntity* target = GetRegionEntityByID(region, entity->ownerID);
    if(target)
    {
        Assert(target->IDs[Component_Creature]);
        Vec3 toTarget = target->P - entity->P;
        entity->acceleration = toTarget;
        entity->action = Action_Move;
        
        r32 targetDistanceSq = Square(0.2f);
        r32 distanceSq = LengthSq(toTarget);
        if(distanceSq <= targetDistanceSq)
        {
            EssenceDelta(region, target, entity->taxonomy, 1);
            AddFlags(entity, Flag_deleted);
        }
    }
}

internal b32 UpdateCreature(SimRegion* region, SimEntity* entity)
{
    b32 result = false;
    
    
    Assert(entity->IDs[Component_Creature]);
    CreatureComponent* creature = Creature(region, entity);
    
    result = (creature->lifePoints <= 0);
    creature->startedAction = 0;
    creature->completedAction = 0;
    
    creature->skillCooldown -= region->timeToUpdate;
    creature->skillCooldown = Max(0, creature->skillCooldown);
    DispatchPassiveEffects(region, entity);
    
    Brain* brain = &creature->brain;
    if(brain->valid)
    {
        MemPerceive(region, entity, &brain->memory);
        HandleAI(region, entity);
    }
    
    if( entity->action != brain->oldAction || entity->targetID != brain->oldTargetID )
    {
        if(entity->action > Action_Move)
        {
            creature->startedAction = SafeTruncateToU8(entity->action);
            creature->startedActionTarget = entity->targetID;
        }
        
        if(brain->oldAction == Action_Craft)
        {
            SimEntity* target = GetRegionEntityByID(region, brain->oldTargetID);
            Assert(IsSet(target, Flag_Equipped));
            if(!PickObject(region, entity, target))
            {
                AddFlags(entity, Flag_deleted);
                AddEntity(region, entity->P, target->taxonomy, target->recipeIndex, Dropped(1, (i16) target->status, 0));
            }
        }
        
        entity->actionTime = 0;
        brain->oldAction = entity->action;
        brain->oldTargetID = entity->targetID;
    }
    
    return result;
}

inline void UpdateStatus(SimRegion* region, SimEntity* entity)
{
	Assert(entity->status != 0.0f);
	TaxonomySlot* slot = GetSlotForTaxonomy(region->taxTable, entity->taxonomy);
	r32 statusDropDelta = region->timeToUpdate;
	entity->status -= statusDropDelta;
	if(entity->status <= 0)
	{
		AddFlags(entity, Flag_deleted);
	}
}

internal void MoveEntity(SimRegion* region, SimEntity* entity)
{
    MoveSpec moveSpec = DefaultMoveSpec();
    
    if(entity->IDs[Component_Creature] && entity->action == Action_Move)
    {
        CreatureComponent* creature = Creature(region, entity);
        Brain* brain = &creature->brain;
        
        if(creature->brain.path.nodeCount)
        {
            u32 stepCount = (brain->path.nodeCount - 1) - brain->path.currentNodeIndex; 
            u8 reachingNodeIndex = brain->path.currentNodeIndex + 1;
            moveSpec.steps = brain->path.steps + reachingNodeIndex;
            moveSpec.stepCount = stepCount;
        }
    }
    
    MoveEntityServer(region, entity, moveSpec);
}

internal void UpdateRegionEntities(SimRegion* region, MemoryPool* tempPool)
{
    TIMED_FUNCTION();
    
    for(SimEntityBlock* entityBlock = region->firstEntityBlock; entityBlock; entityBlock = entityBlock->next)
    {
        for( u32 blockIndex = 0; blockIndex < entityBlock->entityCount; blockIndex++ )
        {
            SimEntity* entity = entityBlock->entities + blockIndex;
            if(!IsSet(entity, Flag_deleted) && !IsSet(entity, Flag_Equipped))
            {
                b32 died = false;
                if(IsSet(entity, Flag_insideRegion))
                {
                    if(region->border != Border_Mirror)
                    {
                        ServerPlayer* player = 0;
                        if(entity->playerID)
                        {
                            player = region->server->players + entity->playerID;
                            for(u32 requestIndex = 0; requestIndex < player->requestCount; ++requestIndex)
                            {
                                PlayerRequest* request = player->requests + requestIndex;
                                HandlePlayerRequest(region, entity, request);
                            }
                            player->requestCount = 0;
                        }
                        
                        u32 entityTaxonomy = entity->taxonomy;
						
                        if(IsObject(region->taxTable, entityTaxonomy))
                        {
                            UpdateStatus(region, entity);
                        }
                        
                        if(IsPlant(region->taxTable, entityTaxonomy))
                        {
                            UpdatePlant(region, entity);
                        }
                        
                        if(IsEssence(region->taxTable, entityTaxonomy))
                        {
                            UpdateEssence(region, entity);
                        }
                        
                        if(IsCreature(region->taxTable, entityTaxonomy))
                        {
                            died = UpdateCreature(region, entity);
                        }
                        
                        if(IsCreature(region->taxTable, entityTaxonomy) || IsEssence(region->taxTable, entityTaxonomy))
                        {
                            MoveEntity(region, entity);
                        }
                    }
                }
                
                UniversePos P = GetUniverseP(region, entity->P);
                b32 entityOutsideWorld = !PositionInsideWorld(region->server->lateralChunkSpan, &P);
                
				b32 handleEntityAction = ((region->border != Border_Mirror && IsSet(entity, Flag_insideRegion)) || (region->border == Border_Mirror && entityOutsideWorld));
                if(handleEntityAction)
                {
                    if(entity->action > Action_Move)
                    {
                        HandleAction(region, entity);
                    }
                    else
                    {
                        entity->actionTime = 0;
                    }
                    
                    
                    if(died)
                    {
                        DispatchEffects(region, entity, 0, Action_Die);
                        AddFlags(entity, Flag_deleted);
                    }
                }
                
                //if(entity->fluid)
                {
                    //HandleFluid(region, entity);
                }
            }
        }
    }
    
    
    u32 entityIndex = 0;
    for(SimEntityBlock* entityBlock = region->firstEntityBlock; entityBlock; entityBlock = entityBlock->next)
    {
        for( u32 entityBlockIndex = 0; entityBlockIndex < entityBlock->entityCount; entityBlockIndex++ )
        {
            SimEntity* entity = entityBlock->entities + entityBlockIndex;
            if(!IsSet(entity, Flag_deleted) && IsSet(entity, Flag_Equipped))
            {
                Assert(entity->ownerID);
                SimEntity* owner = GetRegionEntityByID(region, entity->ownerID);
                entity->P = owner->P;
            }
            
            if(region->border != Border_Mirror && entity->playerID)
            {
                CollisionData collider = {};
                collider.entityIndex = entityIndex;
                collider.insideRegion = IsSet(entity, Flag_insideRegion);
                
                Rect3 updateBounds = RectCenterDim(V3(0, 0, 0), V3(30.0f, 30.0f, 10.0f));
                AddToSpacePartition(region, &region->playerPartition, tempPool, 
                                    entity->P, updateBounds, collider);
            }
            ++entityIndex;
        }
    }
}

internal void SimulateRegionServer(SimRegion* region, MemoryPool* pool)
{
    TempMemory simulationMemory = BeginTemporaryMemory(pool);
    
    SimEntityBlock* firstBlock = PushStruct(pool, SimEntityBlock, NoClear());
    firstBlock->entityCount = 0;
    firstBlock->next = 0;
    region->firstEntityBlock = firstBlock;
    END_BLOCK();
    
    BeginSim(region, pool);
    if(region->border == Border_Mirror)
    {
        DispatchRegionUpdate(region, pool);
    }
    else
    {
        Assert(!region->updateHash);
        //UpdateRegionTiles(region);
        UpdateRegionEntities(region, pool);
    }
    EndSim(region);
    
    EndTemporaryMemory(simulationMemory);
    region->timeToUpdate = 0;
}