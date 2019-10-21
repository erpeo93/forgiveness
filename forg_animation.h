#pragma once
enum AnimationLoopingType
{
    Loop_none,
    Loop_normal,
    Loop_pingPong,
};

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

struct AnimationComponent
{
    r32 time;
    
    u64 skinHash;
    GameProperties skinProperties;
    
    u64 skeletonHash;
    GameProperties skeletonProperties;
    
    b32 flippedByDefault;
    b32 flipOnYAxis;
    
    ShadowComponent shadow;
};

struct AnimationPiece
{
    Vec2 pivot;
    u64 nameHash;
    b32 placeHolder;
    Vec3 originOffset;
    r32 angle;
    Vec2 scale;
    Vec4 color;
};

struct AnimationParams
{
    r32 elapsedTime;
    r32 angle;
    Vec3 P;
    Lights lights;
    r32 scale;
    b32 flipOnYAxis;
    ObjectTransform transform;
    EquipmentMappingComponent* equipment;
    UsingMappingComponent* equipped;
};