
inline void AddAssAlteration(TaxonomySlot* slot, char* assIndex, char* scaleX, char* scaleY, char* offsetX, char* offsetY, b32 specialColoration, Vec4 color)
{
    TaxonomyAssAlterations* alt;
    TAXTABLE_ALLOC(alt, TaxonomyAssAlterations);
    
    alt->assIndex = ToU32(assIndex);
    
    alt->alt.valid = true;
    
    alt->alt.scale.x = ToR32(scaleX);
    alt->alt.scale.y = ToR32(scaleY);
    
    alt->alt.boneOffset.x = ToR32(offsetX);
    alt->alt.boneOffset.y = ToR32(offsetY);
    
    alt->alt.specialColoration = specialColoration;
    alt->alt.color = color;
    
    FREELIST_INSERT(alt, slot->firstAssAlteration);
}

internal void ImportAssAlterationsTab(TaxonomySlot* slot, EditorElement* root)
{
    
    FREELIST_FREE(slot->firstAssAlteration, TaxonomyAssAlterations, taxTable_->firstFreeTaxonomyAssAlterations);
    
    EditorElement* alterations = root->firstInList;
    while(alterations)
    {
        char* assIndex = GetValue(alterations, "assIndex");
        EditorElement* scale = GetStruct(alterations, "scale");
        char* scaleX = GetValue(scale, "x");
        char* scaleY = GetValue(scale, "y");
        
        EditorElement* offset = GetStruct(alterations, "boneOffset");
        char* offsetX = GetValue(offset, "x");
        char* offsetY = GetValue(offset, "y");
        
        b32 specialColoration = ToB32(GetValue(alterations, "specialColoration"));
        Vec4 color = ToV4Color(GetStruct(alterations, "color"));
        
        AddAssAlteration(slot, assIndex, scaleX, scaleY, offsetX, offsetY, specialColoration, color);
        
        alterations = alterations->next;
    }
}