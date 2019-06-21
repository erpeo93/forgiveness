
inline void CanConsume(char* action, char* name)
{
    TaxonomySlot* slot = currentSlot_;
    TaxonomySlot* target = NORUNTIMEGetTaxonomySlotByName(taxTable_, name);
    
    if(target)
    {
        ConsumeMapping* mapping;
        TAXTABLE_ALLOC(mapping, ConsumeMapping);
        
        
        mapping->action = (EntityAction) GetValuePreprocessor(EntityAction, action);
        mapping->taxonomy = target->taxonomy;
        
        mapping->next = slot->firstConsumeMapping;
        slot->firstConsumeMapping = mapping;
    }
    else
    {
        EditorErrorLog(name);
    }
    
}


internal void ImportConsumeMappingsTab(TaxonomySlot* slot, EditorElement* root)
{
    FREELIST_FREE(slot->firstConsumeMapping, ConsumeMapping, taxTable_->firstFreeConsumeMapping);
    EditorElement* consume = root->firstInList;
    while(consume)
    {
        char* actionName = GetValue(consume, "action");
        char* objectName = GetValue(consume, "object");
        CanConsume(actionName, objectName);
        
        consume = consume->next;
    }
}