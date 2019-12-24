#ifdef FORG_SERVER
internal void DamageEntityPhysically(ServerState* server, EntityID ID, u32 damage)
{
    DefaultComponent* def = GetComponent(server, ID, DefaultComponent);
    AliveComponent* alive = GetComponent(server, ID, AliveComponent);
    
    if(alive)
    {
        u32 newHealth = 0;
        if(GetU32(alive->physicalHealth) > damage)
        {
            newHealth = GetU32(alive->physicalHealth) - damage;
        }
        
        SetU32(def, &alive->physicalHealth, newHealth);
    }
}

internal void DamageEntityMentally(ServerState* server, EntityID ID, u32 damage)
{
    DefaultComponent* def = GetComponent(server, ID, DefaultComponent);
    AliveComponent* alive = GetComponent(server, ID, AliveComponent);
    
    if(alive)
    {
        u32 newHealth = 0;
        if(GetU32(alive->mentalHealth) > damage)
        {
            newHealth = GetU32(alive->mentalHealth) - damage;
        }
        
        SetU32(def, &alive->mentalHealth, newHealth);
    }
}

internal void DeleteEntity(ServerState* server, EntityID ID, DeleteEntityReasonType reason = DeleteEntity_None);
internal void DispatchGameEffect(ServerState* server, EntityID ID, UniversePos targetP, GameEffect* effect, EntityID targetID, u16* essences)
{
    switch(effect->effectType.value)
    {
        case spawnEntity:
        {
            AddEntity(server, targetP, &server->entropy, effect->spawnType, DefaultAddEntityParams());
        } break;
        
        case spawnProjectileTowardTarget:
        {
            AddEntityParams params = SpawnEntityParams(ID);
            
            DefaultComponent* targetDef = GetComponent(server, targetID, DefaultComponent);
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
            EntityID destID = targetID;
            DefaultComponent* def = GetComponent(server, destID, DefaultComponent);
            if((def->P.chunkZ + 1) == (i32) server->maxDeepness)
            {
                DeleteEntity(server, destID, DeleteEntity_None);
            }
            else
            {
                def->P = FindPlayerStartingP(server, (def->P.chunkZ + 1));
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
            u32 damage = 1;
            DamageEntityPhysically(server, targetID, damage);
        } break;
        
        case damageMentally:
        {
            u32 damage = 1;
            DamageEntityMentally(server, targetID, damage);
        } break;
        
        case addSkillPoint:
        {
            if(HasComponent(targetID, PlayerComponent))
            {
                PlayerComponent* player = GetComponent(server, targetID, PlayerComponent);
                ++player->skillPoints;
            }
        } break;
    }
}

internal void DispatchEntityEffects(ServerState* server, UniversePos P, u16 action, EntityID ID, r32 elapsedTime, u16* essences)
{
    EffectComponent* effects = GetComponent(server, ID, EffectComponent);
    
    if(effects)
    {
        for(u32 effectIndex = 0; effectIndex < effects->effectCount; ++effectIndex)
        {
            GameEffect* effect = effects->effects + effectIndex;
            if(effect->action.value == action)
            {
                effects->timers[effectIndex] += elapsedTime;
                if(effects->timers[effectIndex] >= effect->timer)
                {
                    effects->timers[effectIndex] = 0;
                    DispatchGameEffect(server, ID, P, effect, {}, essences);
                    
                    if(effect->timer == 0.0f)
                    {
                        effect->effectType.value = invalid_game_effect;
                    }
                }
            }
        }
    }
}

STANDARD_ECS_JOB_SERVER(DispatchEquipmentEffects)
{
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
                DispatchEntityEffects(server, def->P, action, equipID, elapsedTime, essences);
                
                ContainerComponent* container = GetComponent(server, equipID, ContainerComponent);
                for(u32 usingIndex = 0; usingIndex < ArrayCount(container->usingObjects); ++usingIndex)
                {
                    EntityID usingID = GetBoundedID(container->usingObjects + usingIndex);
                    if(IsValidID(usingID))
                    {
                        DispatchEntityEffects(server, def->P, action, usingID, elapsedTime, essences);
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
            if(!SkillSlot(equipmentIndex))
            {
                EntityID usingID = GetBoundedID(equipped->slots + equipmentIndex);
                if(IsValidID(usingID))
                {
                    DispatchEntityEffects(server, def->P, action, usingID, elapsedTime, essences);
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
            GameEffect* effect = effects->effects + effectIndex;
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
            GameEffect* effect = effects->effects + effectIndex;
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
