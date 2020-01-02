#if FORG_SERVER
#define GetContainerEntityType(state, ID) GetEntityType((ServerState*) state, ID)
internal EntityType GetEntityType(ServerState* server, EntityID ID)
{
    EntityType result = {};
    if(IsValidID(ID))
    {
        DefaultComponent* def = GetComponent(server, ID, DefaultComponent);
        result = def->type;
    }
    
    return result;
}
#else
#define GetContainerEntityType(state, ID) GetEntityType((GameModeWorld*) state, ID)
internal EntityType GetEntityType(GameModeWorld* worldMode, EntityID serverID)
{
    EntityType result = {};
    if(IsValidID(serverID))
    {
        EntityID clientID = GetClientIDMapping(worldMode, serverID);
        BaseComponent* def = GetComponent(worldMode, clientID, BaseComponent);
        result = def->type;
    }
    
    return result;
}
#endif
internal b32 ContainerHasType(void* state, ContainerComponent* container, EntityType type, EntityID* outputID)
{
    b32 result = false;
    if(container)
    {
        for(u32 objectIndex = 0; objectIndex < ArrayCount(container->storedObjects); ++objectIndex)
        {
            InventorySlot* slot = container->storedObjects + objectIndex;
            if(slot->flags_type == InventorySlot_Invalid)
            {
                break;
            }
            
            EntityID slotID = GetBoundedID(slot);
            EntityType slotType = GetContainerEntityType(state, slotID);
            if(AreEqual(slotType, type) && !(slot->flags_type & InventorySlot_Locked))
            {
                result = true;
                *outputID = slotID;
                break;
            }
        }
        
        if(!result)
        {
            for(u32 objectIndex = 0; objectIndex < ArrayCount(container->usingObjects); ++objectIndex)
            {
                InventorySlot* slot = container->usingObjects + objectIndex;
                if(slot->flags_type == InventorySlot_Invalid)
                {
                    break;
                }
                
                
                EntityID slotID = GetBoundedID(slot);
                EntityType slotType = GetContainerEntityType(state, slotID);
                if(AreEqual(slotType, type) && !(slot->flags_type & InventorySlot_Locked))
                {
                    result = true;
                    *outputID = slotID;
                    break;
                }
            }
            
        }
    }
    
    return result;
}

#if FORG_SERVER
internal b32 EntityHasType(ServerState* server, EntityID ID, EntityType type, EntityID* outputID)
{
    b32 result = false;
    EquipmentComponent* equipment = GetComponent(server, ID, EquipmentComponent);
    if(equipment)
    {
        for(u32 slotIndex = 0; slotIndex < ArrayCount(equipment->slots); ++slotIndex)
        {
            InventorySlot* slot = equipment->slots + slotIndex;
            EntityID slotID = GetBoundedID(slot);
            EntityType slotType = GetEntityType(server, slotID);
            if(AreEqual(slotType, type))
            {
                result = true;
                *outputID = slotID;
                break;
            }
            else
            {
                if(ContainerHasType(server, GetComponent(server, GetBoundedID(slot), ContainerComponent), type, outputID))
                {
                    result= true;
                    break;
                }
            }
        }
    }
    
    if(!result)
    {
        UsingComponent* equipped = GetComponent(server, ID, UsingComponent);
        if(equipped)
        {
            EntityType draggingType = GetEntityType(server, equipped->draggingID.ID);
            if(AreEqual(draggingType, type))
            {
                result = true;
                *outputID = equipped->draggingID.ID;
            }
            else
            {
                for(u32 usingIndex = 0; usingIndex < ArrayCount(equipped->slots); ++usingIndex)
                {
                    InventorySlot* slot = equipped->slots + usingIndex;
                    EntityID slotID = GetBoundedID(slot);
                    EntityType slotType = GetEntityType(server, slotID);
                    if(AreEqual(slotType, type))
                    {
                        result = true;
                        *outputID = slotID;
                        break;
                    }
                    else
                    {
                        if(ContainerHasType(server, GetComponent(server, GetBoundedID(slot), ContainerComponent), type, outputID))
                        {
                            result= true;
                            break;
                        }
                    }
                }
            }
        }
    }
    
    return result;
}
#else
internal b32 EntityHasType(GameModeWorld* worldMode, EntityID ID, EntityType type, EntityID* outputID)
{
    b32 result = false;
    EquipmentComponent* equipment = GetComponent(worldMode, ID, EquipmentComponent);
    if(equipment)
    {
        for(u32 slotIndex = 0; slotIndex < ArrayCount(equipment->slots); ++slotIndex)
        {
            InventorySlot* slot = equipment->slots + slotIndex;
            EntityID slotID = GetClientIDMapping(worldMode, GetBoundedID(slot));
            EntityType slotType = GetEntityType(worldMode, slotID);
            if(AreEqual(slotType, type))
            {
                result = true;
                *outputID = slotID;
                break;
            }
            else
            {
                if(ContainerHasType(worldMode, GetComponent(worldMode, slotID, ContainerComponent), type, outputID))
                {
                    result= true;
                    break;
                }
            }
        }
    }
    
    if(!result)
    {
        UsingComponent* equipped = GetComponent(worldMode, ID, UsingComponent);
        if(equipped)
        {
            EntityType draggingType = GetEntityType(worldMode, equipped->draggingID.ID);
            if(AreEqual(draggingType, type))
            {
                result = true;
                *outputID = equipped->draggingID.ID;
            }
            else
            {
                for(u32 usingIndex = 0; usingIndex < ArrayCount(equipped->slots); ++usingIndex)
                {
                    InventorySlot* slot = equipped->slots + usingIndex;
                    EntityID slotID = GetClientIDMapping(worldMode, GetBoundedID(slot));
                    EntityType slotType = GetEntityType(worldMode, slotID);
                    if(AreEqual(slotType, type))
                    {
                        result = true;
                        *outputID = slotID;
                        break;
                    }
                    else
                    {
                        if(ContainerHasType(worldMode, GetComponent(worldMode, slotID, ContainerComponent), type, outputID))
                        {
                            result= true;
                            break;
                        }
                    }
                }
            }
        }
    }
    
    return result;
}
#endif