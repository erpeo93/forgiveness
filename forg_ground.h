#pragma once
struct TileSplash
{
    u64 nameHash;
    r32 weight;
};

struct TileDefinition
{
    
    Vec4 tileBorderColor;
    Vec4 tileColor;
    Vec4 colorDelta;
    r32 groundPointMaxOffset;
    r32 groundPointPerTile;
    r32 groundPointPerTileV;
    r32 chunkynessWithSame;
    r32 chunkynessWithOther;
    u32 tilePointsLayout;
    r32 colorRandomness;
    
    NoiseParams tileNoise;
    
    u32 textureSplashCount;
    r32 splashOffsetV;
    r32 splashAngleV;
    r32 splashMinScale;
    r32 splashMaxScale;
    
    r32 totalWeights;
    u32 splashCount;
    TileSplash splashes[32];
    
    TileDefinition* nextFree;
};