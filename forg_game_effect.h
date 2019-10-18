#pragma once

struct GameEffect
{
    u16 type;
};

struct EffectBinding
{
    u16 essenceType;
    GameEffect effect;
};

struct EffectComponent
{
    u32 effectCount;
    GameEffect effects[8];
};