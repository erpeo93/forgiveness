inline WorldTile* GetTile(WorldChunk* chunk, u32 tileX, u32 tileY)
{
    Assert(tileY < CHUNK_DIM);
    Assert(tileX < CHUNK_DIM);
    
    WorldTile* result = &chunk->tiles[tileY][tileX];
    
    return result;
}

inline r32 NormalizeCoordinate(r32 offset, r32 chunkSide, r32 oneOverChunkSide, i32* chunkIndex)
{
    i32 chunkOffset = Floor(offset * oneOverChunkSide);
    *chunkIndex += chunkOffset;
    r32 result = offset - chunkOffset * chunkSide;
    return result;
}

inline UniversePos NormalizePosition(UniversePos pos)
{
    r32 chunkSide = VOXEL_SIZE * CHUNK_DIM;
    r32 oneOverChunkSide = 1.0f / chunkSide;
    
    UniversePos result = pos;
    result.chunkOffset.x = NormalizeCoordinate(result.chunkOffset.x, chunkSide, oneOverChunkSide, &result.chunkX);
    result.chunkOffset.y = NormalizeCoordinate(result.chunkOffset.y, chunkSide, oneOverChunkSide, &result.chunkY);
    
    return result;
}

inline UniversePos Offset(UniversePos pos, Vec2 offset)
{
    pos.chunkOffset.x += offset.x;
    pos.chunkOffset.y += offset.y;
    UniversePos result = NormalizePosition(pos);
    
    return result;
    
}

inline b32 ChunkOutsideWorld(i32 chunkX, i32 chunkY)
{
    b32 result = false;
    if(chunkX < 0 || chunkY < 0 || chunkX >= WORLD_CHUNK_SPAN || chunkY >= WORLD_CHUNK_SPAN)
    {
        result = true;
    }
    
    return result;
}

internal WorldChunk* GetChunk(WorldChunk** chunks, u32 chunkCount, i32 worldX, i32 worldY, MemoryPool* pool)
{
    WorldChunk* result = 0;
    u32 chunkMask = chunkCount - 1;
    i32 hashIndex = ( worldX + worldY ) & chunkMask;
    WorldChunk** testChunkPtr = chunks + hashIndex;
    while( *testChunkPtr )
    {
        WorldChunk* testChunk = *testChunkPtr;
        if( testChunk->worldX == worldX && testChunk->worldY == worldY )
        {
            result = testChunk;
            break;
        }
        testChunkPtr = &testChunk->next;
    }
    
    if( !result )
    {
        if(pool)
        {
            *testChunkPtr = PushStruct( pool, WorldChunk );
            result = *testChunkPtr;
        }
    }
    
    return result;
}

inline b32 PositionInsideWorld(UniversePos* pos)
{
    b32 result = (!ChunkOutsideWorld(pos->chunkX, pos->chunkY));
    return result;
}

inline Vec3 Subtract(UniversePos A, UniversePos B)
{
    Vec3 result = A.chunkOffset - B.chunkOffset;
    
    r32 chunkSide = VOXEL_SIZE * CHUNK_DIM;
    result += chunkSide * V3((r32) (A.chunkX - B.chunkX), (r32) (A.chunkY - B.chunkY), 0.0f);
    
    return result;
}

#ifdef FORG_SERVER
#else

#if 0
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
    
    i32 chunkOffsetX = Floor(baseP.chunkOffset.x * worldMode->oneOverChunkSide);
    baseP.chunkX += chunkOffsetX;
    baseP.chunkOffset.x -= chunkOffsetX * worldMode->chunkSide;
    
    i32 chunkOffsetY = Floor(baseP.chunkOffset.y * worldMode->oneOverChunkSide);
    baseP.chunkY += chunkOffsetY;
    baseP.chunkOffset.y -= chunkOffsetY * worldMode->chunkSide;
    
    WorldChunk* chunk = GetChunk(worldMode->chunks, ArrayCount(worldMode->chunks), baseP.chunkX, baseP.chunkY, 0);
	if(chunk)
    {
        result.tileX = TruncateReal32ToI32(baseP.chunkOffset.x * worldMode->oneOverVoxelSide);
        result.tileY = TruncateReal32ToI32(baseP.chunkOffset.y * worldMode->oneOverVoxelSide);
        
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
    WorldTile* result = &worldMode->nullTile;
    GetUniversePosQuery query = TranslateRelativePos(worldMode, baseP, P);
    if(query.chunk)
    {
        result = GetTile(query.chunk, query.tileX, query.tileY);
    }
    return result;
}

inline WorldTile* GetTile(GameModeWorld* worldMode, WorldChunk* chunk, i32 tileX, i32 tileY)
{
    WorldTile* result = 0;
    
    i32 chunkX = chunk->worldX;
    i32 chunkY = chunk->worldY;
    
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
    
    
    i32 lateralChunkSpan = WORLD_CHUNK_SPAN;
    chunkX = Wrap(0, chunkX, lateralChunkSpan);
    chunkY = Wrap(0, chunkY, lateralChunkSpan);
    
    
    WorldChunk* newChunk = GetChunk(worldMode->chunks, ArrayCount(worldMode->chunks), chunkX, chunkY, worldMode->persistentPool);
    if(!newChunk->initialized)
    {
        InvalidCodePath;
    }
    
    result = GetTile(newChunk, (u32) tileX, (u32) tileY);
    
    return result;
}
#endif

#endif
