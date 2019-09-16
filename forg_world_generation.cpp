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

inline GameProperty SelectFromBiomePyramid(BiomePyramid* pyramid, r32 precipitationLevel, r32 temperature, u32 seed)
{
    GameProperty result = {};
    r32 row = Select(&pyramid->drySelector, 0, 0, precipitationLevel, seed);
    if((u32) row < pyramid->rowCount)
    {
        PropertySelector* temperatureSelector = pyramid->temperatureSelectors + (u32) row;
        result = Select(temperatureSelector, temperature, seed);
    }
    
    return result;
}

global_variable r32 minHeight = -100.0f;
global_variable r32 maxHeight = 1000.0f;

inline WorldTile GenerateTile(Assets* assets, world_generator* generator, r32 tileNormX, r32 tileNormY, RandomSequence* seq, u32 seed)
{
    WorldTile result = {};
    
    // NOTE(Leonardo): elevation
    r32 landscape = Evaluate(tileNormX, tileNormY, generator->landscapeNoise, seed);
    
    r32 waterMargin = Clamp01(generator->waterSafetyMargin);
    r32 standardElevation = minHeight;
    if(tileNormX < waterMargin || tileNormY < waterMargin || 
       tileNormX >= (1.0f - waterMargin) || tileNormY >= (1.0f - waterMargin))
    {
    }
    else
    {
        standardElevation = Select(&generator->landscapeSelect, tileNormX, tileNormY, landscape, seed);
        // NOTE(Leonardo): modify elevation to match out island shapes
        r32 normalizedElevation = Clamp01MapToRange(minHeight, standardElevation, maxHeight);
        r32 distanceFromCenter = Length(V2(tileNormX, tileNormY) - V2(0.5f, 0.5f));
        r32 elevationCoeff = Evaluate(tileNormX, tileNormY, generator->elevationNoise, seed);
        normalizedElevation = (generator->elevationNormOffset + normalizedElevation) - distanceFromCenter * elevationCoeff;
        normalizedElevation = Clamp01(normalizedElevation);
        normalizedElevation = Pow(normalizedElevation, generator->elevationPower);
        standardElevation = Lerp(minHeight, normalizedElevation, maxHeight);
    }
    
    result.elevation = standardElevation;
    r32 temperatureNoise = Evaluate(tileNormX, tileNormY, generator->temperatureNoise, seed);
    r32 temperatureDegrees = Select(&generator->temperatureSelect, temperatureNoise, temperatureNoise, standardElevation, seed, false);
    
    // NOTE(Leonardo): precipitations
    r32 annualMMPrecipitation = Evaluate(tileNormX, tileNormY, generator->precipitationNoise, seed);
    
    GameProperty property = SelectFromBiomePyramid(&generator->biomePyramid, annualMMPrecipitation, temperatureDegrees, seed);
    
    GameProperties properties = {};
    properties.properties[0] = property;
    AssetID ID = QueryDataFiles(assets, tile_definition, 0, seq, &properties);
    if(IsValid(ID))
    {
        tile_definition* definition = GetData(assets, tile_definition, ID);
        Assert(definition);
        result.asset = definition->asset;
        result.property = definition->property;
        result.color = definition->color;
    }
    
    
    
    result.waterPhase = 0;
    result.movingNegative = false;
    result.waterSine = 0;
    result.waterSeq = Seed((i32) (tileNormX * 1000.24f) + (i32)(tileNormY * 1223424.0f));
    
    return result;
}

internal RandomSequence GetChunkSeed(u32 chunkX, u32 chunkY, u32 worldSeed)
{
    RandomSequence result = Seed(chunkX * chunkY * worldSeed);
    return result;
}

internal void BuildChunk(Assets* assets, WorldChunk* chunk, i32 chunkX, i32 chunkY, u32 seed)
{
    RandomSequence generatorSeq = Seed(seed);
    GameProperties properties = {};
    AssetID ID = QueryDataFiles(assets, world_generator, 0, &generatorSeq, &properties);
    if(IsValid(ID))
    {
        chunk->initialized = true;
        chunk->worldX = chunkX;
        chunk->worldY = chunkY;
        
        RandomSequence seq = GetChunkSeed(chunk->worldX, chunk->worldY, seed);
        world_generator* generator = GetData(assets, world_generator, ID);
        
        u32 maxTile = (WORLD_CHUNK_SPAN  + 2 * WORLD_CHUNK_APRON)* CHUNK_DIM;
        
        
        i32 normalizedChunkX = chunkX + WORLD_CHUNK_APRON;
        i32 normalizedChunkY = chunkY + WORLD_CHUNK_APRON;
        
        for(u8 tileY = 0; tileY < CHUNK_DIM; ++tileY)
        {
            for(u8 tileX = 0; tileX < CHUNK_DIM; ++tileX)
            {
                u32 realTileX = normalizedChunkX * CHUNK_DIM + tileX;
                u32 realTileY = normalizedChunkY * CHUNK_DIM + tileY;
                
                // NOTE(Leonardo): normalized values
                r32 tileNormX = (r32) realTileX / maxTile;
                r32 tileNormY = (r32) realTileY / maxTile;
                
                Assert(Normalized(tileNormX));
                Assert(Normalized(tileNormY));
                
                chunk->tiles[tileY][tileX] = GenerateTile(assets, generator, tileNormX, tileNormY, &seq, seed);
            }
        }
    }
}