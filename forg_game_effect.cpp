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
internal void DispatchGameEffect(ServerState* server, EntityID ID, UniversePos P, GameEffect* effect, EntityID targetID)
{
    switch(effect->effectType.value)
    {
        case spawnEntity:
        {
            AddEntity(server, P, &server->entropy, effect->spawnType, DefaultAddEntityParams());
        } break;
        
        case spawnEntityTowardTarget:
        {
            AddEntityParams params = DefaultAddEntityParams();
            
            DefaultComponent* targetDef = GetComponent(server, targetID, DefaultComponent);
            DefaultComponent* def = GetComponent(server, ID, DefaultComponent);
            Vec3 toTarget = SubtractOnSameZChunk(targetDef->P, def->P);
            params.acceleration = toTarget;
            params.speed = 1.0f * Normalize(toTarget);
            AddEntity(server, P, &server->entropy, effect->spawnType, params);
        } break;
        
        case spawnEntityTowardDirection:
        {
            AddEntityParams params = DefaultAddEntityParams();
            
            DefaultComponent* def = GetComponent(server, ID, DefaultComponent);
            Vec3 toTarget = SubtractOnSameZChunk(P, def->P);
            params.acceleration = toTarget;
            params.speed = 1.0f * Normalize(toTarget);
            AddEntity(server, def->P, &server->entropy, effect->spawnType, params);
        } break;
        
        case moveOnZSlice:
        {
            EntityID destID = targetID;
            DefaultComponent* def = GetComponent(server, destID, DefaultComponent);
            if(++def->P.chunkZ == (i32) server->maxDeepness)
            {
                --def->P.chunkZ;
                DeleteEntity(server, destID, DeleteEntity_None);
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
            DamageEntityPhysically(server, targetID, 1);
        } break;
        
        case damageMentally:
        {
            DamageEntityMentally(server, targetID, 1);
        } break;
    }
}

internal void DispatchEntityEffects(ServerState* server, EntityID ID, r32 elapsedTime)
{
    DefaultComponent* def = GetComponent(server, ID, DefaultComponent);
    EffectComponent* effects = GetComponent(server, ID, EffectComponent);
    if(effects)
    {
        for(u32 effectIndex = 0; effectIndex < effects->effectCount; ++effectIndex)
        {
            GameEffect* effect = effects->effects + effectIndex;
            effects->timers[effectIndex] += elapsedTime;
            if(effects->timers[effectIndex] >= effect->timer)
            {
                effects->timers[effectIndex] = 0;
                DispatchGameEffect(server, ID, def->P, effect, {});
            }
        }
    }
}

STANDARD_ECS_JOB_SERVER(DispatchEquipmentEffects)
{
    EquipmentComponent* equipment = GetComponent(server, ID, EquipmentComponent);
    if(equipment)
    {
        for(u32 equipmentIndex = 0; equipmentIndex < ArrayCount(equipment->slots); ++equipmentIndex)
        {
            EntityID equipID = GetBoundedID(equipment->slots + equipmentIndex);
            if(IsValidID(equipID))
            {
                DispatchEntityEffects(server, equipID, elapsedTime);
                
                ContainerComponent* container = GetComponent(server, equipID, ContainerComponent);
                for(u32 usingIndex = 0; usingIndex < ArrayCount(container->usingObjects); ++usingIndex)
                {
                    EntityID usingID = GetBoundedID(container->usingObjects + usingIndex);
                    if(IsValidID(usingID))
                    {
                        DispatchEntityEffects(server, usingID, elapsedTime);
                    }
                }
            }
        }
    }
    
    UsingComponent* equipped = GetComponent(server, ID, UsingComponent);
    if(equipped)
    {
        for(u32 equipmentIndex = 0; equipmentIndex < ArrayCount(equipped->slots); ++equipmentIndex)
        {
            EntityID usingID = GetBoundedID(equipped->slots + equipmentIndex);
            if(IsValidID(usingID))
            {
                DispatchEntityEffects(server, usingID, elapsedTime);
            }
        }
    }
}

internal void DispatchCollisitonEffects(ServerState* server, UniversePos P, EntityID actor, EntityID trigger)
{
    CollisionEffectsComponent* effects = GetComponent(server, trigger, CollisionEffectsComponent);
    if(effects)
    {
        for(u32 effectIndex = 0; effectIndex < effects->effectCount; ++effectIndex)
        {
            GameEffect* effect = effects->effects + effectIndex;
            DispatchGameEffect(server, trigger, P, effect, actor);
        }
    }
}

internal void DispatchOverlappingEffects(ServerState* server, UniversePos P, EntityID actor, EntityID overlap)
{
    OverlappingEffectsComponent* effects = GetComponent(server, overlap, OverlappingEffectsComponent);
    if(effects)
    {
        for(u32 effectIndex = 0; effectIndex < effects->effectCount; ++effectIndex)
        {
            GameEffect* effect = effects->effects + effectIndex;
            DispatchGameEffect(server, overlap, P, effect, actor);
        }
    }
}
#else
#endif
internal b32 CompatibleSlot(InteractionComponent* interaction, InventorySlot* slot)
{
    b32 result = (interaction->inventorySlotType == slot->type);
    return result;
}
