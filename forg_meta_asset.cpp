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

struct StringArray
{
    char** strings;
    u32 count;
};

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
        if(StrEqual(name.length, name.ptr, definition->name))
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

struct EditorLayout;
internal void EditU32(EditorLayout* layout, char* name, u32* number);
internal void EditU16(EditorLayout* layout, char* name, u16* number);
internal void EditR32(EditorLayout* layout, char* name, r32* number);
internal void EditVec2(EditorLayout* layout, char* name, Vec2* v);
internal void EditVec3(EditorLayout* layout, char* name, Vec3* v);
internal void EditVec4(EditorLayout* layout, char* name, Vec4 * v);
internal void NextRaw(EditorLayout* layout);
internal void Push(EditorLayout* layout);
internal void Pop(EditorLayout* layout);
internal Rect2 EditorTextDraw(EditorLayout* layout, Vec4 color, u32 flags, char* format, ...);
internal b32 EditorCollapsible(EditorLayout* layout, char* string, AUID ID);
internal b32 StandardEditorButton(EditorLayout* layout, char* name, AUID ID, Vec4 color);

internal u32 U32Operation(EditorLayout* layout, MemberDefinition* field, FieldOperationType operation, void* ptr, Tokenizer* source, Stream* output, b32 isInArray)
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
            value = *(u32*) ptr;
            
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
        } break;
        
#ifndef FORG_SERVER        
        case FieldOperation_Edit:
        {
            EditU32(layout, field->name, (u32*) ptr);
        } break;
#endif
        
        InvalidDefaultCase;
        
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

internal u32 R32Operation(EditorLayout* layout, MemberDefinition* field, FieldOperationType operation, void* ptr, Tokenizer* source, Stream* output, b32 isInArray)
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
            value = *(r32*) ptr;
            
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
        } break;
        
#ifndef FORG_SERVER
        case FieldOperation_Edit:
        {
            EditR32(layout, field->name, (r32*) ptr);
        } break;
#endif
        InvalidDefaultCase;
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

internal u32 Vec2Operation(EditorLayout* layout, MemberDefinition* field, FieldOperationType operation, void* ptr, Tokenizer* tokenizer, Stream* output, b32 isInArray)
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
            value = *(Vec2*) ptr;
            
            
            if(isInArray)
            {
                OutputToStream(output, "{");
                if(value != field->def.def_Vec2)
                {
                    if(value.x != field->def.def_Vec2.x)
                    {
                        OutputToStream(output, "x=%f;", value.x);
                    }
                    
                    if(value.y != field->def.def_Vec2.y)
                    {
                        OutputToStream(output, "y=%f;", value.y);
                    }
                }
                OutputToStream(output, "}");
            }
            else
            {
                if(value != field->def.def_Vec2)
                {
                    OutputToStream(output, "%s={", field->name);
                    if(value.x != field->def.def_Vec2.x)
                    {
                        OutputToStream(output, "x=%f;", value.x);
                    }
                    
                    if(value.y != field->def.def_Vec2.y)
                    {
                        OutputToStream(output, "y=%f;", value.y);
                    }
                    OutputToStream(output, "};");
                }
            }
        } break;
        
#ifndef FORG_SERVER
        case FieldOperation_Edit:
        {
            EditVec2(layout, field->name, (Vec2*) ptr);
        } break;
#endif
        
        InvalidDefaultCase;
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


internal u32 Vec3Operation(EditorLayout* layout, MemberDefinition* field, FieldOperationType operation, void* ptr, Tokenizer* tokenizer, Stream* output, b32 isInArray)
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
            value = *(Vec3*) ptr;
            
            if(isInArray)
            {
                OutputToStream(output, "{");
                if(value != field->def.def_Vec3)
                {
                    if(value.x != field->def.def_Vec3.x)
                    {
                        OutputToStream(output, "x=%f;", value.x);
                    }
                    
                    if(value.y != field->def.def_Vec3.y)
                    {
                        OutputToStream(output, "y=%f;", value.y);
                    }
                    
                    if(value.z != field->def.def_Vec3.z)
                    {
                        OutputToStream(output, "z=%f;", value.z);
                    }
                }
                OutputToStream(output, "}");
            }
            else
            {
                if(value != field->def.def_Vec3)
                {
                    OutputToStream(output, "%s={", field->name);
                    
                    if(value.x != field->def.def_Vec3.x)
                    {
                        OutputToStream(output, "x=%f;", value.x);
                    }
                    
                    if(value.y != field->def.def_Vec3.y)
                    {
                        OutputToStream(output, "y=%f;", value.y);
                    }
                    
                    if(value.z != field->def.def_Vec3.z)
                    {
                        OutputToStream(output, "z=%f;", value.z);
                    }
                    OutputToStream(output, "};");
                }
            }
        } break;
        
#ifndef FORG_SERVER
        case FieldOperation_Edit:
        {
            EditVec3(layout, field->name, (Vec3*) ptr);
        } break;
#endif
        
        InvalidDefaultCase;
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

internal u32 Vec4Operation(EditorLayout* layout, MemberDefinition* field, FieldOperationType operation, void* ptr, Tokenizer* source, Stream* output, b32 isInArray)
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
            value = *(Vec4*) ptr;
            
            
            if(isInArray)
            {
                OutputToStream(output, "{");
                if(value != field->def.def_Vec4)
                {
                    if(value.x != field->def.def_Vec4.x)
                    {
                        OutputToStream(output, "x=%f;", value.x);
                    }
                    
                    if(value.y != field->def.def_Vec4.y)
                    {
                        OutputToStream(output, "y=%f;", value.y);
                    }
                    
                    if(value.z != field->def.def_Vec4.z)
                    {
                        OutputToStream(output, "z=%f;", value.z);
                    }
                    if(value.w != field->def.def_Vec4.w)
                    {
                        OutputToStream(output, "w=%f;", value.w);
                    }
                }
                OutputToStream(output, "}");
            }
            else
            {
                if(value != field->def.def_Vec4)
                {
                    OutputToStream(output, "%s={", field->name);
                    
                    if(value.x != field->def.def_Vec4.x)
                    {
                        OutputToStream(output, "x=%f;", value.x);
                    }
                    
                    if(value.y != field->def.def_Vec4.y)
                    {
                        OutputToStream(output, "y=%f;", value.y);
                    }
                    
                    if(value.z != field->def.def_Vec4.z)
                    {
                        OutputToStream(output, "z=%f;", value.z);
                    }
                    
                    if(value.w != field->def.def_Vec4.w)
                    {
                        OutputToStream(output, "w=%f;", value.w);
                    }
                    OutputToStream(output, "};");
                }
            }
        } break;
        
#ifndef FORG_SERVER
        case FieldOperation_Edit:
        {
            EditVec4(layout, field->name, (Vec4*) ptr);
        } break;
#endif
        
        InvalidDefaultCase;
    }
    
    return size;
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

struct StructOperationResult
{
    u32 size;
    b32 deleted;
};


internal StructOperationResult StructOperation(EditorLayout* layout, String structName, Tokenizer* tokenizer, void* dataPtr, FieldOperationType operation, Stream* output, ReservedSpace* reserved, b32 parentWasPointer);
internal u32 FieldOperation(EditorLayout* layout, MemberDefinition* field, FieldOperationType operation, void* fieldPtr, Tokenizer* tokenizer, Stream* output, ReservedSpace* reserved, u32* elementCount)
{
    u32 result = 0;
    b32 pointer = (field->flags & MetaFlag_Pointer);
    u32 elements = 0;
    
    if(pointer && operation == FieldOperation_Dump)
    {
        Assert(*elementCount);
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
        u32 fieldSize = 0;
        switch(field->type)
        {
            case MetaType_u32:
            {
                fieldSize = U32Operation(layout, field, operation, fieldPtr, tokenizer, output, pointer);
            } break;
            
            case MetaType_r32:
            {
                fieldSize = R32Operation(layout, field, operation, fieldPtr, tokenizer, output, pointer);
            } break;
            
            case MetaType_Vec2:
            {
                fieldSize = Vec2Operation(layout, field, operation, fieldPtr, tokenizer, output, pointer);
            } break;
            
            case MetaType_Vec3:
            {
                fieldSize = Vec3Operation(layout, field, operation, fieldPtr, tokenizer, output, pointer);
            } break;
            
            case MetaType_Vec4:
            {
                fieldSize = Vec4Operation(layout, field, operation, fieldPtr, tokenizer, output, pointer);
            } break;
            
            default:
            {
                if(pointer && operation == FieldOperation_Dump)
                {
                    OutputToStream(output, "{");
                }
                
                String structName = {};
                structName.ptr = field->typeName;
                structName.length = StrLen(field->typeName);
                StructOperationResult structOp = StructOperation(layout, structName, tokenizer, fieldPtr, operation, output, reserved, pointer);
                
                
                if(structOp.deleted)
                {
                    Assert(*elementCount > 0);
                    u32 grabThis = *elementCount - 1;
                    u32 offset = field->size * grabThis - (elements - 1);
                    void* sourcePtr = (void*) ((u8*) fieldPtr + offset);
                    
                    Copy(field->size, fieldPtr, sourcePtr);
                    
                    *elementCount = *elementCount - 1;
                }
                
                fieldSize += structOp.size;
                
                if(pointer && operation == FieldOperation_Dump)
                {
                    OutputToStream(output, "}");
                }
            } break;
        }
        
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
                result += fieldSize;
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

internal u32* GetMetaPtrElementCountForArray(StructDefinition* definition, MemberDefinition* arrayField, void* structPtr)
{
    Token counter = {};
    char counterName[128];
    counter.text = counterName;
    counter.textLength =(u32) FormatString(counterName, sizeof(counterName),"%s_%s", EDITOR_COUNTER_STRING, arrayField->name);
    
    MemberDefinition* counterDefinition = FindMetaField(definition, counter);
    Assert(counterDefinition->type == MetaType_u32);
    void* counterPtr = (void*) ((u8*) structPtr + counterDefinition->offset);
    
    u32* result = (u32*) counterPtr;
    return result;
}

internal StructOperationResult StructOperation(EditorLayout* layout, String structName, Tokenizer* tokenizer, void* dataPtr, FieldOperationType operation, Stream* output, ReservedSpace* reserved, b32 parentWasPointer = false)
{
    StructOperationResult result = {};
    StructDefinition* definition = GetMetaStructDefinition(structName);
    result.size += definition->size;
    
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
            for(u32 fieldIndex = 0; fieldIndex < definition->memberCount; ++fieldIndex)
            {
                MemberDefinition* member = definition->members + fieldIndex;
                // NOTE(Leonardo): we can't dump nor edit counters!
                if(!StrEqual(StrLen(EDITOR_COUNTER_STRING), EDITOR_COUNTER_STRING, member->name))
                {
                    void* fieldPtr = (void*) ((u8*) dataPtr + member->offset);
                    
                    u32 fakeElementCount = 1;
                    u32* elementCount = &fakeElementCount;
                    b32 show = true;
                    
                    if(member->flags & MetaFlag_Pointer)
                    {
                        elementCount = GetMetaPtrElementCountForArray(definition, member, dataPtr);
                        Assert(sizeof(u64) == sizeof(void*));
                        fieldPtr = (void*) (*(u64*)fieldPtr);
                        
#ifndef FORG_SERVER                
                        if(operation == FieldOperation_Edit)
                        {
                            NextRaw(layout);
                            show = EditorCollapsible(layout, 0, auID(fieldPtr, "collapsible"));
                            EditorTextDraw(layout, V4(1, 1, 1, 1), 0, "%s [%d]", member->name, *elementCount);
                        }
#endif
                    }
                    
                    if(*elementCount && show)
                    {
#ifndef FORG_SERVER
                        if((member->flags & MetaFlag_Pointer) && operation == FieldOperation_Edit)
                        {
                            Push(layout);
                        }
#endif
                        result.size += FieldOperation(layout, member, operation, fieldPtr, tokenizer, output, reserved, elementCount);
#ifndef FORG_SERVER
                        if((member->flags & MetaFlag_Pointer) && operation == FieldOperation_Edit)
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
                MemberDefinition* field = FindMetaField(definition, fieldName);
                if(StrEqual(StrLen(EDITOR_COUNTER_STRING), EDITOR_COUNTER_STRING, field->name))
                {
                    InvalidCodePath;
                }
                
                if(RequireToken(tokenizer, Token_EqualSign))
                {
                    void* fieldPtr = (void*) ((u8*) dataPtr + field->offset);
                    
                    if(operation == FieldOperation_Parse && field->flags & MetaFlag_Pointer)
                    {
                        Tokenizer fake = {};
                        fake.at = tokenizer->at;
                        
                        u32 elementCount;
                        FieldOperation(layout, field, FieldOperation_GetSize, fieldPtr, &fake, output, reserved, &elementCount);
                        u32* counterPtr = GetMetaPtrElementCountForArray(definition, field, dataPtr);
                        *counterPtr = elementCount;
                        void* arrayPtr = ReserveSpace(reserved, elementCount * field->size);
                        
                        Assert(sizeof(u64) == sizeof(void*));
                        *(u64*) fieldPtr = (u64) arrayPtr;
                        
                        fieldPtr = arrayPtr;
                    }
                    u32 ignored;
                    result.size += FieldOperation(layout, field, operation, fieldPtr, tokenizer, output, reserved, &ignored);
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
