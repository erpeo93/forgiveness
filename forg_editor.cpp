#ifndef FORG_SERVER
inline void SwapTables(GameModeWorld* worldMode)
{
    TaxonomyTable* old = worldMode->oldTable;
    Clear(&old->pool_);
    ZeroStruct(*old);
    worldMode->oldTable = worldMode->table;
    worldMode->table = old;
}
#else
inline void SwapTables(ServerState* server)
{
    TaxonomyTable* toClear = server->oldTable;
    Clear(&toClear->pool_);
    ZeroStruct(*toClear);
    
    TaxonomyTable* temp = toClear;
    server->oldTable = server->activeTable;
    server->activeTable = temp;
}
#endif


inline ShortcutSlot* SaveShortcut(TaxonomyTable* table, char* name, u32 nameLength)
{
    MemoryPool* pool = &table->pool_;
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

#define Push(table, name) Push_(table, #name)
inline void Push_(TaxonomyTable* table, char* name, u32 nameLength = 0)
{
    ShortcutSlot* result = 0;
    Assert(stackShortcutCount < ArrayCount(shortcutStack));
    ShortcutSlot * currentSlot = shortcutStack[stackShortcutCount - 1];
    
    
    if(name[0] == '#')
    {
        ++currentSlot->invalidTaxonomiesCount;
    }
    
    AddSubTaxonomy(currentSlot, name, nameLength);
    result = SaveShortcut(table, name, nameLength);
    
    shortcutStack[stackShortcutCount++] = result;
}

inline void Pop()
{
    Assert(stackShortcutCount > 0);
    --stackShortcutCount;
}

inline void FinalizeShortcut(TaxonomyTable* table, ShortcutSlot* shortcut, u32 taxonomy)
{
    char* name = shortcut->name;
    if(StrEqual(name, "creatures"))
    {
        table->creatureTaxonomy = taxonomy;
    }
    else if(StrEqual(name, "objects"))
    {
        table->objectTaxonomy = taxonomy;
    }
    else if(StrEqual(name, "equipment"))
    {
        table->equipmentTaxonomy = taxonomy;
    }
    else if(StrEqual(name, "plants"))
    {
        table->plantTaxonomy = taxonomy;
    }
    else if(StrEqual(name, "rocks"))
    {
        table->rockTaxonomy = taxonomy;
    }
    else if(StrEqual(name, "essences"))
    {
        table->essenceTaxonomy = taxonomy;
    }
    else if(StrEqual(name, "recipe"))
    {
        table->recipeTaxonomy = taxonomy;
    }
    else if(StrEqual(name, "behaviors"))
    {
        table->behaviorTaxonomy = taxonomy;
    }
    else if(StrEqual(name, "tiles"))
    {
        table->tileTaxonomy = taxonomy;
    }
    else if(StrEqual(name, "generators"))
    {
        table->generatorTaxonomy = taxonomy;
    }
    else if(StrEqual(name, "particleEffects"))
    {
        table->particleEffectsTaxonomy = taxonomy;
    }    
}


internal u32 FinalizeTaxonomies(TaxonomyTable* table, char* giantBuffer, u32 giantBufferLength, b32 writeToFile, TaxonomySlot* parent, TaxonomySlot* slot, ShortcutSlot* shortcut)
{
    u32 written = 0;
    if(giantBuffer && writeToFile)
    {
        FormatString(giantBuffer, giantBufferLength, "\"%s\"", shortcut->name);
        written = (StrLen(shortcut->name) + 2);
        giantBuffer += written;
        giantBufferLength -= written;
    }
    
    FinalizeShortcut(table, shortcut, slot->taxonomy);
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
        table->rootBits = slot->necessaryBits;
    }
    
    
    Assert(slot->usedBitsTotal <= 32);
    StrCpy(shortcut->name, StrLen(shortcut->name), slot->name, ArrayCount(slot->name));
    slot->stringHashID = StringHash(slot->name, ArrayCount(slot->name));
    for(u32 subIndex = 0; subIndex < shortcut->subTaxonomiesCount; ++subIndex)
    {
        u32 taxonomy = slot->taxonomy + ((subIndex + 1) << (32 - slot->usedBitsTotal));
        TaxonomySlot* newSlot = GetSlotForTaxonomy(table, taxonomy);
        newSlot->taxonomy = taxonomy;
        
        ShortcutSlot* newShortcut = GetShortcut(table, shortcut->subTaxonomies[subIndex]);
        newShortcut->taxonomy = taxonomy;
        u32 writtenChild = FinalizeTaxonomies(table, giantBuffer, giantBufferLength, true, slot, newSlot, newShortcut);
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
inline void WriteDataFilesRecursive(TaxonomyTable* table, char* path, char* name)
{
    if(!StrEqual(name, ".") && !StrEqual(name, "..") && !StrEqual(name, "side"))
    {
        Push_(table, name);
        
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
                WriteDataFilesRecursive(table, newPath, subName);
            }
        }
        
		for(u32 subdirIndex = 0; subdirIndex < subdir.subDirectoryCount; ++subdirIndex)
		{
			char* subName = subdir.subdirs[subdirIndex];
			if(subName[0] == '#')
			{
				WriteDataFilesRecursive(table, newPath, subName);
            }
		}
        
        
        Pop();
    }
}

internal void WriteDataFiles(TaxonomyTable* table)
{
	platformAPI.DeleteFileWildcards("assets", "*.fed");
    shortcutStack[stackShortcutCount++] = SaveShortcut(table, "root", 0);
    char* rootPath = "definition/root";
    
    PlatformSubdirNames subdir;
    subdir.subDirectoryCount = 0;
    
    platformAPI.GetAllSubdirectoriesName(&subdir, rootPath);
    for(u32 subdirIndex = 0; subdirIndex < subdir.subDirectoryCount; ++subdirIndex)
    {
        WriteDataFilesRecursive(table, rootPath, subdir.subdirs[subdirIndex]);
    }
    
    
    ShortcutSlot* rootShortcut = GetShortcut(table, "root");
    TaxonomySlot* rootSlot = &table->root;
    rootSlot->taxonomy = 0;
    
    MemoryPool tempPool = {};
    TempMemory bufferMemory = BeginTemporaryMemory(&tempPool);
    
    u32 giantBufferSize = MegaBytes(2);
    char* giantBuffer = PushArray(&tempPool, char, giantBufferSize);
    u32 written = FinalizeTaxonomies(table, giantBuffer, giantBufferSize, false, 0, rootSlot, rootShortcut);
    platformAPI.DEBUGWriteFile("assets/taxonomies.fed", giantBuffer, written);
    
    EndTemporaryMemory(bufferMemory);
}

internal void ReadTaxonomiesFromFile(TaxonomyTable* table)
{
    shortcutStack[stackShortcutCount++] = SaveShortcut(table, "root", 0);
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
                Push_(table, string.text, string.textLength);
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
    
    ShortcutSlot* rootShortcut = GetShortcut(table, "root");
    TaxonomySlot* rootSlot = &table->root;
    rootSlot->taxonomy = 0;
    FinalizeTaxonomies(table, 0, 0, 0, 0, rootSlot, rootShortcut);
}

struct DefinitionHash
{
    
};

#define AddToDefinitionHash(hash, name, definition) AddToDefinitionHash_(hash, #name, definition)
internal void AddToDefinitionHash_(DefinitionHash* hash, char* name, MemberDefinition* structDefinition)
{
    
}

internal void LoadAllStructDefinitions(DefinitionHash* definitionHash)
{
    META_HANDLE_ADD_TO_DEFINITION_HASH();
}

#if 0
internal void ParseBufferIntoStruct(char* buffer, void* struct, StructDefinition, Allocator)
{
    
    for(EachStructMember)
    {
        char* bufferPtr = SearchIntoBuffer();
        
        switch(fieldDefinition.type)
        {
            case MetaType_u32:
            {
                struct->field = atoi();
            } break;
            
            case MetaType_i32:
            {
                struct->field = atoi();
            } break;
            
            case MetaType_b32:
            {
                struct->field = atoi();
            } break;
            
            case MetaType_r32:
            {
                struct->field = atof();
            } break;
            
            case MetaType_Vec2:
            {
                ...
            } break;
            
            case MetaType_Vec3:
            {
                ...
            } break;
            
            case List:
            {
                
            } break;
            
            case Array:
            {
                
            } break;
            
            Default:
            {
                MemberDefinition childDefinition = GetMemberDefinition(member->name);
                ParseBufferIntoStruct(childDefinition, &bufferPtr);
            } break;
        }
    }
}

internal void DumpStructToBuffer()
{
    for(eachMember)
    {
        switch( member->type )
        {
            case MetaType_u32:
            {
                FormatString( textBuffer, textBufferLeft, "%s: %u", member->name, 
                             *( u32* ) memberPtr );
            } break;
            
            case MetaType_i32:
            {
                FormatString( textBuffer, textBufferLeft, "%s: %d", member->name, 
                             *( i32* ) memberPtr );
            } break;
            
            case MetaType_b32:
            {
                FormatString( textBuffer, textBufferLeft, "%s: %d", member->name, 
                             *( b32* ) memberPtr );
            } break;
            
            case MetaType_r32:
            {
                FormatString( textBuffer, textBufferLeft, "%s: %f", member->name, 
                             *( r32* ) memberPtr );
            } break;
            
            case MetaType_Vec2:
            {
                FormatString( textBuffer, textBufferLeft, "%s: {%f, %f}", member->name,
                             ( r32 ) ( ( Vec2* ) memberPtr )->x, ( r32 ) ( ( Vec2* ) memberPtr )->y );
            } break;
            
            case MetaType_Vec3:
            {
                FormatString( textBuffer, textBufferLeft, "%s: {%f, %f, %f}", 
                             member->name,
                             ( r32 ) ( ( Vec3* ) memberPtr )->x, 
                             ( r32 ) ( ( Vec3* ) memberPtr )->y,
                             ( r32 ) ( ( Vec3* ) memberPtr )->z );
            } break;
            
            case List:
            {
                
            } break;
            
            case Array:
            {
                
            } break;
            
            Default:
            {
                MemberDefinition childDefinition = GetMemberDefinition(member->name);
                DumpStruct(childDefinition);
            } break;
        }
    }
}

inline void EditU32()
{
    // NOTE(Leonardo): this should be the same on client and server!
    GUID guid = ?;
    
    if(guid == context->editedGuid)
    {
        *value = context->editedValue;
    }
}

internal void EditStruct()
{
    for(eachMember)
    {
        switch( member->type )
        {
            case MetaType_u32:
            {
                EditUnsigned();
            } break;
            
            case MetaType_i32:
            {
                EditSigned();
            } break;
            
            case MetaType_b32:
            {
                EditBool();
            } break;
            
            case MetaType_r32:
            {
                EditR32();
            } break;
            
            case MetaType_Vec2:
            {
                EditVec2();
            } break;
            
            case MetaType_Vec3:
            {
                EditVec3();
            } break;
            
            case List:
            {
                
            } break;
            
            case Array:
            {
                
            } break;
            
            Default:
            {
                MemberDefinition childDefinition = GetMemberDefinition(member->name);
                EditStruct(childDefinition);
            } break;
        }
    }
}
#endif