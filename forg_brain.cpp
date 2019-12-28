internal BrainParams* GetBrainParams(ServerState* server, EntityID ID)
{
    DefaultComponent* def = GetComponent(server, ID, DefaultComponent);
    EntityDefinition* entityDef = GetEntityTypeDefinition(server->assets, def->definitionID);
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

internal BrainDirection* GetRightDirection(BrainComponent* brain, Vec3 direction)
{
    u16 index = GetDirectionIndex(direction);
    if(++index == ArrayCount(brain->directions))
    {
        index = 0;
    }
    
    BrainDirection* result = GetDirection(brain, index);
    return result;
}

internal BrainDirection* GetLeftDirection(BrainComponent* brain, Vec3 direction)
{
    u16 index = GetDirectionIndex(direction);
    if(--index >= ArrayCount(brain->directions))
    {
        index = ArrayCount(brain->directions) - 1;
    }
    
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


internal void ModulateDirection(BrainComponent* brain, Vec3 direction, r32 coeff, r32 adiacentCoeff)
{
    BrainDirection* dir = GetDirection(brain, direction);
    dir->coeff *= coeff;
    
    if(adiacentCoeff > 0)
    {
        BrainDirection* left = GetLeftDirection(brain, direction);
        left->coeff *= (coeff * adiacentCoeff);
        
        BrainDirection* right = GetRightDirection(brain, direction);
        right->coeff *= (coeff * adiacentCoeff);
    }
}

internal void ZeroDirection(BrainComponent* brain, Vec3 direction, b32 adiacentDirections)
{
    r32 adiacentCoeff = adiacentDirections ? 1.0f : 0.0f;
    ModulateDirection(brain, direction, 0, adiacentCoeff);
}

internal void SumScore(BrainComponent* brain, Vec3 direction, r32 score, r32 adiacentDirectionCoeff)
{
    BrainDirection* dir = GetDirection(brain, direction);
    dir->sum += score;
    
    dir = GetRightDirection(brain, direction);
    dir->sum += score * adiacentDirectionCoeff;
    
    dir = GetLeftDirection(brain, direction);
    dir->sum += score * adiacentDirectionCoeff;
}

internal void ScoreDirection(ServerState* server, EntityID ID, EntityID targetID, r32 score, r32 adiacentDirectionCoeff)
{
    BrainComponent* brain = GetComponent(server, ID, BrainComponent);
    DefaultComponent* def = GetComponent(server, ID, DefaultComponent);
    DefaultComponent* targetDef = GetComponent(server, targetID, DefaultComponent);
    Vec3 toTarget = SubtractOnSameZChunk(targetDef->P, def->P);
    SumScore(brain, toTarget, score, adiacentDirectionCoeff);
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

EVALUATOR(CollisionAvoidance)
{
    DefaultComponent* def = GetComponent(server, ID, DefaultComponent);
    PhysicComponent* physic = GetComponent(server, ID, PhysicComponent);
    BrainComponent* brain = GetComponent(server, ID, BrainComponent);
    
    if(physic)
    {
        r32 minDistanceSq = R32_MAX;
        EntityID avoidID = {};
        Vec3 avoidV = {};
        
        r32 seeAhead = 3.0f;
        u32 segmentCount = 8;
        Vec3 normalizedSpeed = Normalize(physic->speed);
        r32 increment = 1.0f / segmentCount;
        
        for(u32 avoidIndex = 0; avoidIndex < segmentCount && !IsValidID(avoidID); ++avoidIndex)
        {
            Vec3 avoid = seeAhead * normalizedSpeed * (increment * (avoidIndex + 1));
            UniversePos ahead = Offset(def->P, avoid);
            SpatialPartitionQuery avoidQuery = QuerySpatialPartitionAtPoint(&server->standardPartition, ahead);
            for(EntityID testID = GetCurrent(&avoidQuery); IsValid(&avoidQuery); testID = Advance(&avoidQuery))
            {
                DefaultComponent* testDef = GetComponent(server, testID, DefaultComponent);
                PhysicComponent* testPhysic = GetComponent(server, testID, PhysicComponent);
                if(ShouldPerceive(def, ID, testDef, testID))
                {
                    if(ShouldCollide(physic->boundType, testPhysic->boundType))
                    {
                        Vec3 distance = SubtractOnSameZChunk(testDef->P, ahead);
                        r32 distanceSq = LengthSq(distance);
                        r32 testRadiousSq =(Square(Max(GetDim(testPhysic->bounds).x, GetDim(testPhysic->bounds).y)) + 
                                            Square(Max(GetDim(physic->bounds).x, GetDim(physic->bounds).y)));
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

EVALUATOR(Wander)
{
    BrainComponent* brain = GetComponent(server, ID, BrainComponent);
    BrainParams* params = GetBrainParams(server, ID);
    
    brain->time += elapsedTime;
    if(brain->time >= 0)
    {
        r32 score = 1.0f;
        r32 adiacentDirectionCoeff = 0.5f;
        SumScore(brain, brain->wanderDirection, score, adiacentDirectionCoeff);
        
        if(brain->time >= params->wanderTargetTime)
        {
            brain->time = -params->idleTimeWhenWandering;
            brain->wanderDirection = V3(RandomBil(&server->entropy), RandomBil(&server->entropy), 0);
        }
    }
}

EVALUATOR(MaintainDistance)
{
    DefaultComponent* def = GetComponent(server, ID, DefaultComponent);
    BrainComponent* brain = GetComponent(server, ID, BrainComponent);
    BrainParams* params = GetBrainParams(server, ID);
    
    SpatialPartitionQuery perceiveQuery = QuerySpatialPartitionAtPoint(&server->standardPartition, def->P);
    
    for(EntityID testID = GetCurrent(&perceiveQuery); IsValid(&perceiveQuery); testID = Advance(&perceiveQuery))
    {
        DefaultComponent* testDef = GetComponent(server, testID, DefaultComponent);
        PhysicComponent* testPhysic = GetComponent(server, testID, PhysicComponent);
        if(ShouldPerceive(def, ID, testDef, testID))
        {
            Vec3 toTarget = SubtractOnSameZChunk(testDef->P, def->P);
            if(AreEqual(testDef->definitionID, params->maintainDistanceType))
            {
                if(LengthSq(toTarget) <= Square(params->maintainDistanceRadious))
                {
                    ModulateDirection(brain, -toTarget, params->maintainDistanceModulationCoeff, params->maintainDistanceModulationAdiacentCoeff);
                }
            }
            else if(EntityHasFlags(def, EntityFlag_fearsLight))
            {
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
    
    for(EntityID testID = GetCurrent(&perceiveQuery); IsValid(&perceiveQuery); testID = Advance(&perceiveQuery))
    {
        DefaultComponent* testDef = GetComponent(server, testID, DefaultComponent);
        PhysicComponent* testPhysic = GetComponent(server, testID, PhysicComponent);
        if(ShouldPerceive(def, ID, testDef, testID))
        {
            Vec3 toTarget = SubtractOnSameZChunk(testDef->P, def->P);
            
            if(EntityHasFlags(def, EntityFlag_fearsLight) && LengthSq(toTarget) <= GetLightRadiousSq(server, testID))
            {
                ZeroDirection(brain, toTarget, true);
                SumScore(brain, -toTarget, 1.0f, 0.5f);
            }
            else if(AreEqual(testDef->definitionID, params->scaryType))
            {
                if(LengthSq(toTarget) <= Square(params->safeDistanceRadious))
                {
                    ZeroDirection(brain, toTarget, true);
                    SumScore(brain, -toTarget, 1.0f, 0.5f);
                }
            }
        }
    }
}

internal b32 SearchForHostileEnemies(ServerState* server, EntityID brainID, EntityID* ID)
{
    b32 result = false;
    DefaultComponent* def = GetComponent(server, brainID, DefaultComponent);
    PhysicComponent* physic = GetComponent(server, brainID, PhysicComponent);
    BrainParams* params = GetBrainParams(server, brainID);
    SpatialPartitionQuery perceiveQuery = QuerySpatialPartitionAtPoint(&server->standardPartition, def->P);
    
    for(EntityID testID = GetCurrent(&perceiveQuery); IsValid(&perceiveQuery); testID = Advance(&perceiveQuery))
    {
        DefaultComponent* testDef = GetComponent(server, testID, DefaultComponent);
        PhysicComponent* testPhysic = GetComponent(server, testID, PhysicComponent);
        if(ShouldPerceive(def, brainID, testDef, testID))
        {
            if(AreEqual(testDef->definitionID, params->hostileType))
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
    
    for(EntityID testID = GetCurrent(&perceiveQuery); IsValid(&perceiveQuery); testID = Advance(&perceiveQuery))
    {
        DefaultComponent* testDef = GetComponent(server, testID, DefaultComponent);
        PhysicComponent* testPhysic = GetComponent(server, testID, PhysicComponent);
        if(ShouldPerceive(def, brainID, testDef, testID))
        {
            Vec3 toTarget = SubtractOnSameZChunk(testDef->P, def->P);
            if(EntityHasFlags(def, EntityFlag_fearsLight) && LengthSq(toTarget) <= GetLightRadiousSq(server, testID))
            {
                result = true;
                break;
            }
            else if(AreEqual(testDef->definitionID, params->scaryType))
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

internal b32 ActionIsPossible(ServerState* server, u16 action, EntityID ID, EntityID targetID)
{
    b32 result = false;
    
    InteractionComponent* targetInteraction = GetComponent(server, targetID, InteractionComponent);
    UsingComponent* equippedComponent = GetComponent(server, ID, UsingComponent);
    
    EntityRef equipped[Count_usingSlot];
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
    MiscComponent* misc = GetComponent(server, ID, MiscComponent);
    UsingComponent* equippedComponent = GetComponent(server, ID, UsingComponent);
    
    DefaultComponent* targetDef = GetComponent(server, targetID, DefaultComponent);
    InteractionComponent* targetInteraction = GetComponent(server, targetID, InteractionComponent);
    
    Vec3 toTarget = SubtractOnSameZChunk(targetDef->P, def->P);
    r32 distanceSq = LengthSq(toTarget);
    r32 targetTime;
    
    EntityRef equipped[Count_usingSlot];
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
    if(ActionIsPossibleAtDistance(targetInteraction, action, oldAction, distanceSq, &targetTime, misc, equipped, equippedCount))
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
                        EntityRef type = EntityReference(server->assets, "default", "wolf");
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
                switch(brain->state)
                {
                    case BrainState_Wandering:
                    {
                        Evaluate(Wander);
                        Evaluate(MaintainDistance);
                        Evaluate(CollisionAvoidance);
                        
                        EntityID hostileID;
                        if(SearchForHostileEnemies(server, ID, &hostileID))
                        {
                            ChangeState(brain, Chasing);
                            brain->targetID = hostileID;
                        }
                        
                        if(SearchForScaryEntities(server, ID))
                        {
                            ChangeState(brain, Fleeing);
                        }
                    } break;
                    
                    case BrainState_Fleeing:
                    {
                        Evaluate(Flee);
                        if(!SearchForScaryEntities(server, ID))
                        {
                            ChangeState(brain, Wandering);
                        }
                    } break;
                    
                    case BrainState_Chasing:
                    {
                        ScoreDirection(server, ID, brain->targetID, 1.0f, 0.99f);
                        Evaluate(MaintainDistance);
                        Evaluate(CollisionAvoidance);
                        
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
                        
                        if(SearchForScaryEntities(server, ID))
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
                        
                        if(SearchForScaryEntities(server, ID))
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