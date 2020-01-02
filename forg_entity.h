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
    
#if FORG_SERVER
    u16 selectedCrafingEssences[MAX_RECIPE_ESSENCES];
    r32 time;
#endif
};

struct HealthComponent
{
    GameplayR32(physicalHealth);
    GameplayR32WithDefault(maxPhysicalHealth);
    
    GameplayR32(mentalHealth);
    GameplayR32WithDefault(maxMentalHealth);
};

struct CombatComponent
{
    GameplayR32WithDefault(attackDistance);
    GameplayR32WithDefault(attackContinueCoeff);
};

struct VegetationComponent
{
    GameplayR32WithDefault(flowerGrowingSpeed);
    GameplayR32WithDefault(fruitGrowingSpeed);
    
    r32 requiredFlowerDensity;
    r32 requiredFruitDensity;
    
    GameplayR32(flowerDensity);
    GameplayR32(fruitDensity);
};

struct LightComponent
{
    GameplayR32WithDefault(lightRadious);
    
#ifndef FORG_SERVER
    Vec3 lightColor;
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
};
