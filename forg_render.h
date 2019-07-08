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

struct EnvironmentMap
{
    // NOTE(leonardo): LOD[0] is the most detailed env map.
    Bitmap LOD[4];
    r32 distanceZ;
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
    
    u32 countMissing;
    RenderSetup lastSetup;
    
    CameraTransform gameCamera;
    CameraTransform debugCamera;
    
    TexturedQuadsCommand* currentQuads;
    
    GameRenderCommands* commands;
    Vec2 screenDim;
    
    RenderTexture whiteTexture;
};

inline Vec3 ProjectOnGround(Vec3 P, Vec3 cameraP)
{
    Vec3 rayOriginP = cameraP;
    Vec3 rayDirection = Normalize(P - rayOriginP);
    
    Vec3 planeOrigin = V3(0, 0, 0);
    Vec3 planeNormal = V3(0, 0, 1);
    
    Vec3 groundP = RayPlaneIntersection(rayOriginP, rayDirection, planeOrigin, planeNormal);
    
    return groundP;
}

inline Rect2 ProjectOnGround(RenderGroup* group, Vec3 groundOrigin, Vec3 offset, Vec3 dim)
{
    Vec3 minOffset = offset - 0.5f * dim;
    Vec3 maxOffset = offset + 0.5f * dim;
    
    Vec3 rectMin = groundOrigin + (minOffset.x * group->gameCamera.X + minOffset.y * group->gameCamera.Y + minOffset.z * group->gameCamera.Z);
    Vec3 rectMax = groundOrigin + (maxOffset.x * group->gameCamera.X + maxOffset.y * group->gameCamera.Y + maxOffset.z * group->gameCamera.Z);
    
    Vec3 rectMinGround = ProjectOnGround(rectMin, group->gameCamera.P);
    Vec3 rectMaxGround = ProjectOnGround(rectMax, group->gameCamera.P);
    Rect2 rect = RectMinMax(rectMinGround.xy, rectMaxGround.xy);
    
    return rect;
}

inline Vec2 ProjectOnScreen(RenderGroup* group, Vec3 worldP, r32* clipZ)
{
    Vec2 result = V2(R32_MIN, R32_MIN);
    Vec4 clip = group->gameCamera.proj.forward * V4(worldP, 1);
    if(clip.w > 0)
    {
        Vec3 clipSpace = clip.xyz * (1.0f / clip.w);
        
        *clipZ = clipSpace.z;
        
        Vec2 screenCenter = 0.5f * group->screenDim;
        result = Hadamart(clipSpace.xy, screenCenter) + screenCenter;
        
    }
    
    return result;
}

inline Rect2 ProjectOnScreenCameraAligned(RenderGroup* group, Vec3 P, Rect2 worldSpaceBounds, r32* cameraZ = 0)
{
    Vec2 center = GetCenter(worldSpaceBounds);
    Vec2 halfDim = 0.5f * GetDim(worldSpaceBounds);
    
    Vec3 X = group->gameCamera.X;
    Vec3 Y = group->gameCamera.Y;
    Vec3 Z = group->gameCamera.Z;
    
    Vec3 origin = P + center.x * X + center.y * Y;
    
    Vec3 minXminY = origin - halfDim.x * X - halfDim.y * Y;
    Vec3 maxXmaxY = origin + halfDim.x * X + halfDim.y * Y;
    
    r32 minZ, maxZ;
    
    Vec2 minScreen = ProjectOnScreen(group, minXminY, &minZ);
    Vec2 maxScreen = ProjectOnScreen(group, maxXmaxY, &maxZ);
    
    if(cameraZ)
    {
        *cameraZ = Min(minZ, maxZ);
    }
    
    Rect2 result = RectMinMax(minScreen, maxScreen);
    
    return result;
}

inline Rect2 ProjectOnScreen(RenderGroup* group, Rect3 rect, r32* cameraZ)
{
    Vec3 min = rect.min;
    Vec3 max = rect.max;
    
    Vec3 toProject[8];
    
    Rect2 result = InvertedInfinityRect2();
    r32 minZ = R32_MAX;
    
    toProject[0] = V3(min.x, min.y, min.z);
    toProject[1] = V3(min.x, min.y, max.z);
    toProject[2] = V3(min.x, max.y, min.z);
    toProject[3] = V3(min.x, max.y, max.z);
    toProject[4] = V3(max.x, min.y, min.z);
    toProject[5] = V3(max.x, min.y, max.z);
    toProject[6] = V3(max.x, max.y, min.z);
    toProject[7] = V3(max.x, max.y, max.z);
    
    
    for(u32 index = 0; index < ArrayCount(toProject); ++index)
    {
        r32 projectedZ;
        Vec2 projected = ProjectOnScreen(group, toProject[index], &projectedZ);
        minZ = Min(minZ, projectedZ);
        
        result = Union(result, projected);
    }
    
    *cameraZ = minZ;
    
    
    return result;
}

inline Vec3 UnprojectAtZ( RenderGroup* group, CameraTransform* camera, Vec2 screenP, r32 Z)
{
    Vec4 probeZ = V4(0, 0, Z, 1.0f );
    probeZ = camera->proj.forward * probeZ;
    r32 clipZ =probeZ.z;
    r32 clipW = probeZ.w;
    
    Vec2 screenCenter = 0.5f * group->screenDim;
    
    Vec2 clipSpaceXY = (screenP - screenCenter);
    clipSpaceXY.x *= (2.0f / group->screenDim.x);
    clipSpaceXY.y *= (2.0f / group->screenDim.y);
    
    Vec4 clip = V4( clipSpaceXY * clipW, clipZ, clipW);
    Vec3 world = (camera->proj.backward * clip).xyz;
    Vec3 result = world;
    
    return result;
}


inline Vec3 Unproject( RenderGroup* group, CameraTransform* camera, Vec2 screenP, r32 worldDistanceFromCameraZ)
{
    Vec4 probeZ = V4( camera->P -worldDistanceFromCameraZ * camera->Z, 1.0f );
    Vec3 result = UnprojectAtZ(group, camera, screenP, probeZ.z);
    return result;
}

inline Rect3 GetScreenBoundsAtDistance( RenderGroup* group, r32 cameraDistanceZ )
{
    Vec3 min = Unproject(group, &group->gameCamera, V2( 0, 0 ), cameraDistanceZ);
    Vec3 max = Unproject(group, &group->gameCamera, group->screenDim, cameraDistanceZ);
    Rect3 result = RectMinMax( min, max );
    return result;
}

inline Rect3 GetScreenBoundsAtTargetDistance(RenderGroup* group)
{
    r32 z = 0.0f;
    Rect3 result = GetScreenBoundsAtDistance( group, z );
    return result;
}

inline Vec3 GetWorldP(RenderGroup* group, Vec2 screenOffset)
{
    Vec2 realScreenP = screenOffset + group->gameCamera.screenCameraOffset;
    Vec3 result = realScreenP.x * group->gameCamera.X + realScreenP.y * group->gameCamera.Y;
    
    return result;
}

inline ObjectTransform DefaultOverDrawTransform( r32 z = 0.0f )
{
    ObjectTransform result = {};
    return result;
}

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

inline void PushSetup( RenderGroup* group, RenderSetup* setup );
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