inline void UpdateAndRenderGround(GameModeWorld* worldMode, RenderGroup* group, UniversePos origin, UniversePos oldOrigin, r32 timeToAdvance)
{
    u32 worldSeed = worldMode->worldSeed;
    
    i32 originChunkX = origin.chunkX;
    i32 originChunkY = origin.chunkY;
    i32 Z = origin.chunkZ;
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
    for(i32 Y = originChunkY - chunkApron - 1; Y <= originChunkY + chunkApron + 1; Y++)
    {
        for(i32 X = originChunkX - chunkApron - 1; X <= originChunkX + chunkApron + 1; X++)
        {
            if(ChunkValid(X, Y, Z))
            {
                WorldChunk* chunk = GetChunk(worldMode->chunks, ArrayCount(worldMode->chunks), X, Y, Z, worldMode->persistentPool);
                if(!chunk->initialized)
                {
                    Assert(chunk->texture.textureHandle.width == 0);
                    Assert(chunk->texture.textureHandle.height == 0);
                    
                    RandomSequence generatorSeq = Seed(worldSeed);
                    GameProperties properties = {};
                    AssetID ID = QueryDataFiles(group->assets, world_generator, "default", &generatorSeq, &properties);
                    if(IsValid(ID))
                    {
                        world_generator* generator = GetData(group->assets, world_generator, ID);
                        worldMode->nullTile = NullTile(generator);
                        
                        BuildChunk(group->assets, worldMode->persistentPool, generator, 
                                   chunk, X, Y, Z, worldSeed);
                    }
                    
                }
            }
        }
    }
    
    b32 generatedTextureThisFrame = false;
    for(i32 chunkY = originChunkY - chunkApron; (chunkY <= originChunkY + chunkApron) && !generatedTextureThisFrame; chunkY++)
    {
        for(i32 chunkX = originChunkX - chunkApron; 
            (chunkX <= originChunkX + chunkApron) && !generatedTextureThisFrame; chunkX++)
        {
            if(ChunkValid(chunkX, chunkY, Z))
            {	
                WorldChunk* chunk = GetChunk(worldMode->chunks, ArrayCount(worldMode->chunks), chunkX, chunkY, Z, worldMode->persistentPool);
                if(chunk->initialized)
                {
                    Vec3 chunkLowLeftCornerOffset = V3(V2i(chunk->worldX - originChunkX, chunk->worldY - originChunkY), 0.0f) * chunkSide - origin.chunkOffset;
                    Vec3 chunkBaseCenterOffset = chunkLowLeftCornerOffset + 0.5f * V3(chunkSide, chunkSide, 0);
                    
                    if(worldMode->worldChunkView)
                    {
                        
#if 0                        
                        Vec4 averageColor = {};
                        u32 waterCount = 0;
                        Vec4 waterColor = {};
                        
                        for(u8 Y = 0; Y < CHUNK_DIM; ++Y)
                        {
                            for(u8 X = 0; X < CHUNK_DIM; ++X)
                            {
                                WorldTile* tile = GetTile(worldMode, chunk, X, Y);
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
#endif
                        
                        
                    }
                    else if(worldMode->worldTileView)
                    {
                        
#if 0                        
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
                            }
                        }
#endif
                        
                    }
                    else
                    {
                        if(IsValidSpecial(&chunk->texture))
                        {
                            UniversePos P = {};
                            P.chunkX = chunkX;
                            P.chunkY = chunkY;
                            P.chunkZ = worldMode->player.universeP.chunkZ;
                            P.chunkOffset = 0.5f * V3(CHUNK_SIDE, CHUNK_SIDE, 0);
                            Lights lights =  GetLights(worldMode, GetRelativeP(worldMode, P));
                            PushTexture(group, chunk->texture.textureHandle, chunkLowLeftCornerOffset, V3(chunkSide, 0, 0), V3(0, chunkSide, 0), V4(1, 1, 1, 1), lights, 0, 0, 1, 0);
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
                                    
#if 0                                    
                                    Vec4 c0 = BlendTilesColor(sTiles[0], sTiles[1], sTiles[3], sTiles[4]);
                                    Vec4 c1 = BlendTilesColor(sTiles[1], sTiles[2], sTiles[4], sTiles[5]);
                                    Vec4 c2 = BlendTilesColor(sTiles[4], sTiles[5], sTiles[7], sTiles[8]);
                                    Vec4 c3 = BlendTilesColor(sTiles[3], sTiles[4], sTiles[6], sTiles[7]);
                                    
                                    PushRect4Colors(group, FlatTransform(), V3(tileCenter, 0), tileDim, 
                                                    c0, c1, c2, c3, {});
#endif
                                    
                                }
                            }
                            
                            for(i32 deltaY = -1; deltaY <= 1; ++deltaY)
                            {
                                for(i32 deltaX = -1; deltaX <= 1; ++deltaX)
                                {
                                    i32 splatX = chunk->worldX + deltaX;
                                    i32 splatY = chunk->worldY + deltaY;
                                    
                                    if(ChunkValid(splatX, splatY, Z))
                                    {
                                        WorldChunk* splatChunk = GetChunk(worldMode->chunks, ArrayCount(worldMode->chunks), splatX, splatY, Z, 0);
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
                                                
                                                WorldTile* tile = GetTile(worldMode, splatChunk, X, Y);
                                                
                                                GameProperties properties = {};
                                                properties.properties[0] = tile->property;
                                                
                                                u32 subtype = GetAssetSubtype(group->assets, AssetType_Image, tile->asset.subtypeHash);
                                                BitmapId groundID = QueryBitmaps(group->assets, subtype, &seq, &properties);
                                                
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
                                                    transform.scale = scale;
                                                    transform.tint = color;
                                                    PushBitmap(group, transform, groundID, splatP, height * voxelSide);
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            PushSetup_(group, &lastSetup);
                            //generatedTextureThisFrame = true;
                        }
                        
                        Assert(IsValidSpecial(&chunk->texture));
                        RefreshSpecialTexture(group->assets, &chunk->texture);
                    }
                }
            }
        }
    }
    
    
    
    
    
#if 1
    for(u8 tileY = 0; tileY < CHUNK_DIM; ++tileY)
    {
        for(u8 tileX = 0; tileX < CHUNK_DIM; ++tileX)
        {
            for(i32 Y = originChunkY - chunkApron; Y <= originChunkY + chunkApron; Y++)
            {
                for(i32 X = originChunkX - chunkApron; X <= originChunkX + chunkApron; X++)
                {
                    if(ChunkValid(X, Y, Z))
                    {
                        WorldChunk* chunk = GetChunk(worldMode->chunks, ArrayCount(worldMode->chunks), X, Y, Z, worldMode->persistentPool);
                        Assert(chunk->initialized);
                        
                        Vec3 chunkLowLeftCornerOffset = V3(V2i(chunk->worldX - originChunkX, chunk->worldY - originChunkY), 0.0f) * chunkSide - origin.chunkOffset;
                        Vec3 chunkBaseCenterOffset = chunkLowLeftCornerOffset + 0.5f * V3(chunkSide, chunkSide, 0);
                        WorldTile* tile = GetTile(worldMode, chunk, tileX, tileY);
                        
                        
                        Vec3 tileMin = -0.5f * V3(chunkSide, chunkSide, 0) + V3(tileX * voxelSide, tileY * voxelSide, 0);
                        Vec2 tileDim = V2(voxelSide, voxelSide);
                        Rect2 tileSurface = RectMinDim(tileMin.xy, tileDim);
                        Vec2 tileCenter = GetCenter(tileSurface);
                        Lights lights = chunk->lights;
                        
                        GameProperties properties = {};
                        if(IsValid(tile->underSeaLevelFluid))
                        {
                            AddGamePropertyRaw(&properties, tile->underSeaLevelFluid);
                            AssetID ID = QueryDataFiles(group->assets, TileAnimationEffect, "default", 0, &properties);
                            if(IsValid(ID))
                            {
                                TileAnimationEffect* effect = GetData(group->assets, TileAnimationEffect, ID);
                                Assert(effect);
                                AssetID splashID = QueryBitmaps(group->assets, GetAssetSubtype(group->assets, effect->asset.type, effect->asset.subtypeHash), 0, 0);
                                Bitmap* bitmap = GetBitmap(group->assets, splashID).bitmap;
                                if(bitmap)
                                {
                                    Vec3 maxOffset = effect->maxOffset;
                                    Vec4 colorRef = effect->color;
                                    Vec4 colorV = effect->colorV;
                                    r32 scaleRef = effect->scale;
                                    r32 scaleV = effect->scaleV;
                                    r32 speed = effect->sineSpeed;
                                    u32 patchCount = effect->patchCount;
                                    r32 baseZBias = (effect->tileZBias * (tileY * CHUNK_DIM + tileX));
                                    r32 zBias = effect->patchZBias;
                                    r32 advanceTime = speed * timeToAdvance;
                                    
                                    for(u32 patchIndex = 0; patchIndex < patchCount; ++patchIndex)
                                    {
                                        TilePatch* patch = tile->patches + patchIndex;
                                        patch->offsetTime += advanceTime;
                                        patch->colorTime += advanceTime;
                                        patch->scaleTime += advanceTime;
                                        
                                        Vec3 offset = Sin(patch->offsetTime) * maxOffset;
                                        Vec4 color = colorRef + Sin(patch->colorTime) * colorV;
                                        color = Clamp01(color);
                                        r32 scale = scaleRef + Sin(patch->scaleTime) * scaleV;
                                        
                                        Vec3 finalTileP = chunkBaseCenterOffset + V3(tileCenter, baseZBias + zBias) + offset;
                                        u32 finalColor = RGBAPack8x4(color * 255.0f);
                                        r32 finalDim = 0.5f * tileDim.y * scale;
                                        Vec4 lateral = finalDim * V4(1, 0, 0, 0);
                                        Vec4 up = finalDim * V4(0, 1, 0, 0);
                                        
                                        PushMagicQuad(group, V4(finalTileP, 0), lateral, up, finalColor, bitmap->textureHandle, lights, 0, 0, 1, {}, 0, {}, 0);
                                        
                                        zBias += effect->patchZBias;
                                    }
                                }
                                else
                                {
                                    LoadBitmap(group->assets, splashID);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
#endif
    
}
