#pragma once

enum MetaType
{
    MetaType_None,
    MetaType_WorldChunk,
    MetaType_u32,
    MetaType_u8,
    MetaType_u64,
    MetaType_i32,
    MetaType_Vec2,
    MetaType_Vec3,
    MetaType_r32,
    MetaType_RigidBody,
    MetaType_b32,
    MetaType_ActionSlot,
    MetaType_SimEntity,
    MetaType_UniversePos,
    MetaType_TempAttributeEffect,
    MetaType_TempDisableActionEffect,
    MetaType_Rect2,
    MetaType_Rect3,
    MetaType_AdiacentWorld,
    MetaType_SimRegion,
    MetaType_Effect,
    MetaType_World,
    MetaType_VisiblePiece,
    MetaType_Object,
    MetaType_RegionWorkContext,
    MetaType_Container,
    MetaType_StoreResult,
    MetaType_ServerState,
    MetaType_Brain,
    MetaType_Entity,
    MetaType_union,
    MetaType_PerceivableInfo,
    MetaType_ContainedObjects,
    MetaType_TaxonomySlot,
    MetaType_Fluid,
    MetaType_PlantLifeStatus,
    MetaType_StimulusType,
    MetaType_SkillSlot,
    MetaType_Owner,
    MetaType_EssenceSlot,
    MetaType_ForgBoundType,
    MetaType_EffectComponent,
    MetaType_PlantComponent,
    MetaType_ObjectComponent,
    MetaType_FluidComponent,
    MetaType_CreatureComponent,
    MetaType_EquipmentSlot,
    MetaType_EntityAction,
    MetaType_PassiveSkillSlot,
    MetaType_PassiveSkillEffects,
    MetaType_Recipe,
};

enum MemberMetaFlags
{
    MetaFlag_Pointer = ( 1 << 0 ),
};

struct MemberDefinition
{
    //u32 flags;
    MetaType type;
    char* name;
    u32 offset;
};

struct MetaFlag
{
	char* name;
	u32 value;
};
