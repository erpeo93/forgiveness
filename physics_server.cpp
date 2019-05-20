internal void HandleCollision(SimEntity* entity, SimEntity* entityToCheck)
{
    
#if 0    
    if( entityToCheck->type < entity->type )
    {
        SimEntity* temp = entity;
        entity = entityToCheck;
        entityToCheck = temp;
    }
    
    if( entity->type == Entity_Human && entityToCheck->type == Entity_Doungeon )
    {
        entity->destPosition = entityToCheck->destPosition;
        entity->acceleration = V3( 0, 0, 0 );
        entity->velocity = V3( 0, 0, 0 );
        AddFlags( entity, Flag_teleported );
    }
    
    if( entityToCheck->type == Entity_Stairs )
    {
        entity->regionPosition.z = 5.6f;
        entity->regionPosition.y = entityToCheck->regionPosition.y;
        entity->acceleration = V3( 0, 0, 0 );
        entity->velocity = V3( 0, 0, 0 );
    }
#endif
    
}

struct OverlappingSurfaceResult
{
    u32 X;
    u32 Y;
};

inline OverlappingSurfaceResult OverlappingSurfaceIndex(SimRegion* region, SpacePartition* partition, Vec3 P)
{
    OverlappingSurfaceResult result = {};
    result.X = TruncateReal32ToU32((P.x + region->halfGridDim) * partition->oneOverSurfaceDim);
    result.Y = TruncateReal32ToU32((P.y + region->halfGridDim) * partition->oneOverSurfaceDim);
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
    
    maxX = Min(maxX, partition->partitionSurfaceDim - 1);
    maxY = Min(maxY, partition->partitionSurfaceDim - 1);
    
    u32 outputCount = 0;
    for(u32 Y = minY; Y <= maxY; ++Y)
    {
        for(u32 X = minX; X <= maxX; ++X)
        {
            Assert(outputCount < maxOutputCount);
            u32 surfaceIndex = (Y * partition->partitionSurfaceDim) + X;
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
        Assert(surfaceIndex < (u32) Squarei(partition->partitionSurfaceDim));
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
    RegionPartitionSurface* surface  = partition->partitionSurfaces + (overlapping.Y * partition->partitionSurfaceDim) + overlapping.X;
    PartitionSurfaceEntityBlock* result = surface->first;
    
    return result;
}

inline r32 CalculateCollisionRadiousSq(Rect3 bounds)
{
    Vec3 boundRadious = 0.5f * GetDim(bounds);
    r32 result = Square(boundRadious.x) + Square(boundRadious.y);
    return result;
}

internal r32 CheckCollisions(SimRegion* region, SimEntity* entity, 
                             Vec3 deltaP, r32 tRemaining, Vec3* wallNormalMin, CheckCollisionCurrent* hitMin)
{
    r32 result = tRemaining;
    if(entity->boundType)
    {
        r32 radiousSq = CalculateCollisionRadiousSq(entity->bounds);
        
        Assert(HasArea(entity->bounds));
        RegionPartitionQueryResult query = QuerySpacePartition(region, &region->collisionPartition, entity->P, deltaP, entity->bounds);
        for(u32 surfaceIndex = 0; surfaceIndex < ArrayCount(query.surfaceIndexes); ++surfaceIndex)
        {
            RegionPartitionSurface* surface = region->collisionPartition.partitionSurfaces + query.surfaceIndexes[surfaceIndex];
            
            PartitionSurfaceEntityBlock* block = surface->first;
            while(block)
            {
                for(u32 blockIndex = 0; blockIndex < block->entityCount; ++blockIndex)
                {
                    CollisionData* collider = block->colliders + blockIndex;
                    r32 testRadiousSq = collider->radiousSq;
                    
                    Vec3 distance = collider->P - entity->P;
                    if(LengthSq(distance.xy) <= (testRadiousSq + radiousSq))
                    {
                        u32 entityIndex = collider->entityIndex;
                        SimEntity* entityToCheck = GetRegionEntity(region, entityIndex);
                        
                        if(ShouldCollide(entity->identifier, entity->boundType, entityToCheck->identifier, entityToCheck->boundType))
                        {
                            CheckCollisionCurrent test = {};
                            test.isEntity = true;
                            test.entity = entityToCheck;
                            test = HandleVolumeCollision( entity->P, entity->bounds, deltaP, entityToCheck->P, entityToCheck->bounds, &result, wallNormalMin, test );
                            
                            if(test.entity)
                            {
                                *hitMin = test;
                            }
                        }
                    }
                }
                
                block = block->next;
            }
        }
        
#if 0        
        for( u32 tileIndex = 0; tileIndex < ArrayCount( region->tiles ); ++tileIndex )
        {
            RegionTile* tile = region->tiles + tileIndex;
            if( ShouldCollide( entity, tile ) )
            {
                CollVolume testVolume_ = {};
                testVolume_.offset = V3( 0.0f, 0.0f, -0.5f * tile->P.z );
                testVolume_.dim = V3( VOXEL_SIZE, VOXEL_SIZE, tile->P.z );
                
                CollVolume* testVolume = &testVolume_;
                
                
                CheckCollisionCurrent test = {};
                test.isEntity = false;
                test.tile = tile;
                test = HandleVolumeCollision( entity->P, volume, deltaP, tile->P, testVolume, &tMin, &wallNormalMin, test );
                if( test.tile )
                {
                    hitMin = test;
                }
            }
        }
#endif
    }
    
    return result;
}

inline Vec3 GetMaxDelta(SimRegion* region)
{
    Vec3 result = V3(2.0f, 2.0f, 2.0f);
    return result;
}

internal void MoveEntityServer(SimRegion* region, SimEntity* entity, MoveSpec moveSpec)
{
    r32 timeToAdvance = region->timeToUpdate;
    
	if(moveSpec.stepCount > 0)
	{
        Vec3 maxDeltaP = GetMaxDelta(region);
        r32 maxDistanceAllowed = Length(maxDeltaP.xy);
        
        r32 distanceAllowed = Square(timeToAdvance) * moveSpec.acceleration;
        distanceAllowed = Min(distanceAllowed, maxDistanceAllowed);
        for(u8 stepIndex = 0; stepIndex < moveSpec.stepCount; ++stepIndex)
        {
            if(distanceAllowed > 0)
            {
                Vec3 step = moveSpec.steps[stepIndex];
                r32 stepLength = Length(step);
                
                if(stepLength <= distanceAllowed)
                {
                    Vec3 wallNormalIgnored = {};
                    CheckCollisionCurrent hitMin = {};
                    r32 tStop = CheckCollisions(region, entity, step, 1.0f, &wallNormalIgnored, &hitMin);
                    
                    if(hitMin.isEntity)
                    {
                        HandleCollision(entity, hitMin.entity);
                    }
                    entity->P += step * tStop;
                    distanceAllowed -= stepLength;
                    
                    if(tStop < 1.0f)
                    {
                        break;
                    }
                }
                else
                {
                    break;
                }
            }
        }
    }
	else
	{
        Vec3 initialAcc = entity->acceleration;
        Vec3 acceleration = ComputeAcceleration(initialAcc, entity->velocity, moveSpec);
        
        if(!IsSet( entity, Flag_floating))
        {
            acceleration.z += -9.8f;
        }
        
        Vec3 deltaP = 0.5f * acceleration * Square(timeToAdvance) +
            entity->velocity * timeToAdvance;
        Vec3 maxDeltaP = GetMaxDelta(region);
        deltaP = ClampV3(-maxDeltaP, deltaP, maxDeltaP);
        
        
        entity->velocity += acceleration * timeToAdvance;
        r32 distanceCovered = Length(deltaP);
        
        
        r32 tRemaining = 1.0f;
        if(IsSet(entity, Flag_distanceLimit))
        {
            tRemaining = entity->distanceToTravel / distanceCovered;
            tRemaining = Min(1.0f, tRemaining);
        }
        
        for( u32 iteration = 0; (iteration < 2) && tRemaining > 0; iteration++)
        {
            Vec3 wallNormalMin = {};
            CheckCollisionCurrent hitMin = {};
            r32 tStop = CheckCollisions(region, entity, deltaP, tRemaining, &wallNormalMin, &hitMin);
            
            if(hitMin.isEntity)
            {
                HandleCollision(entity, hitMin.entity);
            }
            
            
#if 0            
            if(IsSet(entity, Flag_distanceLimit))
            {
                entity->distanceToTravel -= Length(tStop * deltaP);
                if(entity->distanceToTravel <= 0)
                {
                    ClearFlags(entity, Flag_distanceLimit);
                    entity->distanceToTravel = 0;
                    entity->brain.acceleration = V3( 0, 0, 0 );
                    entity->velocity = V3( 0, 0, 0 );
                    
                    if(IsSet(entity, Flag_deleteWhenDistanceCovered))
                    {
                        ClearFlags(entity, Flag_deleteWhenDistanceCovered);
                        result.toDelete = true;
                    }
                }
            }
#endif
            
            Vec3 wallNormal = wallNormalMin;
            entity->P += tStop * deltaP;
            
            entity->velocity = entity->velocity - Dot(entity->velocity, wallNormal) * wallNormal;
            deltaP = deltaP - Dot(deltaP, wallNormal) * wallNormal;
            tRemaining -= tStop * tRemaining;
        }
	}
    
    
    
    
    if(entity->P.z <= 0)
    {
        entity->velocity.z = 0;
        entity->P.z = 0;
    }
}