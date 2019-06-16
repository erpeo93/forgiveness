#pragma once

struct BoltDefinition
{
    r32 animationTick;
    r32 fadeinTime;
    r32 fadeoutTime;
    r32 ttl;
    r32 ttlV;
    Vec4 color;
    r32 thickness;
    r32 thicknessV;
    r32 magnitudoStructure;
    r32 magnitudoAnimation;
    
    u32 subdivisions;
    u32 subdivisionsV;
    
    Vec3 lightColor;
    r32 lightIntensity;
    r32 lightStartTime;
    r32 lightEndTime;
};

#define MAX_BOLT_SUBDIVISIONS 32
struct Bolt
{
    r32 ttl;
    u32 seed;
    r32 timeSinceLastAnimationTick;
    RandomSequence animationSeq;
    Vec3 startP;
    Vec3 endP;
    
    Vec3 subdivisionAnimationOffsets[MAX_BOLT_SUBDIVISIONS];
    
    union
    {
        Bolt* next;
        Bolt* nextFree;
    };
};

struct BoltCache
{
    Vec3 deltaP;
    Bolt* firstBolt;
    RandomSequence seedSource;
    
    MemoryPool pool;
    Bolt* firstFreeBolt;
    
    BoltDefinition definition;
};