internal void UpdateGround(GameModeWorld* worldMode, RenderGroup* group, UniversePos origin)
{
    u32 worldSeed = worldMode->worldSeed;
    i16 originChunkX = origin.chunkX;
    i16 originChunkY = origin.chunkY;
    i16 originChunkZ = origin.chunkZ;
    i16 chunkApron = WORLD_CHUNK_APRON;
    r32 chunkSide = CHUNK_DIM * VOXEL_SIZE;
    r32 voxelSide = VOXEL_SIZE;
    
    for(u32 chunkIndex = 0; chunkIndex < ArrayCount(worldMode->chunks); ++chunkIndex)
    {
        WorldChunk** chunkPtr = &worldMode->chunks[chunkIndex];
        while(*chunkPtr)
        {
            WorldChunk* chunk = *chunkPtr;
            i16 deltaChunkX = (i16) Abs(chunk->worldX - originChunkX);
            i16 deltaChunkY = (i16) Abs(chunk->worldY - originChunkY);
            if(!IsValidSpecial(&chunk->texture) && 
               (chunk->worldZ != originChunkZ || 
                chunk->worldSeed != worldMode->worldSeed ||
                deltaChunkX > (chunkApron + 3) || 
                deltaChunkY > (chunkApron + 3))
               )
            {
                FreeSpecialTexture(group->assets, &chunk->texture);
                *chunkPtr = chunk->next;
                FREELIST_DEALLOC(chunk, worldMode->firstFreeChunk);
            }
            else
            {
                chunkPtr = &chunk->next;
            }
        }
    }
    
    for(i16 Y = originChunkY - chunkApron - 1; Y <= originChunkY + chunkApron + 1; Y++)
    {
        for(i16 X = originChunkX - chunkApron - 1; X <= originChunkX + chunkApron + 1; X++)
        {
            if(ChunkValid(X, Y, originChunkZ))
            {
                b32 allocated = false;
                WorldChunk* chunk = GetOrAllocateChunk(worldMode, X, Y, originChunkZ, &allocated);
                if(allocated)
                {
                    Assert(chunk->texture.textureHandle.width == 0);
                    Assert(chunk->texture.textureHandle.height == 0);
                    
                    RandomSequence generatorSeq = Seed(worldSeed);
                    GameProperties properties = {};
                    AssetID ID = QueryDataFiles(group->assets, world_generator, "default", &generatorSeq, &properties);
                    if(IsValid(ID))
                    {
                        world_generator* generator = GetData(group->assets, world_generator, ID);
                        BuildChunk(group->assets, worldMode->persistentPool, generator, 
                                   chunk, X, Y, originChunkZ, worldSeed);
                    }
                    else
                    {
                        InvalidCodePath;
                    }
                }
            }
        }
    }
    
    
    for(i16 chunkY = originChunkY - chunkApron;
        (chunkY <= originChunkY + chunkApron); 
        chunkY++)
    {
        for(i16 chunkX = originChunkX - chunkApron; 
            (chunkX <= originChunkX + chunkApron); 
            chunkX++)
        {
            if(ChunkValid(chunkX, chunkY, originChunkZ))
            {	
                WorldChunk* chunk = GetChunk(worldMode, chunkX, chunkY, originChunkZ);
                if(IsValidSpecial(&chunk->texture))
                {
                }
                else
                {
                    u32 textureIndex = chunk->texture.textureHandle.index;
                    if(!textureIndex)
                    {
                        textureIndex = AcquireSpecialTextureHandle(group->assets);
                    }
                    chunk->texture.textureHandle = TextureHandle(textureIndex, MAX_TEXTURE_DIM, MAX_TEXTURE_DIM);
                    Assert(IsValidSpecial(&chunk->texture));
                    SetOrthographicTransform(group, chunkSide, chunkSide, textureIndex);  
                    
                    for(i16 deltaY = -1; deltaY <= 1; ++deltaY)
                    {
                        for(i16 deltaX = -1; deltaX <= 1; ++deltaX)
                        {
                            i16 splatX = chunk->worldX + deltaX;
                            i16 splatY = chunk->worldY + deltaY;
                            
                            if(ChunkValid(splatX, splatY, originChunkZ))
                            {
                                WorldChunk* splatChunk = GetChunk(worldMode, splatX, splatY, originChunkZ);
                                Assert(splatChunk);
                                Vec3 chunkCenterOffset = chunkSide * V3((r32) deltaX, (r32) deltaY, 0);
                                
                                RandomSequence seq = GetChunkSeed(splatChunk->worldX, splatChunk->worldY, originChunkZ, worldSeed);
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
                                            r32 height = 2.0f * voxelSide;
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
                                            PushBitmap(group, transform, groundID, splatP, height);
                                        }
                                    }
                                }
                            }
                        }
                    }
                    
                    Assert(IsValidSpecial(&chunk->texture));
                    RefreshSpecialTexture(group->assets, &chunk->texture);
                }
            }
        }
    }
}

internal b32 GroundIsComplete(GameModeWorld* worldMode, UniversePos origin)
{
    b32 result = true;
    for(i16 Y = origin.chunkY - 1; Y <= origin.chunkY + 1; Y++)
    {
        for(i16 X = origin.chunkX - 1; X <= origin.chunkX + 1; X++)
        {
            if(ChunkValid(X, Y, origin.chunkZ))
            {
                WorldChunk* chunk = GetChunk(worldMode, X, Y, origin.chunkZ);
                if(!IsValidSpecial(&chunk->texture))
                {
                    result = false;
                }
            }
        }
    }
    
    return result;
}

inline void RenderGroundAndPlaySounds(GameModeWorld* worldMode, RenderGroup* group, UniversePos origin, r32 timeToAdvance)
{
    u32 worldSeed = worldMode->worldSeed;
    
    i16 originChunkX = origin.chunkX;
    i16 originChunkY = origin.chunkY;
    i16 originChunkZ = origin.chunkZ;
    i16 chunkApron = WORLD_CHUNK_APRON;
    
    r32 chunkSide = CHUNK_DIM * VOXEL_SIZE;
    r32 voxelSide = VOXEL_SIZE;
    
    for(i16 chunkY = originChunkY - chunkApron; 
        chunkY <= (originChunkY + chunkApron); 
        chunkY++)
    {
        for(i16 chunkX = originChunkX - chunkApron; 
            chunkX <= (originChunkX + chunkApron); 
            chunkX++)
        {
            if(ChunkValid(chunkX, chunkY, originChunkZ))
            {	
                WorldChunk* chunk = GetChunk(worldMode, chunkX, chunkY, originChunkZ);
                if(IsValidSpecial(&chunk->texture))
                {
                    Vec3 chunkLowLeftCornerOffset = V3(V2i(chunk->worldX - originChunkX, chunk->worldY - originChunkY), 0.0f) * chunkSide - origin.chunkOffset;
                    Vec3 chunkBaseCenterOffset = chunkLowLeftCornerOffset + 0.5f * V3(chunkSide, chunkSide, 0);
                    
                    UniversePos P = {};
                    P.chunkX = chunkX;
                    P.chunkY = chunkY;
                    P.chunkZ = worldMode->player.universeP.chunkZ;
                    P.chunkOffset = 0.5f * V3(CHUNK_SIDE, CHUNK_SIDE, 0);
                    Lights lights =  GetLights(worldMode, GetRelativeP(worldMode, P));
                    b32 flat = true;
                    PushTexture(group, chunk->texture.textureHandle, chunkLowLeftCornerOffset, flat, V3(chunkSide, 0, 0), V3(0, chunkSide, 0), V4(1, 1, 1, 1), lights, 0, 0, 1);
                    
                    RefreshSpecialTexture(group->assets, &chunk->texture);
                    if(worldMode->editorUI.renderChunkBounds)
                    {
                        PushRectOutline(group, FlatTransform(), RectMinDim(chunkLowLeftCornerOffset.xy, V2(CHUNK_SIDE, CHUNK_SIDE)), 0.1f);
                    }
                }
            }
        }
    }
    
    for(i32 Y = originChunkY - chunkApron; Y <= originChunkY + chunkApron; Y++)
    {
        for(i32 X = originChunkX - chunkApron; X <= originChunkX + chunkApron; X++)
        {
            if(ChunkValid(X, Y, originChunkZ))
            {
                WorldChunk* chunk = GetChunk(worldMode, X, Y, originChunkZ);
                Vec3 chunkLowLeftCornerOffset = V3(V2i(chunk->worldX - originChunkX, chunk->worldY - originChunkY), 0.0f) * chunkSide - origin.chunkOffset;
                Vec3 chunkBaseCenterOffset = chunkLowLeftCornerOffset + 0.5f * V3(chunkSide, chunkSide, 0);
                for(u8 tileY = 0; tileY < CHUNK_DIM; ++tileY)
                {
                    for(u8 tileX = 0; tileX < CHUNK_DIM; ++tileX)
                    {
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
                                
                                AssetID splashID = QueryBitmaps(group->assets, GetAssetSubtype(group->assets, effect->asset.type, effect->asset.subtypeHash), 0, &properties);
                                if(IsValid(splashID))
                                {
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
                                        Vec2 invUV = GetInvUV(bitmap->width, bitmap->height);
                                        
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
                                            Vec3 lateral = finalDim * V3(1, 0, 0);
                                            Vec3 up = finalDim * V3(0, 1, 0);
                                            b32 flat = true;
                                            PushMagicQuad(group, finalTileP, flat, lateral, up, invUV, finalColor, bitmap->textureHandle, lights, 0, 0, 1, {}, 0, {}, 0, 0);
                                            
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
    }
    
    for(i16 chunkY = originChunkY - chunkApron; 
        chunkY <= (originChunkY + chunkApron); 
        chunkY++)
    {
        for(i16 chunkX = originChunkX - chunkApron; 
            chunkX <= (originChunkX + chunkApron); 
            chunkX++)
        {
            if(ChunkValid(chunkX, chunkY, originChunkZ))
            {	
                WorldChunk* chunk = GetChunk(worldMode, chunkX, chunkY, originChunkZ);
                
                for(u8 tileY = 0; tileY < CHUNK_DIM; ++tileY)
                {
                    for(u8 tileX = 0; tileX < CHUNK_DIM; ++tileX)
                    {
                        WorldTile* tile = GetTile(worldMode, chunk, tileX, tileY);
                        for(u32 soundIndex = 0; soundIndex < tile->soundCount; ++soundIndex)
                        {
                            UpdateSoundMapping(worldMode, tile->sounds + soundIndex, timeToAdvance);
                        }
                    }
                }
                
            }
        }
    }
    
    ResetQuads(group);
}
