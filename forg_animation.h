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

struct AnimationComponent
{
    r32 time;
    
    u16 skin;
    GameProperties skinProperties;
    
    u16 skeleton;
    GameProperties skeletonProperties;
    
    b32 flipOnYAxis;
};

struct AnimationPiece
{
    Vec2 pivot;
    u64 nameHash;
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
    r32 scale;
    b32 flipOnYAxis;
    ObjectTransform transform;
};