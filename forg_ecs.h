#pragma once
introspection() struct EntityID
{
    u32 archetype_archetypeIndex; // NOTE(Leonardo): 8 bit archetype, 24 bits index
};

#define INIT_COMPONENT_FUNCTION(name) void name(void* state, void* componentPtr, EntityID ID, struct CommonEntityInitParams* common, struct ServerEntityInitParams* s, struct ClientEntityInitParams* c)
typedef INIT_COMPONENT_FUNCTION(init_component_function);

struct ArchetypeComponent
{
    b32 exists;
    b32 pointer;
    u32 offset;
    init_component_function* init;
};

struct ArchetypeLayout
{
    u32 totalSize;
    union
    {
        ArchetypeComponent hasComponents[256];
        struct
        {
            ArchetypeComponent hasBaseComponent;
            ArchetypeComponent hasAnimationComponent;
            ArchetypeComponent hasRockComponent;
            ArchetypeComponent hasPlantComponent;
            ArchetypeComponent hasGrassComponent;
            ArchetypeComponent hasPhysicComponent;
            ArchetypeComponent hasPlayerComponent;
            ArchetypeComponent hasStandardImageComponent;
            ArchetypeComponent hasLayoutComponent;
            ArchetypeComponent hasEffectComponent;
            ArchetypeComponent hasEquipmentComponent;
            ArchetypeComponent hasUsingComponent;
            ArchetypeComponent hasEquipmentMappingComponent;
            ArchetypeComponent hasUsingMappingComponent;
            ArchetypeComponent hasAnimationEffectsComponent;
            ArchetypeComponent hasCollisionEffectsComponent;
            ArchetypeComponent hasOverlappingEffectsComponent;
            ArchetypeComponent hasContainerComponent;
            ArchetypeComponent hasContainerMappingComponent;
        };
    };
};

#define HasComponent_(arch, component) archetypeLayouts[arch].has##component.exists
#define HasComponent(ID, component) HasComponent_(GetArchetype(ID), component)
#define GetPtr(state, ID) Get_(state->archetypes + GetArchetype(ID), GetArchetypeIndex(ID))
#define GetComponent(state, ID, component) HasComponent(ID, component) ?((archetypeLayouts[GetArchetype(ID)].has##component.pointer) ? (component*) (*(u64*) AdvanceVoidPtrBytes(GetPtr(state, ID), archetypeLayouts[GetArchetype(ID)].has##component.offset)) : (component*) AdvanceVoidPtrBytes(GetPtr(state, ID), archetypeLayouts[GetArchetype(ID)].has##component.offset)) : 0

#define GetComponentPtr(state, ID, component) AdvanceVoidPtrBytes(GetPtr(state, ID), archetypeLayouts[GetArchetype(ID)].has##component.offset)

#define SetComponent(state, ID, component, value) if(HasComponent(ID, component)){*(u64*) GetComponentPtr(state, ID, component) = (u64) value;}else{InvalidCodePath;}

#define InitArchetype(state, arch, maxCount) state->archetypes[arch] = InitResizableArray_(archetypeLayouts[arch].totalSize, maxCount)
#define InitComponentArray(state, component, maxCount) state->component##_ = InitResizableArray_(sizeof(component), maxCount)
#define First(state, arch) First_(&state->archetypes[arch], arch)
#define FirstComponent(state, component) FirstComponent_(&state->component##_)
#define GetComponentRaw(state, iter, component) (component*) Get_(&state->component##_, iter.index)
#define AcquireComponent(state, component, idAddress) (component*) Acquire_(&state->component##_, idAddress, 0xffffff)
#define FreeComponent(state, component, id) Free_(&state->component##_, id)

#define AcquireArchetype(state, arch, idAddress) SetArchetype(idAddress, arch); Acquire_(&state->archetypes[arch], &(idAddress)->archetype_archetypeIndex, 0xffffff)
#define FreeArchetype(state, idAddress) Free_(&state->archetypes[GetArchetype(*idAddress)], GetArchetypeIndex(*idAddress))
#define DeletedArchetype(state, id) Deleted_(&state->archetypes[GetArchetype(id)], GetArchetypeIndex(id))


inline EntityID BuildEntityID(u16 archetype, u32 archetypeIndex)
{
    EntityID result;
    Assert(archetypeIndex <= 0xffffff);
    result.archetype_archetypeIndex = ((u32) SafeTruncateToU8(archetype) << 24) | archetypeIndex;
    return result;
}

inline u8 GetArchetype(EntityID ID)
{
    u8 result = SafeTruncateToU8(ID.archetype_archetypeIndex >> 24);
    return result;
}

inline void SetArchetype(EntityID* ID, u8 archetype)
{
    ID->archetype_archetypeIndex |= (((u32) archetype) << 24);
}

inline u32 GetArchetypeIndex(EntityID ID)
{
    u32 result = (ID.archetype_archetypeIndex & 0xffffff);
    return result;
}


struct CompIterator
{
    u32 count;
    u32 index;
};

inline CompIterator FirstComponent_(ResizableArray* array)
{
    CompIterator result = {};
    result.count = array->count;
    result.index = 1;
    
    return result;
}

inline CompIterator Next(CompIterator iter)
{
    CompIterator result = iter;
    ++result.index;
    return result;
}

inline b32 IsValid(CompIterator iter)
{
    b32 result = (iter.index < iter.count);
    return result;
}

struct ArchIterator
{
    u32 count;
    EntityID ID;
};

inline ArchIterator First_(ResizableArray* array, u16 archetype)
{
    ArchIterator result = {};
    result.count = array->count;
    result.ID = BuildEntityID(archetype, 1);
    
    return result;
}

inline ArchIterator Next(ArchIterator iter)
{
    ArchIterator result = iter;
    ++result.ID.archetype_archetypeIndex;
    return result;
}

inline b32 IsValid(ArchIterator iter)
{
    b32 result = (GetArchetypeIndex(iter.ID) < iter.count);
    return result;
}

inline b32 IsValid(EntityID ID)
{
    b32 result = (GetArchetypeIndex(ID) > 0);
    return result;
}

inline b32 AreEqual(EntityID i1, EntityID i2)
{
    b32 result = (i1.archetype_archetypeIndex == i2.archetype_archetypeIndex);
    return result;
}


#define STANDARD_ECS_JOB_SERVER(name) internal void name(ServerState* server, EntityID ID, r32 elapsedTime)
#define STANDARD_ECS_JOB_CLIENT(name) internal void name(GameModeWorld* worldMode, EntityID ID, r32 elapsedTime)
#define RENDERING_ECS_JOB_CLIENT(name) internal void name(GameModeWorld* worldMode, RenderGroup* group, EntityID ID, r32 elapsedTime)
#define INTERACTION_ECS_JOB_CLIENT(name) internal void name(GameModeWorld* worldMode, RenderGroup* group, PlatformInput* input, EntityID ID, r32 elapsedTime)


#define ArchetypeHas(component) HasComponent_(archetypeIndex, component)
#define EXECUTE_JOB(state, job, query, elapsedTime)\
for(u16 archetypeIndex = 0; archetypeIndex < Archetype_Count; ++archetypeIndex)\
{\
    if(query)\
    {\
        for(ArchIterator iter = First(state, archetypeIndex); \
        IsValid(iter); \
        iter = Next(iter))\
        {\
            if(!DeletedArchetype(state, iter.ID)) job(state, iter.ID, elapsedTime);\
        }\
    }\
}

#define EXECUTE_RENDERING_JOB(state, group, job, query, elapsedTime)\
for(u16 archetypeIndex = 0; archetypeIndex < Archetype_Count; ++archetypeIndex)\
{\
    if(query)\
    {\
        for(ArchIterator iter = First(state, archetypeIndex); \
        IsValid(iter); \
        iter = Next(iter))\
        {\
            if(!DeletedArchetype(state, iter.ID)) job(state, group, iter.ID, elapsedTime);\
        }\
    }\
}

#define EXECUTE_INTERACTION_JOB(state, group, input, job, query, elapsedTime)\
for(u16 archetypeIndex = 0; archetypeIndex < Archetype_Count; ++archetypeIndex)\
{\
    if(query)\
    {\
        for(ArchIterator iter = First(state, archetypeIndex); \
        IsValid(iter); \
        iter = Next(iter))\
        {\
            if(!DeletedArchetype(state, iter.ID)) job(state, group, input, iter.ID, elapsedTime);\
        }\
    }\
}