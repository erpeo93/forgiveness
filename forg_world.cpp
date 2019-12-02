#ifdef FORG_SERVER
inline WorldTile* GetTile(ServerState* server, WorldChunk* chunk, u32 tileX, u32 tileY)
{
    Assert(tileY < CHUNK_DIM);
    Assert(tileX < CHUNK_DIM);
    WorldTile* result = 0;
    if(chunk->tiles)
    {
        result = chunk->tiles + (tileY * CHUNK_DIM) + tileX;
    }
    else
    {
        result = &server->nullTile;
    }
    return result;
}
#else

inline WorldTile* GetTile(GameModeWorld* worldMode, WorldChunk* chunk, u32 tileX, u32 tileY)
{
    Assert(tileY < CHUNK_DIM);
    Assert(tileX < CHUNK_DIM);
    WorldTile* result = 0;
    if(chunk->tiles)
    {
        result = chunk->tiles + (tileY * CHUNK_DIM) + tileX;
    }
    else
    {
        result = &worldMode->nullTile;
    }
    
    return result;
}
#endif

inline r32 NormalizeCoordinate(r32 offset, i16* chunkIndex)
{
    r32 chunkSide = VOXEL_SIZE * (r32) CHUNK_DIM;
    r32 oneOverChunkSide = 1.0f / chunkSide;
    
    i16 chunkOffset = TruncateToI16(Floor(offset * oneOverChunkSide));
    *chunkIndex += chunkOffset;
    r32 computed = chunkOffset * chunkSide;
    r32 result = offset - computed;
    return result;
}

inline UniversePos NormalizePosition(UniversePos pos)
{
    UniversePos result = pos;
    result.chunkOffset.x = NormalizeCoordinate(result.chunkOffset.x, &result.chunkX);
    result.chunkOffset.y = NormalizeCoordinate(result.chunkOffset.y, &result.chunkY);
    
    return result;
}

inline UniversePos Offset(UniversePos pos, Vec3 offset)
{
    pos.chunkOffset.x += offset.x;
    pos.chunkOffset.y += offset.y;
    pos.chunkOffset.z += offset.z;
    UniversePos result = NormalizePosition(pos);
    
    return result;
    
}

inline b32 ChunkValid(i32 chunkX, i32 chunkY, i32 chunkZ)
{
    b32 result = true;
    if(chunkX < -WORLD_CHUNK_APRON || chunkY < -WORLD_CHUNK_APRON || chunkX >= (WORLD_CHUNK_SPAN + WORLD_CHUNK_APRON - 1) || chunkY > (WORLD_CHUNK_SPAN + WORLD_CHUNK_APRON - 1))
    {
        result = false;
    }
    
    return result;
}

inline b32 ChunkOutsideWorld(i32 chunkX, i32 chunkY, i32 chunkZ)
{
    b32 result = false;
    if(chunkX < 0 || chunkY < 0 || chunkX >= WORLD_CHUNK_SPAN || chunkY >= WORLD_CHUNK_SPAN)
    {
        result = true;
    }
    
    return result;
}

#ifndef FORG_SERVER
#endif

inline b32 PositionInsideWorld(UniversePos* pos)
{
    b32 result = (!ChunkOutsideWorld(pos->chunkX, pos->chunkY, pos->chunkZ));
    return result;
}

inline Vec3 SubtractOnSameZChunk(UniversePos A, UniversePos B)
{
    Vec3 result = V3(R32_MAX, R32_MAX, R32_MAX);
    if(A.chunkZ == B.chunkZ)
    {
        result = A.chunkOffset - B.chunkOffset;
        r32 chunkSide = VOXEL_SIZE * CHUNK_DIM;
        result += chunkSide * V3((r32) (A.chunkX - B.chunkX), (r32) (A.chunkY - B.chunkY), 0.0f);
    }
    
    return result;
}

#ifdef FORG_SERVER
#else
internal WorldChunk* GetChunk(GameModeWorld* worldMode, i32 worldX, i32 worldY, i32 worldZ)
{
    WorldChunk* result = 0;
    u32 chunkMask = ArrayCount(worldMode->chunks) - 1;
    i32 hashIndex = (worldX + worldY + worldZ) & chunkMask;
    WorldChunk* testChunk = worldMode->chunks[hashIndex];
    while(testChunk)
    {
        if(testChunk->worldX == worldX && 
           testChunk->worldY == worldY &&
           testChunk->worldZ == worldZ)
        {
            result = testChunk;
            break;
        }
        testChunk = testChunk->next;
    }
    
    return result;
}

internal WorldChunk* GetOrAllocateChunk(GameModeWorld* worldMode, i32 worldX, i32 worldY, i32 worldZ, b32* allocated)
{
    WorldChunk* result = GetChunk(worldMode, worldX, worldY, worldZ);
    if(!result)
    {
        u32 chunkMask = ArrayCount(worldMode->chunks) - 1;
        i32 hashIndex = (worldX + worldY + worldZ) & chunkMask;
        
        FREELIST_ALLOC(result, worldMode->firstFreeChunk, PushStruct(worldMode->persistentPool, WorldChunk));
        result->next = worldMode->chunks[hashIndex];
        worldMode->chunks[hashIndex] = result;
        *allocated = true;
    }
    
    return result;
}

struct GetUniversePosQuery
{
    WorldChunk* chunk;
    u32 tileX;
    u32 tileY;
    
    Vec2 chunkOffset;
};


GetUniversePosQuery TranslateRelativePos(GameModeWorld* worldMode, UniversePos baseP, Vec2 relativeP)
{
    GetUniversePosQuery result = {};
    
    baseP.chunkOffset.xy += relativeP;
    
    r32 chunkSide = CHUNK_SIDE;
    r32 oneOverChunkSide = 1.0f / chunkSide;
    
    r32 oneOverVoxelSide = 1.0f / VOXEL_SIZE;
    
    i16 chunkOffsetX = TruncateToI16(Floor(baseP.chunkOffset.x * oneOverChunkSide));
    baseP.chunkX += chunkOffsetX;
    baseP.chunkOffset.x -= chunkOffsetX * chunkSide;
    
    i16 chunkOffsetY = TruncateToI16(Floor(baseP.chunkOffset.y * oneOverChunkSide));
    baseP.chunkY += chunkOffsetY;
    baseP.chunkOffset.y -= chunkOffsetY * chunkSide;
    
    WorldChunk* chunk = GetChunk(worldMode, baseP.chunkX, baseP.chunkY, baseP.chunkZ);
	if(chunk)
    {
        result.tileX = TruncateReal32ToI32(baseP.chunkOffset.x * oneOverVoxelSide);
        result.tileY = TruncateReal32ToI32(baseP.chunkOffset.y * oneOverVoxelSide);
        
        result.chunkOffset = baseP.chunkOffset.xy;
        if(result.tileX < CHUNK_DIM && result.tileY < CHUNK_DIM)
        {
            result.chunk = chunk;
        }
    }
    
    
    return result;
}

inline WorldTile* GetTile(GameModeWorld* worldMode, UniversePos baseP, Vec2 P)
{
    WorldTile* result = 0;
    GetUniversePosQuery query = TranslateRelativePos(worldMode, baseP, P);
    if(query.chunk)
    {
        result = GetTile(worldMode, query.chunk, query.tileX, query.tileY);
    }
    return result;
}

inline WorldTile* GetTile(GameModeWorld* worldMode, WorldChunk* chunk, i32 tileX, i32 tileY)
{
    WorldTile* result = 0;
    
    i32 chunkX = chunk->worldX;
    i32 chunkY = chunk->worldY;
    i32 chunkZ = chunk->worldZ;
    
    if(tileX < 0)
    {
        Assert(tileX == -1);
        --chunkX;
        tileX = CHUNK_DIM - 1;
    }
    else if(tileX >= CHUNK_DIM)
    {
        Assert(tileX == CHUNK_DIM);
        ++chunkX;
        tileX = 0;
    }
    
    
    if(tileY < 0)
    {
        Assert(tileY == -1);
        --chunkY;
        tileY = CHUNK_DIM - 1;
    }
    else if(tileY >= CHUNK_DIM)
    {
        Assert(tileY == CHUNK_DIM);
        ++chunkY;
        tileY = 0;
    }
    
    Assert(tileX >= 0 && tileX < CHUNK_DIM);
    Assert(tileY >= 0 && tileY < CHUNK_DIM);
    
    if(ChunkValid(chunkX, chunkY, chunkZ))
    {
        WorldChunk* newChunk = GetChunk(worldMode, chunkX, chunkY, chunkZ);
        Assert(newChunk);
        result = GetTile(worldMode, newChunk, (u32) tileX, (u32) tileY);
        
    }
    return result;
}
#endif
