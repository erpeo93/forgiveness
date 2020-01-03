#pragma once
introspection() struct GameEffectInstance
{
    u16 commandIndex;
    r32 deleteTime;
    r32 targetTime;
    b32 targetEffect;
    u16 action;
    u16 type;
    EntityType spawnType;
    r32 power;
    r32 radious;
};


introspection() struct GameEffect
{
    r32 timer;
    r32 deleteTime;
    b32 targetEffect;
    GameProperty action MetaDefault("{Property_action, none}") MetaFixed(property);
    GameProperty effectType MetaDefault("{Property_gameEffect}") MetaFixed(property);
    
    EntityName spawnType;
    r32 power;
};

introspection() struct EffectBinding
{
    GameProperty property;
    GameEffect effect;
};

introspection() struct ProbabilityEffectOption
{
    r32 weight;
    GameEffect effect;
};

introspection() struct ProbabilityEffect
{
    r32 probability;
    
    ArrayCounter optionCount MetaCounter(options);
    ProbabilityEffectOption* options;
};

struct ActiveEffect
{
    GameEffectInstance effect;
    r32 time;
    r32 totalTime;
};

struct ActiveEffectComponent
{
    u32 effectCount;
    ActiveEffect effects[8];
};

struct CollisionEffectsComponent
{
    u32 effectCount;
    GameEffectInstance effects[8];
};

struct OverlappingEffectsComponent
{
    u32 effectCount;
    GameEffectInstance effects[8];
};