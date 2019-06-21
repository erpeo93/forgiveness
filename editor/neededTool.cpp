
inline void Requires(char* toolName)
{
    TaxonomySlot* toolSlot = NORUNTIMEGetTaxonomySlotByName(taxTable_, toolName);
    if(toolSlot)
    {
        b32 inserted = false;
        for(u32 toolIndex = 0; toolIndex < ArrayCount(currentSlot_->neededToolTaxonomies); ++toolIndex)
        {
            if(!currentSlot_->neededToolTaxonomies[toolIndex])
            {
                inserted = true;
                currentSlot_->neededToolTaxonomies[toolIndex] = toolSlot->taxonomy;
                break;
            }
        }
        
        Assert(inserted);
    }
    else
    {
        
        EditorErrorLog(toolName);
    }
}


internal void ImportNeededToolsTab(TaxonomySlot* slot, EditorElement* root)
{
    for(u32 toolIndex = 0; toolIndex < ArrayCount(currentSlot_->neededToolTaxonomies); ++toolIndex)
    {
        slot->neededToolTaxonomies[toolIndex] = 0;
    }
    EditorElement* tools = root->firstInList;
    while(tools)
    {
        char* toolName = tools->name;
        Requires(toolName);
        tools = tools->next;
    }
}