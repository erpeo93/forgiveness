
#define _GET_NTH_ARG(_1, _2, _3, _4, _5, N, ...) N
#define _fe_0(_call, ...)
#define _fe_1(_call, x) _call(x)
#define _fe_2(_call, x, ...) _call(x) _fe_1(_call, __VA_ARGS__)
#define _fe_3(_call, x, ...) _call(x) _fe_2(_call, __VA_ARGS__)
#define _fe_4(_call, x, ...) _call(x) _fe_3(_call, __VA_ARGS__)

#define CALL_MACRO_X_FOR_EACH(x, ...) \
_GET_NTH_ARG("ignored", ##__VA_ARGS__, _fe_4, _fe_3, _fe_2, _fe_1, _fe_0)(x, ##__VA_ARGS__)
#define BUCKET_DISTR(type, ...) BucketDistr({ CALL_MACRO_X_FOR_EACH(PRINT_BUCKET, type) })
#if 0
BucketDistr({ Tree_Palm, 30, Tree_Oak, 30, Tree_Pine, 40 });
#endif







inline RandomSequence GetChunkSeed(i32 worldX, i32 worldY, i32 universeX, i32 universeY)
{
    Assert(universeX >= 0);
    Assert(universeX < UNIVERSE_DIM);
    Assert(universeY >= 0);
    Assert(universeY < UNIVERSE_DIM);
    
    RandomSequence result = Seed(worldX * 17 + worldY * 19 + universeX * 13 + universeY + 14);
    return result;
}


inline r32 NormalizedNoise(r32 dx, r32 dy, r32 frequency, u32 seed)
{
    r32 noiseValue  = noise(dx * frequency, dy * frequency, 0.0f, seed);
    r32 result = (noiseValue + 1.0f) * 0.5f;
    return result;
}

inline r32 Evaluate(r32 dx, r32 dy, NoiseParams params, u32 seed)
{
    r32 total = 0;
    r32 maxValue = 0;
    r32 amplitude = params.amplitude;
    r32 frequency = params.frequency;
    
    Assert(params.octaves > 0);
    for(u32 octave = 0; octave < params.octaves; ++octave)
    {
        total +=  (NormalizedNoise(dx, dy, frequency, seed) + params.offset) * amplitude;
        maxValue += amplitude;
        
        amplitude *= params.persistance;
        frequency *= 2.0f;
    }
    
    r32 result = (total / maxValue) * params.amplitude;
    return result;
}

inline r32 Evaluate(GenerationMinMax minMax, r32 lerping)
{
    Assert(minMax.min <= minMax.max);
    r32 result = Lerp(minMax.min, lerping, minMax.max);
    return result;
}

inline GenerationBucket* AddEmptyBucket(Selector* selector, GenerationBucketType type, r32 bucketMark)
{
    GenerationBucket* result = 0;
    if(!selector->type)
    {
        selector->type = type;
    }
    Assert(selector->type == type);
    
    if(selector->bucketCount < ArrayCount(selector->buckets))
    {
        result = selector->buckets + selector->bucketCount++;
        result->referencePoint = bucketMark;
    }
    
    return result;
}

inline void AddBucket(Selector* selector, r32 point, NoiseParams params)
{
    GenerationBucket* bucket = AddEmptyBucket(selector, Bucket_Noise, point);
    if(bucket)
    {
        bucket->params = params;
    }
}

inline void AddBucket(Selector* selector, r32 point, GenerationMinMax minMax)
{
    GenerationBucket* bucket = AddEmptyBucket(selector, Bucket_MinMax, point);
    if(bucket)
    {
        bucket->minMax = minMax;
    }
}

inline void AddBucket(Selector* selector, r32 point, r32 fixed)
{
    GenerationBucket* bucket = AddEmptyBucket(selector, Bucket_Fixed, point);
    if(bucket)
    {
        bucket->fixed = fixed;
    }
}

inline void AddBucket(Selector* selector, r32 point, u32 taxonomy)
{
    AddBucket(selector, point, (r32) taxonomy);
}

inline Selector* AddSelectorForDryness(BiomePyramid* pyramid, r32 threesold)
{
    Assert(pyramid->rowCount < ArrayCount(pyramid->temperatureSelectors));
    u32 column = pyramid->rowCount++;
    AddBucket(&pyramid->drySelector, threesold, (r32) column); 
    Selector* result = pyramid->temperatureSelectors + column;
    
    return result;
}

inline r32 Select(Selector* selector, r32 dx, r32 dy, r32 selectionValue, u32 seed)
{
    r32 result = 0;
    if(selector->bucketCount > 0)
    {
        GenerationBucket* previousBucket = selector->buckets + 0;
        GenerationBucket* currentBucket = 0;
        r32 bucketLerping = 0;
        
        b32 bucketFound = false;
        r32 previousRef = 0;
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
            GenerationBucket* lastBucket = currentBucket;
            Assert(selectionValue >= lastBucket->referencePoint);
            previousBucket = lastBucket;
        }
        
        switch(selector->type)
        {
            case Bucket_Noise:
            {
                r32 prev = Evaluate(dx, dy, previousBucket->params, seed);
                r32 current = Evaluate(dx, dy, currentBucket->params, seed);
                
                result = Lerp(prev, bucketLerping, current);
                
            } break;
            
            case Bucket_MinMax:
            {
                Assert(dx == dy);
                
                r32 prev = Evaluate(previousBucket->minMax, dx);
                r32 current = Evaluate(currentBucket->minMax, dx);
                
                result = Lerp(prev, bucketLerping, current);
                
            } break;
            
            case Bucket_Fixed:
            {
                result = currentBucket->fixed;
            } break;
            InvalidDefaultCase;
        }
    }
    else
    {
        InvalidCodePath;
    }
    
    
    return result;
}

inline u32 SelectFromBiomePyramid(BiomePyramid* pyramid, r32 precipitationLevel, r32 temperature, u32 seed)
{
    r32 row = Select(&pyramid->drySelector, 0, 0, precipitationLevel, seed);
    Assert((u32) row < pyramid->rowCount);
    
    Selector* temperatureSelector = pyramid->temperatureSelectors + (u32) row;
    r32 biome = Select(temperatureSelector, 0, 0, temperature, seed);
    
    u32 result = (u32) biome;
    return result;
}

inline WorldTile GenerateTile(TaxonomyTable* table, WorldGenerator* generator, r32 tileNormX, r32 tileNormY, u32 seed)
{
    WorldTile result = {};
    
    r32 tileLandscape = Evaluate(tileNormX, tileNormY, generator->landscapeNoise, seed);
    r32 finalHeight = Select(&generator->landscapeSelect, tileNormX, tileNormY, tileLandscape, seed);
    
    
    r32 tilePrecipitation = Evaluate(tileNormX, tileNormY, generator->precipitationNoise, seed);
    r32 temperatureTurbolence = Evaluate(tileNormX, tileNormY, generator->temperatureNoise, seed);
    r32 tileTemperature = Select(&generator->temperatureSelect, temperatureTurbolence, temperatureTurbolence, tileLandscape, seed);
    
    
    u32 biome = SelectFromBiomePyramid(&generator->biomePyramid, tilePrecipitation, tileTemperature, seed);
#if 0    
    u32 terracesCount = 256;
    r32 terracesStep = maxHeight / terracesCount;
    
    u32 testTerraces = (u32) ((finalHeight / maxHeight) * terracesCount);
    finalHeight = testTerraces * terracesStep;
#endif
    finalHeight = 0;
    
    
    r32 elevation = Evaluate(tileNormX, tileNormY, generator->elevationNoise, seed);
    
    r32 distanceFromCenter = Clamp01(Length(V2(tileNormX, tileNormY) - V2(0.5f, 0.5f)) / 0.47f);
    elevation = (1 + elevation - distanceFromCenter) / 2;
    
    elevation = Pow(elevation, generator->elevationPower);
    Assert(Normalized(elevation));
    result.waterLevel = elevation;
    
    
    if(elevation >= (WATER_LEVEL - generator->beachThreesold) && elevation < (WATER_LEVEL + generator->beachThreesold) && generator->beachTaxonomy)
    {
        biome = GetSlotForTaxonomy(table, generator->beachTaxonomy)->taxonomy;
    }
    
    
    result.height = finalHeight;
    result.taxonomy = biome;
    Assert(result.taxonomy);
    
    
#ifndef FORG_SERVER
    TaxonomySlot* tileSlot = GetSlotForTaxonomy(table, biome);
    result.layoutNoise = Evaluate(tileNormX, tileNormY, tileSlot->tileNoise, seed);
    
    result.baseColor = tileSlot->tileColor; 
    result.colorDelta = tileSlot->colorDelta;
    result.borderColor = tileSlot->tileBorderColor;
    result.chunkynessSame = tileSlot->chunkynessWithSame;
    result.chunkynessOther = tileSlot->chunkynessWithOther;
    result.colorRandomness = tileSlot->colorRandomness;
    
    result.waterPhase = 0;
    result.movingNegative = false;
    result.waterSine = 0;
    
    
    result.waterSeq = Seed((i32) (tileNormX * 1000.24f) + (i32)(tileNormY * 1223424.0f));
#endif
    
    return result;
}


inline WorldTile OceanTile(TaxonomyTable* table, WorldGenerator* generator, r32 tileNormX, r32 tileNormY, u32 seed)
{
    Assert(generator);
    
    WorldTile result = {};
    
    r32 finalHeight = 0;
    u32 biome = GetSlotForTaxonomy(table, generator->beachTaxonomy)->taxonomy;
    
    result.height = finalHeight;
    result.waterLevel = 0;
    result.taxonomy = biome;
    Assert(result.taxonomy);
    
    
#ifndef FORG_SERVER
    TaxonomySlot* tileSlot = GetSlotForTaxonomy(table, biome);
    result.layoutNoise = Evaluate(tileNormX, tileNormY, tileSlot->tileNoise, seed);
    
    result.baseColor = tileSlot->tileColor; 
    result.colorDelta = tileSlot->colorDelta;
    result.borderColor = tileSlot->tileBorderColor;
    result.chunkynessSame = tileSlot->chunkynessWithSame;
    result.chunkynessOther = tileSlot->chunkynessWithOther;
    result.colorRandomness = tileSlot->colorRandomness;
    
    result.waterPhase = 0;
    result.movingNegative = false;
    result.waterSine = 0;
    
    
    result.waterSeq = Seed((i32) (tileNormX * 1000.24f) + (i32)(tileNormY * 1223424.0f));
#endif
    
    return result;
}


internal void BuildChunk(TaxonomyTable* table, WorldGenerator* generator, WorldChunk* chunk, i32 chunkX, i32 chunkY, u32 seed)
{
    Assert(generator);
    
    Assert(CHUNK_DIM % 4 == 0);
    
    chunk->initialized = true;
    chunk->worldX = chunkX;
    chunk->worldY = chunkY;
    
    i32 lateralChunkSpan = SERVER_REGION_SPAN * SIM_REGION_CHUNK_SPAN;
    
    chunkX = Wrap(0, chunkX, lateralChunkSpan);
    chunkY = Wrap(0, chunkY, lateralChunkSpan);
    
    u32 baseTileX = chunkX * CHUNK_DIM;
    u32 baseTileY = chunkY * CHUNK_DIM;
    
    b32 chunkOutsideWorld = ChunkOutsideWorld(lateralChunkSpan, chunkX, chunkY);
    
    
    for(u8 tileY = 0; tileY < CHUNK_DIM; ++tileY)
    {
        for(u8 tileX = 0; tileX < CHUNK_DIM; ++tileX)
        {
            u32 realTileX = baseTileX + tileX;
            u32 realTileY = baseTileY + tileY;
            
            // NOTE(Leonardo): normalized values
            r32 tileNormX = (r32) realTileX / (lateralChunkSpan * CHUNK_DIM);
            r32 tileNormY = (r32) realTileY / (lateralChunkSpan * CHUNK_DIM);
            
            Assert(Normalized(tileNormX));
            Assert(Normalized(tileNormY));
            
            if(chunkOutsideWorld)
            {
                chunk->tiles[tileY][tileX] = OceanTile(table, generator, tileNormX, tileNormY, seed);
            }
            else
            {
                chunk->tiles[tileY][tileX] = GenerateTile(table, generator, tileNormX, tileNormY, seed);
            }
        }
    }
}
