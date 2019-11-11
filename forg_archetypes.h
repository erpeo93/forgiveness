#pragma once
Archetype() struct AnimalArchetype
{
#ifdef FORG_SERVER
    PhysicComponent physic;
    PlayerComponent* player;
    EquipmentComponent equipment;
    UsingComponent equipped;
    InteractionComponent interaction;
    SkillComponent skills;
#else
    BaseComponent base;
    AnimationComponent animation;
    EquipmentMappingComponent equipment;
    UsingMappingComponent equipped;
    AnimationEffectsComponent animationEffects;
    InteractionComponent interaction;
    SkillMappingComponent skillMappings;
#endif
};

Archetype() struct RockArchetype
{
#ifdef FORG_SERVER
    PhysicComponent physic;
    InteractionComponent interaction;
#else
    BaseComponent base;
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
    BrainComponent brain;
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

Archetype() struct NullArchetype
{
    
};

introspection() struct UseLayout
{
    ArrayCounter slotCount MetaCounter("slots");
    Enumerator* slots MetaEnumerator("usingSlot");
};

introspection() struct EquipLayout
{
    ArrayCounter slotCount MetaCounter("slots");
    Enumerator* slots MetaEnumerator("equipmentSlot");
};

introspection() struct PossibleActionDefinition
{
    Enumerator action MetaEnumerator("action");
    r32 distance MetaDefault("1.0f");
};

introspection() struct CommonEntityInitParams
{
    EntityRef definitionID MetaUneditable();
    
    Vec3 boundOffset;
    Vec3 boundDim MetaDefault("V3(1, 1, 1)");
    
    ArrayCounter groundActionCount MetaCounter("groundActions");
    PossibleActionDefinition* groundActions;
    
    ArrayCounter equipmentActionCount MetaCounter("equipmentActions");
    PossibleActionDefinition* equipmentActions;
    
    ArrayCounter containerActionCount MetaCounter("containerActions");
    PossibleActionDefinition* containerActions;
    
    ArrayCounter equippedActionCount MetaCounter("equippedActions");
    PossibleActionDefinition* equippedActions;
    
    ArrayCounter usingConfigurationCount MetaCounter("usingConfigurations");
    UseLayout* usingConfigurations;
    
    ArrayCounter equipConfigurationCount MetaCounter("equipConfigurations");
    EquipLayout* equipConfigurations;
    
    Enumerator inventorySlotType MetaEnumerator("inventorySlotType");
};

introspection() struct ServerEntityInitParams
{
    UniversePos P MetaUneditable();
    u32 seed MetaUneditable();
    
    ArrayCounter bindingCount MetaCounter(bindings);
    EffectBinding* bindings;
    
    u16 storeCount;
    u16 specialStoreCount;
    
    u16 usingCount;
    u16 specialUsingCount;
    
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
    Enumerator inventorySlotType MetaEnumerator("inventorySlotType");
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
    
    ArrayCounter usingPieceCount MetaCounter(usingLayoutPieces);
    LayoutPieceProperties* usingLayoutPieces;
    
    Vec3 shadowOffset;
    Vec2 shadowScale MetaDefault("V2(1, 1)");
    Vec4 shadowColor MetaDefault("V4(1, 1, 1, 0.5f)");
    
    r32 lootingZoomCoeff MetaDefault("3.0f");
    Vec2 desiredOpenedDim MetaDefault("V2(400, 400)");
    Vec2 desiredUsingDim MetaDefault("V2(200, 200)");
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
