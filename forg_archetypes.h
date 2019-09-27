#pragma once
Archetype() struct FirstEntityArchetype
{
#ifdef FORG_SERVER
    PhysicComponent physic;
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
    PlayerComponent* player;
#else
    BaseComponent base;
    AnimationComponent animation;
#endif
};

introspection() struct ServerEntityInitParams
{
    UniversePos P MetaUneditable();
    u32 seed MetaUneditable();
};

introspection() struct ClientEntityInitParams
{
    EntityID ID MetaUneditable();
    Enumerator skeleton MetaEnumerator(AssetSkeletonType);
    Enumerator skin MetaEnumerator(AssetImageType);
};

#define INIT_ENTITY(name) inline void Init##name(void* state, EntityID ID, void* par)
typedef INIT_ENTITY(entity);

introspection() struct EntityDefinition
{
    Enumerator archetype MetaEnumerator(EntityArchetype);
    
    ServerEntityInitParams server;
    ClientEntityInitParams client;
};