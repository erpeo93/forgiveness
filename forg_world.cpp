inline r32 NormalizeCoordinate(r32 offset, r32 chunkSide, r32 oneOverChunkSide, i32* chunkIndex)
{
    i32 chunkOffset = Floor(offset * oneOverChunkSide);
    *chunkIndex += chunkOffset;
    r32 result = offset - chunkOffset * chunkSide;
    return result;
}

inline UniversePos NormalizePosition(UniversePos pos, r32 chunkSide, r32 oneOverChunkSide)
{
    UniversePos result = pos;
    result.chunkOffset.x = NormalizeCoordinate(result.chunkOffset.x, chunkSide, oneOverChunkSide, &result.chunkX);
    result.chunkOffset.y = NormalizeCoordinate(result.chunkOffset.y, chunkSide, oneOverChunkSide, &result.chunkY);
    
    return result;
}

inline UniversePos Offset(UniversePos pos, Vec2 offset)
{
    r32 chunkSide = VOXEL_SIZE * CHUNK_DIM;
    
    pos.chunkOffset.x += offset.x;
    pos.chunkOffset.y += offset.y;
    UniversePos result = NormalizePosition(pos, chunkSide, 1.0f / chunkSide);
    
    return result;
    
}

inline b32 ChunkValid(i32 lateralChunkSpan, i32 chunkX, i32 chunkY)
{
    b32 result = true;
    
    i32 offset = 0;
    i32 max = lateralChunkSpan;
    
    offset -= SIM_REGION_CHUNK_SPAN;
    max += SIM_REGION_CHUNK_SPAN;
    
    if(chunkX < offset || chunkX >= max || chunkY < offset || chunkY >= max)
    {
        result = false;
    }
    
    return result;
}

inline b32 ChunkOutsideWorld(i32 lateralChunkSpan, i32 chunkX, i32 chunkY)
{
    b32 result = false;
    if(chunkX < 0 || chunkY < 0 || chunkX >= lateralChunkSpan || chunkY >= lateralChunkSpan)
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

inline b32 PositionInsideWorld(i32 lateralChunkSpan, UniversePos* pos)
{
    b32 result = (!ChunkOutsideWorld(lateralChunkSpan, pos->chunkX, pos->chunkY));
    return result;
}

inline Vec3 Subtract(UniversePos A, UniversePos B)
{
    Vec3 result = A.chunkOffset - B.chunkOffset;
    
    r32 chunkSide = VOXEL_SIZE * CHUNK_DIM;
    result += chunkSide * V3((r32) (A.chunkX - B.chunkX), (r32) (A.chunkY - B.chunkY), 0.0f);
    
    return result;
}


