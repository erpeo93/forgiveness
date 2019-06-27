
inline void AddLabel(TaxonomySlot* slot, u64 ID, r32 value)
{
    VisualLabel* dest;
    TAXTABLE_ALLOC(dest, VisualLabel);
    
    u32 hash = (u32) (ID >> 32);
    dest->ID = (hash & (LABEL_HASH_COUNT - 1)) + Tag_count;
    dest->value = value;
    
    FREELIST_INSERT(dest, slot->firstVisualLabel);
}

internal void ImportVisualLabelsTab(TaxonomySlot* slot, EditorElement* root)
{
    
    FREELIST_FREE(slot->firstVisualLabel, VisualLabel, taxTable_->firstFreeVisualLabel);
    EditorElement* labels = root->firstInList;
    
    while(labels)
    {
        char* labelName = labels->name;
        char* value = GetValue(labels, "value");
        
        u64 ID = StringHash(labelName);
        r32 val = ToR32(value);
        AddLabel(slot, ID, val);
        labels = labels->next;
    }
}