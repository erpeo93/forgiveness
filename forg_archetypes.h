#pragma once
Archetype() struct AnimalArchetype
{
#ifdef FORG_SERVER
    PhysicComponent physic;
    PlayerComponent* player;
    EquipmentComponent equipment;
    UsingComponent equipped;
    InteractionComponent interaction;
#else
    BaseComponent base;
    AnimationComponent animation;
    EquipmentMappingComponent equipment;
    UsingMappingComponent equipped;
    AnimationEffectsComponent animationEffects;
    InteractionComponent interaction;
#endif
};

Archetype() struct RockArchetype
{
#ifdef FORG_SERVER
    PhysicComponent physic;
    InteractionComponent interaction;
#else
    BaseComponent base;
    RockComponent rock;
    StandardImageComponent image;
    InteractionComponent interaction;
#endif
};

Archetype() struct PlantArchetype
{
#ifdef FORG_SERVER
    PhysicComponent physic;
    PlayerComponent* player;
    InteractionComponent interaction;
#else
    BaseComponent base;
    PlantComponent plant;
    StandardImageComponent image;
    InteractionComponent interaction;
#endif
};

Archetype() struct GrassArchetype
{
#ifdef FORG_SERVER
    PhysicComponent physic;
    PlayerComponent* player;
#else
    BaseComponent base;
    GrassComponent grass;
    StandardImageComponent image;
#endif
};

Archetype() struct ObjectArchetype
{
#ifdef FORG_SERVER
    PhysicComponent physic;
    PlayerComponent* player;
    EffectComponent effect;
    InteractionComponent interaction;
#else
    BaseComponent base;
    LayoutComponent layout;
    InteractionComponent interaction;
#endif
};

Archetype() struct ContainerArchetype
{
#ifdef FORG_SERVER
    PhysicComponent physic;
    EffectComponent effect;
    ContainerComponent container;
    InteractionComponent interaction;
#else
    BaseComponent base;
    LayoutComponent layout;
    ContainerMappingComponent container;
    InteractionComponent interaction;
#endif
};

Archetype() struct PortalArchetype
{
#ifdef FORG_SERVER
    PhysicComponent physic;
    CollisionEffectsComponent collision;
#else
    BaseComponent base;
    StandardImageComponent image;
#endif
};

introspection() struct CommonEntityInitParams
{
    EntityRef definitionID MetaUneditable();
    
    Vec3 boundOffset;
    Vec3 boundDim MetaDefault("V3(1, 1, 1)");
    
    
    ArrayCounter groundActionCount MetaCounter("groundActions");
    Enumerator* groundActions MetaEnumerator("action");
    
    ArrayCounter equipmentActionCount MetaCounter("equipmentActions");
    Enumerator* equipmentActions MetaEnumerator("action");
    
    ArrayCounter containerActionCount MetaCounter("containerActions");
    Enumerator* containerActions MetaEnumerator("action");
    
    ArrayCounter equippedActionCount MetaCounter("equippedActions");
    Enumerator* equippedActions MetaEnumerator("action");
};

introspection() struct ServerEntityInitParams
{
    UniversePos P MetaUneditable();
    u32 seed MetaUneditable();
    
    ArrayCounter bindingCount MetaCounter(bindings);
    EffectBinding* bindings;
    
    ArrayCounter collisionEffectsCount MetaCounter(collisionEffects);
    GameEffect* collisionEffects;
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

introspection() struct LayoutPieceProperties
{
    AssetLabel name;
    r32 height MetaDefault("1.0f");
    ImageProperties properties;
};

introspection() struct ClientEntityInitParams
{
    EntityID ID MetaUneditable();
    u32 seed MetaUneditable();
    
    GameAssetType skeleton MetaDefault("{AssetType_Skeleton, 0}") MetaFixed(type);
    GameAssetType skin MetaDefault("{AssetType_Image, 0}") MetaFixed(type);
    
    ImageProperties entityProperties;
    ImageProperties leafProperties;
    
    AssetLabel name;
    
    AssetLabel layoutRootName;
    ArrayCounter pieceCount MetaCounter(layoutPieces);
    LayoutPieceProperties* layoutPieces;
    
    ArrayCounter openPieceCount MetaCounter(openLayoutPieces);
    LayoutPieceProperties* openLayoutPieces;
    
    Vec3 shadowOffset;
    Vec2 shadowScale MetaDefault("V2(1, 1)");
    Vec4 shadowColor MetaDefault("V4(1, 1, 1, 0.5f)");
    
    r32 lootingZoomCoeff MetaDefault("3.0f");
    Vec2 desiredOpenedDim MetaDefault("V2(400, 400)");
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
