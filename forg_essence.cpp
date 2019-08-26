

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


inline b32 HasEssences(EssenceSlot* essenceSlots, TaxonomyEssence* firstEssence)
{
    b32 result = true;
    for(TaxonomyEssence* essence = firstEssence; essence; essence = essence->next)
    {
        EssenceSlot* slot = &essence->essence;
        result = false;
        for(u32 testIndex = 0; testIndex < MAX_DIFFERENT_ESSENCES; ++testIndex)
        {
            EssenceSlot* test = essenceSlots + testIndex;
            if(slot->taxonomy == test->taxonomy && slot->quantity <= test->quantity)
            {
                result = true;
                break;
            }
        }
    }
    return result;
}

inline void RemoveEssences(SimRegion* region, SimEntity* entity, TaxonomyEssence* firstEssence)
{
    for(TaxonomyEssence* essence = firstEssence; essence; essence = essence->next)
    {
        EssenceSlot* slot = &essence->essence;
        
        i16 delta = -(i16) slot->quantity;
        EssenceDelta(region, entity, slot->taxonomy, delta);
    }
}
