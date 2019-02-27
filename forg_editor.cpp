#include "miniz.c"

inline ShortcutSlot* SaveShortcut(TaxonomyTable* table, char* name, u32 nameLength, MemoryPool* pool)
{
    if(!nameLength)
    {
        nameLength = StrLen(name);
    }
    Assert(pool);
    u64 hashIndex = StringHash(name, nameLength);
    u32 index =  hashIndex & (ArrayCount(table->shortcutSlots) - 1);
    ShortcutSlot* slot = table->shortcutSlots[index];
    while(slot)
    {
        Assert(!StrEqual(slot->name, name));
		Assert(slot->hashIndex != hashIndex);
        slot = slot->nextInHash;
    }
    
    if(!slot)
    {
        slot = PushStruct(pool, ShortcutSlot);
        slot->nextInHash = table->shortcutSlots[index];
        table->shortcutSlots[index] = slot;
    }
    
    StrCpy(name, nameLength, slot->name, ArrayCount(slot->name));
    slot->hashIndex = hashIndex;
    
    return slot;
}


inline void GetNameWithoutPoint(char* dest, u32 destLength, char* source)
{
    Assert(StrLen(source) < destLength);
    char* toCopy = source;
    while(*toCopy )
    {
        if(*toCopy == '.' )
        {
            break;
        }
        
        *dest++ = *toCopy++;
    }
    *dest++ = 0;
}


global_variable TaxonomyTable* taxTable_;
global_variable MemoryPool* taxPool_;
global_variable TaxonomySlot* currentSlot_;
TaxonomyComponent* currentActiveComponent_;

inline void InitTaxonomyReadWrite(TaxonomyTable* table)
{
    taxTable_ = table;
    taxPool_ = &table->pool;
    currentActiveComponent_ = 0;
}



global_variable u32 stackShortcutCount = 0;
global_variable ShortcutSlot* shortcutStack[32];

inline void AddSubTaxonomy(ShortcutSlot* shortcut, char* name, u32 nameLength)
{
    if(nameLength == 0)
    {
        nameLength = StrLen(name);
    }
    
    Assert(shortcut->subTaxonomiesCount < ArrayCount(shortcut->subTaxonomies));
    char* dest = shortcut->subTaxonomies[shortcut->subTaxonomiesCount++];
    StrCpy(name, nameLength, dest, ArrayCount(shortcut->subTaxonomies[0]));
}

#define Push(name) Push_(#name)
inline void Push_(char* name, u32 nameLength = 0)
{
    ShortcutSlot* result = 0;
    Assert(stackShortcutCount < ArrayCount(shortcutStack));
    ShortcutSlot * currentSlot = shortcutStack[stackShortcutCount - 1];
    
    AddSubTaxonomy(currentSlot, name, nameLength);
    result = SaveShortcut(taxTable_, name, nameLength, taxPool_);
    
    shortcutStack[stackShortcutCount++] = result;
}

inline void Pop()
{
    Assert(stackShortcutCount > 0);
    --stackShortcutCount;
}

inline void WriteDataFilesRecursive(char* path, char* name)
{
    if(!StrEqual(name, ".") && !StrEqual(name, "..") && !StrEqual(name, "side") && name[0] != '#')
    {
        Push_(name);
        
        char newPath[512];
        FormatString(newPath, sizeof(newPath), "%s/%s", path, name);
        
        PlatformSubdirNames subdir;
        subdir.subDirectoryCount = 0;
        platformAPI.GetAllSubdirectoriesName(&subdir, newPath);
        
        char copyPath[512];
        FormatString(copyPath, sizeof(copyPath), "%s/*.fed", newPath);
        
		platformAPI.CopyAllFiles(copyPath, "assets");
        
        for(u32 subdirIndex = 0; subdirIndex < subdir.subDirectoryCount; ++subdirIndex)
        {
            char* subName = subdir.subdirs[subdirIndex];
            char nameWithoutPoint[64];
            GetNameWithoutPoint(nameWithoutPoint, ArrayCount(nameWithoutPoint), subName + 1);
            WriteDataFilesRecursive(newPath, subName);
        }
        
        Pop();
    }
}

inline void FinalizeShortcut(ShortcutSlot* shortcut, u32 taxonomy)
{
    char* name = shortcut->name;
    if(StrEqual(name, "creatures"))
    {
        taxTable_->creatureTaxonomy = taxonomy;
    }
    else if(StrEqual(name, "fluids"))
    {
        taxTable_->fluidTaxonomy = taxonomy;
    }
    else if(StrEqual(name, "objects"))
    {
        taxTable_->objectTaxonomy = taxonomy;
    }
    else if(StrEqual(name, "equipment"))
    {
        taxTable_->equipmentTaxonomy = taxonomy;
    }
    else if(StrEqual(name, "plants"))
    {
        taxTable_->plantTaxonomy = taxonomy;
    }
    else if(StrEqual(name, "rocks"))
    {
        taxTable_->rockTaxonomy = taxonomy;
    }
    else if(StrEqual(name, "essences"))
    {
        taxTable_->essenceTaxonomy = taxonomy;
    }
    else if(StrEqual(name, "recipe"))
    {
        taxTable_->recipeTaxonomy = taxonomy;
    }
    else if(StrEqual(name, "effects"))
    {
        taxTable_->effectTaxonomy = taxonomy;
    }
    else if(StrEqual(name, "behaviors"))
    {
        taxTable_->behaviorTaxonomy = taxonomy;
    }
}


internal u32 WriteTaxonomies(char* giantBuffer, u32 giantBufferLength, b32 writeToFile, TaxonomySlot* parent, TaxonomySlot* slot, ShortcutSlot* shortcut)
{
    u32 written = 0;
    if(giantBuffer && writeToFile)
    {
        FormatString(giantBuffer, giantBufferLength, "\"%s\"", shortcut->name);
        written = (StrLen(shortcut->name) + 2);
        giantBuffer += written;
        giantBufferLength -= written;
    }
    
    FinalizeShortcut(shortcut, slot->taxonomy);
    slot->subTaxonomiesCount = shortcut->subTaxonomiesCount;
    slot->necessaryBits = TruncateReal32ToU32(log2f((r32) slot->subTaxonomiesCount) + 1);
    slot->usedBitsTotal = slot->necessaryBits;
    if(parent)
    {
        slot->parentNecessaryBits = parent->necessaryBits;
        slot->usedBitsTotal += parent->usedBitsTotal;
    }
    else
    {
        slot->parentNecessaryBits = 0;
        taxTable_->rootBits = slot->necessaryBits;
    }
    
    
    Assert(slot->usedBitsTotal <= 32);
    StrCpy(shortcut->name, StrLen(shortcut->name), slot->name, ArrayCount(slot->name));
    slot->stringHashID = StringHash(slot->name, ArrayCount(slot->name));
    for(u32 subIndex = 0; subIndex < shortcut->subTaxonomiesCount; ++subIndex)
    {
        u32 taxonomy = slot->taxonomy + ((subIndex + 1) << (32 - slot->usedBitsTotal));
        TaxonomySlot* newSlot = GetSlotForTaxonomy(taxTable_, taxonomy, taxPool_);
        newSlot->taxonomy = taxonomy;
        
        ShortcutSlot* newShortcut = GetShortcut(taxTable_, shortcut->subTaxonomies[subIndex]);
        newShortcut->taxonomy = taxonomy;
        u32 writtenChild = WriteTaxonomies(giantBuffer, giantBufferLength, true, slot, newSlot, newShortcut);
        if(giantBuffer)
        {
            written += writtenChild;
            giantBuffer += writtenChild;
            giantBufferLength -= writtenChild;
        }
    }
    
    if(giantBuffer && writeToFile)
    {
        FormatString(giantBuffer, giantBufferLength, "#");
        ++written;
    }
    
    return written;
}


internal void WriteDataFiles()
{
    shortcutStack[stackShortcutCount++] = SaveShortcut(taxTable_, "root", 0, taxPool_);
    char* rootPath = "definition/root";
    
    PlatformSubdirNames subdir;
    subdir.subDirectoryCount = 0;
    
    platformAPI.GetAllSubdirectoriesName(&subdir, rootPath);
    for(u32 subdirIndex = 0; subdirIndex < subdir.subDirectoryCount; ++subdirIndex)
    {
        WriteDataFilesRecursive(rootPath, subdir.subdirs[subdirIndex]);
    }
    
    
    ShortcutSlot* rootShortcut = GetShortcut(taxTable_, "root");
    TaxonomySlot* rootSlot = &taxTable_->root;
    rootSlot->taxonomy = 0;
    
    MemoryPool tempPool = {};
    TempMemory bufferMemory = BeginTemporaryMemory(&tempPool);
    
    u32 giantBufferSize = MegaBytes(2);
    char* giantBuffer = PushArray(&tempPool, char, giantBufferSize);
    u32 written = WriteTaxonomies(giantBuffer, giantBufferSize, false, 0, rootSlot, rootShortcut);
    platformAPI.DEBUGWriteFile("assets/taxonomies.fed", giantBuffer, written);
    
    EndTemporaryMemory(bufferMemory);
}







//
//
//
//NOTE: this only deals with the taxonomy.
internal void ReadTaxonomiesFromFile()
{
    shortcutStack[stackShortcutCount++] = SaveShortcut(taxTable_, "root", 0, taxPool_);
    PlatformFile taxonomiesFile = platformAPI.DEBUGReadFile("assets/taxonomies.fed");
    char* content = (char*) taxonomiesFile.content;
    Tokenizer tokenizer = {};
    tokenizer.at = content;
    
    b32 reading = true;
    while(reading)
    {
        Token t = GetToken(&tokenizer);
        switch(t.type)
        {
            case Token_String:
            {
                Token string = Stringize(t);
                Push_(string.text, string.textLength);
            } break;
            
            case Token_Pound:
            {
                Pop();
            } break;
            
            case Token_EndOfFile:
            {
                reading = false;
            } break;
        }
    }
    
    ShortcutSlot* rootShortcut = GetShortcut(taxTable_, "root");
    TaxonomySlot* rootSlot = &taxTable_->root;
    rootSlot->taxonomy = 0;
    WriteTaxonomies(0, 0, 0, 0, rootSlot, rootShortcut);
}




























#define GetValuePreprocessor(table, test) GetValuePreprocessor_(MetaTable_##table, ArrayCount(MetaTable_##table), test) 
inline u32 GetValuePreprocessor_(char** values, u32 count, char* test)
{
    b32 found = false;
    
    u32 result = 0;
    
    for(u32 valueIndex = 0; valueIndex < count; ++valueIndex)
    {
        if(StrEqual(test, values[valueIndex]))
        {
            result = valueIndex;
            found = true;
            break;
        }
    }
    
    
    Assert(found);
    
    return result;
}


#define GetFlagPreprocessor(flagName, test) GetFlagPreprocessor_(MetaFlags_##flagName, ArrayCount(MetaFlags_##flagName), test)
inline u32 GetFlagPreprocessor_(MetaFlag* values, u32 count, char* test)
{
    b32 found = false;
    
	u32 result = 0;
    
    for(u32 valueIndex = 0; valueIndex < count; ++valueIndex)
    {
		MetaFlag* flag = values + valueIndex;
        if(StrEqual(test, flag->name))
        {
            result = flag->value;
            found = true;
            break;
        }
    }
    
    Assert(found);
    return result;
}


inline void Begin(char* name)
{
    currentSlot_ = NORUNTIMEGetTaxonomySlotByName(taxTable_, name);
}


inline u8 ToU8(char* string)
{
    u8 result = SafeTruncateToU8(atoi(string));
    return result;
}

inline u32 ToU32(char* string)
{
    i32 intValue = atoi(string);
    
    Assert(intValue >= 0);
    u32 result = (u32) intValue;
    return result;
}

inline b32 ToB32(char* string)
{
    b32 result = (StrEqual(string, "true"));
    return result;
}

inline r32 ToR32(char* string)
{
    r32 result = (r32) atof(string);
    
    return result;
}














#define TAXTABLE_ALLOC(ptr, type) FREELIST_ALLOC(ptr, taxTable_->firstFree##type, PushStruct(taxPool_, type, NoClear())) ZeroStruct(*(ptr));
EquipmentMapping* currentEquipmentMap_;

inline void CanEquip(char* name, char* leftSlot, char* rightSlot)
{
    TaxonomySlot* slot = currentSlot_;
    TaxonomySlot* target = NORUNTIMEGetTaxonomySlotByName(taxTable_, name);
    EquipmentMapping* mapping;
    TAXTABLE_ALLOC(mapping, EquipmentMapping);
    
    mapping->taxonomy = target->taxonomy;
    mapping->mapping.multiPart = false;
    mapping->mapping.left = (SlotName) GetValuePreprocessor(SlotName, leftSlot);
    mapping->mapping.right = (SlotName) GetValuePreprocessor(SlotName, rightSlot);
    
    mapping->next = slot->firstEquipmentMapping;
    slot->firstEquipmentMapping = mapping;
}

inline void CanEquipMultipart(char* name)
{
    TaxonomySlot* slot = currentSlot_;
    TaxonomySlot* target = NORUNTIMEGetTaxonomySlotByName(taxTable_, name);
    EquipmentMapping* mapping;
    TAXTABLE_ALLOC(mapping, EquipmentMapping);
    
    mapping->taxonomy = target->taxonomy;
    mapping->mapping.multiPart = true;
    currentEquipmentMap_ = mapping;
    
    mapping->next = slot->firstEquipmentMapping;
    slot->firstEquipmentMapping = mapping;
}

inline void AddPart(char* slot)
{
    
    EquipmentMapping* currentMapping = currentEquipmentMap_;
    Assert(currentMapping->mapping.slotCount < ArrayCount(currentMapping->mapping.slots));
    currentMapping->mapping.slots[currentMapping->mapping.slotCount++] = (SlotName) GetValuePreprocessor(SlotName, slot);
}

inline void CanConsume(char* action, char* name)
{
    TaxonomySlot* slot = currentSlot_;
    TaxonomySlot* target = NORUNTIMEGetTaxonomySlotByName(taxTable_, name);
    
    ConsumeMapping* mapping;
    TAXTABLE_ALLOC(mapping, ConsumeMapping);
    
    
    mapping->action = (EntityAction) GetValuePreprocessor(EntityAction, action);
    mapping->taxonomy = target->taxonomy;
    
    mapping->next = slot->firstConsumeMapping;
    slot->firstConsumeMapping = mapping;
}






















inline void DefinePlantBounds(char* maxRootSegmentNumber_, char* maxRootRadious_,
                              char* maxRootLength_)
{
    u8 maxRootSegmentNumber = ToU8(maxRootSegmentNumber_);
    r32 maxRootRadious = ToR32(maxRootRadious_);
    r32 maxRootLength = ToR32(maxRootLength_);
    
    TaxonomySlot* slot = currentSlot_;
    slot->plantBaseParams.maxZeroLevelSegmentNumber = maxRootSegmentNumber;
    slot->plantBaseParams.maxRootSegmentRadious = maxRootRadious;
    slot->plantBaseParams.maxRootSegmentLength = maxRootLength;
    
    Vec3 min = V3(-maxRootRadious, -maxRootRadious, 0);
    Vec3 max = V3(maxRootRadious, maxRootRadious, maxRootSegmentNumber * maxRootLength);
    
    slot->boundType = ForgBound_Standard;
    slot->physicalBounds = RectMinMax(min, max);
}

inline void DefineBounds_(char* boundsHeight_, char* boundsRadious_, ForgBoundType type)
{
    TaxonomySlot* slot = currentSlot_;
    
    r32 boundsRadious = ToR32(boundsRadious_);
    r32 boundsHeight = ToR32(boundsHeight_);
    
    Vec3 min = V3(-boundsRadious, -boundsRadious, 0);
    Vec3 max = V3(boundsRadious, boundsRadious, boundsHeight);
    
    slot->boundType = type;
    slot->physicalBounds = RectMinMax(min, max);
}

inline void DefineBounds(char* boundsHeight, char* boundsRadious)
{
    DefineBounds_(boundsHeight, boundsRadious, ForgBound_Standard);
}

inline void DefineNullBounds(char* boundsHeight, char* boundsRadious)
{
    DefineBounds_(boundsHeight, boundsRadious, ForgBound_None);
}






























inline TaxonomyComponent* AddComponent_(u64 stringHashID)
{
    TaxonomySlot* slot = currentSlot_;
    TaxonomyComponent* result;
    TAXTABLE_ALLOC(result, TaxonomyComponent);
    result->stringHashID = stringHashID;
    result->ingredientCount = 0;
    
    result->next = slot->firstComponent;
    slot->firstComponent = result;
    return result;
}

inline void AddComponent(char* name)
{
    u64 stringHashID = StringHash(name);
    currentActiveComponent_ = AddComponent_(stringHashID);
}



inline void AddIngredient(char* name)
{
    TaxonomyComponent* component = currentActiveComponent_;
    Assert(component->ingredientCount < ArrayCount(component->ingredientTaxonomies));
    
    TaxonomySlot* ingredientSlot = NORUNTIMEGetTaxonomySlotByName(taxTable_, name);
    component->ingredientTaxonomies[component->ingredientCount++] = ingredientSlot->taxonomy;
}

inline void MadeOf(char* name)
{
    if(!currentActiveComponent_)
    {
        currentActiveComponent_ = AddComponent_(currentSlot_->stringHashID);
    }
    AddIngredient(name);
}















#if FORG_SERVER
TaxonomyEffect* currentEffect_;

inline void AddEffect(char* action, char* ID)
{
    TaxonomyEffect* newEffect;
    TAXTABLE_ALLOC(newEffect, TaxonomyEffect);
    
    newEffect->effect.triggerAction = (EntityAction) GetValuePreprocessor(EntityAction, action);
    newEffect->effect.ID = (EffectIdentifier) GetValuePreprocessor(EffectIdentifier, ID);
    
    FREELIST_INSERT(newEffect, currentSlot_->firstEffect);
    currentEffect_ = newEffect;
    
}

inline void IsPassive()
{
    currentSlot_->isPassiveSkill = true;
}

inline void Power(char* power)
{
	currentEffect_->effect.data = {};
    currentEffect_->effect.data.power = ToR32(power);    
}

inline void Spawn(char* name)
{
    TaxonomySlot* spawnSlot = NORUNTIMEGetTaxonomySlotByName(taxTable_, name);
	currentEffect_->effect.data = {};
    currentEffect_->effect.data.taxonomy = spawnSlot->taxonomy;
}

inline void AddFlag(char* flagName)
{
	u32 flag = GetFlagPreprocessor(EffectFlags, flagName);
    currentEffect_->effect.flags |= flag;
}

inline void Timer(char* timer)
{
    currentEffect_->effect.targetTimer = ToR32(timer);
}

inline void AddFreeHandReq(char* slot, char* taxonomy)
{
    NakedHandReq* req;
    TAXTABLE_ALLOC(req, NakedHandReq);
    TaxonomySlot* taxonomyslot = NORUNTIMEGetTaxonomySlotByName(taxTable_, taxonomy);
    req->slotIndex = SafeTruncateToU8(GetValuePreprocessor(SlotName, slot));
    req->taxonomy = taxonomyslot->taxonomy;
    
    req->next = currentSlot_->nakedHandReq;
    currentSlot_->nakedHandReq = req;
}


internal void CustomEffects()
{
#if 0    
    BeginCustomEffect("combat special damage");
    AddEffect("combat_standardDamage");
    AddEffect("combat_fireDamage");
    
    DefineUtilities("combat_standardDamage");
    AddUtility("offensivePower", EffectUtility_Standard);
    
    DefineUtilities("combat_fireDamage");
    AddUtility("offensivePower", EffectUtility_Standard);
    AddUtility("fireDamage", EffectUtility_Huge);
#endif
}










CraftingEffectLink* activeCraftingLink_;
inline void LinkStandard(char* action, char* effectName, char* target)
{
    TaxonomySlot* effectSlot = NORUNTIMEGetTaxonomySlotByName(taxTable_, effectName);
    CraftingEffectLink* link;
    TAXTABLE_ALLOC(link, CraftingEffectLink);
    link->triggerAction = (EntityAction) GetValuePreprocessor(EntityAction, action);
    link->target = target ? ToB32(target) : false;
    link->ID = effectSlot->effectID;
    activeCraftingLink_ = link;
    
    FREELIST_INSERT(link, currentSlot_->links);
}

inline void Requires_(char* essenceName)
{
    TaxonomySlot* essenceSlot = NORUNTIMEGetTaxonomySlotByName(taxTable_, essenceName);
    for(u32 essenceIndex = 0; essenceIndex < MAX_ESSENCES_PER_EFFECT; ++essenceIndex)
    {
        if(!activeCraftingLink_->essences[essenceIndex].taxonomy)
        {
            activeCraftingLink_->essences[essenceIndex].taxonomy = essenceSlot->taxonomy;
            return;
        }
    }
}

inline void Requires(char* essenceName, u32 quantity)
{
    for(u32 quantityIndex = 0; quantityIndex < quantity; ++quantityIndex)
    {
        Requires_(essenceName);
    }
}

inline void ReadCraftingEffects()
{
#if 0    
    BeginPossibleEffects("armour");
    Link({Property_Strength, Property_Fire}, "fireProtection");
    Link(Property_Fire, "defensiveFireAbility");
#endif
    
    
    
    
}




























inline void PlantStatus(char* s, r32 duration, char* d)
{
    Assert(IsPlant(taxTable_, currentSlot_->taxonomy));
    
    u32 source = GetValuePreprocessor(PlantLifeStatus, s);
    u32 dest = GetValuePreprocessor(PlantLifeStatus, d);
    currentSlot_->plantStatusDuration[source] = duration;
    currentSlot_->nextStatus[source] = (PlantLifeStatus) dest;
}


inline void SaveCreatureAttribute(char* attributeName, r32 value)
{
    MemberDefinition member = SLOWGetRuntimeOffsetOf(CreatureComponent, attributeName);
    u32 offset = member.offset;
    
    AttributeSlot* attr = GetAttributeSlot(currentSlot_, offset);
    attr->offsetFromBase = offset;
    attr->valueR32 = value;
}

inline void InventorySpace(u8 width, u8 height)
{
    TaxonomySlot* slot = currentSlot_;
    
    slot->gridDimX = width;
    slot->gridDimY = height;
}
#endif


inline void AddEssence(char* name, u32 quantity)
{
    TaxonomySlot* slot = NORUNTIMEGetTaxonomySlotByName(taxTable_, name);
    Assert(IsEssence(taxTable_, slot->taxonomy));
    
    
    b32 found = false;
    for(TaxonomyEssence* essence = currentSlot_->essences; essence; essence = essence->next)
    {
        if(essence->essence.taxonomy == slot->taxonomy)
        {
            essence->essence.quantity += quantity;
            essence->essence.quantity = SafeTruncateToU8(essence->essence.quantity);
            found = true;
        }
    }
    
    if(!found)
    {
        TaxonomyEssence* essence;
        TAXTABLE_ALLOC(essence, TaxonomyEssence);
        essence->essence.taxonomy = slot->taxonomy;
        essence->essence.quantity = quantity;
        FREELIST_INSERT(essence, currentSlot_->essences);
    }
}













inline void BeginPlayerAction(char* action)
{
    PlayerPossibleAction* possibleAction;
    TAXTABLE_ALLOC(possibleAction, PlayerPossibleAction);
    
    u8 actionInt = SafeTruncateToU8(GetValuePreprocessor(EntityAction, action));
    possibleAction->action = (EntityAction) actionInt;
    possibleAction->flags = 0;
    
    possibleAction->next = currentSlot_->firstPossibleAction;
    currentSlot_->firstPossibleAction = possibleAction;
}

inline void AddActionFlag(char* flagName)
{
    PlayerPossibleAction* possibleAction = currentSlot_->firstPossibleAction;
    u32 flag = GetFlagPreprocessor(CanDoActionFlags, flagName);
    possibleAction->flags |= flag;
}

inline TaxonomyNode* AddToTaxonomyTree(TaxonomyTree* tree, TaxonomySlot* slot)
{
    if(!tree->root)
    {
        TaxonomyNode* newNode;
        TAXTABLE_ALLOC(newNode, TaxonomyNode);
        newNode->key = 0;
        tree->root = newNode;
    }
    TaxonomyNode* root = tree->root;
    TaxonomyNode* result = root;
    
    u32 currentTaxonomy = 0;
    TaxonomyNode* current = root;
    
    u32 taxonomy = slot->taxonomy;
    while(currentTaxonomy != taxonomy)
    {
        TaxonomySlot* currentSlot = GetSlotForTaxonomy(taxTable_, currentTaxonomy);
        u32 childTaxonomy = GetChildTaxonomy(currentSlot, taxonomy);
        
        
        if(!current->firstChild)
        {
            TaxonomyNode* newNode;
            TAXTABLE_ALLOC(newNode, TaxonomyNode);
            newNode->key = childTaxonomy;
            current->firstChild = newNode;
        }
        TaxonomyNode* firstChild = current->firstChild;
        
        while(true)
        {
            if(firstChild->key == childTaxonomy)
            {
                if(childTaxonomy == taxonomy)
                {
                    result = firstChild;
                    break;
                }
                else
                {
                    current = firstChild;
                    break;
                }
            }
            else if(!firstChild->next)
            {
                TaxonomyNode* newNode;
                TAXTABLE_ALLOC(newNode, TaxonomyNode);
                firstChild->next = newNode;
                
                newNode->key = childTaxonomy;
                if(newNode->key == taxonomy)
                {
                    result = newNode;
                }
                else
                {
                    current = newNode;
                }
                break;
            }
            
            firstChild = firstChild->next;
        }
        
        currentTaxonomy = childTaxonomy;
    }
    
    return result;
}

inline void AddTarget(char* name)
{
    TaxonomySlot* target = NORUNTIMEGetTaxonomySlotByName(taxTable_, name);
    PlayerPossibleAction* possibleAction = currentSlot_->firstPossibleAction;
    TaxonomyNode* node = AddToTaxonomyTree(&possibleAction->tree, target);
    node->data.possible = true;
}














#ifndef FORG_SERVER

inline void AddActionEffect(AnimationEffect effect, u32 action, char* pieceName = 0)
{
    AnimationEffect* dest;
    TAXTABLE_ALLOC(dest, AnimationEffect);
    if(pieceName)
    {
        effect.stringHashID = StringHash(pieceName);
    }
    else
    {
        effect.stringHashID = 0;
    }
    
    effect.triggerAction = action;
    
    *dest = effect;
    dest->timer = 0;
    
    FREELIST_INSERT(dest, currentSlot_->firstAnimationEffect);
}


inline void AddTriggerEffect(AnimationEffect effect, char* effectName)
{
    AnimationEffect* dest;
    TAXTABLE_ALLOC(dest, AnimationEffect);
    
    TaxonomySlot* slot = NORUNTIMEGetTaxonomySlotByName(taxTable_, effectName);
    effect.triggerEffectTaxonomy = slot->taxonomy;
    
    *dest = effect;
    dest->timer = 0;
    
    FREELIST_INSERT(dest, currentSlot_->firstAnimationEffect);
}

inline void AddPerpetualEffect(AnimationEffect effect, char* pieceName = 0)
{
    AddActionEffect(effect, Action_None, pieceName);
}

inline AnimationEffect StandardColorEffect(Vec4 color, r32 fadeTime)
{
    AnimationEffect result = {};
    result.type = AnimationEffect_ChangeColor;
    result.color = color;
    result.fadeTime = fadeTime;
    return result;
}

inline AnimationEffect SpawnParticlesEffect(FluidSpawnType type)
{
    AnimationEffect result = {};
    result.type = AnimationEffect_SpawnParticles;
    result.particleType = type;
    
    return result;
}

inline AnimationEffect SpawnAshToDestEffect(Vec4 color, r32 lastingTime, r32 timeToArriveAtDest, r32 dim)
{
    AnimationEffect result = {};
    result.flags = AnimationEffect_AllActions;
    result.type = AnimationEffect_SpawnAshesTowardEntity;
    result.targetTimer = lastingTime;
    result.color = color;
    result.timeToArriveAtDest = timeToArriveAtDest;
    result.dim = dim;
    
    return result;
}

inline void AddLight(r32 intensity, Vec3 color)
{
    currentSlot_->lightIntensity = intensity;
    currentSlot_->lightColor = color;
}

inline void UsesSkeleton(char* skeletonName)
{
    currentSlot_->skeletonHashID = StringHash(skeletonName);
}

internal void ReadAnimationData()
{
    
#if 0    
    AddActionEffect(StandardColorEffect(V4(0, 0, 1, 1), 1.0f), Action_Attack, "belly");
    AddTriggerEffect(SpawnAshToDestEffect(V4(1, 0, 0, 1), 2.0f, 1.0f, 0.05f), "generic_spawn");
#endif
}















inline TaxonomySound* AddSoundEffect(char* animationName, char* threesoldIn, char* eventName)
{
    r32 threesold = ToR32(threesoldIn);
    u64 animationHash = StringHash(animationName);
    u64 eventHash = StringHash(eventName);
    
    TaxonomySound* dest;
    TAXTABLE_ALLOC(dest, TaxonomySound);
    
    dest->animationNameHash = animationHash;
    dest->threesold = threesold;
    dest->eventNameHash = eventHash;
    
    FREELIST_INSERT(dest, currentSlot_->firstSound);
    
    return dest;
}

inline SoundContainer* AddSoundEvent(char* eventName)
{
    Assert(taxTable_->eventCount < ArrayCount(taxTable_->events));
    
    SoundEvent* event = taxTable_->events + taxTable_->eventCount++;
    event->eventNameHash = StringHash(eventName);
    SoundContainer* result = &event->rootContainer;
    
    event->rootContainer.soundCount = 0;
    event->rootContainer.containerCount = 0;
    return result;
}

inline void AddSoundToContainer(SoundContainer* container, char* soundType, char* soundName)
{
    ++container->soundCount;
    
    LabeledSound* sound;
    TAXTABLE_ALLOC(sound, LabeledSound);
    
    sound->typeHash = StringHash(soundType);
    sound->nameHash = StringHash(soundName);
    
    FREELIST_INSERT(sound, container->firstSound);
}

inline SoundContainer* AddChildContainer(SoundContainer* container)
{
    ++container->containerCount;
    
    SoundContainer* newContainer;
    TAXTABLE_ALLOC(newContainer, SoundContainer);
    
    newContainer->soundCount = 0;
    newContainer->containerCount = 0;
    
    FREELIST_INSERT(newContainer, container->firstChildContainer);
    
    return newContainer;
}

#if 0
inline void AddLabel(TaxonomySound* sound, char* labelName, char* labelValue)
{
    r32 valueReal = labelValue ? ToR32(labelValue) : 0;
    
    u32 hash = (u32) (StringHash(labelName) >> 32);
    u32 hashIndex = (hash & (LABEL_HASH_COUNT - 1)) + Tag_count;
    
    Assert(sound->labelCount < ArrayCount(sound->labels));
    Label* label = sound->labels + sound->labelCount++;
    label->hashID = hashIndex;
    label->value = valueReal;
}
#endif



inline SoundEvent* GetSoundEvent(TaxonomyTable* table, u64 eventHash)
{
    SoundEvent* result = 0;
    for(u32 eventIndex = 0; eventIndex < table->eventCount; ++eventIndex)
    {
        SoundEvent* event = table->events + eventIndex;
        if(event->eventNameHash == eventHash)
        {
            result = event;
            break;
        }
    }
    
    return result;
}


































PlantParams* currentPlantParams_;
PlantRule* currentPlantRule_;
NewSegment* currentNewSegment_;

inline void BeginParams(char* name)
{
    TaxonomySlot* slot = NORUNTIMEGetTaxonomySlotByName( taxTable_, name );
    currentSlot_ = slot;
    TAXTABLE_ALLOC(slot->plantParams, PlantParams);
    currentPlantParams_ = slot->plantParams;
    currentPlantParams_->plantCount = 1;
    currentPlantParams_->leafSize[0] = 1.0f;
}

inline void RenderingGeneralParams(u8 plantCount, r32 minOffset, r32 maxOffset)
{
    currentPlantParams_->plantCount = plantCount;
    currentPlantParams_->minOffset = minOffset;
    currentPlantParams_->maxOffset = maxOffset;
}

inline void GeneralParams(u8 maxLevels, r32 rootStrength, Vec4 branchColorAlive, Vec4 branchColorDead)
{
    Assert(Normalized( rootStrength));
    Assert(maxLevels <= MAX_LEVELS);
    
    u8 maxZeroLevelSegmentNumber = currentSlot_->plantBaseParams.maxZeroLevelSegmentNumber;
    r32 maxRootSegmentLength = currentSlot_->plantBaseParams.maxRootSegmentLength;
    r32 maxRootSegmentRadious = currentSlot_->plantBaseParams.maxRootSegmentRadious;
    
    currentPlantParams_->maxSegmentNumber[0] = maxZeroLevelSegmentNumber;
    currentPlantParams_->maxRadious[0] = maxRootSegmentRadious;
    currentPlantParams_->segmentLengths[0] = maxRootSegmentLength;
    
    currentPlantParams_->maxLevels = maxLevels;
    currentPlantParams_->plantStrength[0] = rootStrength;
    
    currentPlantParams_->oneOverMaxRadious = 1.0f / maxRootSegmentLength;
    
    currentPlantParams_->branchColorAlive = branchColorAlive;
    currentPlantParams_->branchColorDead = branchColorDead;
}

inline void DeltaCoeffBetweenLevels( r32 maxRadiousCoeff, r32 maxLengthCoeff, r32 maxSegmentCoeff, r32 leafSizeCoeff, r32 strengthCoeff )
{
    Assert( currentPlantParams_->maxLevels && currentPlantParams_->maxLevels <= MAX_LEVELS );
    
    u8* maxSegm = currentPlantParams_->maxSegmentNumber;
    r32* maxR = currentPlantParams_->maxRadious;
    r32* maxL = currentPlantParams_->segmentLengths;
    r32* maxLS = currentPlantParams_->leafSize;
    r32* maxS = currentPlantParams_->plantStrength;
    
    Assert( maxSegm[0] );
    Assert( maxR[0] != 0.0f );
    Assert( maxL[0] != 0.0f );
    Assert( maxLS[0] != 0.0f );
    Assert( maxS[0] != 0.0f );
    
    for( u8 levelIndex = 1; levelIndex < currentPlantParams_->maxLevels; ++levelIndex )
    {
        maxSegm[levelIndex] = ( u8 ) ( ( r32 ) maxSegm[levelIndex - 1] * maxSegmentCoeff );
        maxR[levelIndex] = maxR[levelIndex - 1] * maxRadiousCoeff;
        maxL[levelIndex] = maxL[levelIndex - 1] * maxLengthCoeff;
        maxLS[levelIndex] = maxLS[levelIndex - 1] * leafSizeCoeff;
        maxS[levelIndex] = maxS[levelIndex - 1] * strengthCoeff;
    }
}

inline void DeltaCoeffBetweenLevelSegment( r32 radiousCoeff, r32 lengthCoeff, r32 branchLengthCoeff, r32 strengthCoeff )
{
    for( u32 levelIndex = 0; levelIndex < ArrayCount( currentPlantParams_->segmentBySegmentRadiousCoeff ); ++levelIndex )
    {
        currentPlantParams_->segmentBySegmentRadiousCoeff[levelIndex] = radiousCoeff;
    }
    
    for( u32 levelIndex = 0; levelIndex < ArrayCount( currentPlantParams_->segmentBySegmentLengthCoeff ); ++levelIndex )
    {
        currentPlantParams_->segmentBySegmentLengthCoeff[levelIndex] = lengthCoeff;
    }
    
    for( u32 levelIndex = 0; levelIndex < ArrayCount( currentPlantParams_->segmentBySegmentBranchLengthCoeff ); ++levelIndex )
    {
        currentPlantParams_->segmentBySegmentBranchLengthCoeff[levelIndex] = branchLengthCoeff;
    }
    
    for( u32 levelIndex = 0; levelIndex < ArrayCount( currentPlantParams_->segmentBySegmentStrengthCoeff ); ++levelIndex )
    {
        currentPlantParams_->segmentBySegmentStrengthCoeff[levelIndex] = strengthCoeff;
    }
}

#define AddRule( type, ... ) AddRule_( PlantSegment_##type, __VA_ARGS__ )
inline void AddRule_( PlantSegmentType type, b32 allowsleafGrowth = true )
{
    Assert( currentPlantParams_->ruleCount < ArrayCount( currentPlantParams_->rules ) );
    currentPlantRule_ = currentPlantParams_->rules + currentPlantParams_->ruleCount++;
    currentPlantRule_->requestedType = type;
    currentPlantRule_->newType = type;
}

#define Becomes( type, leafCount ) Becomes_( PlantSegment_##type, leafCount )
inline void Becomes_( PlantSegmentType type, u8 leafCount )
{
    currentPlantRule_->newType = type;
    currentPlantRule_->leafCountDelta = leafCount;
}

#define GenNewSegment( pos, type ) GenNewSegment_( PlantChild_##pos, PlantSegment_##type )
inline void GenNewSegment_( PlantChildPosition position, PlantSegmentType type )
{
    currentNewSegment_ = currentPlantRule_->newSegments + position;
    currentNewSegment_->type = type;
    
    u8 levelDelta = 1;
    if( position == PlantChild_Top )
    {
        levelDelta = 0;
    }
    currentNewSegment_->levelDelta = levelDelta;
}

inline void NewSegmentAngleX( r32 angleMin, r32 angleMax, i8 magnitude, i8 repeat, i8 offset = 0 )
{
    Assert( angleMin <= angleMax );
    currentNewSegment_->angleXDistr = RangeDistr( DegToRad( angleMin ), DegToRad( angleMax ), magnitude, repeat, offset );
}

inline void NewSegmentAngleY( r32 angleMin, r32 angleMax, i8 magnitude, i8 repeat, i8 offset = 0 )
{
    Assert( angleMin <= angleMax );
    currentNewSegment_->angleYDistr = RangeDistr( DegToRad( angleMin ), DegToRad( angleMax ), magnitude, repeat, offset );
}

inline void MaxCenterDelta(r32 maxDelta)
{
    currentPlantParams_->maxCenterDelta = maxDelta;
}
inline void GrowthParams( r32 radious )
{
    currentPlantRule_->radiousIncrement = radious;
}

inline void SpecialRequirement( u8 level, u8 minSegmentIndex )
{
    Assert( currentPlantRule_->specialRequirementCount < ArrayCount( currentPlantRule_->specialRequirement ) );
    SpecialRuleRequirement* req = currentPlantRule_->specialRequirement + currentPlantRule_->specialRequirementCount++;
    req->level = level;
    req->minSegmentIndex = minSegmentIndex;
}

inline void LeafParams(r32 leafRadious, r32 leafLength, r32 leafMinAngle, r32 leafMaxAngle, Vec4 leafColorAlive, Vec4 leafColorDead, Vec3 leafColorRandomization)
{
    currentPlantParams_->leafRadious = leafRadious;
    currentPlantParams_->leafLength = leafLength;
    currentPlantParams_->leafMinAngle = DegToRad( leafMinAngle );
    currentPlantParams_->leafMaxAngle = DegToRad( leafMaxAngle );
    currentPlantParams_->leafColorAlive = leafColorAlive;
    currentPlantParams_->leafColorDead = leafColorDead;
    currentPlantParams_->leafColorRandomization = leafColorRandomization;
}

inline void IsHerbaceous()
{
    currentPlantParams_->isHerbaceous = true;
}


internal void ReadPlantChart()
{
    BeginParams( "pine" );
    
    LeafParams( 0.14f, 0.13f, -30.0f, 30.0f, SRGBLinearize(V4( 0.06f, 0.1f, 0.02f, 1.0f)), SRGBLinearize(V4(0.35f, 0.12f, 0.14f, 1.0f)), V3(0.002f, 0.03f, 0.001f));
    
    GeneralParams( 2, 0.9f, SRGBLinearize(0.15f,0.08f,0.08f, 1.0f), SRGBLinearize( V4( 0.2f, 0.08f, 0.05f, 1.0f ) ) );
    MaxCenterDelta(0.1f);
    DeltaCoeffBetweenLevels( 0.6f, 0.6f, 0.5f, 0.8f, 0.9f );
    DeltaCoeffBetweenLevelSegment( 0.84f, 0.95f, 0.9f, 0.9f );
    
    AddRule( Meristem );
    Becomes(Branch, 0);
    
    GenNewSegment( Top, Meristem );
    NewSegmentAngleX( -10.0f, 10.0f, 1, 10 );
    NewSegmentAngleY( -1.0f, 1.0f, 3, 6 );
    
    
    AddRule( Meristem );
    Becomes(Branch, 4);
    
    GenNewSegment( Top, Meristem );
    NewSegmentAngleX( -10.0f, 10.0f, 1, 10 );
    NewSegmentAngleY( -1.0f, 1.0f, 3, 6 );
    
    GenNewSegment( Up, Meristem );
    NewSegmentAngleX( -30.0f, 30.0f, 3, 6 );
    NewSegmentAngleY( -30.0f, 30.0f, 3, 6 );
    
    GenNewSegment( Down, Meristem );
    NewSegmentAngleX( -30.0f, 30.0f, 3, 6 );
    NewSegmentAngleY( -30.0f, 30.0f, 3, 6 );
    
    
    GenNewSegment( Right, Meristem );
    NewSegmentAngleX( -30.0f, 30.0f, 3, 6 );
    NewSegmentAngleY( -30.0f, 30.0f, 3, 6 );
    
    GenNewSegment( Left, Meristem );
    NewSegmentAngleX( -30.0f, 30.0f, 3, 6 );
    NewSegmentAngleY( -30.0f, 30.0f, 3, 6 );
    
    SpecialRequirement( 0, 2 );
    
    AddRule(Branch);
    GrowthParams(0.03f);
    
    BeginParams( "testGrass" );
    IsHerbaceous();
    RenderingGeneralParams(5, -0.4f, 0.4f);
    LeafParams(0.2f, 0.3f, -60.0f, 60.0f, SRGBLinearize(V4(0.05f,0.14f, 0.05f, 1.0f)), V4( 0.06f, 0.13f, 0.02f, 1.0f ), V3(0.002f, 0.03f, 0.001f));
}


























































inline void AddTag(TagId ID, r32 value)
{
    TaxonomySlot* slot = currentSlot_;
    VisualTag* dest;
    TAXTABLE_ALLOC(dest, VisualTag);
    dest->ID = ID;
    dest->value = value;
    
    FREELIST_INSERT(dest, slot->firstVisualTag);
}
#endif



#ifdef FORG_SERVER

AIBehavior* currentBehavior_;
AIAction* currentAction_;
Consideration* currentConsideration_;


inline void BeginBehavior(char* name)
{
    TaxonomySlot* slot = NORUNTIMEGetTaxonomySlotByName(taxTable_, name);
    Assert(IsBehavior(taxTable_, slot->taxonomy));
    TAXTABLE_ALLOC(slot->behaviorContent, AIBehavior);
    currentBehavior_ = slot->behaviorContent;
}

inline void AddAction(EntityAction todo, char* targetCriteria = 0, r32 importance = 1.0f)
{
    
    Assert(currentBehavior_->actionCount < ArrayCount(currentBehavior_->actions));
    AIAction* action = currentBehavior_->actions + currentBehavior_->actionCount++;
    action->type = AIAction_Command;
    action->command.action = todo;
    
    if(targetCriteria)
    {
        TaxonomySlot* targetSlot = NORUNTIMEGetTaxonomySlotByName(taxTable_, targetCriteria);
        action->associatedConcept = targetSlot->taxonomy;
    }
    action->importance = importance;
    currentAction_ = action;
}

inline void AddAction(char* behaviorName, r32 importance = 1.0f)
{
    Assert(currentBehavior_->actionCount < ArrayCount(currentBehavior_->actions));
    AIAction* action = currentBehavior_->actions + currentBehavior_->actionCount++;
    action->type = AIAction_Behavior;
    action->behaviorTaxonomy = NORUNTIMEGetTaxonomySlotByName(taxTable_, behaviorName)->taxonomy;
    
    action->importance = importance;
    currentAction_ = action;
}

inline void AssociateBehavior(char* referenceName, char* specificName, b32 isStartingBlock = false)
{
    TaxonomyBehavior* behavior;
    TAXTABLE_ALLOC(behavior, TaxonomyBehavior);
    TaxonomySlot* referenceSlot = NORUNTIMEGetTaxonomySlotByName(taxTable_, referenceName);
    TaxonomySlot* blockSlot = NORUNTIMEGetTaxonomySlotByName(taxTable_, specificName);
    behavior->referenceTaxonomy = referenceSlot->taxonomy;
    behavior->specificTaxonomy = blockSlot->taxonomy;
    
    FREELIST_INSERT(behavior, currentSlot_->firstPossibleBehavior);
    
    if(isStartingBlock)
    {
        currentSlot_->startingBehavior = behavior;
    }
}

inline void DefineConsideration(char* considerationName, char* expression)
{
    TaxonomySlot* slot = NORUNTIMEGetTaxonomySlotByName(taxTable_, considerationName);
    Assert(!slot->consideration);
    TAXTABLE_ALLOC(slot->consideration, TaxonomyConsideration);
    
    StrCpy(expression, StrLen(expression), slot->consideration->expression, ArrayCount(slot->consideration->expression));
}

inline ResponseCurve Gaussian()
{
    ResponseCurve result = {};
    return result;
}

inline void AddConsideration(char* name, r32 bookEndMin, r32 bookEndMax, ResponseCurve curve)
{
    Assert(currentAction_->considerationCount < ArrayCount(currentAction_->considerations));
    Consideration* dest = currentAction_->considerations + currentAction_->considerationCount++;
    dest->expression = NORUNTIMEGetTaxonomySlotByName(taxTable_, name)->consideration->expression;
    
    dest->bookEndMin = bookEndMin;
    dest->bookEndMax = bookEndMax;
    
    dest->curve = curve;
    dest->params.paramCount = 0;
    
    currentConsideration_ = dest;
}


inline void AddBooleanConsideration(char* name)
{
    AddConsideration(name, 0, 1, Gaussian());
}

#define AddParam(value) AddParam_(ExpressionVal(value))
inline void AddParam_(ExpressionValue value)
{
    ConsiderationParams* params = &currentConsideration_->params;
    AddParam_(params, value);
}


internal void ReadBehaviors()
{
    DefineConsideration("nullConsideration", "0.1");
    DefineConsideration("angryLevel", "0.5");
    DefineConsideration("closeness", "Div(Square(params[0]), LengthSq(Diff(targetGen(P), selfGen(P))))");
    
    DefineConsideration("hisThreat", "Div(target(lifePoints), self(lifePoints))");
    DefineConsideration("myThreat", "Div(self(lifePoints), target(lifePoints))");
    
    DefineConsideration("theirTotalThreat", "foreach(target){SetResult(sum(result, hisThreat()))}");
    DefineConsideration("myTotalThreat", "foreach(target){SetResult(sum(result, myThreat()))}");
    
    DefineConsideration("tooDistant", "GtEq(LengthSq(Diff(targetGen(P), selfGen(P))), Square(params[0]))");
    DefineConsideration("notTooDistant", "Not(GtEq(LengthSq(Diff(targetGen(P), selfGen(P))), Square(params[0])))");
    DefineConsideration("tooDistantFromAllTargets", "SetResult(1.0); foreach(target){ if(notTooDistant(params[0])){SetResult(0); endLoop} }");
    
    //DefineConsideration("AttackedRecently", "Available(BrainNode(ID)) && node->timer <= 10.0f)");
    //DefineConsideration("AttackedRecently", "Available(BrainNode(taxonomy) && node->timer <= 10.0f))"); 
    
#if 0    
    BeginInfluenceMap("enemies");
    FallOffFunction(???);
    
    BeginInfluenceMap("allies");
    FallOffFunction(???);
    
    
    BeginInfluenceMap("prey");
    SetScale(Region);
    FallOffFunction(???);
    
    BeginBehavior("help random");
    AddAction("find someone");
    AddAction(Action_Attack);
    
    
#endif
    
#if 0
    BeginBehavior("fleeFromEnemies");
    AddAction(Action_Move);
    AddParam("destination", "ToPosition(Maximize(Min(Enemies(All)), Min(Environment(All), Max(Allies(All))))");
    
    BeginBehavior("enemiesSpacing");
    AddAction(Action_Move);
    AddParam("destination", "ToPosition(Maximize(Min(Enemies(All)), Min(Environment(All), Min(Allies(All))))");
#endif
    
    
    
    
    BeginBehavior("idiot routine");
    AddAction(Action_Attack, "enemy");
    AddConsideration("myThreat", 0.0f, 1.0f, Gaussian());
    
    AddAction(Action_Eat, "foodCrit");
    AddConsideration("angryLevel", 0.0f, 1.0f, Gaussian());
    AddConsideration("closeness", 0.0f, 10.0f, Gaussian());
    AddParam(2.0f);
    
#if 0    
    AddAction(Action_None);
    AddConsideration("nullConsideration", 0.0f, 20.0f, Gaussian());
#endif
    
    
#if 0    
    AddAction(Action_Move);
    AddDestination("Maximize(random)");
    AddConsideration("nullConsideration", 0.0f, 20.0f, Gaussian());
#endif
    
    
#if 0    
    AddAction("fleeFromEnemies");
    AddConsideration("enemiesThreat");
    
    AddAction("enemySpacing");
    AddInfluenceConsideration("EnemiesProximity(self)", Gaussian());
    AddInfluenceConsideration("AlliesProximity(self)", Gaussian());
#endif
}


MemCriteria* currentMemCriteria_;
TaxonomyTree* currentSynthTree_;
TaxonomyNode* currentSynthNode_;
MemSynthOption* currentSynthOption_;

inline void BeginMemoryBehavior(char* behaviorName)
{
    TaxonomySlot* slot = NORUNTIMEGetTaxonomySlotByName(taxTable_, behaviorName);
    Assert(IsBehavior(taxTable_, slot->taxonomy));
    currentSlot_ = slot;
}

inline void AddCriteria(char* criteriaName)
{
    TaxonomySlot* criteriaTax = NORUNTIMEGetTaxonomySlotByName(taxTable_, criteriaName);
    MemCriteria* newCriteria;
    TAXTABLE_ALLOC(newCriteria, MemCriteria);
    newCriteria->taxonomy = criteriaTax->taxonomy;
    
    FREELIST_INSERT(newCriteria, currentSlot_->criteria);
    currentMemCriteria_ = newCriteria;
}

inline void AddMemConsideration(char* requiredConceptName)
{
    TaxonomySlot* slot = NORUNTIMEGetTaxonomySlotByName(taxTable_, requiredConceptName);
    Assert(currentMemCriteria_->possibleTaxonomiesCount < ArrayCount(currentMemCriteria_->requiredConceptTaxonomy));
    currentMemCriteria_->requiredConceptTaxonomy[currentMemCriteria_->possibleTaxonomiesCount++] = slot->taxonomy;
}

inline void AddSynthesisRule(EntityAction action)
{
    MemSynthesisRule* rule;
    TAXTABLE_ALLOC(rule, MemSynthesisRule);
    rule->action = action;
    
    FREELIST_INSERT(rule, currentSlot_->synthRules);
    
    currentSynthTree_ = &rule->tree;
}

inline void AddNode(char* name)
{
    TaxonomySlot* target = NORUNTIMEGetTaxonomySlotByName(taxTable_, name);
    currentSynthNode_ = AddToTaxonomyTree(currentSynthTree_, target);
}

inline void AddOption(char* conceptName, u32 lastingtimeUnits, u32 refreshTimeUnits)
{
    MemSynthOption* option;
    TAXTABLE_ALLOC(option, MemSynthOption);
    option->lastingtimeUnits = SafeTruncateToU16(lastingtimeUnits);
    option->refreshTimeUnits = SafeTruncateToU16(refreshTimeUnits);
    option->outputConcept = NORUNTIMEGetTaxonomySlotByName(taxTable_, conceptName)->taxonomy;
    
    FREELIST_INSERT(option, currentSynthNode_->data.firstOption);
    
    currentSynthOption_ = option;
}

#if 0
inline void OnCondition(char* requiredAssociationConcept)
{
    CriteriaOption* option;
    TAXTABLE_ALLOC(option, CriteriaOption);
    TaxonomySlot* conceptSlot = NORUNTIMEGetTaxonomySlotByName(behaviorsTaxTable_, requiredAssociationConcept);
    option->requiredAssociationConcept = conceptSlot->taxonomy;
    
    FREELIST_INSERT(option, currentCriteriaNode_->data.firstOption);
}
#endif

inline void AddMemBehavior(char* behaviorName)
{
    TaxonomyMemBehavior* newBehavior;
    TAXTABLE_ALLOC(newBehavior, TaxonomyMemBehavior);
    newBehavior->taxonomy = NORUNTIMEGetTaxonomySlotByName(taxTable_, behaviorName)->taxonomy;
    
    FREELIST_INSERT(newBehavior, currentSlot_->firstMemBehavior);
}

#define Seconds(value) value
#define Minutes(value) 60 * Seconds(value)
#define Hours(value) 60 * Minutes(value)

internal void ReadSynthesisRules()
{
    BeginMemoryBehavior("idiot memory behavior");
    AddCriteria("enemy");
    AddMemConsideration("attacker");
    AddMemConsideration("hostile");
    
    AddSynthesisRule(Action_Attack);
    AddNode("monsters");
    AddOption("attacker", Minutes(40), Minutes(2));
    //AddBooleanConsideration();
    
    AddCriteria("foodCrit");
    AddMemConsideration("foodCandidate");
    
    AddSynthesisRule(Action_None);
    AddNode("apple");
    AddOption("foodCandidate", Minutes(10), Minutes(1));
    
    
    
    
    
    
    BeginMemoryBehavior("idiot memory behavior 2");
    AddCriteria("enemy");
    AddMemConsideration("attacker");
    AddMemConsideration("hostile");
    
    AddSynthesisRule(Action_Attack);
    AddNode("root");
    AddOption("attacker", Minutes(40), Minutes(2));
    //AddBooleanConsideration();
    
    AddSynthesisRule(Action_None);
    AddNode("monsters");
    AddOption("hostile", Minutes(20), Minutes(1));
    
    AddCriteria("foodCrit");
    AddMemConsideration("foodCandidate");
    
    AddSynthesisRule(Action_None);
    AddNode("apple");
    AddOption("foodCandidate", Minutes(10), Minutes(1));
}
#endif







inline void Requires(char* toolName)
{
    TaxonomySlot* toolSlot = NORUNTIMEGetTaxonomySlotByName(taxTable_, toolName);
    b32 inserted = false;
    for(u32 toolIndex = 0; toolIndex < ArrayCount(currentSlot_->neededToolTaxonomies); ++toolIndex)
    {
        if(!currentSlot_->neededToolTaxonomies[toolIndex])
        {
            inserted = true;
            currentSlot_->neededToolTaxonomies[toolIndex] = toolSlot->taxonomy;
            break;
        }
    }
    
    Assert(inserted);
}






















internal void InitializeNewTaxonomyTabs(TaxonomySlot* slot)
{
    InvalidCodePath;
#if 0    
	for(u32 tabIndex = 0; tabIndex < Tab_Count; ++tabIndex)
	{
		EditorTab* tab = FindDefaultTab();
		Copy(tab, slot->tabs[tabIndex]);
	}
#endif
    
}

inline char* OutputToBuffer(char* buffer, u32* bufferSize, char* string)
{
    u32 written = (u32) FormatString(buffer, *bufferSize, "%s", string);
    *bufferSize = *bufferSize - written;
    char* result = buffer + written;
    
    return result;
}

inline char* WriteElements(char* buffer, u32* bufferSize, EditorElement* element)
{
	while(element)
	{
		if(element->name[0])
		{
            buffer = OutputToBuffer(buffer, bufferSize, element->name);
			buffer = OutputToBuffer(buffer, bufferSize, " = ");
		}
        
		switch(element->type)
		{
			case EditorElement_String:
			{
                buffer = OutputToBuffer(buffer, bufferSize, " \"");
				buffer = OutputToBuffer(buffer, bufferSize, element->value);
                buffer = OutputToBuffer(buffer, bufferSize, "\" ");
			} break;
            
            case EditorElement_Real:
			{
				b32 encounteredPoint = false;
                
				for(char* test = element->value; *test; ++test)
				{
					if(*test == '.')
					{
						encounteredPoint = true;
					}
				}
				buffer = OutputToBuffer(buffer, bufferSize, element->value);
				if(!encounteredPoint)
				{
					buffer = OutputToBuffer(buffer, bufferSize, ".0");	
				}
			} break;
            case EditorElement_Signed:
			{
				
				if(element->value[0] != '-' && element->value[0] != '+')
				{
					buffer = OutputToBuffer(buffer, bufferSize, "+");
				}
                
				buffer = OutputToBuffer(buffer, bufferSize, element->value);
			}
            case EditorElement_Unsigned:
            {
                buffer = OutputToBuffer(buffer, bufferSize, element->value);
            } break;
			
			case EditorElement_List:
			{
				buffer = OutputToBuffer(buffer, bufferSize, " (");
                buffer = WriteElements(buffer, bufferSize, element->emptyElement);
				buffer = WriteElements(buffer, bufferSize, element->firstInList);
				buffer = OutputToBuffer(buffer, bufferSize, ") ");
			} break;
            
			case EditorElement_Struct:
			{
				buffer = OutputToBuffer(buffer, bufferSize, " {");
				buffer = WriteElements(buffer, bufferSize, element->firstValue);
				buffer = OutputToBuffer(buffer, bufferSize, "} ");
			} break;
		}
        
		if(element->next)
		{
			buffer = OutputToBuffer(buffer, bufferSize, ",");
		}
		element = element->next;
	}
    
    return buffer;
}

enum LoadElementsMode
{
    LoadElements_Tab,
    LoadElements_Asset,
};

inline EditorElement* LoadElementInMemory(LoadElementsMode mode, Tokenizer* tokenizer, b32* end)
{
	EditorElement* newElement;
    FREELIST_ALLOC(newElement, taxTable_->firstFreeElement, PushStruct(taxPool_, EditorElement));
    *newElement = {};
    
    
	Token firstToken = GetToken(tokenizer);
    if(firstToken.type == Token_String)
    {
        firstToken = Stringize(firstToken);
        firstToken.type = Token_Identifier;
    }
    
	if(firstToken.type == Token_Identifier)
	{
        StrCpy(firstToken.text, firstToken.textLength, newElement->name, sizeof(newElement->name));
        
		if(RequireToken(tokenizer, Token_EqualSign))
		{
			Token t = GetToken(tokenizer);
			switch(t.type)
			{
				case Token_String:
				{
					newElement->type = EditorElement_String;
                    Token value = Stringize(t);
                    StrCpy(value.text, value.textLength, newElement->value, sizeof(newElement->value));
                    
                    if(TokenEquals(firstToken, "eventName"))
                    {
                        AddFlags(newElement, EditorElem_AlwaysEditable);
                    }
                } break;
                
                case Token_Number:
                {
                    b32 realNumber = false;
                    for(i32 charIndex = 0; charIndex < t.textLength; ++charIndex)
                    {
                        char test = t.text[charIndex];
                        if(test == '.')
                        {
                            realNumber = true;
                            break;
                        }
                    }
                    if(realNumber)
                    {
                        newElement->type = EditorElement_Real;
                    }
                    else if(t.text[0] == '-' || t.text[0] == '+')
                    {
                        newElement->type = EditorElement_Signed;
                    }
                    else
                    {
                        newElement->type = EditorElement_Unsigned;
                    }
                    
                    StrCpy(t.text, t.textLength, newElement->value, sizeof(newElement->value));
                } break;
                
                case Token_OpenParen:
                {
                    newElement->type = EditorElement_List;
                    
                    if(NextTokenIs(tokenizer, Token_Identifier))
                    {
                    }
                    
                    b32 parsingParams = true;
                    while(parsingParams)
                    {
                        if(NextTokenIs(tokenizer, Token_Pound))
                        {
                            Token pound = GetToken(tokenizer);
                            Token paramName = GetToken(tokenizer);
                            if(RequireToken(tokenizer, Token_EqualSign))
                            {
                                if(TokenEquals(paramName, "empty"))
                                {
                                    newElement->emptyElement = LoadElementInMemory(mode, tokenizer, end);
                                    FormatString(newElement->emptyElement->name, sizeof(newElement->emptyElement->name), "empty");
                                }
                                else if(TokenEquals(paramName, "recursiveEmpty"))
                                {
                                    
                                }
                                else
                                {
                                    InvalidCodePath;
                                }
                            }
                        }
                        else
                        {
                            parsingParams = false;
                        }
                    }
                    
                    
                    if(NextTokenIs(tokenizer, Token_CloseParen))
                    {
                        newElement->firstInList = 0;
                        GetToken(tokenizer);
                    }
                    else
                    {
                        if(!NextTokenIs(tokenizer, Token_OpenBraces))
                        {
                            InvalidCodePath;
                        }
                        newElement->firstInList = LoadElementInMemory(mode, tokenizer, end);
                        if(!RequireToken(tokenizer, Token_CloseParen))
                        {
                            InvalidCodePath;
                        }
                    }
                } break;
                
                case Token_OpenBraces:
                {
                    newElement->type = EditorElement_Struct;
                    newElement->firstValue = LoadElementInMemory(mode, tokenizer, end);
                    
                    if(!RequireToken(tokenizer, Token_CloseBraces))
                    {
                        InvalidCodePath;
                    }
                }
            }
        }
        else
        {
            InvalidCodePath;
        }
    }
    else
    {
        newElement->name[0] = 0;
        if(firstToken.type == Token_OpenBraces)
        {
            newElement->type = EditorElement_Struct;
            newElement->firstValue = LoadElementInMemory(mode, tokenizer, end);
            
            if(!RequireToken(tokenizer, Token_CloseBraces))
            {
                InvalidCodePath;
            }
        }
    }
    
    
    if(NextTokenIs(tokenizer, Token_Comma))
    {
        Token comma = GetToken(tokenizer);
        
        if(NextTokenIs(tokenizer, Token_Identifier) || 
           NextTokenIs(tokenizer, Token_String) || 
           NextTokenIs(tokenizer, Token_OpenBraces))
        {
            newElement->next = LoadElementInMemory(mode, tokenizer, end);
        }
    }
    else if(NextTokenIs(tokenizer, Token_EndOfFile))
    {
        *end = true;
    }
    
    return newElement;
}

inline void FreeElement(EditorElement* element)
{
    if(element)
    {
        switch(element->type)
        {
            case EditorElement_String:
            case EditorElement_Real:
            case EditorElement_Signed:
            case EditorElement_Unsigned:
            {
            } break;
            
            case EditorElement_List:
            {
                FreeElement(element->firstInList);
            } break;
            
            case EditorElement_Struct:
            {
                FreeElement(element->firstValue);
            } break;
            
            case EditorElement_Taxonomy:
            {
                FreeElement(element->firstChild);
            } break;
            
            case EditorElement_EmptyTaxonomy:
            {
            } break;
            
            InvalidDefaultCase;
        }
        
        if(element->next)
        {
            FreeElement(element->next);
        }
        
        FREELIST_DEALLOC(element, taxTable_->firstFreeElement);
    }
}

inline b32 IsEditableByRole(EditorElement* root, u32 role)
{
    char* tabName = root->name;
    
    b32 result = false;
    if(StrEqual(tabName, "soundEffects"))
    {
        result = (role & EditorRole_SoundDesigner);
    }
    else
    {
        result = (role == EditorRole_Everyone);
    }
    
    return result;
}

internal void LoadFileInTaxonomySlot(char* content)
{
    Tokenizer tokenizer = {};
    tokenizer.at = content;
    
    for(u32 tabIndex = 0; tabIndex < currentSlot_->tabCount; ++tabIndex)
    {
        EditorTab* tab = currentSlot_->tabs + tabIndex;
        FreeElement(tab->root);
        tab->editable = 0;
    }
    currentSlot_->tabCount = 0;
    
    
    b32 end = false;
    while(true)
    {
        Assert(currentSlot_->tabCount < ArrayCount(currentSlot_->tabs));
        
        EditorTab* newTab = currentSlot_->tabs + currentSlot_->tabCount++;
        newTab->root = LoadElementInMemory(LoadElements_Tab, &tokenizer, &end);
        
        if(end)
        {
            break;
        }
    }
}

inline EditorElement* GetElement(EditorElement* element, char* name)
{
    EditorElement* result = 0;
    if(element->type == EditorElement_Struct)
    {
        EditorElement* value = element->firstValue;
        while(value)
        {
            {
                if(StrEqual(value->name, name))
                {
                    result = value;
                    break;
                }
            }
            
            value = value->next;
        }
    }
    else
    {
        InvalidCodePath;
    }
    
    return result;
}

inline char* GetValue(EditorElement* element, char* name)
{
    char* result = 0;
    if(element->type == EditorElement_Struct)
    {
        EditorElement* value = element->firstValue;
        while(value)
        {
            {
                if(StrEqual(value->name, name))
                {
                    result = value->value;
                    break;
                }
            }
            
            value = value->next;
        }
    }
    else
    {
        InvalidCodePath;
    }
    
    return result;
}

inline EditorElement* GetStruct(EditorElement* element, char* name)
{
    EditorElement* result = 0;
    if(element->type == EditorElement_Struct)
    {
        EditorElement* value = element->firstValue;
        while(value)
        {
            if(value->type == EditorElement_Struct)
            {
                if(StrEqual(value->name, name))
                {
                    result = value;
                    break;
                }
            }
            
            value = value->next;
        }
    }
    else
    {
        InvalidCodePath;
    }
    
    return result;
}

inline EditorElement* GetList(EditorElement* element, char* listName)
{
    EditorElement* result = 0;
    if(element->type == EditorElement_Struct)
    {
        EditorElement* value = element->firstValue;
        while(value)
        {
            if(value->type == EditorElement_List)
            {
                if(StrEqual(value->name, listName))
                {
                    result = value->firstInList;
                    break;
                }
            }
            
            value = value->next;
        }
    }
    else
    {
        InvalidCodePath;
    }
    
    return result;
}

#ifndef FORG_SERVER
inline void AddSoundAndChildContainersRecursively(SoundContainer* rootContainer, EditorElement* root)
{
    EditorElement* sounds = GetList(root, "sounds");
    while(sounds)
    {
        char* soundType = GetValue(sounds, "soundType");
        char* soundName = GetValue(sounds, "sound");
        if(soundType && soundName)
        {
            AddSoundToContainer(rootContainer, soundType, soundName);
        }
        sounds = sounds->next;
    }
    
    EditorElement* childs = GetList(root, "childs");
    while(childs)
    {
        SoundContainer* childContainer = AddChildContainer(rootContainer);
        AddSoundAndChildContainersRecursively(childContainer, childs);
        
        childs = childs->next;
    }
}
#endif

internal void Import(TaxonomySlot* slot, EditorElement* root)
{
    currentSlot_ = slot;
    
    char* name = root->name;
    if(StrEqual(name, "bounds"))
    {
        char* exists = GetValue(root, "physical");
        b32 physical = ToB32(exists);
        
        char* height = GetValue(root, "width");
        char* radious = GetValue(root, "radious");
        
        if(physical)
        {
            DefineBounds(height, radious);
        }
        else
        {
            DefineNullBounds(height, radious);
        }
    }
    else if(StrEqual(name, "plantBounds"))
    {
        char* segmentNumber = GetValue(root, "segmentNumber");
        char* radious = GetValue(root, "radious");
        char* length = GetValue(root, "length");
        DefinePlantBounds(segmentNumber, radious, length);
    }
    
    
    
    else if(StrEqual(name, "equipmentMappings"))
    {
        EditorElement* singleSlot = GetList(root, "singleSlot");
        EditorElement* doubleSlot = GetList(root, "doubleSlot");
        EditorElement* multiPart = GetList(root, "multiPart");
        
        
        while(singleSlot)
        {
            char* equipName = GetValue(singleSlot, "equipment");
            char* slotName = GetValue(singleSlot, "slot");
            CanEquip(equipName, slotName, slotName);
            
            singleSlot = singleSlot->next;
        }
        
        while(doubleSlot)
        {
            char* equipName = GetValue(doubleSlot, "equipment");
            char* slotName = GetValue(doubleSlot, "slot");
            char* slotName2 = GetValue(doubleSlot, "slot2");
            CanEquip(equipName, slotName, slotName2);
            
            doubleSlot = doubleSlot->next;
        }
        
        while(multiPart)
        {
            char* equipName = GetValue(multiPart, "equipment");
            CanEquipMultipart(equipName);
            
            EditorElement* part = GetList(multiPart, "slots");
            while(part)
            {
                char* partName = GetValue(part, "name");
                AddPart(partName);
                part = part->next;
            }
            
            multiPart = multiPart->next;
        }
    }
    
    
    else if(StrEqual(name, "consumeMappings"))
    {
        EditorElement* consume = root->firstInList;
        while(consume)
        {
            char* actionName = GetValue(consume, "action");
            char* objectName = GetValue(consume, "object");
            CanConsume(actionName, objectName);
            
            consume = consume->next;
        }
    }
    
    else if(StrEqual(name, "components"))
    {
        EditorElement* templates = root->firstInList;
        while(templates)
        {
            EditorElement* component = GetList(templates, "componentList");
            while(component)
            {
                char* componentName = GetValue(component, "name");
                if(componentName)
                {
                    AddComponent(componentName);
                }
                
                EditorElement* ingredient = GetList(component, "ingredients");
                while(ingredient)
                {
                    char* ingredientName = GetValue(ingredient, "name");
                    MadeOf(ingredientName);
                    
                    ingredient = ingredient->next;
                }
                
                component = component->next;
            }
            
            templates = templates->next;
        }
    }
    else if(StrEqual(name, "neededTools"))
    {
        EditorElement* tools = root->firstInList;
        while(tools)
        {
            char* toolName = GetValue(tools, "name");
            Requires(toolName);
            tools = tools->next;
        }
    }
    else if(StrEqual(name, "requireEssences"))
    {
        EditorElement* essences = root->firstInList;
        
        while(essences)
        {
            char* essenceName = GetValue(essences, "name");
            char* quantity = GetValue(essences, "quantity");
            
            AddEssence(essenceName, ToU8(quantity));
            
            essences = essences->next;
        }
    }
#if FORG_SERVER
    else if(StrEqual(name, "craftingEssences"))
    {
        EditorElement* essences = root->firstInList;
        
        while(essences)
        {
            char* essenceName = GetValue(essences, "name");
            char* quantity = GetValue(essences, "quantity");
            
            AddEssence(essenceName, ToU8(quantity));
            
            essences = essences->next;
        }
    }
    else if(StrEqual(name, "effects"))
    {
        EditorElement* effectList = root->firstInList;
        while(effectList)
        {
            char* action = GetValue(effectList, "action");
            char* effectName = GetValue(effectList, "id");
            AddEffect(action, effectName);
            
            
            char* passive = GetValue(effectList, "passive");
            if(passive)
            {
                InvalidCodePath;
                IsPassive();
            }
            
            char* timer = GetValue(effectList, "timer");
            if(timer)
            {
                Timer(timer);
            }
            
            EditorElement* flagList = GetList(effectList, "flags");
            while(flagList)
            {
                char* flag = GetValue(flagList, "name");
                AddFlag(flag);
                
                flagList = flagList->next;
            }
            
            EditorElement* data = GetStruct(effectList, "data");
            char* type = GetValue(data, "type");
            if(StrEqual(type, "power"))
            {
                char* power = GetValue(data, "value");
                Power(power);
            }
            else if(StrEqual(type, "spawn"))
            {
                char* taxonomy = GetValue(data, "value");
                Spawn(taxonomy);
            }
            else
            {
                InvalidCodePath;
            }
            
            EditorElement* freeHandsReq = GetList(effectList, "freeHandReq");
            while(freeHandsReq)
            {
                char* slotName = GetValue(freeHandsReq, "slot");
                char* taxonomy = GetValue(freeHandsReq, "name");
                
                AddFreeHandReq(slotName, taxonomy);
                
                freeHandsReq = freeHandsReq->next;
            }
            
            effectList = effectList->next;
        }
    }
    else if(StrEqual(name, "craftingEffects"))
    {
        EditorElement* craftingEffectsList = root->firstInList;
        while(craftingEffectsList)
        {
            char* action = GetValue(craftingEffectsList, "action");
            char* id = GetValue(craftingEffectsList, "id");
            char* target = GetValue(craftingEffectsList, "target");
            
            LinkStandard(action, id, target);
            
            EditorElement* requirements = GetList(craftingEffectsList, "requirements");
            Assert(requirements);
            while(requirements)
            {
                char* ingredient = GetValue(requirements, "name");
                char* quantity = GetValue(requirements, "quantity");
                
                Requires(ingredient, ToU32(quantity));
                
                requirements = requirements->next;
            }
            
            craftingEffectsList = craftingEffectsList->next;
        }
    }
    else if(StrEqual(name, "attributes"))
    {
        EditorElement* attributes = root->firstInList;
        
        while(attributes)
        {
            char* attributeName = GetValue(attributes, "name");
            char* value = GetValue(attributes, "value");
            
            SaveCreatureAttribute(attributeName, ToR32(value));
            
            attributes = attributes->next;
        }
    }
    
    else if(StrEqual(name, "space"))
    {
        u8 width = ToU8(GetValue(root, "width"));
        u8 height = ToU8(GetValue(root, "height"));
        
        InventorySpace(width, height);
    }
    else if(StrEqual(name, "plantPhases"))
    {
        EditorElement* status = root->firstInList;
        while(status)
        {
            char* statusName = GetValue(status, "name");
            r32 time = ToR32(GetValue(status, "time"));
            char* newStatus = GetValue(status, "newStatus");
            
            PlantStatus(statusName, time, newStatus);
            status = status->next;
        }
    }
    
    
    else if(StrEqual(name, "playerActions"))
    {
        EditorElement* actions = root->firstInList;
        while(actions)
        {
            char* action = GetValue(actions, "action");
            
            BeginPlayerAction(action);
            
            EditorElement* flags = GetList(actions, "flags");
            while(flags)
            {
                char* flagName = GetValue(flags, "name");
                AddActionFlag(flagName);
                
                flags = flags->next;
            }
            
            EditorElement* targets = GetList(actions, "targets");
            while(targets)
            {
                char* targetName = GetValue(targets, "name");
                AddTarget(targetName);
                
                targets = targets->next;
            }
            
            actions = actions->next;
        }
    }
    
    else if(StrEqual(name, "behaviors"))
    {
        EditorElement* behaviors = root->firstInList;
        while(behaviors)
        {
            char* generic = GetValue(behaviors, "generic");
            char* specific = GetValue(behaviors, "specific");
            char* primary = GetValue(behaviors, "primary");
            
            AssociateBehavior(generic, specific, ToB32(primary));
            
            behaviors = behaviors->next;
        }
    }
    else if(StrEqual(name, "memBehaviors"))
    {
        EditorElement* behaviors = root->firstInList;
        while(behaviors)
        {
            char* behavior = GetValue(behaviors, "name");
            AddMemBehavior(behavior);
            
            behaviors = behaviors->next;
        }
    }
#endif
    
#ifndef FORG_SERVER
    else if(StrEqual(name, "visualTags"))
    {
        EditorElement* tags = root->firstInList;
        
        while(tags)
        {
            char* tagName = GetValue(tags, "name");
            char* value = GetValue(tags, "value");
            
            TagId ID = (TagId) GetValuePreprocessor(TagId, tagName);
            r32 val = ToR32(value);
            AddTag(ID, val);
            tags = tags->next;
        }
    }
    else if(StrEqual(name, "skeleton"))
    {
        char* skeleton = GetValue(root, "name");
        UsesSkeleton(skeleton);
    }
    else if(StrEqual(name, "light"))
    {
        char* intensity = GetValue(root, "intensity");
        AddLight(ToR32(intensity), V3(1, 1, 1));
    }
    else if(StrEqual(name, "animationEffects"))
    {
        EditorElement* effects = root->firstInList;
        while(effects)
        {
            char* type = GetValue(effects, "type");
            if(StrEqual(type, "perpetual"))
            {
                AddPerpetualEffect(SpawnParticlesEffect(FluidSpawn_Fire));
            }
            else
            {
                InvalidCodePath;
            }
            
            effects = effects->next;
        }
    }
    else if(StrEqual(name, "soundEffects"))
    {
        EditorElement* effects = root->firstInList;
        while(effects)
        {
            char* animationName = GetValue(effects, "animationName");
            char* time = GetValue(effects, "time");
            char* event = GetValue(effects, "eventName");
            
            
            TaxonomySound* sound = AddSoundEffect(animationName, time, event);
            
            
#if 0            
            EditorElement* labels = GetList(effects, "labels");
            while(labels)
            {
                char* labelName = GetValue(labels, "name");
                char* labelValue = GetValue(labels, "value");
                
                AddLabel(sound, labelName, labelValue);
                
                labels = labels->next;
            }
#endif
            
            effects = effects->next;
        }
    }
    else if(StrEqual(name, "soundEvents"))
    {
        EditorElement* events = root->firstInList;
        while(events)
        {
            char* eventName = GetValue(events, "eventName");
            SoundContainer* rootContainer = AddSoundEvent(eventName);
            
            AddSoundAndChildContainersRecursively(rootContainer, events);
            
            events = events->next;
        }
    }
#endif
    
    
    
    if(root->next)
    {
        Import(slot, root->next);
    }
}

inline Token GetFileTaxonomyName(char* content)
{
    Tokenizer tokenizer = {};
    tokenizer.at = content;
    
    Token t = GetToken(&tokenizer);
    
    Assert(t.type == Token_String);
    
    Token result = Stringize(t);
    
    return result;
}

internal void ImportAllFiles(char* dataPath, MemoryPool* tempPool, b32 freeTab)
{
    TempMemory fileMemory = BeginTemporaryMemory(tempPool);
    
    u32 bufferSize = MegaBytes(64);
    char* buffer = (char*) PushSize(tempPool, bufferSize, NoClear());
    
    for(u32 effectIndex = 0; effectIndex < ArrayCount(MetaTable_EffectIdentifier); ++effectIndex)
    {
        TaxonomySlot* effectSlot = NORUNTIMEGetTaxonomySlotByName(taxTable_, MetaTable_EffectIdentifier[effectIndex]);
        effectSlot->effectID = (EffectIdentifier) effectIndex;
    }
    
    
    PlatformFileGroup fileGroup = platformAPI.GetAllFilesBegin(PlatformFile_entityDefinition, dataPath);
    for(u32 fileIndex = 0; fileIndex < fileGroup.fileCount; ++fileIndex)
    {
        PlatformFileHandle handle = platformAPI.OpenNextFile(&fileGroup, dataPath);
        if(!StrEqual(handle.name, "taxonomies.fed"))
        {
            Assert(handle.fileSize <= bufferSize);
            platformAPI.ReadFromFile(&handle, 0, handle.fileSize, buffer);
            buffer[handle.fileSize] = 0;
            char* source = (char*) buffer;
            
            char taxonomyName[64];
            GetNameWithoutPoint(taxonomyName, sizeof(taxonomyName), handle.name);
            
            currentSlot_ = NORUNTIMEGetTaxonomySlotByName(taxTable_, taxonomyName);
            if(currentSlot_)
            {
                LoadFileInTaxonomySlot(source);
                for(u32 tabIndex = 0; tabIndex < currentSlot_->tabCount; ++tabIndex)
                {
                    EditorTab* tab = currentSlot_->tabs + tabIndex;
                    Import(currentSlot_, tab->root);
                    
                    if(freeTab)
                    {
                        FreeElement(tab->root);
                        tab->root = 0;
                    }
                }
                
                if(freeTab)
                {
                    currentSlot_->tabCount = 0;
                }
                
            }
        }
        platformAPI.CloseHandle(&handle);
    }
    platformAPI.GetAllFilesEnd(&fileGroup);
    
    EndTemporaryMemory(fileMemory);
}

#ifndef FORG_SERVER
internal void ImportAllAssetFiles(GameModeWorld* worldMode, char* dataPath, MemoryPool* tempPool)
{
    TempMemory fileMemory = BeginTemporaryMemory(tempPool);
    u32 bufferSize = MegaBytes(64);
    char* buffer = (char*) PushSize(tempPool, bufferSize, NoClear());
    
    
    PlatformFileGroup assetGroup = platformAPI.GetAllFilesBegin(PlatformFile_assetDefinition, dataPath);
    for(u32 fileIndex = 0; fileIndex < assetGroup.fileCount; ++fileIndex)
    {
        PlatformFileHandle handle = platformAPI.OpenNextFile(&assetGroup, dataPath);
        Assert(handle.fileSize <= bufferSize);
        platformAPI.ReadFromFile(&handle, 0, handle.fileSize, buffer);
        buffer[handle.fileSize] = 0;
        
        Tokenizer tokenizer = {};
        tokenizer.at = (char*) buffer;
        
        b32 ign = false;
        if(StrEqual(handle.name, "sound.fad"))
        {
            FreeElement(worldMode->UI->soundNamesRoot);
            worldMode->UI->soundNamesRoot = LoadElementInMemory(LoadElements_Asset, &tokenizer, &ign);
        }
        else if(StrEqual(handle.name, "soundEvents.fad"))
        {
            FreeElement(worldMode->UI->soundEventsRoot);
            worldMode->UI->soundEventsRoot = LoadElementInMemory(LoadElements_Asset, &tokenizer, &ign);
            Import(0, worldMode->UI->soundEventsRoot);
        }
        
        platformAPI.CloseHandle(&handle);
    }
    platformAPI.GetAllFilesEnd(&assetGroup);
    
    EndTemporaryMemory(fileMemory);
}
#endif

#if FORG_SERVER
inline void LoadAssets()
{
    PlatformProcessHandle assetBuilder = platformAPI.DEBUGExecuteSystemCommand(".", "../build/asset_builder.exe", "");
    while(true)
    {
        PlatformProcessState assetBuilderState = platformAPI.DEBUGGetProcessState(assetBuilder);
        if(!assetBuilderState.isRunning)
        {
            break;
        }
    }
}

internal void WriteToFile(TaxonomyTable* table, TaxonomySlot* slot)
{    
    char path[512];
    char* writeHere = path;
    
    u32 remainingSize = sizeof(path);
    writeHere += FormatString(writeHere, remainingSize, "definition/");
    
    TaxonomySlot* toWrite = &table->root;
    while(true)
    {
        u32 written = (u32) FormatString(writeHere, remainingSize, "%s/", toWrite->name);
        remainingSize -= written;
        writeHere += written;
        
        if(toWrite->taxonomy == slot->taxonomy)
        {
            break;
        }
        
        
        toWrite = GetChildSlot(table, toWrite, slot->taxonomy);
    } 
    
    
    
    
    
    char buffer[KiloBytes(16)] = {};
    u32 remaining = sizeof(buffer);
    u32 writtenTotal = 0;
    
    char* currentBuffer = buffer;
    for(u32 tabIndex = 0; tabIndex < slot->tabCount; ++tabIndex)
    {
        u32 newRemaining = remaining;
        WriteElements(currentBuffer, &newRemaining, slot->tabs[tabIndex].root);
        u32 written = remaining - newRemaining;
        writtenTotal += written;
        
        currentBuffer += written;
        remaining = newRemaining;
        
        
        *currentBuffer++ = ' ';
        *currentBuffer++ = '\n';
        remaining -= 2;
        writtenTotal += 2;
    }
    
    
    FormatString(writeHere, remainingSize, "%s.fed", slot->name);
    platformAPI.DEBUGWriteFile(path, buffer, writtenTotal);
}

inline void PatchLocalServer()
{
    char* destinationFolder = "../server/definition";
    platformAPI.DeleteFolderRecursive(destinationFolder);
    platformAPI.CreateFolder(destinationFolder);
    platformAPI.CopyAllFiles("definition", destinationFolder);
}


internal void SendAllDataFiles(b32 editorMode, char* path, ServerPlayer* player, MemoryPool* tempPool, b32 sendTaxonomyFiles, char* singleFileNameToSend = 0)
{
    TempMemory fileMemory = BeginTemporaryMemory(tempPool);
    
    u32 bufferSize = MegaBytes(64);
    char* buffer = (char*) PushSize(tempPool, bufferSize, NoClear());
    
    
    if(sendTaxonomyFiles)
    {
        PlatformFileGroup definitionGroup = platformAPI.GetAllFilesBegin(PlatformFile_entityDefinition, path);
        for(u32 fileIndex = 0; fileIndex < definitionGroup.fileCount; ++fileIndex)
        {
            PlatformFileHandle handle = platformAPI.OpenNextFile(&definitionGroup, path);
            
            if(!singleFileNameToSend || StrEqual(singleFileNameToSend, handle.name))
            {
                Assert(handle.fileSize <= bufferSize);
                platformAPI.ReadFromFile(&handle, 0, handle.fileSize, buffer);
                buffer[handle.fileSize] = 0;
                char* source = (char*) buffer;
                
                
                SendDataFile(player, handle.name, source, handle.fileSize);
            }
            
            platformAPI.CloseHandle(&handle);
        }
        platformAPI.GetAllFilesEnd(&definitionGroup);
    }
    
    if(editorMode)
    {
        PlatformFileGroup autocompleteGroup = platformAPI.GetAllFilesBegin(PlatformFile_autocomplete, path);
        for(u32 fileIndex = 0; fileIndex < autocompleteGroup.fileCount; ++fileIndex)
        {
            PlatformFileHandle handle = platformAPI.OpenNextFile(&autocompleteGroup, path);
            
            if(!singleFileNameToSend || StrEqual(singleFileNameToSend, handle.name))
            {
                Assert(handle.fileSize <= bufferSize);
                platformAPI.ReadFromFile(&handle, 0, handle.fileSize, buffer);
                buffer[handle.fileSize] = 0;
                char* source = (char*) buffer;
                
                
                SendDataFile(player, handle.name, source, handle.fileSize);
            }
            
            platformAPI.CloseHandle(&handle);
        }
        platformAPI.GetAllFilesEnd(&autocompleteGroup);
        
        
        PlatformFileGroup assetGroup = platformAPI.GetAllFilesBegin(PlatformFile_assetDefinition, path);
        for(u32 fileIndex = 0; fileIndex < assetGroup.fileCount; ++fileIndex)
        {
            PlatformFileHandle handle = platformAPI.OpenNextFile(&assetGroup, path);
            
            if(!singleFileNameToSend || StrEqual(singleFileNameToSend, handle.name))
            {
                Assert(handle.fileSize <= bufferSize);
                platformAPI.ReadFromFile(&handle, 0, handle.fileSize, buffer);
                buffer[handle.fileSize] = 0;
                char* source = (char*) buffer;
                
                
                SendDataFile(player, handle.name, source, handle.fileSize);
            }
            
            platformAPI.CloseHandle(&handle);
        }
        platformAPI.GetAllFilesEnd(&assetGroup);
    }
    
    SendAllDataFileSentMessage(player, sendTaxonomyFiles);
    EndTemporaryMemory(fileMemory);
}
#endif

internal void WriteAllFiles(MemoryPool* tempPool, char* dataPath, DataFileArrived* firstArrived, b32 compressed)
{
    
    
    while(firstArrived)
    {
        TempMemory fileMemory = BeginTemporaryMemory(tempPool);
        
        Assert(firstArrived->runningFileSize == firstArrived->fileSize);
        char completeName[128];
        FormatString(completeName, sizeof(completeName), "%s/%s", dataPath, firstArrived->name);
        
        u8* data = firstArrived->data;
        u32 fileSize = firstArrived->fileSize;
        
        if(compressed)
        {
            uLong uncompressedSize = *((uLong*) firstArrived->data);
            fileSize = uncompressedSize;
            
            u8* uncompressed = PushArray(tempPool, u8, uncompressedSize);
            
            u8* compressedSource = (u8*) firstArrived->data + 4;
            uLong compressedLen = firstArrived->fileSize - 4;
            
            int cmp_status = uncompress(uncompressed, &uncompressedSize, compressedSource, compressedLen);
            Assert(cmp_status == Z_OK);
            Assert(uncompressedSize == fileSize);
            
            data = uncompressed;
        }
        
        platformAPI.DEBUGWriteFile(completeName, data, fileSize);
        
        firstArrived = firstArrived->next;
        
        
        EndTemporaryMemory(fileMemory);
    }
    
}


#if 0
internal void Merge()
{
    GetAllFiles(f1);
    GetAllFiles(f2);
    
    for(everyf2File)
    {
        if(!existsInf1)
        {
            Add();
        }
        else
        {
            if(file.type == type_Fed)
            {
                MergeDefinitionFiles();
            }
            else
            {
                Assert(fileAreEquals);
            }
        }
    }
    
    for(everyFolder1)
    {
        MergeSubFolder();
    }
}

internal void Merge()
{
    MyDefinition = ?;
    GetAllSubDirectories()
    {
        if(subDirectoryStartsWith(root))
        {
            MergeDefinitions();
        }
    }
}

internal void ProduceOutput(folder)
{
    if(everythingIsValid)
    {
        for(everyFile)
        {
            CopyFile(completeBuild);
            if(file->dirty)
            {
                CopyFile(patch);
            }
        }
        for(everyFoder)
        {
            ProduceOutput(subfoolder);
        }
    }	
}
#endif
