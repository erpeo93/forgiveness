global_variable u32 meta_definitionCount;
global_variable StructDefinition meta_definitions[1024];
MemberDefinition* FindMetaField(MemberDefinition* members, u32 memberCount, Token fieldName)
{
    MemberDefinition* result = 0;
    for(u32 fieldIndex = 0; fieldIndex < memberCount; ++fieldIndex)
    {
        MemberDefinition* test = members + fieldIndex;
        if(TokenEquals(fieldName, test->name))
        {
            result = test;
            break;
        }
    }
    
    return result;
    
}
MemberDefinition* FindMetaField(StructDefinition* definition, Token fieldName)
{
    MemberDefinition* result = FindMetaField(definition->members, definition->memberCount, fieldName);
    Assert(result);
    return result;
}

#define AddToMetaDefinitions(name, definition) AddToMetaDefinitions_(#name, sizeof(name), ArrayCount(definition), definition)
internal void AddToMetaDefinitions_(char* name, u32 size, u32 memberCount, MemberDefinition* members)
{
    Assert(meta_definitionCount < ArrayCount(meta_definitions));
    StructDefinition* definition = meta_definitions + meta_definitionCount++;
    
    FormatString(definition->name, sizeof(definition->name), "%s", name);
    definition->memberCount = memberCount;
    definition->size = size;
    definition->members = members;
}


global_variable u32 meta_labelTypeCount = 0;
global_variable MetaLabelList meta_labels[1024];
internal u32 GetMetaLabelType(Token labelName)
{
    u32 result = 0;
    for(u32 labelType = 0; labelType < meta_labelTypeCount; ++labelType)
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
    Assert(value < meta_labelTypeCount);
    MetaLabelList* list = meta_labels + value;
    char* result = list->name;
    
    return result;
}

internal u16 ExistMetaLabelValue(u32 labelType, Token value)
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
    Assert(labelType < meta_labelTypeCount);
    MetaLabelList* list = meta_labels + labelType;
    
    Assert(labelValue < list->labelCount);
    char* result = list->labels[labelValue];
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

internal void LoadMetaData()
{
    
    META_DEFAULT_VALUES_CPP_SUCKS();
    META_HANDLE_ADD_TO_DEFINITION_HASH();
    AddToMetaDefinitions(Vec2, memberDefinitionOfVec2);
    AddToMetaDefinitions(Vec3, memberDefinitionOfVec3);
    AddToMetaDefinitions(Vec4, memberDefinitionOfVec4);
    
    
    AddToMetaLabels_("INVALIDINVALIDINVALID", 0, 0);
    META_LABELS_ADD();
}


internal StructDefinition* GetMetaStructDefinition(String name)
{
    StructDefinition* result = 0;
    for(u32 definitionIndex = 0; definitionIndex < meta_definitionCount; ++definitionIndex)
    {
        StructDefinition* definition = meta_definitions + definitionIndex;
        if(StrEqual(name.size, (char*) name.b, definition->name))
        {
            result = definition;
            break;
        }
    }
    
    Assert(result);
    return result;
}

enum FieldOperationType
{
    FieldOperation_GetSize,
    FieldOperation_Dump,
    FieldOperation_Edit,
    FieldOperation_Parse,
};

internal u32 Parseu32(Tokenizer* tokenizer, u32 defaultVal)
{
    u32 result = defaultVal;
    
    Token t = GetToken(tokenizer);
    Assert(t.type == Token_Number);
    result = (u32) StringToInt(t.text);
    
    return result;
}

internal unm OutputToBuffer(Buffer* output, char* format, ...)
{
    va_list argList;
    va_start(argList, format);
    unm result = FormatStringList((char*) output->b, output->size, format, argList);
    va_end( argList );
    
    output->b += result;
    output->size -= SafeTruncateUInt64ToU32(result);
    
    return result;
}

internal u32 U32Operation(MemberDefinition* field, FieldOperationType operation, void* ptr, Tokenizer* source, Buffer* output)
{
    u32 size = sizeof(u32);
    u32 value = field->def.def_u32;
    if(source)
    {
        value = Parseu32(source, value);
    }
    
    switch(operation)
    {
        case FieldOperation_GetSize:
        {
        } break;
        
        case FieldOperation_Parse:
        {
            *((u32*)ptr) = value;
        } break;
        
        case FieldOperation_Dump:
        {
            u32 structValue = *(u32*) ptr;
            if(value != field->def.def_u32)
            {
                OutputToBuffer(output, "%s=%d,", field->name, structValue);
            }
        } break;
    }
    
    return size;
}


internal r32 Parser32(Tokenizer* tokenizer, r32 defaultVal)
{
    r32 result = defaultVal;
    
    Token t = GetToken(tokenizer);
    Assert(t.type == Token_Number);
    result = (r32) StringToFloat(t.text);
    
    return result;
}

internal u32 R32Operation(MemberDefinition* field, FieldOperationType operation, void* ptr, Tokenizer* source, Buffer* output)
{
    u32 size = sizeof(r32);
    r32 value = field->def.def_r32;
    if(source)
    {
        value = Parser32(source, value);
    }
    
    switch(operation)
    {
        case FieldOperation_GetSize:
        {
        } break;
        
        case FieldOperation_Parse:
        {
            *((r32*)ptr) = value;
        } break;
        
        case FieldOperation_Dump:
        {
            r32 structValue = *(r32*) ptr;
            if(value != field->def.def_r32)
            {
                OutputToBuffer(output, "%s=%f,", field->name, structValue);
            }
        } break;
    }
    
    return size;
}


internal Vec2 ParseVec2(Tokenizer* tokenizer, Vec2 defaultVal)
{
    Vec2 result = defaultVal;
    if(RequireToken(tokenizer, Token_OpenBraces))
    {
        while(true)
        {
            Token t = GetToken(tokenizer);
            if(t.type == Token_Identifier)
            {
                if(RequireToken(tokenizer, Token_EqualSign))
                {
                    r32 value = Parser32(tokenizer, 0);
                    MemberDefinition* field = FindMetaField(memberDefinitionOfVec2, ArrayCount(memberDefinitionOfVec2), t);
                    if(field)
                    {
                        r32* dest = (r32*) ((u8*) &result + field->offset);
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
    else
    {
        InvalidCodePath;
    }
    
    return result;
}

internal u32 Vec2Operation(MemberDefinition* field, FieldOperationType operation, void* ptr, Tokenizer* tokenizer, Buffer* output)
{
    u32 size = sizeof(Vec2);
    Vec2 value = field->def.def_Vec2;
    if(tokenizer)
    {
        value = ParseVec2(tokenizer, value);
    }
    switch(operation)
    {
        case FieldOperation_GetSize:
        {
        } break;
        
        case FieldOperation_Parse:
        {
            *((Vec2*)ptr) = value;
        } break;
        
        case FieldOperation_Dump:
        {
            Vec2 structValue = *(Vec2*) ptr;
            if(value != field->def.def_Vec2)
            {
                OutputToBuffer(output, "%s={", field->name);
                
                if(structValue.x != field->def.def_Vec2.x)
                {
                    OutputToBuffer(output, "x=%f,", structValue.x);
                }
                
                if(structValue.y != field->def.def_Vec2.y)
                {
                    OutputToBuffer(output, "y=%f,", structValue.y);
                }
                OutputToBuffer(output, "},");
            }
        } break;
    }
    
    return size;
}



internal Vec3 ParseVec3(Tokenizer* tokenizer, Vec3 defaultVal)
{
    Vec3 result = defaultVal;
    if(RequireToken(tokenizer, Token_OpenBraces))
    {
        while(true)
        {
            Token t = GetToken(tokenizer);
            if(t.type == Token_Identifier)
            {
                if(RequireToken(tokenizer, Token_EqualSign))
                {
                    r32 value = Parser32(tokenizer, 0);
                    MemberDefinition* field = FindMetaField(memberDefinitionOfVec3, ArrayCount(memberDefinitionOfVec3), t);
                    if(field)
                    {
                        r32* dest = (r32*) ((u8*) &result + field->offset);
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
    else
    {
        InvalidCodePath;
    }
    
    return result;
}


internal u32 Vec3Operation(MemberDefinition* field, FieldOperationType operation, void* ptr, Tokenizer* tokenizer, Buffer* output)
{
    u32 size = sizeof(Vec3);
    Vec3 value = field->def.def_Vec3;
    if(tokenizer)
    {
        value = ParseVec3(tokenizer, value);
    }
    
    switch(operation)
    {
        case FieldOperation_GetSize:
        {
        } break;
        
        case FieldOperation_Parse:
        {
            *((Vec3*)ptr) = value;
        } break;
        
        case FieldOperation_Dump:
        {
            Vec3 structValue = *(Vec3*) ptr;
            if(value != field->def.def_Vec3)
            {
                OutputToBuffer(output, "%s={", field->name);
                
                if(structValue.x != field->def.def_Vec3.x)
                {
                    OutputToBuffer(output, "x=%f,", structValue.x);
                }
                
                if(structValue.y != field->def.def_Vec3.y)
                {
                    OutputToBuffer(output, "y=%f,", structValue.y);
                }
                
                if(structValue.z != field->def.def_Vec3.z)
                {
                    OutputToBuffer(output, "y=%f,", structValue.z);
                }
                OutputToBuffer(output, "},");
            }
        } break;
    }
    
    return size;
}


internal Vec4 ParseVec4(Tokenizer* tokenizer, Vec4 defaultVal)
{
    Vec4 result = defaultVal;
    if(RequireToken(tokenizer, Token_OpenBraces))
    {
        while(true)
        {
            Token t = GetToken(tokenizer);
            if(t.type == Token_Identifier)
            {
                if(RequireToken(tokenizer, Token_EqualSign))
                {
                    r32 value = Parser32(tokenizer, 0);
                    MemberDefinition* field = FindMetaField(memberDefinitionOfVec4, ArrayCount(memberDefinitionOfVec4), t);
                    if(field)
                    {
                        r32* dest = (r32*) ((u8*) &result + field->offset);
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
    else
    {
        InvalidCodePath;
    }
    
    return result;
}

internal u32 Vec4Operation(MemberDefinition* field, FieldOperationType operation, void* ptr, Tokenizer* source, Buffer* output)
{
    u32 size = sizeof(Vec4);
    Vec4 value = field->def.def_Vec4;
    if(source)
    {
        value = ParseVec4(source, value);
    }
    
    switch(operation)
    {
        case FieldOperation_GetSize:
        {
        } break;
        
        case FieldOperation_Parse:
        {
            *((Vec4*)ptr) = value;
        } break;
        
        case FieldOperation_Dump:
        {
            Vec4 structValue = *(Vec4*) ptr;
            if(structValue != field->def.def_Vec4)
            {
                OutputToBuffer(output, "%s={", field->name);
                
                if(structValue.x != field->def.def_Vec4.x)
                {
                    OutputToBuffer(output, "x=%f,", structValue.x);
                }
                
                if(structValue.y != field->def.def_Vec4.y)
                {
                    OutputToBuffer(output, "y=%f,", structValue.y);
                }
                
                if(structValue.z != field->def.def_Vec4.z)
                {
                    OutputToBuffer(output, "y=%f,", structValue.z);
                }
                
                if(structValue.w != field->def.def_Vec4.w)
                {
                    OutputToBuffer(output, "y=%f,", structValue.w);
                }
                OutputToBuffer(output, "},");
            }
        } break;
    }
    
    return size;
}



internal u32 StructOperation(String structName, Tokenizer* tokenizer, void* dataPtr, FieldOperationType operation, Buffer* output, b32 printName);
internal u32 FieldOperation(MemberDefinition* field, FieldOperationType operation, void* fieldPtr, Tokenizer* tokenizer, Buffer* output)
{
    u32 result = 0;
    b32 pointer = (field->flags & pointer);
    
    while(true)
    {
        u32 fieldSize = 0;
        switch(field->type)
        {
            case MetaType_u32:
            {
                fieldSize = U32Operation(field, operation, fieldPtr, tokenizer, output);
            } break;
            
            case MetaType_r32:
            {
                fieldSize = R32Operation(field, operation, fieldPtr, tokenizer, output);
            } break;
            
            case MetaType_Vec2:
            {
                fieldSize = Vec2Operation(field, operation, fieldPtr, tokenizer, output);
            } break;
            
            case MetaType_Vec3:
            {
                fieldSize = Vec3Operation(field, operation, fieldPtr, tokenizer, output);
            } break;
            
            case MetaType_Vec4:
            {
                fieldSize = Vec4Operation(field, operation, fieldPtr, tokenizer, output);
            } break;
            
            default:
            {
                String structName = {};
                structName.b = (u8*) field->typeName;
                structName.size = StrLen(field->typeName);
                StructOperation(structName, tokenizer, fieldPtr, operation, output, true);
            } break;
        }
        
        if(pointer)
        {
            result += fieldSize;
            Token t = GetToken(tokenizer);
            if(t.type == Token_Comma)
            {
                // NOTE(Leonardo): continue operating on the array!
            }
            else
            {
                Assert(t.type == Token_SemiColon);
                break;
            }
        }
        else
        {
            break;
        }
    }
    
    return result;
}

internal void CopyDefaultValue(MemberDefinition* member, void* ptr)
{
    switch(member->type)
    {
        case MetaType_u8:
        {
            *(u8*) ptr = member->def.def_u8;
        } break;
        
        case MetaType_i8:
        {
            *(i8*) ptr = member->def.def_i8;
        } break;
        
        case MetaType_u16:
        {
            *(u16*) ptr = member->def.def_u16;
        } break;
        case MetaType_i16:
        {
            *(i16*) ptr = member->def.def_i16;
        } break;
        case MetaType_u32:
        {
            *(u32*) ptr = member->def.def_u32;
        } break;
        
        case MetaType_i32:
        {
            *(i32*) ptr = member->def.def_i32;
        } break;
        
        case MetaType_u64:
        {
            *(u64*) ptr = member->def.def_u64;
        } break;
        case MetaType_i64:
        {
            *(i64*) ptr = member->def.def_i64;
        } break;
        
        case MetaType_Vec2:
        {
            *(Vec2*) ptr = member->def.def_Vec2;
        } break;
        case MetaType_Vec3:
        {
            *(Vec3*) ptr = member->def.def_Vec3;
        } break;
        case MetaType_Vec4:
        {
            *(Vec4*) ptr = member->def.def_Vec4;
        } break;
        case MetaType_r32:
        {
            *(r32*) ptr = member->def.def_r32;
        } break;
        case MetaType_b32:
        {
            *(b32*) ptr = member->def.def_b32;
        } break;
    }
}

internal u32 StructOperation(String structName, Tokenizer* tokenizer, void* dataPtr, FieldOperationType operation, Buffer* output, b32 printName = true)
{
    u32 result = 0;
    StructDefinition* definition = GetMetaStructDefinition(structName);
    result += definition->size;
    
    if(operation == FieldOperation_Parse)
    {
        for(u32 fieldIndex = 0; fieldIndex < definition->memberCount; ++fieldIndex)
        {
            MemberDefinition* member = definition->members + fieldIndex;
            
            if(!member->flags & MetaFlag_Pointer)
            {
                void* fieldPtr = (void*) ((u8*) dataPtr + member->offset);
                CopyDefaultValue(member, fieldPtr);
            }
        }
    }
    
    if(operation == FieldOperation_Dump || operation == FieldOperation_Edit)
    {
        if(operation == FieldOperation_Dump)
        {
            OutputToBuffer(output, "%.*s%s{", 
                           printName ? structName.size : 0, 
                           structName.b,
                           printName ? "=" : "");
        }
        
        for(u32 fieldIndex = 0; fieldIndex < definition->memberCount; ++fieldIndex)
        {
            MemberDefinition* member = definition->members + fieldIndex;
            void* fieldPtr = (void*) ((u8*) dataPtr + member->offset);
            result += FieldOperation(member, operation, fieldPtr, tokenizer, output);
            
        }
        
        if(operation == FieldOperation_Dump)
        {
            OutputToBuffer(output, "},");
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
            else if(t.type == Token_Comma)
            {
            }
            else if(t.type == Token_EndOfFile)
            {
                break;
            }
            else
            {
                Token fieldName = Stringize(t);
                MemberDefinition* field = FindMetaField(definition, fieldName);
                if(RequireToken(tokenizer, Token_EqualSign))
                {
                    void* fieldPtr = (void*) ((u8*) dataPtr + field->offset);
                    result += FieldOperation(field, operation, fieldPtr, tokenizer, output);
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
    u32 result = StructOperation(structName, tokenizer, 0, FieldOperation_GetSize, 0, false);
    return result;
}

internal void ParseBufferIntoStruct(String structName, Tokenizer* tokenizer, void* structPtr)
{
    StructOperation(structName, tokenizer, structPtr, FieldOperation_Parse, 0, false);
}

internal void DumpStructToBuffer(String structName, Buffer* dest, void* structPtr)
{
    StructOperation(structName, 0, structPtr, FieldOperation_Dump, dest, false);
}

#if 0
internal void EditAssetFile()
{
    switch(file->type)
    {
        EditAssetLabels();
        case Image:
        {
            if()
            {
                PushBitmap();
            }
        } break;
        
        case Sound:
        {
            if()
            {
                PlaySound();
            }
        } break;
        
        case Model:
        {
            if()
            {
                PushModel();
            }
        }
        
        Default:
        {
            EditDataStructure();
            
            if(SaveButton())
            {
                WriteBackAssetFileToFileSystem();
            }
        } break;
    }
}
#endif