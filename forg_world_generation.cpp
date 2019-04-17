
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







internal RandomSequence GetChunkSeed(i32 worldX, i32 worldY, i32 universeX, i32 universeY)
{
    Assert(universeX >= 0);
    Assert(universeX < UNIVERSE_DIM);
    Assert(universeY >= 0);
    Assert(universeY < UNIVERSE_DIM);
    
    RandomSequence result = Seed(worldX * 17 + worldY * 19 + universeX * 13 + universeY + 14);
    return result;
}


r32 NormalizedNoise(r32 dx, r32 dy, r32 frequency, u32 seed)
{
    r32 noiseValue  = noise(dx * frequency, dy * frequency, 0.0f, seed);
    r32 result = (noiseValue + 1.0f) * 0.5f;
    return result;
}

inline r32 Evaluate(r32 dx, r32 dy, NoiseParams params)
{
    r32 total = 0;
    r32 maxValue = 0;
    r32 amplitude = params.amplitude;
    r32 frequency = params.frequency;
    
    Assert(params.octaves > 0);
    for(u32 octave = 0; octave < params.octaves; ++octave)
    {
        total +=  (NormalizedNoise(dx, dy, frequency, params.seed) + params.offset) * amplitude;
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
    if(!selector->type)
    {
        selector->type = type;
    }
    Assert(selector->type == type);
    
    Assert(selector->bucketCount < ArrayCount(selector->buckets));
    GenerationBucket* result = selector->buckets + selector->bucketCount++;
    result->referencePoint = bucketMark;
    
    return result;
}

inline void AddBucket(Selector* selector, r32 point, NoiseParams params)
{
    GenerationBucket* bucket = AddEmptyBucket(selector, Bucket_Noise, point);
    bucket->params = params;
}

inline void AddBucket(Selector* selector, r32 point, GenerationMinMax minMax)
{
    GenerationBucket* bucket = AddEmptyBucket(selector, Bucket_MinMax, point);
    bucket->minMax = minMax;
}

inline void AddBucket(Selector* selector, r32 point, r32 fixed)
{
    GenerationBucket* bucket = AddEmptyBucket(selector, Bucket_Fixed, point);
    bucket->fixed = fixed;
}

inline void AddBucket(Selector* selector, r32 point, TaxonomyTable* table, char* taxonomyName)
{
    TaxonomySlot* slot = NORUNTIMEGetTaxonomySlotByName(table, taxonomyName);
    AddBucket(selector, point, (r32) slot->taxonomy);
}

inline Selector* AddSelectorForDryness(BiomePyramid* pyramid, r32 threesold)
{
    Assert(pyramid->rowCount < ArrayCount(pyramid->temperatureSelectors));
    u32 column = pyramid->rowCount++;
    AddBucket(&pyramid->drySelector, threesold, (r32) column); 
    Selector* result = pyramid->temperatureSelectors + column;
    
    return result;
}

inline r32 Select(Selector* selector, r32 dx, r32 dy, r32 selectionValue)
{
    Assert(selector->bucketCount > 0);
    r32 result = 0;
    
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
            r32 prev = Evaluate(dx, dy, previousBucket->params);
            r32 current = Evaluate(dx, dy, currentBucket->params);
            
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
    
    return result;
}


inline void InitializeWorldGenerator(TaxonomyTable* table, WorldGenerator* generator, i32 universeX, i32 universeY)
{
    generator->lateralChunkSpan = SERVER_REGION_SPAN * SIM_REGION_CHUNK_SPAN;
    generator->maxHeight = 30.0f * VOXEL_SIZE;
    
    universeX = Wrap(0, universeX, UNIVERSE_DIM);
    universeY = Wrap(0, universeY, UNIVERSE_DIM);
    
    generator->universeX = universeX;
    generator->universeY = universeY;
    
    RandomSequence worldSequence = Seed(universeX * universeY);
    // NOTE(Leonardo): these are the different "landscapes" we have
    NoiseParams lowLandscape = NoisePar(1.0f, 1, 0.0f, 3.0f * VOXEL_SIZE, &worldSequence);
    
    r32 lowThreesold = 0.5f;
    
    generator->landscapeSelect = {};
    AddBucket(&generator->landscapeSelect, lowThreesold, lowLandscape);
    
    generator->landscapeNoise = NoisePar(6.0f, 1, 0.0f, 1.0f, &worldSequence);
    generator->temperatureNoise = NoisePar(64.0f, 1, 0.0f, 1.0f, &worldSequence);
    generator->drynessNoise = NoisePar(3.0f, 1, 0.0f, 1.0f, &worldSequence);
    generator->tileLayoutNoise = NoisePar(10.0f, 1, 0.0f, 1.0f, &worldSequence);
    
    
    generator->temperatureSelect = {};
    AddBucket(&generator->temperatureSelect, lowThreesold, MinMax(13.0f, 22.0f)); 
    
    generator->biomePyramid = {};
    Selector* lowDryness = AddSelectorForDryness(&generator->biomePyramid, 0.5f);
    
    AddBucket(lowDryness, 15.0f, table, "dirt");
    AddBucket(lowDryness, 17.0f, table, "forest");
    AddBucket(lowDryness, 20.0f, table, "grassTile");
}

inline u32 SelectFromBiomePyramid(BiomePyramid* pyramid, r32 dryness, r32 temperature)
{
    Assert(Normalized(dryness));
    r32 row = Select(&pyramid->drySelector, 0, 0, dryness);
    Assert((u32) row < pyramid->rowCount);
    
    Selector* temperatureSelector = pyramid->temperatureSelectors + (u32) row;
    r32 biome = Select(temperatureSelector, 0, 0, temperature);
    
    u32 result = (u32) biome;
    return result;
}

inline TileGenerationData GenerateTile(WorldGenerator* generator, r32 tileNormX, r32 tileNormY)
{
    TileGenerationData result = {};
    
    r32 tileLandscape = Evaluate(tileNormX, tileNormY, generator->landscapeNoise);
    r32 finalHeight = Select(&generator->landscapeSelect, tileNormX, tileNormY, tileLandscape);
    Assert(finalHeight <= generator->maxHeight);
    
    
    r32 tileDryness = Evaluate(tileNormX, tileNormY, generator->drynessNoise);
    r32 temperatureTurbolence = Evaluate(tileNormX, tileNormY, generator->temperatureNoise);
    r32 tileTemperature = Select(&generator->temperatureSelect, temperatureTurbolence, temperatureTurbolence, tileLandscape);
    
    
    u32 biome = SelectFromBiomePyramid(&generator->biomePyramid, tileDryness, tileTemperature);
#if 0    
    u32 terracesCount = 256;
    r32 terracesStep = maxHeight / terracesCount;
    
    u32 testTerraces = (u32) ((finalHeight / maxHeight) * terracesCount);
    finalHeight = testTerraces * terracesStep;
#endif
    finalHeight = 0;
    
    
    result.height = finalHeight;
    result.biomeTaxonomy = biome;
    Assert(result.biomeTaxonomy);
    
    result.layoutNoise = Evaluate(tileNormX, tileNormY, generator->tileLayoutNoise);
    
    return result;
}


internal void BuildChunk(WorldGenerator* generator, WorldChunk* chunk, i32 chunkX, i32 chunkY)
{
    Assert(CHUNK_DIM % 4 == 0);
    
    chunk->initialized = true;
    chunk->worldX = chunkX;
    chunk->worldY = chunkY;
    
    i32 universeX = generator->universeX;
    i32 universeY = generator->universeY;
    chunkX = Wrap(0, chunkX, generator->lateralChunkSpan, &universeX);
    chunkY = Wrap(0, chunkY, generator->lateralChunkSpan, &universeY);
    
    
    //r32 chunkXInTiles = ((chunkX + 0.5f) * CHUNK_DIM) / (lateralChunkSpan * CHUNK_DIM);
    //r32 chunkYInTiles = ((chunkY + 0.5f) * CHUNK_DIM) / (lateralChunkSpan * CHUNK_DIM);
    
#if 0    
    r32 halfWorld = (r32) lateralChunkSpan * 0.5f;
    r32 halfWorldInTile = halfWorld * CHUNK_DIM;
    
    // NOTE(Leonardo): normalized values
    r32 chunkNormX = (r32) chunkX / lateralChunkSpan;
    r32 chunkNormY = (r32) chunkY / lateralChunkSpan;
    
    Assert(Normalized(chunkNormX));
    Assert(Normalized(chunkNormY));
    
    r32 d = 2.0f * Max(Abs(chunkNormX), Abs(chunkNormY));
    r32 dropOffCoeff = 1.0f - 0.79f * powf(d, 0.76f);
    // NOTE(Leonardo): 0.1 minimum 0.15 max?
    height = (height + 0.14f) * dropOffCoeff;
    height = Max(height, 0.0f);
#endif
    
    
    u32 baseTileX = chunkX * CHUNK_DIM;
    u32 baseTileY = chunkY * CHUNK_DIM;
    
    for(u8 tileY = 0; tileY < CHUNK_DIM; ++tileY)
    {
        for(u8 tileX = 0; tileX < CHUNK_DIM; ++tileX)
        {
            u32 realTileX = baseTileX + tileX;
            u32 realTileY = baseTileY + tileY;
            
            // NOTE(Leonardo): normalized values
            r32 tileNormX = (r32) realTileX / (generator->lateralChunkSpan * CHUNK_DIM);
            r32 tileNormY = (r32) realTileY / (generator->lateralChunkSpan * CHUNK_DIM);
            
            Assert(Normalized(tileNormX));
            Assert(Normalized(tileNormY));
            
            chunk->tileData[tileY][tileX] = GenerateTile(generator, tileNormX, tileNormY);
        }
    }
}

#undef TARGET
#undef ACTOR