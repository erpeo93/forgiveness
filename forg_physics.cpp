internal void InitSpatialPartition(MemoryPool* pool, SpatialPartition* partition)
{
	partition->width = WORLD_CHUNK_SPAN;
	partition->height = WORLD_CHUNK_SPAN;
	partition->cellDim = CHUNK_DIM;
    
	partition->chunks = PushArray(pool, SpatialPartitionChunk, partition->width * partition->height);
}

internal SpatialPartitionChunk* GetPartitionChunk(SpatialPartition* partition, i32 X, i32 Y)
{
    Assert(X >= 0 && X < partition->width);
    Assert(Y >= 0 && Y < partition->height);
    
    SpatialPartitionChunk* result = partition->chunks + (Y * partition->width) + X;
    return result;
}

struct AddToPartitionResult
{
    SpatialPartitionEntityBlock* block;
};

internal AddToPartitionResult AddToSpatialPartition(MemoryPool* pool, SpatialPartition* partition, UniversePos P, Rect3 bounds, EntityID ID)
{
    AddToPartitionResult result = {};
    
    Assert(IsValidID(ID));
    
	P.chunkOffset += GetCenter(bounds);
    
    UniversePos min = P;
    min.chunkOffset -= 0.5f * GetDim(bounds);
    min = NormalizePosition(min);
    
    min.chunkX = Max(0, min.chunkX);
    min.chunkY = Max(0, min.chunkY);
    
    UniversePos max = P;
    max.chunkOffset += 0.5f * GetDim(bounds);
    max = NormalizePosition(max);
    
    max.chunkX = Min(WORLD_CHUNK_SPAN - 1, max.chunkX);
    max.chunkY = Min(WORLD_CHUNK_SPAN - 1, max.chunkY);
    
	for(i32 chunkY = min.chunkY; chunkY <= max.chunkY; ++chunkY)
	{
		for(i32 chunkX = min.chunkX; chunkX <= max.chunkX; ++chunkX)
		{
			SpatialPartitionChunk* chunk = GetPartitionChunk(partition, chunkX, chunkY);
            ++chunk->totalCount;
            
            SpatialPartitionEntityBlock* block = 0;
            for(SpatialPartitionEntityBlock* freeBlock = chunk->entities; freeBlock; freeBlock = freeBlock->next)
            {
                if(freeBlock->count < ArrayCount(freeBlock->IDs))
                {
                    block = freeBlock;
                    break;
                }
            }
            
            if(!block)
            {
                block = PushStruct(pool, SpatialPartitionEntityBlock, NoClear());
                block->count = 0;
                block->next = chunk->entities;
                chunk->entities = block;
            }
            
            result.block = block;
            block->IDs[block->count++] = ID;
		}
	}
    
    return result;
}

internal void RemoveFromSpatialPartition(SpatialPartition* partition, SpatialPartitionChunk* chunk, SpatialPartitionEntityBlock* block, EntityID ID)
{
    Assert(chunk);
    Assert(block);
    
    b32 found = false;
    for(u32 entityIndex = 0; entityIndex < block->count; ++entityIndex)
    {
        if(AreEqual(block->IDs[entityIndex], ID))
        {
            block->IDs[entityIndex] = block->IDs[--block->count];
            found = true;
            break;
        }
    }
    
	--chunk->totalCount;
    Assert(found);
}

internal SpatialPartitionQuery QuerySpatialPartition(SpatialPartition* partition, UniversePos P, Rect3 bounds)
{
	SpatialPartitionQuery result = {};
    
	P.chunkOffset += GetCenter(bounds);
    
    UniversePos min = P;
    min.chunkOffset -= 0.5f * GetDim(bounds);
    min = NormalizePosition(min);
    
    UniversePos max = P;
    max.chunkOffset += 0.5f * GetDim(bounds);
    max = NormalizePosition(max);
    
    result.partition = partition;
    
	result.minX = Max(0, min.chunkX);
	result.minY = Max(0, min.chunkY);
    
	result.maxX = Min(WORLD_CHUNK_SPAN - 1, max.chunkX);
	result.maxY = Min(WORLD_CHUNK_SPAN - 1, max.chunkY);
    
	result.currentX = result.minX;
	result.currentY = result.minY;
	result.currentIndex = 0;
    
    return result;
}

internal SpatialPartitionQuery QuerySpatialPartitionAtPoint(SpatialPartition* partition, UniversePos P)
{
    SpatialPartitionQuery result = QuerySpatialPartition(partition, P, {});
    return result;
}

internal b32 IsValid(SpatialPartitionQuery* query)
{
    b32 result = (query->currentX >= query->minX && 
                  query->currentY >= query->minY &&
                  query->currentX <= query->maxX && 
                  query->currentY <= query->maxY);
    return result;
}

internal void AdvanceInternal(SpatialPartitionQuery* query)
{
	Assert(query->currentX >= query->minX && query->currentX <= query->maxX);
    Assert(query->currentY >= query->minY && query->currentY <= query->maxY);
    
	SpatialPartitionChunk* chunk = GetPartitionChunk(query->partition, query->currentX, query->currentY);
    
    if(++query->currentIndex >= chunk->totalCount)
    {
        query->currentIndex = 0;
        if(++query->currentX > query->maxX)
        {
            query->currentX = query->minX;
            ++query->currentY;
        }
    }
}

internal EntityID GetCurrent(SpatialPartitionQuery* query)
{
	EntityID result = {};
    
    SpatialPartitionChunk* chunk = 0;
    while(!chunk && IsValid(query))
    {
        SpatialPartitionChunk* test = GetPartitionChunk(query->partition, query->currentX, query->currentY);
        Assert(test);
        if(test->totalCount)
        {
            chunk = test;
            break;
        }
        AdvanceInternal(query);
    }
    
    if(chunk)
    {
        Assert(chunk->totalCount);
        u32 runningIndex = 0;
        SpatialPartitionEntityBlock* block = chunk->entities;
        while(true)
        {
            if(query->currentIndex >= runningIndex && query->currentIndex < (runningIndex + block->count))
            {
                u32 index = query->currentIndex - runningIndex;
                result = block->IDs[index];
                break;
            }
            runningIndex += block->count;
            block = block->next;
        }
    }
    
    return result;
}

internal EntityID Advance(SpatialPartitionQuery* query)
{
    AdvanceInternal(query);
    EntityID result = GetCurrent(query);
    
    return result;
}

internal void PointCubeCollision(Vec3 P, Vec3 deltaP, Rect3 minkowski, r32* tMin, Vec3* tMinNormal)
{
    r32 deltaX = deltaP.x;
    r32 deltaY = deltaP.y;
    r32 deltaZ = deltaP.z;
    
    Vec3 minCorner = minkowski.min;
    Vec3 maxCorner = minkowski.max;
    
    Wall walls[] = { 
        {maxCorner.x, 
            deltaX, deltaY, deltaZ, 
            P.x, P.y, P.z,
            minCorner.y, maxCorner.y, 
            minCorner.z, maxCorner.z, V3(1, 0, 0)},
        
        {minCorner.x, 
            deltaX, deltaY, deltaZ, 
            P.x, P.y, P.z,
            minCorner.y, maxCorner.y, 
            minCorner.z, maxCorner.z, V3(-1, 0, 0)},
        
        {maxCorner.y, 
            deltaY, deltaX, deltaZ, 
            P.y, P.x, P.z,
            minCorner.x, maxCorner.x, 
            minCorner.z, maxCorner.z, V3(0, 1, 0)},
        {minCorner.y, 
            deltaY, deltaX, deltaZ, 
            P.y, P.x, P.z,
            minCorner.x, maxCorner.x, 
            minCorner.z, maxCorner.z, V3(0, -1, 0)}
#if 0        
        ,
        {maxCorner.z, 
            deltaZ, deltaX, deltaY, 
            P.z, P.x, P.y,
            minCorner.x, maxCorner.x, 
            minCorner.y, maxCorner.y, V3(0, 0, 1)},
        {minCorner.z, 
            deltaZ, deltaX, deltaY, 
            P.z, P.x, P.y,
            minCorner.x, maxCorner.x, 
            minCorner.y, maxCorner.y, V3(0, 0, -1)}
#endif
        
    };
    
    r32 epsilon = 0.01f;
    for(u32 wallIndex = 0; wallIndex < ArrayCount(walls); wallIndex++)
    {
        Wall* wall = walls + wallIndex;
        if(wall->deltax != 0)
        {
            r32 newT = (wall->x - wall->px) / wall->deltax;
            if(newT >= 0)
            {
                newT = Max(newT - epsilon, 0);
                if(newT < *tMin)
                {
                    r32 y = wall->py + newT * wall->deltay;
                    r32 Z = wall->pz + newT * wall->deltaz;
                    if(y >= wall->miny && y <= wall->maxy &&
                       Z >= wall->minz && Z <= wall->maxz)
                    {
                        *tMinNormal = wall->normal;
                        *tMin = newT;
                    }
                }
            }
        }
    }
}

internal Rect3 ComputeMinkowski(Vec3 P, Rect3 bounds, Rect3 testBounds)
{
    Vec3 minkowskiCenter = P + GetCenter(testBounds);
    Vec3 minkowskiDimension =  GetDim(bounds) + GetDim(testBounds);
    Rect3 minkowski = RectCenterDim(minkowskiCenter, minkowskiDimension);
    
    return minkowski;
}

internal b32 HandleVolumeCollision(Rect3 bounds, Vec3 deltaP, Vec3 testP, Rect3 testBounds, r32* tMin, Vec3* wallNormalMin, b32 shouldCollide)
{
    b32 result = false;
    
    Vec3 fakeDelta = GetCenter(bounds);
    Vec3 myP = fakeDelta;
    deltaP -= fakeDelta;
    
    Rect3 minkowski = ComputeMinkowski(testP, bounds, testBounds);
    if(PointInRect(minkowski, myP))
    {
        result = true;
    }
    else
    {
        if(shouldCollide)
        {
            PointCubeCollision(myP, deltaP, minkowski, tMin, wallNormalMin);
        }
    }
    
    return result;
}
