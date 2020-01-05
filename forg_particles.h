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
    Vec3 finalAcceleration MetaUneditable();
    
    ArrayCounter propertyCount MetaCounter(properties);
    GameProperty* properties;
    GameAssetType asset MetaDefault("{AssetType_Image, 0}") MetaFixed(type);
    
    b32 updateColorOverLifetime MetaDefault("true");
    b32 modulateColorWithVelocity;
    
    Vec4 colorIdle MetaDefault("V4(1, 1, 1, 1)");
    Vec4 colorFullSpeed MetaDefault("V4(1, 1, 1, 1)");
    
    b32 updateScaleOverTime MetaDefault("true");
    b32 modulateScaleWithVelocity;
    
    r32 scaleXIdle MetaDefault("1.0f");
    r32 scaleXFullspeed MetaDefault("1.0f");
    
    r32 scaleYIdle MetaDefault("1.0f");
    r32 scaleYFullspeed MetaDefault("1.0f");
    
    b32 updateAngleOvertime MetaDefault("true");
    b32 modulateAngleWithVelocity;
    
    r32 angleIdle;
    r32 angleFullspeed;
    
    Vec3 ddP;
    Vec4 ddC;
    
    r32 dScaleX;
    r32 dScaleY;
    r32 dAngle;
};

printTable() enum ParticleEmissionType
{
    Emission_RateOverTime,
    Emission_BurstOverTime,
    Emission_Fixed,
};

printTable() enum ParticleBoundType
{
    Bound_None,
    Bound_Sphere,
    Bound_Rect,
};

introspection() struct ParticleEmitter
{
    r32 accumulatedTime MetaUneditable();
    r32 targetAccumulatedTime MetaUneditable();
    
    u16 emissionType MetaUneditable();
    u16 boundType MetaUneditable();
    
    Enumerator emission MetaEnumerator("ParticleEmissionType");
    Enumerator bound MetaEnumerator("ParticleBoundType");
    
    r32 radious;
    Vec3 rectDim;
    
    b32 modulateWithVelocity;
    b32 outline;
    
    r32 particleCount;
    r32 particleCountV;
    
    r32 particlesPerSec MetaDefault("1.0f");
    r32 targetAccumulatedTimeRef MetaDefault("1.0f");
    r32 targetAccumulatedTimeV;
    
    r32 lifeTime MetaDefault("1.0f");
    r32 lifeTimeV;
    
    Vec3 startPV;
    
    r32 followOrientation;
    Vec3 defaultOrientation MetaDefault("V3(0, 0, 1)");
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
struct ParticleEffectInstance
{
    b32 active;
    
    r32 spawnParticlesLeftOff;
    u32 particle4xCount;
    Particle_4x* firstParticle;
    
    ParticleEmitter emitter;
    u32 phaseCount;
    ParticlePhase phases[MAX_PHASE_COUNT];
    
    b32 influencedByWind;
    Vec4 windInfluences;
    u8 windFrequency;
    
    Vec3 P;
    Vec3 speed;
    r32 boundsScale;
    r32 maxSpeedMagnitudo;
    
    ParticleEffectInstance* subEffect;
    
    union
    {
        ParticleEffectInstance* next;
        ParticleEffectInstance* nextFree;
    };
};

struct ParticleCache
{
    RandomSequence entropy;
    ParticleEffectInstance* firstActiveEffect;
    Vec3 deltaParticleP;
    
    ParticleEffectInstance* firstFreeEffect;
    Particle_4x* firstFreeParticle4x;
    MemoryPool* pool;
};


introspection() struct ParticleEffect
{
    b32 influencedByWind;
    Vec4 windInfluences MetaDefault("V4(0.05f, 0.05f, 0.05f, 0.05f)");
    u16 windFrequency MetaDefault("1");
    
    r32 maxSpeedMagnitudo;
    ParticleEmitter emitter;
    ArrayCounter phaseCount MetaCounter(phases);
    ParticlePhase* phases;
    
    r32 subEffectSpeedMagnitudo;
    ParticleEmitter subEmitter;
    ArrayCounter subPhaseCount MetaCounter(subPhases);
    ParticlePhase* subPhases;
};