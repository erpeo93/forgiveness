#pragma once
introspection() struct GameEffect
{
    r32 timer MetaDefault("1");
    GameProperty action MetaDefault("{Property_action, none}") MetaFixed(property);
    GameProperty effectType MetaDefault("{Property_gameEffect}") MetaFixed(property);
    EntityRef spawnType;
};

introspection() struct EffectBinding
{
    GameProperty property;
    GameEffect effect;
};

struct EffectComponent
{
    u32 effectCount;
    
    GameEffect effects[8];
    r32 timers[8];
};

struct CollisionEffectsComponent
{
    u32 effectCount;
    GameEffect effects[8];
};

struct OverlappingEffectsComponent
{
    u32 effectCount;
    GameEffect effects[8];
};