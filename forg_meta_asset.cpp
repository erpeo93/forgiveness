global_variable u32 meta_definitionCount;
global_variable StructDefinition meta_definitions[1024];
FieldDefinition* FindMetaField(FieldDefinition* fields, u32 fieldCount, Token fieldName)
{
    FieldDefinition* result = 0;
    for(u32 fieldIndex = 0; fieldIndex < fieldCount; ++fieldIndex)
    {
        FieldDefinition* test = fields + fieldIndex;
        if(TokenEquals(fieldName, test->name))
        {
            result = test;
            break;
        }
    }
    
    return result;
    
}
FieldDefinition* FindMetaField(StructDefinition* definition, Token fieldName)
{
    FieldDefinition* result = FindMetaField(definition->fields, definition->fieldCount, fieldName);
    Assert(result);
    return result;
}

#define IsFixedField(field, ptr, name) IsFixedField_(field, #name, &ptr->name)
internal b32 IsFixedField_(FieldDefinition* field, char* fieldName, void* fieldPtr)
{
    b32 result = false;
    
    if(field->fixedField)
    {
        if(StrEqual(field->fixedField, fieldName))
        {
            result = true;
        }
    }
    return result;
}

#define AddToMetaDefinitions(name, definition) AddToMetaDefinitions_(#name, sizeof(name), ArrayCount(definition), definition)
internal void AddToMetaDefinitions_(char* name, u32 size, u32 fieldCount, FieldDefinition* fields)
{
    Assert(meta_definitionCount < ArrayCount(meta_definitions));
    StructDefinition* definition = meta_definitions + meta_definitionCount++;
    
    FormatString(definition->name, sizeof(definition->name), "%s", name);
    definition->fieldCount = fieldCount;
    definition->size = size;
    definition->fields = fields;
}


global_variable u16 meta_labelTypeCount;
global_variable MetaLabelList meta_labels[Label_Count];
global_variable char* meta_labelsString[Label_Count - 1];
internal u16 GetMetaLabelType(Token labelName)
{
    u16 result = 0;
    for(u16 labelType = 0; labelType < meta_labelTypeCount; ++labelType)
    {
        MetaLabelList* list = meta_labels + labelType;
        if(TokenEquals(labelName, list->name))
        {
            result = labelType;
            break;
        }
    }
    return result;
}

internal char* GetMetaLabelTypeName(u16 value)
{
    char* result = 0;
    if(value < meta_labelTypeCount)
    {
        MetaLabelList* list = meta_labels + value;
        result = list->name;
    }
    return result;
}

internal u16 ExistMetaLabelValue(u16 labelType, Token value)
{
    u16 result = INVALID_LABEL_VALUE;
    Assert(labelType < meta_labelTypeCount);
    MetaLabelList* list = meta_labels + labelType;
    
    for(u16 labelIndex = 0; labelIndex < list->labelCount; ++labelIndex)
    {
        char* name = list->labels[labelIndex];
        if(TokenEquals(value, name))
        {
            result = labelIndex;
            break;
        }
    }
    
    return result;
}

internal char* GetMetaLabelValueName(u16 labelType, u16 labelValue)
{
    char* result = 0;
    if(labelType < meta_labelTypeCount)
    {
        MetaLabelList* list = meta_labels + labelType;
        if(labelValue < list->labelCount)
        {
            result = list->labels[labelValue];
        }
    }
    return result;
}

struct StringArray
{
    char** strings;
    u32 count;
};

internal StringArray GetAssetTypeList()
{
    StringArray result;
    result.strings = &metaAsset_assetType[1];
    result.count = AssetType_Count - 1; // NOTE(Leonardo): avoid the "invalid" asset type
    return result;
}

internal StringArray GetAssetSubtypeList(u16 type)
{
    StringArray result = {};
    
    Assert(type < AssetType_Count);
    MetaAssetType* typeArray = metaAsset_subTypes + type;
    
    result.strings = typeArray->names;
    result.count = typeArray->subtypeCount;
    
    return result;
}

internal StringArray GetLabelTypeList()
{
    StringArray result = {};
    
    result.strings = meta_labelsString;
    result.count = meta_labelTypeCount - 1; // NOTE(Leonardo): avoid the "invalid" label
    
    return result;
}


internal StringArray GetLabelValueList(u16 labelType)
{
    StringArray result = {};
    
    Assert(labelType < meta_labelTypeCount);
    MetaLabelList* list = meta_labels + labelType;
    
    result.strings = list->labels;
    result.count = list->labelCount;
    
    return result;
}


#define AddToMetaLabels(name, labels) AddToMetaLabels_(#name, ArrayCount(labels), labels)
internal void AddToMetaLabels_(char* name, u16 labelCount, char** labels)
{
    Assert(meta_labelTypeCount < ArrayCount(meta_labels));
    MetaLabelList* list = meta_labels + meta_labelTypeCount++;
    
    FormatString(list->name, sizeof(list->name), "%s", name);
    list->labelCount = labelCount;
    list->labels = labels;
}

char* MetaLabels_Invalid[] =
{
    "invalid",
};

internal void LoadMetaData()
{
    META_DEFAULT_VALUES_CPP_SUCKS();
    META_HANDLE_ADD_TO_DEFINITION_HASH();
    AddToMetaDefinitions(Vec2, fieldDefinitionOfVec2);
    AddToMetaDefinitions(Vec3, fieldDefinitionOfVec3);
    AddToMetaDefinitions(Vec4, fieldDefinitionOfVec4);
    
    AddToMetaLabels(INVALID, MetaLabels_Invalid);
    META_LABELS_ADD();
    META_ASSET_LABEL_STRINGS();
}


internal StructDefinition* GetMetaStructDefinition(String name)
{
    StructDefinition* result = 0;
    for(u32 definitionIndex = 0; definitionIndex < meta_definitionCount; ++definitionIndex)
    {
        StructDefinition* definition = meta_definitions + definitionIndex;
        if(StrEqual(name.length, name.ptr, definition->name))
        {
            result = definition;
            break;
        }
    }
    
    return result;
}



internal u32 Parse_u32(Tokenizer* tokenizer, u32 defaultVal)
{
    u32 result = defaultVal;
    
    Token t = GetToken(tokenizer);
    Assert(t.type == Token_Number);
    result = StringToUInt32(t.text);
    
    return result;
}

internal void Dump_u32(Stream* output, FieldDefinition* field, u32 value, b32 isInArray)
{
    if(isInArray)
    {
        OutputToStream(output, "%d", value);
    }
    else
    {
        if(value != field->def.def_u32)
        {
            OutputToStream(output, "%s=%d;", field->name, value);
        }
    }
}


internal r32 Parse_r32(Tokenizer* tokenizer, r32 defaultVal)
{
    r32 result = defaultVal;
    
    Token t = GetToken(tokenizer);
    Assert(t.type == Token_Number);
    result = (r32) StringToFloat(t.text);
    
    return result;
}

internal void Dump_r32(Stream* output, FieldDefinition* field, r32 value, b32 isInArray)
{
    if(isInArray)
    {
        OutputToStream(output, "%f", value);
    }
    else
    {
        if(value != field->def.def_r32)
        {
            OutputToStream(output, "%s=%f;", field->name, value);
        }
    }
}


internal Hash64 Parse_Hash64(Tokenizer* tokenizer, Hash64 defaultVal)
{
    Hash64 result = defaultVal;
    
    Token t = GetToken(tokenizer);
    Assert(t.type == Token_Number);
    result = (Hash64) StringToUInt64(t.text, t.textLength);
    
    return result;
}

internal void Dump_Hash64(Stream* output, FieldDefinition* field, Hash64 value, b32 isInArray)
{
    if(isInArray)
    {
        OutputToStream(output, "%ul", value);
    }
    else
    {
        if(value != field->def.def_Hash64)
        {
            OutputToStream(output, "%s=%ul;", field->name, value);
        }
    }
}

internal u32 PackLabel(GameLabel label)
{
    u32 result = (u32) (label.label << 16) | (u32) (label.value);
    return result;
}

internal GameLabel UnpackLabel(u32 packed)
{
    GameLabel result;
    result.label = (u16) (packed >> 16);
    result.value = (u16) (packed & 0xffff);
    
    return result;
}

internal u32 PackAssetType(GameAssetType type)
{
    u32 result = (u32) (type.type << 16) | (u32) (type.subtype);
    return result;
}

internal GameAssetType UnpackAssetType(u32 packed)
{
    GameAssetType result;
    result.type = (u16) (packed >> 16);
    result.subtype = (u16) (packed & 0xffff);
    
    return result;
}

internal GameLabel Parse_GameLabel(Tokenizer* tokenizer, GameLabel defaultVal)
{
    GameLabel result = defaultVal;
    
    Token t = GetToken(tokenizer);
    Assert(t.type == Token_Number);
    u32 packed = StringToUInt32(t.text);
    result = UnpackLabel(packed);
    return result;
}

internal void Dump_GameLabel(Stream* output, FieldDefinition* field, GameLabel value, b32 isInArray)
{
    u32 packed = PackLabel(value);
    if(isInArray)
    {
        OutputToStream(output, "%d", packed);
    }
    else
    {
        u32 def = PackLabel(field->def.def_GameLabel);
        if(packed != def)
        {
            OutputToStream(output, "%s=%d;", field->name, packed);
        }
    }
}

internal GameAssetType Parse_GameAssetType(Tokenizer* tokenizer, GameAssetType defaultVal)
{
    GameAssetType result = defaultVal;
    
    Token t = GetToken(tokenizer);
    Assert(t.type == Token_Number);
    u32 packed = StringToUInt32(t.text);
    result = UnpackAssetType(packed);
    return result;
}

internal void Dump_GameAssetType(Stream* output, FieldDefinition* field, GameAssetType value, b32 isInArray)
{
    u32 packed = PackAssetType(value);
    if(isInArray)
    {
        OutputToStream(output, "%d", packed);
    }
    else
    {
        u32 def = PackAssetType(field->def.def_GameAssetType);
        if(packed != def)
        {
            OutputToStream(output, "%s=%d;", field->name, packed);
        }
    }
}


internal void ParseVectorMembers(Tokenizer* tokenizer, FieldDefinition* fields, u32 fieldCount, void* ptr)
{
    while(true)
    {
        Token t = GetToken(tokenizer);
        if(t.type == Token_Identifier)
        {
            if(RequireToken(tokenizer, Token_EqualSign))
            {
                r32 value = Parse_r32(tokenizer, 0);
                FieldDefinition* field = FindMetaField(fields, fieldCount, t);
                if(field)
                {
                    r32* dest = (r32*) ((u8*) ptr + field->offset);
                    *dest = value;
                }
            }
        }
        else if(t.type == Token_CloseBraces)
        {
            break;
        }
    }
}

internal Vec2 Parse_Vec2(Tokenizer* tokenizer, Vec2 defaultVal)
{
    Vec2 result = defaultVal;
    if(RequireToken(tokenizer, Token_OpenBraces))
    {
        FieldDefinition* fields = fieldDefinitionOfVec2;
        u32 count = ArrayCount(fieldDefinitionOfVec2);
        
        ParseVectorMembers(tokenizer, fields, count, &result);
    }
    else
    {
        InvalidCodePath;
    }
    
    return result;
}


#define OUTPUT_VECTOR_ELEMENT(v, el) if(value.el != field->def.def_##v.el) {OutputToStream(output, "%s=%f;", #el, value.el);}

internal void Dump_Vec2(Stream* output, FieldDefinition* field, Vec2 value, b32 isInArray)
{
    if(isInArray)
    {
        OutputToStream(output, "{");
        OUTPUT_VECTOR_ELEMENT(Vec2, x);
        OUTPUT_VECTOR_ELEMENT(Vec2, y);
        OutputToStream(output, "}");
    }
    else
    {
        if(value != field->def.def_Vec2)
        {
            OutputToStream(output, "{", field->name);
            OUTPUT_VECTOR_ELEMENT(Vec2, x);
            OUTPUT_VECTOR_ELEMENT(Vec2, y);
            OutputToStream(output, "};");
        }
    }
}

internal Vec3 Parse_Vec3(Tokenizer* tokenizer, Vec3 defaultVal)
{
    Vec3 result = defaultVal;
    if(RequireToken(tokenizer, Token_OpenBraces))
    {
        ParseVectorMembers(tokenizer, fieldDefinitionOfVec3, ArrayCount(fieldDefinitionOfVec3), &result);
    }
    else
    {
        InvalidCodePath;
    }
    
    return result;
}

internal void Dump_Vec3(Stream* output, FieldDefinition* field, Vec3 value, b32 isInArray)
{
    if(isInArray)
    {
        OutputToStream(output, "{");
        OUTPUT_VECTOR_ELEMENT(Vec3, x);
        OUTPUT_VECTOR_ELEMENT(Vec3, y);
        OUTPUT_VECTOR_ELEMENT(Vec3, z);
        OutputToStream(output, "}");
    }
    else
    {
        if(value != field->def.def_Vec3)
        {
            OutputToStream(output, "%s={", field->name);
            OUTPUT_VECTOR_ELEMENT(Vec3, x);
            OUTPUT_VECTOR_ELEMENT(Vec3, y);
            OUTPUT_VECTOR_ELEMENT(Vec3, z);
            OutputToStream(output, "};");
        }
    }
}

internal Vec4 Parse_Vec4(Tokenizer* tokenizer, Vec4 defaultVal)
{
    Vec4 result = defaultVal;
    if(RequireToken(tokenizer, Token_OpenBraces))
    {
        ParseVectorMembers(tokenizer, fieldDefinitionOfVec4, ArrayCount(fieldDefinitionOfVec4), &result);
    }
    else
    {
        InvalidCodePath;
    }
    
    return result;
}

internal void Dump_Vec4(Stream* output, FieldDefinition* field, Vec4 value, b32 isInArray)
{
    if(isInArray)
    {
        OutputToStream(output, "{");
        if(value != field->def.def_Vec4)
        {
            OUTPUT_VECTOR_ELEMENT(Vec4, x);
            OUTPUT_VECTOR_ELEMENT(Vec4, y);
            OUTPUT_VECTOR_ELEMENT(Vec4, z);
            OUTPUT_VECTOR_ELEMENT(Vec4, w);
        }
        OutputToStream(output, "}");
    }
    else
    {
        if(value != field->def.def_Vec4)
        {
            OutputToStream(output, "%s={", field->name);
            OUTPUT_VECTOR_ELEMENT(Vec4, x);
            OUTPUT_VECTOR_ELEMENT(Vec4, y);
            OUTPUT_VECTOR_ELEMENT(Vec4, z);
            OUTPUT_VECTOR_ELEMENT(Vec4, w);
            OutputToStream(output, "};");
        }
    }
}



struct StructOperationResult
{
    u32 size;
    b32 deleted;
};

enum FieldOperationType
{
    FieldOperation_GetSize,
    FieldOperation_Dump,
    FieldOperation_Edit,
    FieldOperation_Parse,
};

struct EditorLayout;
internal b32 Edit_u32(EditorLayout* layout, char* name, u32* number, b32 isInArray);
internal b32 Edit_u16(EditorLayout* layout, char* name, u16* number, b32 isInArray);
internal b32 Edit_r32(EditorLayout* layout, char* name, r32* number, b32 isInArray);
internal b32 Edit_Vec2(EditorLayout* layout, char* name, Vec2* v, b32 isInArray);
internal b32 Edit_Vec3(EditorLayout* layout, char* name, Vec3* v, b32 isInArray);
internal b32 Edit_Vec4(EditorLayout* layout, char* name, Vec4 * v, b32 isInArray);
internal b32 Edit_Hash64(EditorLayout* layout, char* name, Hash64* h, char* optionsName, b32 isInArray);
internal b32 Edit_GameLabel(EditorLayout* layout, char* name, GameLabel* label, b32 isInArray);
internal b32 Edit_GameAssetType(EditorLayout* layout, char* name, GameAssetType* type, b32 typeEditable, b32 isInArray);
internal void NextRaw(EditorLayout* layout);
internal void Nest(EditorLayout* layout);
internal void Push(EditorLayout* layout);
internal void Pop(EditorLayout* layout);
internal Rect2 EditorTextDraw(EditorLayout* layout, Vec4 color, u32 flags, char* format, ...);
internal b32 EditorCollapsible(EditorLayout* layout, char* string, AUID ID);
internal b32 StandardEditorButton(EditorLayout* layout, char* name, AUID ID, Vec4 color);

#define DUMB_OPERATION_BOILERPLATE_(type)\
case FieldOperation_GetSize:{} break;\
case FieldOperation_Parse:{*((type*)ptr) = value;} break;

#ifdef FORG_SERVER
#define DUMB_OPERATION_BOILERPLATE(type) DUMB_OPERATION_BOILERPLATE_(type)
#else
#define DUMB_OPERATION_BOILERPLATE(type)\
DUMB_OPERATION_BOILERPLATE_(type)\
case FieldOperation_Edit:{result.deleted = Edit_##type(layout, field->name, (type*) ptr, isInArray);} break;
#endif

#define DUMB_INIT_BOILERPLATE(type)\
result.size = sizeof(type);\
type value = field->def.def_##type;\
if(source){value = Parse_##type(source, value);}

#define STANDARD_OPERATION_FUNCTION(type)\
internal StructOperationResult type##Operation(EditorLayout* layout, FieldDefinition* field, FieldOperationType operation, void* ptr, Tokenizer* source, Stream* output, b32 isInArray)\
{\
    StructOperationResult result = {};\
    DUMB_INIT_BOILERPLATE(type);\
    switch(operation)\
    {\
        DUMB_OPERATION_BOILERPLATE(type);\
        case FieldOperation_Dump:\
        {\
            value = *(type*) ptr;\
            Dump_##type(output, field, value, isInArray);\
        } break;\
        InvalidDefaultCase;\
    }\
    return result;\
}

STANDARD_OPERATION_FUNCTION(u32);
STANDARD_OPERATION_FUNCTION(r32);
STANDARD_OPERATION_FUNCTION(GameLabel);
STANDARD_OPERATION_FUNCTION(Vec2);
STANDARD_OPERATION_FUNCTION(Vec3);
STANDARD_OPERATION_FUNCTION(Vec4);

internal StructOperationResult Hash64Operation(EditorLayout* layout, FieldDefinition* field, FieldOperationType operation, void* ptr, Tokenizer* source, Stream* output, b32 isInArray)
{
    StructOperationResult result = {};
    DUMB_INIT_BOILERPLATE(Hash64);
    switch(operation)
    {
        DUMB_OPERATION_BOILERPLATE_(Hash64);
#ifndef FORG_SERVER
        case FieldOperation_Edit:
        {
            Edit_Hash64(layout, field->name, (Hash64*) ptr, field->optionsName, isInArray);
        } break;
#endif
        case FieldOperation_Dump:
        {
            value = *(Hash64*) ptr;
            Dump_Hash64(output, field, value, isInArray);
        } break;
        
        InvalidDefaultCase;
    }
    
    return result;
}

internal StructOperationResult GameAssetTypeOperation(EditorLayout* layout, FieldDefinition* field, FieldOperationType operation, void* ptr, Tokenizer* source, Stream* output, b32 isInArray)
{
    StructOperationResult result = {};
    DUMB_INIT_BOILERPLATE(GameAssetType);
    switch(operation)
    {
        DUMB_OPERATION_BOILERPLATE_(GameAssetType);
#ifndef FORG_SERVER
        case FieldOperation_Edit:
        {
            GameAssetType* type = (GameAssetType*) ptr;
            b32 typeEditable = !IsFixedField(field, type, type);
            Edit_GameAssetType(layout, field->name, type, typeEditable, isInArray);
        } break;
#endif
        
        case FieldOperation_Dump:
        {
            value = *(GameAssetType*) ptr;
            Dump_GameAssetType(output, field, value, isInArray);
        } break;
        InvalidDefaultCase;
    }
    
    return result;
}




struct ReservedSpace
{
    void* ptr;
    u32 size;
};

internal void* ReserveSpace(ReservedSpace* space, u32 size)
{
    Assert(size <= space->size);
    void* result = space->ptr;
    space->ptr = (void*) ((u8*) space->ptr + size);
    space->size -= size;
    
    return result;
}

struct MetaArrayHeader
{
    u16 count;
    u16 maxCount;
};

struct MetaArrayTrailer
{
    void* nextBlock;
};

internal void* InitMetaArrayBlock(void* memory, u32 elementCount, u32 filled, u32 elementSize)
{
    MetaArrayHeader* header = (MetaArrayHeader*) memory;
    
    Assert(filled <= elementCount);
    header->count = SafeTruncateToU16(filled);
    header->maxCount = SafeTruncateToU16(elementCount);
    
    void* result = (header + 1);
    void* trailerPtr = (void*) ((u8*) result + (elementCount * elementSize));
    MetaArrayTrailer* trailer = (MetaArrayTrailer*) trailerPtr;
    trailer->nextBlock = 0;
    
    return result;
}

internal MetaArrayHeader* GetHeader(void* dataPtr)
{
    MetaArrayHeader* result = (MetaArrayHeader*) dataPtr - 1;
    return result;
}

internal MetaArrayTrailer* GetTrailer(void* dataPtr, u32 elementSize)
{
    MetaArrayHeader* header = GetHeader(dataPtr);
    MetaArrayTrailer* result = (MetaArrayTrailer*) ((u8*) dataPtr + header->maxCount * elementSize);
    
    return result;
    
}

internal StructOperationResult StructOperation(EditorLayout* layout, String structName, Tokenizer* tokenizer, void* dataPtr, FieldOperationType operation, Stream* output, ReservedSpace* reserved, b32 parentWasPointer);
internal u32 FieldOperation(EditorLayout* layout, FieldDefinition* field, FieldOperationType operation, void* fieldPtr, Tokenizer* tokenizer, Stream* output, ReservedSpace* reserved, u16* elementCount)
{
    u32 result = 0;
    b32 pointer = (field->flags & MetaFlag_Pointer);
    u16 elements = 0;
    
    Assert(*elementCount);
    u16* blockElementCount = elementCount;
    if(pointer)
    {
        MetaArrayHeader* header = GetHeader(fieldPtr);
        blockElementCount = &header->count;
        if(operation == FieldOperation_Dump)
        {
            OutputToStream(output, "%s=", field->name);
        }
    }
    
    while(true)
    {
        ++elements;
#ifndef FORG_SERVER
        if(operation == FieldOperation_Edit)
        {
            NextRaw(layout);
        }
#endif
        
        StructOperationResult op = {};
        
#define DUMB_OPERATION(type) case MetaType_##type:{op = type##Operation(layout, field, operation, fieldPtr, tokenizer, output, pointer);} break;
        switch(field->type)
        {
            DUMB_OPERATION(u32);
            DUMB_OPERATION(r32);
            DUMB_OPERATION(Vec2);
            DUMB_OPERATION(Vec3);
            DUMB_OPERATION(Vec4);
            DUMB_OPERATION(Hash64);
            DUMB_OPERATION(GameLabel);
            DUMB_OPERATION(GameAssetType);
            
            default:
            {
                Assert(field->type > MetaType_FirstCustomMetaType);
                if(pointer && operation == FieldOperation_Dump)
                {
                    OutputToStream(output, "{");
                }
                
                String structName = {};
                structName.ptr = field->typeName;
                structName.length = StrLen(field->typeName);
                op = StructOperation(layout, structName, tokenizer, fieldPtr, operation, output, reserved, pointer);
                
                if(pointer && operation == FieldOperation_Dump)
                {
                    OutputToStream(output, "}");
                }
            } break;
        }
        
        if(op.deleted)
        {
            --*elementCount;
            Assert(*blockElementCount > 0);
            
            u32 offset = field->size * --*blockElementCount;
            void* targetPtr = AdvanceVoidPtrBytes(fieldPtr, offset);
            void* destPtr = fieldPtr;
            void* sourcePtr = AdvanceVoidPtrBytes(fieldPtr, field->size);
            
            while(destPtr != targetPtr)
            {
                Copy(field->size, destPtr, sourcePtr);
                destPtr = sourcePtr;
                sourcePtr = AdvanceVoidPtrBytes(sourcePtr, field->size);
            }
        }
        
        if(pointer)
        {
            if(operation == FieldOperation_Dump || operation == FieldOperation_Edit)
            {
                if(elements >= *blockElementCount)
                {
                    break;
                }
                else
                {
                    if(output)
                    {
                        OutputToStream(output, ",");
                    }
                    fieldPtr = (void*) ((u8*) fieldPtr + field->size);
                }
            }
            else
            {
                result += op.size;
                Token t = GetToken(tokenizer);
                if(t.type == Token_Comma)
                {
                    // NOTE(Leonardo): advance in the array and continue!
                    if(operation == FieldOperation_Parse)
                    {
                        fieldPtr = (void*) ((u8*) fieldPtr + field->size);
                    }
                }
                else
                {
                    Assert(t.type == Token_SemiColon);
                    break;
                }
            }
        }
        else
        {
            break;
        }
    }
    
    if(operation == FieldOperation_GetSize)
    {
        *elementCount = elements;
    }
    
    if(pointer && operation == FieldOperation_Dump)
    {
        OutputToStream(output, ";");
    }
    
    return result;
}

internal void CopyDefaultValue(FieldDefinition* field, void* ptr)
{
    if(field->flags & MetaFlag_Pointer)
    {
        Assert(sizeof(u64) == sizeof(void*));
        *(u64*) ptr = (u64) 0;
    }
    else
    {
#define DUMB_COPY_DEF(type) case MetaType_##type: {*(type*) ptr = field->def.def_##type;} break;
        switch(field->type)
        {
            DUMB_COPY_DEF(u8);
            DUMB_COPY_DEF(u16);
            DUMB_COPY_DEF(u32);
            DUMB_COPY_DEF(u64);
            DUMB_COPY_DEF(i8);
            DUMB_COPY_DEF(i16);
            DUMB_COPY_DEF(i32);
            DUMB_COPY_DEF(i64);
            DUMB_COPY_DEF(b32);
            DUMB_COPY_DEF(r32);
            DUMB_COPY_DEF(Vec2);
            DUMB_COPY_DEF(Vec3);
            DUMB_COPY_DEF(Vec4);
            DUMB_COPY_DEF(Hash64);
            DUMB_COPY_DEF(ArrayCounter);
            DUMB_COPY_DEF(GameLabel);
            DUMB_COPY_DEF(GameAssetType);
            
            InvalidDefaultCase;
        }
    }
}

internal ArrayCounter* GetMetaPtrElementCountForArray(StructDefinition* definition, FieldDefinition* arrayField, void* structPtr)
{
    ArrayCounter* result = 0;
    for(u32 fieldIndex = 0; fieldIndex < definition->fieldCount; ++fieldIndex)
    {
        FieldDefinition* field = definition->fields + fieldIndex;
        if(StrEqual(field->counterName, arrayField->name))
        {
            Assert(field->type == MetaType_ArrayCounter);
            result = (ArrayCounter*) ((u8*) structPtr + field->offset);
            break;
        }
    }
    
    return result;
}

internal void InitStructDefault(StructDefinition* definition, void* dataPtr)
{
    for(u32 fieldIndex = 0; fieldIndex < definition->fieldCount; ++fieldIndex)
    {
        FieldDefinition* field = definition->fields + fieldIndex;
        void* fieldPtr = (void*) ((u8*) dataPtr + field->offset);
        CopyDefaultValue(field, fieldPtr);
    }
}

internal void InitFieldDefault(FieldDefinition* field, void* dataPtr)
{
    String structName = Stringize(field->typeName);
    StructDefinition* structDefinition = GetMetaStructDefinition(structName);
    if(structDefinition)
    {
        InitStructDefault(structDefinition, dataPtr);
    }
    else
    {
        CopyDefaultValue(field, dataPtr);
    }
}

internal StructOperationResult StructOperation(EditorLayout* layout, String structName, Tokenizer* tokenizer, void* dataPtr, FieldOperationType operation, Stream* output, ReservedSpace* reserved, b32 parentWasPointer = false)
{
    StructOperationResult result = {};
    StructDefinition* definition = GetMetaStructDefinition(structName);
    result.size += definition->size;
    
    if(operation == FieldOperation_Parse)
    {
        InitStructDefault(definition, dataPtr);
    }
    
    if(operation == FieldOperation_Dump || operation == FieldOperation_Edit)
    {
        b32 canProceed = true;
        
#ifndef FORG_SERVER
        if(operation == FieldOperation_Edit)
        {
            canProceed = EditorCollapsible(layout, 0, auID(dataPtr, "structCollapse"));
            EditorTextDraw(layout, V4(1, 1, 1, 1), EditorText_StartingSpace, "%.*s {}", structName.length, structName.ptr);
            
            if(parentWasPointer)
            {
                if(StandardEditorButton(layout, "canc", auID(dataPtr, "cancButton"), V4(0, 0.5f, 1.0f, 1.0f)))
                {
                    result.deleted = true;
                }
            }
        }
#endif
        
        if(canProceed)
        {
#ifndef FORG_SERVER
            Push(layout);
#endif
            for(u32 fieldIndex = 0; fieldIndex < definition->fieldCount; ++fieldIndex)
            {
                FieldDefinition* field = definition->fields + fieldIndex;
                b32 pointer = (field->flags & MetaFlag_Pointer);
                
                // NOTE(Leonardo): we can't dump nor edit counters!
                if(field->type != MetaType_ArrayCounter)
                {
                    void* originalfieldPtr = (void*) ((u8*) dataPtr + field->offset);
                    void* fieldPtr = originalfieldPtr;
                    
                    u16 fakeElementCount = 1;
                    ArrayCounter* elementCount = &fakeElementCount;
                    b32 show = true;
                    
                    if(pointer)
                    {
                        result.size += (sizeof(MetaArrayHeader) + sizeof(MetaArrayTrailer));
                        
                        elementCount = GetMetaPtrElementCountForArray(definition, field, dataPtr);
                        Assert(elementCount);
                        Assert(sizeof(u64) == sizeof(void*));
                        MetaArrayHeader* header = (MetaArrayHeader*) (*(u64*)fieldPtr);
                        fieldPtr = (void*) (header + 1);
#ifndef FORG_SERVER                
                        if(operation == FieldOperation_Edit)
                        {
                            NextRaw(layout);
                            show = EditorCollapsible(layout, 0, auID(originalfieldPtr, "collapsible"));
                            EditorTextDraw(layout, V4(1, 1, 1, 1), 0, "%s [%d]", field->name, *elementCount);
                            
                            if(StandardEditorButton(layout, "add", auID(originalfieldPtr, "addButton"), V4(0, 1.0f, 1.0f, 1.0f)))
                            {
                                u32 newElementCount = 32;
                                
                                
                                *elementCount = *elementCount + 1;
                                if(*elementCount == 1)
                                {
                                    void* memory = PushSize(layout->context->pool, newElementCount * field->size + sizeof(MetaArrayHeader) + sizeof(MetaArrayTrailer));
                                    InitMetaArrayBlock(memory, newElementCount, 0, field->size);
                                    *(u64*)originalfieldPtr = (u64) memory;
                                    header = (MetaArrayHeader*) memory;
                                    fieldPtr = header + 1;
                                }
                                
                                
                                void* here = 0;
                                MetaArrayHeader* currentHeader = header;
                                while(!here)
                                {
                                    void* arrayPtr = (void*) (currentHeader + 1);
                                    
                                    MetaArrayTrailer* trailer = GetTrailer(arrayPtr, field->size);
                                    if(currentHeader->count < currentHeader->maxCount)
                                    {
                                        here = (void*)((u8*)arrayPtr + currentHeader->count++ * field->size);
                                    }
                                    else
                                    {
                                        if(!trailer->nextBlock)
                                        {
                                            void* memory = PushSize(layout->context->pool, newElementCount * field->size + sizeof(MetaArrayHeader) + sizeof(MetaArrayTrailer));
                                            InitMetaArrayBlock(memory, newElementCount, 0, field->size);
                                            
                                            trailer->nextBlock = memory;
                                        }
                                        
                                        currentHeader = (MetaArrayHeader*) trailer->nextBlock;
                                    }
                                }
                                
                                InitFieldDefault(field, here);
                            }
                        }
#endif
                    }
                    
                    if(*elementCount && show)
                    {
#ifndef FORG_SERVER
                        if(pointer && operation == FieldOperation_Edit)
                        {
                            Push(layout);
                        }
#endif
                        
                        while(true)
                        {
                            b32 showBlock = true;
                            if(pointer)
                            {
                                MetaArrayHeader* header = GetHeader(fieldPtr);
                                showBlock = (header->count > 0);
                            }
                            
                            if(showBlock)
                            {
                                result.size += FieldOperation(layout, field, operation, fieldPtr, tokenizer, output, reserved, elementCount);
                            }
                            if(pointer)
                            {
                                MetaArrayTrailer* trailer = GetTrailer(fieldPtr, field->size);
                                if(trailer->nextBlock)
                                {
                                    MetaArrayHeader* nextHeader = (MetaArrayHeader*) trailer->nextBlock;
                                    fieldPtr = nextHeader + 1;
                                }
                                else
                                {
                                    break;
                                }
                            }
                            else
                            {
                                break;
                            }
                        }
#ifndef FORG_SERVER
                        if(pointer && operation == FieldOperation_Edit)
                        {
                            Pop(layout);
                        }
#endif
                        
                    }
                }
            }
            
#ifndef FORG_SERVER
            Pop(layout);
#endif
        }
    }
    else
    {
        Token o = GetToken(tokenizer);
        Assert(o.type == Token_OpenBraces);
        for(;;)
        {
            Token t = GetToken(tokenizer);
            if(t.type == Token_CloseBraces)
            {
                break;
            }
            else if(t.type == Token_SemiColon)
            {
            }
            else if(t.type == Token_EndOfFile)
            {
                break;
            }
            else
            {
                Token fieldName = Stringize(t);
                FieldDefinition* field = FindMetaField(definition, fieldName);
                Assert(field->type != MetaType_ArrayCounter);
                
                if(RequireToken(tokenizer, Token_EqualSign))
                {
                    void* fieldPtr = (void*) ((u8*) dataPtr + field->offset);
                    u32 additionalSize = sizeof(MetaArrayHeader) + sizeof(MetaArrayTrailer);
                    if(field->flags & MetaFlag_Pointer)
                    {
                        result.size += additionalSize;
                    }
                    
                    b32 valid = true;
                    if(operation == FieldOperation_Parse && field->flags & MetaFlag_Pointer)
                    {
                        Tokenizer fake = {};
                        fake.at = tokenizer->at;
                        
                        u16 elementCount;
                        FieldOperation(layout, field, FieldOperation_GetSize, fieldPtr, &fake, output, reserved, &elementCount);
                        
                        ArrayCounter* counterPtr = GetMetaPtrElementCountForArray(definition, field, dataPtr);
                        Assert(counterPtr);
                        *counterPtr = elementCount;
                        
                        Assert(sizeof(u64) == sizeof(void*));
                        if(elementCount)
                        {
                            void* headerPtr = ReserveSpace(reserved, elementCount * field->size + additionalSize);
                            void* arrayPtr = InitMetaArrayBlock(headerPtr, elementCount, elementCount, field->size);
                            
                            *(u64*) fieldPtr = (u64) headerPtr;
                            fieldPtr = arrayPtr;
                        }
                        else
                        {
                            *(u64*) fieldPtr = 0;
                        }
                    }
                    
                    if(valid)
                    {
                        u16 ignored = 1;
                        result.size += FieldOperation(layout, field, operation, fieldPtr, tokenizer, output, reserved, &ignored);
                    }
                }
                else
                {
                    InvalidCodePath;
                }
            }
            
        }
    }
    
    return result;
}

internal u32 GetStructSize(String structName, Tokenizer* tokenizer)
{
    ReservedSpace ignored = {};
    u32 result = StructOperation(0, structName, tokenizer, 0, FieldOperation_GetSize, 0, &ignored).size;
    return result;
}

internal void ParseBufferIntoStruct(String structName, Tokenizer* tokenizer, void* structPtr, u32 reservedSize)
{
    ReservedSpace reserved = {};
    StructDefinition* definition = GetMetaStructDefinition(structName);
    reserved.ptr = (void*) ((u8*) structPtr + definition->size);
    reserved.size = reservedSize - definition->size;
    
    StructOperation(0, structName, tokenizer, structPtr, FieldOperation_Parse, 0, &reserved);
    
    Assert(reserved.size == 0);
}

internal void DumpStructToStream(String structName, Stream* dest, void* structPtr)
{
    ReservedSpace ignored = {};
    OutputToStream(dest, "{");
    StructOperation(0, structName, 0, structPtr, FieldOperation_Dump, dest, &ignored);
    OutputToStream(dest, "}");
}
