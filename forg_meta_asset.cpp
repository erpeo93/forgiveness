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


global_variable u16 meta_propertyTypeCount;
global_variable MetaPropertyList meta_properties[Property_Count];
global_variable char* meta_propertiesString[Property_Count];
internal u16 GetMetaPropertyType(Token propertyName)
{
    u16 result = 0;
    for(u16 propertyType = 0; propertyType < meta_propertyTypeCount; ++propertyType)
    {
        MetaPropertyList* list = meta_properties + propertyType;
        if(TokenEquals(propertyName, list->name))
        {
            result = propertyType;
            break;
        }
    }
    return result;
}

internal char* GetMetaPropertyTypeName(u16 value)
{
    char* result = 0;
    if(value < meta_propertyTypeCount)
    {
        MetaPropertyList* list = meta_properties + value;
        result = list->name;
    }
    return result;
}

internal u16 ExistMetaPropertyValue(u16 propertyType, Token value)
{
    u16 result = INVALID_PROPERTY_VALUE;
    Assert(propertyType < meta_propertyTypeCount);
    MetaPropertyList* list = meta_properties + propertyType;
    
    for(u16 propertyIndex = 0; propertyIndex < list->propertyCount; ++propertyIndex)
    {
        char* name = list->properties[propertyIndex];
        if(TokenEquals(value, name))
        {
            result = propertyIndex;
            break;
        }
    }
    
    return result;
}

internal char* GetMetaPropertyValueName(u16 propertyType, u16 propertyValue)
{
    char* result = 0;
    if(propertyType < meta_propertyTypeCount)
    {
        MetaPropertyList* list = meta_properties + propertyType;
        if(propertyValue < list->propertyCount)
        {
            result = list->properties[propertyValue];
        }
    }
    return result;
}

struct StringArray
{
    char** strings;
    u32 count;
};

internal u32 MetaGetCurrentVersion(char* assetType)
{
    u32 result = 0;
    if(StrEqual(assetType, "Image"))
    {
        result = PAK_BITMAP_VERSION;
    }
    else if(StrEqual(assetType, "Font"))
    {
        result = PAK_FONT_VERSION;
    }
    else if(StrEqual(assetType, "Sound"))
    {
        result = PAK_SOUND_VERSION;
    }
    else if(StrEqual(assetType, "Skeleton"))
    {
        result = PAK_ANIMATION_VERSION;
    }
    else if(StrEqual(assetType, "Model"))
    {
        result = PAK_MODEL_VERSION;
    }
    else
    {
        result = PAK_DATA_VERSION;
    }
    
    return result;
}

internal StringArray GetAssetTypeList()
{
    StringArray result;
    result.strings = metaAsset_assetType;
    result.count = AssetType_Count; 
    return result;
}

internal StringArray GetPropertyTypeList()
{
    StringArray result = {};
    
    result.strings = meta_propertiesString;
    result.count = meta_propertyTypeCount;
    
    return result;
}


internal StringArray GetPropertyValueList(u16 propertyType)
{
    StringArray result = {};
    
    Assert(propertyType < meta_propertyTypeCount);
    MetaPropertyList* list = meta_properties + propertyType;
    
    result.strings = list->properties;
    result.count = list->propertyCount;
    
    return result;
}

#define AddToMetaProperties(name, properties) AddToMetaProperties_(#name, ArrayCount(properties), properties)
internal void AddToMetaProperties_(char* name, u16 propertyCount, char** properties)
{
    Assert(meta_propertyTypeCount < ArrayCount(meta_properties));
    MetaPropertyList* list = meta_properties + meta_propertyTypeCount++;
    
    FormatString(list->name, sizeof(list->name), "%s", name);
    list->propertyCount = propertyCount;
    list->properties = properties;
}

char* MetaTable_Invalid[] =
{
    "invalid",
};


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

global_variable Assets* meta_assets;
internal AssetLabel Parse_AssetLabel(Tokenizer* tokenizer, AssetLabel defaultVal)
{
    AssetLabel result = defaultVal;
    Token t = GetToken(tokenizer);
    Assert(t.type == Token_String);
    t = Stringize(t);
    FormatString(result.name, sizeof(result.name), "%.*s", t.textLength, t.text);
    return result;
}


internal void Dump_AssetLabel(Stream* output, FieldDefinition* field, AssetLabel label, b32 isInArray)
{
    Assert(!isInArray);
    if(!StrEqual(label.name, "null"))
    {
        OutputToStream(output, "%s=\"%s\";", field->name, label.name);
    }
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

internal u16 Parse_u16(Tokenizer* tokenizer, u16 defaultVal)
{
    u16 result = defaultVal;
    Token t = GetToken(tokenizer);
    Assert(t.type == Token_Number);
    result = (u16) StringToUInt32(t.text);
    return result;
}

internal void Dump_u16(Stream* output, FieldDefinition* field, u16 value, b32 isInArray)
{
    if(isInArray)
    {
        OutputToStream(output, "%d", value);
    }
    else
    {
        if(value != field->def.def_u16)
        {
            OutputToStream(output, "%s=%d;", field->name, value);
        }
    }
}

internal i32 Parse_i32(Tokenizer* tokenizer, i32 defaultVal)
{
    i32 result = defaultVal;
    
    Token t = GetToken(tokenizer);
    Assert(t.type == Token_Number);
    result = StringToInt32(t.text);
    
    return result;
}

internal void Dump_i32(Stream* output, FieldDefinition* field, i32 value, b32 isInArray)
{
    if(isInArray)
    {
        OutputToStream(output, "%d", value);
    }
    else
    {
        if(value != field->def.def_i32)
        {
            OutputToStream(output, "%s=%d;", field->name, value);
        }
    }
}

internal Enumerator Parse_Enumerator(Tokenizer* tokenizer, Enumerator defaultVal)
{
    Enumerator result = defaultVal;
    Token t = GetToken(tokenizer);
    if(t.type == Token_String)
    {
        t = Stringize(t);
        Assert(t.textLength <= sizeof(result.value));
        FormatString(result.value, sizeof(result.value), "%.*s", t.textLength, t.text);
    }
    
    return result;
}

internal void Dump_Enumerator(Stream* output, FieldDefinition* field, Enumerator value, b32 isInArray)
{
    if(isInArray)
    {
        InvalidCodePath;
    }
    else
    {
        if(value.value[0])
        {
            OutputToStream(output, "%s=\"%s\";", field->name, value.value);
        }
    }
}

internal r32 Parse_r32(Tokenizer* tokenizer, r32 defaultVal)
{
    r32 result = defaultVal;
    
    Token t = GetToken(tokenizer);
    Assert(t.type == Token_Number);
    result = (r32) StringToR32(t.text);
    
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

internal u16 GetAssetIndex(Assets* assets, u16 type, u32 subtype, Token indexName);
internal char* GetAssetIndexName(Assets* assets, AssetID ID);
internal u32 GetAssetSubtype(Assets* assets, u16 type, Token subtype);
internal u32 GetAssetSubtype(Assets* assets, u16 type, char* subtype);
internal char* GetAssetSubtypeName(Assets* assets, u16 type, u32 subtype);
internal char* GetAssetSubtypeName(Assets* assets, u16 type, u64 subtypeHash);

internal EntityRef Parse_EntityRef(Tokenizer* tokenizer, EntityRef defaultVal)
{
    EntityRef result = defaultVal;
    
    Token t = GetToken(tokenizer);
    if(t.type == Token_String)
    {
        t = Stringize(t);
    }
    
    Token kind = {};
    kind.text = t.text;
    kind.textLength = FindFirstInString(t.text, '|');
    
    if(kind.textLength != 0xffffffff)
    {
        Token index = {};
        index.text = kind.text + kind.textLength + 1;
        index.textLength = t.textLength - kind.textLength - 1;
        
        result.subtypeHashIndex = GetAssetSubtype(meta_assets, AssetType_EntityDefinition, kind);
        result.index = GetAssetIndex(meta_assets, AssetType_EntityDefinition, result.subtypeHashIndex, index);
    }
    
    return result;
}

internal void Dump_EntityRef(Stream* output, FieldDefinition* field, EntityRef value, b32 isInArray)
{
    if(value.subtypeHashIndex || value.index)
    {
        char* subtypeName = GetAssetSubtypeName(meta_assets, AssetType_EntityDefinition, value.subtypeHashIndex);
        
        AssetID ID;
        ID.type = AssetType_EntityDefinition;
        ID.subtypeHashIndex = value.subtypeHashIndex;
        ID.index = value.index;
        char* indexName = GetAssetIndexName(meta_assets, ID);
        
        if(isInArray)
        {
            OutputToStream(output, "\"%s|%s\"", subtypeName, indexName);
        }
        else
        {
            OutputToStream(output, "%s=\"%s|%s\";", field->name, subtypeName, indexName);
        }
    }
}

internal GameProperty Parse_GameProperty(Tokenizer* tokenizer, GameProperty defaultVal)
{
    GameProperty result = defaultVal;
    
    Token t = GetToken(tokenizer);
    if(t.type == Token_String)
    {
        t = Stringize(t);
    }
    
    Token name = {};
    name.text = t.text;
    name.textLength = FindFirstInString(t.text, '|');
    
    if(name.textLength != 0xffffffff)
    {
        Token value = {};
        value.text = name.text + name.textLength + 1;
        value.textLength = t.textLength - name.textLength - 1;
        
        result.property = GetMetaPropertyType(name);
        result.value = ExistMetaPropertyValue(result.property, value);
    }
    
    return result;
}

internal void Dump_GameProperty(Stream* output, FieldDefinition* field, GameProperty value, b32 isInArray)
{
    if(value.property)
    {
        char* propertyType = GetMetaPropertyTypeName(value.property);
        char* propertyValue = GetMetaPropertyValueName(value.property, value.value);
        
        if(isInArray)
        {
            OutputToStream(output, "\"%s|%s\"", propertyType, propertyValue);
        }
        else
        {
            OutputToStream(output, "%s=\"%s|%s\";", field->name, propertyType, propertyValue);
        }
    }
}


internal GameAssetType Parse_GameAssetType(Tokenizer* tokenizer, GameAssetType defaultVal)
{
    GameAssetType result = defaultVal;
    
    Token t = GetToken(tokenizer);
    if(t.type == Token_String)
    {
        t = Stringize(t);
    }
    
    Token type = {};
    type.text = t.text;
    type.textLength = Min(t.textLength, (i32) FindFirstInString(t.text, '|'));
    
    if(type.textLength != 0xffffffff)
    {
        Token subtype = {};
        subtype.text = type.text + type.textLength + 1;
        subtype.textLength = Max(t.textLength - type.textLength - 1, 0);
        
        char type_[32];
        char subtype_[32];
        
        FormatString(type_, sizeof(type_), "%.*s", type.textLength, type.text);
        FormatString(subtype_, sizeof(subtype_), "%.*s", subtype.textLength, subtype.text);
        
        result.type = GetMetaAssetType(type_);
        result.subtypeHash = StringHash(subtype_);
    }
    
    return result;
}

internal void Dump_GameAssetType(Stream* output, FieldDefinition* field, GameAssetType value, b32 isInArray)
{
    if(value.type)
    {
        char* type = GetAssetTypeName(value.type);
        char* subtype = GetAssetSubtypeName(meta_assets, value.type, value.subtypeHash);
        
        if(isInArray)
        {
            OutputToStream(output, "\"%s|%s\"", type, subtype);
        }
        else
        {
            OutputToStream(output, "%s=\"%s|%s\";", field->name, type, subtype);
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
            OutputToStream(output, "%s={", field->name);
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

internal Color Parse_Color(Tokenizer* tokenizer, Color defaultVal)
{
    Vec4 result = Parse_Vec4(tokenizer, defaultVal);
    return result;
}

internal void Dump_Color(Stream* output, FieldDefinition* field, Color value, b32 isInArray)
{
    Dump_Vec4(output, field, value, isInArray);
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
internal b32 Edit_u32(EditorLayout* layout, char* name, u32* number, b32 isInArray, AssetID assetID);
internal b32 Edit_i32(EditorLayout* layout, char* name, i32* number, b32 isInArray, AssetID assetID);
internal b32 Edit_u16(EditorLayout* layout, char* name, u16* number, b32 isInArray, AssetID assetID);
internal b32 Edit_r32(EditorLayout* layout, char* name, r32* number, b32 isInArray, AssetID assetID, b32 clamp01 = false);
internal b32 Edit_Color(EditorLayout* layout, char* name, Color* color, b32 isInArray, AssetID assetID);
internal b32 Edit_Vec2(EditorLayout* layout, char* name, Vec2* v, b32 isInArray, AssetID assetID, b32 clamp01 = false);
internal b32 Edit_Vec3(EditorLayout* layout, char* name, Vec3* v, b32 isInArray, AssetID assetID, b32 clamp01 = false);
internal b32 Edit_Vec4(EditorLayout* layout, char* name, Vec4 * v, b32 isInArray, AssetID assetID, b32 clamp01 = false);
internal b32 Edit_Hash64(EditorLayout* layout, char* name, Hash64* h, char* optionsName, b32 isInArray, AssetID assetID);
internal b32 Edit_Enumerator(EditorLayout* layout, char* name, Enumerator* enumerator, StringArray options, b32 isInArray, AssetID assetID);
internal b32 Edit_EntityRef(EditorLayout* layout, char* name, EntityRef* ref, b32 isInArray, AssetID assetID);
internal b32 Edit_GameProperty(EditorLayout* layout, char* name, GameProperty* property, b32 isInArray, AssetID assetID);
internal b32 Edit_GameAssetType(EditorLayout* layout, char* name, GameAssetType* type, b32 typeEditable, b32 isInArray, AssetID assetID);
internal b32 Edit_AssetLabel(EditorLayout* layout, char* name, AssetLabel* label, b32 isInArray, AssetID assetID);
internal void NextRaw(EditorLayout* layout);
internal void Nest(EditorLayout* layout);
internal void Push(EditorLayout* layout);
internal void Pop(EditorLayout* layout);
internal Rect2 EditorTextDraw(EditorLayout* layout, Vec4 color, u32 flags, char* format, ...);
internal b32 EditorCollapsible(EditorLayout* layout, char* string, AUID ID);
internal b32 StandardEditorButton(EditorLayout* layout, char* name, AUID ID, Vec4 color);
internal void AddUndoRedoAdd(EditorUIContext* context, ArrayCounter* counter, void* ptr, void* oldPtr, void* newPtr, AssetID assetID);
internal void AddUndoRedoDelete(EditorUIContext* context, ArrayCounter* counter, void* deletedElement, void* ptr, void* lastElementPtr, u32 elementSize, AssetID assetID);

#define DUMB_OPERATION_BOILERPLATE_(type)\
case FieldOperation_GetSize:{} break;\
case FieldOperation_Parse:{*((type*)ptr) = value;} break;

#ifdef FORG_SERVER
#define DUMB_OPERATION_BOILERPLATE(type) DUMB_OPERATION_BOILERPLATE_(type)
#else
#define DUMB_OPERATION_BOILERPLATE(type)\
DUMB_OPERATION_BOILERPLATE_(type)\
case FieldOperation_Edit:{result.deleted = Edit_##type(layout, field->name, (type*) ptr, isInArray, assetID);} break;
#endif

#define DUMB_INIT_BOILERPLATE(type)\
result.size = sizeof(type);\
type value = field->def.def_##type;\
if(source){value = Parse_##type(source, value);}


#define STANDARD_OPERATION_FUNCTION(type)\
internal StructOperationResult type##Operation(EditorLayout* layout, FieldDefinition* field, FieldOperationType operation, void* ptr, Tokenizer* source, Stream* output, b32 isInArray, AssetID assetID)\
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


STANDARD_OPERATION_FUNCTION(u16);
STANDARD_OPERATION_FUNCTION(u32);
STANDARD_OPERATION_FUNCTION(i32);
STANDARD_OPERATION_FUNCTION(r32);
STANDARD_OPERATION_FUNCTION(Color);
STANDARD_OPERATION_FUNCTION(GameProperty);
STANDARD_OPERATION_FUNCTION(Vec2);
STANDARD_OPERATION_FUNCTION(Vec3);
STANDARD_OPERATION_FUNCTION(Vec4);
STANDARD_OPERATION_FUNCTION(AssetLabel);
STANDARD_OPERATION_FUNCTION(EntityRef);

internal StructOperationResult EnumeratorOperation(EditorLayout* layout, FieldDefinition* field, FieldOperationType operation, void* ptr, Tokenizer* source, Stream* output, b32 isInArray, AssetID assetID)
{
    StructOperationResult result = {};
    DUMB_INIT_BOILERPLATE(Enumerator);
    switch(operation)
    {
        DUMB_OPERATION_BOILERPLATE_(Enumerator);
#ifndef FORG_SERVER
        case FieldOperation_Edit:
        {
            StringArray options = {};
            options.strings = field->options;
            options.count = field->optionCount;
            Edit_Enumerator(layout, field->name, (Enumerator*) ptr, options, isInArray, assetID);
        } break;
#endif
        case FieldOperation_Dump:
        {
            value = *(Enumerator*) ptr;
            Dump_Enumerator(output, field, value, isInArray);
        } break;
        
        InvalidDefaultCase;
    }
    
    return result;
}

internal StructOperationResult Hash64Operation(EditorLayout* layout, FieldDefinition* field, FieldOperationType operation, void* ptr, Tokenizer* source, Stream* output, b32 isInArray, AssetID assetID)
{
    StructOperationResult result = {};
    DUMB_INIT_BOILERPLATE(Hash64);
    switch(operation)
    {
        DUMB_OPERATION_BOILERPLATE_(Hash64);
#ifndef FORG_SERVER
        case FieldOperation_Edit:
        {
            Edit_Hash64(layout, field->name, (Hash64*) ptr, field->optionsName, isInArray, assetID);
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

internal StructOperationResult GameAssetTypeOperation(EditorLayout* layout, FieldDefinition* field, FieldOperationType operation, void* ptr, Tokenizer* source, Stream* output, b32 isInArray, AssetID assetID)
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
            Edit_GameAssetType(layout, field->name, type, typeEditable, isInArray, assetID);
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
    u16 maxCount;
};

internal void* InitMetaArray(void* memory, u32 elementCount)
{
    MetaArrayHeader* header = (MetaArrayHeader*) memory;
    header->maxCount = SafeTruncateToU16(elementCount);
    void* result = (header + 1);
    return result;
}

internal MetaArrayHeader* GetHeader(void* dataPtr)
{
    MetaArrayHeader* result = (MetaArrayHeader*) dataPtr - 1;
    return result;
}

internal void* GetMemory(MemoryPool* pool, u32 elementCount, u32 elementSize)
{
    void* result = (void*) PushSize(pool, elementCount * elementSize + sizeof(MetaArrayHeader));
    return result;
}

internal void SetMetaAssets(Assets* assets)
{
    meta_assets = assets;
}

internal StructOperationResult StructOperation(EditorLayout* layout, String structName, String name, Tokenizer* tokenizer, void* dataPtr, FieldOperationType operation, Stream* output, ReservedSpace* reserved, b32 parentWasPointer, AssetID assetID);
internal u32 FieldOperation(EditorLayout* layout, FieldDefinition* field, FieldOperationType operation, void* fieldPtr, Tokenizer* tokenizer, Stream* output, ReservedSpace* reserved, u16* elementCount, AssetID assetID)
{
    u32 result = 0;
    b32 pointer = (field->flags & MetaFlag_Pointer);
    u16 elements = 0;
    
	if(pointer && operation == FieldOperation_Dump)
	{
		OutputToStream(output, "%s=", field->name);
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
        
#define DUMB_OPERATION(type) case MetaType_##type:{op = type##Operation(layout, field, operation, fieldPtr, tokenizer, output, pointer, assetID);} break;
        switch(field->type)
        {
            DUMB_OPERATION(u16);
            DUMB_OPERATION(u32);
            DUMB_OPERATION(i32);
            DUMB_OPERATION(r32);
            DUMB_OPERATION(Vec2);
            DUMB_OPERATION(Vec3);
            DUMB_OPERATION(Vec4);
            DUMB_OPERATION(Color);
            DUMB_OPERATION(Hash64);
            DUMB_OPERATION(Enumerator);
            DUMB_OPERATION(GameProperty);
            DUMB_OPERATION(GameAssetType);
            DUMB_OPERATION(AssetLabel);
            DUMB_OPERATION(EntityRef);
            
            default:
            {
                Assert(field->type > MetaType_FirstCustomMetaType);
                
                StreamState beforeName = {};
                StreamState afterName = {};
                
                
                if(operation == FieldOperation_Dump)
                {
                    beforeName = SaveStreamState(output);
                    if(!pointer)
                    {
                        OutputToStream(output, "%s=", field->name);
                    }
                    afterName = SaveStreamState(output);
                    OutputToStream(output, "{");
                }
                
                char nameString[64];
                if(pointer)
                {
                    FormatString(nameString, sizeof(nameString), "%s[%d]", field->name, elements - 1);
                }
                else
                {
                    FormatString(nameString, sizeof(nameString), "%s", field->name);
                }
                
                String structName = Stringize(field->typeName);
                String name = Stringize(nameString);
                
                op = StructOperation(layout, structName, name, tokenizer, fieldPtr, operation, output, reserved, pointer, assetID);
                
                if(operation == FieldOperation_Dump)
                {
                    OutputToStream(output, "}");
                    
                    StreamState afterStruct = SaveStreamState(output);
					if(!pointer)
					{
						if(DeltaStreamState(afterStruct, afterName) == 2)
						{
							RestoreStreamState(output, beforeName);
						}
						else
						{
							OutputToStream(output, ";");
						}
					}
                }
            } break;
        }
        
        result += op.size;
        
#ifndef FORG_SERVER
        if(op.deleted)
        {
            u32 offset = field->size * --*elementCount;
            void* targetPtr = AdvanceVoidPtrBytes(fieldPtr, offset);
            
            
            void* destPtr = fieldPtr;
            void* sourcePtr = AdvanceVoidPtrBytes(fieldPtr, field->size);
            while(destPtr != targetPtr)
            {
                Copy(field->size, destPtr, sourcePtr);
                destPtr = sourcePtr;
                sourcePtr = AdvanceVoidPtrBytes(sourcePtr, field->size);
            }
            
            AddUndoRedoDelete(layout->context, elementCount, fieldPtr, fieldPtr, targetPtr, field->size, assetID);
        }
#endif
        
        if(pointer)
        {
            if(operation == FieldOperation_Dump || operation == FieldOperation_Edit)
            {
                if(elements >= *elementCount)
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
            result -= field->size;
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

internal void InitStructDefault(StructDefinition* definition, void* dataPtr);
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
            DUMB_COPY_DEF(Color);
            DUMB_COPY_DEF(Hash64);
            DUMB_COPY_DEF(ArrayCounter);
            DUMB_COPY_DEF(Enumerator);
            DUMB_COPY_DEF(GameProperty);
            DUMB_COPY_DEF(GameAssetType);
            DUMB_COPY_DEF(AssetLabel);
            DUMB_COPY_DEF(EntityRef);
            
            default:
            {
                Assert(field->type > MetaType_FirstCustomMetaType);
                String structName = Stringize(field->typeName);
                StructDefinition* structDefinition = GetMetaStructDefinition(structName);
                InitStructDefault(structDefinition, ptr);
            } break;
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

internal StructOperationResult StructOperation(EditorLayout* layout, String structName, String name, Tokenizer* tokenizer, void* dataPtr, FieldOperationType operation, Stream* output, ReservedSpace* reserved, b32 parentWasPointer, AssetID assetID)
{
    StructOperationResult result = {};
    StructDefinition* definition = GetMetaStructDefinition(structName);
    if(definition)
    {
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
                canProceed = EditorCollapsible(layout, 0, auID(dataPtr, structName.ptr, "structCollapse"));
                EditorTextDraw(layout, V4(1, 1, 1, 1), EditorText_StartingSpace, "%.*s {}", name.length, name.ptr);
                
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
                if(operation == FieldOperation_Edit)
                {
                    Push(layout);
                }
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
                            result.size += sizeof(MetaArrayHeader);
                            elementCount = GetMetaPtrElementCountForArray(definition, field, dataPtr);
                            if(elementCount)
                            {
                                Assert(sizeof(u64) == sizeof(void*));
                                fieldPtr = (void*) (*(u64*)fieldPtr);
                                MetaArrayHeader* header = fieldPtr ? (MetaArrayHeader*) fieldPtr - 1 : 0;
#ifndef FORG_SERVER                
                                if(operation == FieldOperation_Edit)
                                {
                                    NextRaw(layout);
                                    show = EditorCollapsible(layout, 0, auID(originalfieldPtr, "collapsible"));
                                    EditorTextDraw(layout, V4(1, 1, 1, 1), 0, "%s [%d]", field->name, *elementCount);
                                    
                                    if(StandardEditorButton(layout, "add", auID(originalfieldPtr, "addButton"), V4(0, 1.0f, 1.0f, 1.0f)))
                                    {
                                        
                                        void* ptr = 0;
                                        void* oldPtr = 0;
                                        void* newPtr = 0;
                                        
                                        if(*elementCount == 0 || (*elementCount > header->maxCount / 2))
                                        {
                                            ptr = originalfieldPtr;
                                            oldPtr = fieldPtr;
                                            
                                            if(header)
                                            {
                                                u32 oldSize = header->maxCount * field->size;
                                                void* toFree = header;
                                            }
                                            
                                            u32 newElementCount = header ? (header->maxCount * 2) : 16;
                                            void* oldArray = fieldPtr;
                                            u32 sizeToCopy = *elementCount * field->size;
                                            void* newArray = GetMemory(layout->context->pool, newElementCount, field->size);
                                            fieldPtr = InitMetaArray(newArray, newElementCount);
                                            
                                            newPtr = fieldPtr;
                                            
                                            Copy(sizeToCopy, fieldPtr, oldArray);
                                            *(u64*)originalfieldPtr = (u64) fieldPtr;
                                        }
                                        
                                        u16 index = (*elementCount)++;
                                        void* here = (void*)((u8*)fieldPtr + index * field->size);
                                        InitFieldDefault(field, here);
                                        
                                        
                                        AddUndoRedoAdd(layout->context, elementCount, ptr, oldPtr, newPtr, assetID);
                                    }
                                }
#endif
                            }
                            else
                            {
                                show = false;
                                InvalidCodePath;
                                // TODO(Leonardo): show something on the editor here, like "error: field doesn't have a counter!
                            }
                            
                        }
                        
                        if(show && *elementCount)
                        {
                            if(operation == FieldOperation_Edit && (field->flags & MetaFlag_Uneditable))
                            {
                                
                            }
                            else
                            {
#ifndef FORG_SERVER
                                if(pointer && operation == FieldOperation_Edit)
                                {
                                    Push(layout);
                                }
#endif
                                result.size += FieldOperation(layout, field, operation, fieldPtr, tokenizer, output, reserved, elementCount, assetID);
#ifndef FORG_SERVER
                                if(pointer && operation == FieldOperation_Edit)
                                {
                                    Pop(layout);
                                }
#endif
                            }
                        }
                    }
                }
                
#ifndef FORG_SERVER
                if(operation == FieldOperation_Edit)
                {
                    Pop(layout);
                }
#endif
            }
        }
        else
        {
            for(;;)
            {
                Token t = GetToken(tokenizer);
                if(t.type == Token_CloseBraces)
                {
                    break;
                }
                else if(t.type == Token_OpenBraces)
                {
                    
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
                    if(field)
                    {
                        Assert(field->type != MetaType_ArrayCounter);
                        
                        if(RequireToken(tokenizer, Token_EqualSign))
                        {
                            void* fieldPtr = (void*) ((u8*) dataPtr + field->offset);
                            u32 additionalSize = sizeof(MetaArrayHeader);
                            if(field->flags & MetaFlag_Pointer)
                            {
                                result.size += additionalSize;
                            }
                            
                            if(operation == FieldOperation_Parse && field->flags & MetaFlag_Pointer)
                            {
                                Tokenizer fake = {};
                                fake.at = tokenizer->at;
                                
                                u16 elementCount;
                                FieldOperation(layout, field, FieldOperation_GetSize, fieldPtr, &fake, output, reserved, &elementCount, assetID);
                                
                                ArrayCounter* counterPtr = GetMetaPtrElementCountForArray(definition, field, dataPtr);
                                Assert(counterPtr);
                                *counterPtr = elementCount;
                                
                                Assert(sizeof(u64) == sizeof(void*));
                                if(elementCount)
                                {
                                    void* headerPtr = ReserveSpace(reserved, elementCount * field->size + additionalSize);
                                    void* arrayPtr = InitMetaArray(headerPtr, elementCount);
                                    
                                    *(u64*) fieldPtr = (u64) arrayPtr;
                                    fieldPtr = arrayPtr;
                                }
                                else
                                {
                                    *(u64*) fieldPtr = 0;
                                }
                            }
                            
                            u16 ignored = 1;
                            result.size += FieldOperation(layout, field, operation, fieldPtr, tokenizer, output, reserved, &ignored, assetID);
                        }
                        else
                        {
                            InvalidCodePath;
                        }
                    }
                    else
                    {
                        if(RequireToken(tokenizer, Token_EqualSign))
                        {
                            u32 currentDepth = 0;
                            
                            while(true)
                            {
                                Token skip = GetToken(tokenizer);
                                
                                if(skip.type == Token_OpenBraces)
                                {
                                    ++currentDepth;
                                }
                                else if(skip.type == Token_CloseBraces)
                                {
                                    Assert(currentDepth > 0);
                                    --currentDepth;
                                }
                                if(skip.type == Token_SemiColon)
                                {
                                    if(currentDepth == 0)
                                    {
                                        break;
                                    }
                                }
                            }
                        }
                        else
                        {
                            InvalidCodePath;
                        }
                    }
                }
            }
        }
        
    }
    return result;
}

internal u32 GetStructSize(String structName, Tokenizer* tokenizer)
{
    ReservedSpace ignored = {};
    u32 result = StructOperation(0, structName, structName, tokenizer, 0, FieldOperation_GetSize, 0, &ignored, false, {}).size;
    return result;
}

internal void ParseBufferIntoStruct(Assets* assets, String structName, Tokenizer* tokenizer, void* structPtr, u32 reservedSize)
{
    SetMetaAssets(assets);
    
    ReservedSpace reserved = {};
    StructDefinition* definition = GetMetaStructDefinition(structName);
    reserved.ptr = (void*) ((u8*) structPtr + definition->size);
    reserved.size = reservedSize - definition->size;
    
    StructOperation(0, structName, structName, tokenizer, structPtr, FieldOperation_Parse, 0, &reserved, false, {});
    
    Assert(reserved.size == 0);
}

internal void DumpStructToStream(Assets* assets, String structName, Stream* dest, void* structPtr)
{
    SetMetaAssets(assets);
    
    ReservedSpace ignored = {};
    OutputToStream(dest, "{");
    StructOperation(0, structName, structName, 0, structPtr, FieldOperation_Dump, dest, &ignored, false, {});
    OutputToStream(dest, "}");
}
