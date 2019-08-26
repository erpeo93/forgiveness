inline void AddBoneAlteration(BoneAlterations* slot, char* boneIndex, char* scaleX, char* scaleY)
{
    EditorBoneAlteration* alt = PushStruct(&currentSlot_->pool, EditorBoneAlteration);
    
    alt->boneIndex = ToU32(boneIndex);
    
    alt->alt.valid = true;
    
    alt->alt.scale.x = ToR32(scaleX);
    alt->alt.scale.y = ToR32(scaleY);
    
    FREELIST_INSERT(alt, slot->firstBoneAlteration);
}

internal void ImportBoneAlterationsTab(BoneAlterations* slot, EditorElement* root)
{
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