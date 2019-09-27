#define JC_VORONOI_IMPLEMENTATION
#undef internal
#include "jc_voronoi.h"
#define internal static

struct GenerateVoronoiWork
{
    TaskWithMemory* task;
    u32 pointCount;
    jcv_point* points;
    jcv_rect rect;
    VoronoiDiagram* diagram;
    VoronoiDiagram** activeDiagramPtr;
    GameModeWorld* worldMode;
};


PLATFORM_WORK_CALLBACK(GenerateVoronoiPoints)
{
    GenerateVoronoiWork* work = (GenerateVoronoiWork*) param;
    
    jcv_diagram_generate(work->pointCount, work->points, &work->rect, &work->diagram->diagram);
    
    CompletePastWritesBeforeFutureWrites;
    *work->activeDiagramPtr = work->diagram;
    
    CompletePastWritesBeforeFutureWrites;
    work->worldMode->generatingVoronoi = false;
    
    EndTaskWithMemory(work->task);
}

internal void GenerateVoronoi(GameModeWorld* worldMode, UniversePos originP, i32 originChunkX, i32 originChunkY, i32 chunkApron)
{
    GameState* gameState = worldMode->gameState;
    TaskWithMemory* task = BeginTaskWithMemory(gameState->tasks, ArrayCount(gameState->tasks), false);
    if(task)
    {
        worldMode->generatingVoronoi = true;
        VoronoiDiagram* diagram = worldMode->voronoiPingPong + 0;
        if(diagram == worldMode->activeDiagram)
        {
            diagram = worldMode->voronoiPingPong + 1;
        }
        diagram->deltaP = {};
        diagram->originP = originP;
        
        r32 voxelSide = VOXEL_SIZE;
        r32 chunkSide = voxelSide * CHUNK_DIM;
        u8 chunkDim = CHUNK_DIM;
        
        jcv_rect rect;
        r32 dim = (chunkApron + 4.0f) * chunkSide;
        rect.min.x = -dim;
        rect.min.y = -dim;
        rect.max.x = dim;
        rect.max.y = dim;
        
        u32 maxPointsPerTile = 16;
        u32 maxPointCount = Squarei(chunkDim) * Squarei(2 * chunkApron + 1) * maxPointsPerTile;
        
        
        jcv_point* points = PushArray(&task->pool, jcv_point, maxPointCount, NoClear());
        u32 pointCount = 0;
        for(i32 Y = originChunkY - chunkApron; Y <= originChunkY + chunkApron; Y++)
        {
            for(i32 X = originChunkX - chunkApron; X <= originChunkX + chunkApron; X++)
            {
                Vec3 chunkLowLeftCornerOffset = V3(V2i(X - originChunkX, Y - originChunkY), 0.0f) * chunkSide - originP.chunkOffset;
                Rect2 chunkRect = RectMinDim(chunkLowLeftCornerOffset.xy, V2(chunkSide, chunkSide));
                
                RandomSequence seq = Seed((X + 10) * (Y + 10));
                WorldChunk* chunk = GetChunk(worldMode->chunks, ArrayCount(worldMode->chunks), X, Y, worldMode->persistentPool);
                
                Assert(chunk->initialized);
                
                Assert(X == chunk->worldX);
                Assert(Y == chunk->worldY);
                
                for(u8 tileY = 0; tileY < chunkDim; tileY++)
                {
                    for(u8 tileX = 0; tileX < chunkDim; tileX++)
                    {
                        Vec2 tileCenter = voxelSide * V2(tileX + 0.5f, tileY + 0.5f);
                        Vec2 destP2D = chunkLowLeftCornerOffset.xy + tileCenter;
                        
                        
                        WorldTile* tile = GetTile(chunk, tileX, tileY);
                        
                        r32 groundPointMaxOffset = 0.5f;
                        u32 groundPointPerTile = 4;
                        r32 groundPointPerTileV = 1.0f;
                        
                        r32 pointMaxOffset = Min(0.5f * voxelSide, groundPointMaxOffset);
                        
                        u32 pointsPerTile = RoundReal32ToU32(groundPointPerTile + RandomBil(&seq) * groundPointPerTileV);
                        
                        pointsPerTile = Min(maxPointsPerTile, pointsPerTile);
                        for(u32 pointI = 0; pointI < pointsPerTile; ++pointI)
                        {
                            points[pointCount].x = destP2D.x + RandomBil(&seq) * pointMaxOffset;
                            points[pointCount].y = destP2D.y + RandomBil(&seq) * pointMaxOffset;
                            
                            ++pointCount;
                        }
                    }
                }
            }
        }
        
        if(diagram == worldMode->activeDiagram)
        {
            jcv_diagram_free(&diagram->diagram);
            memset(&diagram->diagram, 0, sizeof(jcv_diagram));
        }
        
        GenerateVoronoiWork* work = PushStruct(&task->pool, GenerateVoronoiWork);
        work->task = task;
        work->pointCount = pointCount;
        work->points = points;
        work->rect = rect;
        work->diagram = diagram;
        work->activeDiagramPtr = &worldMode->activeDiagram;
        work->worldMode = worldMode;
        platformAPI.PushWork(gameState->slowQueue, GenerateVoronoiPoints, work);
    }
}

internal WaterPhase* GetWaterPhaseLeft(WaterParams* params, r32 elevation)
{
    WaterPhase* result = 0;
    for(u32 phaseIndex = 0; phaseIndex < params->phaseCount; ++phaseIndex)
    {
        WaterPhase* test = params->phases + phaseIndex;
        if(test->currentHeight > elevation)
        {
            result = test;
            break;
        }
    }
    
    return result;
}

internal WaterPhase* GetWaterPhaseRight(WaterParams* params, r32 elevation)
{
    WaterPhase* result = 0;
    for(u32 phaseIndex = 0; phaseIndex < params->phaseCount; ++phaseIndex)
    {
        WaterPhase* test = params->phases + phaseIndex;
        if(test->currentHeight <= elevation)
        {
            result = test;
        }
    }
    
    return result;
}

inline Vec4 GetWaterColor(WaterParams* params, WorldTile* tile)
{
    Vec4 waterColor = {};
    if(tile->elevation < 0)
    {
        WaterPhase* left = GetWaterPhaseLeft(params, tile->elevation);
        WaterPhase* right = GetWaterPhaseRight(params, tile->elevation);
        
        if(left && right)
        {
            r32 normalizedWaterLevel = Clamp01MapToRange(left->referenceHeight, tile->elevation, right->referenceHeight);
            
            Vec3 minColor = Lerp(left->minColor, normalizedWaterLevel, right->minColor);
            Vec3 maxColor = Lerp(left->maxColor, normalizedWaterLevel, right->maxColor);
            
            r32 minAlpha = Lerp(left->minAlpha, normalizedWaterLevel, right->minAlpha);
            r32 maxAlpha = Lerp(left->maxAlpha, normalizedWaterLevel, right->maxAlpha);
            
            // TODO(Leonardo): make this vary on waterSeed
            r32 sineWeight = Lerp(left->sineWeight, normalizedWaterLevel, right->sineWeight);
            
            r32 maxColorDisplacement = Lerp(left->maxColorDisplacement, normalizedWaterLevel, right->maxColorDisplacement);
            r32 maxAlphaDisplacement = Lerp(left->maxAlphaDisplacement, normalizedWaterLevel, right->maxAlphaDisplacement);
            
            r32 sine = Sin(tile->waterTime);
            r32 colorNoiseSine = Lerp(tile->blueNoise, sineWeight, sine);
            r32 alphaNoiseSine = Lerp(tile->alphaNoise, sineWeight, sine);
            
            r32 blueDisplacement = colorNoiseSine * maxColorDisplacement;
            r32 alphaDisplacement = alphaNoiseSine * maxAlphaDisplacement;
            
            r32 blueLerp = Clamp01(normalizedWaterLevel + blueDisplacement);
            r32 alphaLerp = Clamp01(normalizedWaterLevel + alphaDisplacement);
            
            Vec3 color = Lerp(minColor, blueLerp, maxColor);
            r32 alpha = Lerp(maxAlpha, alphaLerp, minAlpha);
            
            waterColor = V4(color, alpha);
        }
    }
    
    return waterColor;
}

struct RenderVoronoiWork
{
    RenderGroup* group;
    VoronoiDiagram* voronoi;
    WaterParams* water;
    jcv_edge* edges;
    u32 edgeCount;
    
    ReservedVertexes triangleVertexes;
    ReservedVertexes quadVertexes;
};

PLATFORM_WORK_CALLBACK(RenderVoronoiEdges)
{
    RenderVoronoiWork* work = (RenderVoronoiWork*) param;
    
    RenderGroup* group = work->group;
    VoronoiDiagram* voronoi = work->voronoi;
    
    u32 counter = 0;
    jcv_edge* edge = work->edges;
    
    
    WorldTile nullTile = {};
    nullTile.elevation = minHeight;
    
    ReservedVertexes* triangleVertexes = &work->triangleVertexes;
    ReservedVertexes* quadVertexes = &work->quadVertexes;
    while(counter < work->edgeCount)
    {
        jcv_site* site0 = edge->sites[0];
        jcv_site* site1 = edge->sites[1];
        
        Vec2 site0P = V2(site0->p.x, site0->p.y);
        Vec2 site1P = V2(site1->p.x, site1->p.y);
        
        WorldTile* QSite0 = site0->tile ? site0->tile : &nullTile;
        WorldTile* QSite1 = site1->tile ? site1->tile : &nullTile;
        WorldTile* QFrom = edge->tile[0] ? edge->tile[0] : &nullTile;
        WorldTile* QTo = edge->tile[1] ? edge->tile[1] : &nullTile;;
        
        Vec2 offsetFrom = V2(edge->pos[0].x, edge->pos[0].y);
        Vec2 offsetTo = V2(edge->pos[1].x, edge->pos[1].y);
        
        if(offsetFrom.y > offsetTo.y)
        {
            Vec2 temp = offsetFrom;
            offsetFrom = offsetTo;
            offsetTo = temp;
            
            WorldTile* tempTile = QFrom;
            QFrom = QTo;
            QTo = tempTile;
        }
        
        
        
        Vec2 site0TileOffset = {};
        Vec2 site1TileOffset = {};
        
        
        Vec2 fromTileOffset = offsetFrom;
        Vec2 toTileOffset = offsetTo;
        
        
        r32 chunkyness0 = 0.5f;
        r32 chunkyness1 = 0.5f;
        
        r32 outer0 = Outer(offsetFrom, offsetTo, site0P);
        r32 probe0 = Outer(offsetFrom, offsetTo, offsetFrom - V2(1, 0));
        b32 zeroIsOnLeftSide = ((probe0 < 0 && outer0 < 0) || 
                                (probe0 > 0 && outer0 > 0));
        
        
        r32 zBias = 0.01f;
        
        Vec4 site0PCamera = V4(site0P + voronoi->deltaP.xy, 0, zBias);
        Vec4 site1PCamera = V4(site1P + voronoi->deltaP.xy, 0, zBias);
        Vec4 offsetFromCamera = V4(offsetFrom + voronoi->deltaP.xy, 0, zBias);
        Vec4 offsetToCamera = V4(offsetTo + voronoi->deltaP.xy, 0, zBias);
        
        Vec4 smooth0From = Lerp(offsetFromCamera, chunkyness0, site0PCamera);
        Vec4 smooth1From = Lerp(offsetFromCamera, chunkyness1, site1PCamera);
        
        Vec4 smooth0To = Lerp(offsetToCamera, chunkyness0, site0PCamera);
        Vec4 smooth1To = Lerp(offsetToCamera, chunkyness1, site1PCamera);
        
        Vec4 waterColor0 = GetWaterColor(work->water, QSite0);
        Vec4 waterColor1 = GetWaterColor(work->water, QSite1);
        Vec4 waterColorFrom = GetWaterColor(work->water, QFrom);
        Vec4 waterColorTo = GetWaterColor(work->water, QTo);
        
        Vec4 waterOffset = V4(0, 0, 0.01f, 0);
        
        if(QSite0->elevation < 0)
        {
            if(zeroIsOnLeftSide)
            {
                PushTriangle(group, group->whiteTexture, QSite0->lights, triangleVertexes, site0PCamera + waterOffset, waterColor0, offsetFromCamera + waterOffset, waterColorFrom, offsetToCamera + waterOffset, waterColorTo, 0);
            }
            else
            {
                PushTriangle(group, group->whiteTexture, QSite0->lights, triangleVertexes, site0PCamera + waterOffset, waterColor0, offsetToCamera + waterOffset, waterColorTo, offsetFromCamera + waterOffset, waterColorFrom, 0);
            }
        }
        
        if(QSite1->elevation < 0)
        {
            if(zeroIsOnLeftSide)
            {
                PushTriangle(group, group->whiteTexture, QSite1->lights, triangleVertexes, site1PCamera + waterOffset, waterColor1, offsetToCamera + waterOffset, waterColorTo, offsetFromCamera + waterOffset, waterColorFrom, 0);
            }
            else
            {
                PushTriangle(group, group->whiteTexture, QSite1->lights, triangleVertexes, site1PCamera + waterOffset, waterColor1, offsetFromCamera + waterOffset, waterColorFrom, offsetToCamera + waterOffset, waterColorTo, 0);
            }
        }
        
        
        edge = edge->next;
        ++counter;
    }
}

internal Vec4 BlendTilesColor(WorldTile* t0, WorldTile* t1, WorldTile* t2, WorldTile* t3)
{
    Vec4 defaultColor = V4(0.5f, 0.5f, 0.5f, 1.0f);
    Vec4 t0C = t0 ? t0->color : defaultColor;
    Vec4 t1C = t1 ? t1->color : defaultColor;
    Vec4 t2C = t2 ? t2->color : defaultColor;
    Vec4 t3C = t3 ? t3->color : defaultColor;
    Vec4 result = 0.25f * (t0C + t1C + t2C + t3C);
    
    return result;
}

internal Vec4 GetWaterColor(r32 elevation, RandomSequence* seq)
{
    r32 lerp = Clamp01MapToRange(minHeight, elevation, 0);
    
    if(seq)
    {
        lerp += 0.3f * RandomBil(seq);
        lerp = Clamp01(lerp);
    }
    
    Vec4 result = Lerp(V4(0, 0, 1, 0.7f), lerp, V4(0, 0.5f, 1.0f, 0.1f));
    return result;
}

inline void UpdateAndRenderGround(GameModeWorld* worldMode, RenderGroup* group, UniversePos origin, UniversePos oldOrigin, r32 timeToAdvance)
{
    u32 worldSeed = 1111;
    
    i32 originChunkX = origin.chunkX;
    i32 originChunkY = origin.chunkY;
    i32 chunkApron = worldMode->chunkApron;
    
    if(chunkApron > 4)
    {
        worldMode->worldTileView = true;
    }
    
    if(chunkApron > 8)
    {
        worldMode->worldChunkView = true;
    }
    
    r32 chunkSide = CHUNK_DIM * VOXEL_SIZE;
    r32 voxelSide = VOXEL_SIZE;
    
    b32 forceVoronoiRegeneration = false;
    for(i32 Y = originChunkY - chunkApron - 1; Y <= originChunkY + chunkApron + 1; Y++)
    {
        for(i32 X = originChunkX - chunkApron - 1; X <= originChunkX + chunkApron + 1; X++)
        {
            if(ChunkValid(X, Y))
            {
                WorldChunk* chunk = GetChunk(worldMode->chunks, ArrayCount(worldMode->chunks), X, Y, worldMode->persistentPool);
                if(!chunk->initialized)
                {
                    forceVoronoiRegeneration = true;
                    Assert(chunk->texture.textureHandle.width == 0);
                    Assert(chunk->texture.textureHandle.height == 0);
                    BuildChunk(worldMode, group->assets, chunk, X, Y, worldSeed);
                }
            }
        }
    }
    
    b32 changedChunk = (origin.chunkX != oldOrigin.chunkX || origin.chunkY != oldOrigin.chunkY);
    
    RandomSequence assetSeq = Seed(worldSeed);
    AssetID waterID = QueryDataFiles(group->assets, WaterParams, 0, &assetSeq, 0);
    
    if(IsValid(waterID))
    {
        WaterParams* water = GetData(group->assets, WaterParams, waterID);
        for(u32 phaseIndex = 0; phaseIndex < water->phaseCount; ++phaseIndex)
        {
            WaterPhase* phase = water->phases + phaseIndex;
            
            r32 speed = phase->heightSpeed + RandomBil(&worldMode->entropy) * phase->heightSpeedV;
            phase->runningTime += speed * timeToAdvance;
            r32 sine = Sin(phase->runningTime);
            phase->currentHeight = phase->referenceHeight + sine * phase->referenceHeightV;
        }
        
        for(i32 Y = originChunkY - chunkApron - 1; Y <= originChunkY + chunkApron + 1; Y++)
        {
            for(i32 X = originChunkX - chunkApron - 1; X <= originChunkX + chunkApron + 1; X++)
            {
                WorldChunk* chunk = GetChunk(worldMode->chunks, ArrayCount(worldMode->chunks), X, Y, worldMode->persistentPool);
                
                for(u32 tileY = 0; tileY < CHUNK_DIM; ++tileY)
                {
                    for(u32 tileX = 0; tileX < CHUNK_DIM; ++tileX)
                    {
                        WorldTile* tile = GetTile(chunk, tileX, tileY);
                        WaterPhase* phase = GetWaterPhaseLeft(water, tile->elevation);
                        if(phase)
                        {
                            r32 waterTileSpeed = phase->colorSpeed + RandomBil(&tile->entropy) * phase->colorSpeedV;
                            if(tile->movingNegative)
                            {
                                tile->waterRandomization -= waterTileSpeed * timeToAdvance;
                                if(tile->waterRandomization < 0)
                                {
                                    tile->waterRandomization = 0;
                                    tile->movingNegative = false;
                                }
                            }
                            else
                            {
                                tile->waterRandomization += waterTileSpeed * timeToAdvance;
                                if(tile->waterRandomization > 1.0f)
                                {
                                    tile->waterRandomization = 1.0f;
                                    tile->movingNegative = true;
                                }
                            }
                            
                            r32 blueNoise = Evaluate(tile->waterRandomization, 0, phase->noise, tile->waterSeed);
                            r32 alphaNoise = Evaluate(tile->waterRandomization, 0, phase->noise, tile->waterSeed);
                            tile->blueNoise = UnilateralToBilateral(blueNoise);
                            tile->alphaNoise = UnilateralToBilateral(alphaNoise);
                            
                            tile->waterTime += timeToAdvance;
                        }
                    }
                }
            }
        }
        
        
        if(changedChunk || forceVoronoiRegeneration || !worldMode->activeDiagram)
        {
            if(!worldMode->generatingVoronoi)
            {
                GenerateVoronoi(worldMode, origin, originChunkX, originChunkY, chunkApron);
            }
        }
        
        
        VoronoiDiagram* voronoi = worldMode->activeDiagram;
        if(voronoi)
        {
            TempMemory voronoiMemory = BeginTemporaryMemory(worldMode->temporaryPool);
            BEGIN_BLOCK("voronoi sites");
            jcv_site* sites = jcv_diagram_get_sites(&voronoi->diagram);
            for(int i = 0; i < voronoi->diagram.numsites; ++i)
            {
                jcv_site* site = sites + i;
                Vec2 siteP = V2(site->p.x, site->p.y);
                if(!site->tile)
                {
                    site->tile = GetTile(worldMode, voronoi->originP, siteP);
                }
            }
            END_BLOCK();
            
            
            BEGIN_BLOCK("edge rendering");
            jcv_edge* edge = jcv_diagram_get_edges(&voronoi->diagram);
            
            jcv_edge* toRender = edge;
            u32 counter = 0;
            u32 waterCounter = 0;
            while(edge)
            {
                Vec2 offsetFrom = V2(edge->pos[0].x, edge->pos[0].y);
                Vec2 offsetTo = V2(edge->pos[1].x, edge->pos[1].y);
                
                if(!edge->tile[0])
                {
                    edge->tile[0] = GetTile(worldMode, voronoi->originP, offsetFrom);
                }
                if(edge->sites[0]->tile->elevation < 0)
                {
                    ++waterCounter;
                }
                
                if(!edge->tile[1])
                {
                    edge->tile[1] = GetTile(worldMode, voronoi->originP, offsetTo);
                }
                if(edge->sites[1]->tile->elevation < 0)
                {
                    ++waterCounter;
                }
                
                edge = edge->next;
                if(++counter == 512 || !edge)
                {
                    RenderVoronoiWork* work = PushStruct(worldMode->temporaryPool, RenderVoronoiWork);
                    work->group = group;
                    work->voronoi = voronoi;
                    work->water = water;
                    work->edges = toRender;
                    work->edgeCount = counter;
                    work->triangleVertexes = ReserveTriangles(group, waterCounter);
                    
#if 0                     
                    platformAPI.PushWork(worldMode->gameState->renderQueue, RenderVoronoiEdges, work);
#else
                    RenderVoronoiEdges(work);
#endif
                    
                    toRender = edge;
                    counter = 0;
                    waterCounter = 0;
                }
            }
            
            platformAPI.CompleteQueueWork(worldMode->gameState->renderQueue);
            EndTemporaryMemory(voronoiMemory);
            END_BLOCK();
        }
        
    }
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    b32 generatedTextureThisFrame = false;
    for(i32 chunkY = originChunkY - chunkApron; (chunkY <= originChunkY + chunkApron) && !generatedTextureThisFrame; chunkY++)
    {
        for(i32 chunkX = originChunkX - chunkApron; 
            (chunkX <= originChunkX + chunkApron) && !generatedTextureThisFrame; chunkX++)
        {
            if(ChunkValid(chunkX, chunkY))
            {	
                WorldChunk* chunk = GetChunk(worldMode->chunks, ArrayCount(worldMode->chunks), chunkX, chunkY, worldMode->persistentPool);
                
                if(chunk->initialized)
                {
                    Vec3 chunkLowLeftCornerOffset = V3(V2i(chunk->worldX - originChunkX, chunk->worldY - originChunkY), 0.0f) * chunkSide - origin.chunkOffset;
                    Vec3 chunkBaseCenterOffset = chunkLowLeftCornerOffset + 0.5f * V3(chunkSide, chunkSide, 0);
                    
                    if(worldMode->worldChunkView)
                    {
                        Vec4 averageColor = {};
                        u32 waterCount = 0;
                        Vec4 waterColor = {};
                        
                        for(u8 Y = 0; Y < CHUNK_DIM; ++Y)
                        {
                            for(u8 X = 0; X < CHUNK_DIM; ++X)
                            {
                                WorldTile* tile = GetTile(chunk, X, Y);
                                averageColor += tile->color;
                                
                                if(tile->elevation < 0)
                                {
                                    ++waterCount;
                                    waterColor += GetWaterColor(tile->elevation, 0);
                                }
                            }
                        }
                        
                        Vec4 finalColor = averageColor * (1.0f / Square(CHUNK_DIM));
                        PushRect(group, FlatTransform(), 
                                 chunkBaseCenterOffset, V2(chunkSide, chunkSide), finalColor, {});
                        
                        Vec4 finalWaterColor = waterColor * (1.0f / waterCount);
                        PushRect(group, FlatTransform(), 
                                 chunkBaseCenterOffset, V2(chunkSide, chunkSide), finalWaterColor, {});
                        
                    }
                    else if(worldMode->worldTileView)
                    {
                        for(u8 Y = 0; Y < CHUNK_DIM; ++Y)
                        {
                            for(u8 X = 0; X < CHUNK_DIM; ++X)
                            {
                                Vec3 tileMin = -0.5f * V3(chunkSide, chunkSide, 0) + V3(X * voxelSide, Y * voxelSide, 0);
                                Vec2 tileDim = V2(voxelSide, voxelSide);
                                
                                Rect2 tileSurface = RectMinDim(tileMin.xy, tileDim);
                                Vec2 tileCenter = GetCenter(tileSurface);
                                
                                WorldTile* sTiles[9];
                                u32 index = 0;
                                for(i32 tileY = (i32) Y - 1; tileY <= (i32) Y + 1; ++tileY)
                                {
                                    for(i32 tileX = (i32) X - 1; tileX <= (i32) X + 1; ++tileX)
                                    {
                                        sTiles[index++] = GetTile(worldMode, chunk, tileX, tileY);
                                    }
                                }
                                
                                Vec4 c0 = BlendTilesColor(sTiles[0], sTiles[1], sTiles[3], sTiles[4]);
                                Vec4 c1 = BlendTilesColor(sTiles[1], sTiles[2], sTiles[4], sTiles[5]);
                                Vec4 c2 = BlendTilesColor(sTiles[4], sTiles[5], sTiles[7], sTiles[8]);
                                Vec4 c3 = BlendTilesColor(sTiles[3], sTiles[4], sTiles[6], sTiles[7]);
                                
                                Vec3 finalTileP = chunkBaseCenterOffset + V3(tileCenter, 0);
                                
                                PushRect4Colors(group, FlatTransform(), finalTileP, tileDim, 
                                                c0, c1, c2, c3, {});
                                WorldTile* tile = sTiles[4];
                                if(tile->elevation < 0)
                                {
                                    Vec4 waterColor = GetWaterColor(tile->elevation, 0);
                                    PushRect(group, FlatTransform(), finalTileP, tileDim, waterColor);
                                }
                            }
                        }
                    }
                    else
                    {
                        if(IsValidSpecial(&chunk->texture))
                        {
                            PushTexture(group, chunk->texture.textureHandle, chunkLowLeftCornerOffset, V3(chunkSide, 0, 0), V3(0, chunkSide, 0), V4(1, 1, 1, 1));
                        }
                        else
                        {
                            u32 textureIndex = chunk->texture.textureHandle.index;
                            if(!textureIndex)
                            {
                                textureIndex = AcquireSpecialTextureHandle(group->assets);
                            }
                            
                            chunk->texture.textureHandle = TextureHandle(textureIndex, MAX_IMAGE_DIM, MAX_IMAGE_DIM);
                            
                            RenderSetup lastSetup = group->lastSetup;
                            SetOrthographicTransform(group, chunkSide, chunkSide, textureIndex);
                            
                            for(u8 Y = 0; Y < CHUNK_DIM; ++Y)
                            {
                                for(u8 X = 0; X < CHUNK_DIM; ++X)
                                {
                                    Vec3 tileMin = -0.5f * V3(chunkSide, chunkSide, 0) + V3(X * voxelSide, Y * voxelSide, 0);
                                    Vec2 tileDim = V2(voxelSide, voxelSide);
                                    
                                    Rect2 tileSurface = RectMinDim(tileMin.xy, tileDim);
                                    Vec2 tileCenter = GetCenter(tileSurface);
                                    
                                    WorldTile* sTiles[9];
                                    u32 index = 0;
                                    for(i32 tileY = (i32) Y - 1; tileY <= (i32) Y + 1; ++tileY)
                                    {
                                        for(i32 tileX = (i32) X - 1; tileX <= (i32) X + 1; ++tileX)
                                        {
                                            sTiles[index++] = GetTile(worldMode, chunk, tileX, tileY);
                                        }
                                    }
                                    
                                    Vec4 c0 = BlendTilesColor(sTiles[0], sTiles[1], sTiles[3], sTiles[4]);
                                    Vec4 c1 = BlendTilesColor(sTiles[1], sTiles[2], sTiles[4], sTiles[5]);
                                    Vec4 c2 = BlendTilesColor(sTiles[4], sTiles[5], sTiles[7], sTiles[8]);
                                    Vec4 c3 = BlendTilesColor(sTiles[3], sTiles[4], sTiles[6], sTiles[7]);
                                    
                                    PushRect4Colors(group, FlatTransform(), V3(tileCenter, 0), tileDim, 
                                                    c0, c1, c2, c3, {});
                                }
                            }
                            
                            for(i32 deltaY = -1; deltaY <= 1; ++deltaY)
                            {
                                for(i32 deltaX = -1; deltaX <= 1; ++deltaX)
                                {
                                    i32 splatX = chunk->worldX + deltaX;
                                    i32 splatY = chunk->worldY + deltaY;
                                    
                                    if(ChunkValid(splatX, splatY))
                                    {
                                        WorldChunk* splatChunk = GetChunk(worldMode->chunks, ArrayCount(worldMode->chunks), splatX, splatY, 0);
                                        Assert(splatChunk->initialized);
                                        Vec3 chunkCenterOffset = chunkSide * V3((r32) deltaX, (r32) deltaY, 0);
                                        
                                        
                                        RandomSequence seq = GetChunkSeed(splatChunk->worldX, splatChunk->worldY, worldSeed);
                                        for(u8 Y = 0; Y < CHUNK_DIM; ++Y)
                                        {
                                            for(u8 X = 0; X < CHUNK_DIM; ++X)
                                            {
                                                Vec3 tileMin = chunkCenterOffset + -0.5f * V3(chunkSide, chunkSide, 0) + V3(X * voxelSide, Y * voxelSide, 0);
                                                Vec2 tileDim = V2(voxelSide, voxelSide);
                                                
                                                Rect2 tileSurface = RectMinDim(tileMin.xy, tileDim);
                                                Vec2 tileCenter = GetCenter(tileSurface);
                                                
                                                WorldTile* tile = GetTile(splatChunk, X, Y);
                                                
                                                GameProperties properties = {};
                                                properties.properties[0] = tile->property;
                                                
                                                BitmapId groundID = QueryBitmaps(group->assets, tile->asset.subtype, &seq, &properties);
                                                
                                                if(IsValid(groundID))
                                                {
                                                    LockAssetForCurrentFrame(group->assets, groundID);
                                                    
                                                    Vec3 splatP = V3(tileCenter, 0);
                                                    r32 height = voxelSide;
                                                    Vec2 scale = V2(RandomRangeFloat(&seq, 1.0f, 2.5f), RandomRangeFloat(&seq, 1.0f, 2.5f));
                                                    Vec4 color = V4(1, 1, 1, 1);
                                                    color.r += RandomBil(&seq) * 0.1f;
                                                    color.g += RandomBil(&seq) * 0.1f;
                                                    color.b += RandomBil(&seq) * 0.1f;
                                                    color = Clamp01(color);
                                                    
                                                    ObjectTransform transform = FlatTransform();
                                                    transform.angle = RandomUni(&seq) * TAU32;
                                                    if(!PushBitmap(group, transform, groundID, splatP, height * voxelSide, scale, color))
                                                    {
                                                        InvalidCodePath;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            PushSetup(group, &lastSetup);
                            //generatedTextureThisFrame = true;
                        }
                        
                        Assert(IsValidSpecial(&chunk->texture));
                        RefreshSpecialTexture(group->assets, &chunk->texture);
                    }
                }
            }
        }
    }
}
