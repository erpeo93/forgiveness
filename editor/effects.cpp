internal void ImportEffectsTab(TaxonomySlot* slot, EditorElement* root)
{
    
    EditorElement* effectList = root->firstInList;
    while(effectList)
    {
        TaxonomyEffect* newEffect = PushStruct(&currentSlot_->pool, TaxonomyEffect);
        FREELIST_INSERT(newEffect, slot->firstEffect);
        ParseEffect(effectList, &newEffect->effect);    
        effectList = effectList->next;
    }
}