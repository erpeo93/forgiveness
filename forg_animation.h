#pragma once

#define ALPHA_CAME_IN_SECONDS 1.0f
#define ALPHA_GO_OUT_SECONDS 1.0f
#define MIN_STATUS_ALPHA -20.0f
#define MAX_STATUS_ALPHA 20.0f

#define bodyDead V4(0.02f, 0.02f, 0.02f, 1)
#define bodyAlive V4(1, 1, 1, 1)

#define ashDead V4(0.0f, 0.0f, 0.0f, 1.0f)
#define ashAlive V4(1, 0, 0, 1)

struct BoneAlterations
{
    b32 valid;
    Vec2 scale;
};

struct AssAlterations
{
    b32 valid;
    Vec2 scale;
    Vec2 boneOffset;
    
    b32 specialColoration;
    Vec4 color;
};


struct VisualComponent
{
    u64 stringHashID;
    u8 index;
    
    u32 labelCount;
    VisualLabel labels[8];
};

struct ComponentsProperties
{
    u32 componentCount;
    VisualComponent components[8];
};

struct EquipmentAnimationPiece
{
    PieceAss ass;
    SpriteInfo sprite;
    
    b32 drawModulated;
    EquipInfo slot;
    
    i16 status;
    struct ComponentsProperties* properties;
};

struct BlendedBone
{
    Bone bone;
    BoneAlterations alterations;
};

struct BlendedAss
{
    PieceAss ass;
    AssAlterations alterations;
    SpriteInfo sprite;
    
    u32 equipmentAssCount;
    EquipmentAnimationPiece associatedEquipment[8];
};

struct BlendResult
{
    u32 boneCount;
    BlendedBone* bones;
    
    u32 assCount;
    BlendedAss* ass;
};

struct PieceResult
{
    Vec2 defaultPivot;
    u32 defaultWidth;
    u32 defaultHeight;
    
    Vec3 originOffset;
    r32 angle;
    Vec2 scale;
    r32 alpha;
    AssetTypeId assetType;
    r32 tags[Tag_count];
};

struct EquipmentAnimationSlot
{
    u64 ID;
    struct ObjectLayout* layout;
    u32 taxonomy;
    i16 status;
    ComponentsProperties properties;
    EquipInfo slot;
    b32 drawModulated;
    b32 isOpen;
};

struct AnimationOutput
{
    u64 playedAnimationNameHash;
    
    i16 nearestAss;
    EquipInfo focusSlots;
    
    i32 focusObjectIndex;
    r32 additionalZoomCoeff;
    
    b32 entityPresent;
    Vec3 entityOffset;
    r32 entityAngle;
    
    
    i16 hotBoneIndex;
    i16 hotAssIndex;
};

enum AnimationSyncState
{
    AnimationSync_None,
    AnimationSync_Preparing,
    AnimationSync_WaitingForCompletion,
};

struct AnimationState
{
    AnimationOutput output;
    EquipInfo nearestCompatibleSlotForDragging;
    
    AnimationSyncState syncState;
    r32 waitingForSyncTimer;
    u32 lastSyncronizedAction;
    
    u32 action;
    u32 nextAction;
    
    r32 totalTime;
    r32 normalizedTime;
    
    b32 stopAtNextBarrier;
    b32 flipOnYAxis;
    
    Rect2 bounds;
    Vec2 cameraEntityOffset;
    
    r32 cameInTime;
    r32 goOutTime;
};

printTable(noPrefix) enum AnimationEffectType
{
    AnimationEffect_None,
    AnimationEffect_ChangeColor,
    AnimationEffect_SpawnParticles,
    AnimationEffect_Light,
    AnimationEffect_Bolt,
};

printFlags(noPrefix) enum AnimationEffectFlags
{
    AnimationEffect_ActionStart = (1 << 1),
    AnimationEffect_ActionCompleted = (1 << 2),
    AnimationEffect_DeleteWhenActionChanges = (1 << 3),
};


struct AnimationEffect
{
    AnimationEffectType type;
    
    u32 flags;
    u32 triggerAction;
    u64 stringHashID;
    u32 triggerEffectTaxonomy;
    u64 targetID;
    r32 fadeTime;
    r32 inTimer;
    r32 timer;
    
    union
    {
        struct
        {
            Vec4 color;
        };
        
        struct
        {
            u32 particleEffectTaxonomy;
        };
        
        struct
        {
            Vec3 lightColor;
            r32 lightIntensity;
        };
        
        struct
        {
            u32 boltTaxonomy;
            r32 boltTargetTimer;
        };
    };
    
    
    union
    {
        AnimationEffect* next;
        AnimationEffect* nextFree;
    };
};

struct SkeletonInfo
{
    u64 skeletonSkinHashID;
    u64 skeletonHashID;
    u64 skinHashID;
    Vec4 coloration;
    Vec2 originOffset;
    b32 flippedOnYAxis;
};

struct AnimationDebugParams
{
    b32 ortho;
    b32 showBones;
    b32 hideBitmaps;
    b32 showPivots;
    r32 modTime;
    u64 forcedNameHashID;
    struct ClientEntity* fakeEquipment;
};

struct AnimationFixedParams
{
    AnimationDebugParams debug;
    
    MemoryPool* tempPool;
    
    r32 timeToAdvance;
    struct GameModeWorld* worldMode;
    struct TaxonomyTable* taxTable;
    Vec2 relativeScreenMouseP;
    Vec3 mousePOnGround;
    b32 combatAnimation;
    r32 defaultModulatonWithFocusColor;
    
    u8 objectCount;
    Object* objects;
    
    u8 objectGridDimX;
    u8 objectGridDimY;
    
    r32 minFocusSlotDistanceSq;
    r32 minHotAssDistanceSq;
    
    struct ClientAnimationEffect* firstActiveEffect;
    struct ClientAnimationEffect* firstActiveEquipmentLightEffect;
    
    r32 cameInTime;
    r32 goOutTime;
    
    struct ClientEntity* entity;
    struct ClientEntity* draggingEntity;
    
    EquipmentAnimationSlot equipment[Slot_Count];
    Vec4 defaultColoration;
    
    AnimationOutput* output;
};

struct AnimationVolatileParams
{
    b32 flipOnYAxis;
    b32 drawEmptySpaces;
    u64 skeletonSkinHashID;
    u64 skeletonHashID;
    u64 skinHashID;
    u64 recipeIndex;
    r32 additionalZbias;
    r32 modulationWithFocusColor;
    Vec3 cameraOffset;
    Lights lights;
    Vec4 color;
    r32 angle;
    Vec2 scale;
    r32 zOffset;
    ComponentsProperties* properties;
};

struct AnimationEntityParams
{
    r32 transparent;
    b32 drawOpened;
    b32 onTop;
    r32 scale; 
    Vec3 offset;
    r32 angle;
    Rect2 bounds;
    r32 additionalZbias;
};

inline AnimationEntityParams StandardEntityParams()
{
    AnimationEntityParams result = {};
    result.scale = 1.0f;
    result.bounds = InvertedInfinityRect2();
    return result;
}

inline AnimationEntityParams ContainerEntityParams(b32 onTop = false)
{
    AnimationEntityParams result = StandardEntityParams();
    result.drawOpened = true;
    result.onTop = onTop;
    return result;
}

struct TileAnimationData
{
    Lights lights;
};