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
    else if(StrEqual(name, "particleEffects"))
    {
        taxTable_->particleEffectsTaxonomy = taxonomy;
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


internal void WriteDataFilesAndTaxonomies()
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
	u32 result = 0;
    
    for(u32 valueIndex = 0; valueIndex < count; ++valueIndex)
    {
		MetaFlag* flag = values + valueIndex;
        if(StrEqual(test, flag->name))
        {
            result = flag->value;
            break;
        }
    }
    
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

inline b32 ToB32(char* string, b32 default = false)
{
    b32 result = default;
    if(string)
    {
        result = (StrEqual(string, "true"));
    }
    return result;
}

inline r32 ToR32(char* string, r32 standard = 1.0f)
{
    r32 result = standard;
    
    if(string)
    {
        result = R32FromChar(string);
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


#define ElemU32(root, name) ToU32(GetValue(root, name))
#define ElemR32(root, name, ...) ToR32(GetValue(root, name), ##__VA_ARGS__)
#define StructV3(root, name) ToV3(GetStruct(root, name))
#define ColorV4(root, name) ToV4Color(GetStruct(root, name))


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

inline void AddEssence(TaxonomyEssence** firstEssence, char* name, u32 quantity)
{
    quantity = Max(quantity, 1);
    TaxonomySlot* essenceSlot = NORUNTIMEGetTaxonomySlotByName(taxTable_, name);
    if(essenceSlot)
    {
        b32 found = false;
        for(TaxonomyEssence* essence = *firstEssence; essence; essence = essence->next)
        {
            if(essence->essence.taxonomy == essenceSlot->taxonomy)
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
            essence->essence.taxonomy = essenceSlot->taxonomy;
            essence->essence.quantity = quantity;
            FREELIST_INSERT(essence, *firstEssence);
        }
    }
    else
    {
        EditorErrorLog(name);
    }
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
                
                if(element->flags & EditorElem_AtLeastOneInList)
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

#define LoadElementsInMemoryTable(mode, tokenizer, end) LoadElementsInMemory(mode, tokenizer, end, &taxTable_->firstFreeElement, &taxTable_->firstFreeEditorText, taxPool_)
inline EditorElement* LoadElementsInMemory(LoadElementsMode mode, Tokenizer* tokenizer, b32* end, EditorElement** firstFreeElement, EditorTextBlock** firstFreeEditorText,  MemoryPool* pool)
{
	EditorElement* newElement;
    FREELIST_ALLOC(newElement, *firstFreeElement, PushStruct(pool, EditorElement));
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
                        FREELIST_ALLOC(text, *firstFreeEditorText, PushStruct(pool, EditorTextBlock));
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
                                    newElement->emptyElement = LoadElementsInMemory(mode, tokenizer, end, firstFreeElement, firstFreeEditorText, pool);
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
                        newElement->firstInList = LoadElementsInMemory(mode, tokenizer, end, firstFreeElement, firstFreeEditorText, pool);
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
                        newElement->firstValue = LoadElementsInMemory(mode, tokenizer, end, firstFreeElement, firstFreeEditorText, pool);
                        
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
            
            newElement->firstValue = LoadElementsInMemory(mode, tokenizer, end, firstFreeElement, firstFreeEditorText, pool);
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
            newElement->next = LoadElementsInMemory(mode, tokenizer, end, firstFreeElement, firstFreeEditorText, pool);
        }
    }
    else if(NextTokenIs(tokenizer, Token_EndOfFile))
    {
        *end = true;
    }
    
    return newElement;
}

#define FreeElementTable(element, freeNext) FreeElement(element, &taxTable_->firstFreeElement, &taxTable_->firstFreeEditorText, freeNext)
inline void FreeElement(EditorElement* element, EditorElement** firstFreeElement, EditorTextBlock** firstFreeEditorText, b32 freeNext)
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
                FREELIST_DEALLOC(element->text, *firstFreeEditorText);
            } break;
            
            case EditorElement_List:
            {
                FreeElement(element->firstInList, firstFreeElement, firstFreeEditorText, freeNext);
            } break;
            
            case EditorElement_Struct:
            {
                FreeElement(element->firstValue, firstFreeElement, firstFreeEditorText, freeNext);
            } break;
            
            case EditorElement_Taxonomy:
            {
                FreeElement(element->firstChild, firstFreeElement, firstFreeEditorText, freeNext);
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
                FreeElement(element->next, firstFreeElement, firstFreeEditorText, freeNext);
            }
            
        }
        
        FREELIST_DEALLOC(element, (*firstFreeElement));
    }
}

inline EditorElement* CopyEditorElement(TaxonomyTable* table, EditorElement* source)
{
    EditorElement* result;
    FREELIST_ALLOC(result, table->firstFreeElement, PushStruct(&table->pool, EditorElement));
    
    *result = *source;
    
    
    switch(source->type)
    {
        case EditorElement_String:
        case EditorElement_Real:
        case EditorElement_Signed:
        case EditorElement_Unsigned:
        {
            
        } break;
        
        case EditorElement_Text:
        {
            FREELIST_ALLOC(result->text, table->firstFreeEditorText, PushStruct(&table->pool, EditorTextBlock));
            Copy(sizeof(result->text->text), result->text->text, source->text->text);
        } break;
        
        case EditorElement_List:
        {
            if(source->emptyElement)
            {
                result->emptyElement = CopyEditorElement(table, source->emptyElement);
            }
            
            if(source->firstChild)
            {
                result->firstChild = CopyEditorElement(table, source->firstChild);
            }
        } break;
        
        case EditorElement_Struct:
        {
            if(source->firstValue)
            {
                result->firstValue = CopyEditorElement(table, source->firstValue);
            }
        } break;
        
        InvalidDefaultCase;
    }
    
    if(source->next)
    {
        result->next = CopyEditorElement(table, source->next);
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
        FreeElementTable(tab->root, true);
        tab->editable = 0;
    }
    currentSlot_->tabCount = 0;
    
    
    if(content[0])
    {
        b32 end = false;
        while(true)
        {
            Assert(currentSlot_->tabCount < ArrayCount(currentSlot_->tabs));
            
            u32 tabIndex = currentSlot_->tabCount++;
            
            EditorTab* newTab = currentSlot_->tabs + tabIndex;
            newTab->root = LoadElementsInMemoryTable(LoadElements_Tab, &tokenizer, &end);
            newTab->editable = IsEditableByRole(newTab->root, editorRoles);
            
            if(end)
            {
                break;
            }
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





#include "editor/tab_translation.cpp"

#include "editor/effect.cpp"

#include "editor/bounds.cpp"
#include "editor/equipmentMappings.cpp"
#include "editor/neededTool.cpp"
#include "editor/craftingEssences.cpp"
#include "editor/defaultEssences.cpp"
#include "editor/container.cpp"
#include "editor/tileParams.cpp"

#include "editor/light.cpp"

#ifndef FORG_SERVER
#include "editor/soundEvent.cpp"
#include "editor/animationEffects.cpp"
#include "editor/soundEffects.cpp"
#include "editor/animationGeneralParams.cpp"
//#include "editor/visualLabels.cpp"
#include "editor/boneAlteration.cpp"
#include "editor/assAlteration.cpp"
#include "editor/icon.cpp"
#include "editor/particleEffectDefinition.cpp"
#include "editor/boltDefinition.cpp"
#endif
#include "editor/rockDefinition.cpp"
#include "editor/plantDefinition.cpp"
#include "editor/generatorParams.cpp"
#include "editor/layouts.cpp"

#if FORG_SERVER
#include "editor/effects.cpp"
#include "editor/freeHandsRequirements.cpp"
#include "editor/craftingEffects.cpp"
#include "editor/attributes.cpp"
#include "editor/possibleActions.cpp"
#include "editor/behaviors.cpp"
#include "editor/memBehaviors.cpp"
#endif


internal void Import(TaxonomySlot* slot, EditorElement* root)
{
    currentSlot_ = slot;
    
    char* name = root->name;
    
    b32 valid = ToB32(GetValue(root, "enabled"), true);
    
    if(valid)
    {
        if(StrEqual(name, "bounds"))
        {
            ImportBoundsTab(slot, root);
        }
        else if(StrEqual(name, "equipmentMappings"))
        {
            ImportEquipmentMappingsTab(slot, root);
            
        }
        else if(StrEqual(name, "neededTools"))
        {
            ImportNeededToolsTab(slot, root);
        }
        else if(StrEqual(name, "craftingEssences"))
        {
            ImportCraftingEssencesTab(slot, root);
        }
#if FORG_SERVER
        else if(StrEqual(name, "effects"))
        {
            ImportEffectsTab(slot, root);
            
        }
        else if(StrEqual(name, "freeHandsRequirements"))
        {
            ImportFreeHandsRequirementsTab(slot, root);
        }
        else if(StrEqual(name, "craftingEffects"))
        {
            ImportCraftingEffectsTab(slot, root);
            
        }
        else if(StrEqual(name, "defaultEssences"))
        {
            ImportDefaultEssencesTab(slot, root);
        }
        else if(StrEqual(name, "attributes"))
        {
            ImportAttributesTab(slot, root);
        }
        
        else if(StrEqual(name, "possibleActions"))
        {
            ImportPossibleActionsTab(slot, root);
        }
        
        else if(StrEqual(name, "behaviors"))
        {
            ImportBehaviorsTab(slot, root);
        }
        else if(StrEqual(name, "memBehaviors"))
        {
            ImportMemBehaviorsTab(slot, root);
        }
#endif
        else if(StrEqual(name, "container"))
        {
            ImportContainerTab(slot, root);
        }
        else if(StrEqual(name, "tileParams"))
        {
            ImportTileParamsTab(slot, root);
        }
#ifndef FORG_SERVER
        else if(StrEqual(name, "visualLabels"))
        {
            //ImportVisualLabelsTab(slot, root);
        }
        else if(StrEqual(name, "animationGeneralParams"))
        {
            ImportAnimationGeneralParamsTab(slot, root);
        }
        else if(StrEqual(name, "light"))
        {
            ImportLightTab(slot, root);
        }
        else if(StrEqual(name, "animationEffects"))
        {
            ImportAnimationEffectsTab(slot, root);
        }
        else if(StrEqual(name, "soundEffects"))
        {
            ImportSoundEffectsTab(slot, root);
        }
        else if(StrEqual(name, "soundEvents"))
        {
            
            ImportSoundEventTab(root);
        }
        else if(StrEqual(name, "boneAlterations"))
        {
            ImportBoneAlterationsTab(slot, root);
        }
        else if(StrEqual(name, "assAlterations"))
        {
            ImportAssAlterationsTab(slot, root);
        }
        else if(StrEqual(name, "icon"))
        {
            ImportIconTab(slot, root);
            
        }
#endif
        else if(StrEqual(name, "rockDefinition"))
        {
            ImportRockDefinitionTab(slot, root);
        }
        else if(StrEqual(name, "plantDefinition"))
        {
            ImportPlantDefinitionTab(slot, root);
            
        }
#ifndef FORG_SERVER
        else if(StrEqual(name, "particleEffectDefinition"))
        {
            ImportParticleEffectDefinitionTab(slot, root);
        }
        else if(StrEqual(name, "boltDefinition"))
        {
            ImportBoltDefinitionTab(slot, root);
        }
#endif
        else if(StrEqual(name, "generatorParams"))
        {
            ImportGeneratorParamsTab(slot, root);
            
        }
        else if(StrEqual(name, "layouts"))
        {
            ImportLayoutsTab(slot, root);
            
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

internal void CopyAndLoadTabsFromOldTable(TaxonomyTable* oldTable)
{
    for(u32 slotIndex = 0; slotIndex < ArrayCount(oldTable->slots); ++slotIndex)
    {
        for(TaxonomySlot* slot = oldTable->slots[slotIndex]; slot; slot = slot->nextInHash)
        {
            TaxonomySlot* newSlot = NORUNTIMEGetTaxonomySlotByName(taxTable_, slot->name);
            
            if(newSlot)
            {
                for(u32 tabIndex = 0; tabIndex < slot->tabCount; ++tabIndex)
                {
                    EditorTab* tab = slot->tabs + tabIndex;
                    EditorTab* newTab = newSlot->tabs + newSlot->tabCount++;
                    
                    newTab->editable = tab->editable;
                    newTab->root = CopyEditorElement(taxTable_, tab->root);
                    Import(newSlot, newTab->root);
                }
            }
        }
    }
}

internal void ImportSpecificFile(u32 editorRoles, b32 freeTab, char* filename)
{
    char* dataPath = "assets";
    MemoryPool tempPool = {};
    TempMemory fileMemory = BeginTemporaryMemory(&tempPool);
    
    u32 bufferSize = MegaBytes(16);
    char* buffer = (char*) PushSize(&tempPool, bufferSize, NoClear());
    
    PlatformFileGroup fileGroup = platformAPI.GetAllFilesBegin(PlatformFile_entityDefinition, dataPath);
    for(u32 fileIndex = 0; fileIndex < fileGroup.fileCount; ++fileIndex)
    {
        PlatformFileHandle handle = platformAPI.OpenNextFile(&fileGroup, dataPath);
        if(StrEqual(handle.name, filename))
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
                        FreeElementTable(tab->root, true);
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

internal void ImportAllFiles(u32 editorRoles, b32 freeTab, char* filename)
{
    char* dataPath = "assets";
    StartingLoadingMessageServer();
    MemoryPool tempPool = {};
    TempMemory fileMemory = BeginTemporaryMemory(&tempPool);
    
    u32 bufferSize = MegaBytes(16);
    char* buffer = (char*) PushSize(&tempPool, bufferSize, NoClear());
    
    PlatformFileGroup fileGroup = platformAPI.GetAllFilesBegin(PlatformFile_entityDefinition, dataPath);
    for(u32 fileIndex = 0; fileIndex < fileGroup.fileCount; ++fileIndex)
    {
        PlatformFileHandle handle = platformAPI.OpenNextFile(&fileGroup, dataPath);
        if(!StrEqual(handle.name, "taxonomies.fed") && (!filename || (StrEqual(filename, handle.name))))
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
                        FreeElementTable(tab->root, true);
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
            FreeElement(worldMode->soundNamesRoot, &worldMode->firstFreeEditorElement, &worldMode->firstFreeEditorText, true);
            worldMode->soundNamesRoot = LoadElementsInMemory(LoadElements_Asset, &tokenizer, &ign, &worldMode->firstFreeEditorElement, &worldMode->firstFreeEditorText, worldMode->persistentPool);
        }
        else if(StrEqual(handle.name, "soundEvents.fad"))
        {
            FreeElement(worldMode->soundEventsRoot, &worldMode->firstFreeEditorElement, &worldMode->firstFreeEditorText, true);
            worldMode->soundEventsRoot = LoadElementsInMemory(LoadElements_Asset, &tokenizer, &ign, &worldMode->firstFreeEditorElement, &worldMode->firstFreeEditorText, worldMode->persistentPool);
            Import(0, worldMode->soundEventsRoot);
        }
        else if(StrEqual(handle.name, "components.fad"))
        {
            FreeElement(worldMode->componentsRoot, &worldMode->firstFreeEditorElement, &worldMode->firstFreeEditorText, true);
            worldMode->componentsRoot = LoadElementsInMemory(LoadElements_Asset, &tokenizer, &ign, &worldMode->firstFreeEditorElement, &worldMode->firstFreeEditorText, worldMode->persistentPool);
        }
        else if(StrEqual(handle.name, "componentvanilla.fad"))
        {
            FreeElement(worldMode->oldComponentsRoot, &worldMode->firstFreeEditorElement, &worldMode->firstFreeEditorText, true);
            worldMode->oldComponentsRoot = LoadElementsInMemory(LoadElements_Asset, &tokenizer, &ign, &worldMode->firstFreeEditorElement, &worldMode->firstFreeEditorText, worldMode->persistentPool);
            
            for(EditorElement** componentTypePtr = &worldMode->componentsRoot; *componentTypePtr; )
            {
                b32 present = false;
                EditorElement* componentType = *componentTypePtr;
                EditorElement* vanilla = 0;
                
                for(EditorElement* vanillaTest = worldMode->oldComponentsRoot; vanillaTest; vanillaTest = vanillaTest->next)
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
                            FreeElement(component, &worldMode->firstFreeEditorElement, &worldMode->firstFreeEditorText, false);
                        }
                    }
                    
                    componentTypePtr = &componentType->next;
                }
                else
                {
                    *componentTypePtr = componentType->next;
                    FreeElement(componentType, &worldMode->firstFreeEditorElement, &worldMode->firstFreeEditorText, false);
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

internal void SendSpecificFile(ServerPlayer* player, char* filename)
{
    char* path = "assets";
    MemoryPool tempPool = {};
    TempMemory fileMemory = BeginTemporaryMemory(&tempPool);
    
    u32 bufferSize = MegaBytes(16);
    char* buffer = (char*) PushSize(&tempPool, bufferSize, NoClear());
    
    
    PlatformFileGroup definitionGroup = platformAPI.GetAllFilesBegin(PlatformFile_entityDefinition, path);
    for(u32 fileIndex = 0; fileIndex < definitionGroup.fileCount; ++fileIndex)
    {
        PlatformFileHandle handle = platformAPI.OpenNextFile(&definitionGroup, path);
        if(StrEqual(handle.name, filename))
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
    EndTemporaryMemory(fileMemory);
}

internal void SendAllDataFiles(b32 editorMode, ServerPlayer* player, DataFileSentType sentType)
{
    char* path = "assets";
    
    MemoryPool tempPool = {};
    TempMemory fileMemory = BeginTemporaryMemory(&tempPool);
    
    u32 bufferSize = MegaBytes(16);
    char* buffer = (char*) PushSize(&tempPool, bufferSize, NoClear());
    
    
    if(sentType == DataFileSent_OnlyTaxonomies || sentType == DataFileSent_Everything)
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
    
    if(sentType == DataFileSent_OnlyAssets || sentType == DataFileSent_Everything)
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
            EditorElement* root = LoadElementsInMemoryTable(LoadElements_Tab, &sourceT, &endSource);
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
                EditorElement* root = LoadElementsInMemoryTable(LoadElements_Tab, &destT, &endDest);
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

