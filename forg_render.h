#pragma once

// NOTE(leonardo): 
/*
      -coordinates are assumed to be Y up and X to the right
      -all bitmaps are assumed to be bottom up
      -all the coordinates passed to the render are in meters
*/

struct Lights
{
    u16 startingIndex;
    u16 endingIndex;
};

enum CameraTransformFlag
{
    Camera_Orthographic = ( 1 << 1 ),
    Camera_Debug = ( 1 << 2 ),
};

struct RenderSetup
{
    Rect2i rect;
    m4x4 proj;
    u32 renderTargetIndex;
    Vec3 ambientLightColor;
    r32 totalTimeElapsed;
    Vec3 windDirection;
    r32 windStrength;
};

struct TexturedQuadsCommand
{
    RenderSetup setup;
    u32 quadCount;
    u32 vertexArrayOffset;
    u32 indexArrayOffset;
};

struct ObjectTransform
{
    Vec3 cameraOffset;
    Vec2 scale;
    
    r32 additionalZBias;
    r32 modulationPercentage;
    r32 lightInfluence;
    r32 lightYInfluence;
    Vec4 windInfluences;
    
    r32 angle;
    b32 upright;
    b32 flipOnYAxis;
    
    b32 dontRender;
};

struct CameraTransform
{
    Vec3 X;
    Vec3 Y;
    Vec3 Z;
    
    Vec3 P;
    Vec2 screenCameraOffset;
    
    m4x4_inv proj;
};

global_variable Vec4 magicLateralVector;
global_variable Vec4 magicUpVector;

struct RenderGroup
{
    struct Assets* assets;
    
    RenderSetup lastSetup;
    
    CameraTransform gameCamera;
    CameraTransform debugCamera;
    
    GameRenderCommands* commands;
    Vec2 screenDim;
    
    RenderTexture whiteTexture;
};


inline ObjectTransform UprightTransform()
{
    ObjectTransform result = {};
    result.scale = V2(1, 1);
    result.upright = true;
    return result;
}

inline ObjectTransform FlatTransform(r32 additionalZBias = 0)
{
    ObjectTransform result = {};
    result.scale = V2(1, 1);
    result.additionalZBias = additionalZBias;
    return result;
}

inline void PushSetup(RenderGroup* group, RenderSetup* setup);
struct TransientClipRect
{
    TransientClipRect( RenderGroup* group, Rect2i newClipRect )
    {
        renderGroup = group;
        
        RenderSetup setup = group->lastSetup;
        oldClipRect = setup.rect;
        setup.rect = newClipRect;
        PushSetup(group, &setup);
    }
    
    ~TransientClipRect()
    {
        RenderSetup setup = renderGroup->lastSetup;
        setup.rect = oldClipRect;
        PushSetup(renderGroup, &setup);
    }
    
    RenderGroup* renderGroup;
    Rect2i oldClipRect;
};