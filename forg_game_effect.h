#pragma once
introspection() struct GameEffectInstance
{
    r32 timer;
    b32 targetEffect;
    u16 action;
    u16 type;
    EntityType spawnType;
    r32 power;
};


introspection() struct GameEffect
{
    r32 timer;
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

struct EffectComponent
{
    u32 effectCount;
    
    GameEffectInstance effects[8];
    r32 timers[8];
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