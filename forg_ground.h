#pragma once
#undef internal
#include "jc_voronoi.h"
#define internal static

struct VoronoiDiagram
{
    jcv_diagram diagram;
    UniversePos originP;
	Vec3 deltaP;
};


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

introspection() struct WaterPhase
{
    r32 referenceHeight;
    
    Vec3 minColor;
    Vec3 maxColor;
    
    r32 maxAlpha;
    r32 minAlpha;
    
    r32 maxColorDisplacement;
    r32 maxAlphaDisplacement;
    
    r32 colorSpeed;
    r32 colorSpeedV;
    
    r32 sineSpeed;
    r32 sineSpeedV;
    
    r32 sineWeight;
    
    NoiseParams noise;
};

introspection() struct WaterParams
{
    WaterPhase deep;
    WaterPhase swallow;
    WaterPhase shore;
};