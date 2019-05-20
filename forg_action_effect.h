#pragma once

#define MAXIMUM_ACTION_DISTANCE 1.0f

struct MemoryNewNodeParams
{
    b32 valid;
};

printTable(noPrefix) enum EntityAction
{
    Action_None,
    Action_Move,
    Action_Examine,
    Action_Attack,
    Action_Cast,
    Action_Eat,
    Action_Drink,
    Action_Pick,
    Action_Equip,
    Action_Open,
    Action_Craft,
    Action_Die,
    
    Action_Count,
};


printTable(noPrefix) enum EffectIdentifier
{
    Effect_Damage,
    Effect_FireDamage,
    Effect_NakedHandsDamage,
    Effect_Spawn,
    Effect_RestoreLifePoints,
};

printTable(noPrefix) enum EffectTargetRangeType
{
    EffectTarget_Target,
    EffectTarget_Actor,
    EffectTarget_AllInActorRange,
    EffectTarget_AllInTargetRange,
};

struct EffectTargetRange
{
    EffectTargetRangeType type;
    r32 radious;
};


struct NakedHandReq
{
    u8 slotIndex;
    u32 taxonomy;
    
    union
    {
        NakedHandReq* next;
        NakedHandReq* nextFree;
    };
};

union EffectData
{
	u32 taxonomy;
};

printFlags(noPrefix) enum EffectFlags
{
    Effect_Target = (1 << 1),
    Effect_ResetAfterTimer = (1 << 2),
    Effect_SendToClientOnTrigger = (1 << 3),
    Effect_SendToClientOnDelete = (1 << 4),
};

struct Effect
{
    EntityAction triggerAction;
    u32 flags;
    
    EffectIdentifier ID;
    EffectData data;
    EffectTargetRange range;
    
    r32 basePower;
    
    r32 timer;
    r32 targetTimer;
};
