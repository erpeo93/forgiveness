internal void ImportCraftingEssencesTab(TaxonomySlot* slot, EditorElement* root)
{
    EditorElement* essences = root->firstInList;
    
    while(essences)
    {
        char* essenceName = GetValue(essences, "essenceName");
        char* quantity = GetValue(essences, "quantity");
        
        AddEssence(&slot->firstEssence, essenceName, ToU8(quantity));
        
        essences = essences->next;
    }
}