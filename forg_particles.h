#pragma once
struct Particle_4x
{
    V3_4x P;
    V3_4x dP;
    __m128 angle4x;
    __m128 height4x;
    
    V4_4x C;
    V4_4x dC;
    
    
    union
    {
        Particle_4x* next;
        Particle_4x* nextFree;
    };
    u64 padding;
    __m128 ttl4x;
    __m128 lerp4x;
    __m128 padding3;
};

enum ParticleUpdaterType
{
    ParticleUpdater_Standard,
    ParticleUpdater_Sine,
    
    ParticleUpdater_Count
};

struct ParticleUpdater
{
    ParticleUpdaterType type;
    
    V3_4x unitDP;
    V3_4x ddP;
    V3_4x UpVector;
    
    V4_4x ddC;
    
    __m128 lerpVel4x;
    __m128 lerpAlpha4x;
};

enum ParticleEmitterType
{
    ParticleEmitter_Standard,
    
    ParticleEmitter_Count
};

struct ParticleEmitter
{
    ParticleEmitterType type;
    
    r32 lifeTime;
    r32 lifeTimeV;
    
    Vec3 startPV;
    
    Vec3 dP;
    Vec3 dPV;
    
    Vec4 C;
    Vec4 CV;
    
    Vec4 dC;
    Vec4 dCV;
    
    r32 angle;
    r32 angleV;
    
    r32 height;
    r32 heightV;
};

struct ParticleEffect
{
    b32 active;
    
    ParticleEmitter emitter;
    ParticleUpdater updater;
    
    u32 particle4xCount;
    Particle_4x* firstParticle;
    
    union
    {
        ParticleEffect* next;
        ParticleEffect* nextFree;
    };
};

struct ParticleCache
{
    RandomSequence particleEntropy;
    ParticleEffect* firstActiveEffect;
    Vec3 deltaParticleP;
    
    ParticleEffect* firstFreeEffect;
    Particle_4x* firstFreeParticle4x;
    MemoryPool pool;
};