inline Vec3 DroppingVelocity(RandomSequence* seq)
{
    Vec3 result = V3(Hadamart(RandomBilV2(seq), V2(20, 20)), 0);
    return result;
}

inline u8 GetMaxObjectCount(TaxonomyTable* table, u32 taxonomy)
{
    u8 result = 0;
    TaxonomySlot* slot = GetSlotForTaxonomy(table, taxonomy);
    while(slot->taxonomy)
    {
        if(slot->gridDimX)
        {
            Assert(slot->gridDimY);
            result = slot->gridDimX * slot->gridDimY;
        }
        
        slot = GetParentSlot(table, slot);
    }
    
    return result;
}


inline void SpawnFromTileAssociation(SimRegion* region, TaxonomyAssociation* firstAssociation, r32 totalWeight, Vec3 P,  RandomSequence* seq)
{
    r32 destWeight = RandomUni(seq) * totalWeight;
    r32 runningWeight = 0;
    for(TaxonomyAssociation* ass = firstAssociation; ass; ass = ass->next)
    {
        runningWeight += ass->weight;
        if(destWeight <= runningWeight)
        {
            for(SpawnTaxonomy* spawn = ass->firstTaxonomy; spawn; spawn = spawn->next)
            {
                for(u32 index = 0; index < spawn->counter; ++index)
                {
                    Vec3 entityP = P;
                    entityP.xy += ass->radious * RandomBilV2(seq);
                    AddRandomEntity(region, seq, entityP, spawn->taxonomy);
                }
            }
            break;
        }
    }
}

internal void BuildSimpleTestWorld(ServerState* server)
{
    if(server->generator)
    {
        WorldGeneratorDefinition* generator = server->generator;
        
        u32 entityPerRegion = 1000;
        for(u32 regionY = 0; regionY < WORLD_REGION_SPAN; ++regionY)
        {
            for(u32 regionX = 0; regionX < WORLD_REGION_SPAN; ++regionX)
            {
                SimRegion* region = GetServerRegion(server, regionX, regionY);
                
                TaxonomyTable* taxTable = server->activeTable;
                RandomSequence* seq = &server->randomSequence;
                
                for(u32 entityIndex = 0; entityIndex < entityPerRegion; ++entityIndex)
                {
                    Vec3 P = V3(Hadamart(RandomBilV2(seq), V2(server->regionSpan, server->regionSpan)), 0);
                    
                    r32 waterLevel;
                    u32 tileTaxonomy = GetTileTaxonomyFromRegionP(server, region, P, &waterLevel);
                    if(waterLevel > WATER_LEVEL)
                    {
                        for(TaxonomyTileAssociations* tileAss = generator->firstAssociation; tileAss; tileAss = tileAss->next)
                        {
                            if(tileAss->taxonomy == tileTaxonomy && tileAss->totalWeight > 0)
                            {
                                SpawnFromTileAssociation(region, tileAss->firstAssociation, tileAss->totalWeight, P, seq);
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
}

internal void BuildServerChunks(ServerState* server)
{
    WorldGeneratorDefinition* generator = server->generator;
    if(generator)
    {
        u32 worldSeed = server->worldSeed;
        i32 offset = REGION_CHUNK_SPAN;
        for(i32 Y = -offset; Y < server->lateralChunkSpan + offset; Y++)
        {
            for(i32 X = -offset; X < server->lateralChunkSpan + offset; X++)
            {
                Assert(ChunkValid(server->lateralChunkSpan, X, Y));
                WorldChunk * chunk = GetChunk(server->chunks, ArrayCount(server->chunks), X, Y, &server->worldPool);
                
                for(EntityBlock* block = chunk->entities; block; block = block->next)
                {
                    for(u32 entityIndex = 0; entityIndex < block->countEntity; ++entityIndex)
                    {
                        u32 entityID = block->entityIDs[entityIndex];
                        SimEntity* entity = GetEntity(server, entityID);
                        DeleteEntityComponents(server, entity, entityID);
                    }
                }
                
                FREELIST_FREE(chunk->entities, EntityBlock, server->threadContext[0].firstFreeBlock);
                BuildChunk(server->activeTable, generator, chunk, X, Y, worldSeed);
            }
        }
    }
}

internal void BuildWorld(ServerState * server, GenerateWorldMode mode)
{
    server->chunkDim = CHUNK_DIM;
    server->chunkSide = server->chunkDim * VOXEL_SIZE;
    server->oneOverChunkSide = 1.0f / server->chunkSide;
    server->lateralChunkSpan = WORLD_REGION_SPAN * REGION_CHUNK_SPAN;
    server->regionSpan = REGION_CHUNK_SPAN * server->chunkSide;
    
    Assert(REGION_CHUNK_SPAN % 2 == 0);
    i32 realServerRegionSpan = WORLD_REGION_SPAN;
    
    for(i32 regionY = 0; regionY < realServerRegionSpan; regionY++)
    {
        for(i32 regionX = 0; regionX < realServerRegionSpan; regionX++)
        {
            i32 X = regionX - 1;
            i32 Y = regionY - 1;
            SimRegion * region = GetServerRegion(server, X, Y);
            
            region->regionX = X;
            region->regionY = Y;
            region->server = server;
            region->taxTable = server->activeTable;
            region->components = server->components;
            
            region->origin.chunkX = (X * REGION_CHUNK_SPAN) + REGION_CHUNK_SPAN / 2;
            region->origin.chunkY = (Y * REGION_CHUNK_SPAN) + REGION_CHUNK_SPAN / 2;
        }
    }
    
    server->randomSequence = Seed((i32) server->worldSeed);
    server->objectSequence = Seed((i32) server->worldSeed + 1);
    
    
    server->editorPlayerPermanent.regionX = WORLD_REGION_SPAN / 2;
    server->editorPlayerPermanent.regionY = WORLD_REGION_SPAN / 2;
    server->editorPlayerPermanent.P = {};
    
    u32 generatorTaxonomy = GetRandomChild(server->activeTable, &server->randomSequence, server->activeTable->generatorTaxonomy);
    WorldGeneratorDefinition* generator = 0;
    if(generatorTaxonomy != server->activeTable->generatorTaxonomy)
    {
        TaxonomySlot* slot = GetSlotForTaxonomy(server->activeTable, generatorTaxonomy);
        
        if(slot)
        {
            generator = slot->generatorDefinition;
        }
    }
    
    server->generator = generator;
    
	BuildServerChunks(server);
	if(mode == GenerateWorld_Standard)
	{
        BuildSimpleTestWorld(server);	
	}
}

internal void UpdateSeasonTimer()
{
    // NOTE(Leonardo): advance server "status"
    server->seasonTime += timeToAdvance;
    if(server->seasonTime >= SEASON_DURATION)
    {
        server->seasonTime = 0;
        server->season = (WorldSeason) ((server->season == Season_Count - 1) ? 0 : server->season + 1);
    }
    server->seasonLerp = Clamp01MapToRange(0.5f * SEASON_DURATION, server->seasonTime, SEASON_DURATION);
}

internal void MoveEntities(ServerState* server)
{
}

internal void SendEntityUpdates(ServerState* server)
{
    
    internal void SendEntityUpdate(SimRegion* region, SimEntity* entity)
    {
        ServerState* server = region->server;
        unsigned char buff_[KiloBytes(2)];
        Assert(sizeof(EntityUpdate) < ArrayCount(buff_));
        
        u16 totalSize = PrepareEntityUpdate(region, entity, buff_);
        
        PartitionSurfaceEntityBlock* playerSurfaceBlock = QuerySpacePartitionPoint(region, &region->playerPartition, entity->P);
        while(playerSurfaceBlock)
        {
            for( u32 playerIndex = 0; playerIndex < playerSurfaceBlock->entityCount; ++playerIndex )
            {
                CollisionData* collider = playerSurfaceBlock->colliders + playerIndex;
                SimEntity* entityToSend = GetRegionEntity(region, collider->entityIndex);
                
                Assert(entityToSend->playerID);
                ServerPlayer* player = server->players + entityToSend->playerID;
                
                if(collider->insideRegion)
                {
                    SendEntityHeader(player, entity->identifier);
                    
                    u8* writeHere = ForgReserveSpace(player, false, totalSize, entity->identifier);
                    Assert(writeHere);
                    if(writeHere)
                    {
                        Copy(totalSize, writeHere, buff_);
                    }
                    
                    if(entity->IDs[Component_Creature])
                    {   
                        CreatureComponent* creature = Creature(region, entity);
                        for(u8 slotIndex = 0; slotIndex < Slot_Count; ++slotIndex)
                        {
                            u64 equipmentID = creature->equipment[slotIndex].ID;
                            if(equipmentID)
                            {
                                SendEquipmentID(player, entity->identifier, slotIndex, equipmentID);
                            }
                        }
                    }
                    
                    if(entity->IDs[Component_Plant])
                    {
                        PlantComponent* plant = Plant(region, entity);
                        SendPlantUpdate(player, entity->identifier, plant);
                    }
                }
                
                
                if(entity->IDs[Component_Creature])
                {   
                    CreatureComponent* creature = Creature(region, entity);
                    if(creature->startedAction)
                    {
                        SendStartedAction(player, entity->identifier, creature->startedAction, creature->startedActionTarget);
                    }
                    
                    if(creature->completedAction)
                    {
                        SendCompletedAction(player, entity->identifier, creature->completedAction, creature->completedActionTarget);
                    }
                }
                
                
                if(collider->insideRegion)
                {
                    if(entityToSend->targetID == entity->identifier)
                    {
                        SendPossibleActions(region, entityToSend, entity, false);
                    }
                    
                    if(player->overlappingEntityID == entity->identifier)
                    {
                        SendPossibleActions(region, entityToSend, entity, true);
                    }
                }
            }
            
            playerSurfaceBlock = playerSurfaceBlock->next;
        }
    }
    
}