internal BrainParams* GetBrainParams(ServerState* server, EntityID ID)
{
    DefaultComponent* def = GetComponent(server, ID, DefaultComponent);
    EntityDefinition* entityDef = GetEntityTypeDefinition(server->assets, def->type);
    BrainParams* result = &entityDef->server.brainParams;
    return result;
}

internal void SetBrainCommand(BrainComponent* brain, u16 action, EntityID targetID, Vec3 acceleration)
{
    brain->currentCommand.action = action;
    brain->currentCommand.targetID = targetID;
    brain->commandParameters.acceleration = acceleration;
}

internal void ResetDirection(BrainDirection* direction, u16 action)
{
    direction->action = action;
    direction->coeff = 1.0f;
    direction->sum = 0.0f;
}

internal void ResetDirections(ServerState* server, EntityID ID)
{
    BrainComponent* brain = GetComponent(server, ID, BrainComponent);
    ResetDirection(&brain->idleDirection, idle);
    for(u32 dirIndex = 0; dirIndex < ArrayCount(brain->directions); ++dirIndex)
    {
        ResetDirection(brain->directions + dirIndex, 0);
    }
}

internal void DoAction(ServerState* server, EntityID ID, u16 action)
{
    BrainComponent* brain = GetComponent(server, ID, BrainComponent);
    brain->idleDirection.action = action;
    brain->idleDirection.sum = R32_MAX;
    brain->idleDirection.coeff = 1.0f;
}

internal b32 ShouldPerceive(DefaultComponent* d1, EntityID i1, DefaultComponent* d2, EntityID i2)
{
    b32 result = (!AreEqual(i1, i2) && d1->P.chunkZ == d2->P.chunkZ);
    return result;
    
}

#define EVALUATOR(name) internal void name(ServerState* server, EntityID ID, r32 elapsedTime)
#define Evaluate(name) name(server, ID, elapsedTime)

internal u16 GetDirectionIndex(Vec3 direction)
{
    r32 angle = RadToDeg(AArm2(direction.xy));
    u16 index = (u16) (angle / DIRECTION_ANGLE);
    return index;
}

internal BrainDirection* GetDirection(BrainComponent* brain, u16 dirIndex)
{
    Assert(dirIndex < ArrayCount(brain->directions));
    BrainDirection* result = brain->directions + dirIndex;
    return result;
}

internal BrainDirection* GetDirection(BrainComponent* brain, Vec3 direction)
{
    u16 index = GetDirectionIndex(direction);
    BrainDirection* result = GetDirection(brain, index);
    return result;
}

internal Vec3 GetDirection(u16 dirIndex)
{
    Assert(dirIndex < DIRECTION_COUNT);
    r32 angle = dirIndex * DIRECTION_ANGLE;
    Vec3 result = V3(Arm2(DegToRad(angle)), 0);
    return result;
}

internal u16 Opposite(u16 dirIndex)
{
    Assert(DIRECTION_COUNT % 2 == 0);
    u16 result = (dirIndex + (DIRECTION_COUNT / 2)) % DIRECTION_COUNT;
    return result;
}









internal void ModulateDirection(BrainComponent* brain, Vec3 direction, r32 coeff, r32 adiacentCoeff)
{
    Vec3 normalized = Normalize(direction);
    BrainDirection* dir = GetDirection(brain, normalized);
    dir->coeff *= coeff;
    
    if(adiacentCoeff > 0)
    {
        Vec3 leftDirection = normalized + V3(Arm2(DegToRad(DIRECTION_ANGLE)), 0);
        BrainDirection* left = GetDirection(brain, leftDirection);
        left->coeff *= (coeff * adiacentCoeff);
        
        Vec3 rightDirection = normalized - V3(Arm2(DegToRad(DIRECTION_ANGLE)), 0);
        BrainDirection* right = GetDirection(brain, rightDirection);
        right->coeff *= (coeff * adiacentCoeff);
    }
}

internal void ZeroDirection(BrainComponent* brain, Vec3 direction, b32 adiacentDirections)
{
    r32 adiacentCoeff = adiacentDirections ? 1.0f : 0.0f;
    ModulateDirection(brain, direction, 0, adiacentCoeff);
}

internal ReachableCell* GetCell(ReachableMapComponent* map, u32 X, u32 Y)
{
    Assert(X < REACHABLE_GRID_DIM);
    Assert(Y < REACHABLE_GRID_DIM);
    
    ReachableCell* result = (ReachableCell*) map->cells + (Y * REACHABLE_GRID_DIM) + X;
    return result;
}

internal ReachableCell* GetCell(BrainComponent* brain, BrainParams* params, Vec3 offset)
{
    ReachableCell* result = 0;
    
    r32 gridDim = params->reachableCellDim * REACHABLE_GRID_DIM;
    offset += 0.5f * V3(gridDim, gridDim, 0);
    
    if(offset.x >= 0 && 
       offset.x < gridDim &&
       offset.y >= 0 && 
       offset.y < gridDim)
    {
        u32 X = TruncateReal32ToU32(offset.x / params->reachableCellDim);
        u32 Y = TruncateReal32ToU32(offset.y / params->reachableCellDim);
        result = GetCell(brain->reachableMap, X, Y);
    }
    
    
    return result;
}

inline void Queue(ReachableQueue* queue, ReachableCell* cell, Vec3 offset)
{
    ReachableQueueElement* el = PushStruct(queue->pool, ReachableQueueElement);
    el->offset = offset;
    el->cell = cell;
    el->next = 0;
    
    if(queue->last)
    {
        Assert(queue->first);
        queue->last->next = el;
        queue->last = el;
    }
    else
    {
        Assert(!queue->first);
        queue->first = queue->last = el;
    }
}

inline ReachableQueueElement* Pop(ReachableQueue* queue)
{
    ReachableQueueElement* result = queue->first;
    if(result && result->next)
    {
        queue->first = result->next;
    }
    else
    {
        Assert(result == queue->last);
        queue->first = queue->last = 0;
    }
    
    return result;
}

internal void ComputeReachabilityGrid(ServerState* server, EntityID ID)
{
    BrainComponent* brain = GetComponent(server, ID, BrainComponent);
    DefaultComponent* def = GetComponent(server, ID, DefaultComponent);
    BrainParams* params = GetBrainParams(server, ID);
    
    for(u32 cellY = 0; cellY < REACHABLE_GRID_DIM; ++cellY)
    {
        for(u32 cellX = 0; cellX < REACHABLE_GRID_DIM; ++cellX)
        {
            ReachableCell* cell = GetCell(brain->reachableMap, cellX, cellY);
            cell->reachable = true;
            cell->shortestDirection = 0xffff;
        }
    }
    
    r32 gridDim = params->reachableCellDim * REACHABLE_GRID_DIM;
    Vec2 offset = 0.5f * V2(gridDim, gridDim);
    
    SpatialPartitionQuery query = QuerySpatialPartition(&server->standardPartition, def->P, RectCenterDim(V3(0, 0, 0), gridDim * V3(1, 1, 0)));
    
    for(EntityID testID = GetCurrent(&query); IsValid(&query); testID = Advance(&query))
    {
        DefaultComponent* testDef = GetComponent(server, testID, DefaultComponent);
        if(ShouldPerceive(def, ID, testDef, testID))
        {
            if(ShouldCollide(def->boundType, testDef->boundType))
            {
                Vec3 toTarget = SubtractOnSameZChunk(testDef->P, def->P) + V3(offset, 0);
                Rect3 bounds = ComputeMinkowski(toTarget, def->bounds, testDef->bounds);
                
                Vec2 points[4];
                
                points[0] = bounds.min.xy;
                points[1] = V2(bounds.max.x, bounds.min.y);
                points[2] = bounds.max.xy;
                points[3] = V2(bounds.min.x, bounds.max.y);
                
                i32 minX = REACHABLE_GRID_DIM;
                i32 maxX = 0;
                
                i32 minY = REACHABLE_GRID_DIM;
                i32 maxY = 0;
                
                for(u32 pIndex = 0; pIndex < ArrayCount(points); ++pIndex)
                {
                    Vec2 P = points[pIndex];
                    if(P.x >= 0 && 
                       P.x < gridDim &&
                       P.y >= 0 && 
                       P.y < gridDim)
                    {
                        i32 X = TruncateReal32ToI32(P.x / params->reachableCellDim);
                        i32 Y = TruncateReal32ToI32(P.y / params->reachableCellDim);
                        
                        Assert(X >= 0 && X < REACHABLE_GRID_DIM);
                        Assert(X >= 0 && X < REACHABLE_GRID_DIM);
                        
                        minX = Min(minX, X);
                        maxX = Max(maxX, X + 1);
                        
                        minY = Min(minY, Y);
                        maxY = Max(maxY, Y + 1);
                    }
                }
                
                for(i32 cellY = minY; cellY < maxY; ++cellY)
                {
                    for(i32 cellX = minX; cellX < maxX; ++cellX)
                    {
                        Assert(cellX >= 0 && cellX < REACHABLE_GRID_DIM);
                        Assert(cellY >= 0 && cellY < REACHABLE_GRID_DIM);
                        
                        ReachableCell* cell = GetCell(brain->reachableMap, cellX, cellY);
                        cell->reachable = false;
                        
#if 0                        
                        cell->anglesOccluded[0] = ?;
                        cell->anglesOccluded[1] = ?;
                        cell->anglesOccluded[2] = ?;
                        cell->anglesOccluded[3] = ?;
#endif
                        
                    }
                }
            }
        }
    }
    
    TempMemory queueMemory = BeginTemporaryMemory(server->frameByFramePool);
    
    ReachableCell* cell = GetCell(brain, params, V3(0, 0, 0));
    cell->reachable = true;
    cell->shortestDirection = 0xffff;
    ReachableQueue queue = {};
    queue.pool = queueMemory.pool;
    Queue(&queue, cell, V3(0, 0, 0));
    
    while(true)
    {
        ReachableQueueElement* el = Pop(&queue);
        
        if(!el)
        {
            break;
        }
        
        Assert(DIRECTION_COUNT == 8);
        Vec2 offsets[DIRECTION_COUNT];
        offsets[0] = V2(1, 0);
        offsets[1] = V2(1, 1);
        offsets[2] = V2(0, 1);
        offsets[3] = V2(-1, 1);
        offsets[4] = V2(-1, 0);
        offsets[5] = V2(-1, -1);
        offsets[6] = V2(0, -1);
        offsets[7] = V2(1, -1);
        
        for(u16 directionIndex = 0; directionIndex < DIRECTION_COUNT; ++directionIndex)
        {
            Vec3 newOffset = el->offset + V3(params->reachableCellDim * offsets[directionIndex], 0);
            ReachableCell* sorrounding = GetCell(brain, params, newOffset);
            if(sorrounding && sorrounding->reachable && (sorrounding->shortestDirection == 0xffff))
            {
                b32 canMove = true;
                
#if 0                
                switch(directionIndex)
                {
                    case 1:
                    {
                        canMove = NotOccluded(angle0) || NotOccluded(angle1);
                    } break;
                    
                    case 3:
                    {
                        canMove = NotOccluded(angle0) || NotOccluded(angle1);
                    } break;
                    
                    case 5:
                    {
                        
                    } break;
                    
                    case 7:
                    {
                        
                    } break;
                }
#endif
                
                if(canMove)
                {
                    if(el->cell->shortestDirection == 0xffff)
                    {
                        sorrounding->shortestDirection = directionIndex;
                    }
                    else
                    {
                        sorrounding->shortestDirection = el->cell->shortestDirection;
                    }
                    Queue(&queue, sorrounding, newOffset);
                }
            }
        }
    }
}

internal void ScoreCell(BrainComponent* brain, ReachableCell* cell, r32 score)
{
    if(cell)
    {
        if(cell->reachable)
        {
            if(cell->shortestDirection != 0xffff)
            {
                Assert(cell->shortestDirection < ArrayCount(brain->directions));
                BrainDirection* dir = brain->directions + cell->shortestDirection;
                dir->sum += score;
            }
        }
    }
}

internal void ScoreInDirection(BrainComponent* brain, BrainParams* params, Vec3 direction, r32 startingLength, r32 baseScore, r32 persistance)
{
    r32 length = startingLength;
    r32 score = baseScore;
    
    while(score > 0)
    {
        Vec3 offset = direction * length;
        ReachableCell* cell = GetCell(brain, params, offset);
        if(cell)
        {
            ScoreCell(brain, cell, score);
        }
        else
        {
            break;
        }
        
        length += params->reachableCellDim;
        score *= persistance;
    }
}

internal void ScoreAround(BrainComponent* brain, BrainParams* params, Vec3 offset, r32 score)
{
    ReachableCell* cell = GetCell(brain, params, offset);
    ScoreCell(brain, cell, score);
    
    for(u16 directionIndex = 0; directionIndex < DIRECTION_COUNT; ++directionIndex)
    {
        cell = GetCell(brain, params, offset + GetDirection(directionIndex) * params->reachableCellDim);
        ScoreCell(brain, cell, score * 0.9f);
    }
}

internal void SumScore(BrainComponent* brain, BrainParams* params, Vec3 offset, r32 score, r32 persistanceVertically, r32 persistanceHorizontally)
{
    r32 length = Length(offset);
    Vec3 direction = Normalize(offset);
    
    ScoreInDirection(brain, params, direction, length, score, persistanceVertically);
    
    Vec3 rightDirection = Normalize(direction + V3(Arm2(DegToRad(DIRECTION_ANGLE)), 0));
    ScoreInDirection(brain, params, rightDirection, length, score * persistanceHorizontally, persistanceVertically);
    
    Vec3 leftDirection = Normalize(direction - V3(Arm2(DegToRad(DIRECTION_ANGLE)), 0));
    ScoreInDirection(brain, params, leftDirection, length, score * persistanceHorizontally, persistanceVertically);
}

internal void ScoreTarget(ServerState* server, EntityID ID, EntityID targetID, r32 score)
{
    BrainComponent* brain = GetComponent(server, ID, BrainComponent);
    BrainParams* params = GetBrainParams(server, ID);
    
    DefaultComponent* def = GetComponent(server, ID, DefaultComponent);
    DefaultComponent* targetDef = GetComponent(server, targetID, DefaultComponent);
    Vec3 toTarget = SubtractOnSameZChunk(targetDef->P, def->P);
    
    ReachableCell* targetCell = GetCell(brain, params, toTarget);
    
    if(targetCell)
    {
        ScoreAround(brain, params, toTarget, score);
    }
    else
    {
        BrainDirection* dir = GetDirection(brain, toTarget);
        dir->sum += score;
    }
}

internal void ComputeFinalDirection(BrainComponent* brain)
{
    SetBrainCommand(brain, brain->idleDirection.action, brain->targetID, V3(0, 0, 0));
    
    r32 highestScore = brain->idleDirection.sum * brain->idleDirection.coeff;
    for(u16 dirIndex = 0; dirIndex < ArrayCount(brain->directions); ++dirIndex)
    {
        BrainDirection* direction = brain->directions + dirIndex;
        r32 score = direction->coeff * direction->sum;
        if(score > highestScore)
        {
            highestScore = score;
            SetBrainCommand(brain, move, {}, GetDirection(dirIndex));
        }
    }
}







#if 0
EVALUATOR(CollisionAvoidance)
{
    DefaultComponent* def = GetComponent(server, ID, DefaultComponent);
    MovementComponent* movement = GetComponent(server, ID, MovementComponent);
    BrainComponent* brain = GetComponent(server, ID, BrainComponent);
    
    if(movement)
    {
        r32 minDistanceSq = R32_MAX;
        EntityID avoidID = {};
        Vec3 avoidV = {};
        
        r32 seeAhead = 3.0f;
        u32 segmentCount = 8;
        Vec3 normalizedSpeed = Normalize(movement->speed);
        r32 increment = 1.0f / segmentCount;
        
        for(u32 avoidIndex = 0; avoidIndex < segmentCount && !IsValidID(avoidID); ++avoidIndex)
        {
            Vec3 avoid = seeAhead * normalizedSpeed * (increment * (avoidIndex + 1));
            UniversePos ahead = Offset(def->P, avoid);
            SpatialPartitionQuery avoidQuery = QuerySpatialPartitionAtPoint(&server->standardPartition, ahead);
            for(EntityID testID = GetCurrent(&avoidQuery); IsValid(&avoidQuery); testID = Advance(&avoidQuery))
            {
                DefaultComponent* testDef = GetComponent(server, testID, DefaultComponent);
                if(ShouldPerceive(def, ID, testDef, testID))
                {
                    if(ShouldCollide(def->boundType, testDef->boundType))
                    {
                        Vec3 distance = SubtractOnSameZChunk(testDef->P, ahead);
                        r32 distanceSq = LengthSq(distance);
                        r32 testRadiousSq =(Square(Max(GetDim(testDef->bounds).x, GetDim(testDef->bounds).y)) + 
                                            Square(Max(GetDim(def->bounds).x, GetDim(def->bounds).y)));
                        if(distanceSq < testRadiousSq)
                        {
                            if(distanceSq <= minDistanceSq)
                            {
                                ZeroDirection(brain, distance, false);
                            }
                        }
                    }
                }
            }
        }
    }
}
#endif


EVALUATOR(Wander)
{
    DefaultComponent* def = GetComponent(server, ID, DefaultComponent);
    UniversePos currentP = def->P;
    
    BrainComponent* brain = GetComponent(server, ID, BrainComponent);
    BrainParams* params = GetBrainParams(server, ID);
    
    brain->time += elapsedTime;
    if(brain->time >= 0)
    {
        r32 score = 1.0f;
        r32 verticalPersistance = 0.5f;
        r32 horizontalPersistance = 0.5f;
        SumScore(brain, params, brain->wanderDirection, score, verticalPersistance, horizontalPersistance);
        
        if(brain->time >= params->wanderTargetTime)
        {
            brain->time = -params->idleTimeWhenWandering;
            
            Vec3 random = V3(RandomBil(&server->entropy), RandomBil(&server->entropy), 0);
            
            r32 lerpHome = 0.0f;
            Vec3 toHome = {};
            
            if(currentP.chunkZ == brain->homeP.chunkZ)
            {
                toHome = SubtractOnSameZChunk(currentP, brain->homeP);
                lerpHome = Clamp01MapToRange(params->minHomeDistance, Length(toHome), params->maxHomeDistance);
                
                toHome = Normalize(toHome);
            }
            brain->wanderDirection = Lerp(random, lerpHome, toHome);
        }
    }
}

EVALUATOR(MaintainDistance)
{
    DefaultComponent* def = GetComponent(server, ID, DefaultComponent);
    BrainComponent* brain = GetComponent(server, ID, BrainComponent);
    BrainParams* params = GetBrainParams(server, ID);
    
    SpatialPartitionQuery perceiveQuery = QuerySpatialPartitionAtPoint(&server->standardPartition, def->P);
    
    EntityType type = GetEntityType(server->assets, params->maintainDistanceType);
    
    for(EntityID testID = GetCurrent(&perceiveQuery); IsValid(&perceiveQuery); testID = Advance(&perceiveQuery))
    {
        DefaultComponent* testDef = GetComponent(server, testID, DefaultComponent);
        if(ShouldPerceive(def, ID, testDef, testID))
        {
            Vec3 toTarget = SubtractOnSameZChunk(testDef->P, def->P);
            if(AreEqual(testDef->type, type))
            {
                if(LengthSq(toTarget) <= Square(params->maintainDistanceRadious))
                {
                    ModulateDirection(brain, -toTarget, params->maintainDistanceModulationCoeff, params->maintainDistanceModulationAdiacentCoeff);
                }
            }
        }
    }
}


EVALUATOR(MaintainDistanceFromLight)
{
    DefaultComponent* def = GetComponent(server, ID, DefaultComponent);
    BrainComponent* brain = GetComponent(server, ID, BrainComponent);
    BrainParams* params = GetBrainParams(server, ID);
    
    if(params->fearsLight)
    {
        SpatialPartitionQuery perceiveQuery = QuerySpatialPartitionAtPoint(&server->standardPartition, def->P);
        
        for(EntityID testID = GetCurrent(&perceiveQuery); IsValid(&perceiveQuery); testID = Advance(&perceiveQuery))
        {
            DefaultComponent* testDef = GetComponent(server, testID, DefaultComponent);
            if(ShouldPerceive(def, ID, testDef, testID))
            {
                Vec3 toTarget = SubtractOnSameZChunk(testDef->P, def->P);
                r32 lightRadiousSq = GetLightRadiousSq(server, testID);
                if(lightRadiousSq > 0)
                {
                    r32 realRadiousSq = lightRadiousSq + Square(params->safetyLightRadious);
                    if(LengthSq(toTarget) <= realRadiousSq)
                    {
                        ZeroDirection(brain, toTarget, true);
                    }
                }
            }
        }
    }
}

EVALUATOR(Flee)
{
    DefaultComponent* def = GetComponent(server, ID, DefaultComponent);
    BrainComponent* brain = GetComponent(server, ID, BrainComponent);
    BrainParams* params = GetBrainParams(server, ID);
    SpatialPartitionQuery perceiveQuery = QuerySpatialPartitionAtPoint(&server->standardPartition, def->P);
    
    EntityType type = GetEntityType(server->assets, params->scaryType);
    for(EntityID testID = GetCurrent(&perceiveQuery); IsValid(&perceiveQuery); testID = Advance(&perceiveQuery))
    {
        DefaultComponent* testDef = GetComponent(server, testID, DefaultComponent);
        if(ShouldPerceive(def, ID, testDef, testID))
        {
            Vec3 toTarget = SubtractOnSameZChunk(testDef->P, def->P);
            if(AreEqual(testDef->type, type))
            {
                if(LengthSq(toTarget) <= Square(params->safeDistanceRadious))
                {
                    ZeroDirection(brain, toTarget, true);
                    SumScore(brain, params, -toTarget, 1.0f, 0.5f, 0.5f);
                }
            }
        }
    }
}

EVALUATOR(FleeFromLight)
{
    DefaultComponent* def = GetComponent(server, ID, DefaultComponent);
    BrainComponent* brain = GetComponent(server, ID, BrainComponent);
    BrainParams* params = GetBrainParams(server, ID);
    
    if(params->fearsLight)
    {
        SpatialPartitionQuery perceiveQuery = QuerySpatialPartitionAtPoint(&server->standardPartition, def->P);
        
        for(EntityID testID = GetCurrent(&perceiveQuery); IsValid(&perceiveQuery); testID = Advance(&perceiveQuery))
        {
            DefaultComponent* testDef = GetComponent(server, testID, DefaultComponent);
            if(ShouldPerceive(def, ID, testDef, testID))
            {
                Vec3 toTarget = SubtractOnSameZChunk(testDef->P, def->P);
                if(LengthSq(toTarget) <= GetLightRadiousSq(server, testID))
                {
                    ZeroDirection(brain, toTarget, true);
                    SumScore(brain, params, -toTarget, 1.0f, 0.5f, 0.5f);
                }
            }
        }
    }
}

internal b32 SearchForHostileEnemies(ServerState* server, EntityID brainID, EntityID* ID)
{
    b32 result = false;
    DefaultComponent* def = GetComponent(server, brainID, DefaultComponent);
    BrainParams* params = GetBrainParams(server, brainID);
    SpatialPartitionQuery perceiveQuery = QuerySpatialPartitionAtPoint(&server->standardPartition, def->P);
    
    EntityType type = GetEntityType(server->assets, params->hostileType);
    for(EntityID testID = GetCurrent(&perceiveQuery); IsValid(&perceiveQuery); testID = Advance(&perceiveQuery))
    {
        DefaultComponent* testDef = GetComponent(server, testID, DefaultComponent);
        if(ShouldPerceive(def, brainID, testDef, testID))
        {
            if(AreEqual(testDef->type, type))
            {
                Vec3 toTarget = SubtractOnSameZChunk(testDef->P, def->P);
                if(LengthSq(toTarget) <= Square(params->hostileDistanceRadious))
                {
                    result = true;
                    *ID = testID;
                    break;
                }
            }
        }
    }
    
    return result;
}

internal b32 SearchForScaryEntities(ServerState* server, EntityID brainID)
{
    b32 result = false;
    
    DefaultComponent* def = GetComponent(server, brainID, DefaultComponent);
    BrainParams* params = GetBrainParams(server, brainID);
    SpatialPartitionQuery perceiveQuery = QuerySpatialPartitionAtPoint(&server->standardPartition, def->P);
    
    EntityType type = GetEntityType(server->assets, params->scaryType);
    for(EntityID testID = GetCurrent(&perceiveQuery); IsValid(&perceiveQuery); testID = Advance(&perceiveQuery))
    {
        DefaultComponent* testDef = GetComponent(server, testID, DefaultComponent);
        if(ShouldPerceive(def, brainID, testDef, testID))
        {
            Vec3 toTarget = SubtractOnSameZChunk(testDef->P, def->P);
            if(AreEqual(testDef->type, type))
            {
                if(LengthSq(toTarget) <= Square(params->scaryDistanceRadious))
                {
                    result = true;
                    break;
                }
            }
        }
    }
    
    return result;
}

internal b32 SearchForScaryLights(ServerState* server, EntityID brainID)
{
    b32 result = false;
    
    DefaultComponent* def = GetComponent(server, brainID, DefaultComponent);
    BrainParams* params = GetBrainParams(server, brainID);
    SpatialPartitionQuery perceiveQuery = QuerySpatialPartitionAtPoint(&server->standardPartition, def->P);
    
    for(EntityID testID = GetCurrent(&perceiveQuery); IsValid(&perceiveQuery); testID = Advance(&perceiveQuery))
    {
        DefaultComponent* testDef = GetComponent(server, testID, DefaultComponent);
        if(ShouldPerceive(def, brainID, testDef, testID))
        {
            Vec3 toTarget = SubtractOnSameZChunk(testDef->P, def->P);
            if(LengthSq(toTarget) <= GetLightRadiousSq(server, testID))
            {
                result = true;
                break;
            }
        }
    }
    
    return result;
}

internal b32 ActionIsPossible(ServerState* server, u16 action, EntityID ID, EntityID targetID)
{
    b32 result = false;
    
    InteractionComponent* targetInteraction = GetComponent(server, targetID, InteractionComponent);
    UsingComponent* equippedComponent = GetComponent(server, ID, UsingComponent);
    
    EntityType equipped[Count_usingSlot];
    u32 equippedCount = 0;
    if(equippedComponent)
    {
        equippedCount = ArrayCount(equippedComponent->slots);
        for(u32 slotIndex = 0; slotIndex < ArrayCount(equippedComponent->slots); ++slotIndex)
        {
            EntityID slotID = GetBoundedID(equippedComponent->slots + slotIndex);
            equipped[slotIndex] = GetEntityType(server, slotID);
        }
    }
    
    if(ActionIsPossible(targetInteraction, action, equipped, equippedCount))
    {
        result = true;
    }
    
    return result;
}

internal b32 ActionIsPossibleAtCurrentDistance(ServerState* server, u16 action, EntityID ID, EntityID targetID)
{
    b32 result = false;
    
    DefaultComponent* def = GetComponent(server, ID, DefaultComponent);
    BrainComponent* brain = GetComponent(server, ID, BrainComponent);
    CombatComponent* combat = GetComponent(server, ID, CombatComponent);
    UsingComponent* equippedComponent = GetComponent(server, ID, UsingComponent);
    
    DefaultComponent* targetDef = GetComponent(server, targetID, DefaultComponent);
    InteractionComponent* targetInteraction = GetComponent(server, targetID, InteractionComponent);
    
    Vec3 toTarget = SubtractOnSameZChunk(targetDef->P, def->P);
    r32 distanceSq = LengthSq(toTarget);
    r32 targetTime;
    
    EntityType equipped[Count_usingSlot];
    u32 equippedCount = 0;
    if(equippedComponent)
    {
        equippedCount = ArrayCount(equippedComponent->slots);
        for(u32 slotIndex = 0; slotIndex < ArrayCount(equippedComponent->slots); ++slotIndex)
        {
            EntityID slotID = GetBoundedID(equippedComponent->slots + slotIndex);
            equipped[slotIndex] = GetEntityType(server, slotID);
        }
    }
    
    u16 oldAction = brain->currentCommand.action;
    if(ActionIsPossibleAtDistance(targetInteraction, action, oldAction, distanceSq, &targetTime, combat, equipped, equippedCount))
    {
        result = true;
    }
    return result;
}

#define ChangeState(brain, s) brain->state = BrainState_##s;
STANDARD_ECS_JOB_SERVER(UpdateBrain)
{
    DefaultComponent* def = GetComponent(server, ID, DefaultComponent);
    BrainComponent* brain = GetComponent(server, ID, BrainComponent);
    
    switch(brain->type)
    {
        case Brain_invalid:
        {
            
        } break;
        
		case Brain_Player:
		{
            PlayerComponent* player = GetComponent(server, ID, PlayerComponent);
            Assert(player);
			brain->currentCommand = player->requestCommand;
            brain->inventoryCommand = player->inventoryCommand;
            brain->commandParameters = player->commandParameters;
            player->inventoryCommand = {};
		} break;	
        
		case Brain_Portal:
		{
            SpatialPartitionQuery playerQuery = QuerySpatialPartitionAtPoint(&server->playerPartition, def->P);
            EntityID playerID = GetCurrent(&playerQuery);
            if(IsValidID(playerID))
            {
                DefaultComponent* playerDef = GetComponent(server, playerID, DefaultComponent);
                r32 maxDistanceSq = Square(2.0f);
                
                r32 distanceSq = LengthSq(SubtractOnSameZChunk(playerDef->P, def->P));
                if(distanceSq < maxDistanceSq)
                {
                    if(!IsValidID(brain->targetID))
                    {
                        AddEntityParams params = DefaultAddEntityParams();
                        params.targetBrainID = ID;
                        EntityType type = GetEntityType(server->assets, "default", "wolf");
                        AddEntityFlags(def, EntityFlag_locked);
                        AddEntity(server, def->P, &server->entropy, type, params);
                    }
				}
                else
                {
                    if(IsValidID(brain->targetID))
                    {
                        DeleteEntity(server, brain->targetID);
                        brain->targetID = {};
                    }
                }
            }
            else
            {
                if(IsValidID(brain->targetID))
                {
                    DeleteEntity(server, brain->targetID);
                    brain->targetID = {};
                }
            }
        } break;
        
        case Brain_stateMachine:
        {
            if(server->updateBrains)
            {
                ResetDirections(server, ID);
                ComputeReachabilityGrid(server, ID);
                
                switch(brain->state)
                {
                    case BrainState_Wandering:
                    {
                        Evaluate(Wander);
                        Evaluate(MaintainDistance);
                        Evaluate(MaintainDistanceFromLight);
                        
                        
                        EntityID hostileID;
                        if(SearchForHostileEnemies(server, ID, &hostileID))
                        {
                            ChangeState(brain, Chasing);
                            brain->targetID = hostileID;
                        }
                        
                        if(SearchForScaryEntities(server, ID) || SearchForScaryLights(server, ID))
                        {
                            ChangeState(brain, Fleeing);
                        }
                    } break;
                    
                    case BrainState_Fleeing:
                    {
                        Evaluate(Flee);
                        Evaluate(FleeFromLight);
                        
                        if(!SearchForScaryEntities(server, ID) && !SearchForScaryLights(server, ID))
                        {
                            ChangeState(brain, Wandering);
                        }
                    } break;
                    
                    case BrainState_Chasing:
                    {
                        ScoreTarget(server, ID, brain->targetID, 1.0f);
                        Evaluate(MaintainDistance);
                        Evaluate(MaintainDistanceFromLight);
                        
                        if(ActionIsPossible(server, attack, ID, brain->targetID))
                        {
                            if(ActionIsPossibleAtCurrentDistance(server, attack, ID, brain->targetID))
                            {
                                ChangeState(brain, Attacking);
                            }
                        }
                        else
                        {
                            ChangeState(brain, Wandering);
                        }
                        
                        if(SearchForScaryEntities(server, ID) || SearchForScaryLights(server, ID))
                        {
                            ChangeState(brain, Fleeing);
                        }
                    } break;
                    
                    case BrainState_Attacking:
                    {
                        if(ActionIsPossibleAtCurrentDistance(server, attack, ID, brain->targetID))
                        {
                            DoAction(server, ID, attack);
                        }
                        else
                        {
                            if(ActionIsPossible(server, attack, ID, brain->targetID))
                            {
                                ChangeState(brain, Chasing);
                            }
                            else
                            {
                                ChangeState(brain, Wandering);
                            }
                        }
                        
                        if(SearchForScaryEntities(server, ID) || SearchForScaryLights(server, ID))
                        {
                            ChangeState(brain, Fleeing);
                        }
                    } break;
                }
            }
            
            ComputeFinalDirection(brain);
        } break;
    }
}


STANDARD_ECS_JOB_SERVER(ExecuteCommand)
{
    ActionComponent* action = GetComponent(server, ID, ActionComponent);
    BrainComponent* brain = GetComponent(server, ID, BrainComponent);
    
    DispatchCommand(server, ID, &brain->currentCommand, &brain->commandParameters, elapsedTime, true);
    DispatchCommand(server, ID, &brain->inventoryCommand, &brain->commandParameters, elapsedTime, false);
}