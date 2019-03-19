#ifdef FORG_SERVER
inline b32 OwnedByOthers(SimEntity* entity, u64 id)
{
    b32 result = true;
    if(!entity->ownerID || (entity->ownerID == id))
    {
        result = false;
    }
    
    return result;
}

inline b32 RequiresOwnership(EntityAction action)
{
    b32 result = (action == Action_Open ||
                  action == Action_Pick ||
                  action == Action_Equip);
    
    return result;
}

inline b32 Owned(SimEntity* entity, u64 myID)
{
    b32 result = false;
    if(!OwnedByOthers(entity, myID))
    {
        entity->ownerID = myID;
        result = true;
    }
    
    return result;
}

inline void ObjectToEntity(TaxonomyTable* table, Object* object, SimEntity* entity)
{
    if(IsRecipe(object))
    {
        entity->taxonomy = table->recipeTaxonomy;
        entity->recipeIndex = 0;
        
        entity->recipeTaxonomy = object->taxonomy;
        entity->recipeIndex = object->recipeIndex;
    }
    else
    {
        entity->taxonomy = object->taxonomy;
        entity->recipeIndex = object->recipeIndex;
        
        entity->recipeTaxonomy = 0;
        
        entity->quantity = (r32) object->quantity;
        if(object->quantity == 1)
        {
            entity->quantity = 0;
        }
    }
    entity->status = (r32) object->status;
}

inline void EntityToObject(SimEntity* entity, Object* object)
{
    if(entity->recipeTaxonomy)
    {
        object->taxonomy = entity->recipeTaxonomy;
        object->recipeIndex = entity->recipeIndex;
        object->quantity = 0xffff;
    }
    else
    {
        object->taxonomy = entity->taxonomy;
        object->recipeIndex = entity->recipeIndex;
        object->quantity = (u16) entity->quantity;
        if(!object->quantity)
        {
            object->quantity = 1;
        }
    }
    object->status = (i16) entity->status;
    
    
    AddFlags(entity, Flag_deleted);
}
#endif

inline EquipmentMapping InventorySlotPresent(TaxonomyTable* table, u32 entityTaxonomy, u32 objectTaxonomy)
{
    EquipmentMapping result = {};
    
    TaxonomySlot* slot = GetSlotForTaxonomy(table, entityTaxonomy);
    TaxonomyNode* node = FindInTaxonomyTree(table, slot->equipmentMappings.root, objectTaxonomy);
    if(node && node->data.equipmentMapping)
    {
        result = *(node->data.equipmentMapping);
    }
    return result;
}

inline EquipInfo PossibleToEquip_(TaxonomyTable* table, u32 entityTaxonomy, EquipmentSlot* equipment, u32 objectTaxonomy, i16 status, SlotPlacement placement)
{
    EquipInfo result;
    result.slotCount = 0;
    
    // NOTE(Leonardo): otherwise it means it's not completed
    if(status >= 0)
    {
        EquipmentMapping slotPresent = InventorySlotPresent(table, entityTaxonomy, objectTaxonomy);
        if(!slotPresent.multiPart)
        {
            if(slotPresent.left || slotPresent.right)
            {
                switch(placement)
                {
                    case SlotPlacement_None:
                    {
                        Assert(slotPresent.left == slotPresent.right);
                        if(!equipment[slotPresent.left].ID)
                        {
                            result.slotCount = 1;
                            result.slots[0] = slotPresent.left;
                        }
                    } break;
                    
                    case SlotPlacement_Left:
                    {
                        if(!equipment[slotPresent.left].ID)
                        {
                            result.slotCount = 1;
                            result.slots[0] = slotPresent.left;
                        }
                    } break;
                    
                    case SlotPlacement_Right:
                    {
                        if(!equipment[slotPresent.right].ID)
                        {
                            result.slotCount = 1;
                            result.slots[0] = slotPresent.right;
                        }
                    } break;
                    
                    case SlotPlacement_Both:
                    {
                        if(!equipment[slotPresent.left].ID)
                        {
                            result.slotCount = 1;
                            result.slots[0] = slotPresent.left;
                        }
                        else if(!equipment[slotPresent.right].ID)
                        {
                            result.slotCount = 1;
                            result.slots[0] = slotPresent.right;
                        }
                    } break;
                    
                    InvalidDefaultCase;
                }
            }
        }
        else
        {
            result.slotCount = slotPresent.slotCount;
            for(u32 slotIndex = 0; slotIndex < slotPresent.slotCount; ++slotIndex)
            {
                SlotName test = slotPresent.slots[slotIndex];
                if(!equipment[test].ID)
                {
                    result.slots[slotIndex] = test;
                }
                else
                {
                    result.slotCount = 0;
                    break;
                }
            }
        }
    }
    
    return result;
}

inline EntityAction CanConsume(TaxonomyTable* table, u32 taxonomy, u32 objectTaxonomy)
{
    EntityAction result = Action_None;
    TaxonomySlot* slot = GetSlotForTaxonomy(table, taxonomy);
    for(ConsumeMapping* mapping = slot->firstConsumeMapping; mapping; mapping = mapping->next)
    {
        TaxonomySlot* parent = GetSlotForTaxonomy(table, mapping->taxonomy);
        if(IsSubTaxonomy(objectTaxonomy, parent))
        {
            result = mapping->action;
        }
    }
    
    return result;
}

#if FORG_SERVER
inline EquipInfo PossibleToEquip(SimRegion* region, SimEntity* entity, Object* object, SlotPlacement placement)
{
    Assert(entity->IDs[Component_Creature]);
    
    CreatureComponent* creature = Creature(region, entity);
    u32 objectTaxonomy = GetObjectTaxonomy(region->taxTable, object);
    EquipInfo result = PossibleToEquip_(region->taxTable, entity->taxonomy, creature->equipment, objectTaxonomy, object->status, placement);
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

inline void RemoveFromContainer(SimRegion* region, u64 ownerID, SimEntity* entity, u8 objectIndex)
{
    Assert(entity);
    ObjectComponent* object = Object(region, entity);
    
    RemoveFromContainer_(region, &object->objects, objectIndex);
    
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
    ObjectComponent* objectComponent = Object(region, entity);
    ContainedObjects* objects = &objectComponent->objects;
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

inline Object* GetObject(SimRegion* region, SimEntity* container, u32 objectIndex)
{
    ObjectComponent* object = Object(region, container);
    
    Object* result = 0;
    if(objectIndex < object->objects.maxObjectCount)
    {
        result = object->objects.objects + objectIndex;
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


inline ObjectReference HasObjectOfKind(SimRegion* region, SimEntity* entity, u32 objectTaxonomy)
{
    ObjectReference result = {};
    CreatureComponent* creature = Creature(region,entity);
    for(u32 slotIndex = 0; slotIndex < Slot_Count && !result.containerID; ++slotIndex)
    {
        u64 equipmentID = creature->equipment[slotIndex].ID;
        if(equipmentID)
        {
            SimEntity* container = GetRegionEntityByID(region, equipmentID);
            ObjectComponent* object = Object(region, container);
            
            for(u32 objectIndex = 0; objectIndex < object->objects.maxObjectCount; ++objectIndex)
            {
                Object* obj = object->objects.objects + objectIndex;
                if(obj->taxonomy)
                {
                    if(IsRecipe(obj))
                    {
                        if(objectTaxonomy == region->taxTable->recipeTaxonomy)
                        {
                            result.containerID = equipmentID;
                            result.objectIndex = SafeTruncateToU8(objectIndex);
                            break;
                        }
                    }
                    else
                    {
                        TaxonomySlot* slot = GetSlotForTaxonomy(region->taxTable, obj->taxonomy);
                        if(IsSubTaxonomy(objectTaxonomy, slot))
                        {
                            result.containerID = equipmentID;
                            result.objectIndex = SafeTruncateToU8(objectIndex);
                            break;
                        }
                    }
                }
            }
        }
    }
    return result;
}

internal b32 EquipObject(SimRegion* region, SimEntity* actor, SimEntity* object)
{
    b32 result = false;
    CreatureComponent* creature = Creature(region, actor);
    EquipInfo info = PossibleToEquip_(region->taxTable, actor->taxonomy, creature->equipment, object->taxonomy, (i16) object->status, SlotPlacement_Both);
    if(info.slotCount)
    {
        AddFlags(object, Flag_Equipped);
        object->velocity = {};
        
        for(u32 slotIndex = 0; slotIndex < info.slotCount; ++slotIndex)
        {
            EquipmentSlot* equipmentSlot = creature->equipment + info.slots[slotIndex];
            equipmentSlot->ID = object->identifier;
        }
        
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
        ObjectComponent* object = Object(region, obj);
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
                    ObjectComponent* containerObject = Object(region, container);
                    
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

