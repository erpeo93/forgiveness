internal void ImportCraftingEssencesTab(TaxonomySlot* slot, EditorElement* root)
{
    FREELIST_FREE(currentSlot_->essences, TaxonomyEssence, taxTable_->firstFreeTaxonomyEssence);
    EditorElement* essences = root->firstInList;
    
    while(essences)
    {
        char* essenceName = GetValue(essences, "essence");
        char* quantity = GetValue(essences, "quantity");
        
        AddEssence(essenceName, ToU8(quantity));
        
        essences = essences->next;
    }
}