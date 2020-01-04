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
#if 0
        result = Select(&generator->landscapeSelect, tileNormX, tileNormY, landscape, seed);
#else
        result = landscape;
#endif
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
#if 0
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
    
#else
    GameProperty property = {};
    r32 elevation = GetTileElevation(generator, tileNormX, tileNormY, tileNormZ, seq, seed);
    if(generator->biomeConfigurationCount > 0)
    {
        RandomSequence configurationSeed = Seed(seed);
        BiomeConfiguration* configuration = generator->biomeConfigurations + RandomChoice(&configurationSeed, generator->biomeConfigurationCount);
        
        r32 minDelta = R32_MAX;
        for(ArrayCounter bandIndex = 0; bandIndex < configuration->biomeBandCount; ++bandIndex)
        {
            BiomeBand* band = configuration->biomeBands + bandIndex;
            r32 delta = Abs(band->referenceHeight - elevation);
            if(delta < minDelta)
            {
                minDelta = delta;
                property = band->tile;
            }
        }
        
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
                result.underSeaLevelFluid = configuration->underSeaLevelFluid;
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
                for(ArrayCounter soundIndex = 0; soundIndex < configuration->underwaterSoundCount; ++soundIndex)
                {
                    if(result.soundCount < ArrayCount(result.sounds))
                    {
                        result.sounds[result.soundCount++] = InitSoundMapping(configuration->underwaterSounds + soundIndex, seq);
                    }
                }
                
            }
#endif
        }
    }
#endif
    
    return result;
}


internal WorldTile NullTile(Assets* assets, world_generator* generator)
{
    RandomSequence seq = {};
    WorldTile result = GenerateTile(assets, generator, 0, 0, 0, &seq, 0);
    return result;
}

internal RandomSequence GetChunkSeed(u32 chunkX, u32 chunkY, u32 chunkZ, u32 worldSeed)
{
    RandomSequence result = Seed(chunkX * chunkY * chunkZ * worldSeed);
    return result;
}

internal void BuildChunk(Assets* assets, MemoryPool* pool, world_generator* generator, WorldChunk* chunk, i16 chunkX, i16 chunkY, i16 chunkZ, u32 seed)
{
    RandomSequence seq = GetChunkSeed(chunkX, chunkY, chunkZ, seed);
    RandomSequence seqTest = seq;
    u32 sliceSeed = seed + chunkZ;
    
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
            
            r32 elevation = GetTileElevation(generator, tileNormX, tileNormY, tileNormZ, &seqTest, sliceSeed);
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
                
                *tile = GenerateTile(assets, generator, tileNormX, tileNormY, tileNormZ, &seq, sliceSeed);
            }
        }
    }
}

#ifdef FORG_SERVER
internal UniversePos BuildPFromSpawnerGrid(r32 cellDim, u32 cellX, u32 cellY, i16 chunkZ)
{
    UniversePos result = {};
    result.chunkZ = chunkZ;
    result.chunkOffset.x = cellDim * cellX + 0.5f * cellDim;
    result.chunkOffset.y = cellDim * cellY + 0.5f * cellDim;
    
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

internal b32 SatisfiesClusterRequirements(ServerState* server, UniversePos referenceP, UniversePos P, r32 cellDim)
{
    Assert(referenceP.chunkZ == P.chunkZ);
    Vec3 delta = SubtractOnSameZChunk(referenceP, P);
    b32 result = (PositionInsideWorld(&P) && (Abs(delta.x) <= cellDim) && (Abs(delta.y) <= cellDim));
    return result;
}

internal b32 SatisfiesTileRadiousRequirement(ServerState* server, UniversePos P, GameProperty requiredTile, GameProperty requiredFluid, u32 requiredRadious, b32 default)
{
    b32 result = default;
    if(requiredTile.value != tile_invalid || requiredFluid.value != fluid_invalid)
    {
        result = false;
        i32 radious = (i32) requiredRadious;
        for(i32 Y = -radious; Y <= radious; ++Y)
        {
            for(i32 X = -radious; X <= radious; ++X)
            {
                Vec3 offset = V3(V2i(X, Y), 0);
                UniversePos tileP = Offset(P, offset);
                if(PositionInsideWorld(&tileP))
                {
                    WorldTile* tile = GetTile(server, tileP);
                    if(AreEqual(tile->property, requiredTile) || AreEqual(tile->underSeaLevelFluid, requiredFluid))
                    {
                        result = true;
                        break;
                    }
                }
            }
        }
    }
    
    return result;
}

internal b32 SatisfiesEntityRequirements(ServerState* server, UniversePos referenceP, UniversePos P, SpawnerEntity* spawner, r32 cellDim)
{
    b32 result = false;
    
    Assert(referenceP.chunkZ == P.chunkZ);
    Vec3 delta = SubtractOnSameZChunk(referenceP, P);
    if(PositionInsideWorld(&P) && (Abs(delta.x) <= cellDim) && (Abs(delta.y) <= cellDim))
    {
        
        result = SatisfiesTileRadiousRequirement(server, P, spawner->requiredTile, spawner->requiredFluid, spawner->requiredRadious, true);
        if(result)
        {
            result = !SatisfiesTileRadiousRequirement(server, P, spawner->repulsionTile, spawner->repulsionFluid, spawner->repulsionRadious, false);
            
            if(result && spawner->occupiesTile)
            {
                WorldTile* tile = GetTile(server, P);
                if(tile->occupiedWorldGeneration)
                {
                    result = false;
                }
            }
        }
    }
    return result;
}

internal void MarkTileAsOccupied(ServerState* server, UniversePos P)
{
    WorldTile* tile = GetTile(server, P);
    tile->occupiedWorldGeneration = true;
}

internal void AddEntity(ServerState* server, UniversePos P, RandomSequence* seq, EntityType type, AddEntityParams params);
internal void SpawnPlayerGhost(ServerState* server, UniversePos P, AddEntityParams params);
internal void TriggerSpawnerInCell(ServerState* server, PoissonP* entities, PoissonP* clusters, MemoryPool* poissonPool, Spawner* spawner, UniversePos referenceP, RandomSequence* seq, r32 cellDim)
{
    if(spawner->optionCount)
    {
        i32 clusterCount = spawner->clusterCount + RoundReal32ToI32(spawner->clusterCountV * RandomBil(seq));
        
        for(i32 clusterIndex = 0; clusterIndex < clusterCount; ++clusterIndex)
        {
            Vec3 maxClusterOffset = spawner->clusterOffsetCellDimCoeff * V3(cellDim, cellDim, 0);
            u32 tries = 0;
            while(tries++ < 100)
            {
                UniversePos clusterP = referenceP;
                Vec3 clusterOffset = Hadamart(maxClusterOffset, RandomBilV3(seq));
                clusterP = Offset(clusterP, clusterOffset);
                
                if(SatisfiesClusterRequirements(server, referenceP, clusterP, cellDim))
                {
                    if(Valid(clusters, clusterP, spawner->minClusterDistance))
                    {
                        AddToPoission(&clusters, poissonPool, clusterP);
                        
                        u32 clusterTries = 0;
                        while(clusterTries++ < 100)
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
                            
                            if(SatisfiesTileRadiousRequirement(server, clusterP, option->requiredTile, option->requiredFluid, option->requiredRadious, true) &&
                               !SatisfiesTileRadiousRequirement(server, clusterP, option->repulsionTile, option->repulsionFluid, option->repulsionRadious, false))
                            {
                                for(u32 entityIndex = 0; entityIndex < option->entityCount; ++entityIndex)
                                {
                                    SpawnerEntity* spawn = option->entities + entityIndex;
                                    
                                    i32 count = spawn->count + RoundReal32ToI32(spawn->countV * RandomBil(seq));
                                    for(i32 index = 0; index < count; ++index)
                                    {
                                        Vec3 maxOffset = spawn->entityOffsetCellDimCoeff * V3(cellDim, cellDim, 0);
                                        u32 entityTries = 0;
                                        while(entityTries++ < 100)
                                        {
                                            UniversePos entityP = clusterP;
                                            Vec3 entityOffset = Hadamart(maxOffset, RandomBilV3(seq));
                                            entityP = Offset(entityP, entityOffset);
                                            
                                            if(SatisfiesEntityRequirements(server, referenceP, entityP, spawn, cellDim))
                                            {
                                                if(Valid(entities, entityP, spawn->minEntityDistance))
                                                {
                                                    AddToPoission(&entities, poissonPool, entityP);
                                                    if(spawn->occupiesTile)
                                                    {
                                                        MarkTileAsOccupied(server, entityP);
                                                    }
                                                    
                                                    AddEntityParams params = DefaultAddEntityParams();
                                                    if(spawn->attachedBrainEntity)
                                                    {
                                                        params.spawnFollowingEntity = true;
                                                        params.attachedEntityType = GetEntityType(server->assets, spawn->attachedBrainType);
                                                    }
                                                    
                                                    AddEntity(server, entityP, seq, GetEntityType(server->assets, spawn->type), params);
                                                    
                                                    break;
                                                }
                                            }
                                        }
                                    }
                                }
                                break;
                            }
                        }
                        break;
                    }
                }
            }
        }
    }
}


internal void Pick(ServerState* server, EntityID ID, EntityID targetID);
internal void GenerateEntity(ServerState* server, NewEntity* newEntity)
{
    Assert(IsValid(newEntity->definitionID));
    EntityDefinition* definition = GetData(server->assets, EntityDefinition, newEntity->definitionID);
    
    b32 essencesPresent = false;
    for(u16 essenceIndex = 0; essenceIndex < ArrayCount(newEntity->params.essences); ++essenceIndex)
    {
        if(newEntity->params.essences[essenceIndex] > 0)
        {
            essencesPresent = true;
            break;
        }
    }
    
    if(!essencesPresent)
    {
        for(u32 essenceIndex = 0; essenceIndex < definition->server.defaultEssenceCount; ++essenceIndex)
        {
            u16 essence = GetRandomEssence(&server->entropy);
            ++newEntity->params.essences[essence];
        }
    }
    
    EntityID ID = {};
    ServerEntityInitParams params = definition->server;
    params.P = newEntity->P;
    params.startingAcceleration = newEntity->params.acceleration;
    params.startingSpeed = newEntity->params.speed;
    definition->common.type = GetEntityType(newEntity->definitionID);
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
            }
        }
        
        QueueGameAccessConfirm(player, server->worldSeed, ID, false, newEntity->params.ghost);
        
        ZLayer* layer = server->layers + newEntity->P.chunkZ;
        QueueDayTime(player, layer->dayTimePhase);
    }
    
    if(newEntity->params.equipPlayerIndex > 0)
    {
        PlayerComponent* player = (PlayerComponent*) Get_(&server->PlayerComponent_, newEntity->params.equipPlayerIndex);
        
        if(IsValidID(player->ID))
        {
            Pick(server, player->ID, ID);
        }
    }
    
    if(newEntity->params.ghost)
    {
        DefaultComponent* def = GetComponent(server, ID, DefaultComponent);
        AddEntityFlags(def, EntityFlag_ghost);
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
            brain->targetID = ID;
        }
    }
    
    if(IsValidID(newEntity->params.spawnerID))
    {
		DefaultComponent* def = GetComponent(server, ID, DefaultComponent);
        def->spawnerID = newEntity->params.spawnerID;
    }
    
    if(HasComponent(ID, BrainComponent))
    {
        BrainComponent* brain = GetComponent(server, ID, BrainComponent);
        brain->homeP = newEntity->P;
        brain->reachableMap = GetComponent(server, ID, ReachableMapComponent);
    }
    
    if(HasComponent(ID, TempEntityComponent) && newEntity->params.timeToLive > 0)
    {
        TempEntityComponent* temp = GetComponent(server, ID, TempEntityComponent);
        temp->targetTime = newEntity->params.timeToLive;
    }
    
    
    EntityType portal = GetEntityType(server->assets, "default", "portal");
    if(AreEqual(GetEntityType(newEntity->definitionID), portal))
    {
        server->portalPositions[newEntity->P.chunkZ] = newEntity->P;
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
                r32 cellDim = Min(WORLD_SIDE, spawner->cellDim);
                u32 cellCount = TruncateReal32ToU32(WORLD_SIDE / cellDim);
                
                u32 cellX = RandomChoice(&server->entropy, cellCount);
                u32 cellY = RandomChoice(&server->entropy, cellCount);
                i16 Z = 0;
                UniversePos P = BuildPFromSpawnerGrid(cellDim, cellX, cellY, Z);
                
                PoissonP* entities = 0;
                PoissonP* clusters = 0;
                MemoryPool poissonPool = {};
                
                TriggerSpawnerInCell(server, entities, clusters, &poissonPool, spawner, P, &server->entropy, cellDim);
                Clear(&poissonPool);
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
            
            SetComponent(server, ID, PlayerComponent, 0);
        }
		
        if(HasComponent(ID, EquipmentComponent))
        {
            EquipmentComponent* equipment = GetComponent(server, ID, EquipmentComponent);
            for(u32 slotIndex = 0; slotIndex < ArrayCount(equipment->slots); ++slotIndex)
            {
                InventorySlot* slot = equipment->slots + slotIndex;
                EntityID slotID = GetBoundedID(slot);
                if(IsValidID(slotID))
                {
                    FreeArchetype(server, slotID);
                    SetBoundedID(slot, {});
                }
            }
        }
        
        if(HasComponent(ID, UsingComponent))
        {
            UsingComponent* equipped = GetComponent(server, ID, UsingComponent);
            
            EntityID draggingID = GetBoundedID(equipped->draggingID);
            if(IsValidID(draggingID))
            {
                FreeArchetype(server, draggingID);
            }
            
            for(u32 slotIndex = 0; slotIndex < ArrayCount(equipped->slots); ++slotIndex)
            {
                InventorySlot* slot = equipped->slots + slotIndex;
                EntityID slotID = GetBoundedID(slot);
                if(IsValidID(slotID))
                {
                    FreeArchetype(server, slotID);
                    SetBoundedID(slot, {});
                }
            }
        }
        
        if(HasComponent(ID, ContainerComponent))
        {
            ContainerComponent* container = GetComponent(server, ID, ContainerComponent);
            for(u32 storedIndex = 0; storedIndex < ArrayCount(container->storedObjects); ++storedIndex)
            {
                InventorySlot* slot = container->storedObjects + storedIndex;
                EntityID storedID = GetBoundedID(slot);
                if(IsValidID(storedID))
                {
                    FreeArchetype(server, storedID);
                    SetBoundedID(slot, {});
                }
            }
            
            for(u32 usingIndex = 0; usingIndex < ArrayCount(container->usingObjects); ++usingIndex)
            {
                InventorySlot* slot = container->usingObjects + usingIndex;
                EntityID usingID = GetBoundedID(slot);
                if(IsValidID(usingID))
                {
                    FreeArchetype(server, usingID);
                    SetBoundedID(slot, {});
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
        
        Clear(&server->chunkPool);
        server->maxDeepness = (i16) generator->maxDeepness;
        server->layers = PushArray(&server->chunkPool, ZLayer, generator->maxDeepness);
        
        for(i16 chunkZ = 0; chunkZ < server->maxDeepness; ++chunkZ)
        {
            ZLayer* layer = server->layers + chunkZ;
            
            if(chunkZ == 0)
            {
                layer->dayTimeTime = 0;
                layer->hasNight = false;
                layer->nightSpeed = 0;
                layer->dayTimePhase = DayTime_Day;
            }
            else
            {
                layer->dayTimeTime = RandomUni(&generatorSeq) * DAYPHASE_DURATION;
                layer->hasNight = true;
                layer->nightSpeed = 1.0f / chunkZ;
                layer->dayTimePhase = SafeTruncateToU16(RandomChoice(&generatorSeq, Count_DayTime));
            }
        }
        
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
            r32 cellDim = spawner->cellDim;
            cellDim = Min(cellDim, WORLD_SIDE);
            u32 cellCount = TruncateReal32ToU32(WORLD_SIDE / cellDim);
            for(i16 chunkZ = 0; chunkZ < (i16) server->maxDeepness; ++chunkZ)
            {
                PoissonP* entities = 0;
                PoissonP* clusters = 0;
                MemoryPool poissonPool = {};
                
                for(u32 Y = 0; Y < cellCount; ++Y)
                {
                    for(u32 X = 0; X < cellCount; ++X)
                    {
                        if(RandomUni(&seq) <= spawner->percentageOfStartingCells)
                        {
                            UniversePos P = BuildPFromSpawnerGrid(cellDim, X, Y, chunkZ);
                            TriggerSpawnerInCell(server, entities, clusters, &poissonPool, spawner, P, &seq, cellDim);
                        }
                    }
                }
                
                Clear(&poissonPool);
            }
        }
        Clear(&tempPool);
    }
}

internal UniversePos FindWalkablePStartingFrom(ServerState* server, UniversePos P, u32 stepCount)
{
    UniversePos result = P;
    u32 destStep = stepCount;
    Vec3 directions[4] = 
    {
        V3(VOXEL_SIZE, 0, 0),
        V3(-VOXEL_SIZE, 0, 0),
        V3(0, VOXEL_SIZE, 0),
        V3(0, -VOXEL_SIZE, 0)
    };
    
    for(u32 stepIndex = 0; stepIndex < destStep; ++stepIndex)
    {
        Vec3 direction = directions[RandomChoice(&server->entropy, ArrayCount(directions))];
        UniversePos newP = Offset(result, direction);
        if(PositionInsideWorld(&newP))
        {
            WorldTile* tile = GetTile(server, newP);
            if(!IsValid(tile->underSeaLevelFluid))
            {
                result = newP;
            }
        }
    }
    
    return result;
}

internal UniversePos FindPlayerStartingP(ServerState* server, u16 chunkZ)
{
    Assert(chunkZ < ArrayCount(server->portalPositions));
    UniversePos P = server->portalPositions[chunkZ];
#if FORGIVENESS_INTERNAL
    u32 stepCount = 4;
#else
    u32 stepCount = STEP_FROM_PORTAL;
#endif
    UniversePos result = FindWalkablePStartingFrom(server, P, stepCount);
    return result;
}
#endif