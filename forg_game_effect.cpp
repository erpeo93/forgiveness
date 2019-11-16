#ifdef FORG_SERVER
internal void DeleteEntity(ServerState* server, EntityID ID);
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
            
            PhysicComponent* targetPhysic = GetComponent(server, targetID, PhysicComponent);
            PhysicComponent* physic = GetComponent(server, ID, PhysicComponent);
            
            if(targetPhysic && physic)
            {
                Vec3 toTarget = SubtractOnSameZChunk(targetPhysic->P, physic->P);
                params.acceleration = toTarget;
                params.speed = 2.0f * Normalize(toTarget);
                AddEntity(server, P, &server->entropy, effect->spawnType, params);
            }
        } break;
        
        case spawnEntityTowardDirection:
        {
            AddEntityParams params = DefaultAddEntityParams();
            
            PhysicComponent* physic = GetComponent(server, ID, PhysicComponent);
            Vec3 toTarget = SubtractOnSameZChunk(P, physic->P);
            params.acceleration = toTarget;
            params.speed = 2.0f * Normalize(toTarget);
            AddEntity(server, physic->P, &server->entropy, effect->spawnType, params);
        } break;
        
        case moveOnZSlice:
        {
            EntityID destID = targetID;
            if(HasComponent(destID, PhysicComponent))
            {
                PhysicComponent* physic = GetComponent(server, destID, PhysicComponent);
                if(++physic->P.chunkZ == (i32) server->maxDeepness)
                {
                    physic->P.chunkZ = 0;
                }
            }
            else
            {
                InvalidCodePath;
            }
        } break;
        
        case deleteTarget:
        {
            if(HasComponent(targetID, PhysicComponent))
            {
                DeleteEntity(server, targetID);
            }
        } break;
        
        case deleteSelf:
        {
            if(HasComponent(ID, PhysicComponent))
            {
                DeleteEntity(server, ID);
            }
        } break;
    }
}

internal void DispatchEntityEffects(ServerState* server, EntityID ID, r32 elapsedTime)
{
    PhysicComponent* physic = GetComponent(server, ID, PhysicComponent);
    EffectComponent* effects = GetComponent(server, ID, EffectComponent);
    if(physic && effects)
    {
        for(u32 effectIndex = 0; effectIndex < effects->effectCount; ++effectIndex)
        {
            GameEffect* effect = effects->effects + effectIndex;
            effects->timers[effectIndex] += elapsedTime;
            if(effects->timers[effectIndex] >= effect->timer)
            {
                effects->timers[effectIndex] = 0;
                DispatchGameEffect(server, ID, physic->P, effect, {});
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
            EntityID equipID = equipment->slots[equipmentIndex].ID;
            if(IsValidID(equipID))
            {
                DispatchEntityEffects(server, equipID, elapsedTime);
                
                ContainerComponent* container = GetComponent(server, equipID, ContainerComponent);
                for(u32 usingIndex = 0; usingIndex < ArrayCount(container->usingObjects); ++usingIndex)
                {
                    EntityID usingID = container->usingObjects[usingIndex].ID;
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
            EntityID usingID = equipped->slots[equipmentIndex].ID;
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
