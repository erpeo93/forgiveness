#include "forg_token.h"
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <math.h>

#define Assert( expression ) if( !( expression ) ) { *( ( int* ) 0 ) = 0; }
#define ArrayCount( value ) ( sizeof( value ) / sizeof( ( value )[0] ) )

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

void ParseMember( Tokenizer* tokenizer, Token structToken, Token memberNameToken )
{
    bool parsing = true;
    bool isPointer = false;
    bool nameParsed = false;
    while( parsing )
    {
        Token token = GetToken( tokenizer );
        switch( token.type )
        {
            case Token_Asterisk:
            {
                isPointer = true;
            } break;
            
            case Token_Identifier:
            {
                if( !nameParsed )
                {
                    
#if 0                    
                    printf( "{ %s, MetaType_%.*s, \"%.*s\", (u32) ( &(( %.*s* )0)->%.*s ) }, \n", isPointer ? "MetaFlag_Pointer" : "0",memberNameToken.textLength, memberNameToken.text, token.textLength, token.text, structToken.textLength, structToken.text, token.textLength, token.text );
#else
                    printf( "{0, MetaType_%.*s, \"%.*s\", (u32) (&((%.*s*)0)->%.*s)}, \n",memberNameToken.textLength, memberNameToken.text, token.textLength, token.text, structToken.textLength, structToken.text, token.textLength, token.text );
#endif
                    
                    nameParsed = true;
                }
            } break;
            
            case Token_SemiColon:
            {
                parsing = false;
            } break;
        }
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
    printf( "MemberDefinition memberDefinitionOf%.*s[] = \n {\n", nameToken.textLength, nameToken.text );
    
    if( RequireToken( tokenizer, Token_OpenBraces ) )
    {
        for(;;)
        {
            Token memberType = GetToken( tokenizer );
            if( memberType.type == Token_CloseBraces )
            {
                break;
            }
            else
            {
                ParseMember( tokenizer, nameToken, memberType );
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

void ParseTable(Tokenizer* tokenizer, bool printPrefix)
{
    Token nameToken = GetToken( tokenizer );
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


bool ParseTableParam( Tokenizer* tokenizer )
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
        
        Token enumToken = GetToken( tokenizer );
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
        "forg_region.h",
        "forg_platform.h",
        "forg_math.h",
        "forg_action_effect.h",
        "forg_plant.h",
        "forg_inventory.h",
        "forg_taxonomy.h",
        "forg_world_generation.cpp",
        "forg_AI.cpp",
        "forg_AI.h",
        "forg_memory.h",
        "forg_asset_enum.h",
        "forg_taxonomy.cpp",
        "forg_editor.h",
        "forg_client.h",
        "forg_animation.h",
        "forg_particles.h",
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
        printf("AddToDefinitionHash(definitionHash, %s, memberDefinitionOf%s);", meta->name, meta->name); 
        
        printf( meta->next ? "\\" : "" );
        printf( "\n" );
    }
    printf( "\n" );
}

