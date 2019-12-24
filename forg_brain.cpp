internal BrainParams* GetBrainParams(ServerState* server, EntityID ID)
{
    DefaultComponent* def = GetComponent(server, ID, DefaultComponent);
    EntityDefinition* entityDef = GetEntityTypeDefinition(server->assets, def->definitionID);
    BrainParams* result = &entityDef->server.brainParams;
    return result;
}

internal void Idle(ServerState* server, EntityID ID)
{
    BrainComponent* brain = GetComponent(server, ID, BrainComponent);
    
    brain->currentCommand = {};
    brain->currentCommand.action = idle;
    brain->commandParameters.acceleration = V3(0, 0, 0);
}

internal void ResetMovementDirection(ServerState* server, EntityID ID)
{
    BrainComponent* brain = GetComponent(server, ID, BrainComponent);
    brain->commandParameters.acceleration = {};
}

internal void ConsiderMovingDirection(ServerState* server, EntityID ID, Vec3 direction)
{
    BrainComponent* brain = GetComponent(server, ID, BrainComponent);
    brain->currentCommand = {};
    brain->currentCommand.action = move;
    
    Assert(direction.z == 0);
    r32 epsilon = 0.0001f;
    if(Abs(Dot(brain->commandParameters.acceleration, direction) + 1) < epsilon)
    {
        Idle(server, ID);
    }
    else
    {
        brain->commandParameters.acceleration += direction;
    }
}

internal void Attack(ServerState* server, EntityID ID, EntityID targetID)
{
    BrainComponent* brain = GetComponent(server, ID, BrainComponent);
    
    brain->currentCommand = {};
    brain->currentCommand.action = attack;
    brain->currentCommand.targetID = targetID;
    brain->commandParameters = {};
}

internal b32 ShouldPerceive(DefaultComponent* d1, EntityID i1, DefaultComponent* d2, EntityID i2)
{
    b32 result = (!AreEqual(i1, i2) && d1->P.chunkZ == d2->P.chunkZ);
    return result;
    
}

internal void ComputeFinalAcceleration(ServerState* server, EntityID ID)
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
                                avoidID = testID;
                                minDistanceSq = distanceSq;
                                avoidV = avoid;
                            }
                        }
                    }
                }
            }
        }
        
        if(IsValidID(avoidID))
        {
            ResetMovementDirection(server, ID);
            DefaultComponent* avoidDef = GetComponent(server, avoidID, DefaultComponent);
            Vec3 toAvoid = SubtractOnSameZChunk(avoidDef->P, def->P);
            Vec3 avoidanceForce = V3(Perp(toAvoid.xy), toAvoid.z);
            if(Dot(avoidanceForce, physic->acc) < 0)
            {
                avoidanceForce = -avoidanceForce;
            }
            
            ConsiderMovingDirection(server, ID, avoidanceForce);
        }
    }
}

internal void Wander(ServerState* server, EntityID ID, r32 elapsedTime)
{
    BrainComponent* brain = GetComponent(server, ID, BrainComponent);
    BrainParams* params = GetBrainParams(server, ID);
    
    brain->time += elapsedTime;
    if(brain->time >= 0)
    {
        ConsiderMovingDirection(server, ID, brain->wanderDirection);
        if(brain->time >= params->wanderTargetTime)
        {
            brain->time = -params->idleTimeWhenWandering;
            brain->wanderDirection = V3(RandomBil(&server->entropy), RandomBil(&server->entropy), 0);
        }
    }
    else
    {
        Idle(server, ID);
    }
}

internal void MaintainDistance(ServerState* server, EntityID ID)
{
    DefaultComponent* def = GetComponent(server, ID, DefaultComponent);
    BrainParams* params = GetBrainParams(server, ID);
    
    SpatialPartitionQuery perceiveQuery = QuerySpatialPartitionAtPoint(&server->standardPartition, def->P);
    
    for(EntityID testID = GetCurrent(&perceiveQuery); IsValid(&perceiveQuery); testID = Advance(&perceiveQuery))
    {
        DefaultComponent* testDef = GetComponent(server, testID, DefaultComponent);
        PhysicComponent* testPhysic = GetComponent(server, testID, PhysicComponent);
        if(ShouldPerceive(def, ID, testDef, testID))
        {
            Vec3 toTarget = SubtractOnSameZChunk(testDef->P, def->P);
            if(EntityHasFlags(def, EntityFlag_fearsLight))
            {
                if(LengthSq(toTarget) <= Square(LIGHT_RADIOUS_PERCENTAGE_MAINTAIN_DISTANCE * GetLightRadiousSq(server, testID)))
                {
                    ConsiderMovingDirection(server, ID, -Normalize(toTarget));
                }
            }
            else if(AreEqual(testDef->definitionID, params->maintainDistanceType))
            {
                if(LengthSq(toTarget) <= Square(params->maintainDistanceRadious))
                {
                    ConsiderMovingDirection(server, ID, -Normalize(toTarget));
                }
            }
        }
    }
}

internal void MoveToward(ServerState* server, EntityID ID, EntityID targetID)
{
    DefaultComponent* def = GetComponent(server, ID, DefaultComponent);
    DefaultComponent* targetDef = GetComponent(server, targetID, DefaultComponent);
    Vec3 toTarget = SubtractOnSameZChunk(targetDef->P, def->P);
    ConsiderMovingDirection(server, ID, Normalize(toTarget));
}

internal void MoveInDirection(ServerState* server, EntityID ID, Vec3 direction)
{
    ConsiderMovingDirection(server, ID, direction);
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

internal b32 Flee(ServerState* server, EntityID brainID, Vec3* scaryDirection)
{
    b32 result = false;
    
    *scaryDirection = {};
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
                *scaryDirection += OneOver(toTarget);
            }
            else if(AreEqual(testDef->definitionID, params->scaryType))
            {
                if(LengthSq(toTarget) <= Square(params->safeDistanceRadious))
                {
                    result = true;
                    *scaryDirection += OneOver(toTarget);
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
                    if(!IsValidID(brain->ID))
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
                    if(IsValidID(brain->ID))
                    {
                        DeleteEntity(server, brain->ID);
                        brain->ID = {};
                    }
                }
            }
            else
            {
                if(IsValidID(brain->ID))
                {
                    DeleteEntity(server, brain->ID);
                    brain->ID = {};
                }
            }
        } break;
        
        case Brain_stateMachine:
        {
            if(server->updateBrains)
            {
                ResetMovementDirection(server, ID);
                switch(brain->state)
                {
                    case BrainState_Wandering:
                    {
                        Wander(server, ID, elapsedTime);
                        MaintainDistance(server, ID);
                        ComputeFinalAcceleration(server, ID);
                        
                        EntityID hostileID;
                        if(SearchForHostileEnemies(server, ID, &hostileID))
                        {
                            ChangeState(brain, Chasing);
                            brain->ID = hostileID;
                        }
                        
                        if(SearchForScaryEntities(server, ID))
                        {
                            ChangeState(brain, Fleeing);
                        }
                    } break;
                    
                    case BrainState_Fleeing:
                    {
                        Vec3 scaryDirection;
                        if(Flee(server, ID, &scaryDirection))
                        {
                            MoveInDirection(server, ID, -scaryDirection);
                            ComputeFinalAcceleration(server, ID);
                        }
                        else
                        {
                            ChangeState(brain, Wandering);
                        }
                    } break;
                    
                    case BrainState_Chasing:
                    {
                        MoveToward(server, ID, brain->ID);
                        MaintainDistance(server, ID);
                        ComputeFinalAcceleration(server, ID);
                        
                        if(ActionIsPossible(server, attack, ID, brain->ID))
                        {
                            if(ActionIsPossibleAtCurrentDistance(server, attack, ID, brain->ID))
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
                        if(ActionIsPossibleAtCurrentDistance(server, attack, ID, brain->ID))
                        {
                            Attack(server, ID, brain->ID);
                        }
                        else
                        {
                            if(ActionIsPossible(server, attack, ID, brain->ID))
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