#pragma once
Archetype() struct AnimalArchetype
{
#ifdef FORG_SERVER
    DefaultComponent default;
    MovementComponent movement;
    PlayerComponent* player;
    BrainComponent brain;
    ReachableMapComponent reachableMap;
    ActiveEffectComponent effects;
#else
    BaseComponent base;
    AnimationComponent animation;
    AnimationEffectComponent animationEffects;
    SoundEffectComponent soundEffects;
    ShadowComponent shadow;
#endif
    UsingComponent equipped;
    EquipmentComponent equipment;
    InteractionComponent interaction;
    HealthComponent alive;
    CombatComponent combat;
    LightComponent light;
    ActionComponent action;
};

Archetype() struct RockArchetype
{
#ifdef FORG_SERVER
    DefaultComponent default;
    ActiveEffectComponent effect;
#else
    BaseComponent base;
    RockComponent rock;
    AnimationEffectComponent animationEffects;
    ShadowComponent shadow;
#endif
    InteractionComponent interaction;
};

Archetype() struct PlantArchetype
{
#ifdef FORG_SERVER
    DefaultComponent default;
    ActiveEffectComponent effect;
#else
    BaseComponent base;
    PlantComponent plant;
    AnimationEffectComponent animationEffects;
    ShadowComponent shadow;
#endif
    InteractionComponent interaction;
    VegetationComponent vegetation;
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
    ActiveEffectComponent effect;
#else
    BaseComponent base;
    LayoutComponent layout;
    AnimationEffectComponent animationEffects;
    ShadowComponent shadow;
#endif
    ContainerComponent container;
    InteractionComponent interaction;
    SkillDefComponent skill;
};

Archetype() struct EssenceArchetype
{
#ifdef FORG_SERVER
    DefaultComponent default;
    ActiveEffectComponent effect;
#else
    BaseComponent base;
    LayoutComponent layout;
    AnimationEffectComponent animationEffects;
    ShadowComponent shadow;
#endif
    InteractionComponent interaction;
    LightComponent light;
};

Archetype() struct ObjectArchetype
{
#ifdef FORG_SERVER
    DefaultComponent default;
    ActiveEffectComponent effect;
#else
    BaseComponent base;
    LayoutComponent layout;
    AnimationEffectComponent animationEffects;
    RecipeEssenceComponent recipeEssences;
    ShadowComponent shadow;
#endif
    ContainerComponent container;
    InteractionComponent interaction;
    LightComponent light;
};

Archetype() struct PortalArchetype
{
#ifdef FORG_SERVER
    DefaultComponent default;
    CollisionEffectsComponent collision;
#else
    BaseComponent base;
    FrameByFrameAnimationComponent animation;
    AnimationEffectComponent animationEffects;
    ShadowComponent shadow;
#endif
};

Archetype() struct ProjectileArchetype
{
#ifdef FORG_SERVER
    DefaultComponent default;
    MovementComponent movement;
    TempEntityComponent temp;
    CollisionEffectsComponent collision;
#else
    BaseComponent base;
    SegmentImageComponent image;
    AnimationEffectComponent animationEffects;
#endif
};

Archetype() struct LightArchetype
{
#ifdef FORG_SERVER
    DefaultComponent default;
#else
    BaseComponent base;
    StandardImageComponent image;
#endif
    LightComponent light;
};

Archetype() struct PatchArchetype
{
#ifdef FORG_SERVER
    DefaultComponent default;
    TempEntityComponent temp;
    OverlappingEffectsComponent overlap;
#else
    BaseComponent base;
    StandardImageComponent image;
    AnimationEffectComponent animationEffects;
#endif
};

Archetype() struct PlaceholderArchetype
{
#ifdef FORG_SERVER
    DefaultComponent default;
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
    EntityName requiredUsingType;
    EntityName requiredEquippedType;
};

introspection() struct CraftingComponent
{
    ArrayCounter optionCount MetaCounter(options);
    EntityName* options;
    b32 deleteAfterCrafting MetaDefault("true");
};

introspection() struct CommonEntityInitParams
{
    EntityType type MetaUneditable();
    u16* essences MetaUneditable();
    
    b32 craftable;
    u16 essenceCountRef;
    u16 essenceCountV;
    
    ArrayCounter componentCount MetaCounter(components);
    CraftingComponent* components;
    
    GameProperty boundType MetaDefault("{Property_boundType, bound_invalid}") MetaFixed(property);
    Vec3 boundOffset;
    Vec3 boundDim MetaDefault("V3(1, 1, 1)");
    Vec3 boundDimV;
    
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
    r32 requiredMentalHealthSafety;
    
    r32 flowerDensity MetaDefault("1.0f");
    r32 fruitDensity MetaDefault("1.0f");
    
    Vec3 lightColor MetaDefault("V3(1, 1, 1)");
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
    
    ArrayCounter overlappingEffectsCount MetaCounter(overlappingEffects);
    GameEffect* overlappingEffects;
    
    ArrayCounter defaultEffectsCount MetaCounter(defaultEffects);
    GameEffect* defaultEffects;
    
    ArrayCounter probabilityEffectsCount MetaCounter(probabilityEffects);
    ProbabilityEffect* probabilityEffects;
    
    
    Enumerator brainType MetaEnumerator("brainType");
    BrainParams brainParams;
    
    b32 canGoIntoWater;
    b32 fearsLight;
    r32 lightRadious;
    
    u32 defaultEssenceCount;
    
    
    r32 flowerGrowingSpeed;
    r32 requiredFlowerDensity MetaDefault("1.0f");
    
    r32 fruitGrowingSpeed;
    r32 requiredFruitDensity MetaDefault("1.0f");
    
    r32 maxPhysicalHealth MetaDefault("100.0f");
    r32 maxMentalHealth MetaDefault("100.0f");
    
    r32 physicalRegenerationPerSecond;
    r32 mentalRegenerationPerSecond;
    r32 fireDamagePerSecond;
    r32 poisonDamagePerSecond;
    
    r32 defaultAttackDistance MetaDefault("1.0f");
    r32 defaultAttackContinueCoeff MetaDefault("2.0f");
    r32 movementSpeedWhileAttacking;
    
    r32 defaultActionSpeed MetaDefault("1.0f");
    r32 targetTimeToLive MetaDefault("1.0f");
};

introspection() struct ImageProperty
{
    b32 optional;
    GameProperty property;
};

introspection() struct ImageProperties
{
    GameAssetType imageType MetaDefault("{AssetType_Image, 0}") MetaFixed(type);
    b32 emittors;
    b32 flat;
    ArrayCounter propertyCount MetaCounter(properties);
    ImageProperty* properties;
};

introspection() struct LayoutPieceProperties
{
    AssetLabel name;
    r32 height MetaDefault("1.0f");
    Color color MetaDefault("V4(1, 1, 1, 1)");
    ImageProperties properties;
    Enumerator inventorySlotType MetaEnumerator("inventorySlotType");
};

introspection() struct MultipartFrameByFramePiece
{
    r32 speed MetaDefault("1.0f");
    b32 emittors;
    GameAssetType image MetaDefault("{AssetType_Image, 0}") MetaFixed(type);
};

introspection() struct MultipartStaticPiece
{
    ImageProperties properties;
};

introspection() struct PossibleSkin
{
    GameAssetType skin MetaDefault("{AssetType_Image, 0}") MetaFixed(type);
};

introspection() struct Coloration
{
    ArrayCounter optionCount MetaCounter(options);
    Color* options;
};

introspection() struct ClientEntityInitParams
{
    EntityID ID MetaUneditable();
    u32 seed MetaUneditable();
    
    GameAssetType skeleton MetaDefault("{AssetType_Skeleton, 0}") MetaFixed(type);
    
    ArrayCounter possibleSkinCount MetaCounter(possibleSkins);
    PossibleSkin* possibleSkins;
    
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
    r32 leafWindInfluence MetaDefault("0");
    r32 flowerWindInfluence MetaDefault("0");
    r32 fruitWindInfluence MetaDefault("0");
    r32 dissolveDuration MetaDefault("0");
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
    r32 shadowHeight MetaDefault("0.5f");
    Vec2 shadowScale MetaDefault("V2(1, 1)");
    Vec4 shadowColor MetaDefault("V4(0, 0, 0, 0.5f)");
    
    r32 lootingZoomCoeff MetaDefault("3.0f");
    r32 lootingZoomSpeed MetaDefault("3.0f");
    b32 displayInStandardMode;
    Vec2 desiredOpenedDim MetaDefault("V2(400, 400)");
    Vec2 desiredUsingDim MetaDefault("V2(200, 200)");
    
    ArrayCounter animationEffectsCount MetaCounter(animationEffects);
    AnimationEffectDefinition* animationEffects;
    
    r32 frameByFrameSpeed MetaDefault("1.0f");
    b32 frameByFrameEmittors;
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
    r32 cameraZOffsetWhenOnFocus MetaDefault("-0.05f");
    r32 scaleCoeffWhenOnFocus MetaDefault("1.0f");
    r32 outlineWidth MetaDefault("1.0f");
    
    ArrayCounter colorationCount MetaCounter(colorations);
    Coloration* colorations;
    
    r32 speedOnFocus;
    r32 speedOnNoFocus;
    Vec3 offsetMaxOnFocus;
    r32 scaleMaxOnFocus MetaDefault("1.0f");
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
