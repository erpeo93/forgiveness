#if FORG_SERVER
inline SlotName PossibleToEquip(SimRegion* region, SimEntity* entity, Object* object)
{
    Assert(entity->IDs[Component_Creature]);
    
    CreatureComponent* creature = Creature(region, entity);
    u32 objectTaxonomy = GetObjectTaxonomy(region->taxTable, object);
    SlotName result = PossibleToEquip_(region->taxTable, entity->taxonomy, creature->equipment, objectTaxonomy, object->status);
    return result;
}


inline void RemoveFromContainer_(SimRegion* region, ContainedObjects* objects, u8 objectIndex)
{
    Assert(objects->objectCount > 0);
    Assert(objects->maxObjectCount > 0);
    
    --objects->objectCount;
    Object* object = objects->objects + objectIndex;
    object->taxonomy = 0;
    object->quantity = 0;
}

inline void SendObjectEntityHeader(ServerPlayer* player, u64 containerID);
inline void SendContainerInfo(ServerPlayer* player, u64 identifier, u8 maxObjectCount);
inline void SendObjectRemoveUpdate(ServerPlayer* player, u8 objectIndex);
inline void SendObjectAddUpdate(ServerPlayer* player, u8 objectIndex, Object* object);
inline void SendCompleteContainerInfo(SimRegion* region, ServerPlayer* player, SimEntity* container);

inline SimEntity* GetRegionEntityByID( SimRegion* region, u64 ID );
inline void RemoveFromContainer(SimRegion* region, u64 ownerID, SimEntity* entity, u8 objectIndex)
{
    Assert(entity);
    ContainerComponent* container = Container(region, entity);
    
    RemoveFromContainer_(region, &container->objects, objectIndex);
    
    Assert(ownerID);
    SimEntity* owner = GetRegionEntityByID(region, ownerID);
    if(owner->playerID)
    {
        ServerPlayer* player = region->server->players + owner->playerID;
        SendObjectEntityHeader(player, entity->identifier);
        SendObjectRemoveUpdate(player, objectIndex);
    }
}

inline void AddToContainer(SimRegion* region, u64 ownerID, SimEntity* entity, u8 objectIndex, Object object)
{
    ContainerComponent* container = Container(region, entity);
    
    ContainedObjects* objects = &container->objects;
    Assert(object.taxonomy);
    Assert(objects->objectCount < objects->maxObjectCount);
    ++objects->objectCount;
    
    objects->objects[objectIndex] = object;
    
    
    Assert(ownerID);
    SimEntity* owner = GetRegionEntityByID(region, ownerID);
    if(owner->playerID)
    {
        ServerPlayer* player = region->server->players + owner->playerID;
        SendObjectEntityHeader(player, entity->identifier);
        SendObjectAddUpdate(player, objectIndex, &object);
    }
    
}

inline Object* GetObject(SimRegion* region, SimEntity* entity, u32 objectIndex)
{
    ContainerComponent* container = Container(region, entity);
    
    Object* result = 0;
    if(objectIndex < container->objects.maxObjectCount)
    {
        result = container->objects.objects + objectIndex;
    }
    else
    {
        InvalidCodePath;
    }
    
    return result;
}

inline Object* GetObjectIfOwned(SimRegion* region, SimEntity* entity, SimEntity* container, u32 objectIndex)
{
    Object* result = 0;
    if(container && Owned(container, entity->identifier))
    {
        result = GetObject(region, container, objectIndex);
    }
    
    return result;
}

inline b32 IsValid(u8 objectIndex)
{
    b32 result = (objectIndex != 0xff);
    
    return result;
}

inline u8 HasObjectOfKind(SimRegion* region, ContainerComponent* container, u32 objectTaxonomy)
{
    u8 result = 0xff;
    Assert(container->objects.maxObjectCount <= 0xff);
    for(u32 objectIndex = 0; objectIndex < container->objects.maxObjectCount; ++objectIndex)
    {
        Object* obj = container->objects.objects + objectIndex;
        if(obj->taxonomy)
        {
            if(IsRecipe(obj))
            {
                if(objectTaxonomy == region->taxTable->recipeTaxonomy)
                {
                    result = SafeTruncateToU8(objectIndex);
                    break;
                }
            }
            else
            {
                TaxonomySlot* slot = GetSlotForTaxonomy(region->taxTable, obj->taxonomy);
                if(IsSubTaxonomy(objectTaxonomy, slot))
                {
                    result = SafeTruncateToU8(objectIndex);
                    break;
                }
            }
        }
    }
    
    return result;
}

inline ObjectReference HasObjectOfKind(SimRegion* region, SimEntity* entity, u32 objectTaxonomy)
{
    ObjectReference result = {};
    CreatureComponent* creature = Creature(region,entity);
    for(u32 slotIndex = 0; slotIndex < Slot_Count && !result.containerID; ++slotIndex)
    {
        u64 equipmentID = creature->equipment[slotIndex].ID;
        if(equipmentID)
        {
            SimEntity* containerEntity = GetRegionEntityByID(region, equipmentID);
            ContainerComponent* container = Container(region, containerEntity);
            
            u8 objectIndex = HasObjectOfKind(region, container, objectTaxonomy);
            if(objectIndex != 0xff)
            {
                result.objectIndex = objectIndex;
                result.containerID = equipmentID;
                break;
            }
            
        }
    }
    return result;
}

internal b32 EquipObject(SimRegion* region, SimEntity* actor, SimEntity* object)
{
    b32 result = false;
    CreatureComponent* creature = Creature(region, actor);
    SlotName slot = PossibleToEquip_(region->taxTable, actor->taxonomy, creature->equipment, object->taxonomy, (i16) object->status);
    if(IsValid(slot))
    {
        AddFlags(object, Flag_Attached);
        object->velocity = {};
        
        EquipmentSlot* equipmentSlot = creature->equipment + slot;
        equipmentSlot->ID = object->identifier;
        
        if(actor->playerID)
        {
            ServerPlayer* player = region->server->players + actor->playerID;
            SendCompleteContainerInfo(region, player, object);
        }
        result = true;
    }
    
    return result;
}

inline SimEntity* GetRegionEntityByID(SimRegion* region, u64 ID);
internal b32 PickObject( SimRegion* region, SimEntity* actor, SimEntity* obj)
{
    b32 result = true;
    if(!EquipObject( region, actor, obj))
    {
        CreatureComponent* creature = Creature(region, actor);
        ContainerComponent* object = Container(region, obj);
        result = false;
        if(!object->objects.objectCount)
        {
            for(u32 slotIndex = 0; slotIndex < Slot_Count; ++slotIndex)
            {
                EquipmentSlot* slot = creature->equipment + slotIndex;
                if(slot->ID)
                {
                    SimEntity* container = GetRegionEntityByID(region, slot->ID);
                    Assert(container);
                    ContainerComponent* containerObject = Container(region, container);
                    
                    if(containerObject->objects.objectCount < containerObject->objects.maxObjectCount)
                    {
                        result = true;
                        for(u8 objectIndex = 0; objectIndex < containerObject->objects.maxObjectCount; ++objectIndex)
                        {
                            Object* candidate = containerObject->objects.objects + objectIndex;
                            if(!candidate->taxonomy)
                            {
                                Object dest;
                                EntityToObject(obj, &dest);
                                AddToContainer(region, actor->identifier, container, objectIndex, dest);
                                break;
                            }
                        }
                        
                        break;
                    }
                }
            }
        }
    }
    
    return result;
}
#endif

