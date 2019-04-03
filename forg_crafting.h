#pragma once

#define MAX_ESSENCES_PER_EFFECT 3
#define MAX_DIFFERENT_ESSENCES 32
struct EssenceSlot
{
    u32 taxonomy;
    u32 quantity;
};


introspection() struct GenerationData
{
    union
    {
        u64 generic;
        u64 recipeIndex;
    };
};


inline GenerationData RecipeGenerationData()
{
    GenerationData result = {};
    return result;
}

inline GenerationData NullGenerationData()
{
    GenerationData result = {};
    return result;
}

inline GenerationData RecipeIndexGenerationData(u64 recipeIndex)
{
    GenerationData result;
    result.recipeIndex = recipeIndex;
    
    return result;
}

inline b32 AreEqual(GenerationData g1, GenerationData g2)
{
    b32 result = (g1.generic == g2.generic);
    return result;
}

struct Recipe
{
    u32 taxonomy;
    GenerationData gen;
};
