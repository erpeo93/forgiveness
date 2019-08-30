

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