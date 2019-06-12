inline r32 Score(ExpressionContext* context, AIAction* action, r32 bonus, r32 min)
{
    r32 result = action->importance + bonus;
    Assert(action->considerationCount > 0);
    for(u32 considerationIndex = 0; (considerationIndex < action->considerationCount) && (result > min); ++considerationIndex)
    {
        Consideration* consideration = action->considerations + considerationIndex;
        context->params = consideration->params;
        result *= EvaluateConsideration(context, consideration);
    }
    
    
    
    return result;
}

inline AIConceptTargets* AIGetConceptTargets(AIConceptTargets* targets, u32 conceptTargetsCount, u32 conceptTaxonomy)
{
    AIConceptTargets* result = 0;
    for(u32 conceptIndex = 0; conceptIndex < conceptTargetsCount; ++conceptIndex)
    {
        AIConceptTargets* targetsProbe = targets + conceptIndex;
        if(conceptTaxonomy == targetsProbe->conceptTaxonomy)
        {
            result = targetsProbe;
        }
    }
    
    return result;
}


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


inline b32 CommandRequiresTarget(AICommand command)
{
    b32 result = (command.action != Action_None && command.action != Action_Idle && command.action != Action_Move);
    return result;
}

inline r32 Score(SimRegion* region, SimEntity* entity, BrainBehavior* behavior, AIAction* action, r32 bonus, r32 min, r32 scoreBonusPercentageIfTargetMatch = 0.2f)
{
    r32 result = 0;
    
    CreatureComponent* creature = Creature(region, entity);
    Brain* brain = &creature->brain;
    
    ExpressionContext context = {};
    context.region = region;
    context.self = entity;
    
    AIConceptTargets* targets = AIGetConceptTargets(brain->conceptTargets, ArrayCount(brain->conceptTargets), action->associatedConcept);
    if(targets)
    {
        for(u32 targetIndex = 0; targetIndex < targets->targetCount; ++targetIndex)
        {
            SimEntity* target = GetRegionEntityByID(region, targets->targets[targetIndex]);
            if(target)
            {
                Vec3 relativeP = target->P - entity->P;
                if(Reachable(brain, relativeP, entity->boundType, entity->bounds, target->boundType, target->bounds))
                {
                    Assert(context.targetCount < ArrayCount(context.targets));
                    if(context.targetCount < ArrayCount(context.targets))
                    {
                        context.targets[context.targetCount++] = target;
                    }
                    else
                    {
                        break;
                    }
                }
            }
        }
    }
    
    if(action->type == AIAction_Command)
    {
        if(CommandRequiresTarget(action->command))
        {
            action->command.targetID = 0;
            for(u32 targetIndex = 0; targetIndex < context.targetCount; ++targetIndex)
            {
                SimEntity* target = context.targets[targetIndex];
                context.target = target;
                r32 targetScore = Score(&context, action, bonus, min);
                if(target->identifier == entity->targetID)
                {
                    targetScore += (scoreBonusPercentageIfTargetMatch * targetScore);
                }
                if(targetScore > result)
                {
                    action->command.targetID = target->identifier;
                    result = targetScore;
                }
            }
        }
        else
        {
            result = Score(&context, action, bonus, min);
        }
    }
    else
    {
        result = Score(&context, action, bonus, min);
    }
    
    return result;
}


inline void PushAIBehavior(TaxonomyTable* table, Brain* brain, u32 behaviorTaxonomy)
{
    TaxonomySlot* slot = GetSlotForTaxonomy(table, behaviorTaxonomy);
    AIBehavior* behavior = slot->behaviorContent;
    
    Assert(behavior->actionCount > 0);
    Assert(brain->behaviorCount < ArrayCount(brain->behaviorStack));
    BrainBehavior* dest = brain->behaviorStack + brain->behaviorCount++;
    
    dest->taxonomy = behaviorTaxonomy;
    dest->importance = 0;
}


internal void ExecuteAction(SimRegion* region, SimEntity* entity, AICommand command, r32 maxDistanceSq)
{
    CreatureComponent* creature = Creature(region, entity);
    Brain* brain = &creature->brain;
    u64 targetID = command.targetID;
    if(targetID)
    {
        SimEntity* target = GetRegionEntityByID(region, targetID);
        if(target)
        {
            Vec3 toTarget = target->P - entity->P;
            if(LengthSq(toTarget) <= maxDistanceSq)
            {
                entity->action = command.action;
                entity->targetID = command.targetID;
                entity->acceleration = V3(0, 0, 0);
            }
            else
            {
                entity->action = Action_Move;
                
                b32 directMovement = false;
                if(entity->boundType)
                {
                    if(FollowPath(entity, brain, entity->P))
                    {
                        directMovement = true;
                    }
                }
                else
                {
                    directMovement = true;
                }
                
                if(directMovement)
                {
                    entity->acceleration = toTarget;
                }
                else
                {
                    entity->acceleration = {};
                }
            }
        }
        else
        {
            entity->targetID = 0;
            entity->action = Action_Idle;
        }
    }
    else
    {
        
#if 0        
        if(command.type == AIAction_Destination)
        {
            FollowPath();
        }
        else
#endif
        
        {
            entity->targetID = 0;
            entity->action = command.action;
        }
    }
}

#if 0
enum GeographicScale
{
    Tile,
    Chunk,
    Islands
}

internal void RecalculateInfluenceMap(SimRegion* region, SimEntity* entity, InfluenceMap* dest. b32 considerMemory, GeographycScale scale)
{
    for(everyEntity)
    {
        Vec3 relativeP = target->P - entity->P;
        u32 index = MapPToIndex(relativeP);
        
        r32 influence = ?;
        u8 influenceInt = ?;
        
        u32 influenceBif = Max(0xff, dest->values[index] + influenceInt);
        dest->values[index] += SafeTruncateToU8(influenceBig);
    }
    
    if(considerMemory)
    {
        AddToInfluenceBasedOnMemoryAsWell();
    }
}
#endif

inline MemCriteria* AIGetCriteria(TaxonomyTable* table, u32 taxonomy, u32 criteriaTaxonomy)
{
    MemCriteria* result = 0;
    
    u32 currentTaxonomy = taxonomy;
    while(currentTaxonomy && !result)
    {
        TaxonomySlot* slot = GetSlotForTaxonomy(table, currentTaxonomy);
        for(TaxonomyMemBehavior* behavior = slot->firstMemBehavior; behavior && !result; behavior = behavior->next)
        {
            TaxonomySlot* behaviorSlot = GetSlotForTaxonomy(table, behavior->taxonomy);
            for(MemCriteria* criteria = behaviorSlot->criteria; criteria; criteria = criteria->next)
            {
                if(criteria->taxonomy == criteriaTaxonomy)
                {
                    result = criteria;
                    break;
                }
            }
        }
        
        currentTaxonomy = GetParentTaxonomy(table, currentTaxonomy);
    }
    
    return result;
}

internal void FillTargets(SimRegion* region, SimEntity* entity)
{	
    CreatureComponent* creature = Creature(region, entity);
    Brain* brain = &creature->brain;
    
    r32 targetLimit = REACHABLE_TILEMAP_SPAN;
    Rect3 targetBounds = RectCenterDim(V3(0, 0, 0), V3(targetLimit, targetLimit, targetLimit));
    RegionPartitionQueryResult query = QuerySpacePartition(region, &region->collisionPartition, entity->P, V3(0, 0, 0), targetBounds);
    for(u32 surfaceIndex = 0; surfaceIndex < ArrayCount(query.surfaceIndexes); ++surfaceIndex)
    {
        RegionPartitionSurface* surface = region->collisionPartition.partitionSurfaces + query.surfaceIndexes[surfaceIndex];
        
        PartitionSurfaceEntityBlock* block = surface->first;
        while(block)
        {
            for(u32 blockIndex = 0; blockIndex < block->entityCount; ++blockIndex)
            {
                CollisionData* collider = block->colliders + blockIndex;
                SimEntity* probe = GetRegionEntity(region, collider->entityIndex);
                MemAssociation* associations = MemGetAssociations(&brain->memory, probe->identifier);
                for(u32 conceptIndex = 0; conceptIndex < ArrayCount(brain->conceptTargets); ++conceptIndex)
                {
                    AIConceptTargets* targets = brain->conceptTargets + conceptIndex;
                    if(targets->conceptTaxonomy)
                    {
                        if(targets->targetCount < ArrayCount(targets->targets))
                        {
                            MemCriteria* criteria = AIGetCriteria(region->taxTable, entity->taxonomy, targets->conceptTaxonomy);
                            if(criteria)
                            {
                                for(u32 possibleRequiredIndex = 0; possibleRequiredIndex < criteria->possibleTaxonomiesCount; ++possibleRequiredIndex)
                                {
                                    if(MemAssociationPresent(&brain->memory, criteria->requiredConceptTaxonomy[possibleRequiredIndex], probe->identifier))
                                    {
                                        targets->targets[targets->targetCount++] = probe->identifier;
                                        break;
                                    }
                                }
                            }
                            else
                            {
                                InvalidCodePath;
                            }
                        }
                        else
                        {
                            //InvalidCodePath;
                        }
                    }
                }
            }
            
            block = block->next;
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
        MemUpdate(region->taxTable, &brain->memory, 
                  entity->taxonomy, region->timeToUpdate);
        
        brain->timeSinceLastUpdate -= brainTargetSeconds;
        brain->path.nodeCount = 0;
        RecalculateObstacleMap(region, entity);
        
#if 0    
        for(u32 criteriaIndex = 0; criteriaIndex < behavior->criteriaCount; ++criteriaIndex)
        {
            RecalculateInfluenceMap(criteriaIndex);
        }
#endif
        for(u32 conceptIndex = 0; conceptIndex < ArrayCount(brain->conceptTargets); ++conceptIndex)
        {
            brain->conceptTargets[conceptIndex] = brain->newConceptTargets[conceptIndex];
            brain->newConceptTargets[conceptIndex].conceptTaxonomy = 0;
        }
        FillTargets(region, entity);
        u32 currentConceptIndex = 0;
        
        
        
        
        
        AIAction* todo = 0;
        u32 todoStackIndex = 0;
        
        b32 pushedAnything = false;
        for(u32 stackIndex = 0; stackIndex < brain->behaviorCount && !pushedAnything; ++stackIndex)
        {
            AIAction* todoAction = 0;
            u32 todoActionIndex = 0;
            r32 bestScore = 0;
            
            BrainBehavior* behavior = brain->behaviorStack + stackIndex;
            TaxonomySlot* slot = GetSlotForTaxonomy(region->taxTable, behavior->taxonomy);
            AIBehavior* b = slot->behaviorContent;
            
            
            for(u32 actionIndex = 0; actionIndex < b->actionCount; ++actionIndex)
            {
                AIAction* action = b->actions + actionIndex;
                if(action->associatedConcept)
                {
                    AIConceptTargets* targets = AIGetConceptTargets(brain->newConceptTargets, ArrayCount(brain->newConceptTargets), action->associatedConcept);
                    if(!targets)
                    {
                        if(currentConceptIndex < ArrayCount(brain->conceptTargets))
                        {
                            AIConceptTargets* newTargets = brain->newConceptTargets + currentConceptIndex++;
                            newTargets->targetCount = 0;
                            newTargets->conceptTaxonomy = action->associatedConcept;
                        }
                        else
                        {
                            InvalidCodePath;
                        }
                    }
                }
                
                
                r32 importance = action->importance;
                if(behavior->importance > 0 && actionIndex == behavior->activeActionIndex)
                {
                    importance = behavior->importance;
                }
                
                r32 bonus = 0.0f;
                r32 score = Score(region, entity, behavior, action, bonus, bestScore);
                if(score > bestScore)
                {
                    todoStackIndex = stackIndex;
                    todoAction = action;
                    todoActionIndex = actionIndex;
                    bestScore = score;
                }
            }
            
            if(todoAction)
            {
                if(todoAction->type == AIAction_Command || todoAction->type == AIAction_Destination)
                {
                    todo = todoAction;
                    brain->behaviorCount = todoStackIndex + 1;
                    pushedAnything = true;
                }
                else
                {
                    if(todoActionIndex != behavior->activeActionIndex)
                    {
                        brain->behaviorCount = todoStackIndex + 1;
                        PushAIBehavior(region->taxTable, brain, todoAction->behaviorTaxonomy);
                        behavior->activeActionIndex = todoActionIndex;
                        pushedAnything = true;
                    }
                }
            }
        }
        
        if(todo)
        {
            for(u32 backwardStackIndex = todoStackIndex; backwardStackIndex > 0; --backwardStackIndex)
            {
                BrainBehavior* behavior = brain->behaviorStack + backwardStackIndex;
                behavior->importance = todo->importance;
            }
            
            
#if 0            
            if(todoAction->type == AIAction_Destination)
            {
                ScoreDestinationexpression();
            }
            
            else
#endif
            
            {
                entity->targetID = todo->command.targetID;
                if(entity->targetID)
                {
                    brain->desiredCommand = todo->command;
                    SimEntity* target = GetRegionEntityByID(region, todo->command.targetID);
                    BuildPathToReach(brain, entity->P, entity->boundType, entity->bounds, target->P, target->boundType, target->bounds);
                }
            }
        }
    }
    
    
    r32 maxDistanceSq = Square(1.5f);
    ExecuteAction(region, entity, brain->desiredCommand, maxDistanceSq);
}






