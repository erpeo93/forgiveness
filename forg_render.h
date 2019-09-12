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

enum Render_command
{
    CommandType_TexturedQuadsCommand,
    CommandType_BeginPeels,
    CommandType_EndPeels,
};

enum CameraTransformFlag
{
    Camera_Orthographic = ( 1 << 1 ),
    Camera_Debug = ( 1 << 2 ),
};

struct CommandHeader
{
    u16 type;
};

struct RenderSetup
{
    Rect2i rect;
    m4x4 proj;
    u32 renderTargetIndex;
    Vec3 ambientLightColor;
    Vec3 directionalLightColor;
    Vec3 directionalLightDir;
    r32 directionalLightIntensity;
};

struct TexturedQuadsCommand
{
    RenderSetup setup;
    u32 triangleCount;
    u32 vertexArrayOffset; // NOTE(Leonardo): 4 vertices per quad
    u32 indexArrayOffset; // NOTE(Leonardo): 6 indeces per quad
};

struct ObjectTransform
{
    Vec3 cameraOffset;
    r32 additionalZBias;
    r32 modulationPercentage;
    
    r32 angle;
    b32 upright;
    b32 flipOnYAxis;
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

struct RenderGroup
{
    struct Assets* assets;
    
    RenderSetup lastSetup;
    
    CameraTransform gameCamera;
    CameraTransform debugCamera;
    
    TexturedQuadsCommand* currentQuads;
    
    GameRenderCommands* commands;
    Vec2 screenDim;
    
    RenderTexture whiteTexture;
};


inline ObjectTransform UprightTransform()
{
    ObjectTransform result = {};
    result.upright = true;
    return result;
}

inline ObjectTransform FlatTransform(r32 additionalZBias = 0)
{
    ObjectTransform result = {};
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