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
    bool nameParsed = false;
    Token defaultValue = {};
    bool hasDefault = false;
    
    Token optionsToken = {};
    optionsToken.text = "invalid";
    optionsToken.textLength = StrLen(optionsToken.text);
    Token counterToken = optionsToken;
    Token fixedToken = optionsToken;
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
                parsing = false;
                printf("{%s, MetaType_%.*s, \"%.*s\", \"%.*s\", (u32) (&((%.*s*)0)->%.*s), {}, sizeof(%.*s),\"%.*s\"",
                       isPointer ? "MetaFlag_Pointer" : "0", 
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

struct MetaLabel
{
    char* name;
    MetaLabel* next;
};


bool ParseLabelParam(Tokenizer* tokenizer)
{
    bool result = true;
    for(;;)
    {
        Token token = GetToken( tokenizer );
        if(token.type == Token_Identifier)
        {
        }
        
        if( token.type == Token_CloseParen || token.type == Token_EndOfFile )
        {
            break;
        }
    }
    return result;
}

global_variable MetaLabel* firstLabelList;
void ParseLabels(Tokenizer* tokenizer)
{
    ParseLabelParam(tokenizer);
    
    Token enumToken = GetToken(tokenizer);
    if( TokenEquals( enumToken, "enum" ) )
    {
        Token nameToken = GetToken(tokenizer);
        Assert( nameToken.type == Token_Identifier );
        printf( "char* MetaLabels_%.*s[] = \n {\n", nameToken.textLength, nameToken.text );
        
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
        
        
        
        MetaLabel* meta = ( MetaLabel* ) malloc( sizeof( MetaLabel ) );
        meta->name = ( char* ) malloc(nameToken.textLength + 1);
        memcpy(meta->name, nameToken.text, nameToken.textLength);
        meta->name[nameToken.textLength] = 0;
        
        meta->next = firstLabelList;
        firstLabelList = meta;
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


int main( int argc, char** argv )
{
    char* fileNames[] =
    {
        "forg_platform.h",
        "forg_math.h",
        "forg_action_effect.h",
        "forg_plant.h",
        "forg_inventory.h",
        "forg_world_generation.cpp",
        "forg_AI.cpp",
        "forg_AI.h",
        "forg_editor.h",
        "forg_client.h",
        "forg_animation.h",
        "forg_particles.h",
        "forg_asset.h",
        "asset_labels.h",
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
                    if( TokenEquals( token, "introspection" ) )
                    {
                        ParseIntrospection( &tokenizer );
                    }
                    else if( TokenEquals( token, "printTable" ) )
                    {
                        ParseEnumTable( &tokenizer );
                    }
                    else if(TokenEquals(token, "printLabels"))
                    {
                        ParseLabels(&tokenizer);
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
    
    printf( "#define META_LABELS_ADD()\\\n" );
    for(MetaLabel* meta = firstLabelList; meta; meta = meta->next)
    {
        printf("AddToMetaLabels(%s, MetaLabels_%s);", meta->name, meta->name); 
        
        printf( meta->next ? "\\" : "" );
        printf( "\n" );
    }
    printf( "\n" );
    
    printf("enum Labels\n{\n");
    printf("Label_Invalid,\n");
    for(MetaLabel* meta = firstLabelList; meta; meta = meta->next)
    {
        printf("%s,", meta->name); 
        printf( "\n" );
    }
    
    printf("Label_Count,\n");
    printf( "};\n" );
    
    printf("#define META_ASSET_LABEL_STRINGS()\\\n");
    for(MetaLabel* meta = firstLabelList; meta; meta = meta->next)
    {
        printf("meta_labelsString[%s - 1] = \"%s\";\\\n", meta->name, meta->name);
    }
    printf("\n");
    
    
    printf("#define META_DEFAULT_VALUES_CPP_SUCKS()\\\n");
    printf(fieldDefaultValues);
    printf("\n;");
    printf("\n");
}

