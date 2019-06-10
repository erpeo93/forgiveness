#pragma once

printFlags(noPrefix) enum CanDoActionFlags
{
    CanDoAction_Own = (1 << 1),
    CanDoAction_EquipmentSlot = (1 << 2),
    CanDoAction_Empty = (1 << 3),
};


struct PossibleActionData
{
    r32 requiredTime;
};

union TaxonomyNodeData
{
    PossibleActionData action;
    struct MemSynthOption* firstOption;
    struct EquipmentMapping* equipmentMapping;
};

struct TaxonomyNode
{
    u32 key;
    
    TaxonomyNode* firstChild;
    
    union
    {
        TaxonomyNode* next;
        TaxonomyNode* nextFree;
    };
    
    TaxonomyNodeData data;
};

struct TaxonomyTree
{
    TaxonomyNode* root;
};

struct PossibleAction
{
    EntityAction action;
    u32 flags;
    TaxonomyTree tree;
    r32 distance;
    
    union
    {
        PossibleAction* next;
        PossibleAction* nextFree;
    };
};

struct ShortcutSlot
{
    u32 subTaxonomiesCount;
    u32 invalidTaxonomiesCount;
    char subTaxonomies[64][32];
    
    u32 taxonomy;
    u32 parentTaxonomy;
    
    u64 hashIndex;
    char name[32];
    ShortcutSlot* nextInHash;
};

struct AttributeSlot
{
    u32 offsetFromBase;
    r32 valueR32;
};

struct EquipmentAss
{
    u64 stringHashID;
    u8 index;
    
    u32 assIndex;
    
    Vec2 assOffset;
    r32 zOffset;
    r32 angle;
    Vec2 scale;
    
    union
    {
        EquipmentAss* next;
        EquipmentAss* nextFree;
    };
};

struct EquipmentLayout
{
    u64 layoutHashID;
    EquipInfo slot;
    
    EquipmentAss* firstEquipmentAss;
    
    union
    {
        EquipmentLayout* next;
        EquipmentLayout* nextFree;
    };
};

struct EquipmentMapping
{
    EquipmentLayout* firstEquipmentLayout;
    
    union
    {
        EquipmentMapping* next;
        EquipmentMapping* nextFree;
    };
};

struct ConsumeMapping
{
    EntityAction action;
    u32 taxonomy;
    
    union
    {
        ConsumeMapping* next;
        ConsumeMapping* nextFree;
    };
};

printTable(noPrefix) enum ObjectState
{
    ObjectState_Default,
    ObjectState_Ground,
    ObjectState_Open,
    ObjectState_GroundOpen,
    
    ObjectState_Count,
};

struct LayoutPieceParams
{
    b32 valid;
    Vec3 parentOffset;
    r32 parentAngle;
    Vec2 scale;
    r32 alpha;
    Vec2 pivot;
};

#define MAX_COMPONENT_PER_MODULE 8
struct LayoutPiece
{
    u64 componentHashID;
    u8 index;
    
    LayoutPieceParams params[ObjectState_Count];
    
    char name[32];
	LayoutPiece* parent;
    u32 ingredientCount;
    u32 ingredientTaxonomies[MAX_COMPONENT_PER_MODULE];
    u32 ingredientQuantities[MAX_COMPONENT_PER_MODULE];
    
    union
    {
        LayoutPiece* next;
        LayoutPiece* nextFree;
    };
};

inline LayoutPieceParams* GetParams(LayoutPiece* piece, ObjectState state)
{
    LayoutPieceParams* params = piece->params + state;
    if(!params->valid)
    {
        params = piece->params + ObjectState_Default;
    }
    
    return params;
}

struct ObjectLayout
{
    char name[32];
    u64 nameHashID;
    
    u32 pieceCount;
    LayoutPiece* firstPiece;
    
    union
    {
        ObjectLayout* next;
        ObjectLayout* nextFree;
    };
};


struct CraftingEffectLink
{
    EssenceSlot essences[MAX_ESSENCES_PER_EFFECT];
    EntityAction triggerAction;
    b32 target;
    EffectIdentifier effectID;
    
    union
    {
        CraftingEffectLink* next;
        CraftingEffectLink* nextFree;
    };
};

struct TaxonomyEssence
{
    EssenceSlot essence;
    union
    {
        TaxonomyEssence* next;
        TaxonomyEssence* nextFree;
    };
};

struct TaxonomyEffect
{
    Effect effect;
    union
    {
        TaxonomyEffect* next;
        TaxonomyEffect* nextFree;
    };
};

struct TaxonomyBehavior
{
    u32 referenceTaxonomy;
    u32 specificTaxonomy;
    
    union
    {
        TaxonomyBehavior* next;
        TaxonomyBehavior* nextFree;
    };
};

struct TaxonomyMemBehavior
{
    u32 taxonomy;
    
    union
    {
        TaxonomyMemBehavior* next;
        TaxonomyMemBehavior* nextFree;
    };
};

enum ForgBoundType
{
    ForgBound_None,
    ForgBound_NonPhysical,
    ForgBound_Standard,
};

struct TaxonomyConsideration
{
    char expression[1024];
    
    union
    {
        TaxonomyConsideration* next;
        TaxonomyConsideration* nextFree;
    };
};

struct TaxonomySound
{
    r32 threesold;
    u64 animationNameHash;
    
    u64 eventNameHash;
    
    union
    {
        TaxonomySound* next;
        TaxonomySound* nextFree;
    };
};

struct EditorTab
{
    b32 editable;
    struct EditorElement* root;
};

#ifndef FORG_SERVER
struct TaxonomyBoneAlterations
{
    u32 boneIndex;
    BoneAlterations alt;
    union
    {
        TaxonomyBoneAlterations* next;
        TaxonomyBoneAlterations* nextFree;
    };
};

struct TaxonomyAssAlterations
{
    u32 assIndex;
    AssAlterations alt;
    union
    {
        TaxonomyAssAlterations* next;
        TaxonomyAssAlterations* nextFree;
    };
};
#endif

struct RockMineral
{
    r32 lerp;
    r32 lerpDelta;
    
    Vec4 color;
    
    union
    {
        RockMineral* next;
        RockMineral* nextFree;
    };
};

struct RockDefinition
{
    b32 collides;
    
    u64 modelTypeHash;
    u64 modelNameHash;
    
    Vec4 color;
    Vec4 startingColorDelta;
    
    Vec4 perVertexColorDelta;
    
    Vec3 scale;
    Vec3 scaleDelta;
    
    r32 smoothness;
    r32 smoothnessDelta;
    u32 iterationCount;
    
    
    r32 minDisplacementY;
    r32 maxDisplacementY;
    
    r32 minDisplacementZ;
    r32 maxDisplacementZ;
    
    
    r32 percentageOfMineralVertexes;
    u32 mineralCount;
    RockMineral* firstPossibleMineral;
    
    u32 renderingRocksCount;
    u32 renderingRocksDelta;
    Vec3 renderingRocksRandomOffset;
    r32 scaleRandomness;
    
    union
    {
        RockDefinition* nextFree;
        RockDefinition* next;
    };
};

struct NoiseParams
{
    r32 frequency;
    u32 octaves;
    r32 persistance;
    r32 offset;
    r32 amplitude;
};

struct TaxonomyContainerInteraction
{
    r32 targetTime;
    
    u32 requiredCount;
    u32 requiredTaxonomies[MAX_CONTAINER_INTERACTION_REQUIRED];
    u32 effectCount;
    Effect effects[MAX_CONTAINER_INTERACTION_EFFECTS];
    
    union
    {
        TaxonomyContainerInteraction* next;
        TaxonomyContainerInteraction* nextFree;
    };
};

struct TaxonomySlot
{
    u32 taxonomy;
    u64 stringHashID;
    
    u32 subTaxonomiesCount;
    u32 invalidTaxonomiesCount;
    
    u32 usedBitsTotal;
    u32 parentNecessaryBits;
    u32 necessaryBits;
    char name[32];
    TaxonomySlot* nextInHash;
    
    
    Vec3 spawnOffsetAttack;
    Vec3 spawnOffsetCast;
    
    TaxonomyTree equipmentMappings;
    ConsumeMapping* firstConsumeMapping;
    PossibleAction* firstPossibleAction;
    
    u32 layoutCount;
    ObjectLayout* firstLayout;
    
    
    TaxonomyEssence* essences;
    CraftingEffectLink* links;
    TaxonomyEffect* firstEffect;
    
    
    u32 neededToolTaxonomies[4];
    
    
    b32 scaleDimBasedOnIntensity;
    r32 scaleDimGenCoeffV;
    ForgBoundType boundType;
    Rect3 physicalBounds;
    
    RockDefinition* rock;
    PlantDefinition* plant;
    struct ParticleEffectDefinition* particleEffect;
    
    
#ifndef FORG_SERVER
    VisualLabel* firstVisualLabel;
    AnimationEffect* firstAnimationEffect;
    TaxonomySound* firstSound;
    
    TaxonomyBoneAlterations* firstBoneAlteration;
    TaxonomyAssAlterations* firstAssAlteration;
    
    
    b32 animationIn3d;
    b32 animationFollowsVelocity;
    Vec3 modelOffset;
    Vec4 modelColoration;
    Vec3 modelScale;
    u64 modelTypeID;
    u64 modelNameID;
    
    
    
    r32 lightIntensity;
    Vec3 lightColor;
    u64 skeletonHashID;
    u64 skinHashID;
    Vec4 defaultColoration;
    Vec2 originOffset;
    
    
    Vec4 iconColor;
    Vec4 iconActiveColor;
    Vec4 iconHoverColor;
    u64 iconModelTypeID;
    u64 iconModelNameID;
    Vec3 iconScale;
#endif
    
    
    
    r32 skillDistanceAllowed;
    r32 cooldown;
    b32 isPassiveSkill;
    u32 turningPointLevel;
    u32 maxLevel;
    r32 radixExponent;
    r32 exponentiationExponent;
    r32 radixLerping;
    r32 exponentiationLerping;
    
    
    u8 gridDimX;
    u8 gridDimY;
    
    
    AttributeSlot attributeHashmap[16];
    
    
    struct AIBehavior* behaviorContent;
    struct MemCriteria* criteria;
    struct MemSynthesisRule* synthRules;
    TaxonomyBehavior* firstPossibleBehavior;
    TaxonomyMemBehavior* firstMemBehavior;
    NakedHandReq* nakedHandReq;
    
    
    
    TaxonomyBehavior* startingBehavior;
    TaxonomyConsideration* consideration;
    
    
    Vec4 tileBorderColor;
    Vec4 tileColor;
    Vec4 colorDelta;
    r32 groundPointMaxOffset;
    r32 groundPointPerTile;
    r32 groundPointPerTileV;
    r32 chunkynessWithSame;
    r32 chunkynessWithOther;
    u32 tilePointsLayout;
    r32 colorRandomness;
    NoiseParams tileNoise;
    
    struct WorldGenerator* generator;
    
    TaxonomyContainerInteraction* firstInsideInteraction;
    
    
    b32 editorCollapsed;
    i32 editorChangeCount;
    u32 tabCount;
    EditorTab tabs[16];
};

struct SoundLabel
{
    u64 hashID;
    r32 value;
};

struct LabeledSound
{
    u64 typeHash;
    u64 nameHash;
    
    r32 delay;
    r32 decibelOffset;
    r32 pitch;
    r32 toleranceDistance;
    r32 distanceFalloffCoeff;
    union
    {
        LabeledSound* next;
        LabeledSound* nextFree;
    };
    
    u32 labelCount;
    SoundLabel labels[8];
};

printTable(noPrefix) enum SoundContainerType
{
    SoundContainer_Random,
    SoundContainer_Labeled,
    SoundContainer_Sequence,
};

struct SoundContainer
{
    SoundContainerType type;
    
    u32 soundCount;
    LabeledSound* firstSound;
    
    u32 containerCount;
    SoundContainer* firstChildContainer;
    
    union
    {
        SoundContainer* next;
        SoundContainer* nextFree;
    };
    
    u32 labelCount;
    SoundLabel labels[8];
};

struct SoundEvent
{
    u64 eventNameHash;
    
    SoundContainer rootContainer;
};

struct TaxonomyTable
{
    MemoryPool pool;
    
    u32 recipeTaxonomy;
    u32 objectTaxonomy;
    u32 equipmentTaxonomy;
    u32 plantTaxonomy;
    u32 creatureTaxonomy;
    u32 fluidTaxonomy;
    u32 rockTaxonomy;
    u32 effectTaxonomy;
    u32 essenceTaxonomy;
    u32 behaviorTaxonomy;
    u32 tileTaxonomy;
    u32 generatorTaxonomy;
    u32 rootBits;
    
    TaxonomySlot root;
    TaxonomySlot* slots[4096];	
    ShortcutSlot* shortcutSlots[4096];
    
    struct EditorElement* firstFreeElement;
    struct EditorTextBlock* firstFreeEditorText;
    
    
    EquipmentMapping* firstFreeEquipmentMapping;
    EquipmentLayout* firstFreeEquipmentLayout;
    EquipmentAss* firstFreeEquipmentAss;
    ConsumeMapping* firstFreeConsumeMapping;
    TaxonomyNode* firstFreeTaxonomyNode;
    MemSynthOption* firstFreeMemSynthOption;
    PossibleAction* firstFreePossibleAction;
    LayoutPiece* firstFreeLayoutPiece;
    ObjectLayout* firstFreeObjectLayout;
    TaxonomyEssence* firstFreeTaxonomyEssence;
    CraftingEffectLink* firstFreeCraftingEffectLink;
    TaxonomyEffect* firstFreeTaxonomyEffect;
    struct WorldGenerator* firstFreeWorldGenerator;
    RockDefinition* firstFreeRockDefinition;
    PlantDefinition* firstFreePlantDefinition;
    ParticleEffectDefinition* firstFreeParticleEffectDefinition;
    RockMineral* firstFreeRockMineral;
    
#ifndef FORG_SERVER
    VisualLabel* firstFreeVisualLabel;
    AnimationEffect* firstFreeAnimationEffect;
    TaxonomySound* firstFreeTaxonomySound;
    LabeledSound* firstFreeLabeledSound;
    SoundContainer* firstFreeSoundContainer;
    TaxonomyBoneAlterations* firstFreeTaxonomyBoneAlterations;
    TaxonomyAssAlterations* firstFreeTaxonomyAssAlterations;
#endif
    
    
    struct MemSynthesisRule* firstFreeMemSynthesisRule;
    struct TaxonomyConsideration* firstFreeTaxonomyConsideration;
    struct AIBehavior* firstFreeAIBehavior;
    struct MemCriteria* firstFreeMemCriteria;
    TaxonomyBehavior* firstFreeTaxonomyBehavior;
    TaxonomyMemBehavior* firstFreeTaxonomyMemBehavior;
    NakedHandReq* firstFreeNakedHandReq;
    
    struct TaxonomyAssociation* firstFreeTaxonomyAssociation;
    struct TaxonomyTileAssociations* firstFreeTaxonomyTileAssociations;
    TaxonomyContainerInteraction* firstFreeTaxonomyContainerInteraction;
    
    RandomSequence eventSequence;
    RandomSequence translateSequence;
    u32 eventCount;
    SoundEvent events[64];
    
    EditorElement* soundNamesRoot;
    EditorElement* soundEventsRoot;
    EditorElement* componentsRoot;
    EditorElement* oldComponentsRoot;
    
    u32 errorCount;
    char errors[512][128];
};

inline u32 GetObjectTaxonomy(TaxonomyTable* table, Object* object)
{
    u32 taxonomy = object->taxonomy;
    if(object->quantity == 0xffff)
    {
        taxonomy = table->recipeTaxonomy;
    }
    
    return taxonomy;
}