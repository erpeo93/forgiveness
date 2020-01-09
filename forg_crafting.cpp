internal EntityType GetCraftingType(Assets* assets, u32 recipeSeed)
{
    RandomSequence seq = Seed(recipeSeed);
    
    EntityType result = {};
    while(true)
    {
        AssetID definitionID = QueryDataFiles(assets, EntityDefinition, "default", &seq, 0);
        Assert(IsValid(definitionID));
        EntityDefinition* defintion = GetData(assets, EntityDefinition, definitionID);
        if(defintion->common.craftable)
        {
            result = GetEntityType(definitionID);
            break;
        }
    }
    
    return result;
}

// TODO(Leonardo): consider rarity!
internal u16 GetCraftingComponents(Assets* assets, EntityType type, u32 seed, EntityType* components, b32* deleteComponents, u16 maxComponentCount)
{
    EntityDefinition* definition = GetEntityTypeDefinition(assets, type);
    Assert(definition->common.craftable);
    
    u16 result = 0;
    if(definition->common.componentCount > 0)
    {
        RandomSequence seq = Seed(seed);
        for(u16 componentIndex = 0; componentIndex < definition->common.componentCount; ++componentIndex)
        {
            CraftingComponent* component = definition->common.components + componentIndex;
            if(component->optionCount > 0)
            {
                Assert(result < maxComponentCount);
                u16 index = result++;
                u32 choice = RandomChoice(&seq, component->optionCount);
                components[index] = GetEntityType(assets, component->options[choice]);
                deleteComponents[index] = component->deleteAfterCrafting;
            }
        }
        
    }
    return result;
}

internal u16 GetCraftingEssenceCount(Assets* assets, EntityType type, u32 seed)
{
    EntityDefinition* definition = GetEntityTypeDefinition(assets, type);
    RandomSequence seq = Seed(seed);
    
    i16 delta = (i16) RandomRangeInt(&seq, -definition->common.essenceCountV, definition->common.essenceCountV);
    i16 temp = Max(0, definition->common.essenceCountRef + delta);
    u16 result = (u16) temp;
    result = 0;
    
    return result;
}

internal u16 GetRandomEssence(RandomSequence* seq)
{
    u16 result = SafeTruncateToU16(RandomChoice(seq, Count_essence));
    return result;
}

#if 0
// TODO(Leonardo): consider rarity!
internal void GetEssences(Assets* assets, EntityType type, u32 seed, u16* essences)
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