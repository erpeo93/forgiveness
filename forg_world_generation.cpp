inline r32 Evaluate(r32 dx, r32 dy, NoiseParams params, u32 seed)
{
    r32 frequency = params.frequency;
    r32 totalNoise = 0;
    r32 range = 1.0f;
    
    if(params.octaves > 0)
    {
        for(u32 octave = 0; octave < params.octaves; ++octave)
        {
            if(octave == 0)
            {
                totalNoise += UnilateralNoise(dx, dy, frequency, seed) * range;
            }
            else
            {
                totalNoise += BilateralNoise(dx, dy, frequency, seed) * range;
            }
            range *= params.persistance;
            frequency *= 2.0f;
        }
    }
    
    totalNoise = Clamp01(totalNoise);
    r32 result = Lerp(params.min, totalNoise, params.max);
    return result;
}


inline r32 Select(NoiseSelector* selector, r32 dx, r32 dy, r32 selectionValue, u32 seed, b32 lerpBuckets = true)
{
    r32 result = 0;
    if(selector->bucketCount > 0)
    {
        NoiseBucket* previousBucket = selector->buckets + 0;
        NoiseBucket* currentBucket = 0;
        r32 bucketLerping = 0;
        
        b32 bucketFound = false;
        r32 previousRef = R32_MIN;
        for(u32 bucketIndex = 0; bucketIndex < selector->bucketCount; ++bucketIndex)
        {
            currentBucket = selector->buckets + bucketIndex;
            
            r32 currentRef = currentBucket->referencePoint;
            if(selectionValue >= previousRef && selectionValue < currentRef)
            {
                bucketFound = true;
                bucketLerping = Clamp01MapToRange(previousRef, selectionValue, currentRef);
                break;
            }
            
            previousBucket = currentBucket;
            previousRef = currentRef;
        }
        
        if(!bucketFound)
        {
            NoiseBucket* lastBucket = currentBucket;
            previousBucket = lastBucket;
        }
        
        r32 current = Evaluate(dx, dy, currentBucket->params, seed);
        
        if(lerpBuckets)
        {
            r32 prev = Evaluate(dx, dy, previousBucket->params, seed);
            result = Lerp(prev, bucketLerping, current);
        }
        else
        {
            result = current;
        }
    }
    
    return result;
}

inline GameProperty Select(PropertySelector* selector, r32 selectionValue, u32 seed)
{
    GameProperty result = {};
    if(selector->bucketCount > 0)
    {
        PropertyBucket* previousBucket = selector->buckets + 0;
        PropertyBucket* currentBucket = 0;
        r32 bucketLerping = 0;
        
        b32 bucketFound = false;
        r32 previousRef = R32_MIN;
        for(u32 bucketIndex = 0; bucketIndex < selector->bucketCount; ++bucketIndex)
        {
            currentBucket = selector->buckets + bucketIndex;
            
            r32 currentRef = currentBucket->referencePoint;
            if(selectionValue >= previousRef && selectionValue < currentRef)
            {
                bucketFound = true;
                bucketLerping = Clamp01MapToRange(previousRef, selectionValue, currentRef);
                break;
            }
            
            previousBucket = currentBucket;
            previousRef = currentRef;
        }
        
        if(!bucketFound)
        {
            PropertyBucket* lastBucket = currentBucket;
            previousBucket = lastBucket;
        }
        
        result = currentBucket->property;
    }
    
    return result;
}

inline GameProperty SelectFromBiomePyramid(BiomePyramid* pyramid, r32 precipitationLevel, r32 darkness, r32 temperature, u32 seed)
{
    GameProperty result = {};
    r32 row = Select(&pyramid->darknessSelector, 0, 0, darkness, seed);
    if((u32) row < pyramid->drynessCount)
    {
        DrynessSelector* selector = pyramid->drynessSelectors + (u32) row;
        r32 row2 = Select(&selector->drynessSelector, 0, 0, precipitationLevel, seed);
        
        if((u32) row2 < selector->rowCount)
        {
            PropertySelector* temperatureSelector = selector->temperatureSelectors + (u32) row2;
            result = Select(temperatureSelector, temperature, seed);
        }
    }
    
    return result;
}

global_variable r32 minHeight = -100.0f;
global_variable r32 maxHeight = 1000.0f;

internal r32 GetTileElevation(world_generator* generator, r32 tileNormX, r32 tileNormY, r32 tileNormZ, RandomSequence* seq, u32 seed)
{
    r32 landscape = Evaluate(tileNormX, tileNormY, generator->landscapeNoise, seed);
    r32 waterMargin = Clamp01(generator->waterSafetyMargin);
    r32 result = minHeight;
    if(tileNormX < waterMargin || tileNormY < waterMargin || 
       tileNormX >= (1.0f - waterMargin) || tileNormY >= (1.0f - waterMargin))
    {
    }
    else
    {
        result = Select(&generator->landscapeSelect, tileNormX, tileNormY, landscape, seed);
        // NOTE(Leonardo): modify elevation to match out island shapes
        r32 normalizedElevation = Clamp01MapToRange(minHeight, result, maxHeight);
        r32 distanceFromCenter = Length(V2(tileNormX, tileNormY) - V2(0.5f, 0.5f));
        r32 elevationCoeff = Evaluate(tileNormX, tileNormY, generator->elevationNoise, seed);
        normalizedElevation = (generator->elevationNormOffset + normalizedElevation) - distanceFromCenter * elevationCoeff;
        normalizedElevation = Clamp01(normalizedElevation);
        normalizedElevation = Pow(normalizedElevation, generator->elevationPower);
        result = Lerp(minHeight, normalizedElevation, maxHeight);
    }
    
    return result;
}

internal ZSlice* GetZSlice(world_generator* generator, r32 tileNormZ)
{
    ZSlice* result = 0;
    
    r32 maxDelta = R32_MAX;
    for(u32 zSliceIndex = 0; zSliceIndex < generator->zSlicesCount; ++zSliceIndex)
    {
        ZSlice* slice = generator->zSlices + zSliceIndex;
        r32 delta = Abs(slice->referenceZ - tileNormZ);
        if(delta < maxDelta)
        {
            maxDelta = delta;
            result = slice;
        }
    }
    return result;
}

internal SoundMapping InitSoundMapping(SoundMappingDefinition* mapping, RandomSequence* seq);
inline WorldTile GenerateTile(Assets* assets, world_generator* generator, r32 tileNormX, r32 tileNormY, r32 tileNormZ, RandomSequence* seq, u32 seed)
{
    WorldTile result = {};
    
    // NOTE(Leonardo): elevation
    r32 elevation = GetTileElevation(generator, tileNormX, tileNormY, tileNormZ, seq, seed);
    r32 temperatureNoise = Evaluate(tileNormX, tileNormY, generator->temperatureNoise, seed);
    r32 temperatureDegrees = Select(&generator->temperatureSelect, temperatureNoise, temperatureNoise, elevation, seed, false);
    
    ZSlice* slice = GetZSlice(generator, tileNormZ);
    
    r32 annualMMPrecipitation = 0;
    r32 darkness = 0;
    
    if(slice)
    {
        // NOTE(Leonardo): precipitations
        annualMMPrecipitation = Evaluate(tileNormX, tileNormY, slice->precipitationNoise, seed);
        
        // NOTE(Leonardo): darkness
        darkness = Evaluate(tileNormX, tileNormY, slice->darknessNoise, seed);
    }
    
    GameProperty property = SelectFromBiomePyramid(&generator->biomePyramid, annualMMPrecipitation, darkness, temperatureDegrees, seed);
    
    GameProperties properties = {};
    properties.properties[0] = property;
    AssetID ID = QueryDataFiles(assets, tile_definition, "default", seq, &properties);
    if(IsValid(ID))
    {
        tile_definition* definition = GetData(assets, tile_definition, ID);
        result.property = definition->property;
        result.underSeaLevelFluid = {};
        if(elevation < 0)
        {
            result.underSeaLevelFluid = definition->underSeaLevelFluid;
			//result.underSeaLevelFluid = GameProp(fluid, ocean);
        }
        
#ifndef FORG_SERVER
        result.asset = definition->asset;
        for(u32 patchIndex = 0; patchIndex < ArrayCount(result.patches); ++patchIndex)
        {
            TilePatch* patch = result.patches + patchIndex;
            patch->offsetTime = RandomRangeFloat(seq, 0, 10);
            patch->colorTime = RandomRangeFloat(seq, 0, 10);
            patch->scaleTime = RandomRangeFloat(seq, 0, 10);
        }
        
        
        for(ArrayCounter soundIndex = 0; soundIndex < definition->soundCount; ++soundIndex)
        {
            if(result.soundCount < ArrayCount(result.sounds))
            {
                result.sounds[result.soundCount++] = InitSoundMapping(definition->sounds + soundIndex, seq);
            }
        }
        
        if(elevation < 0)
        {
            for(ArrayCounter soundIndex = 0; soundIndex < definition->underwaterSoundCount; ++soundIndex)
            {
                if(result.soundCount < ArrayCount(result.sounds))
                {
                    result.sounds[result.soundCount++] = InitSoundMapping(definition->underwaterSounds + soundIndex, seq);
                }
            }
            
        }
#endif
    }
    
    return result;
}


internal WorldTile NullTile(Assets* assets, world_generator* generator)
{
    RandomSequence seq = {};
    WorldTile result = GenerateTile(assets, generator, 0, 0, 0, &seq, 0);
    return result;
}

internal RandomSequence GetChunkSeed(u32 chunkX, u32 chunkY, u32 worldSeed)
{
    RandomSequence result = Seed(chunkX * chunkY * worldSeed);
    return result;
}

internal void BuildChunk(Assets* assets, MemoryPool* pool, world_generator* generator, WorldChunk* chunk, i16 chunkX, i16 chunkY, i16 chunkZ, u32 seed)
{
    RandomSequence seq = GetChunkSeed(chunkX, chunkY, seed);
    RandomSequence seqTest = seq;
    
#ifndef FORG_SERVER
    chunk->worldX = chunkX;
    chunk->worldY = chunkY;
    chunk->worldZ = chunkZ;
    chunk->worldSeed = seed;
#endif
    u32 maxTile = (WORLD_CHUNK_SPAN  + 2 * WORLD_CHUNK_APRON)* CHUNK_DIM;
    
    i16 normalizedChunkX = chunkX + WORLD_CHUNK_APRON;
    i16 normalizedChunkY = chunkY + WORLD_CHUNK_APRON;
    
    b32 buildTiles = false;
    r32 chunkNormZ = 0;
    
    if(generator->maxDeepness > 1)
    {
        chunkNormZ = (r32) chunkZ / (r32) (generator->maxDeepness - 1);
    }
    
    WorldTile nullTile = NullTile(assets, generator);
    if(chunk->tiles)
    {
        for(u8 tileY = 0; tileY < CHUNK_DIM && !buildTiles; ++tileY)
        {
            for(u8 tileX = 0; tileX < CHUNK_DIM && !buildTiles; ++tileX)
            {
                WorldTile* tile = chunk->tiles + (tileY * CHUNK_DIM) + tileX;
                *tile = nullTile;
            }
        }
    }
    
    for(u8 tileY = 0; tileY < CHUNK_DIM && !buildTiles; ++tileY)
    {
        for(u8 tileX = 0; tileX < CHUNK_DIM && !buildTiles; ++tileX)
        {
            u32 realTileX = normalizedChunkX * CHUNK_DIM + tileX;
            u32 realTileY = normalizedChunkY * CHUNK_DIM + tileY;
            
            // NOTE(Leonardo): normalized values
            r32 tileNormX = (r32) realTileX / maxTile;
            r32 tileNormY = (r32) realTileY / maxTile;
            r32 tileNormZ = chunkNormZ;
            
            Assert(Normalized(tileNormX));
            Assert(Normalized(tileNormY));
            Assert(Normalized(tileNormZ));
            
            r32 elevation = GetTileElevation(generator, tileNormX, tileNormY, tileNormZ, &seqTest, seed);
            if(elevation != minHeight)
            {
                buildTiles = true;
            }
        }
    }
    
#ifdef FORG_SERVER
    if(buildTiles)
#endif
    {
        if(!chunk->tiles)
        {
            chunk->tiles = PushArray(pool, WorldTile, CHUNK_DIM * CHUNK_DIM, NoClear());
        }
        
        for(u8 tileY = 0; tileY < CHUNK_DIM; ++tileY)
        {
            for(u8 tileX = 0; tileX < CHUNK_DIM; ++tileX)
            {
                u32 realTileX = normalizedChunkX * CHUNK_DIM + tileX;
                u32 realTileY = normalizedChunkY * CHUNK_DIM + tileY;
                
                // NOTE(Leonardo): normalized values
                r32 tileNormX = (r32) realTileX / maxTile;
                r32 tileNormY = (r32) realTileY / maxTile;
                r32 tileNormZ = chunkNormZ;
                
                Assert(Normalized(tileNormX));
                Assert(Normalized(tileNormY));
                Assert(Normalized(tileNormZ));
                
                WorldTile* tile = chunk->tiles + (tileY * CHUNK_DIM) + tileX;
                
                *tile = GenerateTile(assets, generator, tileNormX, tileNormY, tileNormZ, &seq, seed);
            }
        }
    }
}

#ifdef FORG_SERVER
internal UniversePos BuildPFromSpawnerGrid(r32 cellDim, u32 cellX, u32 cellY, i16 chunkZ)
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

internal b32 SatisfiesClusterRequirements(ServerState* server, UniversePos P)
{
    b32 result = (PositionInsideWorld(&P));
    return result;
}


internal b32 SatisfiesEntityRequirements(ServerState* server, UniversePos P)
{
    b32 result = false;
    if(PositionInsideWorld(&P))
    {
        result = true;
        
        WorldTile* tile = GetTile(server, P);
        if(IsValid(tile->underSeaLevelFluid))
        {
            result = false;
        }
    }
    return result;
}

internal void AddEntity(ServerState* server, UniversePos P, RandomSequence* seq, EntityRef type, AddEntityParams params);
internal void SpawnPlayerGhost(ServerState* server, UniversePos P, AddEntityParams params);
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
                    if(SatisfiesClusterRequirements(server, P))
                    {
                        if(Valid(clusters, P, spawn->minClusterDistance))
                        {
                            valid = true;
                            AddToPoission(&clusters, &tempPool, P);
                        }
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
                            
                            if(SatisfiesEntityRequirements(server, P))
                            {
                                if(Valid(entities, P, spawn->minEntityDistance))
                                {
                                    AddToPoission(&entities, &tempPool, P);
                                    valid = true;
                                }
                            }
                        }
                        
                        if(valid)
                        {
                            AddEntityParams params = DefaultAddEntityParams();
                            if(spawn->attachedBrainEntity)
                            {
                                params.spawnFollowingEntity = true;
                                params.attachedEntityType = spawn->attachedBrainType;
                            }
                            
                            AddEntity(server, P, seq, spawn->type, params);
                        }
                    }
                }
            }
        }
        
        Clear(&tempPool);
    }
}


internal void GenerateEntity(ServerState* server, NewEntity* newEntity)
{
    Assert(IsValid(newEntity->definitionID));
    EntityDefinition* definition = GetData(server->assets, EntityDefinition, newEntity->definitionID);
    EntityID ID = {};
    ServerEntityInitParams params = definition->server;
    params.P = newEntity->P;
    params.startingAcceleration = newEntity->params.acceleration;
    params.startingSpeed = newEntity->params.speed;
    definition->common.definitionID = EntityReference(newEntity->definitionID);
    params.seed = newEntity->seed;
    definition->common.essences = newEntity->params.essences;
    
    u8 archetype = SafeTruncateToU8(ConvertEnumerator(EntityArchetype, definition->archetype));
    AcquireArchetype(server, archetype, (&ID));
    InitEntity(server, ID, &definition->common, &params, 0); 
    
    if(newEntity->params.playerIndex > 0)
    {
        Assert(HasComponent(ID, PlayerComponent));
        PlayerComponent* player = (PlayerComponent*) Get_(&server->PlayerComponent_, newEntity->params.playerIndex);
        player->justEnteredWorld = true;
        player->skillPoints = 0;
        player->ID = ID;
        SetComponent(server, ID, PlayerComponent, player);
        
        if(HasComponent(ID, BrainComponent))
        {
            BrainComponent* brain = GetComponent(server, ID, BrainComponent);
            if(newEntity->params.ghost)
            {
                brain->type = Brain_invalid;
            }
            else
            {
                brain->type = Brain_Player;
                ResetQueue(player->queues + GuaranteedDelivery_None);
                QueueGameAccessConfirm(player, server->worldSeed, ID, false);
            }
        }
    }
    
    if(newEntity->params.spawnFollowingEntity)
    {
        DefaultComponent* def = GetComponent(server, ID, DefaultComponent);
        AddEntityFlags(def, EntityFlag_locked);
        AddEntityParams attachedParams = DefaultAddEntityParams();
        attachedParams.targetBrainID = ID;
        AddEntity(server, newEntity->P, &server->entropy, newEntity->params.attachedEntityType, attachedParams);
    }
    
    if(IsValidID(newEntity->params.targetBrainID))
    {
		DefaultComponent* targetDef = GetComponent(server, newEntity->params.targetBrainID, DefaultComponent);
		Assert(EntityHasFlags(targetDef, EntityFlag_locked));
        ClearEntityFlags(targetDef, EntityFlag_locked);
        BrainComponent* brain = GetComponent(server, newEntity->params.targetBrainID, BrainComponent);
        if(brain)
        {
            brain->ID = ID;
        }
    }
}

internal void SpawnAndDeleteEntities(ServerState* server, r32 elapsedTime)
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
                i16 Z = 0;
                UniversePos P = BuildPFromSpawnerGrid(cellDim, cellX, cellY, Z);
                TriggerSpawner(server, spawner, P, &server->entropy);
            }
        }
    }
    
    Clear(&tempPool);
    
    NewEntity* firstCurrent = server->firstNewEntity;
    for(NewEntity* newEntity = server->firstNewEntity; newEntity; newEntity = newEntity->next)
    {
        GenerateEntity(server, newEntity);
    }
    
	//NOTE(leonardo): "following" entities that could have been spawned in the above loop
    for(NewEntity* newEntity = server->firstNewEntity; newEntity != firstCurrent; newEntity = newEntity->next)
    {
        GenerateEntity(server, newEntity);
    }
    FREELIST_FREE(server->firstNewEntity, NewEntity, server->firstFreeNewEntity);
    
    
    for(DeletedEntity* deleted = server->firstDeletedEntity; deleted; deleted = deleted->next)
	{
        EntityID ID = deleted->ID;
		DefaultComponent* def = GetComponent(server, ID, DefaultComponent);
        
        if(HasComponent(ID, StaticComponent))
        {
            StaticComponent* staticComponent = GetComponent(server, ID, StaticComponent);
            Assert(staticComponent->chunk);
            RemoveFromSpatialPartition(&server->staticPartition, staticComponent->chunk, staticComponent->block, ID);
        }
        else
        {
            Assert(HasComponent(ID, PhysicComponent));
        }
        
        if(HasComponent(ID, PlayerComponent))
        {
            PlayerComponent* player = GetComponent(server, ID, PlayerComponent);
            if(player)
            {
                switch(deleted->type)
                {
                    case DeleteEntity_None:
                    {
                        player->ID = {};
                        AddEntityParams params = GhostEntityParams();
                        params.playerIndex = GetIndex_(&server->PlayerComponent_, player);
                        QueueGameOverMessage(player);
                        SpawnPlayerGhost(server, def->P, params);
                    } break;
                    
                    case DeleteEntity_Ghost:
                    {
                        
                    } break;
                    
                    case DeleteEntity_Won:
                    {
                        player->ID = {};
                        AddEntityParams params = GhostEntityParams();
                        params.playerIndex = GetIndex_(&server->PlayerComponent_, player);
                        QueueGameWonMessage(player);
                        SpawnPlayerGhost(server, def->P, params);
                    } break;
                }
            }
        }
		
        FreeArchetype(server, ID);
	}
    
    FREELIST_FREE(server->firstDeletedEntity, DeletedEntity, server->firstFreeDeletedEntity);
}

internal void BuildWorld(ServerState* server, b32 spawnEntities)
{
    
    RandomSequence generatorSeq = Seed(server->worldSeed);
    GameProperties properties = {};
    AssetID ID = QueryDataFiles(server->assets, world_generator, "default", &generatorSeq, &properties);
    if(IsValid(ID))
    {
        world_generator* generator = GetData(server->assets, world_generator, ID);
        server->nullTile = NullTile(server->assets, generator);
        
        server->maxDeepness = (i16) generator->maxDeepness;
        
        Clear(&server->chunkPool);
        server->chunks = PushArray(&server->chunkPool, WorldChunk, generator->maxDeepness * WORLD_CHUNK_SPAN * WORLD_CHUNK_SPAN);
        WorldChunk* chunk = server->chunks;
        for(i16 chunkZ = 0; chunkZ < server->maxDeepness; ++chunkZ)
        {
            for(i16 chunkY = 0; chunkY < WORLD_CHUNK_SPAN; ++chunkY)
            {
                for(i16 chunkX = 0; chunkX < WORLD_CHUNK_SPAN; ++chunkX)
                {
                    BuildChunk(server->assets, &server->chunkPool, generator,
                               chunk, chunkX, chunkY, chunkZ, server->worldSeed);
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
            for(i16 chunkZ = 0; chunkZ < (i16) server->maxDeepness; ++chunkZ)
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
#endif