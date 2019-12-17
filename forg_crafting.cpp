internal EntityRef GetCraftingType(Assets* assets, u32 recipeSeed)
{
    RandomSequence seq = Seed(recipeSeed);
    
    EntityRef result = {};
    while(true)
    {
        AssetID definitionID = QueryDataFiles(assets, EntityDefinition, "default", &seq, 0);
        Assert(IsValid(definitionID));
        EntityDefinition* defintion = GetData(assets, EntityDefinition, definitionID);
        if(defintion->common.craftable)
        {
            result = EntityReference(definitionID);
            break;
        }
    }
    
    return result;
}

// TODO(Leonardo): consider rarity!
internal u16 GetCraftingComponents(Assets* assets, EntityRef type, u32 seed, EntityRef* components, u16 maxComponentCount)
{
    EntityDefinition* definition = GetEntityTypeDefinition(assets, type);
    Assert(definition->common.craftable);
    
    u16 result = 0;
    if(definition->common.possibleComponentCount > 0)
    {
        RandomSequence seq = Seed(seed);
        result = definition->common.componentCount;
        Assert(result <= maxComponentCount);
        
        for(u16 componentIndex = 0; componentIndex < result; ++componentIndex)
        {
            u16 choice = SafeTruncateToU16(RandomChoice(&seq, definition->common.possibleComponentCount));
            components[componentIndex] = definition->common.possibleComponents[choice];
        }
        
    }
    return result;
}

internal u16 GetRandomEssence(RandomSequence* seq)
{
    u16 result = SafeTruncateToU16(RandomChoice(seq, Count_essence));
    return result;
}

#if 0
// TODO(Leonardo): consider rarity!
internal void GetEssences(Assets* assets, EntityRef type, u32 seed, u16* essences)
{
    for(u16 essenceIndex = 0; essenceIndex < Count_essence; ++essenceIndex)
    {
        essences[essenceIndex] = 0;
    }
    
    EntityDefinition* definition = GetEntityTypeDefinition(assets, type);
    RandomSequence seq = Seed(seed);
    for(u32 index = 0; index < definition->common.essenceCount; ++index)
    {
        u16 essence = GetRandomEssence(&seq);
        essences[essence] += 1;
    }
}
#endif