#pragma once
Archetype() struct AnimalArchetype
{
#ifdef FORG_SERVER
    DefaultComponent default;
    PhysicComponent physic;
    ActionComponent action;
    PlayerComponent* player;
    EquipmentComponent equipment;
    UsingComponent equipped;
    InteractionComponent interaction;
    BrainComponent brain;
    AliveComponent alive;
    MiscComponent misc;
#else
    BaseComponent base;
    AnimationComponent animation;
    EquipmentComponent equipment;
    UsingComponent equipped;
    InteractionComponent interaction;
    AnimationEffectComponent animationEffects;
    SoundEffectComponent soundEffects;
    AliveComponent alive;
    MiscComponent misc;
#endif
};

Archetype() struct RockArchetype
{
#ifdef FORG_SERVER
    DefaultComponent default;
    PhysicComponent physic;
    InteractionComponent interaction;
#else
    BaseComponent base;
    RockComponent rock;
    InteractionComponent interaction;
    AnimationEffectComponent animationEffects;
#endif
};

Archetype() struct PlantArchetype
{
#ifdef FORG_SERVER
    DefaultComponent default;
    PhysicComponent physic;
    PlayerComponent* player;
    InteractionComponent interaction;
#else
    BaseComponent base;
    PlantComponent plant;
    InteractionComponent interaction;
    AnimationEffectComponent animationEffects;
#endif
};

Archetype() struct GrassArchetype
{
#ifdef FORG_SERVER
    DefaultComponent default;
    StaticComponent staticUpdate;
#else
    BaseComponent base;
    GrassComponent grass;
    MagicQuadComponent image;
#endif
};

Archetype() struct RuneArchetype
{
#ifdef FORG_SERVER
    DefaultComponent default;
    PhysicComponent physic;
    SkillDefComponent skill;
    EffectComponent effect;
    ContainerComponent container;
    InteractionComponent interaction;
#else
    BaseComponent base;
    LayoutComponent layout;
    InteractionComponent interaction;
    AnimationEffectComponent animationEffects;
    ContainerComponent container;
    SkillDefComponent skill;
#endif
};

Archetype() struct EssenceArchetype
{
#ifdef FORG_SERVER
    DefaultComponent default;
    PhysicComponent physic;
    InteractionComponent interaction;
#else
    BaseComponent base;
    LayoutComponent layout;
    InteractionComponent interaction;
    AnimationEffectComponent animationEffects;
#endif
};

Archetype() struct ObjectArchetype
{
#ifdef FORG_SERVER
    DefaultComponent default;
    PhysicComponent physic;
    EffectComponent effect;
    ContainerComponent container;
    InteractionComponent interaction;
    MiscComponent misc;
#else
    BaseComponent base;
    LayoutComponent layout;
    ContainerComponent container;
    InteractionComponent interaction;
    AnimationEffectComponent animationEffects;
    RecipeEssenceComponent recipeEssences;
    MiscComponent misc;
#endif
};

Archetype() struct PortalArchetype
{
#ifdef FORG_SERVER
    DefaultComponent default;
    PhysicComponent physic;
    CollisionEffectsComponent collision;
#else
    BaseComponent base;
    FrameByFrameAnimationComponent animation;
    AnimationEffectComponent animationEffects;
#endif
};

Archetype() struct ProjectileArchetype
{
#ifdef FORG_SERVER
    DefaultComponent default;
    PhysicComponent physic;
    TempEntityComponent temp;
    CollisionEffectsComponent collision;
#else
    BaseComponent base;
    SegmentImageComponent image;
    AnimationEffectComponent animationEffects;
#endif
};

Archetype() struct PlaceholderArchetype
{
#ifdef FORG_SERVER
    DefaultComponent default;
    PhysicComponent physic;
    PlayerComponent* player;
#else
    BaseComponent base;
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
    r32 continueDistanceCoeff MetaDefault("1.0f");
    GameProperty special MetaDefault("{Property_specialPropertyType, Special_Invalid}") MetaFixed(property);
    r32 time;
    EntityRef requiredUsingType;
    EntityRef requiredEquippedType;
};

introspection() struct CraftingComponent
{
    ArrayCounter optionCount MetaCounter(options);
    EntityRef* options;
    b32 deleteAfterCrafting MetaDefault("true");
};

introspection() struct CommonEntityInitParams
{
    EntityRef definitionID MetaUneditable();
    u16* essences MetaUneditable();
    
    b32 craftable;
    u16 essenceCountRef;
    u16 essenceCountV;
    
    ArrayCounter componentCount MetaCounter(components);
    CraftingComponent* components;
    
    GameProperty boundType MetaDefault("{Property_boundType, bound_invalid}") MetaFixed(property);
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
    
    ArrayCounter draggingActionCount MetaCounter("draggingActions");
    PossibleActionDefinition* draggingActions;
    
    
    ArrayCounter usingConfigurationCount MetaCounter("usingConfigurations");
    UseLayout* usingConfigurations;
    
    ArrayCounter equipConfigurationCount MetaCounter("equipConfigurations");
    EquipLayout* equipConfigurations;
    
    Enumerator inventorySlotType MetaEnumerator("inventorySlotType");
    
    b32 targetSkill;
    b32 passive;
};

introspection() struct InventorySlots
{
    GameProperty type MetaDefault("{Property_inventorySlotType, InventorySlot_Standard}") MetaFixed(property);
    u16 count;
};

introspection() struct ServerEntityInitParams
{
    UniversePos P MetaUneditable();
    u32 seed MetaUneditable();
    
    Vec3 startingAcceleration MetaUneditable();
    Vec3 startingSpeed MetaUneditable();
    
    r32 accelerationCoeff MetaDefault("27.0f");
    r32 drag MetaDefault("-7.8f");
    
    ArrayCounter bindingCount MetaCounter(bindings);
    EffectBinding* bindings;
    
    
    ArrayCounter storeSlotCounter MetaCounter(storeSlots);
    InventorySlots* storeSlots;
    
    ArrayCounter usingSlotCounter MetaCounter(usingSlots);
    InventorySlots* usingSlots;
    
    
    ArrayCounter collisionEffectsCount MetaCounter(collisionEffects);
    GameEffect* collisionEffects;
    
    Enumerator brainType MetaEnumerator("brainType");
    BrainParams brainParams;
    
    b32 canGoIntoWater;
    b32 fearsLight;
    r32 lightRadious;
    
    u32 defaultEssenceCount;
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

introspection() struct MultipartFrameByFramePiece
{
    r32 speed MetaDefault("1.0f");
    GameAssetType image MetaDefault("{AssetType_Image, 0}") MetaFixed(type);
};

introspection() struct MultipartStaticPiece
{
    ImageProperties properties;
};

introspection() struct ClientEntityInitParams
{
    EntityID ID MetaUneditable();
    u32 seed MetaUneditable();
    
    GameAssetType skeleton MetaDefault("{AssetType_Skeleton, 0}") MetaFixed(type);
    GameAssetType skin MetaDefault("{AssetType_Image, 0}") MetaFixed(type);
    
    ImageProperties entityProperties;
    
    
    b32 hasBranchVariant;
    Color branchColor MetaDefault("V4(1, 1, 1, 1)");
    Vec4 branchColorV;
    ImageProperties trunkProperties;
    ImageProperties branchProperties;
    
    b32 hasLeafVariant;
    Color leafColor MetaDefault("V4(1, 1, 1, 1)");
    Vec4 leafColorV;
    ImageProperties leafProperties;
    
    b32 hasFlowerVariant;
    Color flowerColor MetaDefault("V4(1, 1, 1, 1)");
    Vec4 flowerColorV;
    ImageProperties flowerProperties;
    
    b32 hasFruitVariant;
    Color fruitColor MetaDefault("V4(1, 1, 1, 1)");
    Vec4 fruitColorV;
    ImageProperties fruitProperties;
    
    ImageProperties rockProperties;
    ImageProperties mineralProperties;
    Color rockColor MetaDefault("V4(1, 1, 1, 1)");
    Vec4 rockColorV;
    
    r32 windInfluence MetaDefault("0");
    u32 windFrequencyStandard MetaDefault("1");
    u32 windFrequencyOverlap MetaDefault("10");
    
    u32 instanceCount MetaDefault("1");
    Vec3 instanceMaxOffset;
    Vec3 grassBounds MetaDefault("V3(1, 1, 1)");
    
    AssetLabel name;
    
    r32 slotZoomSpeed MetaDefault("1.0f");
    r32 maxSlotZoom MetaDefault("1.0f");
    
    AssetLabel layoutRootName;
    ArrayCounter pieceCount MetaCounter(layoutPieces);
    LayoutPieceProperties* layoutPieces;
    
    ArrayCounter openPieceCount MetaCounter(openLayoutPieces);
    LayoutPieceProperties* openLayoutPieces;
    
    ArrayCounter usingPieceCount MetaCounter(usingLayoutPieces);
    LayoutPieceProperties* usingLayoutPieces;
    
    ArrayCounter equippedPieceCount MetaCounter(equippedLayoutPieces);
    LayoutPieceProperties* equippedLayoutPieces;
    
    Vec3 shadowOffset;
    Vec2 shadowScale MetaDefault("V2(1, 1)");
    Vec4 shadowColor MetaDefault("V4(1, 1, 1, 0.5f)");
    
    r32 lootingZoomCoeff MetaDefault("3.0f");
    b32 displayInStandardMode;
    Vec2 desiredOpenedDim MetaDefault("V2(400, 400)");
    Vec2 desiredUsingDim MetaDefault("V2(200, 200)");
    
    ArrayCounter animationEffectsCount MetaCounter(animationEffects);
    AnimationEffectDefinition* animationEffects;
    
    r32 frameByFrameSpeed MetaDefault("1.0f");
    GameAssetType frameByFrameImageType MetaDefault("{AssetType_Image, 0}") MetaFixed(type);
    b32 frameByFrameOverridesPivot;
    Vec2 frameByFramePivot;
    
	ArrayCounter multipartStaticCount MetaCounter(multipartStaticPieces);
	MultipartStaticPiece* multipartStaticPieces;
    
    ArrayCounter multipartFrameByFrameCount MetaCounter(multipartFrameByFramePieces);
	MultipartFrameByFramePiece* multipartFrameByFramePieces;
    
    ArrayCounter replacementCount MetaCounter(animationReplacements);
    AnimationReplacement* animationReplacements;
    
    r32 fadeInTime MetaDefault("1.0f");
    r32 fadeOutTime MetaDefault("1.0f");
    
    ArrayCounter soundEffectsCount MetaCounter(soundEffects);
    SoundEffectDefinition* soundEffects;
    
    Vec3 spawnProjectileOffset;
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
