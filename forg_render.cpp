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

inline void InitCurrentTriangles(RenderGroup* group, GameRenderCommands* commands)
{
    group->currentQuads->triangleCount = 0;
    group->currentQuads->indexArrayOffset = commands->indexCount;
    group->currentQuads->vertexArrayOffset = commands->vertexCount;
    group->currentQuads->setup = group->lastSetup;
}

inline TexturedQuadsCommand* GetCurrentTriangles(RenderGroup* renderGroup, u32 triangleCount, u32 vertexCount, u32 indexCount)
{
    if(renderGroup->currentQuads)
    {
        u32 maxIndexesPerBatch = (U16_MAX - 1) / 3;
        u32 currentIndexCount = (renderGroup->currentQuads->triangleCount + triangleCount) * 3;
        if(currentIndexCount > maxIndexesPerBatch)
        {
            renderGroup->currentQuads = 0;
        }
    }
    
    GameRenderCommands* commands = renderGroup->commands;
    if(!renderGroup->currentQuads)
    {
        renderGroup->currentQuads = (TexturedQuadsCommand*) PushRenderElement_(renderGroup, sizeof(TexturedQuadsCommand), CommandType_TexturedQuadsCommand);
        InitCurrentTriangles(renderGroup, commands);
    }
    
    TexturedQuadsCommand* result = renderGroup->currentQuads;
    if((commands->vertexCount + vertexCount) > commands->maxVertexCount ||
       (commands->indexCount + indexCount) > commands->maxIndexCount)
    {
        InvalidCodePath;
        result = 0;
    }
    
    return result;
}

inline TexturedQuadsCommand* GetCurrentQuads(RenderGroup* renderGroup, u32 quadCount)
{
    u32 triangleCount = quadCount * 2;
    u32 vertexCount = quadCount * 4;
    u32 indexCount = quadCount * 6;
    
    TexturedQuadsCommand* result = GetCurrentTriangles(renderGroup, triangleCount, vertexCount, indexCount);
    
    return result;
}

inline void PushQuad(RenderGroup* group, RenderTexture texture, Vec4 lightIndexes,
                     Vec4 P0, Vec2 UV0, u32 C0,
                     Vec4 P1, Vec2 UV1, u32 C1,
                     Vec4 P2, Vec2 UV2, u32 C2,
                     Vec4 P3, Vec2 UV3, u32 C3, r32 modulationPercentage)
{
    GameRenderCommands* commands = group->commands;
    
    TexturedQuadsCommand* entry = group->currentQuads;
    Assert(entry);
    entry->triangleCount += 2;
    
    u32 vertIndex = commands->vertexCount;
    TexturedVertex* vert = commands->vertexArray + vertIndex;
    u16* indeces = commands->indexArray + commands->indexCount;
    
    commands->vertexCount += 4;
    commands->indexCount += 6;
    Assert(commands->vertexCount <= commands->maxVertexCount);
    Assert(commands->indexCount <= commands->maxIndexCount);
    
    Vec3 N = Normalize(Cross(P1.xyz - P0.xyz, P2.xyz - P0.xyz));
    u16 textureIndex = (u16) texture.index;
    
    Vec2 invUV = V2((r32) texture.width / TEXTURE_ARRAY_DIM, (r32) texture.height / TEXTURE_ARRAY_DIM);
    UV0 = Hadamart(UV0, invUV);
    UV1 = Hadamart(UV1, invUV);
    UV2 = Hadamart(UV2, invUV);
    UV3 = Hadamart(UV3, invUV);
    
    
    vert[0].P = P0;
    vert[0].N = N;
    vert[0].UV = UV0;
    vert[0].color = C0;
    vert[0].lightIndexes = lightIndexes;
    vert[0].modulationPercentage = modulationPercentage;
    vert[0].textureIndex = textureIndex;
    
    vert[1].P = P1;
    vert[1].N = N;
    vert[1].UV = UV1;
    vert[1].color = C1;
    vert[1].lightIndexes = lightIndexes;
    vert[1].modulationPercentage = modulationPercentage;
    vert[1].textureIndex = textureIndex;
    
    vert[2].P = P2;
    vert[2].N = N;
    vert[2].UV = UV2;
    vert[2].color = C2;
    vert[2].lightIndexes = lightIndexes;
    vert[2].modulationPercentage = modulationPercentage;
    vert[2].textureIndex = textureIndex;
    
    vert[3].P = P3;
    vert[3].N = N;
    vert[3].UV = UV3;
    vert[3].color = C3;
    vert[3].lightIndexes = lightIndexes;
    vert[3].modulationPercentage = modulationPercentage;
    vert[3].textureIndex = textureIndex;
    
    u16 VI = SafeTruncateToU16(vertIndex - entry->vertexArrayOffset);
    indeces[0] = VI + 0;
    indeces[1] = VI + 1;
    indeces[2] = VI + 3;
    indeces[3] = VI + 1;
    indeces[4] = VI + 2;
    indeces[5] = VI + 3;
}

inline void PushQuad(RenderGroup* group, RenderTexture texture, Vec4 lightIndexes,
                     Vec4 P0, Vec2 UV0, Vec4 C0,
                     Vec4 P1, Vec2 UV1, Vec4 C1,
                     Vec4 P2, Vec2 UV2, Vec4 C2,
                     Vec4 P3, Vec2 UV3, Vec4 C3, r32 modulationPercentage)
{
    PushQuad(group, texture, lightIndexes,
             P0, UV0, RGBAPack8x4(C0 * 255.0f),
             P1, UV1, RGBAPack8x4(C1 * 255.0f),
             P2, UV2, RGBAPack8x4(C2 * 255.0f),
             P3, UV3, RGBAPack8x4(C3 * 255.0f), modulationPercentage);
}

inline void PushVertex(TexturedVertex* vert, Vec4 P, Vec3 N, Vec2 UV, u32 C, Vec4 lightIndexes, r32 modulationPercentage, u16 textureIndex)
{
    vert->P = P;
    vert->N = N;
    vert->UV = UV;
    vert->color = C;
    vert->lightIndexes = lightIndexes;
    vert->modulationPercentage = modulationPercentage;
    vert->textureIndex = textureIndex;
}


inline void PushTriangle(RenderGroup* group, RenderTexture texture, Vec4 lightIndexes,
                         Vec4 P0, u32 C0,
                         Vec4 P1, u32 C1,
                         Vec4 P2, u32 C2, r32 modulationPercentage)
{
    GameRenderCommands* commands = group->commands;
    TexturedQuadsCommand* entry = GetCurrentTriangles(group, 1, 3, 3);
    if(entry)
    {
        ++entry->triangleCount;
        
        u32 vertIndex = commands->vertexCount;
        TexturedVertex* vert = commands->vertexArray + vertIndex;
        u16* indeces = commands->indexArray + commands->indexCount;
        
        commands->vertexCount += 3;
        commands->indexCount += 3;
        Assert(commands->vertexCount <= commands->maxVertexCount);
        Assert(commands->indexCount <= commands->maxIndexCount);
        
        Vec3 N = V3(0, 0, 0);//Normalize(Cross(P1.xyz - P0.xyz, P2.xyz - P1.xyz));
        u16 textureIndex = (u16) texture.index;
        
        
        PushVertex(vert + 0, P0, N, V2(0, 0), C0, lightIndexes, modulationPercentage, textureIndex);
        PushVertex(vert + 1, P1, N, V2(0, 0), C1, lightIndexes, modulationPercentage, textureIndex);
        PushVertex(vert + 2, P2, N, V2(0, 0), C2, lightIndexes, modulationPercentage, textureIndex);
        
        u16 VI = SafeTruncateToU16(vertIndex - entry->vertexArrayOffset);
        indeces[0] = VI + 0;
        indeces[1] = VI + 1;
        indeces[2] = VI + 2;
    }
}

inline void PushTriangle(RenderGroup* group, RenderTexture texture, Vec4 lightIndexes,
                         Vec4 P0, Vec4 C0,
                         Vec4 P1, Vec4 C1,
                         Vec4 P2, Vec4 C2, r32 modulationPercentage)
{
    u32 c0 = RGBAPack8x4(C0 * 255.0f);
    u32 c1 = RGBAPack8x4(C1 * 255.0f);
    u32 c2 = RGBAPack8x4(C2 * 255.0f);
    PushTriangle(group, texture, lightIndexes, P0, c0, P1, c1, P2, c2, modulationPercentage);
}

inline void PushLineSegment(RenderGroup* group, RenderTexture texture, Vec4 color, Vec3 fromP, Vec3 toP, r32 tickness, Vec4 lightIndexes = V4(-1, -1, -1, -1))
{
    Vec3 line = toP - fromP;
    Vec3 perp = Cross(group->gameCamera.Z, line);
    Vec3 normPerp = tickness * Normalize(perp);
    
    Vec4 P0 = V4(fromP - normPerp, 0);
    Vec4 P1 = V4(toP - normPerp, 0);
    Vec4 P2 = V4(toP + normPerp, 0);
    Vec4 P3 = V4(fromP + normPerp, 0);
    
    u32 c = StoreColor(color);
    PushQuad(group, texture, lightIndexes, P0, V2(0, 0), c, P1, V2(1, 0), c, P2, V2(1, 1), c, P3, V2(0, 1), c, 0);
}

inline void PushLine(RenderGroup* group, Vec4 color, Vec3 fromP, Vec3 toP, r32 tickness, Vec4 lightIndexes = V4(-1, -1, -1, -1))
{
    TexturedQuadsCommand* entry = GetCurrentQuads(group, 1);
    if(entry)
    {
        PushLineSegment(group, group->whiteTexture, color, fromP, toP, tickness, lightIndexes);
    }
}

inline void PushRect(RenderGroup* renderGroup, ObjectTransform objectTransform, Vec3 P, Vec2 dim, Vec4 color, Vec4 lightIndexes = V4(-1, -1, -1, -1))
{
    GameRenderCommands* commands = renderGroup->commands;
    TexturedQuadsCommand* entry = GetCurrentQuads(renderGroup, 1);
    if(entry)
    {
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
        
        PushQuad(renderGroup, renderGroup->whiteTexture, lightIndexes,
                 V4(min.x, min.y, min.z, objectTransform.additionalZBias), V2(minUV.x, minUV.y), color,
                 V4(max.x, min.y, min.z, objectTransform.additionalZBias), V2(maxUV.x, minUV.y), color,
                 V4(max.x, max.y, max.z, objectTransform.additionalZBias), V2(maxUV.x, maxUV.y), color,
                 V4(min.x, max.y, max.z, objectTransform.additionalZBias), V2(minUV.x, maxUV.y), color, objectTransform.modulationPercentage);
    }
}

inline void PushRect(RenderGroup* renderGroup, ObjectTransform objectTransform, Rect2 rect, Vec4 color, Vec4 lightIndexes = V4(-1, -1, -1, -1))
{
    PushRect(renderGroup, objectTransform, V3(GetCenter(rect), 0.0f), GetDim(rect), color, lightIndexes);
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
    TexturedQuadsCommand* entry = GetCurrentQuads(group, 6);
    if(entry)
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
}

struct BitmapDim
{
    Vec2 size;
    Vec3 P;
    Vec3 XAxis;
    Vec3 YAxis;
};

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

inline BitmapDim GetBitmapDim(Bitmap* bitmap, Vec3 P, Vec3 XAxis, Vec3 YAxis, r32 height, Vec2 scale = V2(1.0f, 1.0f))
{
    BitmapDim result = GetBitmapDim(bitmap, bitmap->pivot, P, XAxis, YAxis, height, scale);
    return result;
}

inline void PushBitmap__(RenderGroup* group, Bitmap* bitmap, Vec3 P, Vec3 XAxis, Vec3 YAxis, Vec4 color, Vec4 lightIndexes, r32 modulationPercentage, r32 ZBias)
{
    r32 oneTexelU = 1.0f / bitmap->width;
    r32 oneTexelV = 1.0f / bitmap->height;
    Vec2 minUV = V2(oneTexelU, oneTexelV);
    Vec2 maxUV = V2(1.0f - oneTexelU, 1.0f - oneTexelV);
    
    u32 colorInt = RGBAPack8x4(color * 255.0f);
    
    Vec4 minXminY = V4(P, 0.0f + ZBias);
    Vec4 maxXminY = V4(P + XAxis, 0.0f + ZBias);
    Vec4 minXmaxY = V4(P + YAxis, ZBias);
    Vec4 maxXmaxY = V4(P + XAxis + YAxis, ZBias);
    
    
    if((XAxis.x >= 0 && YAxis.y >= 0) ||
       (XAxis.x < 0 && YAxis.y < 0))
    {
        PushQuad(group, bitmap->textureHandle, lightIndexes,
                 minXminY, V2(minUV.x, minUV.y), colorInt,
                 maxXminY, V2(maxUV.x, minUV.y), colorInt,
                 maxXmaxY, V2(maxUV.x, maxUV.y), colorInt,
                 minXmaxY, V2(minUV.x, maxUV.y), colorInt, modulationPercentage);
    }
    else
    {
        PushQuad(group, bitmap->textureHandle, lightIndexes,
                 minXminY, V2(minUV.x, minUV.y), colorInt,
                 minXmaxY, V2(minUV.x, maxUV.y), colorInt,
                 maxXmaxY, V2(maxUV.x, maxUV.y), colorInt,
                 maxXminY, V2(maxUV.x, minUV.y), colorInt, modulationPercentage);
    }
    
}

inline BitmapDim PushBitmap_(RenderGroup* renderGroup, ObjectTransform objectTransform, Bitmap* bitmap,  Vec3 P, r32 height, Vec2 scale, Vec4 color, Vec4 lightIndexes, Vec2 pivot)
{
    BitmapDim result = {};
    GameRenderCommands* commands = renderGroup->commands;
    if(bitmap->width && bitmap->height)
    {
        TexturedQuadsCommand* entry = GetCurrentQuads(renderGroup, 1);
        if(entry)
        {
            r32 angleRad = DegToRad(objectTransform.angle);
            Vec3 XAxis = V3(Cos(angleRad), Sin(angleRad), 0.0f);
            Vec3 YAxis  = V3(Perp(XAxis.xy), 0.0f);
            
            if(objectTransform.flipOnYAxis)
            {
                XAxis.x = -XAxis.x;
                YAxis.xy = -Perp(XAxis.xy);
                objectTransform.cameraOffset.x = -objectTransform.cameraOffset.x;
            }
            
            if(objectTransform.upright)
            {
                P += objectTransform.cameraOffset.x * renderGroup->gameCamera.X + objectTransform.cameraOffset.y * renderGroup->gameCamera.Y + objectTransform.cameraOffset.z * renderGroup->gameCamera.Z;  
                
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
                P += objectTransform.cameraOffset;
            }
            
            
            if(height == 0)
            {
                height = bitmap->nativeHeight;
            }
            
            BitmapDim dim = GetBitmapDim(bitmap, pivot, P, XAxis, YAxis, height, scale);
            result = dim;
            
            P = dim.P;
            XAxis= XAxis * dim.size.x;
            YAxis = YAxis * dim.size.y;
            
            PushBitmap__(renderGroup, bitmap, P, XAxis, YAxis, color, lightIndexes, objectTransform.modulationPercentage, objectTransform.additionalZBias);
        }
    }
    
    return result;
}


inline void PushBitmap(RenderGroup* renderGroup, ObjectTransform objectTransform, BitmapId ID, Vec3 P, r32 height = 0, Vec2 scale = V2(1.0f, 1.0f),  Vec4 color = V4(1.0f,1.0f, 1.0f, 1.0f), Vec4 lightIndexes = V4(-1, -1, -1, -1))
{
    Bitmap* bitmap = GetBitmap(renderGroup->assets, ID);
    if(bitmap)
    {
        color = Hadamart(color, ID.coloration);
        PushBitmap_(renderGroup, objectTransform, bitmap, P, height, scale, color, lightIndexes, bitmap->pivot);
    }
    else
    {
        ++renderGroup->countMissing;
        LoadBitmap(renderGroup->assets, ID, false);
    }
}


inline void PushUIBitmap(RenderGroup* group, BitmapId ID, Vec2 screenCenterOffset, r32 height, r32 angle, r32 additionalZBias, Vec2 scale = V2(1.0f, 1.0f),  Vec4 color = V4(1.0f,1.0f, 1.0f, 1.0f))
{
    ObjectTransform transform = UprightTransform();
    transform.cameraOffset = ToV3(screenCenterOffset + group->gameCamera.screenCameraOffset);
    transform.additionalZBias = additionalZBias;
    transform.angle = angle;
    
    PushBitmap(group, transform, ID, V3(0, 0, 0), height, scale, color);
}

inline BitmapDim PushBitmapWithPivot(RenderGroup* renderGroup, ObjectTransform objectTransform, BitmapId ID, Vec3 P, Vec2 pivot, r32 height = 0, Vec2 scale = V2(1.0f, 1.0f),  Vec4 color = V4(1.0f,1.0f, 1.0f, 1.0f), Vec4 lightIndexes = V4(-1, -1, -1, -1))
{
    BitmapDim result = {};
    Bitmap* bitmap = GetBitmap(renderGroup->assets, ID);
    if(bitmap)
    {
        color = Hadamart(color, ID.coloration);
        result = PushBitmap_(renderGroup, objectTransform, bitmap, P, height, scale, color, lightIndexes, pivot);
    }
    else
    {
        ++renderGroup->countMissing;
        LoadBitmap(renderGroup->assets, ID, false);
    }
    
    return result;
}

inline void PushCube_(RenderGroup* group, Vec3 P, r32 height, r32 width, RenderTexture texture, Vec4 color, Vec4 lightIndexes)
{
    TexturedQuadsCommand* entry = GetCurrentQuads(group, 6);
    if(entry)
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
        
        PushQuad(group, texture, lightIndexes, P0, T0, TC, P1, T1, TC, P2, T2, TC, P3, T3, TC, 0);
        PushQuad(group, texture, lightIndexes, P7, T0, BC, P6, T1, BC, P5, T2, BC, P4, T3, BC, 0);
        
        PushQuad(group, texture, lightIndexes, P4, T0, BGr, P5, T1, BGr, P1, T2, TGr, P0, T3, TGr, 0);
        PushQuad(group, texture, lightIndexes, P2, T0, TGr, P6, T1, BGr, P7, T2, BGr, P3, T3, TGr, 0);
        PushQuad(group, texture, lightIndexes, P1, T0, TGr, P5, T1, BGr, P6, T2, BGr, P2, T3, TGr, 0);
        PushQuad(group, texture, lightIndexes, P7, T0, BGr, P4, T1, BGr, P0, T2, TGr, P3, T3, TGr, 0);
    }
}

inline void PushTexturedCube(RenderGroup* group, Vec3 P, r32 height, r32 width, BitmapId ID, Vec4 color = V4(1.0f, 1.0f, 1.0f, 1.0f), Vec4 lightIndexes = V4(-1, -1, -1, -1))
{
    Bitmap* bitmap = GetBitmap(group->assets, ID);
    if(bitmap)
    {
        PushCube_(group, P, height, width, bitmap->textureHandle, color, lightIndexes);
    }
    else
    {
        LoadBitmap(group->assets, ID, false);
    }
}

inline void PushCube(RenderGroup* group, Vec3 P, r32 height, r32 width, Vec4 color, Vec4 lightIndexes = V4(-1, -1, -1, -1))
{
    PushCube_(group, P, height, width, group->whiteTexture, color, lightIndexes);
}

inline void PushTrunkatedPyramid(RenderGroup* group, Vec3 P, u32 subdivisions, Vec3 XAxisBottom, Vec3 YAxisBottom, Vec3 ZAxis, Vec3 XAxisTop, Vec3 YAxisTop, r32 radiousBottom, r32 radiousTop, r32 height, Vec4 color, Vec4 lightIndexes, r32 modulationPercentage)
{
    RenderTexture texture = group->whiteTexture;
    r32 anglePerSubdivision = TAU32 / subdivisions;
    Vec4 darkColor = V4(color.rgb * 0.65f, color.a); 
    u32 C = StoreColor(color);
    u32 CD = StoreColor(darkColor);
    
    Vec4 PBottom = V4(P, 0);
    Vec4 PTop = PBottom + V4(height * ZAxis, 0);
    
    Vec4 basePBottom = PBottom + V4(XAxisBottom * radiousBottom, 0);
    Vec4 basePTop = PTop + V4(XAxisTop * radiousTop, 0);
    
    r32 angle = anglePerSubdivision;
    
    for(u32 subIndex = 0; subIndex < subdivisions; ++subIndex)
    {
        Vec3 rotatedAxisBottom = RotateVectorAroundAxis(XAxisBottom, ZAxis, angle);
        Vec3 rotatedAxisTop = RotateVectorAroundAxis(XAxisTop, ZAxis, angle);
        
        Vec4 rotatedPBottom = PBottom + V4(rotatedAxisBottom * radiousBottom, 0);
        Vec4 rotatedPTop = PTop + V4(rotatedAxisTop * radiousTop, 0);
        
        u32 cToUse = (subIndex % 2 == 0) ? CD : C;
        
        PushTriangle(group, texture, lightIndexes, basePBottom, cToUse, rotatedPBottom, cToUse, rotatedPTop, cToUse, modulationPercentage);
        PushTriangle(group, texture, lightIndexes, basePBottom, cToUse, rotatedPTop, cToUse, basePTop, cToUse, modulationPercentage);
        
        PushTriangle(group, texture, lightIndexes, rotatedPBottom, cToUse, basePBottom, cToUse, PBottom, cToUse, modulationPercentage);
        PushTriangle(group, texture, lightIndexes, PTop, cToUse, basePTop, cToUse, rotatedPTop, cToUse, modulationPercentage);
        
        basePBottom = rotatedPBottom;
        basePTop = rotatedPTop;
        
        angle += anglePerSubdivision;
    }
}

inline void PushModel(RenderGroup* group, VertexModel* model, Vec3 P, Vec4 lightIndexes,  Vec3 scale = V3(1, 1, 1), Vec4 color = V4(1, 1, 1, 1), r32 modulationPercentage = 0.0f)
{
    GameRenderCommands* commands = group->commands;
    TexturedQuadsCommand* entry = GetCurrentTriangles(group, model->faceCount, model->vertexCount, model->faceCount * 3);
    Assert(entry);
    
    if(entry)
    {
        entry->triangleCount += model->faceCount;
        u32 vertIndex = commands->vertexCount;
        u16* indeces = commands->indexArray + commands->indexCount;
        u16 VI = SafeTruncateToU16(vertIndex - entry->vertexArrayOffset);
        
        commands->indexCount += model->faceCount * 3;
        commands->vertexCount += model->vertexCount;
        
        u16 offset = 0;
        for(u32 faceIndex = 0; faceIndex < model->faceCount; ++faceIndex)
        {
            ModelFace* face = model->faces + faceIndex;
            indeces[offset + 0] = VI + face->i0;
            indeces[offset + 1] = VI + face->i1;
            indeces[offset + 2] = VI + face->i2;
            
            offset += 3;
        }
        
        TexturedVertex* vertexes = commands->vertexArray + vertIndex;
        for(u32 vertexIndex = 0; vertexIndex < model->vertexCount; ++vertexIndex)
        {
            ColoredVertex* vert = model->vertexes + vertexIndex;
            Vec3 vertP = P + Hadamart(vert->P, scale);
            
            PushVertex(vertexes + vertexIndex, V4(vertP, 0), V3(0, 0, 0), V2(0, 0), RGBAPack8x4(Hadamart(vert->color, color) * 255.0f), lightIndexes, modulationPercentage, 0);
        }
    }
}

inline Font* PushFont(RenderGroup* renderGroup, FontId ID)
{
    Font* font = GetFont(renderGroup->assets, ID);
    if(font)
    {
        // NOTE(Leonardo): nothing to do
    }
    else
    {
        ++renderGroup->countMissing;
        LoadFont(renderGroup->assets, ID, false);
    }
    
    return font;
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

#if 0
inline Rect2i GetClipRect(RenderGroup* renderGroup, Vec3 P, Vec2 dim)
{
    Vec2 finalHalfDim = 0.5f * dim;
    Vec2 min = P.xy - finalHalfDim;
    Vec2 max = P.xy + finalHalfDim;
    
    Rect2i rect = { RoundReal32ToI32(min.x), RoundReal32ToI32(min.y), RoundReal32ToI32(max.x), RoundReal32ToI32(max.y) };
    return rect;
}
#endif

inline void PushClipRect(RenderGroup* renderGroup, Rect2 rect, r32 z = 0)
{
    Rect2i clipRect = GetClipRect_(renderGroup, V3(GetCenter(rect), z), GetDim(rect));
    PushClipRect(renderGroup, clipRect);
}

inline void PushAmbientColor(RenderGroup* renderGroup, Vec3 ambientLightColor)
{
    RenderSetup setup = renderGroup->lastSetup;
    setup.ambientLightColor = ambientLightColor;
    PushSetup(renderGroup, &setup);
}

inline u32 PushPointLight(RenderGroup* renderGroup, Vec3 P, Vec3 color, r32 strength)
{
    u32 result = renderGroup->commands->lightCount++;
    PointLight* light = renderGroup->commands->lights + result;
    light->P = P;
    light->color = color;
    light->strength = strength;
    
    return result;
}

inline void SetCameraTransform(RenderGroup* renderGroup, u32 flags, r32 focalLength, Vec3 cameraX = V3(1, 0, 0), Vec3 cameraY = V3(0, 1, 0), Vec3 cameraZ = V3(0, 0, 1), Vec3 cameraP = V3(0, 0, 0), Vec2 screenCameraOffset = V2(0, 0))
{
    b32 orthographic = flags & Camera_Orthographic;
    b32 isDebug = flags & Camera_Debug;
    
    CameraTransform* transform = isDebug ? &renderGroup->debugCamera : &renderGroup->gameCamera;
    
    transform->X = cameraX;
    transform->Y = cameraY;
    transform->Z = cameraZ;
    transform->P = cameraP;
    transform->screenCameraOffset = screenCameraOffset;
    
    r32 aspectRatio = SafeRatio1(1.0f * (r32) renderGroup->commands->settings.width, (r32) renderGroup->commands->settings.height);
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
        int a = 5;
        
    }
    
    
    RenderSetup setup;
    setup.renderTargetIndex = 0;
    setup.proj = transform->proj.forward;
    
    setup.rect = { 0, 0, (i32)renderGroup->commands->settings.width, (i32)renderGroup->commands->settings.height };
    setup.ambientLightColor = V3(1, 1, 1);
    PushSetup(renderGroup, &setup);
}

inline RenderGroup BeginRenderGroup(Assets* assets, GameRenderCommands* commands)
{
    RenderGroup result = {};
    result.assets = assets;
    result.commands = commands;
    result.countMissing = 0;
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
