#pragma once

#define MAX_PARTICLE_COUNT 2048
#define MAX_PARTICLE_COUNT_4 (MAX_PARTICLE_COUNT / 4)
struct Particle_4x
{
    union
    {
        V3_4x P;
        V3_4x startP;
    };
    
    union
    {
        V3_4x dP;
        V3_4x unitDP;
    };
    
    union
    {
        V3_4x ddP;
        V3_4x UpVector;
    };
    
    
    __m128 angle4x;
    __m128 height4x;
    
    union
    {
        V4_4x C;
        V4_4x startC;
        
    };
    
    union
    {
        V4_4x dC;
        
        struct
        {
            __m128 ignored;
            __m128 lerp4x;
            __m128 lerpVel4x;
            __m128 lerpColorAlpha;
        };
    };
};

struct ParticleSystem
{
    Particle_4x particles[MAX_PARTICLE_COUNT_4];
    u32 nextParticle4;
    BitmapId bitmapID;
    ObjectTransform transform;
};

struct ParticleCache
{
    RandomSequence particleEntropy;
    ParticleSystem fireSystem;
    ParticleSystem waterSystem;
    ParticleSystem steamSystem;
    ParticleSystem waterRippleSystem;
    ParticleSystem ashSystem;
    ParticleSystem ashSineSystem;
    Vec3 deltaParticleP;
};