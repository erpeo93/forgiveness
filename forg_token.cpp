inline bool IsDecimalNumber(Token t)
{
    bool result = false;
    for( int charIndex = 0; charIndex < t.textLength; ++charIndex )
    {
        if( t.text[charIndex] == '.' )
        {
            result = true;
            break;
        }
    }
    
    return result;
}



void EatAllWhiteSpace(Tokenizer* tokenizer)
{
    for(;;)
    {
        if( IsWhiteSpace( tokenizer->at[0] ) )
        {
            ++tokenizer->at;
        }
        else if( tokenizer->at[0] == '/' && tokenizer->at[1] == '/' )
        {
            tokenizer->at += 2;
            while( tokenizer->at[0] && !IsEndOfLine( tokenizer->at[0] ) )
            {
                ++tokenizer->at;
            }
        }
        else if( tokenizer->at[0] == '/' && tokenizer->at[1] == '*' )
        {
            tokenizer->at += 2;
            while( tokenizer->at[0] && 
                  tokenizer->at[0] != '*' && 
                  tokenizer->at[1] != '/' )
            {
                ++tokenizer->at;
            }
            if( tokenizer->at[0] == '*' )
            
            {
                tokenizer->at += 2;
            }
        }
        else
        {
            break;
        }
    }
}

Token GetToken( Tokenizer* tokenizer )
{
    EatAllWhiteSpace( tokenizer );
    
    Token result = {};
    result.textLength = 1;
    char C = tokenizer->at[0];
    result.text = tokenizer->at++;
    switch( C )
    {
        case '\0': result.type = Token_EndOfFile; break;
        case '(': result.type = Token_OpenParen; break; 
        case ')': result.type = Token_CloseParen; break; 
        case ':': result.type = Token_Colon; break; 
        case '[': result.type = Token_OpenBracket; break; 
        case ']': result.type = Token_CloseBracket; break; 
        case '{': result.type = Token_OpenBraces; break; 
        case '}': result.type = Token_CloseBraces; break; 
        case '*': result.type = Token_Asterisk; break; 
        case ';': result.type = Token_SemiColon; break; 
        case '#': result.type = Token_Pound; break; 
        case ',': result.type = Token_Comma; break; 
        case '=': result.type = Token_EqualSign; break; 
        case '/': result.type = Token_Slash; break; 
        
        case '"':
        {
            result.type = Token_String;
            
            while( tokenizer->at[0] != '"' && tokenizer->at[0] != '\0' )
            {
                ++tokenizer->at;
                ++result.textLength;
                
                if( tokenizer->at[0] == '\\' && ( tokenizer->at[1] != '\0' ) )
                {
                    ++tokenizer->at;
                    ++result.textLength;
                }
            }
            
            if( tokenizer->at[0] == '"' )
            {
                ++tokenizer->at;
            }
        } break;
        
        default:
        {
            if(IsAlphanumeric(C))
            {
                while(IsAlphanumeric(tokenizer->at[0]) || IsNumeric(tokenizer->at[0]) || tokenizer->at[0] == '_')
                {
                    ++tokenizer->at;
                    ++result.textLength;
                }
                
                result.type = Token_Identifier;
            }
            else if(IsNumeric(C) || C == '-')
            {
                if(C == '-')
                {
                    ++tokenizer->at;
                    ++result.textLength;
                }
                
                bool pointParsed = false;
                bool EParsed = false;
                
                while((IsNumeric(tokenizer->at[0] ) || ( tokenizer->at[0] == '.' && !pointParsed ) || (( tokenizer->at[0] == 'E' && !EParsed )) ) && tokenizer->at[0] != '\0')
                {
                    if(tokenizer->at[0] == '.')
                    {
                        pointParsed = true;
                    }
                    
                    if(tokenizer->at[0] == 'E')
                    {
                        if(tokenizer->at[1] == '+')
                        {
                            ++tokenizer->at;
                            ++result.textLength;
                            EParsed = true;
                        }
                    }
                    ++tokenizer->at;
                    ++result.textLength;
                    
                }
                
                result.type = Token_Number;
            }
            else
            {
                result.type = Token_Unknown;
            }
        }
    }
    
    return result;
}

inline bool TokenEquals( Token token, char* name )
{
    char* at = name;
    for( int index = 0; index < token.textLength; index++, at++)
    {
        if( !( *at ) || ( *at != token.text[index] ) )
        {
            return false;
        }
    }
    
    bool result = (*at == 0);
    return result;
}

inline bool SameToken(Token t1, Token t2)
{
    bool result = false;
    if( t1.textLength == t2.textLength )
    {
        result = true;
        for( int index = 0; index < t1.textLength; index++ )
        {
            if( t1.text[index] != t2.text[index] )
            {
                result = false;
                break;
            }
        }
        
    }
    return result;
}

inline Token Stringize(Token token)
{
    if(token.type == Token_String)
    {
        Assert( token.text[0] == '"' );
        if( token.text[token.textLength] != '"' )
        {
            token.type = Token_Unknown;
        }
        else
        {
            token.type = Token_Identifier;
        }
        
        token.text++;
        token.textLength--;
    }
    
    return token;
}

inline Buffer BufferizeFirstTokenAndAdvance(Tokenizer* tokenizer)
{
    Buffer result;
    
    Token t = GetToken(tokenizer);
    
    result.ptr = t.text;
    result.length = t.textLength;
    
    return result;
}

inline void AdvanceToNextToken(Tokenizer* tokenizer, ForgTokenType type)
{
    bool parsing = true;
    while(parsing)
    {
        Token t = GetToken(tokenizer);
        
        if(t.type == type || t.type == Token_EndOfFile)
        {
            parsing = false;
        }
    }
}

inline void AdvanceToNextToken(Tokenizer* tokenizer, char* tokenName)
{
    bool parsing = true;
    while(parsing)
    {
        Token t = GetToken(tokenizer);
        
        if(t.type == Token_String)
        {
            t = Stringize(t);
        }
        
        if(t.type == Token_EndOfFile || (t.type == Token_Identifier && TokenEquals(t, tokenName)))
        {
            parsing = false;
        }
    }
}

inline bool NextTokenIs(Tokenizer* tokenizer, ForgTokenType type)
{
    Tokenizer testTokenizer = *tokenizer;
    Token test = GetToken(&testTokenizer);
    
    bool result = (test.type == type);
    
    return result;
}

inline bool RequireToken(Tokenizer* tokenizer, int tokenType)
{
    Token token = GetToken( tokenizer );
    bool result = token.type == tokenType;
    return result;
}

inline Token Tokenize(char* string)
{
    Token result = {};
    result.text = string;
    result.textLength = StrLen(string);
    
    return result;
}

inline Token AdvanceToToken(Tokenizer* tokenizer, char* tokenValue)
{
    Token result = GetToken(tokenizer);
    while(result.type != Token_EndOfFile)
    {
        result = Stringize(result);
        if(TokenEquals(result, tokenValue))
        {
            break;
        }
        
        result = GetToken(tokenizer);
    }
    
    return result;
}