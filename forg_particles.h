#pragma once
struct Particle_4x
{
    V3_4x P;
    V3_4x dP;
    __m128 angle4x;
    __m128 scaleX4x;
    __m128 scaleY4x;
    
    V4_4x C;
    V4_4x dC;
    
    union
    {
        Particle_4x* next;
        Particle_4x* nextFree;
    };
    u64 padding;
    __m128 ttl4x;
    __m128 randomUni4x;
    __m128 padding2;
};

printTable(noPrefix) enum ParticleUpdaterType
{
    ParticleUpdater_Standard,
    ParticleUpdater_Sine,
    
    ParticleUpdater_Count
};

printTable(noPrefix) enum ParticleUpdaterEndPosition
{
    UpdaterEndPos_FixedOffset,
    UpdaterEndPos_DestPos,
};


struct ParticleUpdater
{
    ParticleUpdaterType type;
    ParticleUpdaterEndPosition destPType;
    
    BitmapId bitmapID;
    u64 particleHashID;
    r32 startDrawingFollowingBitmapAt;
    Vec3 startOffset;
    Vec3 endOffset;
    
    V3_4x ddP;
    V4_4x ddC;
    V3_4x UpVector;
    
    __m128 dScaleX;
    __m128 dScaleY;
    __m128 dAngle;
    
    __m128 totalRadiants;
    
    u32 sineSubdivisions;
};

enum ParticleEmitterType
{
    ParticleEmitter_Standard,
    
    ParticleEmitter_Count
};

struct ParticleEmitter
{
    ParticleEmitterType type;
    
    r32 particlesPerSec;
    
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
    
    r32 scaleX;
    r32 scaleXV;
    
    r32 scaleY;
    r32 scaleYV;
};

struct ParticlePhase
{
    r32 ttlMax;
    r32 ttlMin;
    
    ParticleUpdater updater;
};


struct ParticleEffectData
{
	Vec3 P;
    Vec3 destP;
    r32 updaterAngle;
};

#define MAX_PHASE_COUNT 8
struct ParticleEffect
{
    b32 active;
    
    ParticleEffectData data;
    r32 spawnParticlesLeftOff;
    ParticleEffectDefinition* definition;
    
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


struct ParticleEffectDefinition
{
    ParticleEmitter emitter;
    
    r32 dAngleSineUpdaters;
    u32 phaseCount;
    ParticlePhase phases[MAX_PHASE_COUNT];
    
    union
    {
        ParticleEffectDefinition* next;
        ParticleEffectDefinition* nextFree;
    };
};