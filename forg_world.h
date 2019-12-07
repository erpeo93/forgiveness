#pragma once
#define WORLD_CHUNK_APRON 1
#define VOXEL_SIZE 2.0f
#define CHUNK_DIM 8
#define CHUNK_SIDE CHUNK_DIM * VOXEL_SIZE
#define UPDATE_DISTANCE (WORLD_CHUNK_APRON + 1) * CHUNK_SIDE
#define WORLD_CHUNK_SPAN 16
#define WORLD_SIDE WORLD_CHUNK_SPAN * CHUNK_SIDE

introspection() struct UniversePos
{
    i16 chunkX;
    i16 chunkY;
    i16 chunkZ;
    Vec3 chunkOffset;
};

struct TilePatch
{
    r32 offsetTime;
    r32 colorTime;
    r32 scaleTime;
};

#define MAX_TILE_PATCHES 8
#define MAX_TILE_SOUNDS 4
struct WorldTile
{
#ifndef FORG_SERVER
    TilePatch patches[MAX_TILE_PATCHES];
    u32 soundCount;
    SoundMapping sounds[MAX_TILE_SOUNDS];
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
    
    i16 worldX;
    i16 worldY;
    i16 worldZ;
    
    u32 worldSeed;
    union
    {
        WorldChunk* next;
        WorldChunk* nextFree;
    };
#endif
    WorldTile* tiles;
};