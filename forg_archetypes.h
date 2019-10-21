#pragma once
Archetype() struct AnimalArchetype
{
#ifdef FORG_SERVER
    PhysicComponent physic;
    PlayerComponent* player;
    EquipmentComponent equipment;
    UsingComponent equipped;
#else
    BaseComponent base;
    AnimationComponent animation;
    EquipmentMappingComponent equipment;
    UsingMappingComponent equipped;
    AnimationEffectsComponent animationEffects;
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

Archetype() struct ObjectArchetype
{
#ifdef FORG_SERVER
    PhysicComponent physic;
    PlayerComponent* player;
    EffectComponent effect;
#else
    BaseComponent base;
    LayoutComponent layout;
#endif
};

introspection() struct CommonEntityInitParams
{
    Vec3 boundOffset;
    Vec3 boundDim MetaDefault("V3(1, 1, 1)");
    
    ArrayCounter possibleCraftingEffects MetaCounter(craftingEffects);
    EffectBinding* craftingEffects;
};

introspection() struct ServerEntityInitParams
{
    UniversePos P MetaUneditable();
    u32 seed MetaUneditable();
    EntityRef definitionID MetaUneditable();
    
    ArrayCounter bindingCount MetaCounter(bindings);
    EffectBinding* bindings;
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
    r32 height;
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
    
    Vec3 shadowOffset;
    Vec2 shadowScale MetaDefault("V2(1, 1)");
    Vec4 shadowColor MetaDefault("V4(1, 1, 1, 0.5f)");
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
