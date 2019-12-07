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

introspection() struct ParticleUpdater
{
    AssetID bitmapID MetaUneditable();
    RenderTexture* texture MetaUneditable();
    Vec2 textureInvUV MetaUneditable();
    
    GameAssetType asset MetaDefault("{AssetType_Image, 0}") MetaFixed(type);
    
    b32 sineUpdater;
    r32 totalRadiants;
    u32 sineSubdivisions MetaDefault("1");
    
    Vec3 ddP;
    Vec3 finalAcceleration;
    Vec4 ddC;
    
    r32 dScaleX;
    r32 dScaleY;
    r32 dAngle;
};

introspection() struct ParticleEmitter
{
    r32 particlesPerSec MetaDefault("1.0f");
    
    r32 lifeTime MetaDefault("1.0f");
    r32 lifeTimeV;
    
    Vec3 startPV;
    
    r32 lerpWithUpVector;
    Vec3 dP MetaDefault("V3(0, 0, 1)");
    Vec3 dPV;
    
    Vec4 C MetaDefault("V4(1, 1, 1, 1)");
    Vec4 CV;
    
    Vec4 dC;
    Vec4 dCV;
    
    r32 angle;
    r32 angleV;
    
    r32 scaleX MetaDefault("1.0f");
    r32 scaleXV;
    
    r32 scaleY MetaDefault("1.0f");
    r32 scaleYV;
};

introspection() struct ParticlePhase
{
    r32 ttl MetaDefault("1.0f");
    ParticleUpdater updater;
};

#define MAX_PHASE_COUNT 8
#define MAX_SOUND_COUNT 4
struct ParticleEffectInstance
{
    b32 active;
    
    r32 spawnParticlesLeftOff;
    u32 particle4xCount;
    Particle_4x* firstParticle;
    
    
    ParticleEmitter emitter;
    r32 dAngleSineUpdaters;
    u32 phaseCount;
    ParticlePhase phases[MAX_PHASE_COUNT];
    
    u32 soundCount;
    SoundMapping sounds[MAX_SOUND_COUNT];
    
    Vec3 P;
    Vec3 UpVector;
    r32 updaterAngle;
    
    union
    {
        ParticleEffectInstance* next;
        ParticleEffectInstance* nextFree;
    };
};

struct ParticleCache
{
    RandomSequence particleEntropy;
    ParticleEffectInstance* firstActiveEffect;
    Vec3 deltaParticleP;
    
    ParticleEffectInstance* firstFreeEffect;
    Particle_4x* firstFreeParticle4x;
    MemoryPool* pool;
};


introspection() struct ParticleEffect
{
    ParticleEmitter emitter;
    r32 dAngleSineUpdaters;
    ArrayCounter phaseCount MetaCounter(phases);
    ParticlePhase* phases;
    
    ArrayCounter soundCount MetaCounter(sounds);
    SoundMappingDefinition* sounds;
};