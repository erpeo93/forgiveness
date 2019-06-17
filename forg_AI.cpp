inline ReachableTile* GetReachableTileRaw(Brain* brain, u16 indexX, u16 indexY)
{
    Assert(indexX < REACHABLE_TILEMAP_DIM);
    Assert(indexY < REACHABLE_TILEMAP_DIM);
    ReachableTile* result = brain->reachableTiles + (indexY * REACHABLE_TILEMAP_DIM) + indexX;
    return result;
}

inline ReachableTile* GetReachableTile(Brain* brain, Vec3 relativeP, i16* indexX, i16* indexY)
{
    ReachableTile* result = 0;
    
    r32 halfTilemapSpan = REACHABLE_TILEMAP_SPAN * 0.5f;
    r32 realX = relativeP.x + halfTilemapSpan;
    r32 realY = relativeP.y + halfTilemapSpan;
    
    r32 tileSpan = REACHABLE_TILEMAP_SPAN / (r32) REACHABLE_TILEMAP_DIM;
    i16 X = TruncateReal32ToI16(realX / tileSpan);
    i16 Y = TruncateReal32ToI16(realY / tileSpan);
    
    *indexX = X;
    *indexY = Y;
    
    if(X >= 0 && 
       Y >= 0 &&
       X < REACHABLE_TILEMAP_DIM && 
       Y < REACHABLE_TILEMAP_DIM)
    {
        result = GetReachableTileRaw(brain, (u16) X, (u16) Y);
    }
    
    return result;
}


inline ReachableTile* GetReachableTile(Brain* brain, Vec3 relativeP)
{
    i16 ignX, ignY;
    ReachableTile* result = GetReachableTile(brain, relativeP, &ignX, &ignY);
    return result;
}



inline b32 IsReachable(ReachableTile* tile)
{
    b32 result = (tile->directionToReachOrigin > 0 && tile->directionToReachOrigin < 0xff);
    return result;
}

inline b32 IsOccupied(ReachableTile* tile)
{
    b32 result = (tile->directionToReachOrigin == 0xff);
    return result;
}

#define MAX_STEPS_TRACKED (0x1f - 1)
inline void SetTileAsUnreachable(ReachableTile* tile)
{
    tile ->directionToReachOrigin = 0xff;
}

inline u8 GetStepsToReachOrigin(ReachableTile* tile)
{
    u8 result = tile->directionToReachOrigin & 0x1f;
    Assert(result <= MAX_STEPS_TRACKED);
    return result;
}

inline u8 GetDirection(ReachableTile* tile)
{
    u8 result = (tile->directionToReachOrigin >> 5) & 3;
    Assert(result <= ReachableDir_Down);
    return result;
}

inline void SetTileAsReachable(ReachableTile* tile, u8 direction, u8 stepsToReachOrigin)
{
    Assert(direction < 4);
    Assert(stepsToReachOrigin <= MAX_STEPS_TRACKED);
    tile->directionToReachOrigin = (1 << 7) | (direction << 5) | stepsToReachOrigin;
}



inline b32 Reachable(Brain* brain, Vec3 relativeP, ForgBoundType myBoundType, Rect3 myBounds, ForgBoundType hisBoundType, Rect3 hisBounds)
{
    b32 result = true;
    if(myBoundType)
    {
        result = false;
        if(hisBoundType)
        {
            Rect3 minkowski = GetMinkowskiRect(myBounds, relativeP, hisBounds);
            
            i16 minX, minY, maxX, maxY;
            GetReachableTile(brain, minkowski.min, &minX, &minY);
            GetReachableTile(brain, minkowski.max, &maxX, &maxY);
            
            for(i16 Y = minY; Y <= maxY && !result; ++Y)
            {
                for(i16 X = minX; X <= maxX && !result; ++X)
                {
                    if(X >= 0 && Y >= 0 && X < REACHABLE_TILEMAP_DIM && Y < REACHABLE_TILEMAP_DIM)
                    {
                        ReachableTile* tile = GetReachableTileRaw(brain, (u16) X, (u16) Y);
                        for(i16 testY = Y - 1; (testY != Y + 1) && !result; ++testY)
                        {
                            for(i16 testX = X - 1; (testX != X + 1) && !result; ++testX)
                            {
                                if(testX >= 0 && testY >= 0 && testX < REACHABLE_TILEMAP_DIM && testY < REACHABLE_TILEMAP_DIM)
                                {
                                    ReachableTile* testTile = GetReachableTileRaw(brain, (u16) testX, (u16) testY);
                                    if(IsReachable(testTile))
                                    {
                                        result = true;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        else
        {
            ReachableTile* tile = GetReachableTile(brain, relativeP);
            if(tile)
            {
                result = IsReachable(tile);
            }
        }
    }
    
    return result;
}

inline void BuildPathToReach(Brain* brain, 
                             Vec3 myP, ForgBoundType myBoundType, Rect3 myBounds,
                             Vec3 targetP, ForgBoundType hisBoundType, Rect3 hisBounds)
{
    Vec3 relativeP = targetP - myP;
    brain->path.originP = myP;
    
    i16 indexX = -1;
    i16 indexY = -1;
    
    ReachableTile* tile = 0;
    if(myBoundType)
    {
        if(hisBoundType)
        {
            Rect3 minkowski = GetMinkowskiRect(myBounds, relativeP, hisBounds);
            
            i16 minX, minY, maxX, maxY;
            GetReachableTile(brain, minkowski.min, &minX, &minY);
            GetReachableTile(brain, minkowski.max, &maxX, &maxY);
            
            i16 startingX = minX;
            i16 endingX = maxX;
            i16 XOffset = 1;
            
            if(relativeP.x < 0)
            {
                startingX = maxX;
                endingX = minX;
                XOffset = 1;
            }
            
            i16 startingY = minY;
            i16 endingY = maxY;
            i16 YOffset = 1;
            
            if(relativeP.y < 0)
            {
                startingY = maxY;
                endingY = minY;
                YOffset = -1;
            }
            
            for(i16 Y = startingY; Y != endingY && !tile; Y += YOffset)
            {
                for(i16 X = startingX; X != endingX && !tile; X += XOffset)
                {
                    if(X >= 0 && Y >= 0 && X < REACHABLE_TILEMAP_DIM && Y < REACHABLE_TILEMAP_DIM)
                    {
                        ReachableTile* boundTile = GetReachableTileRaw(brain, (u16) X, (u16) Y);
                        for(i16 testY = Y - YOffset; (testY != Y + YOffset) && !tile; testY += YOffset)
                        {
                            for(i16 testX = X - XOffset; (testX != X + XOffset) && !tile; testX += XOffset)
                            {
                                if(testX >= 0 && testY >= 0 && testX < REACHABLE_TILEMAP_DIM && testY < REACHABLE_TILEMAP_DIM)
                                {
                                    ReachableTile* testTile = GetReachableTileRaw(brain, (u16) testX, (u16) testY);
                                    if(IsReachable(testTile))
                                    {
                                        indexX = testX;
                                        indexY = testY;
                                        tile = testTile;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        else
        {
            tile = GetReachableTile(brain, relativeP, &indexX, &indexY);
            Assert(tile->directionToReachOrigin);
        }
        
        Assert(indexX >= 0);
        Assert(indexY >= 0);
        
        r32 stepLength = REACHABLE_TILEMAP_SPAN / REACHABLE_TILEMAP_DIM;
        brain->path.nodeCount = Min(GetStepsToReachOrigin(tile) + 1, ArrayCount(brain->path.X));
        while(true)
        {
            u8 stepsToReachOrigin = GetStepsToReachOrigin(tile);
            if(stepsToReachOrigin < ArrayCount(brain->path.X))
            {
                brain->path.X[stepsToReachOrigin] = indexX;
                brain->path.Y[stepsToReachOrigin] = indexY;
            }
            
            u8 direction  = GetDirection(tile);
            Vec3 acc = {};
            switch(direction)
            {
                case ReachableDir_Left:
                {
                    Assert(indexX > 0);
                    --indexX;
                    acc = V3(stepLength, 0, 0);
                } break;
                
                case ReachableDir_Right:
                {
                    Assert(indexX < (REACHABLE_TILEMAP_DIM - 1));
                    ++indexX;
                    acc = V3(-stepLength, 0, 0);
                } break;
                
                case ReachableDir_Down:
                {
                    Assert(indexY > 0);
                    --indexY;
                    acc = V3(0, stepLength, 0);
                } break;
                
                case ReachableDir_Up:
                {
                    Assert(indexY < (REACHABLE_TILEMAP_DIM - 1));
                    ++indexY;
                    acc = V3(0, -stepLength, 0);
                } break;
                
                InvalidDefaultCase;
            }
            
            if(stepsToReachOrigin < ArrayCount(brain->path.X))
            {
                brain->path.steps[stepsToReachOrigin] = acc;
            }
            
            
            if(!stepsToReachOrigin)
            {
                break;
            }
            
            tile = GetReachableTileRaw(brain, (u16) indexX, (u16) indexY);
        }
    }
}

inline b32 FollowPath(SimEntity* entity, Brain* brain, Vec3 currentP)
{
    b32 result = true;
    entity->action = Action_Move;
    Vec3 relativeP = currentP - brain->path.originP;
    
    i16 currentX;
    i16 currentY;
    
    ReachableTile* tile = GetReachableTile(brain, relativeP, &currentX, &currentY);
    
    u16 finalX = brain->path.X[brain->path.nodeCount - 1];
    u16 finalY = brain->path.Y[brain->path.nodeCount - 1];
    
    brain->path.currentNodeIndex = brain->path.nodeCount - 1;
    if(currentX != finalX || currentY != finalY)
    {
        for(u8 pathNodeIndex = 0; pathNodeIndex < (brain->path.nodeCount - 1); ++pathNodeIndex)
        {
            u16 X = brain->path.X[pathNodeIndex];
            u16 Y = brain->path.Y[pathNodeIndex];
            
            if(X == currentX && Y == currentY)
            {
                result = false;
                brain->path.currentNodeIndex = pathNodeIndex;
                break;
            }
        }
    }
    
    return result;
}


struct BreadthFirstQueueElement
{
    u16 X;
    u16 Y;
    u8 stepsToReachOrigin;
    
    union
    {
        BreadthFirstQueueElement* next;
        BreadthFirstQueueElement* nextFree;
    };
};

struct BreadthFirstQueue
{
    BreadthFirstQueueElement* first;
    BreadthFirstQueueElement* last;
    BreadthFirstQueueElement* firstFree;
    
    u32 elementUsedCount;
    BreadthFirstQueueElement elements[512];
};


inline void AddToQueue(BreadthFirstQueue* queue, u16 X, u16 Y, u8 stepsToReachOrigin)
{
    BreadthFirstQueueElement* element = 0;
    if(queue->firstFree)
    {
        element = queue->firstFree;
        queue->firstFree = element->nextFree;
    }
    else
    {
        Assert(queue->elementUsedCount < ArrayCount(queue->elements));
        element = queue->elements + queue->elementUsedCount++;
    }
    
    element->X = X;
    element->Y = Y;
    element->stepsToReachOrigin = stepsToReachOrigin;
    element->next = 0;
    
    if(!queue->last)
    {
        Assert(!queue->first);
        queue->first = queue->last = element;
    }
    else
    {
        queue->last->next = element;
        queue->last = element;
    }
    
}

inline BreadthFirstQueueElement GetFirst(BreadthFirstQueue* queue)
{
    BreadthFirstQueueElement* first = queue->first;
    BreadthFirstQueueElement result = *first;
    
    queue->first = first->next;
    first->nextFree = queue->firstFree;
    queue->firstFree = first;
    
    
    if(!queue->first)
    {
        Assert(queue->last == first);
        queue->last = 0;
    }
    
    return result;
}

internal void RecalculateObstacleMap(SimRegion* region, SimEntity* entity)
{
    CreatureComponent* creature = Creature(region, entity);
    Brain* brain = &creature->brain;
    
    ZeroSize(REACHABLE_TILEMAP_DIM * REACHABLE_TILEMAP_DIM, brain->reachableTiles);
    
    
    
    r32 obstacleLimit  = REACHABLE_TILEMAP_SPAN;
    Rect3 obstacleBounds = RectCenterDim(V3(0, 0, 0), V3(obstacleLimit, obstacleLimit, obstacleLimit));
    RegionPartitionQueryResult query = QuerySpacePartition(region, &region->collisionPartition, entity->P, V3(0, 0, 0), obstacleBounds);
    
    for(u32 surfaceIndex = 0; surfaceIndex < ArrayCount(query.surfaceIndexes); ++surfaceIndex)
    {
        RegionPartitionSurface* surface = region->collisionPartition.partitionSurfaces + query.surfaceIndexes[surfaceIndex];
        
        PartitionSurfaceEntityBlock* block = surface->first;
        while(block)
        {
            for(u32 blockIndex = 0; blockIndex < block->entityCount; ++blockIndex)
            {
                CollisionData* collider = block->colliders + blockIndex;
                SimEntity* test = GetRegionEntity(region, collider->entityIndex);
                
                if(test->boundType && test->identifier != entity->identifier)
                {
                    Vec3 relativeP = test->P - entity->P;
                    Rect3 minkowski = GetMinkowskiRect(entity->bounds, relativeP, test->bounds);
                    
                    if(!PointInRect(minkowski, entity->P))
                    {
                        i16 minX, minY, maxX, maxY;
                        GetReachableTile(brain, minkowski.min, &minX, &minY);
                        GetReachableTile(brain, minkowski.max, &maxX, &maxY);
                        for(i16 Y = minY; Y <= maxY; ++Y)
                        {
                            for(i16 X = minX; X <= maxX; ++X)
                            {
                                if(X >= 0 && Y >= 0 && X < REACHABLE_TILEMAP_DIM && Y < REACHABLE_TILEMAP_DIM)
                                {
                                    ReachableTile* tile = GetReachableTileRaw(brain, (u16) X, (u16) Y);
                                    SetTileAsUnreachable(tile);
                                }
                            }
                        }
                    }
                }
            }
            
            block = block->next;
        }
    }
    
    BreadthFirstQueue queue;
    queue.elementUsedCount = 0;
    queue.first = 0;
    queue.last = 0;
    queue.firstFree = 0;
    
    i16 originX, originY;
    ReachableTile* origin = GetReachableTile(brain, V3(0, 0, 0), &originX, &originY);
    
    SetTileAsReachable(origin, 0, 0);
    AddToQueue(&queue, (u16) originX, (u16) originY, 1);
    
    while(queue.first)
    {
        BreadthFirstQueueElement element = GetFirst(&queue);
        i16 tileX = (i16) element.X;
        i16 tileY = (i16) element.Y;
        u8 stepsToReachOrigin = element.stepsToReachOrigin;
        
        i16 tileXs[] = {tileX - 1, tileX + 1, tileX, tileX};
        i16 tileYs[] = {tileY, tileY, tileY + 1, tileY - 1};
        ReachableDirection directions[] = 
        {ReachableDir_Right, ReachableDir_Left, ReachableDir_Down, ReachableDir_Up};
        
        for(u32 directionIndex = 0; directionIndex < ArrayCount(directions); ++directionIndex)
        {
            i16 X = tileXs[directionIndex];
            i16 Y = tileYs[directionIndex];
            
            if(X >= 0 && Y >= 0 && X < REACHABLE_TILEMAP_DIM && Y < REACHABLE_TILEMAP_DIM)
            {
                ReachableTile* tile = GetReachableTileRaw(brain, (u16) X, (u16) Y);
                if(tile->directionToReachOrigin == 0)
                {
                    SetTileAsReachable(tile, (u8) directions[directionIndex], stepsToReachOrigin);
                    
                    u8 newSteps = Min(MAX_STEPS_TRACKED, stepsToReachOrigin + 1);
                    AddToQueue(&queue, (u16) X, (u16) Y, newSteps);
                }
            }
        }
    }
}

inline b32 Satisfied(Brain* brain, AICondition* condition)
{
    b32 result = false;
    switch(condition->type)
    {
        case AICondition_DoingActionFor:
        {
            if(brain->commandTime >= condition->data.time)
            {
                result = true;
            }
        } break;
        
        InvalidDefaultCase;
    }
    
    return result;
}

inline void ExecuteStateMachineAction(SimEntity* entity, AICommand command)
{
    switch(command.type)
    {
        case AIAction_Behavior:
        {
            InvalidCodePath;
        } break;
        
        case AIAction_StandardAction:
        {
            InvalidCodePath;
        } break;
        
        case AIAction_SpecialAction:
        {
            switch(command.data.specialAction)
            {
                case SpecialAction_MoveLeft:
                {
                    entity->action = Action_Move;
                    entity->acceleration = V3(-1, 0, 0);
                } break;
                
                case SpecialAction_MoveRight:
                {
                    entity->action = Action_Move;
                    entity->acceleration = V3(1, 0, 0);
                } break;
                
                InvalidDefaultCase;
            }
        } break;
        
        default:
        {
            entity->action = Action_Idle;
            entity->acceleration = {};
        } break;
    }
}

inline void HandleStateMachine(Brain* brain, AIStateMachine* stateMachine, r32 timeToUpdate)
{
    AIAction* action = stateMachine->actions + brain->currentActionIndex;
    brain->currentCommand = action->command;
    brain->commandTime += timeToUpdate;
    
    for(u32 transitionIndex = 0; transitionIndex < action->transitionCount; ++transitionIndex)
    {
        AIStateMachineTransition* transition = action->transitions + transitionIndex;
        
        Assert(transition->conditionCount == 1);
        if(Satisfied(brain, transition->conditions))
        {
            brain->commandTime = 0;
            brain->currentActionIndex = transition->destActionIndex;
        }
    }
}

internal void HandleAI(SimRegion* region, SimEntity* entity)
{
    CreatureComponent* creature = Creature(region, entity);
    Brain* brain = &creature->brain;
    brain->timeSinceLastUpdate += region->timeToUpdate;
    
    r32 brainTargetSeconds = 1.0f;
    if(brain->timeSinceLastUpdate >= brainTargetSeconds)
    {
        brain->timeSinceLastUpdate = 0;
        brain->path.nodeCount = 0;
        RecalculateObstacleMap(region, entity);
        
        HandleStateMachine(brain, brain->stateMachine, brainTargetSeconds);
    }
    
    ExecuteStateMachineAction(entity, brain->currentCommand);
}
