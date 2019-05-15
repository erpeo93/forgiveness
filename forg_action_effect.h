#pragma once

#define MAXIMUM_ACTION_DISTANCE 1.0f
enum StimulusType
{
    Stimulus_None,
    Stimulus_Presence,
    Stimulus_Ate,
    
    Stimulus_Count,
};

struct MemoryNewNodeParams
{
    b32 valid;
};

struct StimulusReaction
{
    u32 newState;
    
    u32 parameterCount;
    RuleParameter parameters[4];
    
    MemoryNewNodeParams memoryNode;
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

struct EffectData
{
	u32 taxonomy;
	union
	{
        r32 power;
    };
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
    u32 taxonomy;
    EffectData data;
    
    r32 timer;
    r32 targetTimer;
};
