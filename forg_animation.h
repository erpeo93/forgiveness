#pragma once

#define ALPHA_CAME_IN_SECONDS 1.0f
#define ALPHA_GO_OUT_SECONDS 1.0f
#define MIN_STATUS_ALPHA -20.0f
#define MAX_STATUS_ALPHA 20.0f

enum SpriteInfoFlags
{
    Sprite_Composed = (1 << 1),
    Sprite_Entity = (1 << 2),
};

enum AnimationLoopingType
{
    Loop_none,
    Loop_normal,
    Loop_pingPong,
};


#ifndef FORG_SERVER
struct BoneAlteration
{
    b32 valid;
    Vec2 scale;
};

struct AssAlteration
{
    b32 valid;
    Vec2 scale;
    Vec2 boneOffset;
    
    b32 specialColoration;
    Vec4 color;
};


struct EquipmentAnimationPiece
{
    PieceAss ass;
    SpriteInfo sprite;
    
    b32 drawModulated;
    u32 slot;
    
    i16 status;
    struct ComponentsProperties* properties;
};

struct BlendedBone
{
    Bone bone;
    BoneAlteration alterations;
};

struct BlendedAss
{
    PieceAss ass;
    AssAlteration alterations;
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
    u32 assetIndex;
};

struct VisualComponent
{
    u64 stringHashID;
    u8 index;
    
    u32 labelCount;
};

struct ComponentsProperties
{
    u32 componentCount;
    VisualComponent components[8];
};

struct EquipmentAnimationSlot
{
    u64 ID;
    struct ObjectLayout* layout;
    u32 taxonomy;
    i16 status;
    ComponentsProperties properties;
    u32 slot;
    b32 drawModulated;
    b32 isOpen;
};

struct AnimationOutput
{
    u64 playedAnimationNameHash;
    
    i16 nearestAss;
    u32 focusSlots;
    
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
    u32 nearestCompatibleSlotForDragging;
    
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
    struct Object* objects;
    
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
    
    EquipmentAnimationSlot equipment[64];
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

struct AnimationSoundEvent
{
    r32 threesold;
    u64 animationNameHash;
    u64 eventNameHash;
};


struct ClientAnimationEffect
{
    u32 referenceSlot;
    AnimationEffect effect;
    
    union
    {
        struct ParticleEffect* particleRef;
        r32 boltTimer;
    };
    
    union
    {
        ClientAnimationEffect* next;
        ClientAnimationEffect* nextFree;
    };
};

struct AnimationEffects
{
    AnimationEffect* firstAnimationEffect;
};

struct AnimationGeneralParams
{
    u64 skeletonSkinHashID;
    u64 skeletonHashID;
    u64 skinHashID;
    Vec4 defaultColoration;
    Vec2 originOffset;
    b32 flippedOnYAxis;
    
    b32 animationIn3d;
    b32 animationFollowsVelocity;
    Vec3 modelOffset;
    Vec4 modelColoration;
    Vec3 modelScale;
    u64 modelTypeID;
    u64 modelNameID;
};

struct EditorBoneAlteration
{
    u32 boneIndex;
    BoneAlteration alt;
    union
    {
        EditorBoneAlteration* next;
        EditorBoneAlteration* nextFree;
    };
};

struct EditorAssAlteration
{
    u32 assIndex;
    AssAlteration alt;
    union
    {
        EditorAssAlteration* next;
        EditorAssAlteration* nextFree;
    };
};

struct AssAlterations
{
    EditorAssAlteration* firstAssAlteration;
};

struct BoneAlterations
{
    EditorBoneAlteration* firstBoneAlteration;
};
#endif
