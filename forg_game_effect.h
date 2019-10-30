#pragma once
introspection() struct GameEffect
{
    r32 timer MetaDefault("1");
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

struct PossibleActionList
{
    u16 actionCount;
    u16 actions[8];
};


enum InteractionType
{
    Interaction_Ground,
    Interaction_Equipment,
    Interaction_Container,
    Interaction_Equipped,
    
    Interaction_Count
};

struct InteractionComponent
{
    b32 isOnFocus;
    PossibleActionList actions[Interaction_Count];
};