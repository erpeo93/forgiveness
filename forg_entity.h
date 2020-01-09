#pragma once
#if FORG_SERVER
#define GameplayR32WithDefault(name) r32 default_##name; R32 name;
#else
#define GameplayR32WithDefault(name) r32 name;
#endif

#if FORG_SERVER
#define GameplayR32(name) R32 name;
#else
#define GameplayR32(name) r32 name;
#endif

#define SetR32Default(comp, name, value) SetR32(&comp->name, value); comp->default_##name = value; 



#if FORG_SERVER
#define GameplayU16(name) U16 name;
#else
#define GameplayU16(name) u16 name;
#endif

struct ActionComponent
{
    GameplayU16(action);
    GameplayR32WithDefault(speed);
    
#if FORG_SERVER
    r32 time;
    u16 commandIndex;
#endif
};

struct HealthComponent
{
    GameplayR32(physicalHealth);
    GameplayR32WithDefault(maxPhysicalHealth);
    
    GameplayR32(mentalHealth);
    GameplayR32WithDefault(maxMentalHealth);
    
    GameplayR32(onFirePercentage);
    GameplayR32(poisonPercentage);
    
#if FORG_SERVER
    GameplayR32WithDefault(physicalRegenerationPerSecond);
    GameplayR32WithDefault(mentalRegenerationPerSecond);
    GameplayR32WithDefault(fireDamagePerSecond);
    GameplayR32WithDefault(poisonDamagePerSecond);
#endif
};

struct CombatComponent
{
    GameplayR32WithDefault(attackDistance);
    GameplayR32WithDefault(attackContinueCoeff);
    
    r32 movementSpeedWhileAttacking;
};

struct VegetationComponent
{
    GameplayR32WithDefault(flowerGrowingSpeed);
    GameplayR32WithDefault(fruitGrowingSpeed);
    GameplayR32WithDefault(branchGrowingSpeed);
    
    r32 requiredFlowerDensity;
    r32 requiredFruitDensity;
    r32 requiredBranchDensity;
    
    GameplayR32(flowerDensity);
    GameplayR32(fruitDensity);
    GameplayR32(branchDensity);
};

struct LightComponent
{
    GameplayR32WithDefault(lightRadious);
    
#ifndef FORG_SERVER
    Vec3 lightColor;
#endif
};

struct InfusedEffect
{
    u16 essenceCount;
    U16 effectIndex;
    U16 level;
    
#ifndef FORG_SERVER
    Rect2 projectedOnScreenEffect;
    Rect2 projectedOnScreenEssence;
#endif
};

#define MAX_INFUSED_EFFECTS 4
struct InfusedEffectsComponent
{
    InfusedEffect effects[MAX_INFUSED_EFFECTS];
};


struct SculptureEffect
{
    u64 pieceHash;
    Vec3 noActiveCameraOffset;
    Vec3 activeMinCameraOffset;
    Vec3 activeMaxCameraOffset;
    r32 speed;
    r32 runningTime;
};


struct StatueComponent
{
    b32 active;
    
#ifndef FORG_SERVER
    u32 effectCount;
    SculptureEffect effects[8];
#endif
};

enum EntityFlags
{
    EntityFlag_notInWorld = (1 << 0),
    EntityFlag_occluding = (1 << 1),
    EntityFlag_deleted = (1 << 2),
    EntityFlag_locked = (1 << 3),
    EntityFlag_canGoIntoWater = (1 << 4),
    EntityFlag_ghost = (1 << 5),
    EntityFlag_teleported = (1 << 6),
};
