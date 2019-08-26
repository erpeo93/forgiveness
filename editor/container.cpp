inline void InventorySpace(u8 width, u8 height)
{
    TaxonomySlot* slot = currentSlot_;
    
    slot->gridDimX = width;
    slot->gridDimY = height;
}


internal void ImportContainerTab(TaxonomySlot* slot, EditorElement* root)
{
    
    u8 width = ToU8(GetValue(root, "width"));
    u8 height = ToU8(GetValue(root, "height"));
    
    InventorySpace(width, height);
    
#if FORG_SERVER
    EditorElement* insideInteractions = GetList(root, "interactions");
    while(insideInteractions)
    {
        TaxonomyContainerInteraction* interaction = PushStruct(&currentSlot_->pool, TaxonomyContainerInteraction);
        
        interaction->targetTime = ToR32(GetValue(root, "time"));
        
        EditorElement* ingredients = GetList(insideInteractions, "required");
        while(ingredients)
        {
            char* ingredient = GetValue(ingredients, "taxonomyName");
            TaxonomySlot* ingredientSlot = NORUNTIMEGetTaxonomySlotByName(taxTable_, ingredient);
            
            if(ingredientSlot)
            {
                if(interaction->requiredCount < ArrayCount(interaction->requiredTaxonomies))
                {
                    interaction->requiredTaxonomies[interaction->requiredCount++] = slot->taxonomy;
                }
            }
            else
            {
                EditorErrorLog(ingredient);
            }
            
            ingredients = ingredients->next;
        }
        
        EditorElement* effects = GetList(insideInteractions, "effects");
        while(effects)
        {
            if(interaction->effectCount < ArrayCount(interaction->effects))
            {
                Effect* dest = interaction->effects + interaction->effectCount++;                    
                ParseEffect(effects, dest);
            }
            
            effects = effects->next;
        }
        
        insideInteractions = insideInteractions->next;
    }
#endif
}