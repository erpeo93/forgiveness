#pragma once
inline u64 StringHash( char* string, u32 length = U32_MAX )
{
    u64 result = 0;
    if(string)
    {
        for( char* scan = string; *scan; ++scan )
        {
            if( length-- == 0 )
            {
                break;
            }
            result = 65599 * result + *scan;
        }
    }
    return result;
}

#include "meow_intrinsics.h"
#include "meow_hash.h"
inline u64 DataHash(char* buffer, u64 length)
{
    meow_hash hash = MeowHash_Accelerated(0, length, buffer);
    u64 result = MeowU64From(hash, 0);
    
    return result;
}

inline u32 StrLen( char* string )
{
    u32 result = 0;
    if( string )
    {
        while( *string++ )
        {
            result++;
        }
    }
    return result;
}

inline b32 StrEqual( char* s1, char* s2 )
{
    b32 result = (s1 == s2);
    
    if(s1 && s2)
    {
        while(*s1 && *s2 && (*s1 == *s2))
        {
            ++s1;
            ++s2;
        }
        
        result = ((*s1 == 0) && (*s2 == 0));
    }
    
    return result;
}

inline b32 StrEqual( char* s1, memory_index countS1, char* s2, memory_index countS2)
{
    b32 result = ( countS1 == countS2 );
    
    if( result )
    {
        for( u32 charIndex = 0; charIndex < countS1; ++charIndex )
        {
            if( s1[charIndex] != s2[charIndex] )
            {
                result = false;
                break;
            }
        }
    }
    return result;
}

inline b32 StrEqual(unm length, char* s1, char* s2, b32 notValidIfNotNull = false)
{
    b32 result = false;
    if(s2)
    {
        char* at = s2;
        for(int index = 0; index < length; index++, at++)
        {
            if( !(*at) || (*at != s1[index]))
            {
                return false;
            }
        }
        
        result = true;
        if(notValidIfNotNull)
        {
            result = (*at == 0);
        }
    }
    else
    {
        result = (length == 0);
    }
    
    return result;
}

inline void StrCpy( char* s1, i32 countS1, char* s2, i32 countS2, char* dest, i32 destCount )
{
    Assert( countS1 + countS2 < destCount );
    i32 count = 0;
    while( count++ < countS1 )
    {
        *dest++ = *s1++;
    }
    
    count = 0;
    while( count++ < countS2 )
    {
        *dest++ = *s2++;
    }
    *dest = 0;
}

inline void StrCpy(char* s1, i32 countS1, char* dest, i32 destCount = 10000 )
{
    Assert( countS1 < destCount );
    i32 count = 0;
    while( count++ < countS1 )
    {
        *dest++ = *s1++;
    }
    *dest = 0;
}

inline b32 ContainsSubString(char* reference, char* substring)
{
    b32 result = false;
    
    u32 substringLength = StrLen(substring);
    for(char* test = reference; *test; ++test)
    {
        if(StrEqual(substringLength, test, substring))
        {
            result = true;
            break;
        }
    }
    
    return result;
}

inline void RemoveExtension(char* dest, u32 destLength, char* source)
{
    Assert(StrLen(source) < destLength);
    char* toCopy = source;
    while(*toCopy )
    {
        if(*toCopy == '.' )
        {
            break;
        }
        
        *dest++ = *toCopy++;
    }
    *dest++ = 0;
}

internal i32 I32FromCharInternal( char** string )
{
    i32 result = 0;
    char* str = *string;
    while( ( *str >= '0' ) && ( *str <= '9' ) )
    {
        result *= 10;
        result += ( *str - '0' );
        ++str;
    }
    
    *string = str;
    return result;
}

inline i32 I32FromChar(char* string)
{
    char* ignored = string;
    i32 result = I32FromCharInternal(&ignored);
    return result;
}

inline r32 R32FromChar(char* string)
{
    r32 result;
    sscanf(string, "%f", &result);
    return result;
}

#include <stdarg.h>
struct FormatDest
{
    unm size;
    char* at;
};

inline void OutChar( FormatDest* dest, char value )
{
    if( dest->size )
    {
        --dest->size;
        *dest->at++ = value;
    }
}

inline void OutChars( FormatDest* dest, char* value )
{
    while( *value )
    {
        OutChar( dest, *value++ );
    }
}

#define ReadVarArgInteger( length, argList ) ( length == 8 ? ( ( i64 ) va_arg( argList, i64 ) ) : ( ( i64 ) va_arg( argList, i32 )) )
#define ReadVarArgUnsignedInteger( length, argList ) ( length == 8 ? ( ( u64 ) va_arg( argList, u64 ) ) : ( ( u64 ) va_arg( argList, u32 )) )
#define ReadVarArgFloat( length, argList ) va_arg( argList, r64 )

char decChars[] = "0123456789";
char lowerHexChars[] = "0123456789abcdef";
char upperHexChars[] = "0123456789ABCDEF";

internal void U64ToASCII( FormatDest* dest, u64 value, u32 base, char* digits )
{
    Assert( base != 0 );
    
    char* start = dest->at;
    do
    {
        u64 digitIndex = value % base;
        OutChar( dest, digits[digitIndex] );
        
        value /= base;
    } while( value != 0 );
    
    char* end = dest->at;
    while( start < end )
    {
        --end;
        char temp = *end;
        *end = *start;
        *start = temp;
        ++start;
        
    }
}

internal void R64ToASCII( FormatDest* dest, r64 value, u32 precision )
{
    if( value < 0 )
    {
        value = -value;
        OutChar( dest, '-' );
    }
    
    u64 integerPart = ( u64 ) value;
    value -= ( r64 ) integerPart;
    U64ToASCII( dest, integerPart, 10, decChars );
    
    OutChar( dest, '.' );
    for( u32 precisionIndex = 0; precisionIndex < precision; ++precisionIndex )
    {
        value *= 10.0f;
        u32 integer = ( u32 ) value;
        value -= ( r32 ) integer;
        OutChar( dest, decChars[integer] );
    }
}

internal unm FormatStringList( char* destInit, unm destSize, char* format, va_list argList )
{
    FormatDest dest = { destSize, destInit };
    
    if( dest.size )
    {
        char* at = format;
        while( at[0] )
        {
            if( at[0] == '%' )
            {
                ++at;
                
                b32 forceSign = false;
                b32 padWithZeros = false;
                b32 leftJustify = false;
                b32 positiveSignIsBlank = false;
                b32 annotateIfNotZero = false;
                
                b32 parsing = true;
                while( parsing )
                {
                    switch( *at )
                    {
                        case '+': forceSign = true; break;
                        case '0': padWithZeros = true; break;
                        case '-': leftJustify = true; break;
                        case ' ': positiveSignIsBlank = true; break;
                        case '#': annotateIfNotZero = true; break;
                        default: parsing = false; break;
                    }
                    
                    if( parsing )
                    {
                        ++at;
                    }
                }
                
                b32 widthSpecified = false;
                i32 width = 0;
                if( *at == '*' )
                {
                    width = va_arg(argList, int);
                    widthSpecified = true;
                    ++at;
                }
                else if( ( *at >= '0' ) && ( *at <= '9' ) )
                {
                    width = I32FromCharInternal( &at );
                    widthSpecified = true;
                }
                
                b32 precisionSpecified = false;
                i32 precision = 0;
                
                if( *at == '.' )
                {
                    ++at;
                    if( *at == '*' )
                    {
                        precision = va_arg(argList, int);
                        precisionSpecified = true;
                        ++at;
                    }
                    else if( ( *at >= '0' ) && ( *at <= '9' ) )
                    {
                        precision = I32FromCharInternal( &at );
                        precisionSpecified = true;
                    }
                    else
                    {
                        InvalidCodePath;
                    }
                    
                    
                }
                
                if( !precisionSpecified )
                {
                    precision = 6;
                }
                
                u32 integerLen = 4;
                u32 floatLen = 8;
                
                if( at[0] == 'h' && at[1] == 'h' )
                {
                    at += 2;
                }
                else if( at[0] == 'l' && at[1] == 'l' )
                {
                    at += 2;
                }
                else if( at[0] == 'h' )
                {
                    ++at;
                }
                else if( at[0] == 'l' )
                {
                    integerLen = 8;
                    ++at;
                }
                else if( at[0] == 'j' )
                {
                    ++at;
                }
                else if( at[0] == 'z' )
                {
                    ++at;
                }
                else if( at[0] == 't' )
                {
                    ++at;
                }
                else if( at[0] == 'L' )
                {
                    ++at;
                }
                
                char tempBuff_[64];
                char* tempBuff = tempBuff_;
                FormatDest temp = { ArrayCount( tempBuff_ ), tempBuff };
                char* prefix = "";
                b32 isFloat = false;
                
                switch( *at )
                {
                    case 'd': 
                    case 'i':
                    {
                        i64 value = ReadVarArgInteger( integerLen, argList );
                        b32 wasNegative = ( value < 0 );
                        if( wasNegative )
                        {
                            value = -value;
                        }
                        
                        U64ToASCII( &temp, ( u64 ) value, 10, decChars );
                        if( wasNegative )
                        {
                            prefix = "-";
                        }
                        else if( forceSign )
                        {
                            Assert( !positiveSignIsBlank );
                            prefix = "+";
                        }
                        else if( positiveSignIsBlank )
                        {
                            prefix = " ";
                        }
                    } break;
                    
                    case 'u': 
                    {
                        u64 value = ReadVarArgUnsignedInteger( integerLen, argList );
                        U64ToASCII( &temp, ( u64 ) value, 10, decChars );
                    } break;
                    
                    case 'o': 
                    {
                        u64 value = ReadVarArgUnsignedInteger( integerLen, argList );
                        U64ToASCII( &temp, ( u64 ) value, 8, decChars );
                        
                        if( annotateIfNotZero && ( value != 0 ) )
                        {
                            OutChar( &temp, '0' );
                            prefix = "0";
                        }
                    } break;
                    
                    case 'x': 
                    {
                        u64 value = ReadVarArgUnsignedInteger( integerLen, argList );
                        U64ToASCII( &temp, ( u64 ) value, 16, lowerHexChars );
                        
                        if( annotateIfNotZero && ( value != 0 ) )
                        {
                            prefix = "0x";
                            OutChars( &temp, "x0" );
                        }
                    } break;
                    
                    case 'X': 
                    {
                        u64 value = ReadVarArgUnsignedInteger( integerLen, argList );
                        U64ToASCII( &temp, ( u64 ) value, 16, upperHexChars );
                        
                        if( annotateIfNotZero && ( value != 0 ) )
                        {
                            prefix = "0X";
                        }
                    } break;
                    
                    case 'f': 
                    case 'F': 
                    case 'e': 
                    case 'E': 
                    case 'g':
                    case 'G': 
                    case 'a': 
                    case 'A': 
                    {
                        r64 value = ReadVarArgFloat( floatLen, argList );
                        R64ToASCII( &temp, value, precision );
                        precision += width;
                        isFloat = true;
                    } break;
                    
                    case 'c':
                    {
                        int value = va_arg( argList, int );
                        OutChar( &temp, ( char ) value );
                        
                    } break;
                    
                    case 's':
                    {
                        char* string = va_arg( argList, char* );
                        
                        tempBuff = string;
                        if( isFloat || precisionSpecified)
                        {
                            temp.size = 0;
                            for( char* scan = string; *scan && ( temp.size < precision ); ++scan )
                            {
                                ++temp.size;
                            }
                        }
                        else
                        {
                            temp.size = StrLen(string);
                        }
                        temp.at = string + temp.size;
                    } break;
                    
                    case 'p':
                    {
                        void* value = va_arg( argList, void* );
                        U64ToASCII( &temp, *( unm* ) &value, 16, lowerHexChars );
                    } break;
                    
                    case '%':
                    {
                        OutChar( &dest, '%' );
                    } break;
                    
                    InvalidDefaultCase;
                }
                
                if( temp.at - tempBuff )
                {
                    u32 usePrecision = precision;
                    if( !precisionSpecified )
                    {
                        usePrecision = ( i32 ) ( temp.at - tempBuff );
                    }
                    
                    u32 prefixLen = StrLen( prefix );
                    u32 useWidth = width;
                    
                    u32 computedWidth = usePrecision + prefixLen;
                    if( useWidth < computedWidth )
                    {
                        useWidth = computedWidth;
                    }
                    
                    if( padWithZeros )
                    {
                        Assert( !leftJustify );
                        leftJustify = false;
                    }
                    
                    if( !leftJustify )
                    {
                        while( useWidth > ( usePrecision + prefixLen ) )
                        {
                            OutChar( &dest, padWithZeros ? '0' : ' ' );
                            --useWidth;
                        }
                    }
                    
                    for( char* pre = prefix; *pre && useWidth; ++pre )
                    {
                        OutChar( &dest, *pre );
                        --useWidth;
                    }
                    
                    if( usePrecision > useWidth )
                    {
                        usePrecision = useWidth;
                    }
                    
                    while( usePrecision > ( temp.at - tempBuff ) )
                    {
                        OutChar( &dest, '0' );
                        --usePrecision;
                    }
                    
                    while( usePrecision && ( temp.at != tempBuff ) )
                    {
                        OutChar( &dest, *tempBuff );
                        
                        if( *tempBuff++ != '.' )
                        {
                            --usePrecision;
                        }
                        
                        --useWidth;
                    }
                    
                    if( leftJustify )
                    {
                        while( useWidth )
                        {
                            OutChar( &dest, ' ' );
                            --useWidth;
                        }
                    }
                }
                
                if( *at )
                {
                    ++at;
                }
                
                
            }
            else
            {
                OutChar( &dest, *at++ );
            }
        }
        
        if( dest.size )
        {
            dest.at[0] = 0;
        }
        else
        {
            dest.at[-1] = 0;
        }
    }
    
    unm result = dest.at - destInit;
    return result;
}

internal unm FormatString( char* dest, unm destSize, char* format, ... )
{
    va_list argList;
    va_start( argList, format );
    unm result = FormatStringList( dest, destSize, format, argList );
    va_end( argList );
    
    return result;
}

inline void AppendString(char* original, u32 size, char* toAppend, u32 toAppendCount)
{
    u32 currentLength = StrLen(original);
    u32 toCopy = Min(toAppendCount, size - (currentLength + 1));
    
    char* copyHere = original + currentLength;
    
    for(u32 toAppendIndex = 0; toAppendIndex < toCopy; ++toAppendIndex)
    {
        *copyHere++ = *toAppend++;
    }
    *copyHere = 0;
}
