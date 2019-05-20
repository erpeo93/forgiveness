struct RecipeIngredients
{
    u32 count;
    u32 taxonomies[16];
    u32 quantities[16];
};

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

internal void GetRecipeIngredients(RecipeIngredients* output, TaxonomyTable* table, u32 taxonomy, GenerationData gen)
{
    TaxonomySlot* slot = GetSlotForTaxonomy(table, taxonomy);
    output->count = 0;
    
    
    ObjectLayout* layout = GetLayout(table, taxonomy);
    RandomSequence seq = Seed(gen.ingredientSeed);
    if(layout)
    {
        for(LayoutPiece* piece = layout->firstPiece; piece; piece = piece->next)
        {
            for(u32 ingredientIndex = 0; ingredientIndex < piece->ingredientCount; ++ingredientIndex)
            {
                u32 sourceQuantity = piece->ingredientQuantities[ingredientIndex]; 
                u32 ingredientTaxonomy = GetRandomChild(table, &seq, piece->ingredientTaxonomies[ingredientIndex]);
                u32 quantity = ( sourceQuantity == 0) ? 1 : sourceQuantity;
                
                b32 alreadyPresent = false;
                for(u32 presentIndex = 0; presentIndex < output->count; ++presentIndex)
                {
                    if(output->taxonomies[presentIndex] == ingredientTaxonomy)
                    {
                        output->quantities[presentIndex] += quantity;
                        alreadyPresent = true;
                        break;
                    }
                }
                
                if(!alreadyPresent)
                {
                    Assert(output->count < ArrayCount(output->taxonomies));
                    u32 outputIndex = output->count++;
                    output->taxonomies[outputIndex] = ingredientTaxonomy;
                    output->quantities[outputIndex] = quantity;
                }
            }
        }
    }
    
    u32 essenceCount = 1;
    for(u32 essenceIndex = 0; essenceIndex < essenceCount; ++essenceIndex)
    {
        u32 essenceTaxonomy = GetRandomChild(table, &seq, table->essenceTaxonomy);
        u8 quantity = 1;
        
        Assert(output->count < ArrayCount(output->taxonomies));
        u32 outputIndex = output->count++;
        output->taxonomies[outputIndex] = essenceTaxonomy;
        output->quantities[outputIndex] = quantity;
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

#if FORG_SERVER
inline void RemoveEssences(SimRegion* region, SimEntity* entity, TaxonomyEssence* firstEssence)
{
    for(TaxonomyEssence* essence = firstEssence; essence; essence = essence->next)
    {
        EssenceSlot* slot = &essence->essence;
        
        i16 delta = -(i16) slot->quantity;
        EssenceDelta(region, entity, slot->taxonomy, delta);
    }
}

inline void AddEffectByLink(SimRegion* region, SimEntity* entity, CraftingEffectLink* link)
{
    EffectComponent* effects = Effects(region, entity);
    b32 newEffect = true;
    
    
    for(u32 effectIndex = 0; effectIndex < effects->effectCount; ++effectIndex)
    {
        Effect* effect = effects->effects + effectIndex;
        if(effect->ID == link->effectID)
        {
            newEffect = false;
            effect->basePower += 1.0f;
        }
    }
    
    
    if(newEffect)
    {
        Assert(effects->effectCount < ArrayCount(effects->effects));
        Effect* effect = effects->effects + effects->effectCount++;
        effect->basePower = 1.0f;
        effect->triggerAction = link->triggerAction;
        effect->flags = link->target ? Effect_Target : 0;
        effect->ID = link->effectID;
    }
}


internal void Craft(SimRegion* region, SimEntity* dest, u32 taxonomy, GenerationData gen)
{
    TaxonomyTable* taxTable = region->taxTable;
    RecipeIngredients ingredients;
    GetRecipeIngredients(&ingredients, taxTable, taxonomy, gen);
    
    u32 totalAvailableEssences = 0;
    u32 availableEssenceCount = 0;
    EssenceSlot availableEssences[32];
    
    for(u32 ingredientIndex = 0; ingredientIndex < ingredients.count; ++ingredientIndex)
    {
        u32 ingredientTaxonomy = ingredients.taxonomies[ingredientIndex];
        TaxonomySlot* ingredientSlot = GetSlotForTaxonomy(taxTable, ingredientTaxonomy);
        for(TaxonomyEssence* essenceSlot = ingredientSlot->essences; essenceSlot; essenceSlot = essenceSlot->next)
        {
            EssenceSlot essence = essenceSlot->essence;
            essence.quantity += ingredients.quantities[ingredientIndex];
            
            Assert(availableEssenceCount < ArrayCount(availableEssences));
            availableEssences[availableEssenceCount++] = essence;
            
            totalAvailableEssences += essence.quantity;
        }
    }
    
    RandomSequence seq_ = Seed((u32) gen.ingredientSeed);
    RandomSequence* seq = &seq_;
    
    TaxonomySlot* slot = GetSlotForTaxonomy(taxTable, taxonomy);
    while(availableEssenceCount > 0)
    {
        u32 essenceCount = RandomChoice(seq, MAX_ESSENCES_PER_EFFECT) + 1;
        essenceCount = Min(essenceCount, totalAvailableEssences);
        
        u32 pickedIndexes[MAX_ESSENCES_PER_EFFECT];
        EssenceSlot picked[MAX_ESSENCES_PER_EFFECT];
        
        for(u32 essenceIndex = 0; essenceIndex < essenceCount; ++essenceIndex)
        {
            while(true)
            {
                u32 pickedIndex = RandomChoice(seq, availableEssenceCount);
                u32 alreadyPicked = 0;
                for(u32 testEssenceIndex = 0; testEssenceIndex < essenceIndex; ++testEssenceIndex)
                {
                    if(pickedIndexes[testEssenceIndex] == pickedIndex)
                    {
                        ++alreadyPicked;
                    }
                }
                
                if(alreadyPicked < availableEssences[pickedIndex].quantity)
                {
                    pickedIndexes[essenceIndex] = pickedIndex;
                    picked[essenceIndex] = {availableEssences[essenceIndex].taxonomy, 1};
                    break;
                }
            }
        }
        
        TaxonomySlot* searchingSlot = slot;
        b32 matches = false;
        
        while(searchingSlot->taxonomy && !matches)
        {
            for(CraftingEffectLink* link = searchingSlot->links; link; link = link->next)
            {
                b32 used[MAX_ESSENCES_PER_EFFECT] = {};
                matches = true;
                
                for(u32 essenceIndex = 0; essenceIndex < essenceCount && matches; ++essenceIndex)
                {
                    u32 searchingTaxonomy = picked[essenceIndex].taxonomy;
                    
                    matches = false;
                    for(u32 linkEssenceIndex = 0; linkEssenceIndex < essenceCount; ++linkEssenceIndex)
                    {
                        if(!link->essences[linkEssenceIndex].taxonomy)
                        {
                            break;
                        }
                        
                        if(!used[linkEssenceIndex])
                        {
                            if(searchingTaxonomy == link->essences[linkEssenceIndex].taxonomy)
                            {
                                used[linkEssenceIndex] = true;
                                matches = true;
                                break;
                            }
                        }
                    }
                }
                
                if(matches)
                {
                    AddEffectByLink(region, dest, link);
                    break;
                }
            }
            
            searchingSlot = GetParentSlot(taxTable, searchingSlot);
        }
        
        if(matches || (essenceCount == 1))
        {
            totalAvailableEssences -= essenceCount;
            for(u32 essenceIndex = 0; essenceIndex < essenceCount; ++essenceIndex)
            {
                EssenceSlot* essence = availableEssences + pickedIndexes[essenceIndex];
                essence->quantity -= 1;
            }
            
            
            for(u32 essenceIndex = 0; essenceIndex < essenceCount && availableEssenceCount > 0; ++essenceIndex)
            {
                EssenceSlot* essence = availableEssences + pickedIndexes[essenceIndex];
                if(essence->quantity == 0)
                {
                    *essence = availableEssences[--availableEssenceCount];
                }
            }
        }
    }
}
#endif

