#ifdef FORG_SERVER
internal void DamageEntityPhysically(ServerState* server, EntityID ID, r32 damage)
{
    HealthComponent* health = GetComponent(server, ID, HealthComponent);
    
    if(health)
    {
        r32 newHealth = GetR32(health->physicalHealth) - damage;
        SetR32(&health->physicalHealth, newHealth);
    }
}

internal void DamageEntityMentally(ServerState* server, EntityID ID, r32 damage)
{
    HealthComponent* health = GetComponent(server, ID, HealthComponent);
    
    if(health)
    {
        r32 newHealth = GetR32(health->mentalHealth) - damage;
        SetR32(&health->mentalHealth, newHealth);
    }
}

internal void EssenceDelta(ServerState* server, EntityID ID, u16 essence, i16 delta);
internal void DeleteEntity(ServerState* server, EntityID ID, DeleteEntityReasonType reason = DeleteEntity_None);
internal EntityType GetCraftingType(Assets* assets, u32 recipeSeed);
internal void AddEntity(ServerState* server, UniversePos P, u32 seed, EntityType type, AddEntityParams params);
internal void DispatchGameEffect(ServerState* server, EntityID ID, UniversePos targetP, GameEffectInstance* effect, EntityID otherID, u16* essences)
{
    switch(effect->type)
    {
        case spawnEntity:
        {
            AddEntity(server, targetP, &server->entropy, effect->spawnType, DefaultAddEntityParams());
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
            AddEntityParams params = SpawnEntityParams(ID);
            
            DefaultComponent* targetDef = GetComponent(server, otherID, DefaultComponent);
            DefaultComponent* def = GetComponent(server, ID, DefaultComponent);
            Vec3 toTarget = SubtractOnSameZChunk(targetDef->P, def->P);
            params.acceleration = toTarget;
            params.speed = 1.0f * Normalize(toTarget);
            AddEntity(server, targetP, &server->entropy, effect->spawnType, params);
        } break;
        
        case spawnProjectileTowardDirection:
        {
            AddEntityParams params = SpawnEntityParams(ID);
            
            DefaultComponent* def = GetComponent(server, ID, DefaultComponent);
            Vec3 toTarget = SubtractOnSameZChunk(targetP, def->P);
            params.acceleration = toTarget;
            params.speed = 1.0f * Normalize(toTarget);
            
            UniversePos spawnP = def->P;
            AddEntity(server, spawnP, &server->entropy, effect->spawnType, params);
        } break;
        
        case moveOnZSlice:
        {
            EntityID destID = otherID;
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
                    player->justEnteredWorld = true;
                }
            }
        } break;
        
        case lightRadious:
        {
            LightComponent* light = GetComponent(server, otherID, LightComponent);
            if(light)
            {
                SetR32(&light->lightRadious, Max(GetR32(light->lightRadious), 3.0f));
            }
        } break;
        
        case deleteTarget:
        {
            DeleteEntity(server, otherID);
        } break;
        
        case deleteSelf:
        {
            DeleteEntity(server, ID);
        } break;
        
        case damagePhysically:
        {
            r32 damage = effect->power;
            DamageEntityPhysically(server, otherID, damage);
        } break;
        
        case damageMentally:
        {
            r32 damage = effect->power;
            DamageEntityMentally(server, otherID, damage);
        } break;
        
        case addSkillPoint:
        {
            if(HasComponent(otherID, PlayerComponent))
            {
                PlayerComponent* player = GetComponent(server, otherID, PlayerComponent);
                ++player->skillPoints;
            }
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
                    SetR32(&vegetation->flowerDensity, flowerDensity - requiredFlowerDensity);
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
                    SetR32(&vegetation->fruitDensity, fruitDensity - requiredFruitDensity);
                }
            }
        } break;
        
        case sacrificeRandomEssence:
        {
            DefaultComponent* def = GetComponent(server, otherID, DefaultComponent);
            
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
                    EssenceDelta(server, otherID, essence, -1);
                    PlayerComponent* player = GetComponent(server, otherID, PlayerComponent);
                    player->sacrificeTimer += SACRIFICE_TIME;
                }
            }
        } break;
        
        case restorePhysicalHealth:
        {
            HealthComponent* health = GetComponent(server, otherID, HealthComponent);
            if(health)
            {
                r32 newHealth = GetR32(health->physicalHealth) + effect->power;
                SetR32(&health->physicalHealth, Min(newHealth, GetR32(health->maxPhysicalHealth)));
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
                    EssenceDelta(server, otherID, essenceIndex, (i16) toAdd);
                }
            }
        } break;
    }
}

internal void DispatchEntityEffects(ServerState* server, UniversePos P, u16 action, EntityID parentID, EntityID ID, r32 elapsedTime, u16* essences, b32 targetEffect)
{
    EffectComponent* effects = GetComponent(server, ID, EffectComponent);
    if(effects)
    {
        for(u32 effectIndex = 0; effectIndex < effects->effectCount; ++effectIndex)
        {
            GameEffectInstance* effect = effects->effects + effectIndex;
            if(effect->action == action && effect->targetEffect == targetEffect)
            {
                effects->timers[effectIndex] += elapsedTime;
                if(effects->timers[effectIndex] >= effect->timer)
                {
                    effects->timers[effectIndex] = 0;
                    DispatchGameEffect(server, ID, P, effect, parentID, essences);
                    
                    if(effect->timer < 0)
                    {
                        effect->type = invalid_game_effect;
                    }
                }
            }
        }
    }
}

internal void DispatchActionEffects(ServerState* server, UniversePos P, u16 action, EntityID ID, EntityID targetID, r32 elapsedTime, u16* essences, b32 targetEffect)
{
    DispatchEntityEffects(server, P, action, targetID, ID, elapsedTime, essences, targetEffect);
}


internal void DispatchEquipmentEffects(ServerState* server, UniversePos P, u16 action, EntityID parentID, EntityID equipmentID, r32 elapsedTime, u16* essences)
{
    DispatchEntityEffects(server, P, action, parentID, equipmentID, elapsedTime, essences, false);
}


#define RESET(comp, property) SetR32(&comp->property, comp->default_##property);
internal void ResetComputedProperties(ServerState* server, EntityID ID)
{
    HealthComponent* health = GetComponent(server, ID, HealthComponent);
    if(health)
    {
        RESET(health, maxPhysicalHealth);
        RESET(health, maxMentalHealth);
    }
}

STANDARD_ECS_JOB_SERVER(DispatchEntityDefaultEffects)
{
    ResetComputedProperties(server, ID);
    
    DefaultComponent* def = GetComponent(server, ID, DefaultComponent);
    
    u16* essences = def->essences;
    EquipmentComponent* equipment = GetComponent(server, ID, EquipmentComponent);
    
    u16 action = none;
    if(equipment)
    {
        for(u32 equipmentIndex = 0; equipmentIndex < ArrayCount(equipment->slots); ++equipmentIndex)
        {
            EntityID equipID = GetBoundedID(equipment->slots + equipmentIndex);
            if(IsValidID(equipID))
            {
                DispatchEquipmentEffects(server, def->P, action, ID, equipID, elapsedTime, essences);
                
                ContainerComponent* container = GetComponent(server, equipID, ContainerComponent);
                for(u32 usingIndex = 0; usingIndex < ArrayCount(container->usingObjects); ++usingIndex)
                {
                    EntityID usingID = GetBoundedID(container->usingObjects + usingIndex);
                    if(IsValidID(usingID))
                    {
                        DispatchEquipmentEffects(server, def->P, action, ID, usingID, elapsedTime, essences);
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
                if(SkillSlot(equipmentIndex))
                {
                    SkillDefComponent* skillDef = GetComponent(server, usingID, SkillDefComponent);
                    if(skillDef->passive)
                    {
                        DispatchEquipmentEffects(server, def->P, action, ID, usingID, elapsedTime, essences);
                    }
                }
                else
                {
                    DispatchEquipmentEffects(server, def->P, action, ID, usingID, elapsedTime, essences);
                }
            }
        }
    }
}

internal void DispatchCollisitonEffects(ServerState* server, UniversePos P, EntityID actor, EntityID trigger)
{
    CollisionEffectsComponent* effects = GetComponent(server, trigger, CollisionEffectsComponent);
    DefaultComponent* def = GetComponent(server, trigger, DefaultComponent);
    
    if(effects)
    {
        u16* essences = def->essences;
        for(u32 effectIndex = 0; effectIndex < effects->effectCount; ++effectIndex)
        {
            GameEffectInstance* effect = effects->effects + effectIndex;
            DispatchGameEffect(server, trigger, P, effect, actor, essences);
        }
    }
}

internal void DispatchOverlappingEffects(ServerState* server, UniversePos P, EntityID actor, EntityID overlap)
{
    OverlappingEffectsComponent* effects = GetComponent(server, overlap, OverlappingEffectsComponent);
    DefaultComponent* def = GetComponent(server, overlap, DefaultComponent);
    
    if(effects)
    {
        u16* essences = def->essences;
        for(u32 effectIndex = 0; effectIndex < effects->effectCount; ++effectIndex)
        {
            GameEffectInstance* effect = effects->effects + effectIndex;
            DispatchGameEffect(server, overlap, P, effect, actor, essences);
        }
    }
}
#else
#endif
internal b32 CompatibleSlot(InteractionComponent* interaction, InventorySlot* slot)
{
    b32 result = (interaction->inventorySlotType == slot->flags_type || slot->flags_type == InventorySlot_Generic);
    return result;
}
