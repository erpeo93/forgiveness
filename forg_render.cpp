#define PushRenderElement(renderGroup, type) PushRenderElement_(renderGroup, sizeof(type), CommandType_##type)
inline void* PushRenderElement_(RenderGroup* renderGroup, u32 size, u32 type)
{
    void* result = 0;
    GameRenderCommands* commands = renderGroup->commands;
    size += sizeof(CommandHeader);
    
    if(commands->usedSize + size <= commands->maxBufferSize)
    {
        CommandHeader* header = (CommandHeader*) (commands->pushMemory + commands->usedSize);
        header->type = (u16) type;
        result = (header + 1);
        
        commands->usedSize += size;
        ++commands->bufferElementCount;
    }
    else
    {
        InvalidCodePath;
    }
    
    if(type != CommandType_TexturedQuadsCommand)
    {
        renderGroup->currentQuads = 0;
    }
    return result;
}

inline void Clear(RenderGroup* renderGroup, Vec4 color)
{
    renderGroup->commands->clearColor = color;
}

inline void BeginDepthPeel(RenderGroup* group)
{
    PushRenderElement_(group, 0, CommandType_BeginPeels);
}

inline void EndDepthPeel(RenderGroup* group)
{
    PushRenderElement_(group, 0, CommandType_EndPeels);
}

global_variable u32 maxIndexesPerBatch = (U16_MAX - 1);
inline TexturedQuadsCommand* GetCurrentQuads_(RenderGroup* group, u32 triangleCount)
{
    Assert((triangleCount * 3) <= maxIndexesPerBatch);
    TexturedQuadsCommand* result = 0;
    GameRenderCommands* commands = group->commands;
    
    if(group->currentQuads)
    {
        u32 currentIndexCount = (group->currentQuads->triangleCount + triangleCount) * 3;
        if(currentIndexCount > maxIndexesPerBatch)
        {
            group->currentQuads = 0;
        }
    }
    
    if(!group->currentQuads)
    {
        group->currentQuads = (TexturedQuadsCommand*) PushRenderElement_(group, sizeof(TexturedQuadsCommand), CommandType_TexturedQuadsCommand);
        
        group->currentQuads->triangleCount = 0;
        group->currentQuads->indexArrayOffset = commands->indexCount;
        group->currentQuads->vertexArrayOffset = commands->vertexCount;
        group->currentQuads->setup = group->lastSetup;
    }
    
    result = group->currentQuads;
    
    return result;
}

struct ReservedVertexes
{
    TexturedQuadsCommand* entry;
	u32 vertIndex;
	u32 indexIndex;
};

inline ReservedVertexes ReserveVertexes_(RenderGroup* group, u32 triangleCount, u32 vertexCount)
{
    ReservedVertexes result = {};
    GameRenderCommands* commands = group->commands;
    u32 indexCount = triangleCount * 3;
    
    
    if((commands->vertexCount + vertexCount) > commands->maxVertexCount ||
       (commands->indexCount + indexCount) > commands->maxIndexCount)
    {
        InvalidCodePath;
    }
    else
    {
        TexturedQuadsCommand* entry = GetCurrentQuads_(group, triangleCount);
        if(entry)
        {
            entry->triangleCount += triangleCount;
            result.entry = entry;
            result.vertIndex = commands->vertexCount;
            result.indexIndex = commands->indexCount;
            
            commands->vertexCount += vertexCount;
            commands->indexCount += indexCount;
        }
    }
    
    return result;
}

inline ReservedVertexes ReserveQuads(RenderGroup* group, u32 quadCount)
{
    u32 triangleCount = quadCount * 2;
    u32 vertexCount = quadCount * 4;
    
    ReservedVertexes result = ReserveVertexes_(group, triangleCount, vertexCount);
    
    return result;
}

inline void PushVertex(TexturedVertex* vert, Vec4 P, Vec3 N, Vec2 UV, u32 C, Lights lights, r32 modulationPercentage, u16 textureIndex, r32 lightInfluence, r32 lightYInfluence, r32 windInfluence)
{
    vert->P = P;
    vert->N = N;
    vert->UV = UV;
    vert->color = C;
    vert->lightStartingIndex = lights.startingIndex;
    vert->lightEndingIndex = lights.endingIndex;
    vert->modulationPercentage = (u16) (modulationPercentage * (r32) 0xffff);
    vert->lightInfluence = (u16) ((1.0f - lightInfluence) * (r32) 0xffff);
    vert->lightYInfluence = (u16) ((1.0f - lightYInfluence) * (r32) 0xffff);
    vert->windInfluence = (u16) (windInfluence * (r32) 0xffff);
    vert->textureIndex = textureIndex;
}

inline void PushMagicQuad(RenderGroup* group, Vec4 P, Vec4 lateral, Vec4 up, u32 color, RenderTexture texture, Lights lights, r32 modulationPercentage, r32 lightInfluence, r32 lightYInfluence, Vec4 windInfluences)
{
    ReservedVertexes vertexes = ReserveQuads(group, 1);
    if(vertexes.entry)
    {
        GameRenderCommands* commands = group->commands;
        TexturedVertex* vert = commands->vertexArray + vertexes.vertIndex;
        u16* indeces = commands->indexArray + vertexes.indexIndex;
        u16 VI = SafeTruncateToU16(vertexes.vertIndex - vertexes.entry->vertexArrayOffset);
        
        Vec3 N = V3(0, 0, 1);
        u16 textureIndex = (u16) texture.index;
        Vec2 invUV = V2((r32) texture.width / MAX_IMAGE_DIM, (r32) texture.height / MAX_IMAGE_DIM);
        
        Vec2 UV0 = Hadamart(V2(0, 0), invUV);
        Vec2 UV1 = Hadamart(V2(1, 0), invUV);
        Vec2 UV2 = Hadamart(V2(1, 1), invUV);
        Vec2 UV3 = Hadamart(V2(0, 1), invUV);
        
        Vec4 P0 = P - lateral - up;
        Vec4 P1 = P + lateral - up;
        Vec4 P2 = P + lateral + up;
        Vec4 P3 = P - lateral + up;
        
        u32 C = color;
        
        PushVertex(vert + 0, P0, N, UV0, C, lights, modulationPercentage, textureIndex, lightInfluence, lightYInfluence, windInfluences.x);
        PushVertex(vert + 1, P1, N, UV1, C, lights, modulationPercentage, textureIndex, lightInfluence, lightYInfluence, windInfluences.y);
        PushVertex(vert + 2, P2, N, UV2, C, lights, modulationPercentage, textureIndex, lightInfluence, lightYInfluence, windInfluences.z);
        PushVertex(vert + 3, P3, N, UV3, C, lights, modulationPercentage, textureIndex, lightInfluence, lightYInfluence, windInfluences.w);
        
        indeces[0] = VI + 0;
        indeces[1] = VI + 1;
        indeces[2] = VI + 3;
        indeces[3] = VI + 1;
        indeces[4] = VI + 2;
        indeces[5] = VI + 3;
    }
}

inline void PushQuad(RenderGroup* group, RenderTexture texture, Lights lights,
                     ReservedVertexes* reservedVertexes, 
                     Vec4 P0, Vec2 UV0, u32 C0,
                     Vec4 P1, Vec2 UV1, u32 C1,
                     Vec4 P2, Vec2 UV2, u32 C2,
                     Vec4 P3, Vec2 UV3, u32 C3, r32 modulationPercentage, r32 lightInfluence, r32 lightYInfluence, Vec4 windInfluences)
{
    if(reservedVertexes->entry)
    {
        GameRenderCommands* commands = group->commands;
        TexturedVertex* vert = commands->vertexArray + reservedVertexes->vertIndex;
        u16* indeces = commands->indexArray + reservedVertexes->indexIndex;
        u16 VI = SafeTruncateToU16(reservedVertexes->vertIndex - reservedVertexes->entry->vertexArrayOffset);
        reservedVertexes->vertIndex += 4;
        reservedVertexes->indexIndex += 6;
        
        Vec3 N = Normalize(Cross(P1.xyz - P0.xyz, P2.xyz - P0.xyz));
        u16 textureIndex = (u16) texture.index;
        Vec2 invUV = V2((r32) texture.width / MAX_IMAGE_DIM, (r32) texture.height / MAX_IMAGE_DIM);
        
        
        UV0 = Hadamart(UV0, invUV);
        UV1 = Hadamart(UV1, invUV);
        UV2 = Hadamart(UV2, invUV);
        UV3 = Hadamart(UV3, invUV);
        
        
        PushVertex(vert + 0, P0, N, UV0, C0, lights, modulationPercentage, textureIndex, lightInfluence, lightYInfluence, windInfluences.x);
        PushVertex(vert + 1, P1, N, UV1, C1, lights, modulationPercentage, textureIndex, lightInfluence, lightYInfluence, windInfluences.y);
        PushVertex(vert + 2, P2, N, UV2, C2, lights, modulationPercentage, textureIndex, lightInfluence, lightYInfluence, windInfluences.z);
        PushVertex(vert + 3, P3, N, UV3, C3, lights, modulationPercentage, textureIndex, lightInfluence, lightYInfluence, windInfluences.w);
        
        indeces[0] = VI + 0;
        indeces[1] = VI + 1;
        indeces[2] = VI + 3;
        indeces[3] = VI + 1;
        indeces[4] = VI + 2;
        indeces[5] = VI + 3;
    }
}

inline void PushQuad(RenderGroup* group, RenderTexture texture, Lights lights,
                     ReservedVertexes* reservedVertexes,
                     Vec4 P0, Vec2 UV0, Vec4 C0,
                     Vec4 P1, Vec2 UV1, Vec4 C1,
                     Vec4 P2, Vec2 UV2, Vec4 C2,
                     Vec4 P3, Vec2 UV3, Vec4 C3, r32 modulationPercentage, r32 lightInfluence, r32 lightYInfluence, Vec4 windInfluences)
{
    PushQuad(group, texture, lights,
             reservedVertexes,
             P0, UV0, RGBAPack8x4(C0 * 255.0f),
             P1, UV1, RGBAPack8x4(C1 * 255.0f),
             P2, UV2, RGBAPack8x4(C2 * 255.0f),
             P3, UV3, RGBAPack8x4(C3 * 255.0f), modulationPercentage, lightInfluence, lightYInfluence, windInfluences);
}

inline void PushLineSegment(RenderGroup* group, RenderTexture texture, Vec4 color, Vec3 fromP, Vec3 toP, r32 tickness, Lights lights = {0, 0})
{
    Vec3 line = toP - fromP;
    Vec3 perp = Cross(group->gameCamera.Z, line);
    Vec3 normPerp = tickness * Normalize(perp);
    
    Vec4 P0 = V4(fromP - normPerp, 0);
    Vec4 P1 = V4(toP - normPerp, 0);
    Vec4 P2 = V4(toP + normPerp, 0);
    Vec4 P3 = V4(fromP + normPerp, 0);
    
    u32 c = StoreColor(color);
    
    ReservedVertexes vertexes = ReserveQuads(group, 1);
    PushQuad(group, texture, lights, &vertexes, P0, V2(0, 0), c, P1, V2(1, 0), c, P2, V2(1, 1), c, P3, V2(0, 1), c, 0, 0, 0, {});
}

inline void PushLine(RenderGroup* group, Vec4 color, Vec3 fromP, Vec3 toP, r32 tickness, Lights lights = {0, 0})
{
    PushLineSegment(group, group->whiteTexture, color, fromP, toP, tickness, lights);
}

inline void PushRect4Colors(RenderGroup* renderGroup, ObjectTransform objectTransform, Vec3 P, Vec2 dim, Vec4 c1, Vec4 c2, Vec4 c3, Vec4 c4, Lights lights)
{
    GameRenderCommands* commands = renderGroup->commands;
    Vec3 XAxis = V3(1, 0, 0);
    Vec3 YAxis = V3(0, 1, 0);
    
    if(objectTransform.upright)
    {
        P += objectTransform.cameraOffset.x * renderGroup->gameCamera.X + objectTransform.cameraOffset.y * renderGroup->gameCamera.Y + objectTransform.cameraOffset.z * renderGroup->gameCamera.Z;  
        
        Vec3 XAxis1 = XAxis.x * renderGroup->gameCamera.X + XAxis.y * renderGroup->gameCamera.Y;
        Vec3 YAxis1 = YAxis.x * renderGroup->gameCamera.X + YAxis.y * renderGroup->gameCamera.Y;
        
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
    
    ReservedVertexes vertexes = ReserveQuads(renderGroup, 1);
    PushQuad(renderGroup, renderGroup->whiteTexture, lights, &vertexes,
             V4(min.x, min.y, min.z, objectTransform.additionalZBias), V2(minUV.x, minUV.y), c1,
             V4(max.x, min.y, min.z, objectTransform.additionalZBias), V2(maxUV.x, minUV.y), c2,
             V4(max.x, max.y, max.z, objectTransform.additionalZBias), V2(maxUV.x, maxUV.y), c3,
             V4(min.x, max.y, max.z, objectTransform.additionalZBias), V2(minUV.x, maxUV.y), c4, objectTransform.modulationPercentage, objectTransform.lightInfluence, objectTransform.lightYInfluence, objectTransform.windInfluences);
}

inline void PushRect4Colors(RenderGroup* renderGroup, ObjectTransform objectTransform, Rect2 rect, Vec4 c0, Vec4 c1, Vec4 c2, Vec4 c3, Lights lights = {0, 0})
{
    PushRect4Colors(renderGroup, objectTransform, V3(GetCenter(rect), 0.0f), GetDim(rect), c0, c1, c2, c3, lights);
}

inline void PushRect(RenderGroup* renderGroup, ObjectTransform objectTransform, Vec3 P, Vec2 dim, Vec4 color, Lights lights = {0, 0})
{
    PushRect4Colors(renderGroup, objectTransform, P, dim, color, color, color, color, lights);
}

inline void PushRect(RenderGroup* renderGroup, ObjectTransform objectTransform, Rect2 rect, Vec4 color, Lights lights = {0, 0})
{
    PushRect(renderGroup, objectTransform, V3(GetCenter(rect), 0.0f), GetDim(rect), color, lights);
}


inline void PushRectOutline(RenderGroup* renderGroup, ObjectTransform objectTransform, Vec3 P, 
                            Vec2 dim, Vec4 color, r32 thickness)
{
    Vec2 halfDim = 0.5f * dim;
    PushRect(renderGroup, objectTransform, P + V3(0.0f, halfDim.y, 0.0f), V2(dim.x, thickness), color);
    PushRect(renderGroup, objectTransform, P +V3(0.0f, -halfDim.y, 0.0f), V2(dim.x, thickness), color);
    PushRect(renderGroup, objectTransform, P + V3(halfDim.x, 0.0f, 0.0f), V2(thickness, dim.y), color);
    PushRect(renderGroup, objectTransform, P + V3(-halfDim.x, 0.0f, 0.0f), V2(thickness, dim.y), color);
}

inline void PushRectOutline(RenderGroup* renderGroup, ObjectTransform objectTransform, Rect2 rect, Vec4 color, r32 thickness, r32 z = 0.0f)
{
    PushRectOutline(renderGroup, objectTransform, V3(GetCenter(rect), z), GetDim(rect), color, thickness);
}


inline void PushCubeOutline(RenderGroup* group, Rect3 R, Vec4 color, r32 tickness)
{
    RenderTexture texture = group->whiteTexture;
    
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
    
    
    PushLineSegment(group, texture, color, P0, P1, tickness);
    PushLineSegment(group, texture, color, P0, P3, tickness);
    PushLineSegment(group, texture, color, P0, P4, tickness);
    
    PushLineSegment(group, texture, color, P2, P3, tickness);
    PushLineSegment(group, texture, color, P2, P6, tickness);
    PushLineSegment(group, texture, color, P2, P1, tickness);
    
    PushLineSegment(group, texture, color, P5, P6, tickness);
    PushLineSegment(group, texture, color, P5, P4, tickness);
    PushLineSegment(group, texture, color, P5, P1, tickness);
    
    PushLineSegment(group, texture, color, P7, P3, tickness);
    PushLineSegment(group, texture, color, P7, P4, tickness);
    PushLineSegment(group, texture, color, P7, P6, tickness);
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

inline void PushTexture(RenderGroup* group, RenderTexture texture, Vec3 P, Vec3 XAxis = V3(1, 0, 0), Vec3 YAxis = V3(0, 1, 0), Vec4 color = V4(1, 1, 1, 1), Lights lights = {}, r32 modulationPercentage = 0.0f, r32 lightInfluence = 0, r32 lightYInfluence = 0, r32 ZBias = 0.0f, Vec4 windInfluences = {})
{
    r32 oneTexelU = 1.0f / texture.width;
    r32 oneTexelV = 1.0f / texture.height;
    Vec2 minUV = V2(oneTexelU, oneTexelV);
    Vec2 maxUV = V2(1.0f - oneTexelU, 1.0f - oneTexelV);
    
    u32 colorInt = RGBAPack8x4(color * 255.0f);
    
    Vec4 minXminY = V4(P, 0.0f + ZBias);
    Vec4 maxXminY = V4(P + XAxis, 0.0f + ZBias);
    Vec4 minXmaxY = V4(P + YAxis, ZBias);
    Vec4 maxXmaxY = V4(P + XAxis + YAxis, ZBias);
    
    
    ReservedVertexes vertexes = ReserveQuads(group, 1);
    if((XAxis.x >= 0 && YAxis.y >= 0) ||
       (XAxis.x < 0 && YAxis.y < 0))
    {
        PushQuad(group, texture, lights, &vertexes,
                 minXminY, V2(minUV.x, minUV.y), colorInt,
                 maxXminY, V2(maxUV.x, minUV.y), colorInt,
                 maxXmaxY, V2(maxUV.x, maxUV.y), colorInt,
                 minXmaxY, V2(minUV.x, maxUV.y), colorInt, modulationPercentage, lightInfluence, lightYInfluence, windInfluences);
    }
    else
    {
        PushQuad(group, texture, lights, &vertexes,
                 minXminY, V2(minUV.x, minUV.y), colorInt,
                 minXmaxY, V2(minUV.x, maxUV.y), colorInt,
                 maxXmaxY, V2(maxUV.x, maxUV.y), colorInt,
                 maxXminY, V2(maxUV.x, minUV.y), colorInt, modulationPercentage, lightInfluence, lightYInfluence, windInfluences);
    }
}

inline BitmapDim PushBitmap_(RenderGroup* renderGroup, ObjectTransform transform, ColoredBitmap coloredBitmap,  Vec3 P, r32 height, Vec4 color, Lights lights, Vec2 pivot)
{
    BitmapDim result = {};
    GameRenderCommands* commands = renderGroup->commands;
    
    Bitmap* bitmap = coloredBitmap.bitmap;
    color = Hadamart(color, coloredBitmap.coloration);
    
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
            P += transform.cameraOffset.x * renderGroup->gameCamera.X + transform.cameraOffset.y * renderGroup->gameCamera.Y + transform.cameraOffset.z * renderGroup->gameCamera.Z;  
            
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
        
        
        if(height == 0)
        {
            height = bitmap->nativeHeight;
        }
        
        BitmapDim dim = GetBitmapDim(bitmap, pivot, P, XAxis, YAxis, height, transform.scale);
        result = dim;
        
        P = dim.P;
        XAxis= XAxis * dim.size.x;
        YAxis = YAxis * dim.size.y;
        
        if(!transform.dontRender)
        {
            PushTexture(renderGroup, bitmap->textureHandle, P, XAxis, YAxis, color, lights, transform.modulationPercentage, transform.additionalZBias);
        }
    }
    
    return result;
}

inline BitmapDim GetBitmapDim(RenderGroup* renderGroup, ObjectTransform transform, BitmapId ID, Vec3 P, r32 height = 0)
{
    
    BitmapDim result = {};
    ColoredBitmap bitmap = GetBitmap(renderGroup->assets, ID);
    if(bitmap.bitmap)
    {
        transform.dontRender = true;
        result = PushBitmap_(renderGroup, transform, bitmap, P, height, V4(1, 1, 1, 1), {}, bitmap.pivot);
    }
    else
    {
        LoadBitmap(renderGroup->assets, ID);
    }
    
    return result;
}

inline BitmapDim PushBitmap(RenderGroup* renderGroup, ObjectTransform transform, BitmapId ID, Vec3 P, r32 height = 0, Vec4 color = V4(1.0f,1.0f, 1.0f, 1.0f), Lights lights = {0, 0})
{
    BitmapDim result = {};
    ColoredBitmap bitmap = GetBitmap(renderGroup->assets, ID);
    if(bitmap.bitmap)
    {
        result = PushBitmap_(renderGroup, transform, bitmap, P, height, color, lights, bitmap.pivot);
    }
    else
    {
        LoadBitmap(renderGroup->assets, ID);
    }
    
    return result;
}

inline BitmapDim PushBitmapWithPivot(RenderGroup* renderGroup, ObjectTransform transform, BitmapId ID, Vec3 P, Vec2 pivot, r32 height = 0,  Vec4 color = V4(1.0f,1.0f, 1.0f, 1.0f), Lights lights = {0, 0})
{
    BitmapDim result = {};
    ColoredBitmap bitmap = GetBitmap(renderGroup->assets, ID);
    if(bitmap.bitmap)
    {
        result = PushBitmap_(renderGroup, transform, bitmap, P, height, color, lights, pivot);
    }
    else
    {
        LoadBitmap(renderGroup->assets, ID);
    }
    
    return result;
}

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
    ReservedVertexes vertexes = ReserveQuads(group, 6);
    PushQuad(group, texture, lights, &vertexes, P0, T0, TC, P1, T1, TC, P2, T2, TC, P3, T3, TC, 0, 0, 0, wind);
    PushQuad(group, texture, lights, &vertexes, P7, T0, BC, P6, T1, BC, P5, T2, BC, P4, T3, BC, 0, 0, 0, wind);
    
    PushQuad(group, texture, lights, &vertexes, P4, T0, BGr, P5, T1, BGr, P1, T2, TGr, P0, T3, TGr, 0, 0, 0, wind);
    
    PushQuad(group, texture, lights, &vertexes, P2, T0, TGr, P6, T1, BGr, P7, T2, BGr, P3, T3, TGr, 0, 0, 0, wind);
    
    PushQuad(group, texture, lights, &vertexes, P1, T0, TGr, P5, T1, BGr, P6, T2, BGr, P2, T3, TGr, 0, 0, 0, wind);
    
    PushQuad(group, texture, lights, &vertexes, P7, T0, BGr, P4, T1, BGr, P0, T2, TGr, P3, T3, TGr, 0, 0, 0, wind);
    
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

inline Vec3 GetCameraOffset(RenderGroup* group, Vec3 P)
{
    Vec3 result = P.x * group->gameCamera.X + P.y * group->gameCamera.Y + P.z * group->gameCamera.Z;
    return result;
}

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

inline void PushSetup(RenderGroup* group, RenderSetup* setup)
{
    group->lastSetup = *setup;
    group->currentQuads = 0;
}

inline void PushClipRect(RenderGroup* renderGroup, Rect2i rect)
{
    RenderSetup setup = renderGroup->lastSetup;
    setup.rect = rect;
    PushSetup(renderGroup, &setup);
}

inline void PushClipRect(RenderGroup* renderGroup, Rect2 rect, r32 z = 0)
{
    Rect2i clipRect = GetClipRect_(renderGroup, V3(GetCenter(rect), z), GetDim(rect));
    PushClipRect(renderGroup, clipRect);
}

inline void PushGameRenderSettings(RenderGroup* renderGroup, Vec3 ambientLightColor, r32 totalTimeElapsed, Vec3 windDirection, r32 windStrength)
{
    RenderSetup setup = renderGroup->lastSetup;
    setup.ambientLightColor = ambientLightColor;
    setup.totalTimeElapsed = totalTimeElapsed;
    setup.windDirection = windDirection;
    setup.windStrength = windStrength;
    PushSetup(renderGroup, &setup);
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

internal Rect2 PushText_(RenderGroup* group, FontId fontID, Font* font, PAKFont* info, char* string, Vec3 P, r32 fontScale, TextOperation op, Vec4 color, b32 startingSpace, b32 drawShadow, r32 ZBias)
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
                        if(drawShadow)
                        {
                            PushBitmap(group, FlatTransform(ZBias), ID, P + V3( 2.0f, -2.0f, -0.001f ), glyphHeight, V4( 0.0f, 0.0f, 0.0f, 1.0f ));
                        }
                        
                        PushBitmap(group, FlatTransform(ZBias), ID, P, glyphHeight, color);
                    }
                }
            }
            
            prevCodePoint = codePoint;
        }
        
    }
    
    return result;
}


internal void PushText(RenderGroup* group, FontId fontID, char* string, Vec3 P,r32 fontScale = 1.0f, Vec4 color = V4(1, 1, 1, 1), b32 startingSpace = false, b32 drawShadow = true, r32 ZBias = 0.0f)
{
    Font* font = GetFont(group->assets, fontID);
    if(font)
    {
        PAKFont* info = GetFontInfo(group->assets, fontID);
        PushText_(group, fontID, font, info, string, P, fontScale, TextOp_draw, color, startingSpace, drawShadow, ZBias);
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
        
        result = PushText_(group, fontID, font, info, string, P, fontScale, TextOp_getSize, V4(1, 1, 1, 1), startingSpace, false, 0.0f);
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

inline void SetCameraTransform(RenderGroup* renderGroup, u32 flags, r32 focalLength, Vec3 cameraX = V3(1, 0, 0), Vec3 cameraY = V3(0, 1, 0), Vec3 cameraZ = V3(0, 0, 1), Vec3 cameraP = V3(0, 0, 0), Vec2 screenCameraOffset = V2(0, 0), u32 renderTargetIndex = 0)
{
    b32 orthographic = flags & Camera_Orthographic;
    b32 isDebug = flags & Camera_Debug;
    
    CameraTransform* transform = isDebug ? &renderGroup->debugCamera : &renderGroup->gameCamera;
    
    transform->X = cameraX;
    transform->Y = cameraY;
    transform->Z = cameraZ;
    transform->P = cameraP;
    transform->screenCameraOffset = screenCameraOffset;
    
    u32 width = renderTargetIndex > 0 ? MAX_IMAGE_DIM : renderGroup->commands->settings.width;
    u32 height = renderTargetIndex > 0 ? MAX_IMAGE_DIM : renderGroup->commands->settings.height;
    
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
    
    
    RenderSetup setup;
    setup.renderTargetIndex = renderTargetIndex;
    setup.proj = transform->proj.forward;
    
    setup.rect = { 0, 0, (i32) width, (i32) height };
    setup.ambientLightColor = V3(1, 1, 1);
    PushSetup(renderGroup, &setup);
}

inline void SetOrthographicTransform(RenderGroup* group, r32 width, r32 height, u32 textureIndex = 0)
{
    SetCameraTransform(group, Camera_Orthographic, 0.0f, V3(2.0f / width, 0.0f, 0.0f), V3(0.0f, 2.0f / width, 0.0f), V3( 0, 0, 1), V3(0, 0, 0), V2(0, 0), textureIndex);
}

inline void SetOrthographicTransformScreenDim(RenderGroup* group)
{
    SetOrthographicTransform(group, (r32) group->commands->settings.width, (r32) group->commands->settings.height, 0);
}

inline RenderGroup BeginRenderGroup(Assets* assets, GameRenderCommands* commands)
{
    RenderGroup result = {};
    result.assets = assets;
    result.commands = commands;
    result.screenDim = V2i(commands->settings.width, commands->settings.height);
    
    m4x4_inv I = { Identity(), Identity() };
    
    RenderSetup setup;
    setup.rect = { 0, 0, (i32)commands->settings.width, (i32)commands->settings.height };
    setup.proj = I.forward;
    setup.renderTargetIndex = 0;
    setup.ambientLightColor = V3(1, 1, 1);
    
    PushSetup(&result, &setup);
    
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
    
    Vec4 clip = V4( clipSpaceXY * clipW, clipZ, clipW);
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