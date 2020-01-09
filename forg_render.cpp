inline TexturedVertex* ReserveQuad(RenderGroup* group, b32 transparent, r32 sortKey)
{
    TexturedVertex* result = 0;
    GameRenderCommands* commands = group->commands;
    RenderBuffer* buffer = transparent ? &commands->transparent : &commands->opaque;
    
    if(!buffer->currentSetup)
    {
        u32 size = sizeof(RenderSetup);
        if(commands->usedSize + size <= commands->maxBufferSize)
        {
            if(buffer->quadCount + 1 <= buffer->maxQuadCount)
            {
                RenderSetup* newSetup = (RenderSetup*) (commands->pushMemory + commands->usedSize);
                *newSetup = group->lastSetup;
                newSetup->quadCount = 0;
                newSetup->quadStartingIndex = buffer->quadCount;
                newSetup->vertexArrayOffset = (buffer->quadCount * 4);
                newSetup->indexArrayOffset = (buffer->quadCount * 6);
                newSetup->next = 0;
                
                if(!buffer->lastSetup)
                {
                    Assert(!buffer->firstSetup);
                    buffer->firstSetup = buffer->lastSetup = newSetup;
                }
                else
                {
                    buffer->lastSetup->next = newSetup;
                    buffer->lastSetup = newSetup;
                }
                
                buffer->currentSetup = newSetup;
                commands->usedSize += size;
            }
            else
            {
                InvalidCodePath;
            }
        }
        else
        {
            InvalidCodePath;
        }
    }
    
    RenderSetup* setup = buffer->currentSetup;
    if(setup)
    {
        u32 vertexIndex = setup->vertexArrayOffset + (setup->quadCount * 4);
        result = buffer->vertexArray + vertexIndex;
        
        SortEntry* sortKeyPtr = buffer->sortKeyArray + buffer->quadCount;
        sortKeyPtr->sortKey = sortKey;
        sortKeyPtr->index = buffer->quadCount;
        
        setup->quadCount += 1;
        buffer->quadCount += 1;
    }
    
    return result;
}

inline void Clear(RenderGroup* renderGroup, Vec4 color)
{
    renderGroup->commands->clearColor = color;
}

inline r32 ComputeSortKey(RenderGroup* group, Vec3 P, b32 flat)
{
#if 1
    r32 result = flat ? P.z : Dot(P, group->gameCamera.Z);
#else
    r32 result = P.z;
#endif
    return result;
}

inline Vec3 GetCameraOffset(RenderGroup* group, Vec3 P)
{
    Vec3 result = P.x * group->gameCamera.X + P.y * group->gameCamera.Y + P.z * group->gameCamera.Z;
    return result;
}

inline void PushVertex(TexturedVertex* vert, u32 vertexIndex, Vec3 P, Vec2 UV, u32 C, Lights lights, r32 modulationPercentage, u16 textureIndex, r32 lightInfluence, r32 lightYInfluence, r32 windInfluence, u8 windFrequency, r32 dissolvePercentage, r32 alphaThreesold)
{
    vert->P = V4(P, 0);
    vert->UV = UV;
    vert->color = C;
    vert->lightStartingIndex = lights.startingIndex;
    vert->lightEndingIndex = lights.endingIndex;
    vert->modulationPercentage = (u8) modulationPercentage;
    vert->lightInfluence = (u8) ((1.0f - lightInfluence) * (r32) 0xff);
    vert->lightYInfluence = (u8) ((1.0f - lightYInfluence) * (r32) 0xff);
    vert->windInfluence = (u8) (windInfluence * (r32) 0xff);
    vert->windFrequency = windFrequency;
    vert->dissolvePercentage = (u8) (dissolvePercentage * (r32) 0xff);
    vert->textureIndex = textureIndex;
    vert->alphaThreesold = (u8) (alphaThreesold * (r32) 0xff);
    vert->seed = (u8) vertexIndex;
}

inline void PushMagicQuad(RenderGroup* group, Vec3 P, b32 flat, Vec3 lateral, Vec3 up, Vec2 invUV, u32 color, RenderTexture texture, Lights lights, r32 modulationPercentage, r32 lightInfluence, r32 lightYInfluence, Vec4 windInfluences, u8 windFrequency, Vec4 dissolvePercentages, r32 alphaThreesold, u8 seed)
{
    TIMED_FUNCTION();
    
    r32 sortKey = ComputeSortKey(group, P, flat);
    
    b32 transparent = (alphaThreesold == 0.0f);
    TexturedVertex* vert = ReserveQuad(group, transparent, sortKey);
    if(vert)
    {
        GameRenderCommands* commands = group->commands;
        u16 textureIndex = (u16) texture.index;
        
        Vec2 UV0 = Hadamart(V2(0, 0), invUV);
        Vec2 UV1 = Hadamart(V2(1, 0), invUV);
        Vec2 UV2 = Hadamart(V2(1, 1), invUV);
        Vec2 UV3 = Hadamart(V2(0, 1), invUV);
        
        Vec3 P0 = P - lateral - up;
        Vec3 P1 = P + lateral - up;
        Vec3 P2 = P + lateral + up;
        Vec3 P3 = P - lateral + up;
        
        u32 C = color;
        
        if((lateral.x >= 0 && up.y >= 0) ||
           (lateral.x < 0 && up.y < 0))
        {
            PushVertex(vert + 0, seed + 0, P0, UV0, C, lights, modulationPercentage, textureIndex, lightInfluence, lightYInfluence, windInfluences.x, windFrequency, dissolvePercentages.x, alphaThreesold);
            PushVertex(vert + 1, seed + 1, P1, UV1, C, lights, modulationPercentage, textureIndex, lightInfluence, lightYInfluence, windInfluences.y, windFrequency, dissolvePercentages.y, alphaThreesold);
            PushVertex(vert + 2, seed + 2, P2, UV2, C, lights, modulationPercentage, textureIndex, lightInfluence, lightYInfluence, windInfluences.z, windFrequency, dissolvePercentages.z, alphaThreesold);
            PushVertex(vert + 3, seed + 3, P3, UV3, C, lights, modulationPercentage, textureIndex, lightInfluence, lightYInfluence, windInfluences.w, windFrequency, dissolvePercentages.w, alphaThreesold);
        }
        else
        {
            PushVertex(vert + 0, seed + 0, P0, UV0, C, lights, modulationPercentage, textureIndex, lightInfluence, lightYInfluence, windInfluences.x, windFrequency, dissolvePercentages.x, alphaThreesold);
            PushVertex(vert + 1, seed + 1, P3, UV3, C, lights, modulationPercentage, textureIndex, lightInfluence, lightYInfluence, windInfluences.w, windFrequency, dissolvePercentages.y, alphaThreesold);
            PushVertex(vert + 2, seed + 2, P2, UV2, C, lights, modulationPercentage, textureIndex, lightInfluence, lightYInfluence, windInfluences.z, windFrequency, dissolvePercentages.z, alphaThreesold);
            PushVertex(vert + 3, seed + 3, P1, UV1, C, lights, modulationPercentage, textureIndex, lightInfluence, lightYInfluence, windInfluences.y, windFrequency, dissolvePercentages.w, alphaThreesold);
        }
    }
}

inline void PushQuad(RenderGroup* group, RenderTexture texture, Lights lights,
                     TexturedVertex* vert,
                     Vec3 P0, Vec2 UV0, u32 C0,
                     Vec3 P1, Vec2 UV1, u32 C1,
                     Vec3 P2, Vec2 UV2, u32 C2,
                     Vec3 P3, Vec2 UV3, u32 C3, 
                     r32 modulationPercentage, 
                     r32 lightInfluence, r32 lightYInfluence, 
                     Vec4 windInfluences, u8 windFrequency,Vec4 dissolvePercentages, r32 alphaThreesold)
{
    Vec2 invUV = GetInvUV(texture.width, texture.height);
    UV0 = Hadamart(UV0, invUV);
    UV1 = Hadamart(UV1, invUV);
    UV2 = Hadamart(UV2, invUV);
    UV3 = Hadamart(UV3, invUV);
    
    if(vert)
    {
        GameRenderCommands* commands = group->commands;
        
        u16 textureIndex = (u16) texture.index;
        PushVertex(vert + 0, 0, P0, UV0, C0, lights, modulationPercentage, textureIndex, lightInfluence, lightYInfluence, windInfluences.x, windFrequency, dissolvePercentages.x, alphaThreesold);
        PushVertex(vert + 1, 0, P1, UV1, C1, lights, modulationPercentage, textureIndex, lightInfluence, lightYInfluence, windInfluences.y, windFrequency, dissolvePercentages.y, alphaThreesold);
        PushVertex(vert + 2, 0, P2, UV2, C2, lights, modulationPercentage, textureIndex, lightInfluence, lightYInfluence, windInfluences.z, windFrequency, dissolvePercentages.z, alphaThreesold);
        PushVertex(vert + 3, 0, P3, UV3, C3, lights, modulationPercentage, textureIndex, lightInfluence, lightYInfluence, windInfluences.w, windFrequency, dissolvePercentages.w, alphaThreesold);
    }
}

inline void PushQuad(RenderGroup* group, RenderTexture texture, Lights lights,
                     TexturedVertex* vert,
                     Vec3 P0, Vec2 UV0, Vec4 C0,
                     Vec3 P1, Vec2 UV1, Vec4 C1,
                     Vec3 P2, Vec2 UV2, Vec4 C2,
                     Vec3 P3, Vec2 UV3, Vec4 C3, r32 modulationPercentage, r32 lightInfluence, r32 lightYInfluence, Vec4 windInfluences, u8 windFrequency, Vec4 dissolvePercentages, r32 alphaThreesold)
{
    PushQuad(group, texture, lights,
             vert,
             P0, UV0, RGBAPack8x4(C0 * 255.0f),
             P1, UV1, RGBAPack8x4(C1 * 255.0f),
             P2, UV2, RGBAPack8x4(C2 * 255.0f),
             P3, UV3, RGBAPack8x4(C3 * 255.0f), modulationPercentage, lightInfluence, lightYInfluence, windInfluences, windFrequency, dissolvePercentages, alphaThreesold);
}

inline void PushTextureSegment(RenderGroup* group, RenderTexture texture, Vec4 color, Vec3 fromP, Vec3 toP, r32 tickness, r32 alphaThreesold, Lights lights = {0, 0}, Vec4 dissolvePercentages = {})
{
    Vec3 line = toP - fromP;
    Vec3 perp = Cross(group->gameCamera.Z, line);
    Vec3 normPerp = tickness * Normalize(perp);
    
    Vec3 P0 = fromP - normPerp;
    Vec3 P1 = toP - normPerp;
    Vec3 P2 = toP + normPerp;
    Vec3 P3 = fromP + normPerp;
    
    u32 c = StoreColor(color);
    
    b32 tranparent = (alphaThreesold == 0.0f || color.a != 1.0f);
    r32 sortKey = ComputeSortKey(group, fromP, true);
    TexturedVertex* vert = ReserveQuad(group, tranparent, sortKey);
    PushQuad(group, texture, lights, vert, 
             P0, V2(0, 0), c, 
             P1, V2(1, 0), c, 
             P2, V2(1, 1), c, 
             P3, V2(0, 1), c, 
             0, 0, 0, {}, 0, dissolvePercentages, alphaThreesold);
}

internal void PushTextureSegment(RenderGroup* group, BitmapId ID, Vec4 color, Vec3 fromP, Vec3 toP, r32 tickness, Lights lights, Vec4 dissolvePercentages = {})
{
    Bitmap* bitmap = GetBitmap(group->assets, ID).bitmap;
    if(bitmap)
    {
        PushTextureSegment(group, bitmap->textureHandle, color, fromP, toP, tickness, bitmap->alphaThreesold, lights, dissolvePercentages);
    }
    else
    {
        LoadBitmap(group->assets, ID);
    }
}

inline void PushDebugLine(RenderGroup* group, Vec3 color, Vec3 fromP, Vec3 toP, r32 tickness, Lights lights = {})
{
    PushTextureSegment(group, group->whiteTexture, V4(color, 1.0f), fromP, toP, tickness, 0, lights);
}

inline void PushRect_(RenderGroup* group, ObjectTransform transform, Vec3 P, Vec2 dim, Lights lights)
{
    GameRenderCommands* commands = group->commands;
    Vec3 XAxis = V3(1, 0, 0);
    Vec3 YAxis = V3(0, 1, 0);
    
    if(transform.upright)
    {
        P += GetCameraOffset(group, transform.cameraOffset);
        
        Vec3 XAxis1 = XAxis.x * group->gameCamera.X + XAxis.y * group->gameCamera.Y;
        Vec3 YAxis1 = YAxis.x * group->gameCamera.X + YAxis.y * group->gameCamera.Y;
        
        XAxis = XAxis1;
        YAxis = YAxis1;
        //YAxis = Lerp(YAxis0, 0.5f, YAxis1);
        //YAxis.z = Lerp(YAxis0.z, 0.0f, YAxis1.z);
    }
    else
    {
        //P += objectTransform.offset;
    }
    
    
    Vec3 halfDim = V3(0.5f * dim, 0.0f);
    Vec3 min = P - (halfDim.x * XAxis) - (halfDim.y * YAxis);
    Vec3 max = P + (halfDim.x * XAxis) + (halfDim.y * YAxis);
    
    Vec2 minUV = V2(0.0f, 0.0f);
    Vec2 maxUV = V2(1.0f, 1.0f);
    
    Vec4 C = transform.tint;
    b32 transparent = (transform.tint.a != 1.0f);
    b32 flat = !transform.upright;
    r32 sortKey = ComputeSortKey(group, P, flat);
    TexturedVertex* vert = ReserveQuad(group, transparent, sortKey);
    PushQuad(group, group->whiteTexture, lights, vert,
             V3(min.x, min.y, min.z), V2(minUV.x, minUV.y), C,
             V3(max.x, min.y, min.z), V2(maxUV.x, minUV.y), C,
             V3(max.x, max.y, max.z), V2(maxUV.x, maxUV.y), C,
             V3(min.x, max.y, max.z), V2(minUV.x, maxUV.y), C, transform.modulationPercentage, transform.lightInfluence, transform.lightYInfluence, transform.windInfluences, transform.windFrequency, transform.dissolvePercentages, 0);
}

inline void PushRect(RenderGroup* renderGroup, ObjectTransform objectTransform, Vec3 P, Vec2 dim,Lights lights = {0, 0})
{
    PushRect_(renderGroup, objectTransform, P, dim, lights);
}

inline void PushRect(RenderGroup* renderGroup, ObjectTransform objectTransform, Rect2 rect, Lights lights = {0, 0})
{
    PushRect(renderGroup, objectTransform, V3(GetCenter(rect), 0.0f), GetDim(rect), lights);
}


inline void PushRectOutline(RenderGroup* renderGroup, ObjectTransform objectTransform, Vec3 P, 
                            Vec2 dim, r32 thickness)
{
    Vec2 halfDim = 0.5f * dim;
    PushRect(renderGroup, objectTransform, P + V3(0.0f, halfDim.y, 0.0f), V2(dim.x, thickness));
    PushRect(renderGroup, objectTransform, P +V3(0.0f, -halfDim.y, 0.0f), V2(dim.x, thickness));
    PushRect(renderGroup, objectTransform, P + V3(halfDim.x, 0.0f, 0.0f), V2(thickness, dim.y));
    PushRect(renderGroup, objectTransform, P + V3(-halfDim.x, 0.0f, 0.0f), V2(thickness, dim.y));
}

inline void PushRectOutline(RenderGroup* renderGroup, ObjectTransform objectTransform, Rect2 rect, r32 thickness, r32 z = 0.0f)
{
    PushRectOutline(renderGroup, objectTransform, V3(GetCenter(rect), z), GetDim(rect), thickness);
}


inline void PushDebugCubeOutline(RenderGroup* group, Rect3 R, Vec3 color, r32 tickness)
{
    r32 NX = R.min.x;
    r32 NY = R.min.y;
    r32 NZ = R.min.z;
    r32 PX = R.max.x;
    r32 PY = R.max.y;
    r32 PZ = R.max.z;
    
    Vec3 P0 = { NX, NY, PZ };
    Vec3 P1 = { PX, NY, PZ };
    Vec3 P2 = { PX, PY, PZ };
    Vec3 P3 = { NX, PY, PZ };
    Vec3 P4 = { NX, NY, NZ };
    Vec3 P5 = { PX, NY, NZ };
    Vec3 P6 = { PX, PY, NZ };
    Vec3 P7 = { NX, PY, NZ };
    
    PushDebugLine(group, color, P0, P1, tickness);
    PushDebugLine(group, color, P0, P3, tickness);
    PushDebugLine(group, color, P0, P4, tickness);
    
    PushDebugLine(group, color, P2, P3, tickness);
    PushDebugLine(group, color, P2, P6, tickness);
    PushDebugLine(group, color, P2, P1, tickness);
    
    PushDebugLine(group, color, P5, P6, tickness);
    PushDebugLine(group, color, P5, P4, tickness);
    PushDebugLine(group, color, P5, P1, tickness);
    
    PushDebugLine(group, color, P7, P3, tickness);
    PushDebugLine(group, color, P7, P4, tickness);
    PushDebugLine(group, color, P7, P6, tickness);
}

struct BitmapDim
{
    Vec2 size;
    Vec3 P;
    Vec3 XAxis;
    Vec3 YAxis;
};

inline Vec3 GetAlignP(BitmapDim dim, Vec2 alignment)
{
    Vec3 result = dim.P + alignment.x * dim.size.x * dim.XAxis + alignment.y * dim.size.y * dim.YAxis;
    return result;
}


inline BitmapDim GetBitmapDim(Bitmap* bitmap, Vec2 pivot, Vec3 P, Vec3 XAxis, Vec3 YAxis, r32 height, Vec2 scale = V2(1.0f, 1.0f))
{
    BitmapDim result;
    
    result.size =  Hadamart(V2(height * bitmap->widthOverHeight, height), scale);
    
    pivot = Hadamart(pivot, result.size);
    Vec3 finalPivot = pivot.x * XAxis + pivot.y * YAxis;
    result.P = P - finalPivot;
    result.XAxis = XAxis;
    result.YAxis = YAxis;
    
    return result;
}

inline void PushTexture_(RenderGroup* group, RenderTexture texture, Vec3 P, b32 flat, Vec3 XAxis, Vec3 YAxis, Vec4 color, Lights lights, r32 modulationPercentage, r32 lightInfluence, r32 lightYInfluence, Vec4 windInfluences, u8 windFrequency, Vec4 dissolvePercentages, r32 alphaThreesold, b32 transparent)
{
    Vec2 minUV = V2(0, 0);
    Vec2 maxUV = V2(1, 1);
    
    u32 colorInt = RGBAPack8x4(color * 255.0f);
    
    Vec3 minXminY = P;
    Vec3 maxXminY = P + XAxis;
    Vec3 minXmaxY = P + YAxis;
    Vec3 maxXmaxY = P + XAxis + YAxis;
    
    r32 sortKey = ComputeSortKey(group, P, flat);
    TexturedVertex* vert = ReserveQuad(group, transparent, sortKey);
    if((XAxis.x >= 0 && YAxis.y >= 0) ||
       (XAxis.x < 0 && YAxis.y < 0))
    {
        PushQuad(group, texture, lights, vert,
                 minXminY, V2(minUV.x, minUV.y), colorInt,
                 maxXminY, V2(maxUV.x, minUV.y), colorInt,
                 maxXmaxY, V2(maxUV.x, maxUV.y), colorInt,
                 minXmaxY, V2(minUV.x, maxUV.y), colorInt, modulationPercentage, lightInfluence, lightYInfluence, windInfluences, windFrequency, dissolvePercentages, alphaThreesold);
    }
    else
    {
        PushQuad(group, texture, lights, vert,
                 minXminY, V2(minUV.x, minUV.y), colorInt,
                 minXmaxY, V2(minUV.x, maxUV.y), colorInt,
                 maxXmaxY, V2(maxUV.x, maxUV.y), colorInt,
                 maxXminY, V2(maxUV.x, minUV.y), colorInt, modulationPercentage, lightInfluence, lightYInfluence, windInfluences, windFrequency, dissolvePercentages, alphaThreesold);
    }
}

inline void PushTexture(RenderGroup* group, RenderTexture texture, Vec3 P, b32 flat, Vec3 XAxis = V3(1, 0, 0), Vec3 YAxis = V3(0, 1, 0), Vec4 color = V4(1, 1, 1, 1), Lights lights = {}, r32 modulationPercentage = 0.0f, r32 lightInfluence = 0, r32 lightYInfluence = 0, Vec4 windInfluences = {}, u8 windFrequency = 0, Vec4 dissolvePercentages = {}, r32 alphaThreesold = 0)
{
    b32 transparent = (color.a != 1.0f);
    PushTexture_(group, texture, P, flat, XAxis, YAxis, color, lights, modulationPercentage, lightInfluence, lightYInfluence, windInfluences, windFrequency, dissolvePercentages, alphaThreesold, transparent);
}


inline void PushTransparentTexture(RenderGroup* group, RenderTexture texture, Vec3 P, b32 flat, Vec3 XAxis = V3(1, 0, 0), Vec3 YAxis = V3(0, 1, 0), Vec4 color = V4(1, 1, 1, 1), Lights lights = {}, r32 modulationPercentage = 0.0f, r32 lightInfluence = 0, r32 lightYInfluence = 0, Vec4 windInfluences = {}, u8 windFrequency = 0, Vec4 dissolvePercentages = {}, r32 alphaThreesold = 0)
{
    b32 transparent = true;
    PushTexture_(group, texture, P, flat, XAxis, YAxis, color, lights, modulationPercentage, lightInfluence, lightYInfluence, windInfluences, windFrequency, dissolvePercentages, alphaThreesold, transparent);
}

inline BitmapDim PushBitmap_(RenderGroup* renderGroup, ObjectTransform transform, ColoredBitmap coloredBitmap,  Vec3 P, r32 height, Lights lights, Vec2 pivot)
{
    BitmapDim result = {};
    GameRenderCommands* commands = renderGroup->commands;
    
    Bitmap* bitmap = coloredBitmap.bitmap;
    r32 alphaThreesold = bitmap->alphaThreesold;
    Vec4 color = Hadamart(transform.tint, coloredBitmap.coloration);
    
    if(bitmap->width && bitmap->height)
    {
        r32 angleRad = DegToRad(transform.angle);
        Vec3 XAxis = V3(Cos(angleRad), Sin(angleRad), 0.0f);
        Vec3 YAxis  = V3(Perp(XAxis.xy), 0.0f);
        
        if(transform.flipOnYAxis)
        {
            XAxis.x = -XAxis.x;
            YAxis.xy = -Perp(XAxis.xy);
            transform.cameraOffset.x = -transform.cameraOffset.x;
        }
        
        if(transform.upright)
        {
			P += GetCameraOffset(renderGroup, transform.cameraOffset);
            
            Vec3 XAxis0 = V3(XAxis.x, 0.0f, XAxis.y);
            Vec3 YAxis0 = V3(YAxis.x, 0.0f, YAxis.y);
            
            Vec3 XAxis1 = XAxis.x * renderGroup->gameCamera.X + XAxis.y * renderGroup->gameCamera.Y;
            Vec3 YAxis1 = YAxis.x * renderGroup->gameCamera.X + YAxis.y * renderGroup->gameCamera.Y;
            
            XAxis = XAxis1;
            YAxis = YAxis1;
            
            //YAxis = Lerp(YAxis0, 0.5f, YAxis1);
            //YAxis.z = Lerp(YAxis0.z, 0.0f, YAxis1.z);
        }
        else
        {
            P += transform.cameraOffset;
        }
        
        result = GetBitmapDim(bitmap, pivot, P, XAxis, YAxis, height, transform.scale);
        
        XAxis *= result.size.x;
        YAxis *= result.size.y;
        
        b32 flat = !transform.upright;
        b32 tranparent = (alphaThreesold == 0.0f || color.a != 1.0f);
        if(!transform.dontRender)
        {
            if(tranparent)
            {
                PushTransparentTexture(renderGroup, bitmap->textureHandle, result.P, flat, XAxis, YAxis, color, lights, transform.modulationPercentage, transform.lightInfluence, transform.lightYInfluence, transform.windInfluences, transform.windFrequency, transform.dissolvePercentages, alphaThreesold);
            }
            else
            {
                PushTexture(renderGroup, bitmap->textureHandle, result.P, flat, XAxis, YAxis, color, lights, transform.modulationPercentage, transform.lightInfluence, transform.lightYInfluence, transform.windInfluences, transform.windFrequency, transform.dissolvePercentages, alphaThreesold);
            }
        }
    }
    
    return result;
}

inline BitmapDim GetBitmapDim(RenderGroup* renderGroup, ObjectTransform transform, BitmapId ID, Vec3 P, r32 height)
{
    
    BitmapDim result = {};
    ColoredBitmap bitmap = GetBitmap(renderGroup->assets, ID);
    if(bitmap.bitmap)
    {
        transform.dontRender = true;
        result = PushBitmap_(renderGroup, transform, bitmap, P, height, {}, bitmap.pivot);
    }
    else
    {
        LoadBitmap(renderGroup->assets, ID);
    }
    
    return result;
}

inline BitmapDim PushBitmap(RenderGroup* renderGroup, ObjectTransform transform, BitmapId ID, Vec3 P, r32 height, Lights lights = {0, 0})
{
    BitmapDim result = {};
    ColoredBitmap bitmap = GetBitmap(renderGroup->assets, ID);
    if(bitmap.bitmap)
    {
        result = PushBitmap_(renderGroup, transform, bitmap, P, height, lights, bitmap.pivot);
    }
    else
    {
        LoadBitmap(renderGroup->assets, ID);
    }
    
    return result;
}

inline BitmapDim PushBitmapWithPivot(RenderGroup* renderGroup, ObjectTransform transform, BitmapId ID, Vec3 P, Vec2 pivot, r32 height, Lights lights = {0, 0})
{
    BitmapDim result = {};
    ColoredBitmap bitmap = GetBitmap(renderGroup->assets, ID);
    if(bitmap.bitmap)
    {
        result = PushBitmap_(renderGroup, transform, bitmap, P, height, lights, pivot);
    }
    else
    {
        LoadBitmap(renderGroup->assets, ID);
    }
    
    return result;
}

internal void PushBitmapInRect(RenderGroup* group, ObjectTransform transform, BitmapId ID, Vec3 P, Rect2 rect, Lights lights = {})
{
    if(IsValid(ID))
    {
        
        Bitmap* bitmap = GetBitmap(group->assets, ID).bitmap;
        PAKBitmap* bitmapInfo = GetBitmapInfo(group->assets, ID);
        if(bitmap)
        {
            Vec2 rectDim = GetDim(rect);
            transform.dontRender = true;
            BitmapDim dim = PushBitmap(group, transform, ID, P, rectDim.y, lights);
            
            Rect2 imageDim = RectMinDim(dim.P.xy, dim.size);
            Vec2 actualDim = GetDim(imageDim);
            
            r32 scale = Min(rectDim.x / actualDim.x, rectDim.y / actualDim.y);
            
            dim = PushBitmap(group, transform, ID, P, rectDim.y * scale, lights);
            imageDim = RectMinDim(dim.P.xy, dim.size);
            actualDim = GetDim(imageDim);
            
            Vec2 drawnP = GetCenter(imageDim);
            Vec3 offset = V3(drawnP - GetCenter(rect), 0);
            Vec3 finalP = P - offset;
            
            transform.dontRender = false;
            PushBitmap(group, transform, ID, finalP, rectDim.y * scale, lights);
        }
        else
        {
            LoadBitmap(group->assets, ID);
        }
    }
}


#if 0
inline void PushCube_(RenderGroup* group, Vec3 P, r32 height, r32 width, RenderTexture texture, Vec4 color, Lights lights)
{
    r32 radius = 0.5f * width;
    
    r32 NX = P.x - radius;
    r32 NY = P.y - radius;
    r32 NZ = P.z - height;
    r32 PX = P.x + radius;
    r32 PY = P.y + radius;
    r32 PZ = P.z;
    
    Vec4 P0 = { NX, NY, PZ, 0 };
    Vec4 P1 = { PX, NY, PZ, 0 };
    Vec4 P2 = { PX, PY, PZ, 0 };
    Vec4 P3 = { NX, PY, PZ, 0 };
    Vec4 P4 = { NX, NY, NZ, 0 };
    Vec4 P5 = { PX, NY, NZ, 0 };
    Vec4 P6 = { PX, PY, NZ, 0 };
    Vec4 P7 = { NX, PY, NZ, 0 };
    
    Vec2 T0 = V2(0, 0);
    Vec2 T1 = V2(1, 0);
    Vec2 T2 = V2(0, 1);
    Vec2 T3 = V2(1, 1);
    
    Vec4 TC = color;
    Vec4 BC = color;
    Vec4 TGr = color;
    Vec4 BGr = color;
    
    Vec4 wind = {};
    Vec4 dissolve = {};
    
    u32 sliceIndex = GetSliceIndex(group, Slice_Flat, P);
    u32 vertexIndex = ReserveQuads(group, 6, sliceIndex);
    PushQuad(group, texture, lights, vertexIndex, P0, T0, TC, P1, T1, TC, P2, T2, TC, P3, T3, TC, 0, 0, 0, wind, 0, dissolve);
    PushQuad(group, texture, lights, vertexIndex, P7, T0, BC, P6, T1, BC, P5, T2, BC, P4, T3, BC, 0, 0, 0, wind, 0, dissolve);
    
    PushQuad(group, texture, lights, &vertexes, P4, T0, BGr, P5, T1, BGr, P1, T2, TGr, P0, T3, TGr, 0, 0, 0, wind, 0, dissolve);
    
    PushQuad(group, texture, lights, &vertexes, P2, T0, TGr, P6, T1, BGr, P7, T2, BGr, P3, T3, TGr, 0, 0, 0, wind, 0, dissolve);
    
    PushQuad(group, texture, lights, &vertexes, P1, T0, TGr, P5, T1, BGr, P6, T2, BGr, P2, T3, TGr, 0, 0, 0, wind, 0, dissolve);
    
    PushQuad(group, texture, lights, &vertexes, P7, T0, BGr, P4, T1, BGr, P0, T2, TGr, P3, T3, TGr, 0, 0, 0, wind, 0, dissolve);
    
}

inline void PushTexturedCube(RenderGroup* group, Vec3 P, r32 height, r32 width, BitmapId ID, Vec4 color = V4(1.0f, 1.0f, 1.0f, 1.0f), Lights lights = {0, 0})
{
    ColoredBitmap bitmap = GetBitmap(group->assets, ID);
    if(bitmap.bitmap)
    {
        Bitmap* actualBitmap = bitmap.bitmap;
        color = Hadamart(color, bitmap.coloration);
        PushCube_(group, P, height, width, actualBitmap->textureHandle, color, lights);
    }
    else
    {
        LoadBitmap(group->assets, ID);
    }
}

inline void PushCube(RenderGroup* group, Vec3 P, r32 height, r32 width, Vec4 color, Lights lights = {0, 0})
{
    PushCube_(group, P, height, width, group->whiteTexture, color, lights);
}
#endif

inline Vec2 GetScreenP(RenderGroup* group, Vec3 Point)
{
    Vec4 P = group->debugCamera.proj.forward * V4(Point, 1.0f);
    P.xyz /= P.w;
    
    P.x = group->screenDim.x * 0.5f * (1.0f + P.x);
    P.y = group->screenDim.y * 0.5f * (1.0f + P.y);
    
    return P.xy;
}

inline Rect2i GetClipRect_(RenderGroup* group, Vec3 P, Vec2 dim)
{
    Vec2 min = GetScreenP(group, P);
    Vec2 max = GetScreenP(group, P + V3(dim, 0));
    
    Rect2i result = RectMinMax(RoundReal32ToI32(min.x), RoundReal32ToI32(min.y),  RoundReal32ToI32(max.x), RoundReal32ToI32(max.y));
    
    return result;
}

inline Rect2i GetScreenRect(RenderGroup* group)
{
    Vec2 min = V2(0, 0);
    Vec2 max = group->screenDim;
    
    Rect2i result = RectMinMax(RoundReal32ToI32(min.x), RoundReal32ToI32(min.y),  RoundReal32ToI32(max.x), RoundReal32ToI32(max.y));
    
    return result;
}

inline Rect2i GetClipRect(RenderGroup* group, Rect2 rect, r32 Z)
{
    Rect2i result = GetClipRect_(group, V3(rect.min, Z), GetDim(rect));
    return result;
}

inline u16 PushPointLight(RenderGroup* renderGroup, Vec3 P, Vec3 color, r32 strength)
{
    u16 result = 0xffff;
    if(renderGroup->commands->lightCount < ArrayCount(renderGroup->commands->lightSource0))
    {
        result = renderGroup->commands->lightCount++;
        
        renderGroup->commands->lightSource0[result].xyz = P;
        renderGroup->commands->lightSource0[result].w = strength;
        renderGroup->commands->lightSource1[result].rgb = color;
        
    }
    return result;
}


enum TextOperation
{
    TextOp_draw,
    TextOp_getSize,
};

internal Rect2 PushText_(RenderGroup* group, FontId fontID, Font* font, PAKFont* info, char* string, Vec3 P, r32 fontScale, TextOperation op, Vec4 color, b32 startingSpace, b32 drawShadow)
{
    Rect2 result = InvertedInfinityRect2();
    if(font && info)
    {
        u32 prevCodePoint = startingSpace ? ' ' : 0;
        for( char* at = string; *at; at++)
        {
            u8 codePoint = *at;
            P.x += fontScale * GetHorizontalAdvanceForPair(font, info, prevCodePoint, codePoint);
            if( codePoint != ' ' )
            {
                BitmapId ID = GetBitmapForGlyph(group->assets, fontID, codePoint);
                if(IsValid(ID))
                {
                    PAKBitmap* glyphInfo = GetBitmapInfo(group->assets, ID);
                    r32 glyphHeight = fontScale * glyphInfo->dimension[1];
                    if(op == TextOp_getSize)
                    {
                        Bitmap* bitmap = GetBitmap(group->assets, ID).bitmap;
                        PAKBitmap* bitmapInfo = GetBitmapInfo(group->assets, ID);
                        if(bitmap)
                        {
                            BitmapDim dim = GetBitmapDim(bitmap, V2(bitmapInfo->align[0], bitmapInfo->align[1]), P, V3( 1.0f, 0.0f, 0.0f ), V3( 0.0f, 1.0f, 0.0f ), glyphHeight );
                            Rect2 glyphRect = RectMinDim(dim.P.xy, dim.size);
                            result = Union(result, glyphRect);
                        }
                        else
                        {
                            LoadBitmap(group->assets, ID);
                        }
                    }
                    else
                    {
                        ObjectTransform transform = FlatTransform();
                        transform.tint = V4(0, 0, 0, 1);
                        if(drawShadow)
                        {
                            PushBitmap(group, transform, ID, P + V3(2.0f, -2.0f, +0.01f), glyphHeight);
                        }
                        
                        transform.tint = color;
                        PushBitmap(group, transform, ID, P + V3(0, 0, 0.02f), glyphHeight);
                    }
                }
            }
            
            prevCodePoint = codePoint;
        }
        
    }
    
    return result;
}


internal void PushText(RenderGroup* group, FontId fontID, char* string, Vec3 P,r32 fontScale = 1.0f, Vec4 color = V4(1, 1, 1, 1), b32 startingSpace = false, b32 drawShadow = true)
{
    Font* font = GetFont(group->assets, fontID);
    if(font)
    {
        PAKFont* info = GetFontInfo(group->assets, fontID);
        PushText_(group, fontID, font, info, string, P, fontScale, TextOp_draw, color, startingSpace, drawShadow);
    }
    else
    {
        LoadFont(group->assets, fontID);
    }
}

internal Rect2 GetTextDim(RenderGroup* group, FontId fontID, char* string, Vec3 P, r32 fontScale = 1.0f, b32 startingSpace = false)
{
    Rect2 result = InvertedInfinityRect2();
    
    Font* font = GetFont(group->assets, fontID);
    if(font)
    {
        PAKFont* info = GetFontInfo(group->assets, fontID);
        
        result = PushText_(group, fontID, font, info, string, P, fontScale, TextOp_getSize, V4(1, 1, 1, 1), startingSpace, false);
    }
    else
    {
        LoadFont(group->assets, fontID);
    }
    
    return result;
}

internal void PushTextEnclosed(RenderGroup* group, FontId fontID, char* string, Rect2 rect, r32 fontScale = 1.0f, Vec4 color = V4(1, 1, 1, 1), b32 drawShadow = true)
{
    Vec3 refP = V3(rect.min, 0);
    Rect2 currentDim = GetTextDim(group, fontID, string, refP, fontScale);
    
    r32 coeffY = GetDim(rect).y / GetDim(currentDim).y;
    r32 coeffX = GetDim(rect).x / GetDim(currentDim).x;
    
    r32 coeff = Min(coeffX, coeffY);
    
    r32 adjustedFontScale = coeff * fontScale;
    Rect2 actualDim = GetTextDim(group, fontID, string, refP, adjustedFontScale);
    
    Vec2 offset = GetCenter(rect) - GetCenter(actualDim);
    Vec3 finalP = refP + V3(offset, 0);
    
    PushText(group, fontID, string, finalP, adjustedFontScale, color, false, drawShadow);
}

inline void ResetQuads(RenderGroup* group)
{
    group->commands->opaque.currentSetup = 0;
    group->commands->transparent.currentSetup = 0;
}

inline void PushSetup_(RenderGroup* group, RenderSetup* setup)
{
    group->lastSetup = *setup;
    ResetQuads(group);
}

internal void SetMagicVectors()
{
    m4x4 cameraO = ZRotation(cameraOrbit) * XRotation(cameraPitch);
    magicLateralVector = GetColumn(cameraO, 0);
    magicUpVector = GetColumn(cameraO, 1);
}

inline RenderGroup BeginRenderGroup(Assets* assets, GameRenderCommands* commands)
{
    SetMagicVectors();
    
    RenderGroup result = {};
    result.assets = assets;
    result.commands = commands;
    result.screenDim = V2i(commands->settings.width, commands->settings.height);
    
    m4x4_inv I = {Identity(), Identity()};
    
    RenderSetup setup = {};
    setup.rect = { 0, 0, (i32)commands->settings.width, (i32)commands->settings.height };
    setup.proj = I.forward;
    setup.renderTargetIndex = 0;
    setup.ambientLightColor = V3(1, 1, 1);
    
    PushSetup_(&result, &setup);
    
    return result;
}

inline void EndRenderGroup(RenderGroup* group)
{
}

inline Vec3 ProjectOnGround(Vec3 P, Vec3 cameraP)
{
    Vec3 rayOriginP = cameraP;
    Vec3 rayDirection = Normalize(P - rayOriginP);
    
    Vec3 planeOrigin = V3(0, 0, 0);
    Vec3 planeNormal = V3(0, 0, 1);
    
    Vec3 groundP = RayPlaneIntersection(rayOriginP, rayDirection, planeOrigin, planeNormal);
    
    return groundP;
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
        result = Hadamart(clipSpace.xy, screenCenter);
        
    }
    
    return result;
}

inline Rect2 ProjectOnScreen(RenderGroup* group, BitmapDim dim)
{
    r32 cameraZ;
    
    Vec3 XAxis = dim.XAxis * dim.size.x;
    Vec3 YAxis = dim.YAxis * dim.size.y;
    
    Vec2 Ps[4];
    
    Ps[0] = ProjectOnScreen(group, dim.P, &cameraZ);
    Ps[1] = ProjectOnScreen(group, dim.P + XAxis, &cameraZ);
    Ps[2] = ProjectOnScreen(group, dim.P + XAxis + YAxis, &cameraZ);
    Ps[3] = ProjectOnScreen(group, dim.P + YAxis, &cameraZ);
    
    r32 minX = R32_MAX;
    r32 minY = R32_MAX;
    r32 maxX = R32_MIN;
    r32 maxY = R32_MIN;
    
    for(u32 pIndex = 0; pIndex < ArrayCount(Ps); ++pIndex)
    {
        Vec2 P = Ps[pIndex];
        minX = Min(minX, P.x);
        minY = Min(minY, P.y);
        maxX = Max(maxX, P.x);
        maxY = Max(maxY, P.y);
    }
    
    Rect2 result = RectMinMax(V2(minX, minY), V2(maxX, maxY));
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

inline Vec3 UnprojectAtZ(RenderGroup* group, CameraTransform* camera, Vec2 screenP, r32 Z)
{
    Vec4 probeZ = V4(0, 0, Z, 1.0f);
    probeZ = camera->proj.forward * probeZ;
    r32 clipZ =probeZ.z;
    r32 clipW = probeZ.w;
    
    Vec2 clipSpaceXY = (screenP);
    clipSpaceXY.x *= (2.0f / group->screenDim.x);
    clipSpaceXY.y *= (2.0f / group->screenDim.y);
    
    Vec4 clip = V4(clipSpaceXY * clipW, clipZ, clipW);
    Vec3 world = (camera->proj.backward * clip).xyz;
    Vec3 result = world;
    
    return result;
}


inline Vec3 Unproject(RenderGroup* group, CameraTransform* camera, Vec2 screenP, r32 worldDistanceFromCameraZ)
{
    Vec4 probeZ = V4(camera->P -worldDistanceFromCameraZ * camera->Z, 1.0f);
    Vec3 result = UnprojectAtZ(group, camera, screenP, probeZ.z);
    return result;
}

inline Rect3 GetScreenBoundsAtDistance(RenderGroup* group, r32 cameraDistanceZ)
{
    Vec3 min = Unproject(group, &group->gameCamera, V2( 0, 0 ), cameraDistanceZ);
    Vec3 max = Unproject(group, &group->gameCamera, group->screenDim, cameraDistanceZ);
    Rect3 result = RectMinMax( min, max );
    return result;
}

inline void PushClipRect_(RenderGroup* renderGroup, Rect2i rect)
{
    RenderSetup setup = renderGroup->lastSetup;
    setup.rect = rect;
    PushSetup_(renderGroup, &setup);
}

inline void PushClipRect(RenderGroup* renderGroup, Rect2 rect, r32 z = 0)
{
    Rect2i clipRect = GetClipRect_(renderGroup, V3(GetCenter(rect), z), GetDim(rect));
    PushClipRect_(renderGroup, clipRect);
}

inline void FixedOrderedRendering(RenderGroup* group)
{
    RenderSetup setup = group->lastSetup;
    setup.disableSorting = true;
    setup.disableDepthTesting = true;
    PushSetup_(group, &setup);
}

inline void PushGameRenderSettings(RenderGroup* renderGroup, Vec3 ambientLightColor, r32 totalTimeElapsed, Vec3 windDirection, r32 windStrength)
{
    RenderSetup setup = renderGroup->lastSetup;
    setup.ambientLightColor = ambientLightColor;
    setup.totalTimeElapsed = totalTimeElapsed;
    setup.windDirection = windDirection;
    setup.windStrength = windStrength;
    PushSetup_(renderGroup, &setup);
}

inline void SetCameraBasics(RenderGroup* renderGroup, u32 flags, r32 focalLength, Vec3 cameraX = V3(1, 0, 0), Vec3 cameraY = V3(0, 1, 0), Vec3 cameraZ = V3(0, 0, 1), Vec3 cameraP = V3(0, 0, 0), Vec2 screenCameraOffset = V2(0, 0), u32 renderTargetIndex = 0)
{
    b32 orthographic = flags & Camera_Orthographic;
    b32 isDebug = flags & Camera_Debug;
    
    CameraTransform* transform = isDebug ? &renderGroup->debugCamera : &renderGroup->gameCamera;
    
    transform->X = cameraX;
    transform->Y = cameraY;
    transform->Z = cameraZ;
    transform->P = cameraP;
    transform->screenCameraOffset = screenCameraOffset;
    
    u32 width = renderTargetIndex > 0 ? MAX_TEXTURE_DIM : renderGroup->commands->settings.width;
    u32 height = renderTargetIndex > 0 ? MAX_TEXTURE_DIM : renderGroup->commands->settings.height;
    
    r32 aspectRatio = SafeRatio1((r32) width, (r32) height);
    m4x4_inv proj;
    if(orthographic)
    {
        proj = OrthographicProjection(aspectRatio);
    }
    else
    {
        proj = PerspectiveProjection(aspectRatio, focalLength);
    }
    
    m4x4_inv cameraTransform = CameraTransformMatrix(cameraX, cameraY, cameraZ, cameraP);
    
    transform->proj.forward = proj.forward * cameraTransform.forward;
    transform->proj.backward = cameraTransform.backward * proj.backward;
    
    if(!orthographic)
    {
        Vec4 test = transform->proj.forward * V4(1, 1, 0.0f, 1.0f);
        Vec4 ndc = test * (1.0f / test.w);
        Vec4 clip = ndc * test.w;
        Vec3 world = (transform->proj.backward * clip).xyz;
    }
    
    
	RenderSetup setup = renderGroup->lastSetup;
    setup.renderTargetIndex = renderTargetIndex;
    setup.proj = transform->proj.forward;
    setup.rect = { 0, 0, (i32) width, (i32) height};
    setup.ambientLightColor = V3(1, 1, 1);
    PushSetup_(renderGroup, &setup);
}


inline void SetOrthographicTransform(RenderGroup* group, r32 width, r32 height, u32 textureIndex = 0)
{
    SetCameraBasics(group, Camera_Orthographic, 0.0f, V3(2.0f / width, 0.0f, 0.0f), V3(0.0f, 2.0f / width, 0.0f), V3( 0, 0, 1), V3(0, 0, 0), V2(0, 0), textureIndex);
}

inline void SetOrthographicTransformScreenDim(RenderGroup* group)
{
    SetOrthographicTransform(group, (r32) group->commands->settings.width, (r32) group->commands->settings.height, 0);
}
