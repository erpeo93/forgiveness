internal void ImportDefaultEssencesTab(TaxonomySlot* slot, EditorElement* root)
{
    EditorElement* essences = root->firstInList;
    
    while(essences)
    {
        char* essenceName = GetValue(essences, "essence");
        char* quantity = GetValue(essences, "quantity");
        
        AddEssence(&slot->firstDefaultEssence, essenceName, ToU8(quantity));
        
        essences = essences->next;
    }
}