enum Component
{
    Component_X,
    Component_Y,
    Component_Z,
    
    Component_Count,
};

struct ComponentArchetype
{
    ResizableComponentArray* components[Componentcount];
};

struct Job
{
    void* context; // NOTE(Leonardo): gamestate, serverstate....
    callback;
    requiredComponents;
};

struct ECS
{
    ComponentArchetype archetypes;
    Jobs;
};

internal void AddArchetype()
{
    
}

#pragma once
enum Entity_flags
{
    Flag_floating = (1 << 1),
    Flag_distanceLimit = (1 << 2),
    Flag_deleteWhenDistanceCovered = (1 << 3),
    Flag_insideRegion = (1 << 4),
    Flag_Attached = (1 << 5),
    Flag_deleted = (1 << 20),
};

inline void AddFlags(SimEntity* entity, u32 flags)
{
    entity->flags |= flags;
}

inline b32 IsSet(SimEntity* entity, i32 flag)
{
    b32 result = (entity->flags & flag);
    return result;
}

inline void ClearFlags(SimEntity* entity, i32 flags)
{
    entity->flags &= ~flags;
}

#ifdef FORG_SERVER

enum EntityComponentType
{
    Component_Effect,
    Component_Plant,
    Component_Container,
    Component_Creature,
    
    Component_Count,
};

struct SimEntity
{
    u32 taxonomy;
    r32 generationIntensity;
    u32 flags;
    u64 identifier;
    u32 playerID;
    Vec3 P;
    b32 flipOnYAxis;
    r32 facingDirection;
    Vec3 velocity;
    Vec3 acceleration;
    EntityAction action;
    r32 actionTime;
    u64 targetID;
    r32 distanceToTravel;
    ForgBoundType boundType;
    Rect3 bounds;
    r32 quantity;
    r32 status;
	u32 recipeTaxonomy;
    GenerationData gen;
    u64 ownerID;
    
    u32 IDs[Component_Count];
};


struct CreatureComponent
{
    EquipmentSlot equipment[Slot_Count];
    Brain brain;
    r32 skillCooldown; 
    r32 strength;
    r32 lifePoints;
    r32 maxLifePoints;
    
    r32 stamina;
    r32 maxStamina;
    
    u64 openedContainerID;
    u64 externalDraggingID;
    Vec3 customTargetP;
    SimEntity* draggingEntity;
    i32 activeSkillIndex;
    
    u32 skillCount;
    SkillSlot skills[256];
    
    u32 recipeCount;
    Recipe recipes[256];
    
    SkillSlot passiveSkills[MAX_PASSIVE_SKILLS_ACTIVE];
    PassiveSkillEffects passiveSkillEffects[MAX_PASSIVE_SKILLS_ACTIVE];
	EssenceSlot essences[MAX_DIFFERENT_ESSENCES];
    
    u8 startedAction;
    u64 startedActionTarget;
    
    u8 completedAction;
    u64 completedActionTarget;
    
    u32 nextFree;
};

struct EffectComponent
{
    u32 effectCount;
    Effect effects[16];
    
    u32 nextFree;
};

struct PlantComponent
{
	r32 age;
    r32 life;
    
    r32 leafDensity;
    r32 flowerDensity;
    r32 fruitDensity;
    
    u32 nextFree;
};

struct ContainerComponent
{
	ContainedObjects objects;
    
    ContainerInteraction insideInteraction;
    r32 updateTime;
    
    u32 nextFree;
};

#endif