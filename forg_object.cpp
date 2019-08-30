#if 0

inline ObjectLayout* GetLayout(TaxonomyTable* table, u32 taxonomy)
{
    ObjectLayout* result = 0;
    
    u32 testTaxonomy = taxonomy;
    while(testTaxonomy)
    {
        TaxonomySlot* slot = GetSlotForTaxonomy(table, testTaxonomy);
        if(slot->firstLayout)
        {
            result = slot->firstLayout;
            break;
        }
        testTaxonomy = GetParentTaxonomy(table, testTaxonomy);
    }
    
    return result;
}
#endif

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
#endif


inline EntityAction CanConsume(TaxonomyTable* table, u32 taxonomy, u32 objectTaxonomy)
{
    EntityAction result = Action_None;
    // TODO(Leonardo): search into target action tree
    return result;
}