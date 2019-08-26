#pragma once
struct EssenceSlot
{
    u32 taxonomy;
    u32 quantity;
};

struct TaxonomyEssence
{
    EssenceSlot essence;
    
    union
    {
        TaxonomyEssence* next;
        TaxonomyEssence* nextFree;
    };
};
