#include <stdio.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>

inline void FreeActionTree(TaxonomyTable* table, TaxonomyNode* root)
{    
    if(root->firstChild)
    {
        FreeActionTree(table, root->firstChild);
    }
    
    if(root->next)
    {
        FreeActionTree(table, root->next);
    }
    
    FREELIST_DEALLOC(root, table->firstFreeTaxonomyNode);
}

inline void FreeMemSynthRuleTree(TaxonomyTable* table, TaxonomyNode* root)
{
    FREELIST_FREE(root->data.firstOption, MemSynthOption, table->firstFreeMemSynthOption);
    
    if(root->firstChild)
    {
        FreeMemSynthRuleTree(table, root->firstChild);
    }
    
    if(root->next)
    {
        FreeMemSynthRuleTree(table, root->next);
    }
    
    FREELIST_DEALLOC(root, table->firstFreeTaxonomyNode);
}

inline ShortcutSlot* GetShortcut(TaxonomyTable* table, u64 nameHash)
{
    u32 index =  nameHash & (ArrayCount(table->shortcutSlots) - 1);
    ShortcutSlot* result = table->shortcutSlots[index];
    
    while(result && (result->hashIndex != nameHash))
    {
        result = result->nextInHash;
    }
    
    return result;
}

inline ShortcutSlot* GetShortcut(TaxonomyTable* table, char* name, u32 nameLength = 0)
{
    if(!nameLength)
    {
        nameLength = StrLen(name);
    }
    
    u64 hashIndex = StringHash(name, nameLength);
    ShortcutSlot* result = GetShortcut(table, hashIndex);
    
    return result;
}

inline TaxonomySlot* GetSlotForTaxonomy(TaxonomyTable* table, u32 taxonomy, MemoryPool* pool = 0)
{
    TaxonomySlot* result;
    if(!taxonomy)
    {
        result = &table->root;
    }
    else
    {
        u32 index = taxonomy & (ArrayCount(table->slots) - 1);
        result = table->slots[index];
        while(result && result->taxonomy && result->taxonomy != taxonomy)
        {
            Assert(result->taxonomy > 0);
            result = result->nextInHash;
        }
        
        if(!result && pool)
        {
            result = PushStruct(pool, TaxonomySlot);
            result->nextInHash = table->slots[index];
            table->slots[index] = result;
        }
    }
    
    return result;
}

inline TaxonomySlot* NORUNTIMEGetTaxonomySlotByName(TaxonomyTable* table, char* name, u32 nameLength = 0)
{
    if(!nameLength)
    {
        nameLength = StrLen(name);
    }
    TaxonomySlot* result = 0;
    if(StrEqual(nameLength, name, "root"))
    {
        result = &table->root;
    }
    else
    {
        ShortcutSlot* shortcut = GetShortcut(table, name, nameLength);
        if(shortcut)
        {
            result = GetSlotForTaxonomy(table, shortcut->taxonomy);
        }
    }
    return result;
}

inline b32 IsSubTaxonomy(u32 child, u32 parent, u32 significantBits)
{
    b32 result;
    if(!parent)
    {
        result = true;
    }
    else
    {
        u32 bitsToShift = 32 - significantBits;
        result = (parent >> bitsToShift) == (child >> bitsToShift);
    }
    return result;
}

inline b32 IsSubTaxonomy(u32 child, TaxonomySlot* parent)
{
    b32 result;
    if(parent->subTaxonomiesCount)
    {
        u32 significantBits = parent->usedBitsTotal - parent->necessaryBits;
        result = IsSubTaxonomy(child, parent->taxonomy, significantBits);
    }
    else
    {
        result = (child == parent->taxonomy);
    }
    
    return result;
}

inline b32 IsSubTaxonomy(TaxonomyTable* table, u32 child, u32 parent)
{
    TaxonomySlot* parentSlot = GetSlotForTaxonomy(table, parent);
    b32 result = IsSubTaxonomy(child, parentSlot);
    
    return result;
}


inline u32 GetParentTaxonomy(TaxonomySlot* slot)
{
    u32 result =  slot->taxonomy & (0xFFFFFFFF << (32 - (slot->usedBitsTotal - (slot->necessaryBits + slot->parentNecessaryBits))));
    if(result == slot->taxonomy)
    {
        result = 0;
    }
    return result;
}

inline u32 GetParentTaxonomy(TaxonomyTable* table, u32 taxonomy)
{
    TaxonomySlot* slot = GetSlotForTaxonomy(table, taxonomy);
    u32 result = GetParentTaxonomy(slot);
    return result;
}

inline TaxonomySlot* GetParentSlot(TaxonomyTable* table, TaxonomySlot* slot)
{
    u32 parentTaxonomy = GetParentTaxonomy(slot);
    TaxonomySlot* result = GetSlotForTaxonomy(table, parentTaxonomy);
    return result;
}

inline u32 GetChildTaxonomy(TaxonomySlot* slot, u32 taxonomy)
{
    u32 result = taxonomy & (0xffffffff << (32 - slot->usedBitsTotal));
    
    return result;
}

inline TaxonomySlot* GetChildSlot(TaxonomyTable* table, TaxonomySlot* slot, u32 taxonomy)
{
    u32 childTaxonomy = GetChildTaxonomy(slot, taxonomy);
    TaxonomySlot* result = GetSlotForTaxonomy(table, childTaxonomy);
    return result;
}

inline u32 GetNthChildTaxonomy(TaxonomyTable* table, TaxonomySlot* slot, u32 childIndex)
{
    Assert(childIndex < slot->subTaxonomiesCount);
    u32 result = slot->taxonomy | ((childIndex + 1) << (32 - slot->usedBitsTotal));
    return result;
}

inline TaxonomySlot* GetNthChildSlot(TaxonomyTable* table, TaxonomySlot* slot, u32 childIndex)
{
    Assert(childIndex < slot->subTaxonomiesCount);
    u32 taxonomy = slot->taxonomy | ((childIndex + 1) << (32 - slot->usedBitsTotal));
    TaxonomySlot* result = GetSlotForTaxonomy(table, taxonomy);
    
    return result;
}

inline TaxonomySlot* GetNthChildSlot(TaxonomyTable* table, u32 taxonomy, u32 childIndex)
{
    TaxonomySlot* slot = GetSlotForTaxonomy(table, taxonomy);
    TaxonomySlot* result = GetNthChildSlot(table, slot, childIndex);
    return result;
}

inline u32 GetRandomChild(TaxonomyTable* table, RandomSequence* sequence, u32 taxonomy)
{
    u32 result = taxonomy;
    
    TaxonomySlot* slot = GetSlotForTaxonomy(table, result);
    while(slot->subTaxonomiesCount)
    {
        u32 validSubTaxonomyCount = slot->subTaxonomiesCount - slot->invalidTaxonomiesCount;
        result = result | ((RandomChoice(sequence, validSubTaxonomyCount) + 1) << (32 - slot->usedBitsTotal));
        slot = GetChildSlot(table, slot, result);
    }
    
    return result;
}

inline b32 IsObject(TaxonomyTable* table, u32 taxonomy)
{
    
    b32 result = IsSubTaxonomy(taxonomy, table->objectTaxonomy, table->rootBits);
    return result;
}

inline b32 IsEquipment(TaxonomyTable* table, u32 taxonomy)
{
    
    b32 result = IsSubTaxonomy(taxonomy, table->equipmentTaxonomy, table->rootBits);
    return result;
}

inline b32 IsPlant(TaxonomyTable* table, u32 taxonomy)
{
    b32 result = IsSubTaxonomy(taxonomy, table->plantTaxonomy, table->rootBits);
    return result;
}

inline b32 IsCreature(TaxonomyTable* table, u32 taxonomy)
{
    b32 result = IsSubTaxonomy(taxonomy, table->creatureTaxonomy, table->rootBits);
    return result;
}

inline b32 IsFluid(TaxonomyTable* table, u32 taxonomy)
{
    b32 result = IsSubTaxonomy(taxonomy, table->fluidTaxonomy, table->rootBits);
    return result;
}

inline b32 IsRock(TaxonomyTable* table, u32 taxonomy)
{
    b32 result = IsSubTaxonomy(taxonomy, table->rockTaxonomy, table->rootBits);
    return result;
}

inline b32 IsEssence(TaxonomyTable* table, u32 taxonomy)
{
    b32 result = IsSubTaxonomy(taxonomy, table->essenceTaxonomy, table->rootBits);
    return result;
}

inline b32 IsEffect(TaxonomyTable* table, u32 taxonomy)
{
    b32 result = IsSubTaxonomy(taxonomy, table->effectTaxonomy, table->rootBits);
    return result;
}

inline b32 IsBehavior(TaxonomyTable* table, u32 taxonomy)
{
    b32 result = IsSubTaxonomy(taxonomy, table->behaviorTaxonomy, table->rootBits);
    return result;
}

inline b32 IsTile(TaxonomyTable* table, u32 taxonomy)
{
    b32 result = IsSubTaxonomy(taxonomy, table->tileTaxonomy, table->rootBits);
    return result;
}

inline b32 IsGenerator(TaxonomyTable* table, u32 taxonomy)
{
    b32 result = IsSubTaxonomy(taxonomy, table->generatorTaxonomy, table->rootBits);
    return result;
}

inline b32 IsSpawnable(TaxonomyTable* table, u32 taxonomy)
{
    b32 result = true;
    if(IsTile(table, taxonomy) || IsGenerator(table, taxonomy))
    {
        result = false;
    }
    
    return result;
}

inline AttributeSlot* GetAttribute(TaxonomySlot* slot, u32 offsetOf)
{
    AttributeSlot* result = 0;
    u32 index = offsetOf & (ArrayCount(slot->attributeHashmap) - 1);
    
    for(;;)
    {
        AttributeSlot* test  = slot->attributeHashmap + index;
        if(!test->offsetFromBase)
        {
            break;
        }
        else if(test->offsetFromBase == offsetOf)
        {
            result = test;
            break;
        }
        
        if(++index == ArrayCount(slot->attributeHashmap))
        {
            index = 0;
        }
    }
    
    return result;
}

inline AttributeSlot* GetAttributeRecursively(TaxonomyTable* table, TaxonomySlot* slot, u32 offsetOf)
{
    while(slot->taxonomy)
    {
        IsObject(table, slot->taxonomy);
        AttributeSlot* attr = GetAttribute(slot, offsetOf);
        if(attr)
        {
            return attr;
        }
        
        slot = GetParentSlot(table, slot);
    }
}


inline AttributeSlot* GetAttributeSlot(TaxonomySlot* slot, u32 offsetOf)
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


inline TaxonomyNode* FindInTaxonomyTree(TaxonomyTable* table, TaxonomyNode* tree, u32 taxonomy)
{
    TaxonomyNode* result = tree;
    TaxonomyNode* current = tree;
    while(current)
    {
        if(current->key == taxonomy)
        {
            result = current;
            break;
        }
        
        
        TaxonomyNode* nextCurrent = 0;
        TaxonomyNode* first = current->firstChild;
        while(first)
        {
            TaxonomySlot* slot = GetSlotForTaxonomy(table, first->key);
            if(IsSubTaxonomy(taxonomy, slot))
            {
                nextCurrent = first;
                result = first;
                break;
            }
            
            first = first->next;
        }
        
        current = nextCurrent;
    }
    
    return result;
}

inline Vec3 GetRockDim(RockDefinition* rock, RandomSequence* sequence)
{
    Vec3 result = rock->scale + Hadamart(rock->scaleDelta, RandomBilV3(sequence));
    return result;
}

inline void GetPhysicalProperties(TaxonomyTable* taxTable, u32 taxonomy, u64 identifier, ForgBoundType* type, Rect3* bounds, r32 generationIntensity)
{
    TaxonomySlot* boundSlot = GetSlotForTaxonomy(taxTable, taxonomy);
    if(IsRock(taxTable, taxonomy))
    {
        RandomSequence rockSeq = Seed((u32) identifier);
        if(boundSlot->rock->collides)
        {
            *type = ForgBound_Standard;
        }
        else
        {
            *type = ForgBound_NonPhysical;
        }
        
        Vec3 rockDim = GetRockDim(boundSlot->rock, &rockSeq);
        *bounds = RectCenterDim(V3(0, 0, 0), rockDim);
    }
    else if(IsPlant(taxTable, taxonomy))
    {
        if(boundSlot->plant->collides)
        {
            *type = ForgBound_Standard;
        }
        else
        {
            *type = ForgBound_NonPhysical;
        }
        
        r32 trunkRadious = Max(0.2f, GetPlantStandardTrunkRadious(boundSlot->plant));
        r32 trunkLength = GetPlantStandardTrunkLength(boundSlot->plant);
        
        Vec3 min = V3(-trunkRadious, -trunkRadious, 0);
        Vec3 max = V3(trunkRadious, trunkRadious, trunkLength);
        
        *bounds = RectMinMax(min, max);
    }
    else
    {
        while(boundSlot->taxonomy)
        {
            if(boundSlot->boundType)
            {
                *type = boundSlot->boundType;
                Rect3 slotBound = boundSlot->physicalBounds;
                if(boundSlot->scaleDimBasedOnIntensity)
                {
                    Assert(Normalized(generationIntensity));
                    r32 coeff = UnilateralToBilateral(generationIntensity) * boundSlot->scaleDimGenCoeffV;
                    slotBound = Scale(slotBound, 1.0f + coeff);
                }
                
                *bounds = slotBound;
                break;
            }
            
            boundSlot = GetParentSlot(taxTable, boundSlot);
        }
    }
}

inline void BuildTaxonomyDataPath(TaxonomyTable* table, u32 parentTaxonomy, char* name, char* path, u32 pathSize, char* copyFrom, u32 copyFromSize)
{
    
    char* pathHere = path;
    unm remaining = pathSize;
    
    unm written = FormatString(pathHere, remaining, "definition/");
    remaining -= written;
    pathHere += written;
    
    
    TaxonomySlot* root = GetSlotForTaxonomy(table, 0);
    TaxonomySlot* current = root;
    while(true)
    {
        written = FormatString(pathHere, remaining, "%s/", current->name);
        remaining -= written;
        pathHere += written;
        
        if(current->taxonomy == parentTaxonomy)
        {
            break;
        }
        
        current = GetChildSlot(table, current, parentTaxonomy);
    }
    
    
    FormatString(pathHere, remaining, "%s", name);
    
    
    TaxonomySlot* copyFromSlot = GetChildSlot(table, root, parentTaxonomy);
    
    FormatString(copyFrom, copyFromSize, "definition/%s/%s", root->name, copyFromSlot->name);
    
    
}

inline u32 TranslateTaxonomy(TaxonomyTable* oldTable, TaxonomyTable* newTable, u32 taxonomy)
{
    u32 result = taxonomy;
    if(result)
    {
        u32 currentTaxonomy = taxonomy;
        while(currentTaxonomy)
        {
            TaxonomySlot* slot = GetSlotForTaxonomy(oldTable, currentTaxonomy);
            if(slot)
            {
                u64 hashID = slot->stringHashID;
                ShortcutSlot* shortcut = GetShortcut(newTable, hashID);
                if(shortcut)
                {
                    RandomSequence* seq = &newTable->translateSequence;
                    result = GetRandomChild(newTable, seq, shortcut->taxonomy);
                    
                    TaxonomySlot* newSlot = GetSlotForTaxonomy(newTable, result);
                    Assert(newSlot->taxonomy == result);
                    break;
                }
                
                currentTaxonomy = GetParentTaxonomy(oldTable, currentTaxonomy);
            }
            else
            {
                break;
            }
        }
    }
    return result;
}


inline void TranslateObject(TaxonomyTable* oldTable, TaxonomyTable* newTable, Object* object)
{
    object->taxonomy = TranslateTaxonomy(oldTable, newTable, object->taxonomy);
}


#if FORG_SERVER
inline void TranslateEffect(TaxonomyTable* oldTable, TaxonomyTable* newTable, Effect* effect)
{
    effect->data.taxonomy = TranslateTaxonomy(oldTable, newTable, effect->data.taxonomy);
}


inline void TranslateEffectComponent(EntityComponentArray* components, TaxonomyTable* oldTable, TaxonomyTable* newTable, u32 ID)
{
    EffectComponent* component = Effects_(components, ID);
    for(u32 effectIndex = 0; effectIndex < component->effectCount; ++effectIndex)
    {
        Effect* effect = component->effects + effectIndex;
        TranslateEffect(oldTable, newTable, effect);
    }
}

inline void TranslatePlantComponent(EntityComponentArray* components, TaxonomyTable* oldTable, TaxonomyTable* newTable, u32 ID)
{
    // NOTE(Leonardo): nothing to do!
}

inline void TranslateContainerComponent(EntityComponentArray* components, TaxonomyTable* oldTable, TaxonomyTable* newTable, u32 ID)
{
    ContainerComponent* objects = Container_(components, ID);
    ContainedObjects* objs = &objects->objects;
    
    for(u8 objectIndex = 0; objectIndex < objs->objectCount; ++objectIndex)
    {
        Object* object = objs->objects + objectIndex;
        TranslateObject(oldTable, newTable, object);
    }
}

inline void TranslateFluidComponent(EntityComponentArray* components, TaxonomyTable* oldTable, TaxonomyTable* newTable, u32 ID)
{
    // NOTE(Leonardo): nothing to do!
}


inline void TranslateMemAssociation(TaxonomyTable* oldTable, TaxonomyTable* newTable, MemAssociation* association)
{
    association->conceptTaxonomy = TranslateTaxonomy(oldTable, newTable, association->conceptTaxonomy);
}

inline void TranslateMemory(TaxonomyTable* oldTable, TaxonomyTable* newTable, Mem* mem)
{
    for(u32 associationIndex = 0; associationIndex < ASSOCIATION_COUNT; ++associationIndex)
    {
        MemConcept* concept = mem->conceptHash + associationIndex;
        concept->taxonomy = TranslateTaxonomy(oldTable, newTable, concept->taxonomy);
        
        MemAssociation* association = mem->associations + associationIndex;
        TranslateMemAssociation(oldTable, newTable, association);
    }
    
    
    MemPortion* portion = &mem->shortTerm;
    for(u32 toInsertIndex = 0; toInsertIndex < portion->toInsertCount; ++toInsertIndex)
    {
        MemAssociationToInsert* toInsert = portion->toInsert + toInsertIndex;
        TranslateMemAssociation(oldTable, newTable, &toInsert->association);
    }
    
    for(u32 actionIndex = 0; actionIndex < ArrayCount(mem->actionQueueHash); ++actionIndex)
    {
        MemAction* action = mem->actionQueueHash + actionIndex;
        action->taxonomy = TranslateTaxonomy(oldTable, newTable, action->taxonomy);
    }
}

inline void TranslateBrain(TaxonomyTable* oldTable, TaxonomyTable* newTable, Brain* brain)
{
    for(u32 behaviorIndex = 0; behaviorIndex < brain->behaviorCount; ++behaviorIndex)
    {
        BrainBehavior* behavior = brain->behaviorStack + behaviorIndex;
        behavior->taxonomy = TranslateTaxonomy(oldTable, newTable, behavior->taxonomy);
    }
    
    for(u32 conceptIndex = 0; conceptIndex < ArrayCount(brain->conceptTargets); ++conceptIndex)
    {
        AIConceptTargets* targets = brain->conceptTargets + conceptIndex;
        targets->conceptTaxonomy = TranslateTaxonomy(oldTable, newTable, targets->conceptTaxonomy);
    }
    
    for(u32 conceptIndex = 0; conceptIndex < ArrayCount(brain->newConceptTargets); ++conceptIndex)
    {
        AIConceptTargets* targets = brain->newConceptTargets + conceptIndex;
        targets->conceptTaxonomy = TranslateTaxonomy(oldTable, newTable, targets->conceptTaxonomy);
    }
    
    
    TranslateMemory(oldTable, newTable, &brain->memory);
}

inline void TranslateCreatureComponent(EntityComponentArray* components, TaxonomyTable* oldTable, TaxonomyTable* newTable, u32 ID)
{
    CreatureComponent* creature = Creature_(components, ID);
    TranslateBrain(oldTable, newTable, &creature->brain);
    
    for(u32 skillIndex = 0; skillIndex < creature->skillCount; ++skillIndex)
    {
        SkillSlot* skill = creature->skills + skillIndex;
        skill->taxonomy = TranslateTaxonomy(oldTable, newTable, skill->taxonomy);
    }
    
    for(u32 recipeIndex = 0; recipeIndex < creature->recipeCount; ++recipeIndex)
    {
        Recipe* recipe = creature->recipes + recipeIndex;
        recipe->taxonomy = TranslateTaxonomy(oldTable, newTable, recipe->taxonomy);
    }
    
    for(u32 skillIndex = 0; skillIndex < MAX_PASSIVE_SKILLS_ACTIVE; ++skillIndex)
    {
        SkillSlot* passiveSkill = creature->passiveSkills + skillIndex;
        passiveSkill->taxonomy = TranslateTaxonomy(oldTable, newTable, passiveSkill->taxonomy);
        
        PassiveSkillEffects* effects = creature->passiveSkillEffects + skillIndex;
        for(u32 effectIndex = 0; effectIndex < effects->effectCount; ++effectIndex)
        {
            Effect* effect = effects->effects + effectIndex;
            TranslateEffect(oldTable, newTable, effect);
        }
    }
    
    for(u32 essenceIndex = 0; essenceIndex < MAX_DIFFERENT_ESSENCES; ++essenceIndex)
    {
        EssenceSlot* essence = creature->essences + essenceIndex;
        essence->taxonomy = TranslateTaxonomy(oldTable, newTable, essence->taxonomy);
    }
}


inline void TranslateSimEntity(ServerState* server, SimEntity* entity)
{
    TaxonomyTable* newTable = server->activeTable;
    TaxonomyTable* oldTable = server->oldTable;
    
    entity->taxonomy = TranslateTaxonomy(oldTable, newTable, entity->taxonomy);
    entity->recipeTaxonomy = TranslateTaxonomy(oldTable, newTable, entity->recipeTaxonomy);
    
    EntityComponentArray* components = server->components;
    
    if(entity->IDs[Component_Effect])
    {
        TranslateEffectComponent(components, oldTable, newTable, entity->IDs[Component_Effect]);
    }
    
    if(entity->IDs[Component_Plant])
    {
        TranslatePlantComponent(components, oldTable, newTable, entity->IDs[Component_Plant]);
    }
    
    if(entity->IDs[Component_Container])
    {
        TranslateContainerComponent(components, oldTable, newTable, entity->IDs[Component_Container]);
    }
    
    if(entity->IDs[Component_Fluid])
    {
        TranslateFluidComponent(components, oldTable, newTable, entity->IDs[Component_Fluid]);
    }
    
    if(entity->IDs[Component_Creature])
    {
        TranslateCreatureComponent(components, oldTable, newTable, entity->IDs[Component_Creature]);
    }
}

inline void TranslatePlayer(TaxonomyTable* oldTable, TaxonomyTable* newTable, ServerPlayer* player)
{
    for(u32 categoryIndex = 0; categoryIndex < player->unlockedCategoryCount; ++categoryIndex)
    {
        player->unlockedSkillCategories[categoryIndex] = TranslateTaxonomy(oldTable, newTable, player->unlockedSkillCategories[categoryIndex]);
    }
    
    
    for(u32 recipeIndex = 0; recipeIndex < player->recipeCount; ++recipeIndex)
    {
        Recipe* recipe = player->recipes + recipeIndex;
        recipe->taxonomy = TranslateTaxonomy(oldTable, newTable, recipe->taxonomy);
    }
}
#endif

#ifndef FORG_SERVER
inline void TranslateClientEntity(TaxonomyTable* oldTable, TaxonomyTable* newTable,ClientEntity* entity)
{
    entity->taxonomy = TranslateTaxonomy(oldTable, newTable, entity->taxonomy);
    entity->recipeTaxonomy = TranslateTaxonomy(oldTable, newTable, entity->recipeTaxonomy);
    
    for(u8 objectIndex = 0; objectIndex < entity->objects.objectCount; ++objectIndex)
    {
        Object* object = entity->objects.objects + objectIndex;
        TranslateObject(oldTable, newTable, object);
    }
    
    for(ClientAnimationEffect* effect = entity->firstActiveEffect; effect; effect = effect->next)
    {
        effect->effect.triggerEffectTaxonomy = TranslateTaxonomy(oldTable, newTable, effect->effect.triggerEffectTaxonomy);
    }
    
    entity->prediction = {};
}

inline void TranslateClientPlayer(TaxonomyTable* oldTable, TaxonomyTable* newTable, ClientPlayer* player)
{
    for(u32 essenceIndex = 0; essenceIndex < player->essenceCount; ++essenceIndex)
    {
        EssenceSlot* essence = player->essences + essenceIndex;
        essence->taxonomy = TranslateTaxonomy(oldTable, newTable, essence->taxonomy);
    }
}

inline void TranslateUI(TaxonomyTable* oldTable, TaxonomyTable* newTable, UIState* UI)
{
    Assert(UI->draggingEntity.taxonomy == 0);
    
    for(u32 skillIndex = 0; skillIndex < MAXIMUM_SKILL_SLOTS; ++skillIndex)
    {
        UISkill* skill = UI->skills + skillIndex;
        skill->taxonomy = TranslateTaxonomy(oldTable, newTable, skill->taxonomy);
    }
    
    for(u32 modeIndex = 0; modeIndex < UIBook_Count; ++modeIndex)
    {
        BookMode* mode = UI->bookModes + modeIndex;
        for(BookElementsBlock* block = mode->elements; block; block = block->next)
        {
            for(u32 elementIndex = 0; elementIndex < block->elementCount; ++elementIndex)
            {
                BookElement* element = block->elements + elementIndex;
                element->taxonomy = TranslateTaxonomy(oldTable, newTable, element->taxonomy);
            }
        }
    }
}

inline void TranslateParticleEffects(ParticleCache* cache, TaxonomyTable* oldTable, TaxonomyTable* newTable)
{
    InvalidCodePath;
}
#endif











