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

// NOTE(Leonardo): this is used only for "multipart" things that need to "follow" the animations like pants, body armours, ecc
struct TaxonomyPart
{
    SlotName slot;
    u64 stringHashID;
    
    union
    {
        TaxonomyPart* next;
        TaxonomyPart* nextFree;
    };
};


struct SlotPresentMap
{
    b32 multiPart;
    union
    {
        struct
        {
            SlotName left;
            SlotName right;
        };
        
        struct
        {
            u32 slotCount;
            SlotName slots[4];
        };
    };
};

struct EquipmentMapping
{
    u32 taxonomy;
    SlotPresentMap mapping;
    
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


#define MAX_COMPONENT_PER_MODULE 8
struct LayoutPiece
{
    Vec3 offset;
    r32 angle;
    Vec2 scale;
    r32 alpha;
    Vec2 pivot;
    u64 componentHashID;
    u32 flags;
    
    u32 ingredientCount;
    LayoutPiece* parent;
    u32 ingredientTaxonomies[MAX_COMPONENT_PER_MODULE];
    u8 ingredientQuantities[MAX_COMPONENT_PER_MODULE];
    
    union
    {
        LayoutPiece* next;
        LayoutPiece* nextFree;
    };
};

struct ObjectLayout
{
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
    
    
    
    
    EquipmentMapping* firstEquipmentMapping;
    ConsumeMapping* firstConsumeMapping;
    PlayerPossibleAction* firstPossibleAction;
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
    TaxonomyPart* firstPart;  
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
    u8 spaceSlotCount;
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
    TaxonomyPart* firstFreePart;
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