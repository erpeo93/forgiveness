#include "forg_base.h"
#include "forg_shared.h"
#include "forg_token.h"
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <math.h>

#include "forg_token.cpp"
char* ReadEntireFileAndNullTerminate( char* filename )
{
    char* result = 0;
    FILE* file = fopen( filename, "r" );
    if( file )
    {
        fseek( file, 0, SEEK_END );
        size_t filesize = ftell( file );
        fseek( file, 0, SEEK_SET );
        
        result = ( char* ) malloc( filesize + 1 );
        memset( result, 0, filesize + 1 );
        fread( result, filesize, 1, file );
        
        fclose( file );
    }
    
    return result;
}

void ParseIntrospectParam( Tokenizer* tokenizer )
{
    for(;;)
    {
        Token token = GetToken( tokenizer );
        if( token.type == Token_CloseParen || token.type == Token_EndOfFile )
        {
            break;
        }
    }
}

char fieldDefaultValues[KiloBytes(16)] = {};
char* fieldDefaultValuePtr = fieldDefaultValues;

void ParseField(Tokenizer* tokenizer, Token structToken, Token fieldNameToken, u32 fieldIndex)
{
    bool parsing = true;
    bool isPointer = false;
    bool uneditable = false;
    bool nameParsed = false;
    Token defaultValue = {};
    bool hasDefault = false;
    
    Token optionsToken = {};
    optionsToken.text = "invalid";
    optionsToken.textLength = StrLen(optionsToken.text);
    Token counterToken = optionsToken;
    Token fixedToken = optionsToken;
    Token enumToken = optionsToken;
    b32 fixedFieldCount = 0;
    
    Token nameToken = {};
    
    while( parsing )
    {
        Token token = GetToken(tokenizer);
        switch(token.type)
        {
            case Token_Asterisk:
            {
                isPointer = true;
            } break;
            
            case Token_Identifier:
            {
                if(TokenEquals(token, "MetaDefault"))
                {
                    if(RequireToken(tokenizer, Token_OpenParen))
                    {
                        hasDefault = true;
                        defaultValue = Stringize(GetToken(tokenizer));
                    }
                    
                    if(!RequireToken(tokenizer, Token_CloseParen))
                    {
                        InvalidCodePath;
                    }
                }
                else if(TokenEquals(token, "MetaAutocomplete"))
                {
                    if(RequireToken(tokenizer, Token_OpenParen))
                    {
                        optionsToken = Stringize(GetToken(tokenizer));
                    }
                    
                    if(!RequireToken(tokenizer, Token_CloseParen))
                    {
                        InvalidCodePath;
                    }
                }
                else if(TokenEquals(token, "MetaCounter"))
                {
                    if(RequireToken(tokenizer, Token_OpenParen))
                    {
                        counterToken = Stringize(GetToken(tokenizer));
                    }
                    
                    if(!RequireToken(tokenizer, Token_CloseParen))
                    {
                        InvalidCodePath;
                    }
                }
                else if(TokenEquals(token, "MetaEnumerator"))
                {
                    if(RequireToken(tokenizer, Token_OpenParen))
                    {
                        enumToken = Stringize(GetToken(tokenizer));
                    }
                    
                    if(!RequireToken(tokenizer, Token_CloseParen))
                    {
                        InvalidCodePath;
                    }
                }
                else if(TokenEquals(token, "MetaUneditable"))
                {
                    uneditable = true;
                    if(RequireToken(tokenizer, Token_OpenParen))
                    {
                        if(!RequireToken(tokenizer, Token_CloseParen))
                        {
                            InvalidCodePath;
                        }
                    }
                }
                else if(TokenEquals(token, "MetaFixed"))
                {
                    Assert(fixedFieldCount == 0);
                    fixedFieldCount++;
                    
                    if(RequireToken(tokenizer, Token_OpenParen))
                    {
                        fixedToken = Stringize(GetToken(tokenizer));
                    }
                    
                    if(!RequireToken(tokenizer, Token_CloseParen))
                    {
                        InvalidCodePath;
                    }
                }
                else
                {
                    if(!nameParsed)
                    {
                        nameToken = token;
                    }
                }
            } break;
            
            case Token_SemiColon:
            {
                char flags[128] = {};
                bool something = false;
                
                if(isPointer)
                {
                    something = true;
                    AppendString(flags, sizeof(flags), "MetaFlag_Pointer");
                }
                
                if(uneditable)
                {
                    something = true;
                    AppendString(flags, sizeof(flags), "MetaFlag_Uneditable");
                }
                
                if(!something)
                {
                    AppendString(flags, sizeof(flags), "0");
                }
                
                
                parsing = false;
                printf("{%s, MetaType_%.*s, \"%.*s\", \"%.*s\", (u32) (&((%.*s*)0)->%.*s), {}, sizeof(%.*s),\"%.*s\"",
                       flags, 
                       fieldNameToken.textLength, fieldNameToken.text, 
                       fieldNameToken.textLength, fieldNameToken.text,
                       nameToken.textLength, nameToken.text, 
                       structToken.textLength, structToken.text, 
                       nameToken.textLength, nameToken.text,
                       fieldNameToken.textLength, fieldNameToken.text,
                       optionsToken.textLength, optionsToken.text);
                if(!TokenEquals(counterToken, "invalid"))
                {
                    printf(",\"%.*s\", (u32)(&((%.*s*)0)->%.*s)", 
                           counterToken.textLength, counterToken.text,
                           structToken.textLength, structToken.text, 
                           counterToken.textLength, counterToken.text);
                }
                else
                {
                    printf(",0, 0");
                }
                
                if(!TokenEquals(fixedToken, "invalid"))
                {
                    printf(", \"%.*s\"", fixedToken.textLength, fixedToken.text);
                }
                else
                {
                    printf(", 0");
                }
                
                if(!TokenEquals(enumToken, "invalid"))
                {
                    printf(", MetaTable_%.*s, ArrayCount(MetaTable_%.*s)", 
                           enumToken.textLength, enumToken.text,
                           enumToken.textLength, enumToken.text);
                }
                
                printf("}, \n");
                
            } break;
        }
    }
    
    if(hasDefault)
    {
        fieldDefaultValuePtr += sprintf(fieldDefaultValuePtr, "fieldDefinitionOf%.*s[%d].def.def_%.*s =%.*s;", structToken.textLength, structToken.text, fieldIndex, fieldNameToken.textLength, fieldNameToken.text, defaultValue.textLength, defaultValue.text);
    }
}

struct MetaStruct
{
    char* name;
    MetaStruct* next;
};

static MetaStruct* firstStruct;
void ParseStruct(Tokenizer* tokenizer)
{
    Token nameToken = GetToken( tokenizer );
    //printf( "#ifdef INTROSPECTION\n");
    printf( "FieldDefinition fieldDefinitionOf%.*s[] = \n {\n", nameToken.textLength, nameToken.text );
    
    if(RequireToken( tokenizer, Token_OpenBraces))
    {
        u32 fieldIndex = 0;
        for(;;)
        {
            Token fieldType = GetToken( tokenizer );
            if( fieldType.type == Token_CloseBraces )
            {
                break;
            }
            else
            {
                ParseField(tokenizer, nameToken, fieldType, fieldIndex++);
            }
        }
    }
    printf( "};\n" );
    printf( "\n" );
    //printf( "#endif\n");
    
    MetaStruct* meta = ( MetaStruct* ) malloc( sizeof( MetaStruct ) );
    meta->name = ( char* ) malloc( nameToken.textLength + 1 );
    memcpy( meta->name, nameToken.text, nameToken.textLength );
    meta->name[nameToken.textLength] = 0;
    
    meta->next = firstStruct;
    firstStruct = meta;
}

void ParseIntrospection( Tokenizer* tokenizer )
{
    if( RequireToken( tokenizer, Token_OpenParen ) )
    {
        ParseIntrospectParam( tokenizer );
        
        Token structToken = GetToken( tokenizer );
        if( TokenEquals( structToken, "struct" ) )
        {
            ParseStruct( tokenizer );
        }
        else
        {
            
        }
    }
    else
    {
        
    }
}


inline Token FirstUnderScore( Token token )
{
    Token result = token;
    
    Assert( token.type == Token_Identifier );
    char* at = token.text;
    
    int textLength = token.textLength;
    while( textLength-- )
    {
        if( *at++ == '_' )
        {
            result.text = at;
            result.textLength = textLength;
        }
    }
    
    return result;
}

struct MetaProperty
{
    char* name;
    MetaProperty* next;
};
global_variable MetaProperty* firstPropertyList;

void ParseProperties(Tokenizer* tokenizer)
{
    if(RequireToken(tokenizer, Token_OpenParen))
    {
        Token nameToken = GetToken(tokenizer);
        Assert( nameToken.type == Token_Identifier );
        printf( "char* MetaProperties_%.*s[] = \n {\n", nameToken.textLength, nameToken.text );
        
        if(RequireToken(tokenizer, Token_CloseParen))
        {
            if(RequireToken(tokenizer, Token_OpenBraces))
            {
                for(;;)
                {
                    Token element = GetToken(tokenizer);
                    if( element.type == Token_CloseBraces)
                    {
                        break;
                    }
                    else
                    {
                        if(element.type == Token_Identifier)
                        {
                            element = FirstUnderScore(element);
                            printf( "\"%.*s\",\n", element.textLength, element.text );
                        }
                    }
                }
            }
            printf( "};\n" );
            printf( "\n" );
            
            
            
            MetaProperty* meta = ( MetaProperty* ) malloc( sizeof( MetaProperty ) );
            meta->name = ( char* ) malloc(nameToken.textLength + 1);
            memcpy(meta->name, nameToken.text, nameToken.textLength);
            meta->name[nameToken.textLength] = 0;
            
            meta->next = firstPropertyList;
            firstPropertyList = meta;
        }
    }
    else
    {
        InvalidCodePath;
    }
}


void ParseTable(Tokenizer* tokenizer, bool printPrefix)
{
    Token nameToken = GetToken(tokenizer);
    Assert( nameToken.type == Token_Identifier );
    printf( "char* MetaTable_%.*s[] = \n {\n", nameToken.textLength, nameToken.text );
    
    if( RequireToken( tokenizer, Token_OpenBraces ) )
    {
        for(;;)
        {
            Token element = GetToken( tokenizer );
            if( element.type == Token_CloseBraces )
            {
                break;
            }
            else
            {
                if( element.type == Token_Identifier )
                {
                    if(!printPrefix)
                    {
                        element = FirstUnderScore(element);
                    }
                    printf( "\"%.*s\",\n", element.textLength, element.text );
                }
            }
        }
    }
    printf( "};\n" );
    printf( "\n" );
}


bool ParseTableParam(Tokenizer* tokenizer)
{
    bool result = true;
    for(;;)
    {
        Token token = GetToken( tokenizer );
        if(token.type == Token_Identifier)
        {
            if(TokenEquals(token, "noPrefix"))
            {
                result = false;
            }
        }
        
        if( token.type == Token_CloseParen || token.type == Token_EndOfFile )
        {
            break;
        }
    }
    
    return result;
}

void ParseEnumTable(Tokenizer* tokenizer)
{
    if( RequireToken( tokenizer, Token_OpenParen ) )
    {
        bool printPrefix = ParseTableParam(tokenizer);
        
        Token enumToken = GetToken(tokenizer);
        if( TokenEquals( enumToken, "enum" ) )
        {
            ParseTable(tokenizer, printPrefix);
        }
        else
        {
            
        }
    }
    else
    {
        
    }
}


void ParseFlags(Tokenizer* tokenizer, bool printPrefix)
{
    Token nameToken = GetToken( tokenizer );
    Assert( nameToken.type == Token_Identifier );
    printf( "MetaFlag MetaFlags_%.*s[] = \n {\n", nameToken.textLength, nameToken.text );
    
    if( RequireToken( tokenizer, Token_OpenBraces ) )
    {
        char* flagValue = 0;
        for(;;)
        {
            Token element = GetToken( tokenizer );
            if( element.type == Token_CloseBraces )
            {
                break;
            }
            else
            {
                if( element.type == Token_Identifier )
                {
                    if(!printPrefix)
                    {
                        element = FirstUnderScore(element);
                    }
					printf( "{\"%.*s\", ", element.textLength, element.text );
                }
				else if(element.type == Token_OpenParen)
				{
					flagValue = element.text;
				}
				else if(element.type == Token_CloseParen)
				{
                    int counter = (int) (element.text - flagValue) + 1;
					printf("%.*s},\n", counter, flagValue);
				}
            }
        }
    }
    printf( "};\n" );
    printf( "\n" );
}

void ParseFlags( Tokenizer* tokenizer )
{
    if( RequireToken( tokenizer, Token_OpenParen ) )
    {
        bool printPrefix = ParseTableParam(tokenizer);
        
        Token enumToken = GetToken( tokenizer );
        if( TokenEquals( enumToken, "enum" ) )
        {
            ParseFlags(tokenizer, printPrefix);
        }
        else
        {
            
        }
    }
    else
    {
        
    }
}


struct MetaArchetype
{
    char* name;
    MetaArchetype* next;
};
global_variable MetaArchetype* firstMetaArchetype;


char metaArchetypesServer[KiloBytes(16)] = {};
char* metaArchetypesServerPtr = metaArchetypesServer;

char metaArchetypesClient[KiloBytes(16)] = {};
char* metaArchetypesClientPtr = metaArchetypesClient;

char metaArchetypesBoth[KiloBytes(16)] = {};
char* metaArchetypesBothPtr = metaArchetypesBoth;

void ParseArchetype(Tokenizer* tokenizer)
{
    if(RequireToken(tokenizer, Token_OpenParen))
    {
        if(RequireToken(tokenizer, Token_CloseParen))
        {
            if(RequireToken(tokenizer, Token_Identifier))
            {
                Token name = GetToken(tokenizer);
                MetaArchetype* meta = ( MetaArchetype* ) malloc( sizeof( MetaArchetype ) );
                meta->name = ( char* ) malloc(name.textLength + 1);
                memcpy(meta->name, name.text, name.textLength);
                meta->name[name.textLength] = 0;
                
                meta->next = firstMetaArchetype;
                firstMetaArchetype = meta;
                
                metaArchetypesBothPtr += sprintf(metaArchetypesBothPtr, "archetypeLayouts[Archetype_%s].totalSize = sizeof(%s); ", meta->name, meta->name);
                
                
                b32 parsing = true;
                
                b32 server = false;
                b32 client = false;
                b32 both = true;
                
                while(parsing)
                {
                    Token t = GetToken(tokenizer);
                    
                    
                    switch(t.type)
                    {
                        case Token_Pound:
                        {
                            Token def = GetToken(tokenizer);
                            if(TokenEquals(def, "ifdef"))
                            {
                                Token definition = GetToken(tokenizer);
                                if(TokenEquals(definition, "FORG_SERVER"))
                                {
                                    server = true;
                                    client = false;
                                }
                            }
                            else if(TokenEquals(def, "ifndef"))
                            {
                                Token definition = GetToken(tokenizer);
                                if(TokenEquals(definition, "FORG_SERVER"))
                                {
                                    client = true;
                                    server = false;
                                }
                            }
                            else if(TokenEquals(def, "else"))
                            {
                                if(server)
                                {
                                    server = false;
                                    client = true;
                                }
                                else if(client)
                                {
                                    server = true;
                                    client = false;
                                }
                            }
                            else if(TokenEquals(def, "endif"))
                            {
                                server = false;
                                client = false;
                            }
                            
                        } break;
                        
                        case Token_Identifier:
                        {
                            char** active = &metaArchetypesBothPtr;
                            
                            if(server)
                            {
                                Assert(!client);
                                active = &metaArchetypesServerPtr;
                            }
                            
                            if(client)
                            {
                                Assert(!server)
                                {
                                    active = &metaArchetypesClientPtr;
                                }
                            }
                            
                            (*active) += sprintf(*active, "archetypeLayouts[Archetype_%s].has%.*s.exists = true; ", meta->name, t.textLength, t.text);
                            
                            if(NextTokenIs(tokenizer, Token_Asterisk))
                            {
                                Token asterisk = GetToken(tokenizer);
                                (*active) += sprintf(*active, "archetypeLayouts[Archetype_%s].has%.*s.pointer = true; ", meta->name, t.textLength, t.text);
                            }
                            Token comp = GetToken(tokenizer);
                            Token semi = GetToken(tokenizer);
                            
                            (*active) += sprintf(*active, "archetypeLayouts[Archetype_%s].has%.*s.offset = OffsetOf(%s, %.*s);  ", meta->name, t.textLength, t.text, meta->name, comp.textLength, comp.text);
                        } break;
                        
                        case Token_SemiColon:
                        {
                            parsing = false;
                        } break;
                        
                        case Token_EndOfFile:
                        {
                            parsing = false;
                        } break;
                    }
                }
            }
        }
    }
}

int main(int argc, char** argv)
{
    char* fileNames[] =
    {
        "forg_platform.h",
        "forg_math.h",
        "forg_action_effect.h",
        "forg_plant.h",
        "forg_inventory.h",
        "forg_ground.h",
        "forg_world_generation.cpp",
        "forg_world.h",
        "forg_world_generation.h",
        "forg_AI.cpp",
        "forg_AI.h",
        "forg_editor.h",
        "forg_animation.h",
        "forg_particles.h",
        "forg_asset.h",
        "forg_ecs.h",
        "forg_archetypes.h",
        "../properties/test.properties",
    };
    
    
    
    for( int fileIndex = 0; fileIndex < sizeof( fileNames ) / sizeof( fileNames[0] ); ++fileIndex )
    {
        char* file = ReadEntireFileAndNullTerminate( fileNames[fileIndex] );
        
        bool parsing = true;
        
        Tokenizer tokenizer = {};
        tokenizer.at = file;
        
        bool buildingNodes = false;
        bool parsingRecipe = false;
        while( parsing )
        {
            Token token = GetToken( &tokenizer );
            switch( token.type )
            {
                case Token_EndOfFile:
                {
                    parsing = false;
                } break;
                
                case Token_Unknown:
                {
                    
                } break;
                
                case Token_Identifier:
                {
                    if(TokenEquals(token, "Archetype"))
                    {
                        ParseArchetype(&tokenizer);
                    }
                } break;
                
                default:
                {
                    //printf( "%d: %.*s\n", token.type, token.textLength, token.text );
                } break;
            }
        }
    }
    
    
    printf("enum EntityArchetype\n{");
    for(MetaArchetype* arch = firstMetaArchetype; arch; arch = arch->next)
    {
        printf("Archetype_%s,\n", arch->name);
    }
    printf("Archetype_Count\n");
    printf("};\n");
    
    printf("char* MetaTable_EntityArchetype[] = \n{");
    for(MetaArchetype* arch = firstMetaArchetype; arch; arch = arch->next)
    {
        printf("\"%s\",\n", arch->name);
    }
    printf("};\n");
    
    printf("#define META_ARCHETYPES_INIT_FUNC()\\\n");
    for(MetaArchetype* arch = firstMetaArchetype; arch; arch = arch->next)
    {
        printf("InitFunc[Archetype_%s] = Init%s;", arch->name, arch->name);
    }
    printf("\n;");
    printf("\n");
    
    for( int fileIndex = 0; fileIndex < sizeof( fileNames ) / sizeof( fileNames[0] ); ++fileIndex )
    {
        char* file = ReadEntireFileAndNullTerminate( fileNames[fileIndex] );
        
        bool parsing = true;
        
        Tokenizer tokenizer = {};
        tokenizer.at = file;
        
        bool buildingNodes = false;
        bool parsingRecipe = false;
        while( parsing )
        {
            Token token = GetToken( &tokenizer );
            switch( token.type )
            {
                case Token_EndOfFile:
                {
                    parsing = false;
                } break;
                
                case Token_Unknown:
                {
                    
                } break;
                
                case Token_Identifier:
                {
                    if( TokenEquals( token, "introspection" ) )
                    {
                        ParseIntrospection( &tokenizer );
                    }
                    else if( TokenEquals( token, "printTable" ) )
                    {
                        ParseEnumTable( &tokenizer );
                    }
                    else if(TokenEquals(token, "Property"))
                    {
                        ParseProperties(&tokenizer);
                    }
                    
                    else if(TokenEquals(token, "printFlags"))
                    {
                        ParseFlags(&tokenizer);
                    }
                } break;
                
                default:
                {
                    //printf( "%d: %.*s\n", token.type, token.textLength, token.text );
                } break;
            }
        }
    }
    
    printf( "#define META_HANDLE_ADD_TO_DEFINITION_HASH()\\\n" );
    for(MetaStruct* meta = firstStruct; meta; meta = meta->next)
    {
        printf("AddToMetaDefinitions(%s, fieldDefinitionOf%s);", meta->name, meta->name); 
        
        printf( meta->next ? "\\" : "" );
        printf( "\n" );
    }
    printf( "\n" );
    
    printf( "#define META_PROPERTIES_ADD()\\\n" );
    for(MetaProperty* meta = firstPropertyList; meta; meta = meta->next)
    {
        printf("AddToMetaProperties(%s, MetaProperties_%s);", meta->name, meta->name); 
        
        printf( meta->next ? "\\" : "" );
        printf( "\n" );
    }
    printf( "\n" );
    
    printf("enum Propertys\n{\n");
    printf("Property_Invalid,\n");
    for(MetaProperty* meta = firstPropertyList; meta; meta = meta->next)
    {
        printf("Property_%s,", meta->name); 
        printf( "\n" );
    }
    
    printf("Property_Count,\n");
    printf( "};\n" );
    
    printf("#define META_ASSET_PROPERTIES_STRINGS()\\\n");
    for(MetaProperty* meta = firstPropertyList; meta; meta = meta->next)
    {
        printf("meta_propertiesString[Property_%s - 1] = \"%s\";\\\n", meta->name, meta->name);
    }
    printf("\n");
    
    
    printf("#define META_DEFAULT_VALUES_CPP_SUCKS()\\\n");
    printf(fieldDefaultValues);
    printf("\n;");
    printf("\n");
    
    printf("#define META_ARCHETYPES_BOTH()\\\n");
    printf(metaArchetypesBoth);
    printf("\n;");
    printf("\n");
    
    printf("#define META_ARCHETYPES_SERVER()\\\n");
    printf(metaArchetypesServer);
    printf("\n;");
    printf("\n");
    
    printf("#define META_ARCHETYPES_CLIENT()\\\n");
    printf(metaArchetypesClient);
    printf("\n;");
    printf("\n");
    
}

