#pragma once

struct MemoryNewNodeParams
{
    b32 valid;
};

printTable(noPrefix) enum EntityAction
{
    Action_None,
    Action_Idle,
    Action_Move,
    Action_Protecting,
    Action_Rolling,
    Action_Attack,
    Action_Drag,
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

enum PossibleActionType
{
    PossibleAction_CantBeDone,
    PossibleAction_TooFar,
    PossibleAction_CanBeDone,
};

printTable(noPrefix) enum EffectIdentifier
{
    Effect_Damage,
    Effect_FireDamage,
    Effect_NakedHandsDamage,
    Effect_Spawn,
    Effect_RestoreLifePoints,
    Effect_SpawnRock,
    Effect_SpawnTree,
    Effect_SpawnCreature,
    Effect_SpawnObject,
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

struct EffectData
{
	u32 taxonomy;
    u32 spawnCount;
    r32 timeToLive;
    Vec3 offset;
    Vec3 offsetV;
    Vec3 speed;
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
    
    r32 minIntensity;
    r32 maxIntensity;
    
    r32 timer;
    r32 targetTimer;
};


#define MAX_CONTAINER_INTERACTION_REQUIRED 6
#define MAX_CONTAINER_INTERACTION_EFFECTS 4
struct ContainerInteraction
{
    b32 valid;
    
    r32 validTime;
    r32 targetTime;
    
    u32 requiredCount;
    u32 requiredTaxonomies[MAX_CONTAINER_INTERACTION_REQUIRED];
    u32 containerIndexes[MAX_CONTAINER_INTERACTION_REQUIRED];
    
    u32 effectCount;
    Effect effects[MAX_CONTAINER_INTERACTION_EFFECTS];
};
