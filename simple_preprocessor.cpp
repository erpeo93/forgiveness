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
                    printf( "{ MetaType_%.*s, \"%.*s\", (u32) ( &(( %.*s* )0)->%.*s ) }, \n",memberNameToken.textLength, memberNameToken.text, token.textLength, token.text, structToken.textLength, structToken.text, token.textLength, token.text );
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

void ParseStruct( Tokenizer* tokenizer )
{
    Token nameToken = GetToken( tokenizer );
    printf( "#ifdef INTROSPECTION\n");
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
    printf( "#endif\n");
    
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
					printf( "{\"%.*s\",\n", element.textLength, element.text );
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
}

void ParseEnumTable( Tokenizer* tokenizer )
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


static int counter;
void ParseTaxonomies( Tokenizer* tokenizer )
{
    char names[32][1024];
    int count[32];
    int index = 0;
    bool valid = false;
    
    for( ;; )
    {
        Token token = GetToken( tokenizer );
        if( TokenEquals( token, "EndTaxonomyPreparsing" ) )
        {
            if( valid )
            {
                printf( "#define TAXOSUB_root %d\n", count[0] );
                int toPrint = 32 - ( ( int ) log2f( ( float ) count[0] )  + 1 );
                
                printf( "#define ROOT_BITS %d\n", toPrint );
            }
            
            return;
        }
        else if( TokenEquals( token, "Push" ) )
        {
            valid = true;
            ++count[index++];
            if( RequireToken( tokenizer, Token_OpenParen ) )
            {
                Token name = GetToken( tokenizer );
                sprintf( names[index], "%.*s", name.textLength, name.text ); 
            }
            else
            {
                Assert( !"noooo" );
            }
            
            if( index == 1 )
            {
                printf( "#define %s_INDEX %d\n", names[index], ++counter );
            }
            
        }
        else if( TokenEquals( token, "AddSpecies" ) )
        {
            ++count[index];
        }
        else if( TokenEquals( token, "Pop" ) )
        {
            printf( "#define TAXOSUB_%s %d\n", names[index], count[index] );
            count[index] = 0;
            
            --index;
            Assert( index >= 0 );
        }
    }
}

void InsertUnderscores( char* source, int sourceLength, char* dest )
{
    char* word = source;
    
    int wordCount = 0;
    int wordLength = 0;
    
    for( int charIndex = 0; charIndex < sourceLength; charIndex++ )
    {
        wordLength++;
        
        char current = source[charIndex];
        if( current == ' ' )
        {
            printf( "%.*s_", wordLength - 1, word );
            word += wordLength;
            wordLength = 0;
        }
        else if( charIndex == sourceLength - 1 )
        {
            printf( "%.*s", wordLength, word );
            word += wordLength;
            wordLength = 0;
        }
    }
}

void EndParseRecipes( Tokenizer* tokenizer )
{
    printf( "};\n" );
}

struct ParseAINodeParams
{
    Token ID;
    Token nodeName;
    Token pool;
    Token string;
};

void ParseDebugAnnotationParam( Tokenizer* tokenizer )
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

static int countParsedFiles = 1;
static char parsedFiles[1024][256];

inline bool AlreadyParsed( char* fileName )
{
    bool result = false;
    for( int fileIndex = 0; fileIndex < ArrayCount( parsedFiles ); ++fileIndex )
    {
        char* parsed = parsedFiles[fileIndex];
        if( strcmp( parsed, fileName ) == 0 )
        {
            result = true;
            break;
        }
    }
    
    return result;
}

inline void SignAsParsed( char* filename )
{
    char* parsed = parsedFiles[countParsedFiles++];
    strcpy( parsed, filename );
}

void RecursivelyParseAllFiles( char* filename )
{
    char* file = ReadEntireFileAndNullTerminate( filename );
    
    if( file )
    {
        bool parsing = true;
        Tokenizer tokenizer = {};
        tokenizer.at = file;
        while( parsing )
        {
            Token token = GetToken( &tokenizer );
            switch( token.type )
            {
                case Token_EndOfFile:
                {
                    parsing = false;
                } break;
                
                case Token_Pound:
                {
                    Token includeToken = GetToken( &tokenizer );
                    if( TokenEquals( includeToken, "include" ) )
                    {
                        Token otherFileName = GetToken( &tokenizer );
                        if( otherFileName.type == Token_String )
                        {
                            char newFile[256];
                            sprintf( newFile, "%.*s", otherFileName.textLength - 1, otherFileName.text + 1 );
                            newFile[otherFileName.textLength - 1] = 0;
                            
                            if( !AlreadyParsed( newFile ) )
                            {
                                RecursivelyParseAllFiles( newFile );
                                SignAsParsed( newFile );
                            }
                        }
                    }
                } break;
                
                case Token_Unknown:
                {
                    
                } break;
                
                case Token_Identifier:
                {
                } break;
                
                default:
                {
                    //printf( "%d: %.*s\n", token.type, token.textLength, token.text );
                } break;
            }
        }
    }
}

void ParseEffectValuesFrom( char* filename )
{
    printf( "#define META_FILL_EFFECTS_VALUE " );
    char* file = ReadEntireFileAndNullTerminate( filename );
    bool parsing = true;
    
    Token currentGroupType = {};
    Token currentEffectID = {};
    
    char stringToPrintAtStart[2048] = {};
    char stringToBuild[2048] = {};
    char* currentStringToBuild = stringToBuild;
    char paramsToBuild[2048] = {};
    char* currentParamsToBuild = paramsToBuild;
    
    bool encounteredSatisfaction = false;
    bool firstSatisfaction = false;
    bool addedPerceivedInfo = false;
    bool parsingTable = false;
    Tokenizer tokenizer = {};
    tokenizer.at = file;
    
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
                if( TokenEquals( token, "MetaSat" ) )
                {
                    if( RequireToken( &tokenizer, Token_OpenParen ) )
                    {
                        Token satisfactionName = GetToken( &tokenizer );
                        Token c1 = GetToken( &tokenizer );
                        Token weight = GetToken( &tokenizer );
                        Token c2 = GetToken( &tokenizer );
                        Token string = Stringize( GetToken( &tokenizer ) );
                        
                        encounteredSatisfaction = true;
                        if( firstSatisfaction )
                        {
                            firstSatisfaction = false;
                            sprintf( stringToPrintAtStart, "array->nodes[%.*s].node = PushStruct( pool, AIOperationNode ); newNode = array->nodes[%.*s].node;", currentEffectID.textLength, currentEffectID.text, currentEffectID.textLength, currentEffectID.text );
                            currentStringToBuild += sprintf( currentStringToBuild, "WSum( " );
                            currentParamsToBuild += sprintf( currentParamsToBuild, "[ " );
                        }
                        else
                        {
                            currentStringToBuild += sprintf( currentStringToBuild, ", " );
                            currentParamsToBuild += sprintf( currentParamsToBuild, ", " );
                        }
                        
                        currentStringToBuild += sprintf( currentStringToBuild, "Satis( %.*s )[Satisfaction_%.*s]", string.textLength, string.text, satisfactionName.textLength, satisfactionName.text );
                        currentParamsToBuild += sprintf( currentParamsToBuild, "%.*s", weight.textLength, weight.text );
                    }
                }
                else if( TokenEquals( token, "metaParseEffects" ) )
                {
                    parsingTable = true;
                    if( RequireToken( &tokenizer, Token_OpenParen ) )
                    {
                        Token closeParen = GetToken( &tokenizer );
                        Token voidToken = GetToken( &tokenizer );
                        Token name = GetToken( &tokenizer );
                        currentGroupType = name;
                        
                        char nodeCount[64] = {};
                        printf( "array = effectNodes + EffectType_%.*s;", currentGroupType.textLength, currentGroupType.text );
                        if( TokenEquals( name, "Sickness" ) )
                        {
                            sprintf( nodeCount, "Sickness_count" );
                        }
                        else if( TokenEquals( name, "Combat" ) )
                        {
                            sprintf( nodeCount, "CE_count" );
                        }
                        else if( TokenEquals( name, "Magic" ) )
                        {
                            sprintf( nodeCount, "ME_count" );
                        }
                        else if( TokenEquals( name, "InventoryTrigger" ) )
                        {
                            sprintf( nodeCount, "IT_count" );
                        }
                        else
                        {
                            Assert( 0 );
                        }
                        printf( "array->nodes = PushArray( pool, EffectEvaluation, %s );", nodeCount );
                    }
                }
                else if( TokenEquals( token, "case" ) )
                {
                    encounteredSatisfaction = false;
                    addedPerceivedInfo = false;
                    if( parsingTable )
                    {
                        firstSatisfaction = true;
                        memset( stringToBuild, 0, sizeof( stringToBuild ) );
                        currentEffectID = GetToken( &tokenizer );
                        Assert( currentGroupType.text );
                        Assert( currentEffectID.text );
                        printf( "array->nodes[%.*s] = {};", currentEffectID.textLength, currentEffectID.text );
                    }
                }
                else if( TokenEquals( token, "InvalidDefaultCase" ) )
                {
                    parsingTable = false;
                }
            } break;
            
            default:
            {
            } break;
        }
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
    
    
    printf( "#define META_HANDLE_TYPE_DUMP( MemberPtr, indentLevel )" );
    for( MetaStruct* meta = firstStruct; meta; meta = meta->next )
    {
        printf( "case MetaType_%s:{DEBUGTextLine( member->name ); DumpStruct( memberPtr, memberDefinitionOf%s, ArrayCount(memberDefinitionOf%s ), indentLevel );} break;", meta->name, meta->name, meta->name );
        
        printf( meta->next ? "\\" : "" );
        printf( "\n" );
    }
    printf( "\n" );
    
    RecursivelyParseAllFiles( "forg_server.cpp" );
}

