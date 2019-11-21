#pragma once
#define WORLD_CHUNK_APRON 2
#define VOXEL_SIZE 2.0f
#define CHUNK_DIM 8
#define WORLD_CHUNK_SPAN 4
#define CHUNK_SIDE CHUNK_DIM * VOXEL_SIZE
#define WORLD_SIDE WORLD_CHUNK_SPAN * CHUNK_SIDE

introspection() struct UniversePos
{
    i32 chunkX;
    i32 chunkY;
    i32 chunkZ;
    Vec3 chunkOffset;
};

struct TilePatch
{
    r32 offsetTime;
    r32 colorTime;
    r32 scaleTime;
};

#define MAX_TILE_PATCHES 8
struct WorldTile
{
#ifndef FORG_SERVER
    TilePatch patches[MAX_TILE_PATCHES];
    GameAssetType asset;
#endif
    
    GameProperty property;
    GameProperty underSeaLevelFluid;
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
    TempLight* firstTempLight;
    WorldChunk* next;
    
    
    i32 worldX;
    i32 worldY;
    i32 worldZ;
    b32 initialized;
#endif
    WorldTile* tiles;
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