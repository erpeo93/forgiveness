internal r32 BilateralNoise(r32 dx, r32 dy, r32 frequency, u32 seed)
{
    r32 result  = noise(dx * frequency, dy * frequency, 0.0f, seed);
    return result;
}

inline r32 UnilateralNoise(r32 dx, r32 dy, r32 frequency, u32 seed)
{
    r32 result = BilateralToUnilateral(BilateralNoise(dx, dy, frequency, seed));
    return result;
}

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

internal WorldTile NullTile(world_generator* generator)
{
    WorldTile result = {};
    result.elevation = minHeight;
    
#if 0    
    result.asset = definition->asset;
    result.property = definition->property;
#endif
    result.color = V4(1, 0, 0, 1);
    
    return result;
}

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

inline WorldTile GenerateTile(Assets* assets, world_generator* generator, r32 tileNormX, r32 tileNormY, r32 tileNormZ, RandomSequence* seq, u32 seed, r32 totalRunningTime)
{
    WorldTile result = {};
    
    // NOTE(Leonardo): elevation
    result.elevation = GetTileElevation(generator, tileNormX, tileNormY, tileNormZ, seq, seed);
    r32 temperatureNoise = Evaluate(tileNormX, tileNormY, generator->temperatureNoise, seed);
    r32 temperatureDegrees = Select(&generator->temperatureSelect, temperatureNoise, temperatureNoise, result.elevation, seed, false);
    
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
        result.asset = definition->asset;
        result.property = definition->property;
        result.color = definition->color;
    }
    
    
#ifndef FORG_SERVER
    result.entropy = Seed((i32) (tileNormX * 1000.24f) + (i32)(tileNormY * 1223424.0f));
    result.waterRandomization = RandomUni(seq);
    result.movingNegative = false;
    result.waterTime = totalRunningTime + Length(V2(tileNormX, tileNormY) - V2(0.5f, 0.5f));
    result.waterSeed = GetNextUInt32(&result.entropy);
    result.blueNoise = 0;
    result.alphaNoise = 0;
#endif
    
    return result;
}

internal RandomSequence GetChunkSeed(u32 chunkX, u32 chunkY, u32 worldSeed)
{
    RandomSequence result = Seed(chunkX * chunkY * worldSeed);
    return result;
}

internal void BuildChunk(Assets* assets, MemoryPool* pool, world_generator* generator, WorldChunk* chunk, i32 chunkX, i32 chunkY, i32 chunkZ, u32 seed, r32 totalRunningTime)
{
    RandomSequence seq = GetChunkSeed(chunkX, chunkY, seed);
    RandomSequence seqTest = seq;
    
#ifndef FORG_SERVER
    chunk->initialized = true;
    chunk->worldX = chunkX;
    chunk->worldY = chunkY;
    chunk->worldZ = chunkZ;
#endif
    
    u32 maxTile = (WORLD_CHUNK_SPAN  + 2 * WORLD_CHUNK_APRON)* CHUNK_DIM;
    
    i32 normalizedChunkX = chunkX + WORLD_CHUNK_APRON;
    i32 normalizedChunkY = chunkY + WORLD_CHUNK_APRON;
    
    chunk->tiles = 0;
    b32 buildTiles = false;
    
    r32 chunkNormZ = 0;
    
    if(generator->maxDeepness > 1)
    {
        chunkNormZ = (r32) chunkZ / (r32) (generator->maxDeepness - 1);
    }
    
    WorldTile nullTile = NullTile(generator);
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
            if(elevation != nullTile.elevation)
            {
                buildTiles = true;
            }
        }
    }
    
    if(buildTiles)
    {
        chunk->tiles = PushArray(pool, WorldTile, CHUNK_DIM * CHUNK_DIM, NoClear());
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
                
                *tile = GenerateTile(assets, generator, tileNormX, tileNormY, tileNormZ, &seq, seed, totalRunningTime);
            }
        }
    }
}