#pragma once

#define bodyDead V4(0.02f, 0.02f, 0.02f, 1)
#define bodyAlive V4(1, 1, 1, 1)

#define ashDead V4(0.0f, 0.0f, 0.0f, 1.0f)
#define ashAlive V4(1, 0, 0, 1)


struct BlendResult
{
    u32 boneCount;
    Bone bones[32];
    
    u32 assCount;
    PieceAss ass[32];
    
    SpriteInfo* sprites[32];
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
    b32 ghost;
    b32 dontRender;
    b32 drawOpened;
    b32 container;
    
    u32 equipmentSlotIndex;
    u32 taxonomy;
    u64 recipeIndex;
    r32 status;
    u64 parentStringHashID;
    u64 stringHashID;
    SlotPlacement placement;
};

struct AnimationOutput
{
    u64 playedAnimationNameHash;
    
    EquipInfo nearestCompatibleSlot;
    EquipInfo focusSlots;
    i32 focusObjectIndex;
    r32 additionalZoomCoeff;
    
    b32 entityPresent;
    Vec3 entityOffset;
    r32 entityAngle;
};

struct AnimationState
{
    AnimationOutput output;
    u32 action;
    u32 nextAction;
    r32 totalTime;
    r32 normalizedTime;
    
    b32 preparing;
    b32 stopAtNextBarrier;
    b32 flipOnYAxis;
    
    Rect2 bounds;
    
    u32 lifePointsSeedResetCounter;
    u32 spawnAshParticlesCount;
    r32 ashIdleTimer;
    Vec4 ashColor;
    r32 ashParticleViewPercentage;
    r32 ashDim;
    
    r32 cameInTime;
};

enum AnimationEffectType
{
    AnimationEffect_None,
    AnimationEffect_ChangeColor,
    AnimationEffect_SpawnParticles,
    AnimationEffect_SpawnAshesTowardEntity,
};

enum AnimationEffectFlags
{
    AnimationEffect_AllActions = (1 << 1),
};


struct AnimationEffect
{
    u32 flags;
    AnimationEffectType type;
    
    
    u32 triggerAction;
    u32 triggerEffectTaxonomy;
    
    
    u64 stringHashID;
    r32 timer;
    
    union
    {
        struct
        {
            Vec4 color;
            r32 fadeTime;
        };
        
        FluidSpawnType particleType;
        
        struct
        {
            Vec4 color;
            r32 timeToArriveAtDest;
            r32 dim;
            u64 targetID;
            r32 targetTimer;
        };
    };
    
    
    union
    {
        AnimationEffect* next;
        AnimationEffect* nextFree;
    };
};

struct AnimationFixedParams
{
    b32 ortho;
    r32 timeMod;
    
    r32 timeToAdvance;
    struct GameModeWorld* worldMode;
    struct TaxonomyTable* taxTable;
    Vec3 mousePOnGround;
    b32 combatAnimation;
    r32 defaultModulatonWithFocusColor;
    
    EquipInfo oldFocusSlots; 
    i32 currentObjectIndex;
    
    u32 equipmentCount;
    u32 maxEquipmentCount;
    EquipmentAnimationSlot* equipment;
    u8 objectCount;
    Object* objects;
    
    b32 ghostAllowed;
    
    r32 minGhostDistanceSq;
    r32 minFocusSlotDistanceSq;
    
    AnimationEffect* firstActiveEffect;
    
    r32 cameInTime;
    
    u32 lifePointsSeedResetCounter;
    r32 lifePointRatio;
    r32 lifePointThreesold;
    r32 lifePointFadeDuration;
    
    struct ClientEntity* entity;
    struct ClientEntity* draggingEntity;
    u64 draggingEntityHashIDs[4];
    
    AnimationOutput* output;
};

struct VisualComponent
{
    u64 stringHashID;
    
    u32 tagCount;
    VisualTag tags[8];
};

struct ComponentsProperties
{
    u32 componentCount;
    VisualComponent components[8];
};

struct AnimationVolatileParams
{
    b32 flipOnYAxis;
    b32 drawEmptySpaces;
    u64 entityHashID;
    u64 recipeIndex;
    r32 additionalZbias;
    r32 modulationWithFocusColor;
    Vec3 cameraOffset;
    Vec4 lightIndexes;
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
