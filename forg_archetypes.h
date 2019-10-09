#pragma once
Archetype() struct AnimalArchetype
{
#ifdef FORG_SERVER
    PhysicComponent physic;
    PlayerComponent* player;
#else
    BaseComponent base;
    AnimationComponent animation;
#endif
};

Archetype() struct RockArchetype
{
#ifdef FORG_SERVER
    PhysicComponent physic;
#else
    BaseComponent base;
    RockComponent rock;
    ImageComponent image;
#endif
};

Archetype() struct PlantArchetype
{
#ifdef FORG_SERVER
    PhysicComponent physic;
    PlayerComponent* player;
#else
    BaseComponent base;
    PlantComponent rock;
    ImageComponent image;
#endif
};

Archetype() struct GrassArchetype
{
#ifdef FORG_SERVER
    PhysicComponent physic;
    PlayerComponent* player;
#else
    BaseComponent base;
    GrassComponent rock;
    ImageComponent image;
#endif
};

introspection() struct CommonEntityInitParams
{
    Vec3 boundOffset;
    Vec3 boundDim MetaDefault("V3(1, 1, 1)");
};

introspection() struct ServerEntityInitParams
{
    UniversePos P MetaUneditable();
    u32 seed MetaUneditable();
    AssetID definitionID MetaUneditable();
};

introspection() struct ImageProperty
{
    b32 optional;
    GameProperty property;
};

introspection() struct ImageProperties
{
    GameAssetType imageType MetaDefault("{AssetType_Image, 0}") MetaFixed(type);
    ArrayCounter propertyCount MetaCounter(properties);
    ImageProperty* properties;
};

introspection() struct ClientEntityInitParams
{
    EntityID ID MetaUneditable();
    u32 seed MetaUneditable();
    
    
    Enumerator skeleton;
    Enumerator skin;
    
    ImageProperties entityProperties;
    ImageProperties leafProperties;
};

#define INIT_ENTITY(name) inline void Init##name(void* state, EntityID ID, CommonEntityInitParams* com, void* par)
typedef INIT_ENTITY(entity);

introspection() struct EntityDefinition
{
    Enumerator archetype MetaEnumerator(EntityArchetype);
    
    CommonEntityInitParams common;
    ServerEntityInitParams server;
    ClientEntityInitParams client;
};