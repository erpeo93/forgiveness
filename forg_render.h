#pragma once

// NOTE(leonardo): 
/*
      -coordinates are assumed to be Y up and X to the right
      -all bitmaps are assumed to be bottom up
      -all the coordinates passed to the render are in "world units"
*/
global_variable r32 cameraPitch = 0.32f * PI32;
global_variable r32 cameraDolly = 0.0f;
global_variable r32 cameraOrbit = 0.0f;

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
    
    u32 quadCount;
    u32 quadStartingIndex;
    u32 vertexArrayOffset;
    u32 indexArrayOffset;
    
    b32 disableSorting;
    b32 disableDepthTesting;
    
    RenderSetup* next;
};

struct ObjectTransform
{
    Vec3 cameraOffset;
    Vec2 scale;
    
    r32 modulationPercentage;
    Vec4 dissolvePercentages;
    Vec4 tint;
    
    r32 lightInfluence;
    r32 lightYInfluence;
    Vec4 windInfluences;
    u8 windFrequency;
    
    r32 angle;
    b32 flipOnYAxis;
    b32 dontRender;
    b32 upright;
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

global_variable Vec3 magicLateralVector;
global_variable Vec3 magicUpVector;

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


inline ObjectTransform RenderTransform()
{
    ObjectTransform result = {};
    result.scale = V2(1, 1);
    result.tint = V4(1, 1, 1, 1);
    
    return result;
}

inline ObjectTransform FlatTransform(Vec4 color = V4(1, 1, 1, 1))
{
    ObjectTransform result = RenderTransform();
    result.tint = color;
    return result;
}

inline ObjectTransform BillboardTransform()
{
    ObjectTransform result = RenderTransform();
    result.upright = true;
    return result;
}

inline void PushSetup_(RenderGroup* group, RenderSetup* setup);
struct TransientClipRect
{
    TransientClipRect( RenderGroup* group, Rect2i newClipRect )
    {
        renderGroup = group;
        
        RenderSetup setup = group->lastSetup;
        oldClipRect = setup.rect;
        setup.rect = newClipRect;
        PushSetup_(group, &setup);
    }
    
    ~TransientClipRect()
    {
        RenderSetup setup = renderGroup->lastSetup;
        setup.rect = oldClipRect;
        PushSetup_(renderGroup, &setup);
    }
    
    RenderGroup* renderGroup;
    Rect2i oldClipRect;
};