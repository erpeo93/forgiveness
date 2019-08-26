inline void Requires_(CraftingEffectLink* link, char* essenceName)
{
    TaxonomySlot* essenceSlot = NORUNTIMEGetTaxonomySlotByName(taxTable_, essenceName);
    if(essenceSlot)
    {
        if(link)
        {
            for(u32 essenceIndex = 0; essenceIndex < MAX_ESSENCES_PER_EFFECT; ++essenceIndex)
            {
                if(!link->essences[essenceIndex].taxonomy)
                {
                    link->essences[essenceIndex].taxonomy = essenceSlot->taxonomy;
                    return;
                }
            }
            
            InvalidCodePath;
        }
    }
    else
    {
        EditorErrorLog(essenceName);
    }
}

inline void Requires(CraftingEffectLink* link, char* essenceName, u32 quantity)
{
    quantity = Max(quantity, 1);
    for(u32 quantityIndex = 0; quantityIndex < quantity; ++quantityIndex)
    {
        Requires_(link, essenceName);
    }
}


inline CraftingEffectLink* LinkStandard(TaxonomySlot* slot, char* action, char* effectName, char* target)
{
    CraftingEffectLink* link = PushStruct(&currentSlot_->pool, CraftingEffectLink);
    link->triggerAction = (EntityAction) GetValuePreprocessor(EntityAction, action);
    link->target = target ? ToB32(target) : false;
    link->effectID = (EffectIdentifier) GetValuePreprocessor(EffectIdentifier, effectName);
    
    FREELIST_INSERT(link, slot->firstCraftingLink);
    
    return link;
}

internal void ImportCraftingEffectsTab(TaxonomySlot* slot, EditorElement* root)
{
    EditorElement* craftingEffectsList = root->firstInList;
    while(craftingEffectsList)
    {
        char* action = GetValue(craftingEffectsList, "action");
        char* id = GetValue(craftingEffectsList, "effectID");
        char* target = GetValue(craftingEffectsList, "target");
        
        CraftingEffectLink* link = LinkStandard(slot, action, id, target);
        
        EditorElement* requirements = GetList(craftingEffectsList, "requirements");
        while(requirements)
        {
            char* ingredient = GetValue(requirements, "essenceName");
            char* quantity = GetValue(requirements, "quantity");
            
            Requires(link, ingredient, ToU32(quantity));
            
            requirements = requirements->next;
        }
        
        craftingEffectsList = craftingEffectsList->next;
    }
}