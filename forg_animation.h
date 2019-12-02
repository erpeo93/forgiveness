#pragma once
struct BoneAlteration
{
    Vec2 scale;
};

struct AssAlteration
{
    Vec2 scale;
    Vec2 boneOffset;
    b32 specialColoration;
    Vec4 color;
};

struct ShadowComponent
{
    Vec3 offset;
    Vec2 scale;
    Vec4 color;
};

printTable(noPrefix) enum AnimationEffectType
{
    AnimationEffect_Tint,
    AnimationEffect_Light,
    AnimationEffect_Particles,
    AnimationEffect_SlowDown,
};

struct AnimationEffect
{
    u16 ID;
    u16 type;
    
    r32 time;
    union
    {
        Vec4 tint;
        
        struct
        {
            r32 lightIntensity;
            Vec3 lightColor;
        };
        
        struct ParticleEffectInstance* particles;
        
        r32 slowDownCoeff;
    };
    
};

introspection() struct AnimationEffectDefinition
{
    ArrayCounter propertyCount MetaCounter(properties);
    GameProperty* properties;
    
    Enumerator type MetaEnumerator("AnimationEffectType");
    r32 time;
    
    Vec4 tint MetaDefault("V4(1, 1, 1, 1)");
    
    r32 lightIntensity MetaDefault("1.0f");
    Vec3 lightColor MetaDefault("V3(1, 1, 1)");
    
    ArrayCounter particlePropertyCount MetaCounter(particleProperties);
    GameProperty* particleProperties;
    Vec3 particleEndOffset;
    
    r32 slowDownCoeff;
};

struct EntityAnimationParams
{
    r32 modulationPercentage;
    Vec4 tint;
    r32 dissolveCoeff;
    r32 speed;
};

inline EntityAnimationParams DefaultAnimationParams()
{
    EntityAnimationParams result = {};
    result.tint = V4(1, 1, 1, 1);
    result.speed = 1.0f;
    
    return result;
}

#define MAX_ACTIVE_EFFECTS 8
struct AnimationEffectComponent
{
    EntityAnimationParams params;
    
    r32 lightIntensity;
    Vec3 lightColor;
    
    u32 effectCount;
    AnimationEffect effects[MAX_ACTIVE_EFFECTS];
};

struct AnimationComponent
{
    PAKAnimation* currentAnimation;
    r32 totalTime;
    r32 time;
    r32 oldTime;
    b32 backward;
    
    u64 skinHash;
    GameProperties skinProperties;
    
    u64 skeletonHash;
    GameProperties skeletonProperties;
    
    b32 flipOnYAxis;
    
    r32 scale;
    r32 speed;
};

struct AnimationPiece
{
    Vec2 pivot;
    u64 nameHash;
    b32 placeHolder;
    Vec3 originOffset;
    r32 zBias;
    r32 angle;
    Vec2 scale;
    Vec4 color;
    Vec2 mainAxis;
};

introspection() struct PieceReplacement
{
    GameAssetType imageType MetaDefault("{AssetType_Image, 0}") MetaFixed(type);
    
    Vec3 offset;
    r32 scale MetaDefault("1.0f");
    b32 inheritPivot MetaDefault("true");
    Vec2 pivot;
};

introspection() struct AnimationReplacement
{
    AssetLabel name;
    
    ArrayCounter pieceCount MetaCounter(pieces);
    PieceReplacement* pieces;
};

#ifndef FORG_SERVER
struct AnimationParams
{
    r32 elapsedTime;
    r32 angle;
    Vec3 P;
    Vec4 tint;
    r32 dissolveCoeff;
    Lights lights;
    r32 scale;
    b32 flipOnYAxis;
    ObjectTransform transform;
    EquipmentMappingComponent* equipment;
    UsingMappingComponent* equipped;
    r32 modulationPercentage;
    b32 fakeAnimation;
    
    u32 replacementCount;
    AnimationReplacement* replacements;
};
#endif
