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

internal void GenerateVoronoi(GameState* gameState, GameModeWorld* worldMode, UniversePos originP, i32 originChunkX, i32 originChunkY, i32 chunkApron, i32 lateralChunkSpan)
{
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
        
        r32 voxelSide = worldMode->voxelSide;
        r32 chunkSide = worldMode->chunkSide;
        u8 chunkDim = worldMode->chunkDim;
        
        jcv_rect rect;
        r32 dim = (chunkApron + 4.0f) * chunkSide;
        rect.min.x = -dim;
        rect.min.y = -dim;
        rect.max.x = dim;
        rect.max.y = dim;
        
        u32 maxPointsPerTile = 16;
        u32 maxPointCount = Squarei(worldMode->chunkDim) * Squarei(2 * chunkApron + 1) * maxPointsPerTile;
        
        
        jcv_point* points = PushArray(&task->pool, jcv_point, maxPointCount, NoClear());
        u32 pointCount = 0;
        for(i32 Y = originChunkY - chunkApron; Y <= originChunkY + chunkApron; Y++)
        {
            for(i32 X = originChunkX - chunkApron; X <= originChunkX + chunkApron; X++)
            {
                Vec3 chunkLowLeftCornerOffset = V3(V2i(X - originChunkX, Y - originChunkY), 0.0f) * chunkSide - originP.chunkOffset;
                Rect2 chunkRect = RectMinDim(chunkLowLeftCornerOffset.xy, V2(chunkSide, chunkSide));
                
                i32 chunkX = Wrap(0, X, lateralChunkSpan);
                i32 chunkY = Wrap(0, Y, lateralChunkSpan);
                
                if(ChunkValid(lateralChunkSpan, X, Y))
                {	
                    RandomSequence seq = Seed((chunkX + 10) * (chunkY + 10));
                    WorldChunk* chunk = GetChunk(worldMode->chunks, ArrayCount(worldMode->chunks), X, Y, worldMode->persistentPool);
                    
                    if(!chunk->initialized)
                    {
                        InvalidCodePath;
                    }
                    
                    Assert(X == chunk->worldX);
                    Assert(Y == chunk->worldY);
                    
                    for(u8 tileY = 0; tileY < worldMode->chunkDim; tileY++)
                    {
                        for(u8 tileX = 0; tileX < worldMode->chunkDim; tileX++)
                        {
                            Vec2 tileCenter = worldMode->voxelSide * V2(tileX + 0.5f, tileY + 0.5f);
                            Vec2 destP2D = chunkLowLeftCornerOffset.xy + tileCenter;
                            
                            
                            WorldTile* tile = &chunk->tiles[tileY][tileX];
                            
                            TaxonomySlot* tileSlot = GetSlotForTaxonomy(worldMode->table, tile->taxonomy);
                            TileDefinition* tileDef = tileSlot->tileDefinition;
                            
                            if(tileDef)
                            {
                                r32 pointMaxOffset = Min(0.5f * voxelSide, tileDef->groundPointMaxOffset);
                                
                                u32 pointsPerTile = RoundReal32ToU32(tileDef->groundPointPerTile + RandomBil(&seq) * tileDef->groundPointPerTileV);
                                
                                pointsPerTile = Min(maxPointsPerTile, pointsPerTile);
                                r32 tileLayoutNoise = tile->layoutNoise;
                                
#if 1
                                for(u32 pointI = 0; pointI < pointsPerTile; ++pointI)
                                {
                                    points[pointCount].x = destP2D.x + RandomBil(&seq) * pointMaxOffset;
                                    points[pointCount].y = destP2D.y + RandomBil(&seq) * pointMaxOffset;
                                    
                                    ++pointCount;
                                }
#else
                                switch(tileDef->tilePointsLayout)
                                {
                                    case TilePoints_StraightLine:
                                    {
                                        Vec2 arm = Arm2(DegToRad(tileLayoutNoise * 360.0f));
                                        r32 voxelUsableDim = 0.9f * voxelSide;
                                        Vec2 startingOffset = arm * voxelUsableDim;
                                        Vec2 totalDelta = 2.0f * startingOffset;
                                        
                                        Vec2 pointSeparationVector = totalDelta *= (1.0f / pointsPerTile);
                                        
                                        destP2D += startingOffset;
                                        
                                        for(u32 pointI = 0; pointI < pointsPerTile; ++pointI)
                                        {
                                            points[pointCount].x = destP2D.x + RandomBil(&seq) * pointMaxOffset;
                                            points[pointCount].y = destP2D.y + RandomBil(&seq) * pointMaxOffset;
                                            
                                            ++pointCount;
                                            
                                            destP2D -= pointSeparationVector;
                                        }
                                    } break;
                                    
                                    case TilePoints_Random:
                                    {
                                        for(u32 pointI = 0; pointI < pointsPerTile; ++pointI)
                                        {
                                            points[pointCount].x = destP2D.x + RandomBil(&seq) * pointMaxOffset;
                                            points[pointCount].y = destP2D.y + RandomBil(&seq) * pointMaxOffset;
                                            
                                            ++pointCount;
                                        }
                                    } break;
                                    
                                    case TilePoints_Pound:
                                    {
                                        for(u32 pointI = 0; pointI < pointsPerTile; ++pointI)
                                        {
                                            points[pointCount].x = destP2D.x + RandomBil(&seq) * pointMaxOffset;
                                            points[pointCount].y = destP2D.y + RandomBil(&seq) * pointMaxOffset;
                                            
                                            ++pointCount;
                                        }
                                    } break;
                                }
#endif
                                
                            }
                            
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


struct RenderVoronoiWork
{
    RenderGroup* group;
    VoronoiDiagram* voronoi;
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
    
    
    ReservedVertexes* triangleVertexes = &work->triangleVertexes;
    ReservedVertexes* quadVertexes = &work->quadVertexes;
    while(counter < work->edgeCount)
    {
        jcv_site* site0 = edge->sites[0];
        jcv_site* site1 = edge->sites[1];
        
        Vec2 site0P = V2(site0->p.x, site0->p.y);
        Vec2 site1P = V2(site1->p.x, site1->p.y);
        
        WorldTile* QSite0 = site0->tile;
        WorldTile* QSite1 = site1->tile;
        
        WorldTile* QFrom = edge->tile[0];
        WorldTile* QTo = edge->tile[1];
        
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
        
        
        r32 chunkyness0 = GetChunkyness(QSite0, QSite1);
        r32 chunkyness1 = GetChunkyness(QSite1, QSite0);
        
        r32 outer0 = Outer(offsetFrom, offsetTo, site0P);
        r32 probe0 = Outer(offsetFrom, offsetTo, offsetFrom - V2(1, 0));
        b32 zeroIsOnLeftSide = ((probe0 < 0 && outer0 < 0) || 
                                (probe0 > 0 && outer0 > 0));
        
        
        r32 zBias = 0.01f;
        
        Vec4 site0PCamera = V4(site0P + voronoi->deltaP.xy, QSite0->height, zBias);
        Vec4 site1PCamera = V4(site1P + voronoi->deltaP.xy, QSite1->height, zBias);
        
        Vec4 offsetFromCamera = V4(offsetFrom + voronoi->deltaP.xy, QFrom->height, zBias);
        Vec4 offsetToCamera = V4(offsetTo + voronoi->deltaP.xy, QTo->height, zBias);
        
        Vec4 smooth0From = Lerp(offsetFromCamera, chunkyness0, site0PCamera);
        Vec4 smooth1From = Lerp(offsetFromCamera, chunkyness1, site1PCamera);
        
        Vec4 smooth0To = Lerp(offsetToCamera, chunkyness0, site0PCamera);
        Vec4 smooth1To = Lerp(offsetToCamera, chunkyness1, site1PCamera);
        
        
        RandomSequence seq = Seed((i32) (LengthSq(offsetTo - offsetFrom) * 1000.0f));
        
        
        Vec4 color0 = GetTileColor(QSite0, false, &seq);
        Vec4 color1 = GetTileColor(QSite1, false, &seq);
        
        Vec4 colorFrom = GetTileColor(QFrom, false, &seq);
        Vec4 colorTo = GetTileColor(QTo, false, &seq);
        
        
        Vec4 borderColor = Lerp(QSite0->borderColor, 0.5f, QSite1->borderColor);
        if(!QSite0->borderColor.a)
        {
            borderColor = QSite1->borderColor;
        }
        else if(!QSite1->borderColor.a)
        {
            borderColor = QSite0->borderColor;
        }
        
        
#if 0        
        if(borderColor.a)
        {
            PushLine(group, borderColor, offsetFromCamera.xyz, offsetToCamera.xyz, 0.02f);
        }
#endif
        
        
#if 0        
        if(zeroIsOnLeftSide)
        {
            PushTriangle(group, group->whiteTexture, QSite0->lights, triangleVertexes,  site0PCamera, color0, smooth0From, color0, smooth0To, color0, 0);
            
            PushTriangle(group, group->whiteTexture, QSite1->lights, triangleVertexes, site1PCamera, color1, smooth1To, color1, smooth1From, color1, 0);
        }
        else
        {
            PushTriangle(group, group->whiteTexture, QSite0->lights, triangleVertexes, site0PCamera, color0, smooth0To, color0, smooth0From, color0, 0);
            
            PushTriangle(group, group->whiteTexture, QSite1->lights, triangleVertexes, site1PCamera, color1, smooth1From, color1, smooth1To, color1, 0);
        }
        
        if(zeroIsOnLeftSide)
        {
            PushQuad(group, group->whiteTexture, QSite0->lights, quadVertexes, smooth0From, {}, color0, offsetFromCamera, {}, colorFrom, offsetToCamera, {}, colorTo, smooth0To, {}, color0, 0);
            
            PushQuad(group, group->whiteTexture, QSite1->lights, quadVertexes, smooth1From, {}, color1, smooth1To, {}, color1, offsetToCamera, {}, colorTo, offsetFromCamera, {}, colorFrom, 0);
        }
        else
        {
            PushQuad(group, group->whiteTexture, QSite0->lights, quadVertexes, smooth0To, {}, color0, offsetToCamera, {}, colorTo, offsetFromCamera, {}, colorFrom, smooth0From, {}, color0, 0);
            
            PushQuad(group, group->whiteTexture, QSite1->lights, quadVertexes, smooth1To, {}, color1, smooth1From, {}, color1, offsetFromCamera, {}, colorFrom, offsetToCamera, {}, colorTo, 0);
        }
#endif
        
        
        
        
        Vec4 waterColor0 = GetWaterColor(QSite0);
        Vec4 waterColor1 = GetWaterColor(QSite1);
        Vec4 waterColorFrom = GetWaterColor(QFrom);
        Vec4 waterColorTo = GetWaterColor(QTo);
        
        Vec4 waterOffset = V4(0, 0, 0.01f, 0);
        
        if(QSite0->waterLevel < WATER_LEVEL)
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
        
        if(QSite1->waterLevel < WATER_LEVEL)
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

internal void UpdateAndRenderGround(GameState* gameState, GameModeWorld* worldMode, RenderGroup* group, ClientPlayer* myPlayer, i32 chunkApron, r32 timeToAdvance)
{
    UniversePos voronoiP = myPlayer->universeP;
    b32 changedChunk = (voronoiP.chunkX != myPlayer->oldVoronoiP.chunkX || voronoiP.chunkY != myPlayer->oldVoronoiP.chunkY);
    myPlayer->oldVoronoiP = voronoiP;
    
    i32 lateralChunkSpan = WORLD_REGION_SPAN * REGION_CHUNK_SPAN;
    i32 originChunkX = voronoiP.chunkX;
    i32 originChunkY = voronoiP.chunkY;
    
    u32 seed = worldMode->worldSeed;
    
    
    
    RandomSequence generatorSeq = Seed(seed);
    u32 generatorTaxonomy = GetRandomChild(worldMode->table, &generatorSeq, worldMode->table->generatorTaxonomy);
    
    if(generatorTaxonomy != worldMode->table->generatorTaxonomy)
    {
        TaxonomySlot* newGeneratorSlot =GetSlotForTaxonomy(worldMode->table, generatorTaxonomy);
        
        if(newGeneratorSlot)
        {
            worldMode->generator = newGeneratorSlot->generatorDefinition;
        }
    }
    
    b32 forceVoronoiRegeneration = false;
    if(worldMode->generator)
    {
        for(i32 Y = originChunkY - chunkApron - 1; Y <= originChunkY + chunkApron + 1; Y++)
        {
            for(i32 X = originChunkX - chunkApron - 1; X <= originChunkX + chunkApron + 1; X++)
            {
                i32 chunkX = Wrap(0, X, lateralChunkSpan);
                i32 chunkY = Wrap(0, Y, lateralChunkSpan);
                
                if(ChunkValid(lateralChunkSpan, X, Y))
                {	
                    WorldChunk* chunk = GetChunk(worldMode->chunks, ArrayCount(worldMode->chunks), X, Y, worldMode->persistentPool);
                    
                    if(!chunk->initialized)
                    {
                        forceVoronoiRegeneration = true;
                        BuildChunk(worldMode->table, worldMode->generator, chunk, X, Y, seed);
                    }
                    
                    
                    r32 waterSpeed = 0.12f;
                    r32 waterSineSpeed = 70.0f;
                    for(u32 tileY = 0; tileY < CHUNK_DIM; ++tileY)
                    {
                        for(u32 tileX = 0; tileX < CHUNK_DIM; ++tileX)
                        {
                            WorldTile* tile = GetTile(chunk, tileX, tileY);
                            if(tile->movingNegative)
                            {
                                tile->waterPhase -= waterSpeed * timeToAdvance;
                                if(tile->waterPhase < 0)
                                {
                                    tile->waterPhase = 0;
                                    tile->movingNegative = false;
                                }
                            }
                            else
                            {
                                tile->waterPhase += waterSpeed * timeToAdvance;
                                if(tile->waterPhase > 1.0f)
                                {
                                    tile->waterPhase = 1.0f;
                                    tile->movingNegative = true;
                                }
                            }
                            
                            RandomSequence seq = tile->waterSeq;
                            NoiseParams waterParams = NoisePar(4.0f, 2, 0.0f, 1.0f);
                            r32 blueNoise = Evaluate(tile->waterPhase, 0, waterParams, GetNextUInt32(&seq));
                            r32 alphaNoise = Evaluate(tile->waterPhase, 0, waterParams, GetNextUInt32(&seq));
                            tile->blueNoise = UnilateralToBilateral(blueNoise);
                            tile->alphaNoise = UnilateralToBilateral(alphaNoise);
                            
                            
                            tile->waterSine += waterSineSpeed * timeToAdvance;
                        }
                    }
                }
            }
        }
    }
    
    
    
    if(changedChunk || !worldMode->activeDiagram || forceVoronoiRegeneration)
    {
        if(!worldMode->generatingVoronoi)
        {
            GenerateVoronoi(gameState,worldMode, voronoiP, originChunkX, originChunkY, chunkApron, lateralChunkSpan);
        }
    }
    
    
    if(false)//UI->groundViewMode == GroundView_Tile || UI->groundViewMode == GroundView_Chunk)
    {
        
#if 0        
        for(u32 chunkIndex = 0; chunkIndex < ArrayCount(worldMode->chunks); ++chunkIndex)
        {
            WorldChunk* chunk = worldMode->chunks[chunkIndex];
            while(chunk)
            {
                if(chunk->initialized && !ChunkOutsideWorld(lateralChunkSpan, chunk->worldX, chunk->worldY))
                {
                    r32 chunkSide = worldMode->chunkSide;
                    
                    Vec3 chunkLowLeftCornerOffset = V3(V2i(chunk->worldX - originChunkX, chunk->worldY - originChunkY), 0.0f) * chunkSide - myPlayer->universeP.chunkOffset;
                    
                    if(UI->groundViewMode == GroundView_Chunk)
                    {
                        ObjectTransform chunkTransform = FlatTransform();
                        Rect2 rect = RectMinDim(chunkLowLeftCornerOffset.xy, V2(chunkSide, chunkSide));
                        
                        Vec4 chunkColor = ComputeWeightedChunkColor(chunk);
                        if(ChunkOutsideWorld(lateralChunkSpan, chunk->worldX, chunk->worldY))
                        {
                            chunkColor = V4(1, 0, 0, 1);
                        }
                        
                        
                        PushRect(group, chunkTransform, rect, chunkColor);
                        
                        if(UI->showGroundOutline)
                        {
                            ObjectTransform chunkOutlineTransform = FlatTransform(0.01f);
                            PushRectOutline(group, chunkTransform, rect, V4(1, 1, 1, 1), 0.1f);
                        }
                    }
                    else
                    {
                        for(u8 Y = 0; Y < CHUNK_DIM; ++Y)
                        {
                            for(u8 X = 0; X < CHUNK_DIM; ++X)
                            {
                                Vec3 tileMin = chunkLowLeftCornerOffset + V3(X * worldMode->voxelSide, Y * worldMode->voxelSide, 0);
                                ObjectTransform tileTransform = FlatTransform();
                                Rect2 rect = RectMinDim(tileMin.xy, V2(worldMode->voxelSide, worldMode->voxelSide));
                                
                                WorldTile* tile = GetTile(chunk, X, Y);
                                
                                RandomSequence seq = Seed(0);
                                Vec4 tileColor = GetTileColor(tile, UI->uniformGroundColors, &seq);
                                PushRect(group, tileTransform, rect, tileColor);
                                
                                if(UI->showGroundOutline)
                                {
                                    PushRectOutline(group, tileTransform, rect, V4(1, 1, 1, 1), 0.1f);
                                }
                                
                                
                                r32 waterLevel = tile->waterLevel;
                                if(waterLevel < WATER_LEVEL)
                                {
                                    Vec4 waterColor = V4(0, 0, waterLevel, 1 - waterLevel);
                                    PushRect(group, tileTransform, rect, waterColor);
                                }
                            }
                        }
                    }
                }
                
                chunk = chunk->next;
            }
        }
#endif
        
    }
    else
    {
        r32 rippleThreesold = 0.78f;
        r32 waterRandomPercentage = 0.002f;
        r32 ripplesLifetime = 3.0f;
        
        PrefetchAllGroundBitmaps(group->assets, true);
        
        for(u32 chunkIndex = 0; chunkIndex < ArrayCount(worldMode->chunks); ++chunkIndex)
        {
            WorldChunk* chunk = worldMode->chunks[chunkIndex];
            while(chunk)
            {
                i32 deltaChunkX = chunk->worldX - myPlayer->universeP.chunkX;
                i32 deltaChunkY = chunk->worldY - myPlayer->universeP.chunkY;
                i32 deltaChunk = Max(Abs(deltaChunkX), Abs(deltaChunkY));
                Assert(deltaChunk >= 0);
                
                
                if(deltaChunk <= chunkApron)
                {
                    Assert(chunk->initialized);
                }
                
                if(chunk->initialized && !ChunkOutsideWorld(lateralChunkSpan, chunk->worldX, chunk->worldY) &&
                   deltaChunk <= chunkApron)
                {
                    r32 chunkSide = worldMode->chunkSide;
                    
                    Vec3 chunkLowLeftCornerOffset = V3(V2i(chunk->worldX - originChunkX, chunk->worldY - originChunkY), 0.0f) * chunkSide - myPlayer->universeP.chunkOffset;
                    Vec3 chunkCenter = chunkLowLeftCornerOffset + 0.5f * V3(chunkSide, chunkSide, 0);
                    
                    {
                        if(IsValid(&chunk->textureHandle))
                        {
                            PushTexture(group, chunk->textureHandle, chunkLowLeftCornerOffset, V3(worldMode->chunkSide, 0, 0), V3(0, worldMode->chunkSide, 0), V4(1, 1, 1, 1), chunk->lights);
                            RefreshSpecialTexture(group->assets, &chunk->LRU);
                        }
                        else
                        {
                            u32 textureIndex = AcquireSpecialTextureHandle(group->assets);
                            chunk->textureHandle = TextureHandle(textureIndex, MAX_IMAGE_DIM, MAX_IMAGE_DIM);
                            RefreshSpecialTexture(group->assets, &chunk->LRU);
                            
                            RenderSetup lastSetup = group->lastSetup;
                            
                            SetCameraTransform(group, Camera_Orthographic, 0.0f, V3(2.0f / worldMode->chunkSide, 0.0f, 0.0f), V3(0.0f, 2.0f / worldMode->chunkSide, 0.0f), V3( 0, 0, 1), V3(0, 0, 0), V2(0, 0), textureIndex);
                            
                            for(u8 Y = 0; Y < CHUNK_DIM; ++Y)
                            {
                                for(u8 X = 0; X < CHUNK_DIM; ++X)
                                {
                                    Vec3 tileMin = -0.5f * V3(worldMode->chunkSide, worldMode->chunkSide, 0) + V3(X * worldMode->voxelSide, Y * worldMode->voxelSide, 0);
                                    
                                    Rect2 rect = RectMinDim(tileMin.xy, V2(worldMode->voxelSide, worldMode->voxelSide));
                                    
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
                                    
                                    
                                    PushRect4Colors(group, FlatTransform(), V3(GetCenter(rect), 0), V2(worldMode->voxelSide, worldMode->voxelSide), c0, c1, c2, c3, {});
                                }
                            }
                            
                            
                            for(i32 deltaY = -1; deltaY <= 1; ++deltaY)
                            {
                                for(i32 deltaX = -1; deltaX <= 1; ++deltaX)
                                {
                                    i32 chunkX = chunk->worldX + deltaX;
                                    i32 chunkY = chunk->worldY + deltaY;
                                    
                                    WorldChunk* splatChunk = GetChunk(worldMode->chunks, ArrayCount(worldMode->chunks), chunkX, chunkY, worldMode->persistentPool);
                                    if(!splatChunk->initialized)
                                    {
                                        BuildChunk(worldMode->table, worldMode->generator, splatChunk, chunkX, chunkY, seed);
                                    }
                                    
                                    Vec3 chunkLowLeftOffset = chunkSide * V3((r32) deltaX, (r32) deltaY, 0)  -0.5f * V3(worldMode->chunkSide, worldMode->chunkSide, 0);
                                    
                                    
                                    
                                    RandomSequence seq = Seed(splatChunk->worldX * splatChunk->worldY);
                                    for(u8 Y = 0; Y < CHUNK_DIM; ++Y)
                                    {
                                        for(u8 X = 0; X < CHUNK_DIM; ++X)
                                        {
                                            WorldTile* tile = GetTile(worldMode, splatChunk, X, Y);
                                            TaxonomySlot* slot = GetSlotForTaxonomy(worldMode->table, tile->taxonomy);
                                            
                                            Assert(slot->tileDefinition);
                                            TileDefinition* tileDef = slot->tileDefinition;
                                            if(tileDef->splashCount)
                                            {
                                                Vec3 tileOffset = worldMode->voxelSide * V3(X, Y, 0);
                                                for(u32 decorationIndex = 0; decorationIndex < tileDef->textureSplashCount; ++decorationIndex)
                                                {
                                                    ObjectTransform transform = FlatTransform();
                                                    Vec3 offset = V3(RandomBilV2(&seq) * Clamp01(tileDef->splashOffsetV) * worldMode->voxelSide, 0);
                                                    transform.angle = RandomUni(&seq) * tileDef->splashAngleV;
                                                    
                                                    
                                                    u64 nameHashID = 0;
                                                    r32 randomWeight = RandomUni(&seq) * tileDef->totalWeights;
                                                    r32 runningWeight = 0;
                                                    for(u32 splashIndex = 0; splashIndex < tileDef->splashCount; ++splashIndex)
                                                    {
                                                        TileSplash* splash = tileDef->splashes + splashIndex;
                                                        runningWeight += splash->weight;
                                                        
                                                        if(randomWeight < runningWeight)
                                                        {
                                                            nameHashID = splash->nameHash;
                                                        }
                                                    }
                                                    
                                                    BitmapId groundID = FindBitmapByName(group->assets, ASSET_GROUND, nameHashID);
                                                    if(IsValid(groundID))
                                                    {
                                                        Vec3 splatP = chunkLowLeftOffset + tileOffset + offset;
                                                        LockBitmapForCurrentFrame(group->assets, groundID);
                                                        
                                                        
                                                        r32 height = RandomRangeFloat(&seq, tileDef->splashMinScale, tileDef->splashMaxScale);
                                                        Vec2 scale = V2(RandomRangeFloat(&seq, 1.0f, 2.5f), RandomRangeFloat(&seq, 1.0f, 2.5f));
                                                        Vec4 color = V4(1, 1, 1, 1);
                                                        color.r += RandomBil(&seq) * 0.1f;
                                                        color.g += RandomBil(&seq) * 0.1f;
                                                        color.b += RandomBil(&seq) * 0.1f;
                                                        color = Clamp01(color);
                                                        
                                                        transform.angle = RandomUni(&seq) * TAU32;
                                                        if(!PushBitmap(group, transform, groundID, splatP, height * worldMode->voxelSide, scale, color))
                                                        {
                                                            InvalidCodePath;
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            
                            PushSetup(group, &lastSetup);
                        }
                    }
                }
                chunk = chunk->next;
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
                
                
                WorldTile* tile = site->tile;
                if(tile->waterLevel < rippleThreesold * WATER_LEVEL && RandomUni(&worldMode->waterRipplesSequence) < waterRandomPercentage)
                {
                    Vec3 ripplesP = V3(siteP + voronoi->deltaP.xy, tile->height);
                    //SpawnWaterRipples(particleCache, ripplesP, V3(0, 0, 0), ripplesLifetime);
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
                
                if(edge->sites[0]->tile->waterLevel < WATER_LEVEL)
                {
                    ++waterCounter;
                }
                
                if(!edge->tile[1])
                {
                    edge->tile[1] = GetTile(worldMode, voronoi->originP, offsetTo);
                }
                
                if(edge->sites[1]->tile->waterLevel < WATER_LEVEL)
                {
                    ++waterCounter;
                }
                
                
#if 0                
                if(UI->showGroundOutline)
                {
                    Vec4 offsetFromCamera = V4(offsetFrom + voronoi->deltaP.xy, edge->tile[0]->height, 0);
                    Vec4 offsetToCamera = V4(offsetTo + voronoi->deltaP.xy, edge->tile[1]->height, 0);
                    
                    PushLine(group, V4(1, 1, 1, 1), offsetFromCamera.xyz, offsetToCamera.xyz, 0.02f);
                }
#endif
                
                
                edge = edge->next;
                if(++counter == 512 || !edge)
                {
                    RenderVoronoiWork* work = PushStruct(worldMode->temporaryPool, RenderVoronoiWork);
                    work->group = group;
                    work->voronoi = voronoi;
                    work->edges = toRender;
                    work->edgeCount = counter;
                    // NOTE(Leonardo): 2 standard and 2 water
                    work->triangleVertexes = ReserveTriangles(group, waterCounter);
                    // NOTE(Leonardo): 2 standard
                    //work->quadVertexes = ReserveQuads(group, counter * 2);
                    
#if 1                                
                    platformAPI.PushWork(gameState->renderQueue, RenderVoronoiEdges, work);
#else
                    RenderVoronoiEdges(work);
#endif
                    
                    toRender = edge;
                    counter = 0;
                    waterCounter = 0;
                }
            }
            
            platformAPI.CompleteQueueWork(gameState->renderQueue);
            EndTemporaryMemory(voronoiMemory);
            END_BLOCK();
        }
    }
}
