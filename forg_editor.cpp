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

inline void InitTaxonomyReadWrite(TaxonomyTable* table)
{
    taxTable_ = table;
    taxTable_->errorCount = 0;
    taxPool_ = &table->pool;
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
    
    
    if(name[0] == '#')
    {
        ++currentSlot->invalidTaxonomiesCount;
    }
    
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
    if(!StrEqual(name, ".") && !StrEqual(name, "..") && !StrEqual(name, "side"))
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
            if(subName[0] != '#')
            {
                WriteDataFilesRecursive(newPath, subName);
            }
        }
        
		for(u32 subdirIndex = 0; subdirIndex < subdir.subDirectoryCount; ++subdirIndex)
		{
			char* subName = subdir.subdirs[subdirIndex];
			if(subName[0] == '#')
			{
				WriteDataFilesRecursive(newPath, subName);
            }
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
    else if(StrEqual(name, "tiles"))
    {
        taxTable_->tileTaxonomy = taxonomy;
    }
    else if(StrEqual(name, "generators"))
    {
        taxTable_->generatorTaxonomy = taxonomy;
    }
    
}


internal u32 FinalizeTaxonomies(char* giantBuffer, u32 giantBufferLength, b32 writeToFile, TaxonomySlot* parent, TaxonomySlot* slot, ShortcutSlot* shortcut)
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
    slot->invalidTaxonomiesCount = shortcut->invalidTaxonomiesCount;
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
        u32 writtenChild = FinalizeTaxonomies(giantBuffer, giantBufferLength, true, slot, newSlot, newShortcut);
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
	platformAPI.DeleteFileWildcards("assets", "*.fed");
    
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
    u32 written = FinalizeTaxonomies(giantBuffer, giantBufferSize, false, 0, rootSlot, rootShortcut);
    platformAPI.DEBUGWriteFile("assets/taxonomies.fed", giantBuffer, written);
    
    EndTemporaryMemory(bufferMemory);
}







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
    FinalizeTaxonomies(0, 0, 0, 0, rootSlot, rootShortcut);
}




























#define GetValuePreprocessor(table, test) GetValuePreprocessor_(MetaTable_##table, ArrayCount(MetaTable_##table), test) 
inline u32 GetValuePreprocessor_(char** values, u32 count, char* test)
{
    u32 result = 0;
    if(test)
    {
        for(u32 valueIndex = 0; valueIndex < count; ++valueIndex)
        {
            if(StrEqual(test, values[valueIndex]))
            {
                result = valueIndex;
                break;
            }
        }
        
    }
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


inline u8 ToU8(char* string, u8 default = 0)
{
    u8 result = default;
    if(string)
    {
        result = SafeTruncateToU8(atoi(string));
    }
    return result;
}

inline u32 ToU32(char* string, u32 default = 0)
{
    u32 result = default;
    if(string)
    {
        i32 intValue = atoi(string);
        
        Assert(intValue >= 0);
        result = (u32) intValue;
    }
    return result;
}

inline u64 ToU64(char* string)
{
    i64 intValue = atoi(string);
    
    Assert(intValue >= 0);
    u64 result = (u64) intValue;
    return result;
}

inline b32 ToB32(char* string)
{
    b32 result = (StrEqual(string, "true"));
    return result;
}

inline r32 ToR32(char* string, r32 standard = 1.0f)
{
    r32 result = standard;
    
    if(string)
    {
        result = (r32) atof(string);
    }
    
    return result;
}

inline char* GetValue(EditorElement* element, char* name);
inline Vec4 ToV4Color(EditorElement* element, Vec4 default = V4(1, 1, 1, 1))
{
    Vec4 result = default;
    if(element)
    {
        result.r = ToR32(GetValue(element, "r"));
        result.g = ToR32(GetValue(element, "g"));
        result.b = ToR32(GetValue(element, "b"));
        result.a = ToR32(GetValue(element, "a"));
    }
    return result;
}

inline Vec2 ToV2(EditorElement* element, Vec2 default = {})
{
    Vec2 result = default;
    if(element)
    {
        result.x = ToR32(GetValue(element, "x"));
        result.y = ToR32(GetValue(element, "y"));
    }
    return result;
}

inline Vec3 ToV3(EditorElement* element, Vec3 default = {})
{
    Vec3 result = default;
    if(element)
    {
        result.x = ToR32(GetValue(element, "x"));
        result.y = ToR32(GetValue(element, "y"));
        result.z = ToR32(GetValue(element, "z"));
    }
    return result;
}



#if FORG_SERVER
#define EditorErrorLog(name) EditorErrorLogServer(__FUNCTION__, name)
#else
#define EditorErrorLog(name) EditorErrorLogClient(__FUNCTION__, name)
#endif

inline void StartingLoadingMessageServer()
{
    printf("\n\n\nStarting Loading Datafiles...\n\n\n");
}

inline void EndingLoadingMessageServer()
{
    printf("\n\n\nDatafiles Loaded.\n\n\n");
}

inline void EditorErrorLogServer(char* functionName, char* wasSearching)
{
    Assert(currentSlot_);
    printf("Unable to find %s in function %s when loading %s\n", wasSearching, functionName, currentSlot_->name);
}


inline void EditorErrorLogClient(char* functionName, char* wasSearching)
{
    if(taxTable_->errorCount < ArrayCount(taxTable_->errors))
    {
        char* error = taxTable_->errors[taxTable_->errorCount++];
        FormatString(error, sizeof(taxTable_->errors[0]), "Unable to find %s in function %s when loading %s\n", wasSearching, functionName, currentSlot_->name);
    }
}







#define TAXTABLE_ALLOC(ptr, type) FREELIST_ALLOC(ptr, taxTable_->firstFree##type, PushStruct(taxPool_, type, NoClear())) ZeroStruct(*(ptr));

#define TAXTABLE_DEALLOC(ptr, type) FREELIST_DEALLOC(ptr, taxTable_->firstFree##type)
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






inline EquipmentMapping* AddEquipmentMapping(char* equipmentName)
{
    TaxonomySlot* slot = currentSlot_;
    TaxonomySlot* target = NORUNTIMEGetTaxonomySlotByName(taxTable_, equipmentName);
    
    EquipmentMapping* result = 0;
    if(target)
    {
        EquipmentMapping* mapping;
        TAXTABLE_ALLOC(mapping, EquipmentMapping);
        
        TaxonomyNode* node = AddToTaxonomyTree(&currentSlot_->equipmentMappings, target);
        node->data.equipmentMapping = mapping;
        
        result = mapping;
    }
    else
    {
        EditorErrorLog(equipmentName);
    }
    
    return result;
}

inline void AddPiece(EquipmentLayout* equipmentLayout, u32 assIndex, char* pieceName, u8 index, Vec2 assOffset, r32 zOffset, r32 angle, Vec2 scale)
{
    EquipmentAss* equipmentAss;
    TAXTABLE_ALLOC(equipmentAss, EquipmentAss);
    
    equipmentAss->assIndex = assIndex;
    equipmentAss->stringHashID = StringHash(pieceName);
    equipmentAss->index = index;
    equipmentAss->assOffset = assOffset;
    equipmentAss->zOffset = zOffset;
    equipmentAss->angle = angle;
    equipmentAss->scale = scale;
    
    equipmentAss->next = 0;
    
    FREELIST_INSERT(equipmentAss, equipmentLayout->firstEquipmentAss);
}

inline void CanConsume(char* action, char* name)
{
    TaxonomySlot* slot = currentSlot_;
    TaxonomySlot* target = NORUNTIMEGetTaxonomySlotByName(taxTable_, name);
    
    if(target)
    {
        ConsumeMapping* mapping;
        TAXTABLE_ALLOC(mapping, ConsumeMapping);
        
        
        mapping->action = (EntityAction) GetValuePreprocessor(EntityAction, action);
        mapping->taxonomy = target->taxonomy;
        
        mapping->next = slot->firstConsumeMapping;
        slot->firstConsumeMapping = mapping;
    }
    else
    {
        EditorErrorLog(name);
    }
    
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
    DefineBounds_(boundsHeight, boundsRadious, ForgBound_NonPhysical);
}








































#if FORG_SERVER
global_variable TaxonomyEffect* currentEffect_;
inline void IsPassive()
{
    currentSlot_->isPassiveSkill = true;
}



inline void AddFreeHandReq(char* slot, char* taxonomy)
{
    NakedHandReq* req;
    TAXTABLE_ALLOC(req, NakedHandReq);
    TaxonomySlot* taxonomyslot = NORUNTIMEGetTaxonomySlotByName(taxTable_, taxonomy);
    
    if(taxonomyslot)
    {
        req->slotIndex = SafeTruncateToU8(GetValuePreprocessor(SlotName, slot));
        req->taxonomy = taxonomyslot->taxonomy;
        
        req->next = currentSlot_->nakedHandReq;
        currentSlot_->nakedHandReq = req;
    }
    else
    {
        EditorErrorLog(taxonomy);
    }
}









global_variable CraftingEffectLink* activeCraftingLink_;
inline void LinkStandard(char* action, char* effectName, char* target)
{
    
    CraftingEffectLink* link;
    TAXTABLE_ALLOC(link, CraftingEffectLink);
    link->triggerAction = (EntityAction) GetValuePreprocessor(EntityAction, action);
    link->target = target ? ToB32(target) : false;
    link->effectID = (EffectIdentifier) GetValuePreprocessor(EffectIdentifier, effectName);
    activeCraftingLink_ = link;
    
    FREELIST_INSERT(link, currentSlot_->links);
}

inline void Requires_(char* essenceName)
{
    TaxonomySlot* essenceSlot = NORUNTIMEGetTaxonomySlotByName(taxTable_, essenceName);
    
    if(essenceSlot)
    {
        if(activeCraftingLink_)
        {
            for(u32 essenceIndex = 0; essenceIndex < MAX_ESSENCES_PER_EFFECT; ++essenceIndex)
            {
                if(!activeCraftingLink_->essences[essenceIndex].taxonomy)
                {
                    activeCraftingLink_->essences[essenceIndex].taxonomy = essenceSlot->taxonomy;
                    return;
                }
            }
        }
    }
    else
    {
        EditorErrorLog(essenceName);
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



























inline void SaveCreatureAttribute(char* attributeName, r32 value)
{
    MemberDefinition member = SLOWGetRuntimeOffsetOf(CreatureComponent, attributeName);
    u32 offset = member.offset;
    
    AttributeSlot* attr = GetAttributeSlot(currentSlot_, offset);
    attr->offsetFromBase = offset;
    attr->valueR32 = value;
}

#endif


inline void InventorySpace(u8 width, u8 height)
{
    TaxonomySlot* slot = currentSlot_;
    
    slot->gridDimX = width;
    slot->gridDimY = height;
}

inline void AddEssence(char* name, u32 quantity)
{
    TaxonomySlot* slot = NORUNTIMEGetTaxonomySlotByName(taxTable_, name);
    
    if(slot)
    {
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
    else
    {
        EditorErrorLog(name);
    }
}













inline void BeginPossibleAction(char* action, char* distance)
{
    PossibleAction* possibleAction;
    TAXTABLE_ALLOC(possibleAction, PossibleAction);
    
    u8 actionInt = SafeTruncateToU8(GetValuePreprocessor(EntityAction, action));
    possibleAction->action = (EntityAction) actionInt;
    possibleAction->distance = ToR32(distance, 1.0f);
    possibleAction->flags = 0;
    
    possibleAction->next = currentSlot_->firstPossibleAction;
    currentSlot_->firstPossibleAction = possibleAction;
}

inline void AddActionFlag(char* flagName)
{
    PossibleAction* possibleAction = currentSlot_->firstPossibleAction;
    u32 flag = GetFlagPreprocessor(CanDoActionFlags, flagName);
    possibleAction->flags |= flag;
}


inline void AddActor(char* name, char* requiredTime)
{
    TaxonomySlot* target = NORUNTIMEGetTaxonomySlotByName(taxTable_, name);
    if(target)
    {
        PossibleAction* possibleAction = currentSlot_->firstPossibleAction;
        TaxonomyNode* node = AddToTaxonomyTree(&possibleAction->tree, target);
        node->data.action.requiredTime = ToR32(requiredTime, 1.0f);
    }
    else
    {
        EditorErrorLog(name);
    }
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
    TaxonomySlot* slot = NORUNTIMEGetTaxonomySlotByName(taxTable_, effectName);
    
    if(slot)
    {
        AnimationEffect* dest;
        TAXTABLE_ALLOC(dest, AnimationEffect);
        effect.triggerEffectTaxonomy = slot->taxonomy;
        
        *dest = effect;
        dest->timer = 0;
        
        FREELIST_INSERT(dest, currentSlot_->firstAnimationEffect);
    }
    else
    {
        EditorErrorLog(effectName);
    }
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

inline void UsesSkeleton(char* skeletonName, char* skinName, Vec4 defaultColoration, Vec2 originOffset)
{
    currentSlot_->skeletonHashID = StringHash(skeletonName);
    currentSlot_->skinHashID = StringHash(skinName);
    currentSlot_->defaultColoration = defaultColoration;
    currentSlot_->originOffset = originOffset;
}

inline void AddBoneAlteration(char* boneIndex, char* scaleX, char* scaleY)
{
    TaxonomyBoneAlterations* alt;
    TAXTABLE_ALLOC(alt, TaxonomyBoneAlterations);
    
    alt->boneIndex = ToU32(boneIndex);
    
    alt->alt.valid = true;
    
    alt->alt.scale.x = ToR32(scaleX);
    alt->alt.scale.y = ToR32(scaleY);
    
    FREELIST_INSERT(alt, currentSlot_->firstBoneAlteration);
}

inline void AddAssAlteration(char* assIndex, char* scaleX, char* scaleY, char* offsetX, char* offsetY, b32 specialColoration, Vec4 color)
{
    TaxonomyAssAlterations* alt;
    TAXTABLE_ALLOC(alt, TaxonomyAssAlterations);
    
    alt->assIndex = ToU32(assIndex);
    
    alt->alt.valid = true;
    
    alt->alt.scale.x = ToR32(scaleX);
    alt->alt.scale.y = ToR32(scaleY);
    
    alt->alt.boneOffset.x = ToR32(offsetX);
    alt->alt.boneOffset.y = ToR32(offsetY);
    
    alt->alt.specialColoration = specialColoration;
    alt->alt.color = color;
    
    FREELIST_INSERT(alt, currentSlot_->firstAssAlteration);
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
    event->rootContainer.firstSound = 0;
    
    event->rootContainer.containerCount = 0;
    event->rootContainer.firstChildContainer = 0;
    
    event->rootContainer.labelCount = 0;
    return result;
}

inline LabeledSound* AddSoundToContainer(SoundContainer* container, char* soundType, char* soundName, r32 delay, r32 decibelOffset, r32 pitch, r32 toleranceDistance, r32 distanceFalloffCoeff)
{
    ++container->soundCount;
    
    LabeledSound* sound;
    TAXTABLE_ALLOC(sound, LabeledSound);
    
    sound->typeHash = StringHash(soundType);
    sound->nameHash = StringHash(soundName);
    
    sound->delay = delay;
    sound->decibelOffset = decibelOffset;
    sound->pitch = pitch;
    sound->toleranceDistance = toleranceDistance;
    sound->distanceFalloffCoeff = distanceFalloffCoeff;
    
    FREELIST_INSERT(sound, container->firstSound);
    
    return sound;
}

inline SoundContainer* AddChildContainer(SoundContainer* container)
{
    ++container->containerCount;
    
    SoundContainer* newContainer;
    TAXTABLE_ALLOC(newContainer, SoundContainer);
    
    newContainer->soundCount = 0;
    newContainer->firstSound = 0;
    
    newContainer->containerCount = 0;
    newContainer->firstChildContainer = 0;
    
    newContainer->labelCount = 0;
    
    FREELIST_INSERT(newContainer, container->firstChildContainer);
    
    return newContainer;
}

inline void SetLabel(SoundLabel* label, char* labelName, char* labelValue)
{
    r32 value = labelValue ? ToR32(labelValue) : 0;
    u64 hash = StringHash(labelName);
    
    label->hashID = hash;
    label->value = value;
}

inline void AddSoundLabel(SoundContainer* container, char* labelName, char* labelValue)
{
    Assert(container->labelCount < ArrayCount(container->labels));
    SoundLabel* label = container->labels + container->labelCount++;
    SetLabel(label, labelName, labelValue);
}

inline void AddSoundLabel(LabeledSound* sound, char* labelName, char* labelValue)
{
    Assert(sound->labelCount < ArrayCount(sound->labels));
    SoundLabel* label = sound->labels + sound->labelCount++;
    SetLabel(label, labelName, labelValue);
}



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


























































inline void AddLabel(u64 ID, r32 value)
{
    TaxonomySlot* slot = currentSlot_;
    VisualLabel* dest;
    TAXTABLE_ALLOC(dest, VisualLabel);
    
    u32 hash = (u32) (ID >> 32);
    dest->ID = (hash & (LABEL_HASH_COUNT - 1)) + Tag_count;
    dest->value = value;
    
    FREELIST_INSERT(dest, slot->firstVisualLabel);
}
#endif
inline LayoutPiece* AddLayoutPiece(ObjectLayout* layout, char* componentName, u8 index)
{
    u64 componentHashID = StringHash(componentName);
    
    LayoutPiece* dest;
    TAXTABLE_ALLOC(dest, LayoutPiece);
    
    dest->componentHashID = componentHashID;
	dest->index = index;
    
    FormatString(dest->name, sizeof(dest->name), "%s", componentName);
    dest->ingredientCount = 0;
    dest->parent = 0;
    
    FREELIST_INSERT(dest, layout->firstPiece);
    
    ++layout->pieceCount;
    
    return dest;
}

inline void AddLayoutPieceParams(LayoutPiece* piece, ObjectState state, Vec3 parentOffset, r32 parentAngle, Vec2 scale, r32 alpha, Vec2 pivot)
{
    Assert(state < ObjectState_Count);
    LayoutPieceParams* params = piece->params + state;
    params->valid = true;
    params->parentOffset = parentOffset;
    params->parentAngle = parentAngle;
    params->scale = scale;
    params->alpha = alpha;
    params->pivot = pivot;
}

inline void AddIngredient(LayoutPiece* piece, char* name, u32 quantity)
{
    Assert(piece->ingredientCount < ArrayCount(piece->ingredientTaxonomies));
    TaxonomySlot* ingredientSlot = NORUNTIMEGetTaxonomySlotByName(taxTable_, name);
    if(ingredientSlot)
    {
		u32 ingredientIndex = piece->ingredientCount++;
        piece->ingredientTaxonomies[ingredientIndex] = ingredientSlot->taxonomy;
		piece->ingredientQuantities[ingredientIndex] = quantity;
    }
    else
    {
        EditorErrorLog(name);
    }
}



#ifdef FORG_SERVER

global_variable AIBehavior* currentBehavior_;
global_variable AIAction* currentAction_;
global_variable Consideration* currentConsideration_;


inline void BeginBehavior(char* name)
{
    TaxonomySlot* slot = NORUNTIMEGetTaxonomySlotByName(taxTable_, name);
    
    if(slot)
    {
        FREELIST_FREE(slot->behaviorContent, AIBehavior, taxTable_->firstFreeAIBehavior);
        Assert(IsBehavior(taxTable_, slot->taxonomy));
        TAXTABLE_ALLOC(slot->behaviorContent, AIBehavior);
        currentBehavior_ = slot->behaviorContent;
    }
    else
    {
        currentBehavior_ = 0;
        EditorErrorLog(name);
    }
}

inline void AddAction(EntityAction todo, char* targetCriteria = 0, r32 importance = 1.0f)
{
    if(currentBehavior_)
    {
        Assert(currentBehavior_->actionCount < ArrayCount(currentBehavior_->actions));
        AIAction* action = currentBehavior_->actions + currentBehavior_->actionCount++;
        action->type = AIAction_Command;
        action->command.action = todo;
        
        if(targetCriteria)
        {
            TaxonomySlot* targetSlot = NORUNTIMEGetTaxonomySlotByName(taxTable_, targetCriteria);
            
            if(targetSlot)
            {
                action->associatedConcept = targetSlot->taxonomy;
            }
            else
            {
                EditorErrorLog(targetCriteria);
            }
        }
        action->importance = importance;
        currentAction_ = action;
    }
}

inline void AddAction(char* behaviorName, r32 importance = 1.0f)
{
    TaxonomySlot* behaviorSlot = NORUNTIMEGetTaxonomySlotByName(taxTable_, behaviorName);
    
    if(behaviorSlot)
    {
        if(currentBehavior_)
        {
            Assert(currentBehavior_->actionCount < ArrayCount(currentBehavior_->actions));
            AIAction* action = currentBehavior_->actions + currentBehavior_->actionCount++;
            action->type = AIAction_Behavior;
            action->behaviorTaxonomy = behaviorSlot->taxonomy;
            
            action->importance = importance;
            currentAction_ = action;
        }
    }
    else
    {
        currentAction_ = 0;
        EditorErrorLog(behaviorName);
    }
}

inline void AssociateBehavior(char* referenceName, char* specificName, b32 isStartingBlock = false)
{
    TaxonomyBehavior* behavior;
    TAXTABLE_ALLOC(behavior, TaxonomyBehavior);
    TaxonomySlot* referenceSlot = NORUNTIMEGetTaxonomySlotByName(taxTable_, referenceName);
    TaxonomySlot* blockSlot = NORUNTIMEGetTaxonomySlotByName(taxTable_, specificName);
    
    if(referenceSlot)
    {
        if(blockSlot)
        {
            behavior->referenceTaxonomy = referenceSlot->taxonomy;
            behavior->specificTaxonomy = blockSlot->taxonomy;
            
            FREELIST_INSERT(behavior, currentSlot_->firstPossibleBehavior);
            
            if(isStartingBlock)
            {
                currentSlot_->startingBehavior = behavior;
            }
        }
        else
        {
            EditorErrorLog(specificName);
        }
    }
    else
    {
        EditorErrorLog(referenceName);
    }
}

inline void DefineConsideration(char* considerationName, char* expression)
{
    TaxonomySlot* slot = NORUNTIMEGetTaxonomySlotByName(taxTable_, considerationName);
    if(slot)
    {
        Assert(!slot->consideration);
        TAXTABLE_ALLOC(slot->consideration, TaxonomyConsideration);
        
        StrCpy(expression, StrLen(expression), slot->consideration->expression, ArrayCount(slot->consideration->expression));
    }
    else
    {
        EditorErrorLog(considerationName);
    }
}

inline ResponseCurve Gaussian()
{
    ResponseCurve result = {};
    return result;
}

inline void AddConsideration(char* name, r32 bookEndMin, r32 bookEndMax, ResponseCurve curve)
{
    TaxonomySlot* considerationSlot = NORUNTIMEGetTaxonomySlotByName(taxTable_, name);
    if(considerationSlot)
    {
        if(currentAction_)
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
    }
    else
    {
        currentConsideration_ = 0;
        EditorErrorLog(name);
    }
}


inline void AddBooleanConsideration(char* name)
{
    AddConsideration(name, 0, 1, Gaussian());
}

#define AddParam(value) AddParam_(ExpressionVal(value))
inline void AddParam_(ExpressionValue value)
{
    if(currentConsideration_)
    {
        ConsiderationParams* params = &currentConsideration_->params;
        AddParam_(params, value);
    }
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


global_variable MemCriteria* currentMemCriteria_;
global_variable TaxonomyTree* currentSynthTree_;
global_variable TaxonomyNode* currentSynthNode_;
global_variable MemSynthOption* currentSynthOption_;

inline void BeginMemoryBehavior(char* behaviorName)
{
    TaxonomySlot* slot = NORUNTIMEGetTaxonomySlotByName(taxTable_, behaviorName);
    
    if(slot)
    {
        FREELIST_FREE(slot->criteria, MemCriteria, taxTable_->firstFreeMemCriteria);
        
        for(MemSynthesisRule* rule = slot->synthRules; rule; rule = rule->next)
        {
            FreeMemSynthRuleTree(taxTable_, rule->tree.root);
        }
        FREELIST_FREE(slot->synthRules, MemSynthesisRule, taxTable_->firstFreeMemSynthesisRule);
        
        Assert(IsBehavior(taxTable_, slot->taxonomy));
        currentSlot_ = slot;
    }
    else
    {
        EditorErrorLog(behaviorName);
    }
}

inline void AddCriteria(char* criteriaName)
{
    TaxonomySlot* criteriaTax = NORUNTIMEGetTaxonomySlotByName(taxTable_, criteriaName);
    if(criteriaTax)
    {
        MemCriteria* newCriteria;
        TAXTABLE_ALLOC(newCriteria, MemCriteria);
        newCriteria->taxonomy = criteriaTax->taxonomy;
        
        FREELIST_INSERT(newCriteria, currentSlot_->criteria);
        currentMemCriteria_ = newCriteria;
    }
    else
    {
        currentMemCriteria_ = 0;
        EditorErrorLog(criteriaName);
    }
}

inline void AddMemConsideration(char* requiredConceptName)
{
    TaxonomySlot* slot = NORUNTIMEGetTaxonomySlotByName(taxTable_, requiredConceptName);
    if(slot)
    {
        if(currentMemCriteria_)
        {
            Assert(currentMemCriteria_->possibleTaxonomiesCount < ArrayCount(currentMemCriteria_->requiredConceptTaxonomy));
            currentMemCriteria_->requiredConceptTaxonomy[currentMemCriteria_->possibleTaxonomiesCount++] = slot->taxonomy;
        }
    }
    else
    {
        EditorErrorLog(requiredConceptName);
    }
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
    if(target)
    {
        if(currentSynthTree_)
        {
            currentSynthNode_ = AddToTaxonomyTree(currentSynthTree_, target);
        }
    }
    else
    {
        EditorErrorLog(name);
    }
}

inline void AddOption(char* conceptName, u32 lastingtimeUnits, u32 refreshTimeUnits)
{
    TaxonomySlot* conceptSlot = NORUNTIMEGetTaxonomySlotByName(taxTable_, conceptName);
    
    if(conceptSlot)
    {
        MemSynthOption* option;
        TAXTABLE_ALLOC(option, MemSynthOption);
        option->lastingtimeUnits = SafeTruncateToU16(lastingtimeUnits);
        option->refreshTimeUnits = SafeTruncateToU16(refreshTimeUnits);
        option->outputConcept = conceptSlot->taxonomy;
        
        FREELIST_INSERT(option, currentSynthNode_->data.firstOption);
        currentSynthOption_ = option;
    }
    else
    {
        currentSynthOption_ = 0;
        EditorErrorLog(conceptName);
    }
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
    TaxonomySlot* behaviorSlot = NORUNTIMEGetTaxonomySlotByName(taxTable_, behaviorName);
    
    if(behaviorSlot)
    {
        TaxonomyMemBehavior* newBehavior;
        TAXTABLE_ALLOC(newBehavior, TaxonomyMemBehavior);
        newBehavior->taxonomy = behaviorSlot->taxonomy;
        
        FREELIST_INSERT(newBehavior, currentSlot_->firstMemBehavior);
    }
    else
    {
        EditorErrorLog(behaviorName);
    }
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
    if(toolSlot)
    {
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
    else
    {
        
        EditorErrorLog(toolName);
    }
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


inline char* OutputToBuffer(char* buffer, u32* bufferSize, u32 number)
{
    u32 written = (u32) FormatString(buffer, *bufferSize, "%d", number);
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
            if(!StrEqual(element->name, "empty"))
            {
                buffer = OutputToBuffer(buffer, bufferSize, "\"");
                buffer = OutputToBuffer(buffer, bufferSize, element->name);
                buffer = OutputToBuffer(buffer, bufferSize, "\"");
                
                if(element->versionNumber)
                {
                    buffer = OutputToBuffer(buffer, bufferSize, " *");
                    buffer = OutputToBuffer(buffer, bufferSize, element->versionNumber);
                }
                
                
                buffer = OutputToBuffer(buffer, bufferSize, " = ");
            }
            else
            {
                buffer = OutputToBuffer(buffer, bufferSize, element->name);
                buffer = OutputToBuffer(buffer, bufferSize, " = ");
            }
		}
        
		switch(element->type)
		{
			case EditorElement_String:
			{
                buffer = OutputToBuffer(buffer, bufferSize, " \"");
				buffer = OutputToBuffer(buffer, bufferSize, element->value);
                buffer = OutputToBuffer(buffer, bufferSize, "\" ");
			} break;
            
            case EditorElement_Text:
			{
                buffer = OutputToBuffer(buffer, bufferSize, " \"");
                buffer = OutputToBuffer(buffer, bufferSize, "$");
				buffer = OutputToBuffer(buffer, bufferSize, element->text->text);
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
                
                if(element->emptyElement)
                {
                    buffer = OutputToBuffer(buffer, bufferSize, "#");
                    buffer = WriteElements(buffer, bufferSize, element->emptyElement);
                }
                
                if(element->flags & EditorElem_RecursiveEmpty)
                {
                    buffer = OutputToBuffer(buffer, bufferSize, "#recursiveEmpty ");
                }
                
                if(element->flags & EditorElem_LabelsEditable)
                {
                    buffer = OutputToBuffer(buffer, bufferSize, "#editableLabels ");
                }
                
                if(element->flags & EditorElem_CantBeDeleted)
                {
                    buffer = OutputToBuffer(buffer, bufferSize, "#cantBeDeleted ");
                }
                
                if(element->flags & EditorElem_CantBeDeleted)
                {
                    buffer = OutputToBuffer(buffer, bufferSize, "#atLeastOneInList ");
                }
                
                if(element->flags & EditorElem_PlaySoundButton)
                {
                    buffer = OutputToBuffer(buffer, bufferSize, "#playSound ");
                }
                
                if(element->flags & EditorElem_PlayEventSoundButton)
                {
                    buffer = OutputToBuffer(buffer, bufferSize, "#playEventSound ");
                }
                
                if(element->flags & EditorElem_PlayEventButton)
                {
                    buffer = OutputToBuffer(buffer, bufferSize, "#playEvent ");
                }
                
                if(element->flags & EditorElem_PlayContainerButton)
                {
                    buffer = OutputToBuffer(buffer, bufferSize, "#playContainer ");
                }
                
                if(element->flags & EditorElem_EquipInAnimationButton)
                {
                    buffer = OutputToBuffer(buffer, bufferSize, "#equipInAnimation ");
                }
                
                if(element->flags & EditorElem_ShowLabelBitmapButton)
                {
                    buffer = OutputToBuffer(buffer, bufferSize, "#showBitmap ");
                }
                
                if(element->elementName[0])
                {
                    char outputElementName[64];
                    FormatString(outputElementName, sizeof(outputElementName), "#elementName = %s ", element->elementName);
                    buffer = OutputToBuffer(buffer, bufferSize, outputElementName);
                }
                
                if(element->labelName[0])
                {
                    char outputLabelName[64];
                    FormatString(outputLabelName, sizeof(outputLabelName), "#labelName = \"%s\" ", element->labelName);
                    buffer = OutputToBuffer(buffer, bufferSize, outputLabelName);
                }
                
                buffer = WriteElements(buffer, bufferSize, element->firstInList);
				buffer = OutputToBuffer(buffer, bufferSize, ") ");
			} break;
            
			case EditorElement_Struct:
			{
                
				buffer = OutputToBuffer(buffer, bufferSize, " {");
                
                if(element->flags & EditorElem_Union)
                {
                    buffer = OutputToBuffer(buffer, bufferSize, "#union ");
                }
                
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

inline void AddStructParams(Tokenizer* tokenizer, EditorElement* element)
{
    
    b32 parsingParams = true;
    while(parsingParams)
    {
        if(NextTokenIs(tokenizer, Token_Pound))
        {
            Token pound = GetToken(tokenizer);
            Token paramName = GetToken(tokenizer);
            
            if(TokenEquals(paramName, "union"))
            {
                element->flags |= EditorElem_Union;
            }
            else
            {
                InvalidCodePath;
            }
        }
        else
        {
            parsingParams = false;
        }
    }
}

enum LoadElementsMode
{
    LoadElements_Tab,
    LoadElements_Asset,
};

inline EditorElement* LoadElementsInMemory(LoadElementsMode mode, Tokenizer* tokenizer, b32* end)
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
        
        if(NextTokenIs(tokenizer, Token_Asterisk))
        {
            Token ast = GetToken(tokenizer);
            Token version = GetToken(tokenizer);
            newElement->versionNumber = atoi(version.text);
        }
        
		if(RequireToken(tokenizer, Token_EqualSign))
		{
			Token t = GetToken(tokenizer);
			switch(t.type)
			{
				case Token_String:
				{
					Token value = Stringize(t);
                    
                    if(value.text[0] == '$')
                    {
                        newElement->type = EditorElement_Text;
                        EditorTextBlock* text;
                        FREELIST_ALLOC(text, taxTable_->firstFreeEditorText, PushStruct(taxPool_, EditorTextBlock));
                        StrCpy(value.text + 1, value.textLength - 1, text->text, sizeof(text->text));
                        
                        newElement->text = text;
                    }
                    else
                    {
                        newElement->type = EditorElement_String;
                        StrCpy(value.text, value.textLength, newElement->value, sizeof(newElement->value));
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
                            if(TokenEquals(paramName, "empty"))
                            {
                                if(RequireToken(tokenizer, Token_EqualSign))
                                {
                                    newElement->emptyElement = LoadElementsInMemory(mode, tokenizer, end);
                                    FormatString(newElement->emptyElement->name, sizeof(newElement->emptyElement->name), "empty");
                                }
                            }
                            else if(TokenEquals(paramName, "recursiveEmpty"))
                            {
                                newElement->flags |= EditorElem_RecursiveEmpty;
                            }
                            else if(TokenEquals(paramName, "editableLabels"))
                            {
                                newElement->flags |= EditorElem_LabelsEditable;
                            }
							else if(TokenEquals(paramName, "cantBeDeleted"))
							{
								newElement->flags |= EditorElem_CantBeDeleted;
							}
                            else if(TokenEquals(paramName, "atLeastOneInList"))
							{
								newElement->flags |= EditorElem_AtLeastOneInList;
							}
							else if(TokenEquals(paramName, "playSound"))
							{
								newElement->flags |= EditorElem_PlaySoundButton;
							}
                            else if(TokenEquals(paramName, "playEventSound"))
							{
								newElement->flags |= EditorElem_PlayEventSoundButton;
							}
                            else if(TokenEquals(paramName, "playEvent"))
							{
								newElement->flags |= EditorElem_PlayEventButton;
							}
                            else if(TokenEquals(paramName, "playContainer"))
							{
								newElement->flags |= EditorElem_PlayContainerButton;
							}
                            else if(TokenEquals(paramName, "equipInAnimation"))
                            {
                                newElement->flags |= EditorElem_EquipInAnimationButton;
                            }
                            else if(TokenEquals(paramName, "showBitmap"))
							{
								newElement->flags |= EditorElem_ShowLabelBitmapButton;
							}
                            else if(TokenEquals(paramName, "union"))
                            {
                                newElement->flags |= EditorElem_Union;
                            }
							else if(TokenEquals(paramName, "elementName"))
							{
								if(RequireToken(tokenizer, Token_EqualSign))
                                {
									Token elementName = GetToken(tokenizer);
                                    FormatString(newElement->elementName, sizeof(newElement->elementName), "%.*s", elementName.textLength, elementName.text);
                                }
							}
                            else if(TokenEquals(paramName, "labelName"))
							{
								if(RequireToken(tokenizer, Token_EqualSign))
                                {
									Token labelName = GetToken(tokenizer);
                                    
                                    if(labelName.type == Token_String)
                                    {
                                        labelName = Stringize(labelName);
                                    }
                                    FormatString(newElement->labelName, sizeof(newElement->labelName), "%.*s", labelName.textLength, labelName.text);
                                }
							}
                            else
                            {
                                InvalidCodePath;
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
                        newElement->firstInList = LoadElementsInMemory(mode, tokenizer, end);
                        if(!RequireToken(tokenizer, Token_CloseParen))
                        {
                            InvalidCodePath;
                        }
                    }
                } break;
                
                case Token_OpenBraces:
                {
                    newElement->type = EditorElement_Struct;
                    
                    AddStructParams(tokenizer, newElement);
                    
                    if(NextTokenIs(tokenizer, Token_CloseBraces))
                    {
                        newElement->firstValue = 0;
                        Token closed = GetToken(tokenizer);
                    }
                    else
                    {
                        newElement->firstValue = LoadElementsInMemory(mode, tokenizer, end);
                        
                        if(!RequireToken(tokenizer, Token_CloseBraces))
                        {
                            InvalidCodePath;
                        }
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
            
            AddStructParams(tokenizer, newElement);
            
            newElement->firstValue = LoadElementsInMemory(mode, tokenizer, end);
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
            newElement->next = LoadElementsInMemory(mode, tokenizer, end);
        }
    }
    else if(NextTokenIs(tokenizer, Token_EndOfFile))
    {
        *end = true;
    }
    
    return newElement;
}

inline void FreeElement(EditorElement* element, b32 freeNext = true)
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
            
            
            case EditorElement_Text:
            {
                FREELIST_DEALLOC(element->text, taxTable_->firstFreeEditorText);
            } break;
            
            case EditorElement_List:
            {
                FreeElement(element->firstInList, freeNext);
            } break;
            
            case EditorElement_Struct:
            {
                FreeElement(element->firstValue, freeNext);
            } break;
            
            case EditorElement_Taxonomy:
            {
                FreeElement(element->firstChild, freeNext);
            } break;
            
            case EditorElement_EmptyTaxonomy:
            {
            } break;
            
            InvalidDefaultCase;
        }
        
        if(freeNext)
        {
            if(element->next)
            {
                FreeElement(element->next, freeNext);
            }
            
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
        result = (role & EditorRole_GameDesigner);
    }
    
    return result;
}

internal void LoadFileInTaxonomySlot(char* content, u32 editorRoles)
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
    
    
    if(content[0])
    {
        b32 end = false;
        while(true)
        {
            Assert(currentSlot_->tabCount < ArrayCount(currentSlot_->tabs));
            
            EditorTab* newTab = currentSlot_->tabs + currentSlot_->tabCount++;
            newTab->root = LoadElementsInMemory(LoadElements_Tab, &tokenizer, &end);
            newTab->editable = IsEditableByRole(newTab->root, editorRoles);
            
            if(end)
            {
                break;
            }
        }
    }
    else
    {
        InvalidCodePath;
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
    if(element && element->type == EditorElement_Struct)
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

inline NoiseParams ParseNoiseParams(EditorElement* element)
{
    r32 frequency = 1.0;
    u32 octaves = 1;
    r32 offset = 0;
    r32 amplitude = 1.0f;
    if(element)
    {
        frequency = ToR32(GetValue(element, "frequency"), 1.0f);
        octaves = ToU32(GetValue(element, "octaves"), 1);
        offset = ToR32(GetValue(element, "offset"));
        amplitude = ToR32(GetValue(element, "amplitude"), 1.0f);
        
    }
    
    NoiseParams result = NoisePar(frequency, octaves, offset, amplitude);
    
    return result;
}

#ifndef FORG_SERVER
inline void AddSoundAndChildContainersRecursively(SoundContainer* rootContainer, EditorElement* root)
{
    char* type = GetValue(root, "type");
    if(type)
    {
        rootContainer->type = (SoundContainerType) GetValuePreprocessor(SoundContainerType, type);
    }
    else
    {
        InvalidCodePath;
    }
    
	EditorElement* containerLabels = GetList(root, "labels");
	while(containerLabels)
	{
        char* labelName = GetValue(containerLabels, "name");
        char* labelValue = GetValue(containerLabels, "value");
        
        AddSoundLabel(rootContainer, labelName, labelValue);
		containerLabels = containerLabels->next;
	}
    
    EditorElement* sounds = GetList(root, "sounds");
    while(sounds)
    {
        char* soundType = GetValue(sounds, "soundType");
        char* soundName = GetValue(sounds, "sound");
        
        r32 delay = 0;
        r32 decibelOffset = 0;
        r32 pitch = 1;
        r32 toleranceDistance = 1;
        r32 distanceFalloffCoeff = 1;
        EditorElement* params = GetList(sounds, "params");
        while(params)
        {
            if(StrEqual(params->name, "delay"))
            {
                delay = ToR32(GetValue(params, "value"), 0);
            }
            else if(StrEqual(params->name, "decibelOffset"))
            {
                decibelOffset = ToR32(GetValue(params, "value"), 0);
            }
            else if(StrEqual(params->name, "pitch"))
            {
                pitch = ToR32(GetValue(params, "value"), 1);
            }
            else if(StrEqual(params->name, "toleranceDistance"))
            {
                toleranceDistance = ToR32(GetValue(params, "value"), 1);
            }
            else if(StrEqual(params->name, "distanceFalloffCoeff"))
            {
                distanceFalloffCoeff = ToR32(GetValue(params, "value"), 1);
            }
            
            params = params->next;
        }
        
        if(soundType && soundName)
        {
            LabeledSound* sound = AddSoundToContainer(rootContainer, soundType, soundName, delay, decibelOffset, pitch, toleranceDistance, distanceFalloffCoeff);
			EditorElement* soundLabels = GetList(sounds, "labels");
			while(soundLabels)
			{
                char* labelName = GetValue(soundLabels, "name");
                char* labelValue = GetValue(soundLabels, "value");
                
                AddSoundLabel(sound, labelName, labelValue);
				soundLabels = soundLabels->next;
			}
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

internal void FreeEquipmentTreeNodeRecursive(TaxonomyNode* root)
{
    if(root)
    {
        for(TaxonomyNode* child = root->firstChild; child;)
        {
            TaxonomyNode* next = child->next;
            FreeEquipmentTreeNodeRecursive(child);
            child = next;
        }
        root->firstChild = 0;
        
        if(root->data.equipmentMapping)
        {
            for(EquipmentLayout* layout = root->data.equipmentMapping->firstEquipmentLayout; layout; layout = layout->next)
            {
                FREELIST_FREE(layout->firstEquipmentAss, EquipmentAss, taxTable_->firstFreeEquipmentAss);
                layout->firstEquipmentAss = 0;
            }
            
            
            FREELIST_FREE(root->data.equipmentMapping->firstEquipmentLayout, EquipmentLayout,  taxTable_->firstFreeEquipmentLayout);
            root->data.equipmentMapping->firstEquipmentLayout = 0;
            
            FREELIST_DEALLOC(root->data.equipmentMapping, taxTable_->firstFreeEquipmentMapping);
        }
        root->data.equipmentMapping = 0;
        
        FREELIST_DEALLOC(root, taxTable_->firstFreeTaxonomyNode);
    }
}

inline void AddTileBucket(Selector* band, char* tileName, r32 temperature)
{
    TaxonomySlot* tileSlot = NORUNTIMEGetTaxonomySlotByName(taxTable_, tileName);
    if(tileSlot)
    {
        AddBucket(band, temperature, tileSlot->taxonomy);
    }
    else
    {
        EditorErrorLog(tileName);
    }
}

inline void ParsePlantLevelParams(PlantLevelParams* destParams, EditorElement* levelParams)
{
    if(levelParams)
    {
        destParams->curveRes = ToU8(GetValue(levelParams, "curveRes"));
        destParams->curveBack = ToR32(GetValue(levelParams, "curveBack"));
        destParams->curve = ToR32(GetValue(levelParams, "curve"));
        destParams->curveV = ToR32(GetValue(levelParams, "curveV"));
        
        destParams->segSplits = ToR32(GetValue(levelParams, "segSplits"));
        destParams->baseSplits = ToR32(GetValue(levelParams, "baseSplits"));
        
        destParams->splitAngle = ToR32(GetValue(levelParams, "splitAngle"));
        destParams->splitAngleV = ToR32(GetValue(levelParams, "splitAngleV"));
        
        destParams->branches = ToR32(GetValue(levelParams, "branches"));
        destParams->branchesV = ToR32(GetValue(levelParams, "branchesV"));
        destParams->downAngle = ToR32(GetValue(levelParams, "downAngle"));destParams->downAngleV = ToR32(GetValue(levelParams, "downAngleV"));
        destParams->rotate = ToR32(GetValue(levelParams, "rotate"));destParams->rotateV = ToR32(GetValue(levelParams, "rotateV"));
        destParams->lengthCoeff = ToR32(GetValue(levelParams, "lengthCoeff"));
        destParams->lengthCoeffV = ToR32(GetValue(levelParams, "lengthCoeffV"));
        destParams->taper = ToR32(GetValue(levelParams, "taper"));
        destParams->radiousMod = ToR32(GetValue(levelParams, "radiousMod"), 1.0f);
        destParams->clonePercRatio = ToR32(GetValue(levelParams, "clonePercRatio"), 0.5f);
        destParams->clonePercRatioV = ToR32(GetValue(levelParams, "clonePercRatioV"), 0.0f);
        
        destParams->baseYoungColor = ToV4Color(GetStruct(levelParams, "baseYoungColor"));
        destParams->topYoungColor = ToV4Color(GetStruct(levelParams, "topYoungColor"));
        destParams->baseOldColor = ToV4Color(GetStruct(levelParams, "baseOldColor"));
        destParams->topOldColor = ToV4Color(GetStruct(levelParams, "topOldColor"));
        
        destParams->radiousIncreaseSpeed = ToR32(GetValue(levelParams, "radiousIncreaseSpeed"));
        destParams->lengthIncreaseSpeed = ToR32(GetValue(levelParams, "lengthIncreaseSpeed"));
        
        destParams->leafCount = Min(MAX_LEAFS_PER_STEM, ToU8(GetValue(levelParams, "leafCount")));
        destParams->allLeafsAtStemLength = ToR32(GetValue(levelParams, "allLeafsAtStemLength"), 0.5f);
    }
    else
    {
        *destParams = {};
    }
}

inline void Spawn(Effect* effect, char* name)
{
    TaxonomySlot* spawnSlot = NORUNTIMEGetTaxonomySlotByName(taxTable_, name);
    if(spawnSlot)
    {
        if(effect)
        {
            effect->data.taxonomy = spawnSlot->taxonomy;
        }
    }
    else
    {
        EditorErrorLog(name);
    }
}

internal void ParseEffect(EditorElement* element, Effect* dest)
{
    
            char* action = GetValue(element, "action");
            char* effectName = GetValue(element, "effectID");
            char* minIntensity = GetValue(element, "minIntensity");
            char* maxIntensity = GetValue(element, "maxIntensity");
            
           
            dest->triggerAction = (EntityAction) GetValuePreprocessor(EntityAction, action);
            dest->ID = (EffectIdentifier) GetValuePreprocessor(EffectIdentifier, effectName);
            dest->minIntensity = ToR32(minIntensity, 1.0f);
            dest->maxIntensity = ToR32(maxIntensity, 1.0f);
            
  
            char* timer = GetValue(element, "timer");
            if(timer)
            {
                dest->targetTimer = ToR32(timer);
            }
            
            EditorElement* flagList = GetList(element, "flags");
            while(flagList)
            {
                char* flag = GetValue(flagList, "name");
                u32 flagValue = GetFlagPreprocessor(EffectFlags, flag);
                dest->flags |= flagValue;
                
                flagList = flagList->next;
            }
            
            EditorElement* range = GetStruct(element, "range");
            char* rangeType = GetValue(range, "rangeType");
            char* rangeValue = GetValue(range, "radious");
                dest->range.type = (EffectTargetRangeType) GetValuePreprocessor(EffectTargetRangeType, rangeType);
                dest->range.radious = ToR32(rangeValue, 0.0f);
                        
            dest->data = {};
            EditorElement* params = GetList(element, "effectParams");
            while(params)
            {
                char* paramName = GetValue(params, "effectParamName");
                if(StrEqual(paramName, "velocity"))
                {
                    dest->data.speed = ToV3(GetElement(params, "velocity"));
                }
                else if(StrEqual(paramName, "offset"))
                {
                    dest->data.offset = ToV3(GetElement(params, "offset"));
                }
                else if(StrEqual(paramName, "offsetV"))
                {
                    dest->data.offsetV = ToV3(GetElement(params, "offsetV"));
                }
                else if(StrEqual(paramName, "taxonomyName"))
                {
                    char* taxonomy = GetValue(params, "taxonomyName");
                    Spawn(dest, taxonomy);
                }
                else if(StrEqual(paramName, "timeToLive"))
                {
                    dest->data.timeToLive = ToR32(GetValue(params, "timeToLive"));
                }
                else if(StrEqual(paramName, "spawnCount"))
                {
                    dest->data.spawnCount = ToU32(GetValue(params, "spawnCount"));
                }
                
               
                params = params->next;
            }
}

internal void Import(TaxonomySlot* slot, EditorElement* root)
{
    currentSlot_ = slot;
    
    char* name = root->name;
    if(StrEqual(name, "bounds"))
    {
        char* exists = GetValue(root, "physical");
        b32 physical = ToB32(exists);
        
        char* height = GetValue(root, "height");
        char* radious = GetValue(root, "radious");
        
        if(physical)
        {
            DefineBounds(height, radious);
        }
        else
        {
            DefineNullBounds(height, radious);
        }
        
        
        slot->scaleDimBasedOnIntensity = ToB32(GetValue(root, "scaleDimBasedOnGenIntensity"));
        slot->scaleDimGenCoeffV = ToR32(GetValue(root, "scaleDimGenCoeff"));
    }
    else if(StrEqual(name, "equipmentMappings"))
    {
        FreeEquipmentTreeNodeRecursive(currentSlot_->equipmentMappings.root);
        currentSlot_->equipmentMappings.root = 0;
        
        EditorElement* singleSlot = root->firstInList;
        while(singleSlot)
        {
            char* equipmentName = GetValue(singleSlot, "equipment");
            EquipmentMapping* mapping = AddEquipmentMapping(equipmentName);
            
            EditorElement* layouts = GetList(singleSlot, "layouts");
            while(layouts)
            {
                char* layoutName = GetValue(layouts, "layoutName");
                char* slotName = GetValue(layouts, "slot");
                
                EquipmentLayout* equipmentLayout;
                TAXTABLE_ALLOC(equipmentLayout, EquipmentLayout);
                equipmentLayout->layoutHashID = StringHash(layoutName);
                equipmentLayout->slot = {(SlotName) GetValuePreprocessor(SlotName, slotName)};
                
                FREELIST_INSERT(equipmentLayout, mapping->firstEquipmentLayout);
                
                EditorElement* pieces = GetList(layouts, "pieces");
                while(pieces)
                {
                    u32 assIndex = ToU32(GetValue(pieces, "assIndex"));
                    
                    
                    char* pieceName = GetValue(pieces, "pieceName");
                    u8 index = ToU8(GetValue(pieces, "index"));
                    
                    if(StrEqual(pieceName, "all"))
                    {
                        index = 0xff;
                    }
                    Vec2 assOffset = ToV2(GetStruct(pieces, "assOffset"));
                    r32 zOffset = ToR32(GetValue(pieces, "zOffset"));
                    r32 angle = ToR32(GetValue(pieces, "angle"));
                    Vec2 scale = ToV2(GetStruct(pieces, "scale"));
                    
                    AddPiece(equipmentLayout, assIndex, pieceName, index, assOffset, zOffset, angle, scale);
                    
                    pieces = pieces->next;
                }
                
                
                layouts = layouts->next;
            }
            
            singleSlot = singleSlot->next;
        }
    }
    
    
    else if(StrEqual(name, "consumeMappings"))
    {
        FREELIST_FREE(currentSlot_->firstConsumeMapping, ConsumeMapping, taxTable_->firstFreeConsumeMapping);
        EditorElement* consume = root->firstInList;
        while(consume)
        {
            char* actionName = GetValue(consume, "action");
            char* objectName = GetValue(consume, "object");
            CanConsume(actionName, objectName);
            
            consume = consume->next;
        }
    }
    
    
    else if(StrEqual(name, "neededTools"))
    {
        for(u32 toolIndex = 0; toolIndex < ArrayCount(currentSlot_->neededToolTaxonomies); ++toolIndex)
        {
            currentSlot_->neededToolTaxonomies[toolIndex] = 0;
        }
        EditorElement* tools = root->firstInList;
        while(tools)
        {
            char* toolName = tools->name;
            Requires(toolName);
            tools = tools->next;
        }
    }
    else if(StrEqual(name, "requireEssences"))
    {
        FREELIST_FREE(currentSlot_->essences, TaxonomyEssence, taxTable_->firstFreeTaxonomyEssence);
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
        FREELIST_FREE(currentSlot_->essences, TaxonomyEssence, taxTable_->firstFreeTaxonomyEssence);
        EditorElement* essences = root->firstInList;
        
        while(essences)
        {
            char* essenceName = GetValue(essences, "essence");
            char* quantity = GetValue(essences, "quantity");
            
            AddEssence(essenceName, ToU8(quantity));
            
            essences = essences->next;
        }
    }
    else if(StrEqual(name, "skill"))
    {
        r32 distance = ToR32(GetValue(root, "distance"), 1.0f);
        currentSlot_->skillDistanceAllowed = distance;
        currentSlot_->cooldown = ToR32(GetValue(root, "cooldown"), 0.0f);
        
        currentSlot_->turningPointLevel = ToU32(GetValue(root, "turningPointLevel"), 0);
        currentSlot_->maxLevel = ToU32(GetValue(root, "maxLevel"), 100);
        
        currentSlot_->radixExponent = ToR32(GetValue(root, "radixExponent"), 2.0f);
        currentSlot_->exponentiationExponent = Max(ToR32(GetValue(root, "exponentiationExponent"), 2.0f), 0.0f);
        currentSlot_->radixLerping = ToR32(GetValue(root, "radixLerping"), 0.5f);
        currentSlot_->exponentiationLerping = Max(ToR32(GetValue(root, "exponentiationLerping"), 0.5f), 0.0f);
        
        char* passive = GetValue(root, "passive");
        if(passive)
        {
            InvalidCodePath;
            IsPassive();
        }
        
    }
    else if(StrEqual(name, "effects"))
    {
        FREELIST_FREE(currentSlot_->firstEffect, TaxonomyEffect, taxTable_->firstFreeTaxonomyEffect);
        EditorElement* effectList = root->firstInList;
        while(effectList)
        {
            TaxonomyEffect* newEffect;
            TAXTABLE_ALLOC(newEffect, TaxonomyEffect);            
            FREELIST_INSERT(newEffect, currentSlot_->firstEffect);
            ParseEffect(effectList, &newEffect->effect);    
            effectList = effectList->next;
        }
    }
    else if(StrEqual(name, "freeHandsRequirements"))
    {
        FREELIST_FREE(currentSlot_->nakedHandReq, NakedHandReq, taxTable_->firstFreeNakedHandReq);
        EditorElement* freeHandsReq = root->firstInList;
        while(freeHandsReq)
        {
            char* slotName = GetValue(freeHandsReq, "slot");
            char* taxonomy = GetValue(freeHandsReq, "name");
            
            AddFreeHandReq(slotName, taxonomy);
            
            freeHandsReq = freeHandsReq->next;
        }
    }
    else if(StrEqual(name, "craftingEffects"))
    {
        FREELIST_FREE(currentSlot_->links, CraftingEffectLink, taxTable_->firstFreeCraftingEffectLink);
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
        for(u32 attributeIndex = 0; attributeIndex < ArrayCount(currentSlot_->attributeHashmap); ++attributeIndex)
        {
            currentSlot_->attributeHashmap[attributeIndex] = {};
        }
        EditorElement* attributes = root->firstInList;
        
        while(attributes)
        {
            char* attributeName = GetValue(attributes, "name");
            char* value = GetValue(attributes, "value");
            
            SaveCreatureAttribute(attributeName, ToR32(value));
            
            attributes = attributes->next;
        }
    }
    
    else if(StrEqual(name, "possibleActions"))
    {
        for(PossibleAction* action = currentSlot_->firstPossibleAction; action; action = action->next)
        {
            FreeActionTree(taxTable_, action->tree.root);
        }
        
        FREELIST_FREE(currentSlot_->firstPossibleAction, PossibleAction, taxTable_->firstFreePossibleAction);
        
        EditorElement* actions = root->firstInList;
        while(actions)
        {
            char* action = GetValue(actions, "action");
            char* distance = GetValue(actions, "distance");
            
            BeginPossibleAction(action, distance);
            
            EditorElement* flags = GetList(actions, "flags");
            while(flags)
            {
                char* flagName = GetValue(flags, "name");
                AddActionFlag(flagName);
                
                flags = flags->next;
            }
            
            EditorElement* actors = GetList(actions, "actors");
            while(actors)
            {
                char* actorName = GetValue(actors, "name");
                char* requiredTime = GetValue(actors, "time");
                AddActor(actorName, requiredTime);
                
                actors = actors->next;
            }
            
            actions = actions->next;
        }
    }
    
    else if(StrEqual(name, "behaviors"))
    {
        FREELIST_FREE(currentSlot_->firstPossibleBehavior, TaxonomyBehavior, taxTable_->firstFreeTaxonomyBehavior);
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
        FREELIST_FREE(currentSlot_->firstMemBehavior, TaxonomyMemBehavior, taxTable_->firstFreeTaxonomyMemBehavior);
        EditorElement* behaviors = root->firstInList;
        while(behaviors)
        {
            char* behavior = GetValue(behaviors, "name");
            AddMemBehavior(behavior);
            
            behaviors = behaviors->next;
        }
    }
#endif
    else if(StrEqual(name, "container"))
    {
        u8 width = ToU8(GetValue(root, "width"));
        u8 height = ToU8(GetValue(root, "height"));
        
        InventorySpace(width, height);
        
        #if FORG_SERVER
        FREELIST_FREE(currentSlot_->firstInsideInteraction, TaxonomyContainerInteraction, taxTable_->firstFreeTaxonomyContainerInteraction);
        EditorElement* insideInteractions = GetList(root, "interactions");
        while(insideInteractions)
        {
            TaxonomyContainerInteraction* interaction;
            TAXTABLE_ALLOC(interaction, TaxonomyContainerInteraction);
            
            interaction->targetTime = ToR32(GetValue(root, "time"));
            
            EditorElement* ingredients = GetList(insideInteractions, "required");
            while(ingredients)
            {
                char* ingredient = GetValue(ingredients, "taxonomyName");
                TaxonomySlot* ingredientSlot = NORUNTIMEGetTaxonomySlotByName(taxTable_, ingredient);
                
                if(ingredientSlot)
                {
                    if(interaction->requiredCount < ArrayCount(interaction->requiredTaxonomies))
                    {
                        interaction->requiredTaxonomies[interaction->requiredCount++] = slot->taxonomy;
                    }
                }
                else
                {
                    EditorErrorLog(ingredient);
                }
                
                ingredients = ingredients->next;
            }
            
            EditorElement* effects = GetList(insideInteractions, "effects");
            while(effects)
            {
                if(interaction->effectCount < ArrayCount(interaction->effects))
                {
                    Effect* dest = interaction->effects + interaction->effectCount++;                    
                    ParseEffect(effects, dest);
                }
                
                effects = effects->next;
            }
            
            insideInteractions = insideInteractions->next;
        }
        #endif
    }
    else if(StrEqual(name, "tileParams"))
    {
        currentSlot_->groundPointMaxOffset = ToR32(GetValue(root, "groundPointMaxOffset"));
        currentSlot_->chunkynessWithSame = ToR32(GetValue(root, "chunkynessSame"), 0.5f);
        currentSlot_->chunkynessWithOther = ToR32(GetValue(root, "chunkynessOther"), 0.5f);
        currentSlot_->groundPointPerTile = ToR32(GetValue(root, "pointsPerTile"));
        currentSlot_->groundPointPerTileV = ToR32(GetValue(root, "pointsPerTileV"));
        currentSlot_->tileColor = ToV4Color(GetElement(root, "color"));
        currentSlot_->colorDelta = ToV4Color(GetElement(root, "colorDelta"), V4(0, 0, 0, 0));
        currentSlot_->tileBorderColor = ToV4Color(GetElement(root, "borderColor"), V4(0, 0, 0, 0));
        currentSlot_->tilePointsLayout = GetValuePreprocessor(TilePointsLayout, GetValue(root, "tileLayout"));
        currentSlot_->colorRandomness = ToR32(GetValue(root, "colorRandomness"), 0.0f);
        currentSlot_->tileNoise = ParseNoiseParams(GetStruct(root, "noise"));
    }
#ifndef FORG_SERVER
    else if(StrEqual(name, "visualLabels"))
    {
        FREELIST_FREE(currentSlot_->firstVisualLabel, VisualLabel, taxTable_->firstFreeVisualLabel);
        EditorElement* labels = root->firstInList;
        
        while(labels)
        {
            char* labelName = GetValue(labels, "name");
            char* value = GetValue(labels, "value");
            
            u64 ID = StringHash(labelName);
            r32 val = ToR32(value);
            AddLabel(ID, val);
            labels = labels->next;
        }
    }
    else if(StrEqual(name, "animationGeneralParams"))
    {
        currentSlot_->animationIn3d = ToB32(GetValue(root, "animationIn3d"));        
        currentSlot_->animationFollowsVelocity = ToB32(GetValue(root, "animationFollowsVelocity"));        
        currentSlot_->modelTypeID = StringHash(GetValue(root, "modelType"));
        currentSlot_->modelNameID = StringHash(GetValue(root, "modelName"));
        currentSlot_->modelOffset = ToV3(GetStruct(root, "offset"));
        currentSlot_->modelColoration = ToV4Color(GetStruct(root, "coloration"));
        currentSlot_->modelScale = ToV3(GetStruct(root, "scale"), V3(1, 1, 1));
    }
    else if(StrEqual(name, "skeleton"))
    {
        char* skeleton = GetValue(root, "skeletonName");
        char* skin = GetValue(root, "skinName");
        Vec4 color = ToV4Color(GetStruct(root, "defaultColoration"));
        Vec2 originOffset = ToV2(GetStruct(root, "originOffset"));
        UsesSkeleton(skeleton, skin, color, originOffset);
    }
    else if(StrEqual(name, "light"))
    {
        char* intensity = GetValue(root, "intensity");
        AddLight(ToR32(intensity), V3(1, 1, 1));
    }
    else if(StrEqual(name, "animationEffects"))
    {
        FREELIST_FREE(currentSlot_->firstAnimationEffect, AnimationEffect, taxTable_->firstFreeAnimationEffect);
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
        FREELIST_FREE(currentSlot_->firstSound, TaxonomySound, taxTable_->firstFreeTaxonomySound);
        EditorElement* effects = root->firstInList;
        while(effects)
        {
            char* animationName = GetValue(effects, "animationName");
            char* time = GetValue(effects, "time");
            char* event = GetValue(effects, "name");
            
            
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
        switch(root->versionNumber)
        {
            case 1:
            {
                taxTable_->eventCount = 0;
                EditorElement* events = root->firstInList;
                while(events)
                {
                    char* eventName = events->name;
                    SoundContainer* rootContainer = AddSoundEvent(eventName);
                    
                    AddSoundAndChildContainersRecursively(rootContainer, events);
                    events = events->next;
                }
            } break;
            
            InvalidDefaultCase;
        }
    }
    else if(StrEqual(name, "boneAlterations"))
    {
        FREELIST_FREE(currentSlot_->firstBoneAlteration, TaxonomyBoneAlterations, taxTable_->firstFreeTaxonomyBoneAlterations);
        EditorElement* alterations = root->firstInList;
        while(alterations)
        {
            char* boneIndex = GetValue(alterations, "boneIndex");
            EditorElement* scale = GetStruct(alterations, "scale");
            char* scaleX = GetValue(scale, "x");
            char* scaleY = GetValue(scale, "y");
            
            AddBoneAlteration(boneIndex, scaleX, scaleY);
            
            alterations = alterations->next;
        }
    }
    else if(StrEqual(name, "assAlterations"))
    {
        FREELIST_FREE(currentSlot_->firstAssAlteration, TaxonomyAssAlterations, taxTable_->firstFreeTaxonomyAssAlterations);
        
        EditorElement* alterations = root->firstInList;
        while(alterations)
        {
            char* assIndex = GetValue(alterations, "assIndex");
            EditorElement* scale = GetStruct(alterations, "scale");
            char* scaleX = GetValue(scale, "x");
            char* scaleY = GetValue(scale, "y");
            
            EditorElement* offset = GetStruct(alterations, "boneOffset");
            char* offsetX = GetValue(offset, "x");
            char* offsetY = GetValue(offset, "y");
            
			b32 specialColoration = ToB32(GetValue(alterations, "specialColoration"));
            Vec4 color = ToV4Color(GetStruct(alterations, "color"));
            
            AddAssAlteration(assIndex, scaleX, scaleY, offsetX, offsetY, specialColoration, color);
            
            alterations = alterations->next;
        }
    }
    else if(StrEqual(name, "icon"))
    {
        currentSlot_->iconColor = ToV4Color(GetStruct(root, "standardColor"));
        currentSlot_->iconActiveColor = ToV4Color(GetStruct(root, "activeColor"));
        currentSlot_->iconHoverColor = ToV4Color(GetStruct(root, "hoverColor"));
        
        currentSlot_->iconModelTypeID = StringHash(GetValue(root, "modelType"));
        currentSlot_->iconModelNameID = StringHash(GetValue(root, "modelName"));
        currentSlot_->iconScale = ToV3(GetStruct(root, "modelScale"));
    }
#endif
    else if(StrEqual(name, "rockDefinition"))
    {
        if(currentSlot_->rock)
        {
            if(currentSlot_->rock->firstPossibleMineral)
            {
                FREELIST_FREE(currentSlot_->rock->firstPossibleMineral, RockMineral,  taxTable_->firstFreeRockMineral);
            }
            
            FREELIST_DEALLOC(currentSlot_->rock, taxTable_->firstFreeRockDefinition);
            currentSlot_->rock = 0;
        }
        
        TAXTABLE_ALLOC(currentSlot_->rock, RockDefinition);
        RockDefinition* definition = currentSlot_->rock;
        
        definition->collides = ToB32(GetValue(root, "collides"));
        definition->modelTypeHash = StringHash(GetValue(root, "modelType"));
        definition->modelNameHash = StringHash(GetValue(root, "modelName"));
        definition->color = ToV4Color(GetStruct(root, "color"));
        definition->startingColorDelta = ToV4Color(GetStruct(root, "startingColorDelta"));
        definition->perVertexColorDelta = ToV4Color(GetStruct(root, "perVertexColorDelta"));
        definition->iterationCount = ToU32(GetValue(root, "iterations"));
        definition->minDisplacementY = ToR32(GetValue(root, "minDisplacementY"));
        definition->maxDisplacementY = ToR32(GetValue(root, "maxDisplacementY"));
        definition->minDisplacementZ = ToR32(GetValue(root, "minDisplacementZ"));
        definition->maxDisplacementZ = ToR32(GetValue(root, "maxDisplacementZ"));
        definition->smoothness = ToR32(GetValue(root, "smoothness"));
        definition->smoothnessDelta = ToR32(GetValue(root, "smoothnessDelta"));
        definition->scale = ToV3(GetStruct(root, "scale"));
        definition->scaleDelta = ToV3(GetStruct(root, "scaleDelta"));
        
        definition->percentageOfMineralVertexes = ToR32(GetValue(root, "percentageOfMineralVertexes"));
        definition->mineralCount = 0;
        EditorElement* minerals = GetList(root, "minerals");
        while(minerals)
        {
            RockMineral* mineral;
            TAXTABLE_ALLOC(mineral, RockMineral);
            
            mineral->lerp = ToR32(GetValue(minerals, "lerp"));
            mineral->lerpDelta = ToR32(GetValue(minerals, "lerpDelta"));
            mineral->color = ToV4Color(GetStruct(minerals, "color"));
            
            FREELIST_INSERT(mineral, definition->firstPossibleMineral);
            ++definition->mineralCount;
            
            minerals = minerals->next;
        }
        
        definition->renderingRocksCount = ToU32(GetValue(root, "renderingRocksCount"));
        definition->renderingRocksDelta = ToU32(GetValue(root, "renderingRocksDelta"));
        definition->renderingRocksRandomOffset = ToV3(GetStruct(root, "renderingRocksOffset"));
        definition->scaleRandomness = ToR32(GetValue(root, "scaleRandomness"));
        
    }
    else if(StrEqual(name, "plantDefinition"))
    {
        if(currentSlot_->plant)
        {
            TAXTABLE_DEALLOC(currentSlot_->plant, PlantDefinition);
        }
        TAXTABLE_ALLOC(currentSlot_->plant, PlantDefinition);
        PlantDefinition* plant = currentSlot_->plant;
        
        plant->collides = ToB32(GetValue(root, "collides"));
        plant->shape = (PlantShape) GetValuePreprocessor(PlantShape, GetValue(root, "shape"));
        
        plant->growingCoeff = ToR32(GetValue(root, "growingCoeff"), 1.0f);
        
        plant->plantCount = ToU32(GetValue(root, "plantCount"));
        plant->plantCountV = ToR32(GetValue(root, "plantCountV"));
        
        plant->plantOffsetV = ToV2(GetStruct(root, "plantOffsetV"));
        plant->plantAngleZV = ToR32(GetValue(root, "plantAngleZV"));
        
        plant->attractionUp = ToR32(GetValue(root, "attractionUp"));
        
        plant->maxLevels = Min(4, ToU8(GetValue(root, "levels"), 1));
        plant->baseSize = ToR32(GetValue(root, "baseSize"));
        plant->scale = ToR32(GetValue(root, "scale"));
        plant->scaleV = ToR32(GetValue(root, "scaleV"));
        plant->scale_0 = ToR32(GetValue(root, "scale_0"));
        plant->scaleV_0 = ToR32(GetValue(root, "scaleV_0"));
        plant->ratio = ToR32(GetValue(root, "ratio"));
        plant->ratioPower = ToR32(GetValue(root, "ratioPower"));
        plant->flare = ToR32(GetValue(root, "flare"));
        
        ParsePlantLevelParams(plant->levelParams + 0, GetStruct(root, "level0"));
        ParsePlantLevelParams(plant->levelParams + 1, GetStruct(root, "level1"));
        ParsePlantLevelParams(plant->levelParams + 2, GetStruct(root, "level2"));
        ParsePlantLevelParams(plant->levelParams + 3, GetStruct(root, "level3"));
        
        plant->leafColor = ToV4Color(GetStruct(root, "leafColor"));
        plant->leafColorV = ToV4Color(GetStruct(root, "leafColorV"));
        
        plant->leafDimSpeed = ToR32(GetValue(root, "leafDimSpeed"));
        plant->leafOffsetSpeed = ToR32(GetValue(root, "leafOffsetSpeed"));
        
        plant->leafScale = ToV2(GetStruct(root, "leafScale"));
        plant->leafScaleV = ToV2(GetStruct(root, "leafScaleV"));
        
        plant->leafOffsetV = ToV3(GetStruct(root, "leafOffsetV"));
        plant->leafAngleV = ToR32(GetValue(root, "leafAngleV"));
        
        plant->leafWindAngleV = ToR32(GetValue(root, "leafWindAngleV"));
        plant->leafWindDirectionV = ToR32(GetValue(root, "leafWindDirectionV"));
        
        plant->trunkColorV = ToV4Color(GetStruct(root, "trunkColorV"));
        plant->lobeDepth = ToR32(GetValue(root, "lobeDepth"));
        plant->lobes = ToR32(GetValue(root, "lobes"));
        
        plant->leafStringHash = StringHash(GetValue(root, "leafName"));
        plant->trunkStringHash = StringHash(GetValue(root, "trunkName"));
    }
    else if(StrEqual(name, "generatorParams"))
    {
        if(currentSlot_->generator)
        {
            
            for(TaxonomyTileAssociations* toFree = currentSlot_->generator->firstAssociation; toFree;)
            {
                TaxonomyTileAssociations* next = toFree->next;
                
                
                for(TaxonomyAssociation* assToFree = toFree->firstAssociation; assToFree;)
                {
                    TaxonomyAssociation* assNext = assToFree->next;
                    TAXTABLE_DEALLOC(assToFree, TaxonomyAssociation);
                    assToFree = assNext;
                }
                
                TAXTABLE_DEALLOC(toFree, TaxonomyTileAssociations);
                
                toFree = next;
            }
            
            
            FREELIST_DEALLOC(currentSlot_->generator, taxTable_->firstFreeWorldGenerator);
            currentSlot_->generator = 0;
        }
        TAXTABLE_ALLOC(currentSlot_->generator, WorldGenerator);
        WorldGenerator* generator = currentSlot_->generator;
        
        
        generator->landscapeNoise = ParseNoiseParams(GetStruct(root, "landscapeNoise"));
        generator->temperatureNoise = ParseNoiseParams(GetStruct(root, "temperatureNoise"));
        generator->precipitationNoise = ParseNoiseParams(GetStruct(root, "precipitationNoise"));
        generator->elevationNoise = ParseNoiseParams(GetStruct(root, "elevationNoise"));
        generator->elevationPower = ToR32(GetValue(root, "elevationPower"), 1.0f);
        generator->beachThreesold = ToR32(GetValue(root, "beachThreesold"), 0.05f);
        
        generator->beachTaxonomy = 0;
        char* beachTaxonomy = GetValue(root, "beachTaxonomy");
        if(beachTaxonomy)
        {
            TaxonomySlot* beachSlot = NORUNTIMEGetTaxonomySlotByName(taxTable_, beachTaxonomy);
            if(beachSlot)
            {
                generator->beachTaxonomy = beachSlot->taxonomy;
            }
        }
        
        generator->landscapeSelect = {};
        generator->temperatureSelect = {};
        EditorElement* landscape = GetList(root, "landscapes");
        while(landscape)
        {
            r32 threesold = ToR32(GetValue(landscape, "threesold"));
            
            NoiseParams noise = ParseNoiseParams(GetStruct(landscape, "noiseParams"));
            AddBucket(&generator->landscapeSelect, threesold, noise);
            
            
            r32 minTemperature = ToR32(GetValue(landscape, "minTemperature"));
            r32 maxTemperature = ToR32(GetValue(landscape, "maxTemperature"));
            AddBucket(&generator->temperatureSelect, threesold,MinMax(minTemperature, maxTemperature)); 
            
            landscape = landscape->next;
        }
        
        generator->biomePyramid = {};
        
        EditorElement* precipitationBand = GetList(root, "precipitationBands");
        while(precipitationBand)
        {
            r32 threesold = ToR32(GetValue(precipitationBand, "threesold"));
            Selector* band = AddSelectorForDryness(&generator->biomePyramid, threesold);
            
            EditorElement* tiles = GetList(precipitationBand, "tileTypes");
            while(tiles)
            {
                r32 temperature = ToR32(GetValue(tiles, "temperature"));
                
                AddTileBucket(band, tiles->name, temperature);
                
                tiles = tiles->next;
            }
            
            precipitationBand = precipitationBand->next;
        }
        
        
        EditorElement* tileAssociations = GetList(root, "tileAssociations");
        while(tileAssociations)
        {
            char* tileName = GetValue(tileAssociations, "tileType");
            TaxonomySlot* tileSlot = NORUNTIMEGetTaxonomySlotByName(taxTable_, tileName);
            
            if(tileSlot)
            {
                TaxonomyTileAssociations* tileAssoc;
                TAXTABLE_ALLOC(tileAssoc, TaxonomyTileAssociations);
                
                tileAssoc->taxonomy = tileSlot->taxonomy;
                tileAssoc->totalWeight = 0;
                tileAssoc->firstAssociation = 0;
                
                
                EditorElement* taxonomyAssociations = GetList(tileAssociations, "taxonomies");
                while(taxonomyAssociations)
                {
                    char* taxonomyName = GetValue(taxonomyAssociations, "taxonomyName");
                    r32 weight = ToR32(GetValue(taxonomyAssociations, "weight"));
                    
                    TaxonomySlot* taxonomySlot = NORUNTIMEGetTaxonomySlotByName(taxTable_, taxonomyName);
                    
                    if(taxonomySlot)
                    {
                        TaxonomyAssociation* ass;
                        TAXTABLE_ALLOC(ass, TaxonomyAssociation);
                        
                        ass->taxonomy = taxonomySlot->taxonomy;
                        ass->weight = weight;
                        
                        tileAssoc->totalWeight += weight;
                        FREELIST_INSERT(ass, tileAssoc->firstAssociation);
                        
                    }
                    else
                    {
                        EditorErrorLog(tileName);
                    }
                    
                    taxonomyAssociations = taxonomyAssociations->next;
                }
                
                
                FREELIST_INSERT(tileAssoc, currentSlot_->generator->firstAssociation);
                
            }
            else
            {
                EditorErrorLog(tileName);
            }
            
            tileAssociations = tileAssociations->next;
        }
    }
    else if(StrEqual(name, "layouts"))
    {
        for(ObjectLayout* toDelete = currentSlot_->firstLayout; toDelete; toDelete = toDelete->next)
        {
            FREELIST_FREE(toDelete->firstPiece, LayoutPiece, taxTable_->firstFreeLayoutPiece);
        }
        FREELIST_FREE(currentSlot_->firstLayout, ObjectLayout, taxTable_->firstFreeObjectLayout);
        currentSlot_->layoutCount = 0;
        
        
        
        EditorElement* layouts = root->firstInList;
        while(layouts)
        {
            ObjectLayout* newLayout;
            TAXTABLE_ALLOC(newLayout, ObjectLayout);
            
            newLayout->nameHashID = StringHash(layouts->name);
            FormatString(newLayout->name, sizeof(newLayout->name), "%s", layouts->name);
            
            EditorElement* pieces = GetList(layouts, "pieces");
            while(pieces)
            {
                char* pieceName = GetValue(pieces, "component");
                u64 pieceHash = StringHash(pieceName);
                
				u8 index = 0;
				for(LayoutPiece* test = newLayout->firstPiece; test; test = test->next)
				{
					if(test->componentHashID == pieceHash)
					{
						++index;
					}
				}
                LayoutPiece* piece = AddLayoutPiece(newLayout, pieceName, index);
                
                
                for(u32 state = ObjectState_Default; state < ObjectState_Count; ++state)
                {
                    piece->params[state].valid = false;
                }
                EditorElement* params = GetList(pieces, "params");
                while(params)
                {
                    ObjectState type = (ObjectState) GetValuePreprocessor(ObjectState, GetValue(params, "objectState"));
                    EditorElement* offset = GetStruct(params, "offset");
                    r32 x = ToR32(GetValue(offset, "x"));
                    r32 y = ToR32(GetValue(offset, "y"));
                    r32 z = ToR32(GetValue(offset, "z"));
                    
                    r32 angle = ToR32(GetValue(params, "angle"));
                    
                    EditorElement* scale = GetStruct(params, "scale");
                    r32 scaleX = ToR32(GetValue(scale, "x"));
                    r32 scaleY = ToR32(GetValue(scale, "y"));
                    r32 pieceAlpha = ToR32(GetValue(params, "alpha"));
                    Vec2 pivot = ToV2(GetStruct(params, "pivot"), V2(0.5f, 0.5f));
                    
                    AddLayoutPieceParams(piece, type, V3(x, y, z), angle, V2(scaleX, scaleY), pieceAlpha, pivot);
                    
                    params = params->next;
                }
                
                EditorElement* ingredient = GetList(pieces, "ingredients");
                while(ingredient)
                {
                    char* ingredientName = GetValue(ingredient, "ingredient");
                    u32 quantity = ToU32(GetValue(ingredient, "quantity"));
                    AddIngredient(piece, ingredientName, quantity);
                    
                    ingredient = ingredient->next;
                }
                
                EditorElement* decorationPieces = GetList(pieces, "childPieces");
                while(decorationPieces)
                {
                    char* childName = GetValue(decorationPieces, "component");
                    u64 childHash = StringHash(childName);
                    
                    u8 childIndex = 0;
                    for(LayoutPiece* test = newLayout->firstPiece; test; test = test->next)
                    {
                        if(test->componentHashID == childHash)
                        {
                            ++childIndex;
                        }
                    }
                    
                    LayoutPiece* childPiece = AddLayoutPiece(newLayout, childName, childIndex);
                    childPiece->parent = piece;
                    
                    for(u32 state = ObjectState_Default; state < ObjectState_Count; ++state)
                    {
                        childPiece->params[state].valid = false;
                    }
                    
                    EditorElement* childParams = GetList(decorationPieces, "params");
                    while(childParams)
                    {
                        ObjectState childType = (ObjectState) GetValuePreprocessor(ObjectState, GetValue(childParams, "objectState"));
                        EditorElement* childOffset = GetStruct(childParams, "offset");
                        r32 childX = ToR32(GetValue(childOffset, "x"));
                        r32 childY = ToR32(GetValue(childOffset, "y"));
                        r32 childZ = ToR32(GetValue(childOffset, "z"));
                        
                        r32 childAngle = ToR32(GetValue(childParams, "angle"));
                        
                        EditorElement* scale = GetStruct(decorationPieces, "scale");
                        r32 childScaleX = ToR32(GetValue(scale, "x"));
                        r32 childScaleY = ToR32(GetValue(scale, "y"));
                        r32 childAlpha = ToR32(GetValue(childParams, "alpha"));
                        
                        Vec2 childPivot = ToV2(GetStruct(childParams, "pivot"), V2(0.5f, 0.5f));
                        
                        AddLayoutPieceParams(childPiece, childType, V3(childX, childY, childZ), childAngle, V2(childScaleX, childScaleY), childAlpha, childPivot); 
                        
                        childParams = childParams->next;
                    }
                    
                    decorationPieces = decorationPieces->next;
                    
                }
                
                pieces = pieces->next;
            }
            
            ++currentSlot_->layoutCount;
            FREELIST_INSERT(newLayout, currentSlot_->firstLayout);
            
            layouts = layouts->next;
        }
    }
    
    
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

internal void ImportAllFiles(char* dataPath, u32 editorRoles, b32 freeTab)
{
    StartingLoadingMessageServer();
    MemoryPool tempPool = {};
    TempMemory fileMemory = BeginTemporaryMemory(&tempPool);
    
    u32 bufferSize = MegaBytes(16);
    char* buffer = (char*) PushSize(&tempPool, bufferSize, NoClear());
    
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
            
			if(!currentSlot_ && taxonomyName[0] == '#')
			{
				currentSlot_ = NORUNTIMEGetTaxonomySlotByName(taxTable_, taxonomyName + 1);
			}
            if(currentSlot_)
            {
                LoadFileInTaxonomySlot(source, editorRoles);
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
    
    EndingLoadingMessageServer();
}

#ifndef FORG_SERVER
internal void ImportAllAssetFiles(GameModeWorld* worldMode, char* dataPath, MemoryPool* tempPool)
{
    TempMemory fileMemory = BeginTemporaryMemory(tempPool);
    u32 bufferSize = MegaBytes(16);
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
            worldMode->table->soundNamesRoot = LoadElementsInMemory(LoadElements_Asset, &tokenizer, &ign);
        }
        else if(StrEqual(handle.name, "soundEvents.fad"))
        {
            worldMode->table->soundEventsRoot = LoadElementsInMemory(LoadElements_Asset, &tokenizer, &ign);
            Import(0, worldMode->table->soundEventsRoot);
        }
        else if(StrEqual(handle.name, "components.fad"))
        {
            worldMode->table->componentsRoot = LoadElementsInMemory(LoadElements_Asset, &tokenizer, &ign);
        }
        else if(StrEqual(handle.name, "componentvanilla.fad"))
        {
            worldMode->table->oldComponentsRoot = LoadElementsInMemory(LoadElements_Asset, &tokenizer, &ign);
            
            for(EditorElement** componentTypePtr = &worldMode->table->componentsRoot; *componentTypePtr; )
            {
                b32 present = false;
                EditorElement* componentType = *componentTypePtr;
                EditorElement* vanilla = 0;
                
                for(EditorElement* vanillaTest = worldMode->table->oldComponentsRoot; vanillaTest; vanillaTest = vanillaTest->next)
                {
                    if(StrEqual(vanillaTest->name, componentType->name))
                    {
                        vanilla = vanillaTest;
                        break;
                    }
                }
                
                if(vanilla)
                {
                    for(EditorElement** componentPtr = &componentType->firstInList; *componentPtr; )
                    {
                        b32 componentPresent = false;
                        EditorElement* component = *componentPtr;
                        char* componentName = GetValue(component, "componentName");
                        
                        for(EditorElement* vanillaComponent = vanilla->firstInList; vanillaComponent; vanillaComponent = vanillaComponent->next)
                        {
                            char* vanillaName = GetValue(vanillaComponent, "componentName");
                            if(StrEqual(vanillaName, componentName))
                            {
                                componentPresent = true;
                                break;
                            }
                        }
                        
                        if(componentPresent)
                        {
                            componentPtr = &component->next;
                        }
                        else
                        {
                            *componentPtr = component->next;
                            FreeElement(component, false);
                        }
                    }
                    
                    componentTypePtr = &componentType->next;
                }
                else
                {
                    *componentTypePtr = componentType->next;
                    FreeElement(componentType, false);
                }
            }
        }
        
        platformAPI.CloseHandle(&handle);
    }
    platformAPI.GetAllFilesEnd(&assetGroup);
    
    EndTemporaryMemory(fileMemory);
}
#endif

#if FORG_SERVER
inline char* WriteElementsToBuffer(char* buffer, EditorElement* root, u32* remaining, u32* writtenTotal)
{
    char* result = buffer;
    
    u32 newRemaining = *remaining;
    WriteElements(buffer, &newRemaining, root);
    u32 written = *remaining - newRemaining;
    *writtenTotal += written;
    
    result += written;
    *remaining = newRemaining;
    
    
    *result++ = ' ';
    *result++ = '\n';
    *remaining -= 2;
    *writtenTotal += 2;
    
    return result;
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
        EditorElement* root = slot->tabs[tabIndex].root;
        currentBuffer = WriteElementsToBuffer(currentBuffer, root, &remaining, &writtenTotal);
    }
    
    
    FormatString(writeHere, remainingSize, "%s.fed", slot->name);
    platformAPI.DEBUGWriteFile(path, buffer, writtenTotal);
}

inline void PatchLocalServer(ServerState* server)
{
    char* destinationFolder = "../server/assets";
    char* destinationPath = "../server";
    platformAPI.DeleteFolderRecursive(destinationFolder);
    platformAPI.CopyAllFiles("assets", destinationPath);
    
    SendPatchDoneMessageToAllPlayers(server);
}


internal void SendDataFiles(b32 editorMode, ServerPlayer* player,b32 sendTaxonomyFiles, b32 sendMetaAssetFiles)
{
#if 1
    char* path = "assets";
    
    MemoryPool tempPool = {};
    TempMemory fileMemory = BeginTemporaryMemory(&tempPool);
    
    u32 bufferSize = MegaBytes(16);
    char* buffer = (char*) PushSize(&tempPool, bufferSize, NoClear());
    
    
    if(sendTaxonomyFiles)
    {
        PlatformFileGroup definitionGroup = platformAPI.GetAllFilesBegin(PlatformFile_entityDefinition, path);
        for(u32 fileIndex = 0; fileIndex < definitionGroup.fileCount; ++fileIndex)
        {
            PlatformFileHandle handle = platformAPI.OpenNextFile(&definitionGroup, path);
            
            if(editorMode || handle.name[0] != '#')
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
    
    if(sendMetaAssetFiles)
    {
        if(editorMode)
        {
            PlatformFileGroup autocompleteGroup = platformAPI.GetAllFilesBegin(PlatformFile_autocomplete, path);
            for(u32 fileIndex = 0; fileIndex < autocompleteGroup.fileCount; ++fileIndex)
            {
                PlatformFileHandle handle = platformAPI.OpenNextFile(&autocompleteGroup, path);
                
                if(editorMode || handle.name[0] != '#')
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
                
                if(editorMode || handle.name[0] != '#')
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
    }
    
    EndTemporaryMemory(fileMemory);
#endif
    SendAllDataFileSentMessage(player, sendTaxonomyFiles);
}

inline b32 TabHasPreemption(char* tabName, u32 roles)
{
    b32 result = false;
    
    if(StrEqual(tabName, "soundEffects"))
    {
        result = (roles & EditorRole_SoundDesigner);
    }
    
    return result;
}

inline b32 HasPreemption(char* folderName, u32 roles)
{
    b32 result = false;
    
    if(StrEqual(folderName, "sound") && (roles & EditorRole_SoundDesigner))
    {
        result = true;
    }
    else if(StrEqual(folderName, "soundEvents") && (roles & EditorRole_SoundDesigner))
    {
        result = true;
    }
    
    return result;
}

internal void RecursivelyMerge(char* toMergeDefinitionName, char* toMergePath, char* name, u32 toMergeRoles)
{
    char fedSourceFilePath[512];
    FormatString(fedSourceFilePath, sizeof(fedSourceFilePath), "%s/%s/%s.fed", toMergeDefinitionName, toMergePath, name);
    
    char fedDestFilePath[512];
    FormatString(fedDestFilePath, sizeof(fedDestFilePath), "definition/%s/%s.fed", toMergePath, name);
    
    
    PlatformFile fedSourceFile = platformAPI.DEBUGReadFile(fedSourceFilePath);
    
    if(fedSourceFile.content)
    {
        Tokenizer sourceT = {};
        sourceT.at = (char*) fedSourceFile.content;
        
        u32 sourceTabCount = 0;
        EditorElement* sourceTabs[128] = {};
        
        b32 endSource = false;
        while(true)
        {
            EditorElement* root = LoadElementsInMemory(LoadElements_Tab, &sourceT, &endSource);
            Assert(sourceTabCount < ArrayCount(sourceTabs));
            sourceTabs[sourceTabCount++] = root;
            if(endSource)
            {
                break;
            }
        }
        platformAPI.DEBUGFreeFile(&fedSourceFile);
        
        PlatformFile fedDestFile = platformAPI.DEBUGReadFile(fedDestFilePath);
        if(fedDestFile.content)
        {
            Tokenizer destT = {};
            destT.at = (char*) fedDestFile.content;
            
            u32 destTabCount = 0;
            EditorElement* destTabs[128] = {};
            
            b32 endDest = false;
            while(true)
            {
                EditorElement* root = LoadElementsInMemory(LoadElements_Tab, &destT, &endDest);
                Assert(destTabCount < ArrayCount(destTabs));
                destTabs[destTabCount++] = root;
                if(endDest)
                {
                    break;
                }
            }
            platformAPI.DEBUGFreeFile(&fedDestFile);
            
            
            char buffer[KiloBytes(16)] = {};
            char* currentBuffer = buffer;
            
            u32 remaining = sizeof(buffer);
            u32 writtenTotal = 0;
            
            for(u32 destTabIndex = 0; destTabIndex < destTabCount; ++destTabIndex)
            {
                EditorElement* toWrite = destTabs[destTabIndex];
                
                for(u32 sourceTabIndex = 0; sourceTabIndex < sourceTabCount; ++sourceTabIndex)
                {
                    EditorElement* test = sourceTabs[sourceTabIndex];
                    if(StrEqual(test->name, toWrite->name))
                    {
                        if(TabHasPreemption(test->name, toMergeRoles))
                        {
                            toWrite = test;
                        }
                        break;
                    }
                }
                
                currentBuffer = WriteElementsToBuffer(currentBuffer, toWrite, &remaining, &writtenTotal);
            }
            platformAPI.DEBUGWriteFile(fedDestFilePath, buffer, writtenTotal);
        }
    }
    
    PlatformSubdirNames subdir;
    subdir.subDirectoryCount = 0;
    
    char childToSearchPath[512];
    FormatString(childToSearchPath, sizeof(childToSearchPath), "%s/%s", toMergeDefinitionName, toMergePath);
    platformAPI.GetAllSubdirectoriesName(&subdir, childToSearchPath);
    for(u32 subdirIndex = 0; subdirIndex < subdir.subDirectoryCount; ++subdirIndex)
    {
        char* folderName = subdir.subdirs[subdirIndex];
        if(StrEqual(folderName, "side"))
        {
        }
        else
        {
            if(!StrEqual(folderName, ".") && !StrEqual(folderName, ".."))
            {
                char childPath[512];
                FormatString(childPath, sizeof(childPath), "%s/%s", toMergePath, folderName);
                RecursivelyMerge(toMergeDefinitionName, childPath, folderName, toMergeRoles);
            }
        }
    }
}

internal void Merge(TaxonomyTable* table, char* toMergePath, u32 toMergeRoles)
{
    char* referencePath = "definition";
    
    PlatformSubdirNames subdir;
    subdir.subDirectoryCount = 0;
    
    platformAPI.GetAllSubdirectoriesName(&subdir, toMergePath);
    for(u32 subdirIndex = 0; subdirIndex < subdir.subDirectoryCount; ++subdirIndex)
    {
        char* folderName = subdir.subdirs[subdirIndex];
        if(StrEqual(folderName, "root"))
        {
            InitTaxonomyReadWrite(table);
            RecursivelyMerge(toMergePath, folderName, folderName, toMergeRoles);
            Clear(&table->pool);
        }
        else
        {
            if(HasPreemption(folderName, toMergeRoles))
            {
                char pathToDelete[512];
                FormatString(pathToDelete, sizeof(pathToDelete), "%s/%s", referencePath, folderName);
                platformAPI.DeleteFolderRecursive(pathToDelete);
                
                char sourcePath[512];
                FormatString(sourcePath, sizeof(sourcePath), "%s/%s", toMergePath, folderName);
                platformAPI.CopyAllFiles(sourcePath, pathToDelete);
            }
        }
    }
    
    platformAPI.DeleteFolderRecursive(toMergePath);
}

inline void CheckForDefinitionsToMerge(ServerState* server)
{
    printf("checking for definitions to merge\n");
    PlatformSubdirNames subdir;
    subdir.subDirectoryCount = 0;
    
    platformAPI.GetAllSubdirectoriesName(&subdir, ".");
    for(u32 subdirIndex = 0; subdirIndex < subdir.subDirectoryCount; ++subdirIndex)
    {
        char* definitionName = subdir.subdirs[subdirIndex];
        if(!StrEqual(definitionName, ".") && !StrEqual(definitionName, "..") && 
           !StrEqual(definitionName, "definition") && !StrEqual(definitionName, "assets"))
        {
            if(StrEqual(definitionName, "SoundDesigner"))
            {
                printf("Merging sound designer patch...\n");
                Merge(server->activeTable, definitionName, EditorRole_SoundDesigner);
            }
            else
            {
                printf("unknown patch name %s\n", definitionName);
            }
        }
    }
    
    printf("merging done\n");
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

