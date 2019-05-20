inline b32 Ignored(ServerPlayer* player, EntityAction action)
{
    b32 result = false;
    for(u32 actionIndex = 0; actionIndex < player->ignoredActionCount; ++actionIndex)
    {
        if(action == player->ignoredActions[actionIndex])
        {
            result = true;
            break;
        }
    }
    
    return result;
}

inline void IgnoreAction(SimRegion* region, SimEntity* entity, EntityAction ignore)
{
    ServerState* server = region->server;
    ServerPlayer* player = server->players + entity->playerID;
    Assert(player->ignoredActionCount < ArrayCount(player->ignoredActions));
    player->ignoredActions[player->ignoredActionCount++] = ignore;
}

inline void StopIgnoringAction(SimRegion* region, SimEntity* entity, EntityAction stopIgnoring)
{
    ServerState* server = region->server;
    ServerPlayer* player = server->players + entity->playerID;
    for(u32 ignoreIndex = 0; ignoreIndex < player->ignoredActionCount; ++ignoreIndex)
    {
        if(player->ignoredActions[ignoreIndex] == stopIgnoring)
        {
            player->ignoredActions[ignoreIndex] = player->ignoredActions[--player->ignoredActionCount];
        }
    }
}

struct DispatchEffectsContext
{
    EntityAction followingAction;
};

inline void DealDamage(DispatchEffectsContext* context, SimRegion* region, SimEntity* entity, r32 damage)
{
    CreatureComponent* creature = Creature(region, entity);
    creature->lifePoints -= damage;
    if(creature->lifePoints <= 0)
    {
        creature->lifePoints = 0;
    }
}

inline void EssenceDelta(SimRegion* region, SimEntity* entity, u32 essenceTaxonomy, i16 delta)
{
    CreatureComponent* creature = Creature(region, entity);
    
    EssenceSlot* firstFree = 0;
    b32 present = false;
    for(u32 testIndex = 0; testIndex < MAX_DIFFERENT_ESSENCES; ++testIndex)
    {
        EssenceSlot* test = creature->essences + testIndex;
        if(!test->taxonomy)
        {
            firstFree = test;
        }
        else if(essenceTaxonomy == test->taxonomy)
        {
            if(delta > 0)
            {
                test->quantity += (u32) delta;
            }
            else
            {
                u32 diff = (u32) -delta;
                Assert(test->quantity >= diff);
                test->quantity -= diff;
            }
            
            present = true;
            break;
        }
    }
    
    if(!present)
    {
        Assert(firstFree);
        Assert(delta > 0);
        EssenceSlot* newEssence = firstFree;
        newEssence->taxonomy = essenceTaxonomy;
        newEssence->quantity = (u32) delta;
    }
    
    if(entity->playerID)
    {
        ServerPlayer* player = region->server->players + entity->playerID;
        SendEssenceDelta(player, essenceTaxonomy, delta);
    }
}


inline u64 AddEntity(SimRegion* region, Vec3 P, u32 taxonomy, GenerationData gen, AddEntityAdditionalParams params);
internal void DispatchEffect_(DispatchEffectsContext* context, SimRegion* region, SimEntity* actor, SimEntity* target, Effect* effect, r32 powerMul)
{
    r32 power = effect->basePower * powerMul;
    
    u64 actorID = actor->identifier;
    u64 targetID = target ? target->identifier : 0;
    
    CreatureComponent* actorCreature = Creature(region, actor);
    switch(effect->ID)
    {
        case Effect_Spawn:
        {
            Vec3 P = actor->P + V3(1, 0, 0);
            
            GenerationData gen = NullGenerationData();
			targetID = AddEntity(region, P, effect->data.taxonomy, gen, DefaultAddEntityParams());
        } break;
        
        case Effect_NakedHandsDamage:
        {
            TaxonomySlot* actorSlot = GetSlotForTaxonomy(region->taxTable, actor->taxonomy);
            b32 nakedHands = true;
            
            for(NakedHandReq* nakedHandReq = actorSlot->nakedHandReq; nakedHandReq; nakedHandReq = nakedHandReq->next)
            {
                u64 equipmentID = actorCreature->equipment[nakedHandReq->slotIndex].ID;
                if(equipmentID)
                {
                    SimEntity* equipment = GetRegionEntityByID(region, equipmentID);
                    TaxonomySlot* nakedHandSlot = GetSlotForTaxonomy(region->taxTable, nakedHandReq->taxonomy);
                    if(IsSubTaxonomy(equipment->taxonomy, nakedHandSlot))
                    {
                        nakedHands = false;
                        break;
                    }
                }
            }
            
            if(nakedHands)
            {
                r32 damage = power * actorCreature->strength;
                DealDamage(context, region, target, damage);
            }
            
        } break;
        
        case Effect_Damage:
        {
            r32 damage = power * actorCreature->strength;
            DealDamage(context, region, target, damage);
        } break;
        
        case Effect_FireDamage:
        {
            r32 damage = power * actorCreature->strength;
            DealDamage(context, region, target, damage);
        } break;
        
        case Effect_RestoreLifePoints:
        {
            actorCreature->lifePoints += power;
        } break;
    }
    
    if(effect->flags & Effect_SendToClientOnTrigger)
    {
        Assert(region->effectTriggeredCount < ArrayCount(region->effectToSend));
        EffectTriggeredToSend* effectToSend = region->effectToSend + region->effectTriggeredCount++;
        effectToSend->P = actor->P;
        effectToSend->actor = actorID;
        effectToSend->target = targetID;
        effectToSend->ID = effect->ID;
    }
}

inline b32 SatisfiedEffectRangeConditions(SimRegion* region, SimEntity* possibleTarget)
{
    b32 result = IsCreature(region->taxTable, possibleTarget->taxonomy);
    return result;
}

struct EffectTargetList
{
    u32 targetCount;
    SimEntity* targets[32];
};

inline EffectTargetList GetAllTargets(SimRegion* region, SimEntity* actor, SimEntity* target, Effect* effect, r32 powerMul)
{
    EffectTargetList result;
    result.targetCount = 0;
    
    EffectTargetRange range = effect->range;
    switch(range.type)
    {
        case EffectTarget_Actor:
        {
            result.targets[result.targetCount++] = actor;
        } break;
        
        case EffectTarget_Target:
        {
            result.targets[result.targetCount++] = target;
        } break;
        
        case EffectTarget_AllInActorRange:
        {
            r32 radious = range.radious;
            r32 rangeSq = Square(radious);
            
            RegionPartitionQueryResult query = QuerySpacePartitionRadious(region, &region->collisionPartition, actor->P, V3(radious, radious, radious));
            for(u32 surfaceIndex = 0; surfaceIndex < ArrayCount(query.surfaceIndexes); ++surfaceIndex)
            {
                RegionPartitionSurface* surface = region->collisionPartition.partitionSurfaces + query.surfaceIndexes[surfaceIndex];
                
                PartitionSurfaceEntityBlock* block = surface->first;
                while(block)
                {
                    for(u32 blockIndex = 0; blockIndex < block->entityCount; ++blockIndex)
                    {
                        CollisionData* collider = block->colliders + blockIndex;
                        SimEntity* entity = GetRegionEntity(region, collider->entityIndex);
                        if(entity->identifier != actor->identifier)
                        {
                            Vec3 distance = entity->P - actor->P;
                            if(LengthSq(distance) < rangeSq)
                            {
                                if(SatisfiedEffectRangeConditions(region, entity))
                                {
                                    if(result.targetCount < ArrayCount(result.targets))
                                    {
                                        result.targets[result.targetCount++] = entity;
                                    }
                                    else
                                    {
                                        InvalidCodePath;
                                    }
                                }
                            }
                        }
                    }
                    block = block->next;
                }
            }
        } break;
        
        
        case EffectTarget_AllInTargetRange:
        {
            r32 radious = range.radious;
            r32 rangeSq = Square(radious);
            RegionPartitionQueryResult query = QuerySpacePartitionRadious(region, &region->collisionPartition, target->P, V3(radious, radious, radious));
            for(u32 surfaceIndex = 0; surfaceIndex < ArrayCount(query.surfaceIndexes); ++surfaceIndex)
            {
                RegionPartitionSurface* surface = region->collisionPartition.partitionSurfaces + query.surfaceIndexes[surfaceIndex];
                
                PartitionSurfaceEntityBlock* block = surface->first;
                while(block)
                {
                    for(u32 blockIndex = 0; blockIndex < block->entityCount; ++blockIndex)
                    {
                        CollisionData* collider = block->colliders + blockIndex;
                        SimEntity* entity = GetRegionEntity(region, collider->entityIndex);
                        if(entity->identifier != target->identifier)
                        {
                            Vec3 distance = entity->P - actor->P;
                            if(LengthSq(distance) < rangeSq)
                            {
                                if(SatisfiedEffectRangeConditions(region, entity))
                                {
                                    if(result.targetCount < ArrayCount(result.targets))
                                    {
                                        result.targets[result.targetCount++] = entity;
                                    }
                                    else
                                    {
                                        InvalidCodePath;
                                    }
                                }
                            }
                        }
                    }
                    block = block->next;
                }
            }
        } break;
    }
    
    return result;
}

internal void DispatchEffect(DispatchEffectsContext* context, SimRegion* region, SimEntity* actor, SimEntity* target, Effect* effect, r32 powerMul)
{
    EffectTargetList targets = GetAllTargets(region, actor, target, effect, powerMul);
    for(u32 targetIndex = 0; targetIndex < targets.targetCount; ++targetIndex)
    {
        SimEntity* realTarget = targets.targets[targetIndex];
        DispatchEffect_(context, region, actor, realTarget, effect, powerMul);
    }
}

internal void DispatchStandardEffects(DispatchEffectsContext* context, SimRegion* region, SimEntity* actor, SimEntity* target, u32 action)
{
    Assert(IsCreature(region->taxTable, actor->taxonomy));
    CreatureComponent* actorCreature = Creature(region, actor);
    switch(action)
    {
        case Action_Pick:
        {
            PickObject(region, actor, target);
        } break;
        
        case Action_Equip:
        {
            EquipObject(region, actor, target);
        } break;
        
        case Action_Craft:
        {
            Assert(target->status < 0);
            AddFlags(target, Flag_Equipped);
            target->status += region->timeToUpdate;
            if(target->status >= 0)
            {
                target->status = 0;
            }
            else
            {
                context->followingAction = Action_Craft;
            }
            
        } break;
        
        case Action_Open:
        {
            if(actor->playerID)
            {
                ServerPlayer* player = region->server->players + actor->playerID;
                SendCompleteContainerInfo(region, player, target);
            }
            actorCreature->openedContainerID = target->identifier;
            
            context->followingAction = Action_Examine;
            
            if(actor->playerID)
            {
                IgnoreAction(region, actor, Action_Open);
            }
        } break;
        
        case Action_Attack:
        {
            context->followingAction = Action_Attack;
        } break;
        
        case Action_Cast:
        {
            SkillSlot* skillSlot = actorCreature->skills + actorCreature->activeSkillIndex;
            Assert(skillSlot->taxonomy);
            TaxonomySlot* spellSlot = GetSlotForTaxonomy(region->taxTable, skillSlot->taxonomy);
            for(TaxonomyEffect* effectSlot = spellSlot->firstEffect; effectSlot; effectSlot = effectSlot->next)
            {
                Effect* effect = &effectSlot->effect;
                if(effect->triggerAction == Action_Cast)
                {
                    r32 power = skillSlot->power;
                    power = 1.0f;
                    DispatchEffect(context, region, actor, target, effect, power);
                }
                
            }
            actorCreature->skillCooldown = 1.0f;
        } break;
        
        case Action_Eat:
        {
            AddFlags(target, Flag_deleted);
        } break;
    }
}

enum DispatchEffectFlags
{
    DispatchEffect_IsTarget = ( 1 << 1 ),
    DispatchEffect_EquippedEffects = ( 1 << 2 ),
    DispatchEffect_Reverse = ( 1 << 3 ),
};


inline void DispatchEffects(DispatchEffectsContext* context, SimRegion* region, SimEntity* actor, SimEntity* target, Effect* effects, u32 effectCount, u32 isTargetFlag, EntityAction action)
{
    for(u32 effectIndex = 0; effectIndex < effectCount; ++effectIndex)
    {
        Effect* effect = effects + effectIndex;
        
        if(((effect->flags & Effect_Target) == isTargetFlag) && effect->triggerAction == action)
        {
            b32 dispatch = true;
            
            if(effect->targetTimer)
            {
                effect->timer += region->timeToUpdate;
                dispatch = (effect->timer >= effect->targetTimer);
            }
            
            if(dispatch)
            {
                DispatchEffect(context, region, actor, target, effect, 1.0f);
                if(effect->targetTimer)
                {
                    if(effect->flags & Effect_ResetAfterTimer)
                    {
                        effect->timer = 0;
                    }
                    else
                    {
                        InvalidCodePath;
                        
                        if(effect->flags & Effect_SendToClientOnDelete)
                        {
                            //SendMessageToAllPlayers();
                        }
                    }
                }
            }
        }
    }
}

inline void DispatchCreatureEffects(DispatchEffectsContext* context, SimRegion* region, CreatureComponent* creature, SimEntity* actor, SimEntity* target, EntityAction action)
{
    for(u32 slotIndex = 0; slotIndex < Slot_Count; ++slotIndex)
    {
        EquipmentSlot* equipment = creature->equipment + slotIndex;
        if(equipment->ID)
        {
            SimEntity* equipmentEntity = GetRegionEntityByID(region, equipment->ID);
            if(equipmentEntity)
            {
                EffectComponent* effects = Effects(region, equipmentEntity);
                DispatchEffects(context, region, actor, target, effects->effects, effects->effectCount, false, action);
            }
        }
    }
    
    for(u32 skillIndex = 0; skillIndex < MAX_PASSIVE_SKILLS_ACTIVE; ++skillIndex)
    {
        PassiveSkillEffects* effects = creature->passiveSkillEffects + skillIndex;
        DispatchEffects(context, region, actor, target, effects->effects, effects->effectCount, false, action);
    }
}

internal DispatchEffectsContext DispatchEffects(SimRegion* region, SimEntity* actor, SimEntity* target, EntityAction action)
{
    DispatchEffectsContext context_ = {};
    DispatchEffectsContext* context = &context_;
    
    DispatchStandardEffects(context, region, actor, target, action);
    
    EffectComponent* actorEffects = Effects(region, actor);
    DispatchEffects(context, region, actor, target, actorEffects->effects, actorEffects->effectCount, 0, (EntityAction) action);
    
    if(actor->IDs[Component_Creature])
    {
        CreatureComponent* actorCreature = Creature(region, actor);
        DispatchCreatureEffects(context, region, actorCreature, actor, target, action);
    }
    
    if(target)
    {
        EffectComponent* targetEffects = Effects(region, target);
        DispatchEffects(context, region, actor, target, targetEffects->effects, targetEffects->effectCount, 1, (EntityAction) action);
        if(target->IDs[Component_Creature])
        {
            CreatureComponent* targetCreature = Creature(region, target);
            DispatchCreatureEffects(context, region, targetCreature, actor, target, action);
        }
    }
    
    return context_;
}

inline void DispatchPassiveEffects(SimRegion* region, SimEntity* entity)
{
    DispatchEffects(region, entity, 0, Action_None);
}


inline b32 PlayerCanDoAction(SimRegion* region, SimEntity* actor, SimEntity* target, EntityAction action, b32 distanceConstrain, b32* unableBecauseOfDistance)
{
    ServerPlayer* player = region->server->players + actor->playerID;
    b32 canDoAction = true;
    
    if(action >= Action_Attack)
    {
        TaxonomyTable* taxTable = region->taxTable;
        TaxonomySlot* actorSlot = GetSlotForTaxonomy(taxTable, actor->taxonomy);
        TaxonomySlot* targetSlot = GetSlotForTaxonomy(taxTable, target->taxonomy);
        
        
        PlayerPossibleAction* possibleAction = 0;
        PlayerPossibleAction* test = actorSlot->firstPossibleAction;
        while(test)
        {
            if(test->action == action)
            {
                possibleAction = test;
                break;
            }
            
            test = test->next;
        }
        
        if(possibleAction)
        {
            r32 distanceRequiredSquare = Square(possibleAction->distance);
            if(action == Action_Cast)
            {
                CreatureComponent* creature = Creature(region, actor);
                SkillSlot* skillSlot = creature->skills + creature->activeSkillIndex;
                Assert(skillSlot->taxonomy);
                TaxonomySlot* spellSlot = GetSlotForTaxonomy(region->taxTable, skillSlot->taxonomy);
                distanceRequiredSquare = Square(spellSlot->maxDistanceAllowed);
                
                if((creature->skillCooldown > 0) || (creature->activeSkillIndex == -1))
                {
                    canDoAction = false;
                }
            }
            
            if(distanceConstrain)
            {
                r32 distanceSq = LengthSq(target->P - actor->P);
                if(distanceSq > distanceRequiredSquare)
                {
                    canDoAction = false;
                    *unableBecauseOfDistance = true;
                }
            }
            
            if(action == Action_Craft)
            {
                if(target->status >= 0)
                {
                    canDoAction = false;
                }
            }
            
            if(IsSet(target, Flag_Equipped))
            {
                canDoAction = false;
            }
            
            if(possibleAction->flags & CanDoAction_Own)
            {
                if(OwnedByOthers(target, actor->identifier))
                {
                    canDoAction = false;
                }
            }
            
            if(possibleAction->flags & CanDoAction_EquipmentSlot)
            {
                CreatureComponent* creature = Creature(region, actor);
                EquipInfo info = PossibleToEquip_(taxTable, actor->taxonomy, creature->equipment, targetSlot->taxonomy, (i16) target->status);
                if(!IsValid(info))
                {
                    canDoAction = false;
                }
            }
            
            if(possibleAction->flags & CanDoAction_Empty)
            {
                if(target->IDs[Component_Object])
                {
                    ObjectComponent* object = Object(region, target);
                    if(object->objects.objectCount != 0)
                    {
                        canDoAction = false;
                    }
                }
            }
            
            if(canDoAction)
            {
                TaxonomyNode* node = FindInTaxonomyTree(taxTable, possibleAction->tree.root, targetSlot->taxonomy);
                if(node)
                {
                    if(!node->data.possible)
                    {
                        canDoAction = false;
                    }
                }
                else
                {
                    canDoAction = false;
                }
            }
        }
        else
        {
            canDoAction = false;
        }
    }
    return canDoAction;
}

inline b32 AICanDoAction(SimRegion* region, SimEntity* entity, SimEntity* target, EntityAction action)
{
    b32 result = true;
    return result;
}

inline SimEntity* GetRegionEntityByID( SimRegion* region, u64 ID );
internal void HandleAction(SimRegion* region, SimEntity* entity)
{
    ServerState* server = region->server;
    EntityAction consideringAction = entity->action;
    SimEntity* destEntity = GetRegionEntityByID( region, entity->targetID );
    if(destEntity)
    {
        b32 canDo = true;
        if(RequiresOwnership(consideringAction))
        {
            if(!Owned(destEntity, entity->identifier))
            {
                canDo = false;
            }
        }
        
        if(canDo)
        {
            r32 timeToAdvance = region->timeToUpdate;
            entity->actionTime += timeToAdvance;
            r32 actionTime = entity->actionTime * 1.0f;
            r32 targetTime = 0.3f;
            if(actionTime > targetTime)
            {
                DispatchEffectsContext dispatch = DispatchEffects(region, entity, destEntity, consideringAction);
                if(region->border != Border_Mirror && IsSet(entity, Flag_insideRegion))
                {
					
					CreatureComponent* creature = Creature(region, entity);
					creature->completedAction = SafeTruncateToU8(consideringAction);
					creature->completedActionTarget = destEntity->identifier;    
					entity->actionTime = 0;
                    
                    entity->action = dispatch.followingAction;
                }
            }
        }
    }
}








