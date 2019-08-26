#pragma once


struct RockMineral
{
    r32 lerp;
    r32 lerpDelta;
    
    Vec4 color;
    
    union
    {
        RockMineral* next;
        RockMineral* nextFree;
    };
};

struct RockDefinition
{
    b32 collides;
    
    u64 modelTypeHash;
    u64 modelNameHash;
    
    Vec4 color;
    Vec4 startingColorDelta;
    
    Vec4 perVertexColorDelta;
    
    Vec3 scale;
    Vec3 scaleDelta;
    
    r32 smoothness;
    r32 smoothnessDelta;
    r32 normalSmoothness;
    
    u32 iterationCount;
    
    
    r32 minDisplacementY;
    r32 maxDisplacementY;
    
    r32 minDisplacementZ;
    r32 maxDisplacementZ;
    
    
    r32 percentageOfMineralVertexes;
    u32 mineralCount;
    RockMineral* firstPossibleMineral;
    
    u32 renderingRocksCount;
    u32 renderingRocksDelta;
    Vec3 renderingRocksRandomOffset;
    r32 scaleRandomness;
    
    union
    {
        RockDefinition* nextFree;
        RockDefinition* next;
    };
};


inline Vec3 GetRockDim(RockDefinition* rock, RandomSequence* sequence)
{
    Vec3 result = rock->scale + Hadamart(rock->scaleDelta, RandomBilV3(sequence));
    return result;
}


#ifndef FORG_SERVER
struct Rock
{
    Vec3 dim;
    
    u32 vertexCount;
    ColoredVertex vertexes[1024];
    
    u32 faceCount;
    ModelFace faces[1024];
    
    
    Rock* nextFree;
};
#endif