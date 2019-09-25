#pragma once
Archetype() struct FirstEntityArchetype
{
#ifdef FORG_SERVER
    PhysicComponent physic;
    ServerAnimationComponent animation;
    PlayerComponent* player;
#else
    BaseComponent base;
    AnimationComponent animation;
#endif
};

Archetype() struct SecondEntityArchetype
{
#ifdef FORG_SERVER
    PhysicComponent physic;
    ServerAnimationComponent animation;
    OptionalComponent optional;
    PlayerComponent* player;
#else
    BaseComponent base;
    AnimationComponent animation;
#endif
};

introspection() struct EntityInitParams
{
    Enumerator skeleton MetaEnumerator(AssetSkeletonType);
    Enumerator skin MetaEnumerator(AssetImageType);
};

#define INIT_ENTITY(name) inline void Init##name(void* state, EntityID ID, EntityInitParams* params)
typedef INIT_ENTITY(entity);

introspection() struct EntityDefinition
{
    Enumerator archetype MetaEnumerator(EntityArchetype);
    EntityInitParams params;
};
