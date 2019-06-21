inline void SaveCreatureAttribute(TaxonomySlot* slot, char* attributeName, r32 value)
{
    MemberDefinition member = SLOWGetRuntimeOffsetOf(CreatureComponent, attributeName);
    u32 offset = member.offset;
    
    AttributeSlot* attr = GetAttributeSlot(slot, offset);
    attr->offsetFromBase = offset;
    attr->valueR32 = value;
}

internal void ImportAttributesTab(TaxonomySlot* slot, EditorElement* root)
{
    
    for(u32 attributeIndex = 0; attributeIndex < ArrayCount(slot->attributeHashmap); ++attributeIndex)
    {
        currentSlot_->attributeHashmap[attributeIndex] = {};
    }
    EditorElement* attributes = root->firstInList;
    
    while(attributes)
    {
        char* attributeName = GetValue(attributes, "name");
        char* value = GetValue(attributes, "value");
        
        SaveCreatureAttribute(slot, attributeName, ToR32(value));
        
        attributes = attributes->next;
    }
}