
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


inline TaxonomyNode* AddToTaxonomyTree(TaxonomyTable* table, TaxonomyTree* tree, TaxonomySlot* slot)
{
    if(!tree->root)
    {
        TaxonomyNode* newNode = PushStruct(&slot->pool, TaxonomyNode);
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
        TaxonomySlot* currentSlot = GetSlotForTaxonomy(table, currentTaxonomy);
        u32 childTaxonomy = GetChildTaxonomy(currentSlot, taxonomy);
        
        
        if(!current->firstChild)
        {
            TaxonomyNode* newNode = PushStruct(&slot->pool, TaxonomyNode);
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
                TaxonomyNode* newNode = PushStruct(&slot->pool, TaxonomyNode);
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


#if 0
internal void LoadFileInTaxonomySlot(char* content, u32 editorRoles)
{
    Tokenizer tokenizer = {};
    tokenizer.at = content;
    
    Clear(&currentSlot_->pool);
    ZeroStruct(*currentSlot_);
    
    if(content[0])
    {
        b32 end = false;
        while(true)
        {
            Assert(currentSlot_->tabCount < ArrayCount(currentSlot_->tabs));
            
            u32 tabIndex = currentSlot_->tabCount++;
            
            EditorTab* newTab = currentSlot_->tabs + tabIndex;
            newTab->root = LoadElementsInMemory(LoadElements_Tab, &tokenizer, &end, &currentSlot_->pool);
            newTab->editable = IsEditableByRole(newTab->root, editorRoles);
            
            if(end)
            {
                break;
            }
        }
    }
}
#endif



internal void ImportAllFiles()
{
#if RESTRUCTURING
    InitDefaultStateMachine();
    ReadSynthesisRules();
#endif
}

