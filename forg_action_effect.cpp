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

#if 0
inline r32 GetValue()
{
    switch(type)
    {
        case R32:
        {
            
        } break;
        
        case Pointer_R32:
        {
            
        } break;
    }
}

inline r32* GetPointerR32(EvaluateASTContext* context)
{
    
}

struct EvaluateASTContext
{
    SimRegion* region;
    SimEntity* actor;
    SimEntity* target;
};

inline r32 EvaluateASTOperation(EvaluateASTContext context, ForgASTNode* node)
{
    r32 result = 0;
    
    switch(node->operation)
    {
        case ASTOperation_None:
        {
            Assert(node->operandCount == 0);
            result = GetValue(node->value);
        } break;
        
        case ASTOperation_Sum:
        {
            Assert(node->operandCount == 2);
            result = EvalASTOperation(contexdt, node->operands + 0) + EvalASTOperation(context, node->operands1);
        } break;
        
        case ASTOperation_Mul:
        {
            Assert(node->operandCount == 2);
            result = EvalASTOperation(contexdt, node->operands + 0) * EvalASTOperation(context, node->operands1);
        } break;
        
        case ASTOperation_Sub:
        {
            Assert(node->operandCount == 2);
            result = EvalASTOperation(contexdt, node->operands + 0) - EvalASTOperation(context, node->operands1);
        } break;
        
        case ASTOperation_Div:
        {
            Assert(node->operandCount == 2);
            r32 divisor = EvalASTOperation(context, node->operands1);
            
            if(divisor != 0.0f)
            {
                result = EvalASTOperation(contexdt, node->operands + 0) / divisor;
            }
        } break;
        
        case ASTOperation_Assign:
        {
            Assert(node->operandCount == 2);
            
            r32* dest = GetPointerR32(node->operands[0]);
            r32 value = EvalASTOperation(context, node->operands + 1);
            *dest = value;
        } break;
    }
    
    return result;
}

inline void EvaluateAST(DispatchEffectsContext* context, SimRegion* region, SimEntity* actor, SimEntity* target, ForgAST* ast)
{
    EvalASTResult eval = EvaluateASTNode(context, region, actor, target, ast->root);
    Assert(eval->type == ASTResult_None);
}
#endif

inline void EvaluateAST(DispatchEffectsContext* context, SimRegion* region, SimEntity* actor, SimEntity* target, ForgAST* ast)
{
    //EvalASTResult eval = EvaluateASTNode(context, region, actor, target, ast->root);
    //Assert(eval->type == ASTResult_None);
}


inline u64 AddEntity(SimRegion* region, Vec3 P, u32 taxonomy, GenerationData gen, AddEntityAdditionalParams params);
internal void DispatchEffect(DispatchEffectsContext* context, SimRegion* region, SimEntity* actor, SimEntity* target, Effect* effect)
{
    u64 actorID = actor->identifier;
    u64 targetID = target ? target->identifier : 0;
#if 0
    TaxonomySlot* effectSlot = GetNthChildSlot(region->taxTable, region->taxTable->effectTaxonomy, effect->ID);
    switch(effect->ID)
    {
        case generic_spawn:
        {
            Vec3 P = actor->P + V3(1, 0, 0);
            
            GenerationData gen = NullGenerationData();
			targetID = AddEntity(region, P, effect->data.taxonomy, gen, DefaultAddEntityParams());
        } break;
        
        case combat_nakedHandsDamage:
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
                r32 damage = effect->data.power * actorCreature->strength;
                DealDamage(context, region, target, damage);
            }
            
        } break;
        
        case combat_standardDamage:
        {
            r32 damage = effect->data.power * actorCreature->strength;
            DealDamage(context, region, target, damage);
        } break;
        
        case combat_hugeDamage:
        {
            r32 damage = 2.8f * effect->data.power;
            DealDamage(context, region, target, damage);
        } break;
        
        case combat_fireDamage:
        {
            
        } break;
        
        case consume_restoreLifePoints:
        {
            actorCreature->lifePoints += 10.0f * effect->data.power;
        } break;
    }
#else
    TaxonomySlot* slot = GetSlotForTaxonomy(region->taxTable, effect->taxonomy);
    if(slot->ast.root)
    {
        EvaluateAST(context, region, actor, target, &slot->ast);
    }
#endif
    
    if(effect->flags & Effect_SendToClientOnTrigger)
    {
        Assert(region->effectTriggeredCount < ArrayCount(region->effectToSend));
        EffectTriggeredToSend* effectToSend = region->effectToSend + region->effectTriggeredCount++;
        effectToSend->P = actor->P;
        effectToSend->actor = actorID;
        effectToSend->target = targetID;
        effectToSend->effectTaxonomy = slot->taxonomy;
    }
}

internal void DispatchEffect(DispatchEffectsContext* context, SimRegion* region, SimEntity* actor, SimEntity* target, u32 effectTaxonomy, r32 power = 1.0f)
{
    Effect effect = {};
    effect.taxonomy = effectTaxonomy;
    effect.data.power = power;
    DispatchEffect(context, region, actor, target, &effect);
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
                Assert(effect->triggerAction == Action_Cast);
                
                Effect castEffect = *effect;
                castEffect.data.power *= skillSlot->power;
                
                DispatchEffect(context, region, actor, target, &castEffect);
            }
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
                DispatchEffect(context, region, actor, target, effect);
                
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
        for(u32 slotIndex = 0; slotIndex < Slot_Count; ++slotIndex)
        {
            EquipmentSlot* equipment = actorCreature->equipment + slotIndex;
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
            PassiveSkillEffects* effects = actorCreature->passiveSkillEffects + skillIndex;
            DispatchEffects(context, region, actor, target, effects->effects, effects->effectCount, false, action);
        }
    }
    
    if(target)
    {
        EffectComponent* targetEffects = Effects(region, target);
        DispatchEffects(context, region, actor, target, targetEffects->effects, targetEffects->effectCount, 1, (EntityAction) action);
        if(target->IDs[Component_Creature])
        {
            CreatureComponent* targetCreature = Creature(region, target);
            for(u32 slotIndex = 0; slotIndex < Slot_Count; ++slotIndex)
            {
                EquipmentSlot* equipment = targetCreature->equipment + slotIndex;
                if(equipment->ID)
                {
                    SimEntity* equipmentEntity = GetRegionEntityByID(region, equipment->ID);
                    if(equipmentEntity)
                    {
                        EffectComponent* effects = Effects(region, equipmentEntity);
                        DispatchEffects(context, region, actor, target, effects->effects, effects->effectCount, true, action);
                    }
                }
            }
            
            for(u32 skillIndex = 0; skillIndex < MAX_PASSIVE_SKILLS_ACTIVE; ++skillIndex)
            {
                PassiveSkillEffects* effects = targetCreature->passiveSkillEffects + skillIndex;
                DispatchEffects(context, region, actor, target, effects->effects, effects->effectCount, false, action);
            }
        }
    }
    
    return context_;
}

inline void DispatchPassiveEffects(SimRegion* region, SimEntity* entity)
{
    DispatchEffects(region, entity, 0, Action_None);
}


inline b32 PlayerCanDoAction(SimRegion* region, SimEntity* actor, SimEntity* target, EntityAction action, b32 distanceConstrain)
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
        
        r32 distanceRequiredSquare = Square(MAXIMUM_ACTION_DISTANCE);
        if(possibleAction)
        {
            if(distanceConstrain)
            {
                r32 distanceSq = LengthSq(target->P - actor->P);
                if(distanceSq > distanceRequiredSquare)
                {
                    canDoAction = false;
                }
            }
            
            if(action == Action_Cast)
            {
                CreatureComponent* creature = Creature(region, actor);
                if((creature->skillCooldown > 0) || (creature->activeSkillIndex == -1))
                {
                    canDoAction = false;
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








