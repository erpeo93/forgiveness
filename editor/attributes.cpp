inline AttributeSlot* GetAttributeSlot(Attributes* slot, u32 offsetOf)
{
    u32 index = offsetOf & (ArrayCount(slot->attributeHashmap) - 1);
    for(;;)
    {
        AttributeSlot* attr = slot->attributeHashmap + index;
        if(!attr->offsetFromBase)
        {
            return attr;
            break;
        }
        
        if(++index == ArrayCount(slot->attributeHashmap))
        {
            index = 0;
        }
    }
}

inline void SaveCreatureAttribute(Attributes* slot, char* attributeName, r32 value)
{
    MemberDefinition member = SLOWGetRuntimeOffsetOf(CreatureComponent, attributeName);
    u32 offset = member.offset;
    
    AttributeSlot* attr = GetAttributeSlot(slot, offset);
    attr->offsetFromBase = offset;
    attr->valueR32 = value;
}

internal void ImportAttributesTab(Attributes* slot, EditorElement* root)
{
    for(u32 attributeIndex = 0; attributeIndex < ArrayCount(slot->attributeHashmap); ++attributeIndex)
    {
        slot->attributeHashmap[attributeIndex] = {};
    }
    EditorElement* attributes = root->firstInList;
    
    while(attributes)
    {
        char* attributeName = attributes->name;;
        char* value = GetValue(attributes, "value");
        
        SaveCreatureAttribute(slot, attributeName, ToR32(value));
        
        attributes = attributes->next;
    }
}