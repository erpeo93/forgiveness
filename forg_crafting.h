#pragma once

#define MAX_ESSENCES_PER_EFFECT 3
#define MAX_DIFFERENT_ESSENCES 32
struct RecipeIngredients
{
    u32 count;
    u32 taxonomies[16];
    u32 quantities[16];
};

#ifdef FORG_SERVER
struct CraftingEffectLink
{
    EssenceSlot essences[MAX_ESSENCES_PER_EFFECT];
    u32 triggerAction;
    b32 target;
    EffectIdentifier effectID;
    
    union
    {
        CraftingEffectLink* next;
        CraftingEffectLink* nextFree;
    };
};
#endif