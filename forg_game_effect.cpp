#ifdef FORG_SERVER
internal void DamageEntityPhysically(ServerState* server, EntityID ID, r32 damage)
{
    HealthComponent* health = GetComponent(server, ID, HealthComponent);
    
    if(health)
    {
        r32 newHealth = Max(0, health->physicalHealth - damage);
        health->physicalHealth = newHealth;
    }
}

internal void DamageEntityMentally(ServerState* server, EntityID ID, r32 damage)
{
    HealthComponent* health = GetComponent(server, ID, HealthComponent);
    
    if(health)
    {
        r32 newHealth = Max(0, health->mentalHealth - damage);
        health->mentalHealth = newHealth;
    }
}

internal void AddTempEffect(ServerState* server, EntityID ID, u16 effect, r32 power, r32 time)
{
    ActiveEffectComponent* effects = GetComponent(server, ID, ActiveEffectComponent);
    if(effects->effectCount < ArrayCount(effects->effects))
    {
        ActiveEffect* dest = effects->effects + effects->effectCount++;
        dest->time = 0;
        dest->totalTime = 0;
        
        dest->effect = {};
        dest->effect.deleteTime = time;
        dest->effect.type = effect;
        dest->effect.power = power;
    }
    else
    {
        InvalidCodePath;
    }
}

internal void AddPermanentEffect(ServerState* server, EntityID ID, u16 effect, r32 power)
{
    AddTempEffect(server, ID, effect, power, 0);
}

internal void EssenceDelta(ServerState* server, EntityID ID, u16 essence, i16 delta);
internal void DeleteEntity(ServerState* server, EntityID ID, DeleteEntityReasonType reason = DeleteEntity_None);
internal EntityType GetCraftingType(Assets* assets, u32 recipeSeed);
internal void AddEntity(ServerState* server, UniversePos P, u32 seed, EntityType type, AddEntityParams params);
internal void DispatchGameEffect(ServerState* server, EntityID ID, UniversePos targetP, GameEffectInstance* effect, EntityID parentID, EntityID targetID, u16* essences, r32 elapsedTime)
{
    switch(effect->type)
    {
        case spawnEntity:
        {
            AddEntity(server, targetP, &server->entropy, effect->spawnType, DefaultAddEntityParams());
        } break;
        
        case spawnEntityPick:
        {
            AddEntity(server, targetP, &server->entropy, effect->spawnType, EquipEntityParams(parentID));
        } break;
        
        case spawnRecipe:
        {
            EntityType target = effect->spawnType;
            EntityType recipeType = GetEntityType(server->assets, "default", "recipe");
            
            while(true)
            {
                u32 seed = GetNextUInt32(&server->entropy);
                EntityType rolled = GetCraftingType(server->assets, seed);
                if(AreEqual(rolled, target))
                {
                    AddEntity(server, targetP, seed, recipeType, DefaultAddEntityParams());
                    break;
                }
            }
            
        } break;
        
        case spawnProjectileTowardTarget:
        {
            AddEntityParams params = SpawnEntityParams(parentID);
            
            DefaultComponent* targetDef = GetComponent(server, targetID, DefaultComponent);
            DefaultComponent* def = GetComponent(server, parentID, DefaultComponent);
            
            Vec3 toTarget = SubtractOnSameZChunk(targetDef->P, def->P);
            params.acceleration = toTarget;
            params.speed = 1.0f * Normalize(toTarget);
            AddEntity(server, targetP, &server->entropy, effect->spawnType, params);
        } break;
        
        case spawnProjectileTowardDirection:
        {
            Vec3 offset = V3(1, 0, 0);
            BrainComponent* brain = GetComponent(server, targetID, BrainComponent);
            if(brain)
            {
                offset = brain->commandParameters.targetOffset;
            }
            
            AddEntityParams params = SpawnEntityParams(parentID);
            params.acceleration = offset;
            params.speed = 1.0f * Normalize(offset);
            params.timeToLive = 10.0f;
            
            UniversePos spawnP = Offset(targetP, V3(0, 0, 1.0f));
            AddEntity(server, spawnP, &server->entropy, effect->spawnType, params);
        } break;
        
        case moveOnZSlice:
        {
            EntityID destID = targetID;
            DefaultComponent* def = GetComponent(server, destID, DefaultComponent);
            if((def->P.chunkZ + 1) == (i32) server->maxDeepness)
            {
                DeleteEntity(server, destID, DeleteEntity_None);
            }
            else
            {
                def->P = FindPlayerStartingP(server, (def->P.chunkZ + 1));
                if(HasComponent(destID, PlayerComponent))
                {
                    PlayerComponent* player = GetComponent(server, destID, PlayerComponent);
                    if(player)
                    {
                        player->justEnteredWorld = true;
                    }
                }
            }
        } break;
        
        case teleportOther:
        {
            EntityID destID = targetID;
            DefaultComponent* def = GetComponent(server, destID, DefaultComponent);
            UniversePos oldP = def->P;
            
            Vec3 offset = {};
            BrainComponent* brain = GetComponent(server, destID, BrainComponent);
            if(brain)
            {
                offset = Normalize(brain->commandParameters.targetOffset) * effect->power;
            }
            def->P = Offset(def->P, offset);
            if(PositionInsideWorld(&def->P))
            {
                def->networkFlags |= BasicFlags_Position;
                AddEntityFlags(def, EntityFlag_teleported);
            }
            else
            {
                def->P = oldP;
            }
        } break;
        
        case lightRadious:
        {
            LightComponent* light = GetComponent(server, parentID, LightComponent);
            if(light)
            {
                SetR32(&light->lightRadious, Max(GetR32(light->lightRadious), 3.0f));
            }
        } break;
        
        case deleteTarget:
        {
            DeleteEntity(server, targetID);
        } break;
        
        case deleteSelf:
        {
            DeleteEntity(server, ID);
        } break;
        
        case damagePhysically:
        {
            r32 damage = effect->power;
            DamageEntityPhysically(server, targetID, damage);
        } break;
        
        case damageMentally:
        {
            r32 damage = effect->power;
            DamageEntityMentally(server, targetID, damage);
        } break;
        
        case dropFlowers:
        {
            VegetationComponent* vegetation = GetComponent(server, ID, VegetationComponent);
            if(vegetation)
            {
                r32 flowerDensity = GetR32(vegetation->flowerDensity);
                
                r32 requiredFlowerDensity = vegetation->requiredFlowerDensity;
                if(flowerDensity >= requiredFlowerDensity)
                {
                    Vec3 offset = Hadamart(RandomBilV3(&server->entropy), V3(0.5f, 0.5f, 0));
                    UniversePos P = Offset(targetP, offset);
                    AddEntity(server, P, &server->entropy, effect->spawnType, DefaultAddEntityParams());
                    vegetation->flowerDensity -= requiredFlowerDensity;
                }
            }
        } break;
        
        case dropFruits:
        {
            VegetationComponent* vegetation = GetComponent(server, ID, VegetationComponent);
            if(vegetation)
            {
                r32 fruitDensity = GetR32(vegetation->fruitDensity);
                r32 requiredFruitDensity = vegetation->requiredFruitDensity;
                if(fruitDensity >= requiredFruitDensity)
                {
                    Vec3 offset = Hadamart(RandomBilV3(&server->entropy), V3(0.5f, 0.5f, 0));
                    UniversePos P = Offset(targetP, offset);
                    AddEntity(server, P, &server->entropy, effect->spawnType, DefaultAddEntityParams());
                    
                    vegetation->fruitDensity -= requiredFruitDensity;
                }
            }
        } break;
        
        case dropBranches:
        {
            VegetationComponent* vegetation = GetComponent(server, ID, VegetationComponent);
            if(vegetation)
            {
                r32 branchDensity = vegetation->branchDensity;
                
                r32 requiredBranchDensity = vegetation->requiredBranchDensity;
                if(branchDensity >= requiredBranchDensity)
                {
                    Vec3 offset = Hadamart(RandomBilV3(&server->entropy), V3(0.5f, 0.5f, 0));
                    UniversePos P = Offset(targetP, offset);
                    AddEntity(server, P, &server->entropy, effect->spawnType, DefaultAddEntityParams());
                    vegetation->branchDensity -= requiredBranchDensity;
                }
            }
        } break;
        
        case sacrificeRandomEssence:
        {
            DefaultComponent* def = GetComponent(server, targetID, DefaultComponent);
            
            b32 hasSomeEssences = false;
            for(u16 essenceIndex = 0; essenceIndex < ArrayCount(def->essences); ++essenceIndex)
            {
                if(def->essences[essenceIndex])
                {
                    hasSomeEssences = true;
                    break;
                }
            }
            if(hasSomeEssences)
            {
                u16 essence = GetRandomEssence(&server->entropy);
                if(def->essences[essence])
                {
                    EssenceDelta(server, targetID, essence, -1);
                    PlayerComponent* player = GetComponent(server, targetID, PlayerComponent);
                    
                    
                    //player->sacrificeTimer += SACRIFICE_TIME;
                }
            }
        } break;
        
        case restorePhysicalHealth:
        {
            HealthComponent* health = GetComponent(server, parentID, HealthComponent);
            if(health)
            {
                health->physicalHealth += effect->power;
            }
        } break;
        
        case restoreMentalHealth:
        {
            HealthComponent* health = GetComponent(server, parentID, HealthComponent);
            if(health)
            {
                health->mentalHealth += effect->power;
            }
        } break;
        
        case giveEssences:
        {
            DefaultComponent* source = GetComponent(server, ID, DefaultComponent);
            for(u16 essenceIndex = 0; essenceIndex < ArrayCount(source->essences); ++essenceIndex)
            {
                u16 toAdd = source->essences[essenceIndex];
                if(toAdd)
                {
                    EssenceDelta(server, targetID, essenceIndex, (i16) toAdd);
                    source->essences[essenceIndex] = 0;
                }
            }
        } break;
        
        case alterProperty_GrowingSpeed:
        {
            VegetationComponent* vegetation = GetComponent(server, ID, VegetationComponent);
            if(vegetation)
            {
                vegetation->flowerGrowingSpeed *= effect->power;
                vegetation->fruitGrowingSpeed *= effect->power;
            }
        } break;
        
        case increaseGrowingSpeed:
        {
            r32 power = effect->power;
            r32 time = effect->power;
            
            if(HasComponent(targetID, VegetationComponent))
            {
                AddTempEffect(server, targetID, alterProperty_GrowingSpeed, power, time);
            }
        } break;
        
        case increaseGrowingSpeedArea:
        {
            r32 power = effect->power;
            r32 time = effect->power;
            
            DefaultComponent* def = GetComponent(server, parentID, DefaultComponent);
            SpatialPartitionQuery effectQuery = QuerySpatialPartitionAtPoint(&server->standardPartition, def->P);
            
            for(EntityID effectID = GetCurrent(&effectQuery); IsValid(&effectQuery); effectID = Advance(&effectQuery))
            {
                if(HasComponent(effectID, VegetationComponent))
                {
                    AddTempEffect(server, effectID, alterProperty_GrowingSpeed, power, time);
                }
            }
        } break;
        
        case alterActionVelocity:
        {
            ActionComponent* action = GetComponent(server, parentID, ActionComponent);
            if(action)
            {
                action->speed *= effect->power;
            }
        } break;
        
        case MinDamage:
        {
            CombatComponent* combat = GetComponent(server, parentID, CombatComponent);
            if(combat)
            {
                combat->minDamage += effect->power;
            }
        } break;
        
        case MaxDamage:
        {
            CombatComponent* combat = GetComponent(server, parentID, CombatComponent);
            if(combat)
            {
                combat->maxDamage += effect->power;
            }
        } break;
        
        case DamageRange:
        {
            CombatComponent* combat = GetComponent(server, parentID, CombatComponent);
            if(combat)
            {
                combat->minDamage += effect->power;
                combat->maxDamage += effect->power;
            }
        } break;
        
        case AttackVelocity:
        {
            CombatComponent* combat = GetComponent(server, parentID, CombatComponent);
            if(combat)
            {
                combat->attackSpeed *= effect->power;
            }
        } break;
        
        case Protection:
        {
            CombatComponent* combat = GetComponent(server, parentID, CombatComponent);
            if(combat)
            {
                combat->protection += effect->power;
            }
        } break;
        
        case fire:
        {
            HealthComponent* health = GetComponent(server, targetID, HealthComponent);
            if(health)
            {
                health->onFirePercentage += effect->power;
            }
        } break;
        
        case firePerSecond:
        {
            HealthComponent* health = GetComponent(server, targetID, HealthComponent);
            if(health)
            {
                health->onFirePercentage += effect->power * elapsedTime;
            }
        } break;
        
        case poison:
        {
            HealthComponent* health = GetComponent(server, targetID, HealthComponent);
            if(health)
            {
                health->poisonPercentage += effect->power;
            }
        } break;
        
        case hit:
        {
            CombatComponent* combat = GetComponent(server, parentID, CombatComponent);
            CombatComponent* targetCombat = GetComponent(server, targetID, CombatComponent);
            Assert(combat && targetCombat);
            
            r32 damage = RandomRangeFloat(&server->entropy, GetR32(combat->minDamage), GetR32(combat->maxDamage));
            damage -= GetR32(targetCombat->protection);
            DamageEntityPhysically(server, targetID, damage);
        } break;
    }
}

internal void DispatchEntityEffects(ServerState* server, UniversePos P, u16 commandIndex, u16 action, EntityID parentID, EntityID ID, EntityID targetID, r32 elapsedTime, u16* essences, b32 targetEffect, EntityType usingType = {})
{
    ActiveEffectComponent* effects = GetComponent(server, ID, ActiveEffectComponent);
    if(effects)
    {
        for(u32 effectIndex = 0; effectIndex < effects->effectCount; ++effectIndex)
        {
            ActiveEffect* effect = effects->effects + effectIndex;
            GameEffectInstance* instance = &effect->effect;
            
            if(instance->action == action && instance->targetEffect == targetEffect)
            {
                b32 valid = true;
                if(IsValid(instance->requiredUsingType))
                {
                    valid = AreEqual(instance->requiredUsingType, usingType);
                }
                
                if(valid)
                {
                    if(instance->commandIndex != commandIndex)
                    {
                        instance->commandIndex = commandIndex;
                        effect->time = 0;
                        effect->totalTime = 0;
                    }
                    
                    effect->time += elapsedTime;
                    effect->totalTime += elapsedTime;
                    
                    if(effect->time >= instance->targetTime)
                    {
                        DispatchGameEffect(server, ID, P, instance, parentID, targetID, essences, elapsedTime);
                        
                        if(instance->targetTime < 0 || (instance->deleteTime > 0 && effect->totalTime >= instance->deleteTime))
                        {
                            *effect = effects->effects[--effects->effectCount];
                        }
                        
                        effect->time = 0;
                    }
                }
            }
        }
    }
    
    
    
}

internal void DispatchActionEffects(ServerState* server, UniversePos P, u16 commandIndex, u16 action, EntityID ID, EntityID targetID, r32 elapsedTime, u16* essences, b32 targetEffect, EntityType usingType = {})
{
    DispatchEntityEffects(server, P, commandIndex, action, ID, ID, targetID, elapsedTime, essences, targetEffect, usingType);
    UsingComponent* equipped = GetComponent(server, ID, UsingComponent);
    if(equipped)
    {
        for(u16 equipmentIndex = 0; equipmentIndex < ArrayCount(equipped->slots); ++equipmentIndex)
        {
            EntityID usingID = GetBoundedID(equipped->slots + equipmentIndex);
            if(IsValidID(usingID))
            {
                u16 minEquipmentIndex = equipmentIndex;
                for(u16 eIndex = 0; eIndex < ArrayCount(equipped->slots); ++eIndex)
                {
                    EntityID testID = GetBoundedID(equipped->slots + eIndex);
                    if(AreEqual(testID, usingID))
                    {
                        minEquipmentIndex = Min(minEquipmentIndex, eIndex);
                    }
                }
                
                if(minEquipmentIndex == equipmentIndex)
                {
                    DispatchEntityEffects(server, P, commandIndex, action, ID, usingID, targetID, elapsedTime, essences, targetEffect, usingType);
                }
            }
        }
    }
}


internal void DispatchEquipmentEffects(ServerState* server, UniversePos P, u16 action, EntityID parentID, EntityID equipmentID, r32 elapsedTime, u16* essences)
{
    DispatchEntityEffects(server, P, 0, action, parentID, equipmentID, {}, elapsedTime, essences, false);
}


#define RESET(comp, property) SetR32(&comp->property, comp->default_##property);
#define RESET_VAL(comp, property, value) SetR32(&comp->property, value);

internal void ResetComputedProperties(ServerState* server, EntityID ID)
{
    HealthComponent* health = GetComponent(server, ID, HealthComponent);
    if(health)
    {
        RESET(health, maxPhysicalHealth);
        RESET(health, maxMentalHealth);
    }
    
    VegetationComponent* vegetation = GetComponent(server, ID, VegetationComponent);
    if(vegetation)
    {
        RESET(vegetation, flowerGrowingSpeed);
        RESET(vegetation, fruitGrowingSpeed);
    }
    
    ActionComponent* action = GetComponent(server, ID, ActionComponent);
    if(action)
    {
        RESET(action, speed);
    }
    
    CombatComponent* combat = GetComponent(server, ID, CombatComponent);
    if(combat)
    {
        RESET_VAL(combat, minDamage, 0);
        RESET_VAL(combat, maxDamage, 0);
        RESET_VAL(combat, attackSpeed, 1);
        RESET_VAL(combat, protection, 0);
    }
}

STANDARD_ECS_JOB_SERVER(DispatchEntityDefaultEffects)
{
    ResetComputedProperties(server, ID);
    
    DefaultComponent* def = GetComponent(server, ID, DefaultComponent);
    if(!EntityHasFlags(def, EntityFlag_notInWorld))
    {
        u16* essences = def->essences;
        DispatchEntityEffects(server, def->P, 0, none, ID, ID, {}, elapsedTime, essences, false);
        EquipmentComponent* equipment = GetComponent(server, ID, EquipmentComponent);
        if(equipment)
        {
            
            for(u16 equipmentIndex = 0; equipmentIndex < ArrayCount(equipment->slots); ++equipmentIndex)
            {
                EntityID equipID = GetBoundedID(equipment->slots + equipmentIndex);
                if(IsValidID(equipID))
                {
                    u16 minEquipmentIndex = equipmentIndex;
                    for(u16 eIndex = 0; eIndex < ArrayCount(equipment->slots); ++eIndex)
                    {
                        EntityID testID = GetBoundedID(equipment->slots + eIndex);
                        if(AreEqual(testID, equipID))
                        {
                            minEquipmentIndex = Min(minEquipmentIndex, eIndex);
                        }
                    }
                    
                    if(minEquipmentIndex == equipmentIndex)
                    {
                        DispatchEquipmentEffects(server, def->P, none, ID, equipID, elapsedTime, essences);
                        ContainerComponent* container = GetComponent(server, equipID, ContainerComponent);
                        for(u32 usingIndex = 0; usingIndex < ArrayCount(container->usingObjects); ++usingIndex)
                        {
                            EntityID usingID = GetBoundedID(container->usingObjects + usingIndex);
                            if(IsValidID(usingID))
                            {
                                DispatchEquipmentEffects(server, def->P, none, ID, usingID, elapsedTime, essences);
                            }
                        }
                    }
                }
            }
        }
        
        UsingComponent* equipped = GetComponent(server, ID, UsingComponent);
        if(equipped)
        {
            for(u16 equipmentIndex = 0; equipmentIndex < ArrayCount(equipped->slots); ++equipmentIndex)
            {
                EntityID usingID = GetBoundedID(equipped->slots + equipmentIndex);
                if(IsValidID(usingID))
                {
                    u16 minEquipmentIndex = equipmentIndex;
                    for(u16 eIndex = 0; eIndex < ArrayCount(equipped->slots); ++eIndex)
                    {
                        EntityID testID = GetBoundedID(equipped->slots + eIndex);
                        if(AreEqual(testID, usingID))
                        {
                            minEquipmentIndex = Min(minEquipmentIndex, eIndex);
                        }
                    }
                    
                    if(minEquipmentIndex == equipmentIndex)
                    {
                        if(SkillSlot(equipmentIndex))
                        {
                            SkillDefComponent* skillDef = GetComponent(server, usingID, SkillDefComponent);
                            if(skillDef->passive)
                            {
                                DispatchEquipmentEffects(server, def->P, none, ID, usingID, elapsedTime, essences);
                            }
                        }
                        else
                        {
                            DispatchEquipmentEffects(server, def->P, none, ID, usingID, elapsedTime, essences);
                        }
                    }
                }
            }
        }
    }
}

internal void DispatchCollisitonEffects(ServerState* server, UniversePos P, EntityID actor, EntityID trigger)
{
    r32 elapsedTime = 0;
    
    CollisionEffectsComponent* effects = GetComponent(server, trigger, CollisionEffectsComponent);
    DefaultComponent* def = GetComponent(server, trigger, DefaultComponent);
    
    if(effects)
    {
        u16* essences = def->essences;
        for(u32 effectIndex = 0; effectIndex < effects->effectCount; ++effectIndex)
        {
            GameEffectInstance* effect = effects->effects + effectIndex;
            DispatchGameEffect(server, trigger, P, effect, {}, actor, essences, elapsedTime);
        }
    }
}

internal void DispatchOverlappingEffects(ServerState* server, UniversePos P, EntityID actor, EntityID overlap, r32 elapsedTime)
{
    OverlappingEffectsComponent* effects = GetComponent(server, overlap, OverlappingEffectsComponent);
    DefaultComponent* def = GetComponent(server, overlap, DefaultComponent);
    
    if(effects)
    {
        u16* essences = def->essences;
        for(u32 effectIndex = 0; effectIndex < effects->effectCount; ++effectIndex)
        {
            GameEffectInstance* effect = effects->effects + effectIndex;
            DispatchGameEffect(server, overlap, P, effect, {}, actor, essences, elapsedTime);
        }
    }
}
#else
#endif
internal b32 CompatibleSlot(InteractionComponent* interaction, InventorySlot* slot)
{
    b32 result = false;
    
    if(slot->flags_type != InventorySlot_Invalid && interaction->inventorySlotType != InventorySlot_Invalid)
    {
        result = (interaction->inventorySlotType == slot->flags_type || slot->flags_type == InventorySlot_Generic);
    }
    
    return result;
}
