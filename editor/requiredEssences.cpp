internal void ImportRequiredEssenceTab(TaxonomySlot* slot, EditorElement* root)
{
    FREELIST_FREE(slot->firstEssence, TaxonomyEssence, taxTable_->firstFreeTaxonomyEssence);
    EditorElement* essences = root->firstInList;
    
    while(essences)
    {
        char* essenceName = GetValue(essences, "name");
        char* quantity = GetValue(essences, "quantity");
        
        AddEssence(slot, essenceName, ToU8(quantity));
        essences = essences->next;
    }
}