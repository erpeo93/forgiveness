internal void ImportEffectsTab(TaxonomySlot* slot, EditorElement* root)
{
    
    FREELIST_FREE(slot->firstEffect, TaxonomyEffect, taxTable_->firstFreeTaxonomyEffect);
    EditorElement* effectList = root->firstInList;
    while(effectList)
    {
        TaxonomyEffect* newEffect;
        TAXTABLE_ALLOC(newEffect, TaxonomyEffect);            
        FREELIST_INSERT(newEffect, slot->firstEffect);
        ParseEffect(effectList, &newEffect->effect);    
        effectList = effectList->next;
    }
}