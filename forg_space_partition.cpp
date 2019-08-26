
struct CollisionData
{
    u32 entityIndex;
	b32 insideRegion;
    r32 radiousSq;
    Vec3 P;
};

struct PartitionSurfaceEntityBlock
{
    u32 entityCount;
    CollisionData colliders[32];
    PartitionSurfaceEntityBlock* next;
};

struct RegionPartitionSurface
{
    PartitionSurfaceEntityBlock* first;
};

struct RegionPartitionQueryResult
{
    u32 surfaceIndexes[64];
};

struct SpacePartition
{
    r32 gridDim;
    r32 oneOverSurfaceDim;
    u32 sideSquareCount;
    RegionPartitionSurface* partitionSurfaces;
};

inline void InitSpacePartition(SpacePartition* partition, MemoryPool* pool, r32 gridDim, u32 partitionSideCount)
{
    partition->sideSquareCount = partitionSideCount;
    partition->partitionSurfaces = PushArray(pool, RegionPartitionSurface, Squarei(partition->sideSquareCount));
    r32 surfaceDim = gridDim / partition->sideSquareCount;
    partition->oneOverSurfaceDim = 1.0f / surfaceDim;
    partition->gridDim = gridDim;
}


struct OverlappingSurfaceResult
{
    u32 X;
    u32 Y;
};

inline OverlappingSurfaceResult OverlappingSurfaceIndex(SimRegion* region, SpacePartition* partition, Vec3 P)
{
    OverlappingSurfaceResult result = {};
    r32 halfGridDim = 0.5f * partition->gridDim;
    result.X = TruncateReal32ToU32((P.x + halfGridDim) * partition->oneOverSurfaceDim);
    result.Y = TruncateReal32ToU32((P.y + halfGridDim) * partition->oneOverSurfaceDim);
    return result;
}

inline u32 GetOverlappingSurfaces(SimRegion* region, SpacePartition* partition, Vec3 P, Vec3 deltaP, Rect3 bounds, u32* outputSpaceIndexes, u32 maxOutputCount)
{
    Rect3 boundsHere = Offset(bounds, P);
    Rect3 boundsDelta = Offset(boundsHere, deltaP);
    
    Vec3 minDelta = bounds.min;
    Vec3 maxDelta = bounds.max;
    
    Vec3 points[4];
    points[0] = boundsHere.min;
    points[1] = boundsHere.max;
    points[2] = boundsDelta.min;
    points[3] = boundsDelta.max;
    
    u32 minX = U32_MAX;
    u32 minY = U32_MAX;
    u32 maxX = 0;
    u32 maxY = 0;
    
    for(u32 pIndex = 0; pIndex < ArrayCount(points); ++pIndex)
    {
        OverlappingSurfaceResult surface = OverlappingSurfaceIndex(region, partition, points[pIndex]);
        minX = Min(minX, surface.X);
        minY = Min(minY, surface.Y);
        
        maxX = Max(maxX, surface.X);
        maxY = Max(maxY, surface.Y);
    }
    
    maxX = Min(maxX, partition->sideSquareCount - 1);
    maxY = Min(maxY, partition->sideSquareCount - 1);
    
    u32 outputCount = 0;
    for(u32 Y = minY; Y <= maxY; ++Y)
    {
        for(u32 X = minX; X <= maxX; ++X)
        {
            Assert(outputCount < maxOutputCount);
            u32 surfaceIndex = (Y * partition->sideSquareCount) + X;
            outputSpaceIndexes[outputCount++] = surfaceIndex;
        }
    }
    
    return outputCount;
}

inline void AddToPartitionBlock(MemoryPool* pool, PartitionSurfaceEntityBlock** firstPtr, PartitionSurfaceEntityBlock* block, CollisionData collider)
{
    if(!block || (block->entityCount == ArrayCount(block->colliders)))
    {
        PartitionSurfaceEntityBlock* newBlock = PushStruct(pool, PartitionSurfaceEntityBlock);
        newBlock->next = block;
        *firstPtr = newBlock;
        block = newBlock;
    }
    
    Assert(block->entityCount < ArrayCount(block->colliders));
    block->colliders[block->entityCount++] = collider;
}



inline void AddToSpacePartition(SimRegion* region, SpacePartition* partition, MemoryPool* tempPool, Vec3 P, Rect3 bounds, CollisionData collider)
{
    u32 surfaces[256];
    u32 addedTo = GetOverlappingSurfaces(region, partition, P, V3(0, 0, 0), bounds, surfaces, ArrayCount(surfaces));
    
    for(u32 index = 0; index < addedTo; ++index)
    {
        u32 surfaceIndex = surfaces[index];
        Assert(surfaceIndex < (u32) Squarei(partition->sideSquareCount));
        RegionPartitionSurface* surface = partition->partitionSurfaces + surfaceIndex;
        
        PartitionSurfaceEntityBlock* addHere = surface->first;
        AddToPartitionBlock(tempPool, &surface->first, addHere, collider);
    }
}

inline RegionPartitionQueryResult QuerySpacePartition(SimRegion* region, SpacePartition* partition, Vec3 P, Vec3 deltaP, Rect3 bounds)
{
    RegionPartitionQueryResult result = {};
    
    u32 surfaces[64];
    u32 overlapping = GetOverlappingSurfaces(region, partition, P, deltaP, bounds, surfaces, ArrayCount(surfaces));
    
    Assert(overlapping <= ArrayCount(result.surfaceIndexes));
    
    for(u32 overlappingIndex = 0; overlappingIndex < overlapping; ++overlappingIndex)
    {
        result.surfaceIndexes[overlappingIndex] = surfaces[overlappingIndex];
    }
    
    return result;
}

inline RegionPartitionQueryResult QuerySpacePartitionRadious(SimRegion* region, SpacePartition* partition, Vec3 P, Vec3 radious)
{
    RegionPartitionQueryResult result = QuerySpacePartition(region, partition, P, V3(0, 0, 0), RectCenterDim(V3(0, 0, 0), radious));
    return result;
}


inline PartitionSurfaceEntityBlock* QuerySpacePartitionPoint(SimRegion* region, SpacePartition* partition, Vec3 P)
{
    OverlappingSurfaceResult overlapping = OverlappingSurfaceIndex(region, partition, P);
    RegionPartitionSurface* surface  = partition->partitionSurfaces + (overlapping.Y * partition->sideSquareCount) + overlapping.X;
    PartitionSurfaceEntityBlock* result = surface->first;
    
    return result;
}
