#pragma once
struct MemConcept
{
    u32 taxonomy;
    u8 refreshTimeUnitsBonus;
    u16 firstAssociationIndex;
};

struct MemIdentifier
{
    u64 identifier;
    u8 refreshTimeUnitsBonus;
    u16 firstAssociationIndex;
};

#if 0
struct MemPosition
{
    union
    {
        Tile;
        Chunk;
        Island;
    };
    u16 firstAssociationIndex;
};
#endif

struct MemAssociation
{
    u16 conceptNextIndex;
    u32 conceptTaxonomy;
    
    u16 identifierNextIndex;
    u64 identifier;
    
    
    u8 deletedCount;
    u8 refreshedCount;
    u16 lastingTimeUnits;
};

struct MemAssociationToInsert
{
    u16 targetAssociationIndex;
    u16 refreshTimeUnits;
    MemAssociation association;
};

struct MemLowScoreAssociation
{
    u16 index;
    u16 lastingTimeUnits;
};

#define PORTION_MAXIMUM_INSERT 32
struct MemPortion
{
    u32 secondsBetweenUpdates;
    r32 decadenceFactor;
    r32 timeToUpdate;
    
    u16 firstIndex;
    u16 onePastLastIndex;
    
    MemLowScoreAssociation lowest[PORTION_MAXIMUM_INSERT];
    
    u32 toInsertCount;
    MemAssociationToInsert toInsert[PORTION_MAXIMUM_INSERT];
};

struct MemAction
{
    u64 identifier;
    u32 taxonomy;
    EntityAction action;
};

#define ASSOCIATION_COUNT 64
struct Mem
{
    MemConcept conceptHash[ASSOCIATION_COUNT];
    MemIdentifier identifierHash[ASSOCIATION_COUNT];
    
    MemAssociation associations[ASSOCIATION_COUNT];
    
    MemPortion shortTerm;
    
    MemAction actionQueueHash[256];
};

struct MemCriteria
{
    u32 taxonomy;
    
    u32 possibleTaxonomiesCount;
    u32 requiredConceptTaxonomy[4];
    
    //u32 considerationCount;
    //Consideration considerations[4];
    
    union
    {
        MemCriteria* next;
        MemCriteria* nextFree;
    };
};

struct MemSynthesisRule
{
    EntityAction action;
    TaxonomyTree tree;
    
    union
    {
        MemSynthesisRule* next;
        MemSynthesisRule* nextFree;
    };
};

struct MemSynthOption
{
    u16 lastingtimeUnits;
    u16 refreshTimeUnits;
    
    u32 outputConcept;
    
    u32 considerationCount;
    Consideration considerations[4];
    
    union
    {
        MemSynthOption* next;
        MemSynthOption* nextFree;
    };
};
