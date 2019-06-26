struct GenerateVoronoiWork
{
    TaskWithMemory* task;
    u32 pointCount;
    jcv_point* points;
    jcv_rect rect;
    ForgVoronoiDiagram* diagram;
    ForgVoronoiDiagram** activeDiagramPtr;
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
        ForgVoronoiDiagram* diagram = worldMode->voronoiPingPong + 0;
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
    ForgVoronoiDiagram* voronoi;
    jcv_edge* edges;
    u32 edgeCount;
    
    ReservedVertexes triangleVertexes;
    ReservedVertexes quadVertexes;
};

PLATFORM_WORK_CALLBACK(RenderVoronoiEdges)
{
    RenderVoronoiWork* work = (RenderVoronoiWork*) param;
    
    RenderGroup* group = work->group;
    ForgVoronoiDiagram* voronoi = work->voronoi;
    
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
        
        Vec4 site0PCamera = V4(site0P + voronoi->deltaP.xy, QSite0->height, 0);
        Vec4 site1PCamera = V4(site1P + voronoi->deltaP.xy, QSite1->height, 0);
        
        Vec4 offsetFromCamera = V4(offsetFrom + voronoi->deltaP.xy, QFrom->height, 0);
        Vec4 offsetToCamera = V4(offsetTo + voronoi->deltaP.xy, QTo->height, 0);
        
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

