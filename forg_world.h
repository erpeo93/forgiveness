#pragma once
#define WORLD_CHUNK_APRON 2
#define VOXEL_SIZE 2.0f
#define CHUNK_DIM 8
#define WORLD_CHUNK_SPAN 64
#define CHUNK_SIDE CHUNK_DIM * VOXEL_SIZE

struct GenerationData
{
    u64 generic;
};

struct UniversePos
{
    i32 chunkX;
    i32 chunkY;
    Vec3 chunkOffset;
};

struct WorldTile
{
#ifndef FORG_SERVER
    Lights lights;
    
    r32 waterPhase;
    r32 waterSine;
    b32 movingNegative;
    RandomSequence waterSeq;
    r32 blueNoise;
    r32 alphaNoise;
#endif
    
    Vec4 color;
    GameProperty property;
    GameAssetType asset;
    r32 elevation;
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
    SpecialTexture texture;
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