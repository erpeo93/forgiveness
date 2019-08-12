
inline ClientEntity* GetEntityClient(GameModeWorld* worldMode, u64 identifier, b32 allocate = false)
{
    ClientEntity* result = 0;
    if(identifier)
    {
        u32 index = identifier & (ArrayCount(worldMode->entities) - 1);
        
        BeginTicketMutex(&worldMode->entityMutex);
        
        ClientEntity* entity = worldMode->entities[index];
        ClientEntity* firstFree = 0;
        
        while(entity)
        {
            if(IsSet(entity, Flag_deleted))
            {
                firstFree = entity;
            }
            else
			{
				Assert(entity->identifier);
				if(entity->identifier == identifier)
				{
					result = entity;
					break;
				}
			}
            
            entity = entity->next;
        }
        
        if(!result && allocate)
        {
            if(firstFree)
            {
                result = firstFree;
            }
            else
            {
                result = PushStruct(worldMode->persistentPool, ClientEntity);
                result->next = worldMode->entities[index];
                worldMode->entities[index] = result;
            }
        }
        
        EndTicketMutex(&worldMode->entityMutex);
    }
    
    return result;
}

internal void FreeStem(GameModeWorld* worldMode, PlantStem* stem)
{
    for(PlantSegment* segment = stem->root; segment;)
    {
        PlantSegment* nextToFree = segment->next;
        
        for(PlantStem* clone = segment->clones; clone;)
        {
            PlantStem* next = clone->next;
            FreeStem(worldMode, clone);
            FREELIST_DEALLOC(clone, worldMode->firstFreePlantStem);
            
            clone = next;
        }
        
        for(PlantStem* child = segment->childs; child;)
        {
            PlantStem* next = child->next;
            FreeStem(worldMode, child);
            FREELIST_DEALLOC(child, worldMode->firstFreePlantStem);
            
            child = next;
        }
        
        FREELIST_DEALLOC(segment, worldMode->firstFreePlantSegment);
        segment = nextToFree;
    }
}

internal void DeleteEntityClient(GameModeWorld* worldMode, ClientEntity* entity)
{
    AddFlags(entity, Flag_deleted);
    
    u32 taxonomy = entity->taxonomy;
    Assert(taxonomy);
    if(IsRock(worldMode->table, taxonomy))
    {
        ClientRock* toFree = entity->rock;
        if(toFree)
        {
            FREELIST_DEALLOC(toFree, worldMode->firstFreeRock);
            entity->rock = 0;
        }
    }
    else if(IsPlant(worldMode->table, taxonomy))
    {
        ClientPlant* toFree = entity->plant;
        if(toFree)
        {
            Assert(toFree->canRender);
            for(PlantStem* stem = toFree->plant.firstTrunk; stem; stem = stem->next)
            {
                FreeStem(worldMode, stem);
            }
            FREELIST_FREE(toFree->plant.firstTrunk, PlantStem, worldMode->firstFreePlantStem);
            *toFree = {};
            
            FREELIST_DEALLOC(toFree, worldMode->firstFreePlant);
            entity->plant = 0;
        }
        
    }
    
    entity->beingDeleted = false;
    entity->effectCount = 0;
    entity->objects.maxObjectCount = 0;
    entity->objects.objectCount = 0;
    
    entity->lifePointsTriggerTime = 0;
    entity->staminaTriggerTime = 0;
    
    
    for(u32 slotIndex = 0; slotIndex < Slot_Count; ++slotIndex)
    {
        entity->equipment[slotIndex].ID = 0;
    }
    
    entity->animation = {};
    entity->timeFromLastUpdate = 0;
    
    
    for(ClientAnimationEffect** effectPtr = &entity->firstActiveEffect; *effectPtr;)
    {
        ClientAnimationEffect* effect = *effectPtr;
        
        if(effect->particleRef)
        {
            effect->particleRef->active = false;
        }
        
        *effectPtr = effect->next;
        
        FREELIST_DEALLOC(effect, worldMode->firstFreeEffect);
    }
    
    entity->prediction.type = Prediction_None;
}


internal void HandleClientPrediction(ClientEntity* entity, r32 timeToUpdate)
{
    ClientPrediction* prediction = &entity->prediction;
    prediction->timeLeft -= timeToUpdate;
    if(prediction->timeLeft <= 0)
    {
        prediction->type = Prediction_None;
    }
    switch(prediction->type)
    {
        case Prediction_None:
        {
            
        } break;
        
        case Prediction_EquipmentRemoved:
        {
            entity->equipment[prediction->slot.slot].ID = 0;
        } break;
        
        case Prediction_EquipmentAdded:
        {
            u64 ID = prediction->identifier;
            entity->equipment[prediction->slot.slot].ID = ID;
        } break;
        
        case Prediction_ActionBegan:
        {
            EntityAction currentAction = entity->action;
            if((currentAction == prediction->action) || (currentAction <= Action_Idle))
            {
                entity->action = prediction->action;
            }
            else
            {
                prediction->type = Prediction_None;
            }
        } break;
    }
}


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

