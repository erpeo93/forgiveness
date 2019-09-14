internal Vec3 MoveEntityClient(GameModeWorld* worldMode, ClientEntity* entity, r32 timeToAdvance, Vec3 acceleration, Vec3 velocity, Vec3* velocityToUpdate)
{
    Vec3 result = {};
    
    Vec3 deltaP = 0.5f * acceleration * Square(timeToAdvance) + velocity * timeToAdvance;
    r32 tRemaining = 1.0f;
    for(u32 iteration = 0; (iteration < 2) && tRemaining > 0; iteration++)
    {
        r32 tStop = tRemaining;
        Vec3 wallNormalMin = {};
        
        for(u32 entityIndex = 0; entityIndex < ArrayCount(worldMode->entities); ++entityIndex)
        {
            ClientEntity* entityToCheck = worldMode->entities[entityIndex];
            while(entityToCheck)
            {
                if(ShouldCollide(entity->identifier, entity->boundType, entityToCheck->identifier, entityToCheck->boundType))
                {
                    CheckCollisionCurrent ignored = {};
                    HandleVolumeCollision(entity->P, entity->bounds, deltaP, entityToCheck->P, entityToCheck->bounds, &tStop, &wallNormalMin, ignored);
                }
                
                entityToCheck = entityToCheck->next;
            }
        }
        
        Vec3 wallNormal = wallNormalMin;
        result += tStop * deltaP;
        
        *velocityToUpdate = *velocityToUpdate - Dot(entity->velocity, wallNormal) * wallNormal;
        deltaP = deltaP - Dot(deltaP, wallNormal) * wallNormal;
        tRemaining -= tStop * tRemaining;
    }
    
    return result;
}

inline b32 TooFarForAction(ClientPlayer* myPlayer, u32 desiredAction, u64 targetID)
{
    b32 result = false;
    if(desiredAction >= Action_Attack && targetID)
    {
        if(targetID == myPlayer->targetIdentifier && myPlayer->targetPossibleActions[desiredAction] == PossibleAction_TooFar)
        {
            result = true;
        }
    }
    
    return result;
}

inline b32 NearEnoughForAction(ClientPlayer* myPlayer, u32 desiredAction, u64 targetID, b32* resetAcceleration)
{
    b32 result = false;
    if(desiredAction >= Action_Attack)
    {
        if(targetID)
        {
            if(targetID == myPlayer->targetIdentifier && myPlayer->targetPossibleActions[desiredAction] == PossibleAction_CanBeDone)
            {
                *resetAcceleration = true;
                result = true;
            }
        }
    }
    else
    {
        if(desiredAction > Action_Move)
        {
            if(desiredAction == Action_Rolling)
            {
                *resetAcceleration = false;
            }
            else
            {
                *resetAcceleration = true;
            }
            result = true;
        }
    }
    
    return result;
}

