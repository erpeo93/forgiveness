#pragma once

#define MAX_ESSENCES_PER_EFFECT 3
#define MAX_DIFFERENT_ESSENCES 32
struct EssenceSlot
{
    u32 taxonomy;
    u32 quantity;
};

struct Recipe
{
    u32 taxonomy;
    u64 recipeIndex;
};
