#pragma once
#define WORLD_DIM 200.0f
#define WORLD_CHUNK_SPAN 8
#define VOXEL_SIZE 2.0f

struct UniversePos
{
    i32 chunkX;
    i32 chunkY;
    Vec3 chunkOffset;
};

struct WorldTile
{
#ifndef FORG_SERVER
    
    r32 layoutNoise;
    r32 waterPhase;
    r32 waterSine;
    b32 movingNegative;
    RandomSequence waterSeq;
    r32 blueNoise;
    r32 alphaNoise;
    Lights lights;
    r32 chunkynessSame;
    r32 chunkynessOther;
    Vec4 baseColor;
    Vec4 colorDelta;
    r32 colorRandomness;
    Vec4 borderColor;
#endif
    r32 height;
    r32 waterLevel;
    
    u64 tileDefinitionHash;
};

struct TempLight
{
    Vec3 P;
    Vec3 color;
    r32 strength;
    
    union
    {
        TempLight* next;
        TempLight* nextFree;
    };
};

struct WorldChunk
{
#ifndef FORG_SERVER
    AssetLRULink LRU;
    RenderTexture textureHandle;
    Lights lights;
#endif
    
    b32 initialized;
    
    i32 worldX;
    i32 worldY;
    i32 worldZ;
    
    WorldTile tiles[CHUNK_DIM][CHUNK_DIM];
    
    TempLight* firstTempLight;
    TempLight* firstTempLightNextFrame;
    
    WorldChunk* next;
};

#define SEASON_DURATION 5.0f
enum WorldSeason
{
    Season_Autumn,
    Season_Winter,
    Season_Spring,
    Season_Summer,
    
    Season_Count
};