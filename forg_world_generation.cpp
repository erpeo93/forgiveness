
#define _GET_NTH_ARG(_1, _2, _3, _4, _5, N, ...) N
#define _fe_0(_call, ...)
#define _fe_1(_call, x) _call(x)
#define _fe_2(_call, x, ...) _call(x) _fe_1(_call, __VA_ARGS__)
#define _fe_3(_call, x, ...) _call(x) _fe_2(_call, __VA_ARGS__)
#define _fe_4(_call, x, ...) _call(x) _fe_3(_call, __VA_ARGS__)

#define CALL_MACRO_X_FOR_EACH(x, ...) \
_GET_NTH_ARG("ignored", ##__VA_ARGS__, _fe_4, _fe_3, _fe_2, _fe_1, _fe_0)(x, ##__VA_ARGS__)
#define BUCKET_DISTR( type, ... ) BucketDistr( { CALL_MACRO_X_FOR_EACH( PRINT_BUCKET, type ) } )
#if 0
BucketDistr( { Tree_Palm, 30, Tree_Oak, 30, Tree_Pine, 40 } );
#endif







internal RandomSequence GetChunkSeed( i32 worldX, i32 worldY, i32 universeX, i32 universeY )
{
    Assert( universeX >= 0 );
    Assert( universeX < UNIVERSE_DIM );
    Assert( universeY >= 0 );
    Assert( universeY < UNIVERSE_DIM );
    
    RandomSequence result = Seed( worldX * 17 + worldY * 19 + universeX * 13 + universeY + 14 );
    return result;
}

struct NoiseParams
{
    r32 frequency;
    u32 octaves;
    r32 persistance;
    r32 offset;
    r32 amplitude;
    u32 seed;
};

struct MinMaxChoice
{
    r32 min;
    r32 max;
};

enum ChoiceType
{
    Choice_Noise,
    Choice_MinMax,
    Choice_Fixed,
};


union Choice
{
    NoiseParams params;
    MinMaxChoice minMax;
    r32 fixed;
};

struct Selector
{
    r32 minValue;
    ChoiceType type;
    u32 choiceCount;
    r32 referencePoints[4];
    Choice choices[4];
};

struct BiomePyramid
{
    // NOTE(Leonardo): vertical!
    Selector drySelector;
    
    // NOTE(Leonardo): horizontal, one for every "slice"
    u32 rowCount;
    Selector temperatureSelectors[4];
};

NoiseParams NoisePar( r32 frequency, u32 octaves, r32 offset, r32 amplitude, RandomSequence* seq, r32 persistance = 0.5f )
{
    NoiseParams result = {};
    result.frequency = frequency;
    result.octaves = octaves;
    result.persistance = persistance;
    result.offset = offset;
    result.amplitude = amplitude;
    result.seed = GetNextUInt32( seq );
    return result;
}

MinMaxChoice MinMax( r32 min, r32 max )
{
    MinMaxChoice result = {};
    result.min = min;
    result.max = max;
    
    return result;
}

r32 NormalizedNoise( r32 dx, r32 dy, r32 frequency, u32 seed )
{
    r32 noiseValue  = noise( dx * frequency, dy * frequency, 0.0f, seed );
    r32 result = ( noiseValue + 1.0f ) * 0.5f;
    return result;
}

inline r32 Evaluate( r32 dx, r32 dy, NoiseParams params )
{
    r32 total = 0;
    r32 maxValue = 0;
    r32 amplitude = params.amplitude;
    r32 frequency = params.frequency;
    
    Assert( params.octaves > 0 );
    for( u32 octave = 0; octave < params.octaves; ++octave )
    {
        total +=  ( NormalizedNoise( dx, dy, frequency, params.seed ) + params.offset ) * amplitude;
        maxValue += amplitude;
        
        amplitude *= params.persistance;
        frequency *= 2.0f;
    }
    
    r32 result = ( total / maxValue ) * params.amplitude;
    return result;
}

inline r32 Evaluate( MinMaxChoice choice, r32 lerping )
{
    Assert( choice.min <= choice.max );
    r32 result = Lerp( choice.min, lerping, choice.max );
    return result;
}

inline Choice* AddEmptyChoice( Selector* selector, ChoiceType type, r32 point )
{
    if( !selector->type )
    {
        selector->type = type;
    }
    Assert( selector->type == type );
    
    Assert( selector->choiceCount < ArrayCount( selector->referencePoints ) );
    u32 index = selector->choiceCount++;
    selector->referencePoints[index] = point;
    
    Choice* result = selector->choices + index;
    return result;
}

inline void AddChoice( Selector* selector, r32 point, NoiseParams params )
{
    Choice* choice = AddEmptyChoice( selector, Choice_Noise, point );
    choice->params = params;
}

inline void AddChoice( Selector* selector, r32 point, MinMaxChoice minMax )
{
    Choice* choice = AddEmptyChoice( selector, Choice_MinMax, point );
    choice->minMax = minMax;
}

inline void AddChoice( Selector* selector, r32 point, r32 fixed )
{
    Choice* choice = AddEmptyChoice( selector, Choice_Fixed, point );
    choice->fixed = fixed;
}

inline Selector* AddSelectorForDryness( BiomePyramid* pyramid, r32 threesold )
{
    Assert( pyramid->rowCount < ArrayCount( pyramid->temperatureSelectors ) );
    u32 column = pyramid->rowCount++;
    AddChoice( &pyramid->drySelector, threesold, ( r32 ) column ); 
    Selector* result = pyramid->temperatureSelectors + column;
    
    return result;
}

inline r32 Select( Selector* selector, r32 dx, r32 dy, r32 selectionValue )
{
    //Assert( selector->choiceCount > 1 );
    r32 result = 0;
    
    r32 min = 0;
    r32 max = selector->referencePoints[0];
    
    u32 pointIndexMin = 0;
    u32 pointIndexMax = 0;
    b32 minValid = false;
    
    if(selectionValue >= selector->referencePoints[0])
    {
        minValid = true;
        r32 currentLow = selector->referencePoints[0];
        for( u32 pointIndex = 1; pointIndex < selector->choiceCount; ++pointIndex )
        {
            if( selectionValue >= currentLow && selectionValue < selector->referencePoints[pointIndex] )
            {
                pointIndexMax = pointIndex;
                pointIndexMin = pointIndex - 1;
                Assert(pointIndexMin < selector->choiceCount);
                
                min = selector->referencePoints[pointIndexMin];
                max = selector->referencePoints[pointIndexMax];
                break;
            }
            
            currentLow = selector->referencePoints[pointIndex];
        }
    }
    
    r32 p1 = 0.0f;
    r32 p2 = 0.0f;
    switch( selector->type )
    {
        case Choice_Noise:
        {
            Choice* c2 = selector->choices + pointIndexMax;
            p2 = Evaluate( dx, dy, c2->params );
            
            if(minValid)
            {
                Choice* c1 = selector->choices + pointIndexMin;
                p1 = Evaluate( dx, dy, c1->params );
            }
            else
            {
                p1 = selector->minValue;
            }
            
            r32 lerping = Clamp01MapToRange( min, selectionValue, max );
            result = Lerp( p1, lerping, p2 );
            
        } break;
        
        case Choice_MinMax:
        {
            Assert( dx == dy );
            Choice* c2 = selector->choices + pointIndexMax;
            p2 = Evaluate( c2->minMax, dy );
            if(minValid)
            {
                Choice* c1 = selector->choices + pointIndexMin;
                p1 = Evaluate( dx, dy, c1->params );
            }
            else
            {
                p1 = selector->minValue;
            }
            
            
            r32 lerping = Clamp01MapToRange( min, selectionValue, max );
            result = Lerp( p1, lerping, p2 );
            
        } break;
        
        case Choice_Fixed:
        {
            Choice* c1 = selector->choices + pointIndexMax;
            result = c1->fixed;
        } break;
        InvalidDefaultCase;
    }
    
    return result;
}


r32 GetTemperature( Selector* selector, r32 landscape, r32 temperatureTurbolence )
{
    r32 result = Select( selector, temperatureTurbolence, temperatureTurbolence, landscape );
    return result;
}

inline u8 GetBiomeFor( BiomePyramid* pyramid, r32 dryness, r32 temperature )
{
    Assert( Normalized( dryness ) );
    r32 row = Select( &pyramid->drySelector, 0, 0, dryness );
    Assert( ( u32 ) row < pyramid->rowCount );
    
    Selector* temperatureSelector = pyramid->temperatureSelectors + ( u32 ) row;
    r32 biome = Select( temperatureSelector, 0, 0, temperature );
    
    Assert( ( u8 ) biome < Biome_count );
    u8 result = ( u8 ) biome;
    return result;
}


internal void BuildChunk( WorldChunk* chunk, i32 chunkX, i32 chunkY, i32 universeX, i32 universeY, u32 lateralChunkSpan )
{
    Assert(CHUNK_DIM % 4 == 0);
    
    chunk->initialized = true;
    chunk->worldX = chunkX;
    chunk->worldY = chunkY;
    chunkX = Wrap( 0, chunkX, lateralChunkSpan, &universeX );
    chunkY = Wrap( 0, chunkY, lateralChunkSpan, &universeY );
    
    universeX = Wrap( 0, universeX, UNIVERSE_DIM );
    universeY = Wrap( 0, universeY, UNIVERSE_DIM );
    
    r32 halfWorld = ( r32 ) lateralChunkSpan * 0.5f;
    r32 halfWorldInTile = halfWorld * CHUNK_DIM;
    
    // NOTE(Leonardo): values between -1 and +1
    r32 chunkBilX = ( ( r32 ) chunkX - halfWorld ) / halfWorld;
    r32 chunkBilY = ( ( r32 ) chunkY - halfWorld ) / halfWorld;
    
    // NOTE(Leonardo): normalized values
    r32 chunkNormX = ( r32 ) chunkX / lateralChunkSpan;
    r32 chunkNormY = ( r32 ) chunkY / lateralChunkSpan;
    
    Assert( Normalized( chunkNormX ) );
    Assert( Normalized( chunkNormY ) );
    
    
    RandomSequence worldSequence = Seed( universeX * universeY );
    u32 biomeSeed = GetNextUInt32( &worldSequence );
    u32 heightSelectorSeed = GetNextUInt32( &worldSequence );
    
    
    r32 maxHeight = 30.0f * VOXEL_SIZE;
    // NOTE(Leonardo): these are the different "landscapes" we have
    
    
    NoiseParams lowLandscape = NoisePar( 1.0f, 1, 0.0f, 3.0f * VOXEL_SIZE, &worldSequence );
    NoiseParams midLandscape = NoisePar( 8.0f, 3, 0.1f, 6.0f * VOXEL_SIZE, &worldSequence );
    //NoiseParams high = NoisePar( 15.0f, 3, 0.25f, maxHeight, &worldSequence );
    
    r32 lowThreesold = 0.99f;
    r32 midThreesold = 0.99f;
    //r32 highThreesold = 0.99f;
    
    Selector landscapeSelect = {};
    AddChoice( &landscapeSelect, lowThreesold, lowLandscape );
    AddChoice( &landscapeSelect, midThreesold, midLandscape );
    //AddChoice( &landscapeSelect, highThreesold, high );
    
    r32 maxOtherChunkInfluence = 0.5f;
    NoiseParams influenceNoise = NoisePar( 50.0f, 1, 0.0f, maxOtherChunkInfluence, &worldSequence );
    
    NoiseParams landscapeNoise = NoisePar( 6.0f, 1, 0.0f, 1.0f, &worldSequence );
    NoiseParams temperatureNoise = NoisePar( 6.0f, 1, 0.0f, 1.0f, &worldSequence );
    NoiseParams drynessNoise = NoisePar( 3.0f, 1, 0.0f, 1.0f, &worldSequence );
    
    MinMaxChoice lowTemperature = MinMax( 13.0f, 22.0f );
    MinMaxChoice midTemperature = MinMax( 8.0f, 17.0f );
    //MinMaxChoice highTemperature = MinMax( 2.0f, 12.0f );
    
    Selector temperatureSelect = {};
    AddChoice( &temperatureSelect, lowThreesold, lowTemperature ); 
    AddChoice( &temperatureSelect, midThreesold, midTemperature ); 
    //AddChoice( &temperatureSelect, highThreesold, highTemperature ); 
    
#if 0    
    r32 d = 2.0f * Max( Abs( chunkNormX ), Abs( chunkNormY ) );
    r32 dropOffCoeff = 1.0f - 0.79f * powf( d, 0.76f );
    // NOTE( Leonardo ): 0.1 minimum 0.15 max?
    height = ( height + 0.14f ) * dropOffCoeff;
    height = Max( height, 0.0f );
#endif
    
    
    r32 chunkXInTiles = ( ( chunkX + 0.5f ) * CHUNK_DIM ) / ( lateralChunkSpan * CHUNK_DIM );
    r32 chunkYInTiles = ( ( chunkY + 0.5f ) * CHUNK_DIM ) / ( lateralChunkSpan * CHUNK_DIM );
    
    BiomePyramid pyramid = {};
    
    Selector* lowDryness = AddSelectorForDryness( &pyramid, 0.99f );
    Selector* highDryness = AddSelectorForDryness( &pyramid, 0.99f );
    
    AddChoice( lowDryness, 6.0f, Biome_forest );
    //AddChoice( lowDryness, 22.0f, Biome_mountain );
    
    AddChoice( highDryness, 10.0f, Biome_mountain );
    //AddChoice( highDryness, 15.0f, Biome_forest );
    
    
    r32 chunkDryness = Evaluate( chunkXInTiles, chunkYInTiles, drynessNoise );
    chunk->dryness = chunkDryness;
    
    u32 baseTileX = chunkX * CHUNK_DIM;
    u32 baseTileY = chunkY * CHUNK_DIM;
    
    for( u8 tileY = 0; tileY < CHUNK_DIM; ++tileY )
    {
        for( u8 tileX = 0; tileX < CHUNK_DIM; ++tileX )
        {
            u32 realTileX = baseTileX + tileX;
            u32 realTileY = baseTileY + tileY;
            
            // NOTE(Leonardo): normalized values
            r32 tileNormX = ( r32 ) realTileX / ( lateralChunkSpan * CHUNK_DIM );
            r32 tileNormY = ( r32 ) realTileY / ( lateralChunkSpan * CHUNK_DIM );
            
            Assert( Normalized( tileNormX ) );
            Assert( Normalized( tileNormY ) );
            
            r32 tileLandscape = Evaluate( tileNormX, tileNormY, landscapeNoise );
            r32 finalHeight = Select( &landscapeSelect, tileNormX, tileNormY, tileLandscape );
            Assert( finalHeight <= maxHeight );
            
            r32 tileDryness = Evaluate( tileNormX, tileNormY, drynessNoise );
            
            r32 temperatureTurbolence = Evaluate( tileNormX, tileNormY, temperatureNoise );
            r32 finalTemperature = GetTemperature( &temperatureSelect, tileLandscape, temperatureTurbolence );
            r32 finalBiome = GetBiomeFor( &pyramid, tileDryness, finalTemperature );
            r32 influence = Evaluate(tileNormX, tileNormY, influenceNoise);
            
#if 0    
            u32 terracesCount = 256;
            r32 terracesStep = maxHeight / terracesCount;
            
            u32 testTerraces = ( u32 ) ( ( finalHeight / maxHeight ) * terracesCount );
            finalHeight = testTerraces * terracesStep;
#endif
            finalHeight = 0;
            chunk->heights[tileY][tileX] = finalHeight;
            chunk->biomes[tileY][tileX] = (u8) finalBiome;
            chunk->influences[tileY][tileX] = influence;
            chunk->tileNormals[tileY][tileX] = V2(tileNormX, tileNormY);
            
            u8 quarter = (CHUNK_DIM / 4);
            u8 half = (CHUNK_DIM / 2);
            
            if(tileX == quarter && tileY == quarter)
            {
                chunk->biomeSubChunks[0] = (u8) finalBiome;
            }
            else if(tileX == (quarter + half) && tileY == quarter)
            {
                chunk->biomeSubChunks[1] = (u8) finalBiome;
            }
            else if(tileX == quarter && tileY == (quarter + half))
            {
                chunk->biomeSubChunks[2] = (u8) finalBiome;
            }
            else if(tileX == (quarter + half) && tileY == (quarter + half))
            {
                chunk->biomeSubChunks[3] = (u8) finalBiome;
            }
        }
    }
}

#undef TARGET
#undef ACTOR