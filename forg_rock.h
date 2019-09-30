#pragma once


introspection() struct RockMineral
{
    r32 lerp;
    r32 lerpDelta;
    Vec4 color;
};

introspection() struct RockDefinition
{
    b32 collides;
    
    Vec4 color MetaDefault("V4(1, 1, 1, 1)");
    Vec4 startingColorDelta;
    Vec4 perVertexColorDelta;
    
    Vec3 scale MetaDefault("V3(1, 1, 1)");
    Vec3 scaleDelta;
    
    r32 smoothness MetaDefault("0.5f");
    r32 smoothnessDelta;
    r32 normalSmoothness;
    
    u32 iterationCount MetaDefault("1");
    
    r32 minDisplacementY MetaDefault("0.5f");
    r32 maxDisplacementY MetaDefault("0.5f");
    
    r32 minDisplacementZ MetaDefault("0.6f");
    r32 maxDisplacementZ MetaDefault("0.6f");
    
    r32 percentageOfMineralVertexes;
    
    ArrayCounter mineralCount MetaCounter(minerals);
    RockMineral* minerals;
    
    u32 renderingRocksCount MetaDefault("1");
    u32 renderingRocksDelta;
    Vec3 renderingRocksRandomOffset;
    r32 scaleRandomness;
};


inline Vec3 GetRockDim(RockDefinition* rock, RandomSequence* sequence)
{
    Vec3 result = rock->scale + Hadamart(rock->scaleDelta, RandomBilV3(sequence));
    return result;
}


#ifndef FORG_SERVER
struct RockComponent
{
    Vec3 dim;
    
    u32 vertexCount;
    ColoredVertex vertexes[1024];
    
    u32 faceCount;
    ModelFace faces[1024];
};
#endif