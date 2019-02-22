#pragma once

global_variable Vec3 fluidVectors[] =
{
    { 0, 0, 1 },
    
    
    { 1, 0, 0 },
    { 0.707388222f, 0.000000000f, 0.706825197f },
    
    
    { 0.70710678118f, 0.70710678118f, 0 },
    { 0.500398099f, 0.499999821f, 0.706825197f },
    
    
    { 0, 1, 0 },
    { 0.000000000f, 0.707388222f, 0.706825197f },
    
    
    { -0.70710678118f, 0.70710678118f, 0 },
    { -0.499999821f, 0.500398099f, 0.706825197f },
    
    
    { -1, 0, 0 },
    { -0.707388222f, 0.000000000f, 0.706825197f },
    
    
    { -0.70710678118f, -0.70710678118f, 0 },
    { -0.500398099f, -0.499999821f, 0.706825197f },
    
    
    
    { 0, -1, 0 },
    { 0.000000000f, -0.707388222f, 0.706825197f },
    
    
    { 0.70710678118f, -0.70710678118f, 0 },
    { 0.499999821f, -0.500398099f, 0.706825197f },
    
};


struct FluidRaySegment
{
    b32 touched;
    r32 intensityPercentage;
};

#define SEGMENT_COUNT 4
struct FluidRay
{
    r32 lengthPercentage;
    FluidRaySegment segments[SEGMENT_COUNT];
};


enum FluidSpawnType
{
    FluidSpawn_None,
    FluidSpawn_Fire,
    FluidSpawn_Water,
    FluidSpawn_Steam,
    
    FluidSpawn_Count,
};

struct Fluid
{
    u32 type;
    r32 fuel;
    
    r32 length;
    m4x4 direction;
    r32 intensity;
    
    FluidRay rays[ArrayCount( fluidVectors)];
    
    u64 source1;
    u64 source2;
    
    u32 sourceSegmentIndex1;
    u32 sourceSegmentIndex2;
};

inline r32 GetRayLength( r32 rayMaxLength, FluidRay* ray )
{
    r32 result = 0;
    
    u32 rayValidSegments = 0;
    for( ; rayValidSegments < SEGMENT_COUNT; ++rayValidSegments )
    {
        FluidRaySegment* segment = ray->segments + rayValidSegments;
        if( segment->intensityPercentage == 0.0f )
        {
            break;
        }
    }
    
    r32 rayLengthPercentage = rayValidSegments * ( 1.0f / SEGMENT_COUNT );
    result = rayMaxLength * rayLengthPercentage;
    
    return result;
}

