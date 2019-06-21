inline void AddBoneAlteration(TaxonomySlot* slot, char* boneIndex, char* scaleX, char* scaleY)
{
    TaxonomyBoneAlterations* alt;
    TAXTABLE_ALLOC(alt, TaxonomyBoneAlterations);
    
    alt->boneIndex = ToU32(boneIndex);
    
    alt->alt.valid = true;
    
    alt->alt.scale.x = ToR32(scaleX);
    alt->alt.scale.y = ToR32(scaleY);
    
    FREELIST_INSERT(alt, slot->firstBoneAlteration);
}

internal void ImportBoneAlterationsTab(TaxonomySlot* slot, EditorElement* root)
{
    
    FREELIST_FREE(slot->firstBoneAlteration, TaxonomyBoneAlterations, taxTable_->firstFreeTaxonomyBoneAlterations);
    EditorElement* alterations = root->firstInList;
    while(alterations)
    {
        char* boneIndex = GetValue(alterations, "boneIndex");
        EditorElement* scale = GetStruct(alterations, "scale");
        char* scaleX = GetValue(scale, "x");
        char* scaleY = GetValue(scale, "y");
        
        AddBoneAlteration(slot, boneIndex, scaleX, scaleY);
        
        alterations = alterations->next;
    }
}