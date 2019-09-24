#pragma once
struct ArchetypeComponent
{
    b32 exists;
    b32 pointer;
    u32 offset;
};

struct ArchetypeLayout
{
    u32 totalSize;
    
    ArchetypeComponent hasBaseComponent;
    ArchetypeComponent hasAnimationComponent;
    ArchetypeComponent hasPhysicComponent;
    ArchetypeComponent hasServerAnimationComponent;
    ArchetypeComponent hasPlayerComponent;
    ArchetypeComponent hasOptionalComponent;
};

#define HasComponent(arch, component) archetypeLayouts[arch].has##component.exists
#define GetPtr(state, ID) Get_(state->archetypes + ID.archetype, ID.archetypeIndex)
#define GetComponent(state, ID, component) HasComponent(ID.archetype, component) ?((archetypeLayouts[ID.archetype].has##component.pointer) ? (component*) (*(u64*) AdvanceVoidPtrBytes(GetPtr(state, ID), archetypeLayouts[ID.archetype].has##component.offset)) : (component*) AdvanceVoidPtrBytes(GetPtr(state, ID), archetypeLayouts[ID.archetype].has##component.offset)) : 0

#define GetComponentPtr(state, ID, component) AdvanceVoidPtrBytes(GetPtr(state, ID), archetypeLayouts[ID.archetype].has##component.offset)

#define SetComponent(state, ID, component, value) if(HasComponent(ID.archetype, component)){*(u64*) GetComponentPtr(state, ID, component) = (u64) value;}else{InvalidCodePath;}

#define InitArchetype(state, pool, arch, maxCount) state->archetypes[arch] = InitResizableArray_(pool, archetypeLayouts[arch].totalSize, maxCount)
#define InitComponentArray(state, pool, component, maxCount) state->component##_ = InitResizableArray_(pool, sizeof(component), maxCount)
#define First(state, arch) First_(&state->archetypes[arch], arch)
#define FirstComponent(state, component) FirstComponent_(&state->component##_)
#define GetComponentRaw(state, iter, component) (component*) Get_(&state->component##_, iter.index)
#define AcquireComponent(state, component, idAddress) (component*) Acquire_(&state->component##_, idAddress)

#define Acquire(state, arch, idAddress) idAddress->archetype = arch; Acquire_(&state->archetypes[arch], &(idAddress)->archetypeIndex)
#define GetOrAcquire(state, ID) GetOrAcquire_(&state->archetypes[ID.archetype], ID.archetypeIndex) 


struct EntityID
{
    u16 archetype;
    u32 archetypeIndex;
};

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
    result.ID.archetype = archetype;
    result.ID.archetypeIndex = 1;
    
    return result;
}

inline ArchIterator Next(ArchIterator iter)
{
    ArchIterator result = iter;
    ++result.ID.archetypeIndex;
    return result;
}

inline b32 IsValid(ArchIterator iter)
{
    b32 result = (iter.ID.archetypeIndex < iter.count);
    return result;
}

inline b32 IsValid(EntityID ID)
{
    b32 result = (ID.archetypeIndex > 0);
    return result;
}

inline b32 AreEqual(EntityID i1, EntityID i2)
{
    b32 result = (i1.archetype == i2.archetype && i1.archetypeIndex == i2.archetypeIndex);
    return result;
}
