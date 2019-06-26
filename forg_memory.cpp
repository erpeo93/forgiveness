inline MemAssociation* MemGetAssociation(Mem* memory, u16 index)
{
    MemAssociation* result = 0;
    if(index)
    {
        Assert(index < ASSOCIATION_COUNT);
        result = memory->associations + index;
    }
    return result;
}

inline u16 MemInvalidIndex()
{
    return 0xffff;
}
inline b32 ValidAssociationIndex(u16 index)
{
    b32 result = (index > 0 && index < MemInvalidIndex());
    return result;
}

inline u16* MemGetNextIndex(MemAssociation* association, u32 offset)
{
    u16* result = (u16*) ((u8*) association + offset);
    return result;
}

inline b32 MemHasToBeRefreshed(MemAssociation* association)
{
    b32 result = (association->refreshedCount > 0);
    return result;
}

inline b32 MemRefreshedByEveryone(MemAssociation* association)
{
    b32 result = (association->refreshedCount == 3);
    return result;
}

inline void MemAddTimeUnits(MemAssociation* association, i32 timeUnits)
{
    i32 newLastingTimeUnits = (i32) association->lastingTimeUnits + timeUnits;
    association->lastingTimeUnits = SafeTruncateToU16(Min(Max(newLastingTimeUnits, 0), 0xffff));
}

// NOTE(Leonardo): when an association is refreshed, all the association that share a concept or something are refreshed as well, but the "EXACT" association we refreshed get a bonus
inline void MemMarkToRefresh(MemAssociation* association, u16 refreshBonus)
{
    association->refreshedCount = 1;
    MemAddTimeUnits(association, refreshBonus);
}

inline b32 MemDeleted(MemAssociation* association)
{
    b32 result = (association->deletedCount > 0);
    return result;
}

inline b32 MemCompletelyDeleted(MemAssociation* association)
{
    b32 result = (association->deletedCount == 3);
    return result;
}





internal void MemListConsistency(Mem* memory, u16* firstAssociationIndex, u32 associationIndexOffset, u8 refreshedBonusUnits)
{
    for(u16* nextIndexPtr = firstAssociationIndex; *nextIndexPtr;)
    {
        u16 nextIndex = *nextIndexPtr;
        MemAssociation* next = MemGetAssociation(memory, nextIndex);
        
        u16* nextNextPtr = MemGetNextIndex(next, associationIndexOffset);
        
        if(MemHasToBeRefreshed(next))
        {
            ++next->refreshedCount;
            if(MemRefreshedByEveryone(next))
            {
                next->refreshedCount = 0;
            }
            
            u16 save = *nextIndexPtr;
            
            *nextIndexPtr = *nextNextPtr;
            *nextNextPtr = *firstAssociationIndex;
            *firstAssociationIndex = save;
            
            nextIndexPtr = nextNextPtr;
        }
        else
        {
            MemAddTimeUnits(next, refreshedBonusUnits);
            if(MemDeleted(next))
            {
                next->deletedCount += 1;
                *nextIndexPtr = *nextNextPtr;
            }
            else
            {
                nextIndexPtr = nextNextPtr;
            }
        }
    }
}

inline void HashListConsistency(Mem* memory, u16* indexPtr, u8* refreshBonusPtr, u32 counter, u32 offset, u32 associationIndexOffset)
{
    for(u32 conceptIndex = 0; conceptIndex < counter; ++conceptIndex)
    {
        u8 refreshBonus = *refreshBonusPtr;
        if(ValidAssociationIndex(*indexPtr))
        {
            MemListConsistency(memory, indexPtr, associationIndexOffset, refreshBonus);
        }
        
        indexPtr += offset;
        
        *refreshBonusPtr = 0;
        refreshBonusPtr += offset;
    }
}

#define MemGetHashSlotFun(type, baseType, name)\
inline type* MemGetHashSlot(type* hash, baseType key)\
{\
    type* result = 0;\
    baseType slotCount = ASSOCIATION_COUNT;\
    u16 startingIndex = SafeTruncateToU16(key & (ASSOCIATION_COUNT - 1));\
    u16 index = startingIndex;\
    while(true)\
    {\
        type* dest = hash + index;\
        if(!dest->firstAssociationIndex || dest->name == key)\
        {\
            result = dest;\
            break;\
        }\
        else if(dest->firstAssociationIndex == MemInvalidIndex())\
        {\
            result = dest;\
        }\
        if(++index == slotCount)\
        {\
            index = 0;\
        }\
        Assert(index != startingIndex);\
    }\
    return result;\
}

MemGetHashSlotFun(MemConcept, u32, taxonomy);
MemGetHashSlotFun(MemIdentifier, u64, identifier);


#define MemRemoveFromHashIfLastOneDef(type, baseType)\
inline void RemoveFromHashIfLastOne(type* hash, baseType key, u16 firstIndex)\
{\
    if(key)\
    {\
        type* slot = MemGetHashSlot(hash, key);\
        if(slot->firstAssociationIndex == firstIndex)\
        {\
            slot->firstAssociationIndex = MemInvalidIndex();\
        }\
    }\
}

MemRemoveFromHashIfLastOneDef(MemConcept, u32);
MemRemoveFromHashIfLastOneDef(MemIdentifier, u64);


#define MemAddToHashDef(type, baseType, name)\
inline void AddToHash(type* hash, baseType key, MemAssociation* newFirstAssociation,u16 newFirstIndex)\
{\
    type* slot = MemGetHashSlot(hash, key);\
    if(slot->name == key)\
    {\
        Assert(slot->firstAssociationIndex > 0);\
        newFirstAssociation->conceptNextIndex = slot->firstAssociationIndex;\
    }\
    else\
    {\
        slot->name = key;\
    }\
    slot->firstAssociationIndex = newFirstIndex;\
}

MemAddToHashDef(MemConcept, u32, taxonomy);
MemAddToHashDef(MemIdentifier, u64, identifier);


inline void MemAddAssociation(Mem* memory, u32 concept, u64 identifier, u16 refreshTimeUnits, u16 lastingtimeUnits)
{
    MemPortion* portion = &memory->shortTerm;
    if(portion->toInsertCount < ArrayCount(portion->toInsert))
    {
        MemAssociationToInsert* toInsert = portion->toInsert + portion->toInsertCount++;
        toInsert->targetAssociationIndex = 0;
        toInsert->refreshTimeUnits = refreshTimeUnits;
        toInsert->association.conceptTaxonomy = concept;
        toInsert->association.identifier = identifier;
        
        
        toInsert->association.deletedCount = 0;
        toInsert->association.refreshedCount = 0;
        toInsert->association.lastingTimeUnits = lastingtimeUnits;
    }
    else
    {
        InvalidCodePath;
    }
}


inline MemAssociation* MemAssociationPresent(Mem* memory, u32 concept, u64 identifier);
inline void MemUpdatePortion(Mem* memory, MemPortion* portion, r32 timeToUpdate)
{
    ZeroSize(ArrayCount(portion->lowest) * sizeof(portion->lowest[0]), portion->lowest);
    
    portion->timeToUpdate += timeToUpdate;
    
    i32 unitsToSubtract = 0;
    if(portion->timeToUpdate >= (r32) portion->secondsBetweenUpdates)
    {
        unitsToSubtract = (i32) -(portion->timeToUpdate / portion->decadenceFactor);
        portion->timeToUpdate = 0.0f;
    }
    
    for(u16 associationIndex = portion->firstIndex; associationIndex < portion->onePastLastIndex; ++associationIndex)
    {
        MemAssociation* association = memory->associations + associationIndex;
        if(!MemDeleted(association))
        {
            MemAddTimeUnits(association, unitsToSubtract);
            for(u32 lowestIndex = 0; lowestIndex < ArrayCount(portion->lowest); ++lowestIndex)
            {
                MemLowScoreAssociation* low = portion->lowest + lowestIndex;
                if((association->lastingTimeUnits < low->lastingTimeUnits) || !low->index)
                {
                    MemLowScoreAssociation temp = *low;
                    
                    portion->lowest[lowestIndex].index = associationIndex;
                    portion->lowest[lowestIndex].lastingTimeUnits = association->lastingTimeUnits;
                    
                    if(lowestIndex > 0)
                    {
                        portion->lowest[lowestIndex - 1] = temp;
                    }
                }
                else
                {
                    break;
                }
            }
        }
        else
        {
            if(!association->conceptNextIndex)
            {
                Assert(!association->identifierNextIndex);
                association->deletedCount = 3;
            }
        }
    }
    
#if 0    
    if(decade)
    {
        MemPortion* destPortion = ?;
        for(everyHighest)
        {
            if(highest->score > destPortion->lowest[].score)
            {
                ...
            }
        }
    }
#endif
    
    
    u32 currentLowestIndex = ArrayCount(portion->lowest) - 1;
    for(u32 toInsert = 0; toInsert < portion->toInsertCount; ++toInsert)
    {
        MemAssociationToInsert* newAssoc = portion->toInsert + toInsert;
        if(newAssoc->targetAssociationIndex)
        {
            MemAssociation* association = MemGetAssociation(memory, newAssoc->targetAssociationIndex);
            if(MemCompletelyDeleted(association))
            {
                RemoveFromHashIfLastOne(memory->conceptHash, association->conceptTaxonomy, newAssoc->targetAssociationIndex);
                RemoveFromHashIfLastOne(memory->identifierHash, association->identifier, newAssoc->targetAssociationIndex);
                
                *association = newAssoc->association;
                AddToHash(memory->conceptHash, association->conceptTaxonomy, association, newAssoc->targetAssociationIndex);
                AddToHash(memory->identifierHash, association->identifier, association, newAssoc->targetAssociationIndex);
                newAssoc->association.deletedCount = 1;
            }
            else
            {
                Assert(association->deletedCount == 1);
            }
            
        }
        else
        {
            MemAssociation* association = MemAssociationPresent(memory, newAssoc->association.conceptTaxonomy, newAssoc->association.identifier);
            if(association)
            {
                MemMarkToRefresh(association, newAssoc->refreshTimeUnits);
                //MemRefresh(association->concept, newAssoc->refreshRule);
                //MemRefresh(association->identifier, newAssoc->refreshRule);
                
                newAssoc->association.deletedCount = 1;
            }
            else
            {
                MemLowScoreAssociation* lowest = portion->lowest + currentLowestIndex;
                if(newAssoc->association.lastingTimeUnits > lowest->lastingTimeUnits)
                {
                    newAssoc->targetAssociationIndex = lowest->index;
                    MemAssociation* target = MemGetAssociation(memory, lowest->index);
                    Assert(!target->deletedCount);
                    target->deletedCount = 1;
                    
                    --currentLowestIndex;
                }
                else
                {
                    newAssoc->association.deletedCount = 1;
                }
            }
        }
    }
    
    for(u32 toInsert = 0; toInsert < portion->toInsertCount; ++toInsert)
    {
        MemAssociationToInsert* newAssoc = portion->toInsert + toInsert;
        while(newAssoc->association.deletedCount == 1)
        {
            *newAssoc = portion->toInsert[--portion->toInsertCount];
            if(toInsert == portion->toInsertCount)
            {
                break;
            }
        }
    }
}

internal void MemUpdate(TaxonomyTable* table, Mem* memory, u32 entityTaxonomy, r32 timeToAdvance)
{
    Assert(ASSOCIATION_COUNT < 0xffff);  
    for(u32 queueIndex = 0; queueIndex < ArrayCount(memory->actionQueueHash); ++queueIndex)
    {
        MemAction* toProcess = memory->actionQueueHash + queueIndex;
        
        if(toProcess->identifier)
        {
            MemSynthesisRule* rule = 0;
            
            b32 ruleApplied = false;
            u32 currentTaxonomy = entityTaxonomy;
            while(currentTaxonomy && !ruleApplied)
            {
                TaxonomySlot* slot = GetSlotForTaxonomy(table, currentTaxonomy);
                for(TaxonomyMemBehavior* memBehavior = slot->firstMemBehavior; memBehavior  && !ruleApplied; memBehavior = memBehavior->next)
                {
                    TaxonomySlot* memBehaviorSlot = GetSlotForTaxonomy(table, memBehavior->taxonomy);
                    for(MemSynthesisRule* test = memBehaviorSlot->firstSynthRule; test && !ruleApplied; test = test->next)
                    {
                        if(test->action == toProcess->action)
                        {
                            TaxonomyNode* node = FindInTaxonomyTree(table, test->tree.root, toProcess->taxonomy);
                            if(node)
                            {
                                for(MemSynthOption* option = node->data.firstOption; option; option = option->next)
                                {
                                    if(true)//if(evaluateConsiderations(option->considerations))
                                    {
                                        MemAddAssociation(memory, option->outputConcept, toProcess->identifier, option->refreshTimeUnits, option->lastingtimeUnits);
                                        ruleApplied = true;
                                    }
                                    break;
                                }
                            }
                        }
                    }
                }
                currentTaxonomy = GetParentTaxonomy(table, currentTaxonomy);
            }
            toProcess->identifier = 0;
        }
    }
    
    HashListConsistency(memory, &memory->conceptHash[0].firstAssociationIndex,
                        &memory->conceptHash[0].refreshTimeUnitsBonus, ArrayCount(memory->conceptHash), sizeof(MemConcept), OffsetOf(MemAssociation, conceptNextIndex));
    
    HashListConsistency(memory, &memory->identifierHash[0].firstAssociationIndex,
                        &memory->identifierHash[0].refreshTimeUnitsBonus,ArrayCount(memory->identifierHash), sizeof(MemIdentifier), OffsetOf(MemAssociation, identifierNextIndex));
    
    
    //for(everymemPortion)
    {
        MemUpdatePortion(memory, &memory->shortTerm, timeToAdvance);
    }
}

inline void MemPerceive(SimRegion* region, SimEntity* self, Mem* memory)
{
    r32 perceiveRadious = 6.0f;
    r32 perceiveRadiousSq = Square(perceiveRadious);
    
    RegionPartitionQueryResult query = QuerySpacePartition(region, &region->collisionPartition, self->P, V3(0, 0, 0), RectCenterDim(V3(0, 0, 0), V3(perceiveRadious, perceiveRadious, perceiveRadious)));
    for(u32 surfaceIndex = 0; surfaceIndex < ArrayCount(query.surfaceIndexes); ++surfaceIndex)
    {
        RegionPartitionSurface* surface = region->collisionPartition.partitionSurfaces + query.surfaceIndexes[surfaceIndex];
        
        PartitionSurfaceEntityBlock* block = surface->first;
        while(block)
        {
            for(u32 blockIndex = 0; blockIndex < block->entityCount; ++blockIndex)
            {
                CollisionData* collider = block->colliders + blockIndex;
                SimEntity* entity = GetRegionEntity(region, collider->entityIndex);
                if(entity->identifier != self->identifier)
                {
                    Vec3 distance = entity->P - self->P;
                    
                    if(LengthSq(distance) < perceiveRadiousSq)
                    {
                        u32 actionHashIndex = entity->identifier & (ArrayCount(memory->actionQueueHash) - 1);
                        b32 inserted = false;
                        
                        for(u32 attemptIndex = 0; attemptIndex < 3; ++attemptIndex)
                        {
                            MemAction* dest = memory->actionQueueHash + actionHashIndex;
                            if(!dest->identifier || dest->identifier == entity->identifier)
                            {
                                dest->identifier = entity->identifier;
                                dest->action = entity->action;
                                dest->taxonomy = entity->taxonomy;
                                inserted = true;
                                break;
                            }
                            
                            if(++actionHashIndex == ArrayCount(memory->actionQueueHash))
                            {
                                actionHashIndex = 0;
                            }
                        }
                        //Assert(inserted);
                    }
                }
            }
            
            block = block->next;
        }
    }
}

inline MemAssociation* MemGetAssociations(Mem* memory, u32 conceptTaxonomy)
{
    MemAssociation* result = 0;
    MemConcept* slot = MemGetHashSlot(memory->conceptHash, conceptTaxonomy);
    if(ValidAssociationIndex(slot->firstAssociationIndex))
    {
        result = MemGetAssociation(memory, slot->firstAssociationIndex);
    }
    return result;
}

inline MemAssociation* MemGetAssociations(Mem* memory, u64 identifier)
{
    MemAssociation* result = 0;
    MemIdentifier* slot = MemGetHashSlot(memory->identifierHash, identifier);
    if(ValidAssociationIndex(slot->firstAssociationIndex))
    {
        result = MemGetAssociation(memory, slot->firstAssociationIndex);
    }
    return result;
}

inline MemAssociation* MemAssociationPresent(Mem* memory, u32 concept, u64 identifier)
{
    MemAssociation* result = 0;
    
    MemAssociation* association = MemGetAssociations(memory, identifier);
    while(association)
    {
        if(association->conceptTaxonomy == concept)
        {
            result = association;
            break;
        }
        
        association = MemGetAssociation(memory, association->identifierNextIndex);
    }
    
    return result;
}
