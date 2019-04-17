#pragma once

printFlags(noPrefix) enum CanDoActionFlags
{
    CanDoAction_Own = (1 << 1),
    CanDoAction_EquipmentSlot = (1 << 2),
    CanDoAction_Empty = (1 << 3),
};

union TaxonomyNodeData
{
    b32 possible;
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

struct PlayerPossibleAction
{
    EntityAction action;
    u32 flags;
    TaxonomyTree tree;
    
    union
    {
        PlayerPossibleAction* next;
        PlayerPossibleAction* nextFree;
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
    EffectIdentifier ID;
    
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
    
    
    
    TaxonomyTree equipmentMappings;
    ConsumeMapping* firstConsumeMapping;
    PlayerPossibleAction* firstPossibleAction;
    
    u32 layoutCount;
    ObjectLayout* firstLayout;
    
    
    TaxonomyEssence* essences;
    CraftingEffectLink* links;
    TaxonomyEffect* firstEffect;
    
    
    u32 neededToolTaxonomies[4];
    
    
    ForgBoundType boundType;
    Rect3 physicalBounds;
    
    PlantPhysicalParams plantBaseParams;
    
    
    
#ifndef FORG_SERVER
    PlantParams* plantParams;
    VisualLabel* firstVisualLabel;
    AnimationEffect* firstAnimationEffect;
    TaxonomySound* firstSound;
    
    TaxonomyBoneAlterations* firstBoneAlteration;
    TaxonomyAssAlterations* firstAssAlteration;
    
    r32 lightIntensity;
    Vec3 lightColor;
    u64 skeletonHashID;
    Vec4 defaultColoration;
    Vec2 originOffset;
#endif
    
    
    
    
    b32 isPassiveSkill;
    EffectIdentifier effectID;
    
    u8 gridDimX;
    u8 gridDimY;
    
    
    r32 plantStatusDuration[PlantLife_Count];
    PlantLifeStatus nextStatus[PlantLife_Count];
    
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
    r32 groundPointMaxOffset;
    u32 groundPointPerTile;
    r32 chunkyness;
    u32 tilePointsLayout;
    
    
    
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
    u32 rootBits;
    
    TaxonomySlot root;
    TaxonomySlot* slots[4096];	
    ShortcutSlot* shortcutSlots[4096];
    
    struct EditorElement* firstFreeElement;
    
    
    EquipmentMapping* firstFreeEquipmentMapping;
    EquipmentLayout* firstFreeEquipmentLayout;
    EquipmentAss* firstFreeEquipmentAss;
    ConsumeMapping* firstFreeConsumeMapping;
    TaxonomyNode* firstFreeTaxonomyNode;
    MemSynthOption* firstFreeMemSynthOption;
    PlayerPossibleAction* firstFreePlayerPossibleAction;
    LayoutPiece* firstFreeLayoutPiece;
    ObjectLayout* firstFreeObjectLayout;
    TaxonomyEssence* firstFreeTaxonomyEssence;
    CraftingEffectLink* firstFreeCraftingEffectLink;
    TaxonomyEffect* firstFreeTaxonomyEffect;
    
#ifndef FORG_SERVER
    PlantParams* firstFreePlantParams;
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