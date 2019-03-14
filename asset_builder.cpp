#include <stdio.h>
#include <stdlib.h>
#include "forg_basic_types.h"
#include "net.h"
#include "forg_platform.h"
#include "forg_shared.h"
#include "forg_token.h"
#include "forg_asset_enum.h"
#include "forg_file_formats.h"
#include "forg_intrinsics.h"
#include "forg_math.h"
#include "forg_meta.h"
#include "client_generated.h"
#include "asset_builder.h"
#include "win32_file.cpp"
#include "forg_token.cpp"
#include "miniz.c"


#define MAX_IMAGE_DIM 256

#define STB_IMAGE_IMPLEMENTATION 1
#include "stb_image.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION 1
#include "stb_resize_image.h"

#define PLAYER_VIEW_WIDTH_IN_WORLD_METERS 6.0f

#define USE_FONTS_FROM_WINDOWS 1
#if USE_FONTS_FROM_WINDOWS 
#include <Windows.h>
#else
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#endif

#define VERY_LARGE_NUMBER 256
#define EVEN_BIGGER_NUMBER 1024
struct Assets
{
    u32 countTags;
    PakTag tags[EVEN_BIGGER_NUMBER];
    
    u32 countAssetType;
    PakAssetType types[Asset_count + HASHED_ASSET_SLOTS];
    
    u32 countAssets;
    AssetSource assetSources[VERY_LARGE_NUMBER];
    PakAsset assets[VERY_LARGE_NUMBER];
    
    PakAssetType* DEBUGAssetType;
    
    u32 assetIndex;
    u64 currentStringHashID;
};
global_variable Assets* currentAssets_;

#pragma pack( push, 1 )
struct BitmapHeader
{
    u16 FileType;
    u32 FileSize;
    u16 Reserved1;
    u16 Reserved2;
    u32 BitmapOffset;
    u32 Size;
    i32 Width;
    i32 Height;
    u16 Planes;
    u16 BitsPerPixel;
    u32 Compression;
    u32 SizeOfBitmap;
    i32  HorzResolution;
    i32  VertResolution;
    i32 ColorsUsed;    
    i32 ColorsImportant;
    u32 RedMask;
    u32 GreenMask;
    u32 BlueMask;
};

struct WAVEChunk
{
    u32 id;
    u32 size;
};

struct WAVEHeader
{
    u32 id;
    u32 size;
    u32 waveID;
};

struct WAVEFormat
{
    u16 format;
    u16 channels;
    u32 blocksPerSec;
    u32 dataRate;
    u16 blockSize;
    u16 bitsPerSample;
    u16 extensionSize;
    u16 validBits;
    u32 speakerMask;
    u8 guid[16];
};

#define RIFF_CODE( a, b, c, d ) ( ( u32 ) ( ( a ) << 0 ) | ( u32 ) ( ( b ) << 8 ) | ( u32 ) ( ( c ) << 16 ) | ( u32 ) ( ( d ) << 24 ) ) 

enum WAVE_ChunkID
{
    WAVE_IDriff = RIFF_CODE( 'R', 'I', 'F', 'F' ),
    WAVE_IDfmt = RIFF_CODE( 'f', 'm', 't', ' ' ),
    WAVE_IDwave = RIFF_CODE( 'W', 'A', 'V', 'E' ),
    WAVE_IDdata = RIFF_CODE( 'd', 'a', 't', 'a' ),
};

#pragma pack( pop )


struct EntireFile
{
    u32 size;
    void* content;
};

EntireFile ReadFile( char* filename )
{
    EntireFile result = {};
    
    FILE* in = fopen( filename, "rb" );
    
    if( in )
    {
        fseek( in, 0, SEEK_END );
        result.size = ftell( in );
        fseek( in, 0, SEEK_SET );
        result.content = malloc( result.size );
        fread( result.content, result.size, 1, in );
        fclose( in );
    }
    return result;
};

global_variable HDC globalFontDC;
global_variable VOID* Bits;

#define FONT_MAX_WIDTH 1024
#define FONT_MAX_HEIGHT 1024

#define MAX_FONT_GLYPHS 0x10ffff

internal LoadedFont LoadFont( char* filename, char* fontName, int height )
{
    AddFontResourceExA(filename, FR_PRIVATE, 0);
    LoadedFont font = {};
    font.win32Font = CreateFontA(height, 0, 0, 0,
                                 FW_NORMAL, // NOTE(casey): Weight
                                 FALSE, // NOTE(casey): Italic
                                 FALSE, // NOTE(casey): Underline
                                 FALSE, // NOTE(casey): Strikeout
                                 DEFAULT_CHARSET, 
                                 OUT_DEFAULT_PRECIS,
                                 CLIP_DEFAULT_PRECIS, 
                                 ANTIALIASED_QUALITY,
                                 DEFAULT_PITCH|FF_DONTCARE,
                                 fontName);
    
    if(!globalFontDC)
    {
        globalFontDC = CreateCompatibleDC(GetDC(0));
        
        BITMAPINFO Info = {};
        Info.bmiHeader.biSize = sizeof(Info.bmiHeader);
        Info.bmiHeader.biWidth = FONT_MAX_WIDTH;
        Info.bmiHeader.biHeight = FONT_MAX_HEIGHT;
        Info.bmiHeader.biPlanes = 1;
        Info.bmiHeader.biBitCount = 32;
        Info.bmiHeader.biCompression = BI_RGB;
        Info.bmiHeader.biSizeImage = 0;
        Info.bmiHeader.biXPelsPerMeter = 0;
        Info.bmiHeader.biYPelsPerMeter = 0;
        Info.bmiHeader.biClrUsed = 0;
        Info.bmiHeader.biClrImportant = 0;
        HBITMAP Bitmap = CreateDIBSection(globalFontDC, &Info, DIB_RGB_COLORS, &Bits, 0, 0);
        SelectObject(globalFontDC, Bitmap);
        SetBkColor(globalFontDC, RGB(0, 0, 0));
    }
    
    SelectObject(globalFontDC, font.win32Font );
    GetTextMetrics(globalFontDC, &font.metrics);
    
    font.ascenderHeight = ( r32 ) font.metrics.tmAscent;
    font.descenderHeight = ( r32 ) font.metrics.tmDescent;
    font.externalLeading = ( r32 ) font.metrics.tmExternalLeading;
    
    font.maximumGlyphsCount = 5000;
    font.minCodePoint = INT_MAX;
    font.maxCodePoint = 0;
    font.onePastHighestCodePoint = 0;
    
    u32 glyphsTableSize = sizeof( u32 ) * font.maximumGlyphsCount;
    font.glyphIndexForCodePoint = ( u32* ) malloc( glyphsTableSize );
    memset( font.glyphIndexForCodePoint, 0, glyphsTableSize );
    
    font.glyphs = ( PakGlyph* ) malloc( sizeof( PakGlyph ) * font.maximumGlyphsCount );
    
    
    u32 kerningTableSize = font.maximumGlyphsCount * font.maximumGlyphsCount * sizeof( r32 );
    font.horizontalAdvancement = ( r32* ) malloc( kerningTableSize );
    memset( font.horizontalAdvancement, 0, kerningTableSize );
    
    font.glyphsCount = 1;
    font.glyphs[0].unicodeCodePoint = 0;
    font.glyphs[0].bitmapId.value = 0;
    return font;
}

internal void FinalizeFontKernings( LoadedFont* font )
{
    SelectObject(globalFontDC, font->win32Font );
    
    DWORD kerningPairsCount = GetKerningPairsW( globalFontDC, 0, 0 );
    KERNINGPAIR* pairs = ( KERNINGPAIR* ) malloc( sizeof( KERNINGPAIR ) * kerningPairsCount );
    GetKerningPairsW( globalFontDC, kerningPairsCount, pairs );
    
    for( DWORD pairIndex = 0; pairIndex < kerningPairsCount; pairIndex++ )
    {
        KERNINGPAIR* pair = pairs + pairIndex;
        if( pair->wFirst < font->maximumGlyphsCount &&
           pair->wSecond < font->maximumGlyphsCount )
        {
            u32 first = font->glyphIndexForCodePoint[pair->wFirst];
            u32 second = font->glyphIndexForCodePoint[pair->wSecond];
            
            if( first && second )
            {
                font->horizontalAdvancement[first * font->maximumGlyphsCount + second] += ( r32 ) pair->iKernAmount;
            }
        }
    }
    
    free( pairs );
}

internal void FreeFont( LoadedFont* font )
{
    if( font )
    {
        free( font->glyphs );
        free( font->horizontalAdvancement );
        free( font->glyphIndexForCodePoint );
    }
}

internal LoadedBitmap LoadGlyph( LoadedFont* font, u32 codePoint, PakAsset* asset )
{
    LoadedBitmap result = {};
    
    u32 glyphIndex = font->glyphIndexForCodePoint[codePoint];
#if USE_FONTS_FROM_WINDOWS
    
    int MaxWidth = FONT_MAX_WIDTH;
    int MaxHeight = FONT_MAX_HEIGHT;
    
    SelectObject(globalFontDC, font->win32Font );
    memset(Bits, 0x00, MaxWidth*MaxHeight*sizeof(u32));
    wchar_t CheesePoint = (wchar_t)codePoint;
    
    SIZE Size;
    GetTextExtentPoint32W(globalFontDC, &CheesePoint, 1, &Size);
    
    int preStepX = 128;
    
    int BoundWidth = Size.cx + preStepX;
    if(BoundWidth > MaxWidth)
    {
        BoundWidth = MaxWidth;
    }
    int BoundHeight = Size.cy;
    if(BoundHeight > MaxHeight)
    {
        BoundHeight = MaxHeight;
    }
    
    //    PatBlt(globalFontDC, 0, 0, Width, Height, BLACKNESS);
    //    SetBkMode(globalFontDC, TRANSPARENT);
    SetTextColor(globalFontDC, RGB(255, 255, 255));
    TextOutW(globalFontDC, preStepX, 0, &CheesePoint, 1);
    
    i32 MinX = 10000;
    i32 MinY = 10000;
    i32 MaxX = -10000;
    i32 MaxY = -10000;
    
    u32 *Row = (u32 *)Bits + (MaxHeight - 1)*MaxWidth;
    for(i32 Y = 0;
        Y < BoundHeight;
        ++Y)
    {
        u32 *Pixel = Row;
        for(i32 X = 0;
            X < BoundWidth;
            ++X)
        {
#if 0
            COLORREF RefPixel = GetPixel(globalFontDC, X, Y);
            Assert(RefPixel == *Pixel);
#endif
            if(*Pixel != 0)
            {
                if(MinX > X)
                {
                    MinX = X;                    
                }
                
                if(MinY > Y)
                {
                    MinY = Y;                    
                }
                
                if(MaxX < X)
                {
                    MaxX = X;                    
                }
                
                if(MaxY < Y)
                {
                    MaxY = Y;                    
                }
            }
            
            ++Pixel;
        }
        Row -= MaxWidth;
    }
    
    r32 kerningChange = 0.0f;
    if(MinX <= MaxX)
    {
        int Width = (MaxX - MinX) + 1;
        int Height = (MaxY - MinY) + 1;
        
        result.width = Width + 2;
        result.height = Height + 2;
        result.pixels = malloc(result.height*result.width * 4);
        result.free = result.pixels;
        
        memset(result.pixels, 0, result.height*result.width*4);
        
        u32 *DestRow = (u32 *)result.pixels + (result.height - 1 - 1)*result.width;
        u32 *SourceRow = (u32 *)Bits + (MaxHeight - 1 - MinY)*MaxWidth;
        for(i32 Y = MinY;
            Y <= MaxY;
            ++Y)
        {
            u32 *Source = (u32 *)SourceRow + MinX;
            u32 *Dest = (u32 *)DestRow + 1;
            for(i32 X = MinX;
                X <= MaxX;
                ++X)
            {
#if 0
                COLORREF Pixel = GetPixel(globalFontDC, X, Y);
                Assert(Pixel == *Source);
#else
                u32 Pixel = *Source;
#endif
                r32 Gray = (r32)(Pixel & 0xFF); 
                Vec4 Texel = {255.0f, 255.0f, 255.0f, Gray};
                Texel = SRGB255ToLinear1(Texel);
                Texel.rgb *= Texel.a;
                Texel = Linear1ToSRGB255(Texel);
                
                *Dest++ = (((u32)(Texel.a + 0.5f) << 24) |
                           ((u32)(Texel.r + 0.5f) << 16) |
                           ((u32)(Texel.g + 0.5f) << 8) |
                           ((u32)(Texel.b + 0.5f) << 0));
                
                
                ++Source;
            }
            
            DestRow -= result.width;
            SourceRow -= MaxWidth;
        }
        
        asset->bitmap.align[0] = 1.0f / (r32)result.width;
        asset->bitmap.align[1] = (1.0f + (MaxY - (BoundHeight - font->metrics.tmDescent))) / (r32)result.height;
        
        kerningChange = ( r32 ) ( MinX - preStepX );
    }
    
    ABC abc;
    GetCharABCWidthsW( globalFontDC, codePoint, codePoint, &abc );
    r32 charAdvance = ( r32 ) ( abc.abcA + abc.abcB + abc.abcC );
    
    for( u32 otherGlyphIndex = 0;
        otherGlyphIndex < font->maximumGlyphsCount;
        otherGlyphIndex++ )
    {
        font->horizontalAdvancement[glyphIndex * font->maximumGlyphsCount + otherGlyphIndex] += charAdvance - kerningChange;
        
        if(otherGlyphIndex != 0 )
        {
            font->horizontalAdvancement[otherGlyphIndex * font->maximumGlyphsCount + glyphIndex] += kerningChange;
        }
    }
    
#else
    EntireFile fontFile = ReadFile( filename );
    
    stbtt_fontinfo font;
    stbtt_InitFont( &font, ( u8* ) fontFile.content, stbtt_GetFontOffsetForIndex( ( u8* ) fontFile.content, 0 ) );
    
    
    int width, height, XOffset, YOffset;
    u8* bitmap = stbtt_GetCodepointBitmap( &font, 0, stbtt_ScaleForPixelHeight( &font, 40.0f ), 
                                          codePoint, &width, &height, &XOffset, &YOffset );
    
    
    result.bitmap.pixels = malloc( sizeof( u32 ) * width * height );
    result.free = result.bitmap.pixels;
    result.bitmap.width = ( i16 ) width;
    result.bitmap.height = ( i16 ) height;
    
    Assert(width < 128 && height < 128);
    result.bitmap.widthOverHeight = (r32 ) result.bitmap.width / (r32 ) result.bitmap.height;
    result.bitmap.downsampleFactor = 1;
    
    
    u32* dest = ( u32* ) result.bitmap.pixels + ( result.bitmap.width * ( result.bitmap.height - 1 ) );
    u8* source = bitmap;
    
    for( i32 Y = 0; Y < height; Y++ )
    {
        u32* destRow = dest;
        
        for( i32 X = 0; X < width; X++ )
        {
            u8 alpha = *source++;
            *destRow++ = ( ( alpha << 24 ) |
                          ( alpha <<  16 ) |
                          ( alpha <<   8 ) |
                          ( alpha <<   0 ) );
        }
        
        dest -= width;
    }
    
    stbtt_FreeBitmap( bitmap, 0 );
#endif
    
    return result;
}

internal LoadedBitmap LoadBitmap( char* path, char* filename, b32 loadPixels )
{
    LoadedBitmap result = {};
    char completeName[256];
    sprintf( completeName, "%s/%s", path, filename );
    
#if STB_IMAGE_IMPLEMENTATION
    int x;
    int y;
    int n;
    stbi_set_flip_vertically_on_load(true);
    unsigned char *data = stbi_load(completeName, &x, &y, &n, 0);
    Assert( n == 4 );
    if( data )
    {
        u32* source = ( u32* ) data;
        for( i32 Y = 0; Y < y; ++Y )
        {
            for( i32 X = 0; X < x; X++ )
            {
                u32* sourcePixel = source + ( Y * x ) + X;
                Vec4 texel = V4( ( r32 ) ( *sourcePixel >> 0 &0xff ),
                                ( r32 ) ( *sourcePixel >> 8 & 0xff ),
                                ( r32 ) ( *sourcePixel >> 16 & 0xff ),
                                ( r32 ) ( *sourcePixel >> 24 & 0xff ) );
                
                texel = SRGB255ToLinear1( texel );
                texel.rgb *= texel.a;
                texel = Linear1ToSRGB255( texel );
                
                *sourcePixel = ( ( u32 ) texel.a << 24 | 
                                ( u32 ) texel.r << 16 |
                                ( u32 ) texel.g << 8 |
                                ( u32 ) texel.b << 0 );
            }
        }
        
        r32 xDownsampleFactor = (r32) x / MAX_IMAGE_DIM;
        r32 yDownsampleFactor = (r32) y / MAX_IMAGE_DIM;
        r32 downsampleFactor = Max(xDownsampleFactor, yDownsampleFactor);
        downsampleFactor = Max(downsampleFactor, 1.0f);
        
        int xFinal = RoundReal32ToI32((r32) x / downsampleFactor);
        int yFinal = RoundReal32ToI32((r32) y / downsampleFactor);
        
        Assert(xFinal <= MAX_IMAGE_DIM);
        Assert(yFinal <= MAX_IMAGE_DIM);
        
        result.pixels = ( u8* ) malloc( xFinal * yFinal * n );
        stbir_resize_uint8(data, x, y, 0, (unsigned char* )result.pixels, xFinal, yFinal, 0, 4);
        
        stbi_image_free( data );
        result.width = SafeTruncateToU16(xFinal);
        result.height = SafeTruncateToU16(yFinal);
        result.widthOverHeight = (r32 ) result.width / (r32 ) result.height;
        result.downsampleFactor = downsampleFactor;
        result.free = result.pixels;
    }
    else
    {
        InvalidCodePath;
    }
#else
    InvalidCodePath;
    EntireFile bitmap = ReadFile( filename );
    Assert( bitmap.content );
    if( bitmap.content )
    {
        result.free = bitmap.content;
        BitmapHeader* header = ( BitmapHeader* ) bitmap.content;
        result.pixels = ( ( u8* ) bitmap.content ) + header->BitmapOffset;
        result.width = SafeTruncateToU16( header->Width );
        result.height = SafeTruncateToU16( header->Height );
        result.widthOverHeight = (r32 ) result.width / (r32 ) result.height;
        result.downsampleFactor = 1.0f;
        
        if( loadPixels )
        {
            u32 redMask = header->RedMask;
            u32 greenMask = header->GreenMask;
            u32 blueMask = header->BlueMask;
            u32 alphaMask = ~( redMask | greenMask | blueMask );
            
            u32 redShift = CountLastSignificantBitSet( redMask );
            u32 greenShift = CountLastSignificantBitSet( greenMask );
            u32 blueShift = CountLastSignificantBitSet( blueMask );
            u32 alphaShift = CountLastSignificantBitSet( alphaMask );
            
            u32* pixels = ( u32* ) result.pixels;
            //AA RR GG BB
            for( i32 Y = 0; Y < result.height; Y++ )
            {
                for( i32 X = 0; X < result.width; X++ )
                {
                    Vec4 texel = V4( ( r32 ) ( *pixels >> redShift &0xff ),
                                    ( r32 ) ( *pixels >> greenShift & 0xff ),
                                    ( r32 ) ( *pixels >> blueShift & 0xff ),
                                    ( r32 ) ( *pixels >> alphaShift & 0xff ) );
                    
                    texel = SRGB255ToLinear1( texel );
                    
                    texel.rgb *= texel.a;
                    texel = Linear1ToSRGB255( texel );
                    
                    *pixels = ( ( u32 ) texel.a << 24 | 
                               ( u32 ) texel.r << 16 |
                               ( u32 ) texel.g << 8 |
                               ( u32 ) texel.b << 0 );
                    pixels++;
                }
            }
        }
    }
#endif
    return result;
}

struct RiffIter
{
    u8* at;
    u8* end;
};

inline RiffIter ParseChunkAt( void* source, void* stop )
{
    RiffIter result;
    result.at = ( u8* ) source;
    
    WAVEChunk* chunk = ( WAVEChunk* ) result.at;
    
    result.end = ( u8* ) stop;
    return result;
};

inline b32 IsValid( RiffIter iter )
{
    b32 result = ( iter.at < iter.end );
    return result;
}

inline RiffIter NextChunk( RiffIter iter )
{
    WAVEChunk* chunk = ( WAVEChunk* ) iter.at;
    iter.at += sizeof( WAVEChunk ) + chunk->size;
    
    return iter;
}

inline u32 GetType( RiffIter iter )
{
    WAVEChunk* chunk = ( WAVEChunk* ) iter.at;
    u32 result = chunk->id;
    
    return result;
}

inline u8* GetChunkData( RiffIter iter )
{
    u8* result = iter.at + sizeof( WAVEChunk );
    return result;
}

inline u32 GetChunkDataSize( RiffIter iter )
{
    WAVEChunk* chunk = ( WAVEChunk* ) iter.at;
    u32 result = chunk->size;
    
    return result;
}

internal LoadedSound LoadWAV(char* filename, u32 firstSampleIndex, u32 sectionSampleCount)
{
    LoadedSound result = {};
    
    EntireFile sound = ReadFile( filename );
    Assert( sound.content );
    if( sound.content )
    {
        result.free = sound.content;
        WAVEHeader* header = ( WAVEHeader* ) sound.content;
        Assert( header->id == WAVE_IDriff );
        Assert( header->waveID == WAVE_IDwave );
        
        void* samplesLeft = 0;
        void* samplesRight = 0;
        
        for( RiffIter iter = ParseChunkAt( header + 1, ( u8* ) ( header + 1 ) + header->size - 4 ); IsValid( iter ); iter = NextChunk( iter ) )
        {
            switch( GetType( iter ) )
            {
                case WAVE_IDfmt:
                {
                    WAVEFormat* format = ( WAVEFormat* ) GetChunkData( iter ); 
                    
                    Assert(format->format == 1 );
                    Assert(format->blocksPerSec = 48000);
                    Assert(format->bitsPerSample == 16 );
                    Assert(format->blockSize == format->channels * 2);
                    
                    result.countChannels = format->channels;
                } break;
                
                case WAVE_IDdata:
                {
                    u32 sampleCount = GetChunkDataSize( iter ) /( result.countChannels * sizeof( i16 ) );
                    
                    if(result.countChannels == 1)
                    {
                        result.samples[0] = (i16*) GetChunkData(iter);
                    }
                    else
                    {
                        // TODO( Leonardo ): stereo!
                        Assert(result.countChannels == 2);
                        
                        i16* leftChannel = ( i16*) GetChunkData(iter);
                        
                        for( u32 sampleIndex = 0; sampleIndex < sampleCount / 2; sampleIndex++ )
                        {
                            leftChannel[sampleIndex] = leftChannel[2 * sampleIndex];
                        }
                        
                        result.samples[0] = leftChannel;
                        result.countChannels = 1;
                        
                        b32 atEnd = true;
                        if(sectionSampleCount)
                        {
                            Assert( firstSampleIndex + sectionSampleCount <= sampleCount );
                            atEnd = ( firstSampleIndex + sectionSampleCount == sampleCount );
                            
                            for(u32 channelIndex = 0; channelIndex < result.countChannels; channelIndex++)
                            {
                                result.samples[channelIndex] += firstSampleIndex;
                            }
                            sampleCount= sectionSampleCount;
                        }
                        else
                        {
                            
                        }
                        
                        if( atEnd )
                        {
                            for( u32 channelIndex = 0; channelIndex < result.countChannels; channelIndex++ )
                            {
                                for( u32 sampleIndex = sampleCount; sampleIndex < sampleCount + 8; sampleIndex++ )
                                {
                                    result.samples[channelIndex][sampleIndex] = 0;
                                }
                            }
                            
                        }
                        result.countSamples = sampleCount;
                        
                        
                    }
                } break;
            }
        }
    }
    return result;
}


struct XMLParam
{
    char name[64];
    char value[64];
};

struct XMLTag
{
    char title[64];
    
    i32 paramCount;
    XMLParam params[16];
};

internal char* ReadXMLParam( XMLParam* dest, char* source )
{
    if( *source == ' ' )
    {
        source++;
    }
    
    char* testChar = source;
    char* result = source;
    i32 countChar = 0;
    while( *testChar++ != '=' )
    {
        countChar++;
    }
    StrCpy( source, countChar, dest->name, 
           ArrayCount( dest->name ) );
    source = ++testChar;
    countChar = 0;
    while( *testChar++ != '\"' )
    {
        countChar++;
    }
    StrCpy( source, countChar, dest->value, 
           ArrayCount( dest->value ) );
    result = testChar;
    return result;
}

internal char* ReadXMLTag( char* source, XMLTag* dest )
{
    while( *source != '<' )
    {
        if( StrEqual( source, 6, "spriter", 6 ) )
        {
            return 0;
        }
        if( source++ == 0 )
        {
            return source;
        }
    }
    
    if( *( ++source ) == '/' )
    {
        return source;
    }
    
    i32 countChar = 0;
    char* testChar = source;
    while( *testChar != ' ' && *testChar != '>' )
    {
        testChar++;
        countChar++;
    }
    
    StrCpy( source, countChar, dest->title, ArrayCount( dest->title ) );
    source = testChar;
    
    while( *source != '>' && *source != '/' )
    {
        XMLParam* currentParam = dest->params + dest->paramCount++;
        source = ReadXMLParam( currentParam, source );
        Assert( dest->paramCount < ArrayCount( dest->params ) );
    }
    
    char* result = source;
    return result;
}

internal char* GetXMLValue( XMLTag* tag, char* value )
{
    char* result = 0;
    for( i32 pIndex = 0; pIndex < tag->paramCount; pIndex++ )
    {
        XMLParam* param = tag->params + pIndex;
        if( StrEqual( param->name, value ) )
        {
            result = param->value;
            break;
        }
    }
    return result;
}

inline b32 GetXMLValuei( XMLTag* tag, char* name, i32* i )
{
    b32 result = false;
    char* value = GetXMLValue( tag, name );
    if( value )
    {
        result = true;
        sscanf( value, "%d", i );
    }
    return result;
}

inline b32 GetXMLValuef( XMLTag* tag, char* name, r32* f )
{
    b32 result = false;
    char* value = GetXMLValue( tag, name );
    if( value )
    {
        result = true;
        sscanf( value, "%f", f );
    }
    return result;
}

inline char* GetXMLValues( XMLTag* tag, char* name )
{
    char* result = GetXMLValue( tag, name );
    return result;
}


struct PieceInfo
{
    u32 assetType;
    r32 tags[Tag_count];
    r32 nativeHeight;
};

struct TempBone
{
    i32 parentID;
    i32 timelineIndex;
};

struct TempAss
{
    i32 parentBoneID;
    i32 timelineIndex;
    i32 zIndex;
};

struct TempFrame
{
    i32 countBones;
    TempBone tempBones[64];
    Bone bones[64];
    
    i32 countAss;
    TempAss tempAss[64];
    PieceAss ass[64];
    
    i32 timeLine;
};

char* AdvanceToLastSlash( char* name )
{
    Assert( name );
    char* result = name;
    char* test = name;
    while( *test )
    {
        if( *test == '/' )
        {
            result = ( test + 1 );
        }
        ++test;
    }
    
    return result;
}


internal LoadedAnimation LoadAnimation(char* path, char* filename, u32 animationIndex)
{
    u32 currentIndex = 0;
    LoadedAnimation result = {};
    
    u32 frameCountResult = 64;
    TempFrame* frames = (TempFrame* ) calloc(frameCountResult, sizeof(TempFrame ) );
    
    u32 boneCount = 1024;
    u32 assCount = 1024;
    result.bones = (Bone* ) calloc(boneCount, sizeof(Bone ) );
    result.ass = (PieceAss* ) calloc(assCount, sizeof(PieceAss ) );
    
    char filePath[256] = {};
    char* onePastLastSlash = filename;
    char* holder = filename;
    while(*holder )
    {
        if(*holder++ == '/' )
        {
            onePastLastSlash = holder;
        }
    }
    i32 countPath = StrLen(filename ) - StrLen(onePastLastSlash );
    StrCpy(filename, countPath, filePath, ArrayCount(filePath ) );
    
    char toRead[256];
    sprintf(toRead, "%s/%s", path, filename );
    EntireFile animation = ReadFile(toRead );
    
    SpriteInfo tempSprites[64];
    i32 currentFolderID = 0;
    u32 folderSpriteCount[8] = {};
    u32* currentfolderSpriteCounter = 0;
    u32 tempSpriteCount = 0;
    u32 definitiveSpriteCount = 0;
    
    
    Assert(animation.content );
    if(animation.content )
    {
        result.free = animation.content;
        
        // TODO(Leonardo ): robustness!
        i32 startOffset = 134;
        char* start = (char* ) animation.content + startOffset;
        
        b32 mainLineActive = false;
        b32 loading = false;
        b32 ended = false;
        b32 keyiedOnFirst = false;
        i32 timeLineActive = 0;
        char* activeTimelineName = 0;
        
        i32 frameCount = 0;
        TempFrame* tempFrame = 0;
        i32 realFrameIndex = 0;
        i32 currentSpin = 0;
        
        while(start != 0 && !ended )
        {
            XMLTag currentTag = {};
            start = ReadXMLTag(start, &currentTag );
            
            if(!loading )
            {
                if(StrEqual(currentTag.title, "folder" ) )
                {
                    i32 id = 0;
                    GetXMLValuei(&currentTag, "id", &id );
                    Assert(id == currentFolderID );
                    Assert(currentFolderID < ArrayCount(folderSpriteCount ) );
                    currentfolderSpriteCounter = folderSpriteCount + currentFolderID++;
                }
                else if(StrEqual(currentTag.title, "file" ) )
                {
                    *currentfolderSpriteCounter = *currentfolderSpriteCounter + 1;
                    
                    Assert(tempSpriteCount < ArrayCount(tempSprites));
                    SpriteInfo* tempSprite = tempSprites + tempSpriteCount++;
                    
                    u8 flags = 0;
                    b32 isComposed = false;
                    b32 isEmptySpace = false;
                    b32 isSubPart = false;
                    b32 isEntity = false;
                    
                    char* pieceName = GetXMLValues(&currentTag, "name");
                    char* debug = pieceName;
                    Assert(pieceName);
                    char* slash = AdvanceToLastSlash(pieceName);
                    if(slash != pieceName)
                    {
                        isComposed = true;
                        
                        char* toCheck = "weapons";
                        char* toCheck2 = "Weapons";
                        char* toCheck3 = "equipment";
                        char* toCheck4 = "Equipment";
                        
                        if(!StrEqual(StrLen(toCheck), toCheck, pieceName) &&
                           !StrEqual(StrLen(toCheck2), toCheck2, pieceName) &&
                           !StrEqual(StrLen(toCheck3), toCheck3, pieceName) &&
                           !StrEqual(StrLen(toCheck4), toCheck4, pieceName))
                        {
                            isComposed = false;
                            isEntity = true;
                        }
                        
                        pieceName = slash;
                    }
                    
                    if(pieceName[0] == '#')
                    {
                        pieceName++;
                        isComposed = false;
                        isSubPart = true;
                    }
                    
                    Vec2 pivot;
                    GetXMLValuef(&currentTag, "pivot_x", &pivot.x );
                    GetXMLValuef(&currentTag, "pivot_y", &pivot.y );
                    
                    u64 stringHash;
                    u32 namelength = 0;
                    char* point = pieceName;
                    while(*point)
                    {
                        if(*point == '.')
                        {
                            namelength = (u32) (point - pieceName);
                            break;
                        }
                        ++point;
                    }
                    stringHash = StringHash(pieceName, namelength);
                    
                    isEmptySpace = StrEqual(StrLen("space"), "space", pieceName);
                    if(isEmptySpace)
                    {
                        isEntity = false;
                        isComposed = false;
                    }
                    
                    tempSprite->stringHashID = stringHash;
                    tempSprite->pivot = pivot;
                    
                    tempSprite->flags = 0;
                    if(isComposed)
                    {
                        tempSprite->flags |= Sprite_Composed;
                    }
                    
                    if(isEmptySpace)
                    {
                        tempSprite->flags |= Sprite_EmptySpace;
                    }
                    
                    if(isSubPart)
                    {
                        tempSprite->flags |= Sprite_SubPart;
                    }
                    
                    if(isEntity)
                    {
                        tempSprite->flags |= Sprite_Entity;
                    }
                    
                    StrCpy(pieceName, StrLen(pieceName), tempSprite->name);
                }
                else if(StrEqual(currentTag.title, "animation"))
                {
                    i32 length = 0; 
                    Assert(GetXMLValuei(&currentTag, "length", &length ) );
                    char* animationName = GetXMLValues(&currentTag, "name" );
                    Assert(animationName );
                    
                    
                    char* looping = GetXMLValues(&currentTag, "looping");
                    result.singleCycle = StrEqual(looping, "false");
                    
                    if(animationIndex == currentIndex++ )
                    {
                        loading = true;
                        result.durationMS = SafeTruncateToU16(length);
                        Assert(result.durationMS);
                        
                        u32 animationNameLength = 0;
                        char* animationNameTest = animationName;
                        while(*animationNameTest)
                        {
                            if(*animationNameTest++ == '_')
                            {
                                break;
                            }
                            
                            ++animationNameLength;
                        }
                        
                        result.stringHashID = StringHash(animationName, animationNameLength);
                        
                        frameCount = 0;
                        currentSpin = 0;
                    }
                }
            }
            else
            {
                if(StrEqual(currentTag.title, "animation" ) )
                {
                    ended = true;
                    break;
                }
                else if(StrEqual(currentTag.title, "mainline" ) )
                {
                    mainLineActive = true;
                }
                else if(StrEqual(currentTag.title, "key" ) )
                {
                    if(mainLineActive )
                    {
                        Assert(frameCount < ArrayCount(result.frames ) );
                        tempFrame = frames + frameCount++;
                        Assert(frameCount < (i32 ) frameCountResult );
                        i32 timeLine = 0;
                        GetXMLValuei(&currentTag, "time", &timeLine );
                        tempFrame->timeLine = timeLine;
                        tempFrame->countBones = 0;
                        tempFrame->countAss = 0;
                    }
                    else
                    {
                        // NOTE(leonardo): we are loading data for real NOW!
                        i32 timeline = 0;
                        GetXMLValuei(&currentTag, "time", &timeline );
                        timeline = timeline;
                        
                        b32 found = false;
                        for(i32 frameIndex = 0; frameIndex < frameCount; ++frameIndex )
                        {
                            TempFrame* testFrame = frames + frameIndex;
                            if(testFrame->timeLine == timeline )
                            {
                                realFrameIndex = frameIndex;
                                found = true;
                            }
                        }
                        Assert(found );
                        if(realFrameIndex == 0 )
                        {
                            keyiedOnFirst = true;
                        }
                        else
                        {
                            Assert(keyiedOnFirst );
                        }
                        
                        i32 spin = 0;
                        GetXMLValuei(&currentTag, "spin", &spin );
                        currentSpin = spin;
                    }
                }
                else if(StrEqual(currentTag.title, "bone_ref" ) )
                {
                    Assert(mainLineActive );
                    Assert(tempFrame );
                    
                    i32 parentID = -1;
                    GetXMLValuei(&currentTag, "parent", &parentID );
                    
                    Assert(parentID < tempFrame->countBones );
                    i32 timeLineIndex = 0;
                    Assert(GetXMLValuei(&currentTag, "timeline", &timeLineIndex ) );
                    
                    Assert(tempFrame->countBones < ArrayCount(tempFrame->bones));
                    TempBone* tempBone = tempFrame->tempBones + tempFrame->countBones++;
                    tempBone->timelineIndex = timeLineIndex + 1;
                    tempBone->parentID = parentID;
                    
                    
                    TempFrame* reference = frames;
                    for(i32 refBoneIndex = 0; refBoneIndex < reference->countBones; ++refBoneIndex )
                    {
                        TempBone* refTemp = reference->tempBones + refBoneIndex;
                        if(refTemp->timelineIndex == tempBone->timelineIndex )
                        {
                            Assert(tempBone->parentID == refTemp->parentID );
                        }
                    }
                }
                else if(StrEqual(currentTag.title, "object_ref" ) )
                {
                    Assert(mainLineActive );
                    Assert(tempFrame );
                    
                    i32 parentID = 0;
                    Assert(GetXMLValuei(&currentTag, "parent", &parentID ) );
                    Assert(parentID < tempFrame->countBones ); 
                    i32 timeLineIndex = 0;
                    Assert(GetXMLValuei(&currentTag, "timeline", &timeLineIndex ) );
                    i32 zIndex = 0;
                    Assert(GetXMLValuei(&currentTag, "z_index", &zIndex ) );
                    
                    Assert(tempFrame->countAss < ArrayCount(tempFrame->ass ) );
                    TempAss* tempAss = tempFrame->tempAss + tempFrame->countAss++;
                    tempAss->zIndex = zIndex;
                    tempAss->timelineIndex = timeLineIndex + 1;
                    tempAss->parentBoneID = parentID;
                    
                    TempFrame* reference = frames;
                    for(i32 refAssIndex = 0; refAssIndex < reference->countAss; ++refAssIndex)
                    {
                        TempAss* refTemp = reference->tempAss + refAssIndex;
                        if(refTemp->timelineIndex == tempAss->timelineIndex)
                        {
                            TempBone* refBone = reference->tempBones + refTemp->parentBoneID;
                            TempBone* myBone = tempFrame->tempBones + parentID;
                            
                            Assert(refBone->timelineIndex == myBone->timelineIndex);
                        }
                    }
                }
                else if(StrEqual(currentTag.title, "timeline" ) )
                {
                    if(!mainLineActive )
                    {
                        result.frameCount = SafeTruncateToU16(frameCount );
                    }
                    
                    mainLineActive = false;
                    timeLineActive = 0;
                    Assert(GetXMLValuei(&currentTag, "id", &timeLineActive ) );
                    timeLineActive += 1;
                    activeTimelineName = GetXMLValue(&currentTag, "name" );
                    keyiedOnFirst = false;
                }
                else if(StrEqual(currentTag.title, "bone" ) )
                {
                    Assert(!mainLineActive );
                    
                    r32 x = 0;
                    GetXMLValuef(&currentTag, "x", &x );
                    r32 y = 0;
                    GetXMLValuef(&currentTag, "y", &y );
                    r32 angle = 0;
                    GetXMLValuef(&currentTag, "angle", &angle );
                    
                    TempFrame* temp = frames + realFrameIndex;
                    i32 boneID = -1;
                    i32 parentID = -2;
                    for(i32 indexB = 0; indexB < temp->countBones; indexB++ )
                    {
                        TempBone* tempB = temp->tempBones + indexB;
                        if(tempB->timelineIndex == timeLineActive )
                        {
                            parentID = tempB->parentID;
                            boneID = indexB;
                            break;
                        }
                    }
                    
                    Assert(boneID >= 0 && boneID < temp->countBones );
                    Assert(parentID >= -1 && parentID < temp->countBones );
                    
                    Bone* bone = temp->bones + boneID;
                    bone->parentOffset = V2(x, y ) * (PLAYER_VIEW_WIDTH_IN_WORLD_METERS / DEFAULT_WIDTH );
                    bone->parentAngle = angle;
                    bone->spin = currentSpin;
                    bone->id = boneID;
                    bone->parentID = parentID;
                    bone->timeLineIndex = timeLineActive;
                }
                else if(StrEqual(currentTag.title, "object" ) )
                {
                    Assert(!mainLineActive );
                    
                    r32 x = 0;
                    GetXMLValuef(&currentTag, "x", &x );
                    r32 y = 0;
                    GetXMLValuef(&currentTag, "y", &y );
                    r32 angle = 0;
                    GetXMLValuef(&currentTag, "angle", &angle );
                    
                    r32 scaleX = 1, scaleY = 1;
                    GetXMLValuef(&currentTag, "scale_x", &scaleX );
                    GetXMLValuef(&currentTag, "scale_y", &scaleY );
                    
                    r32 alpha = 1.0f;
                    GetXMLValuef(&currentTag, "a", &alpha );
                    
                    TempFrame* temp = frames + realFrameIndex;
                    
                    i32 zIndex = -1;
                    i32 boneID = -1;
                    for(i32 indexA = 0; indexA < temp->countAss; indexA++ )
                    {
                        TempAss* tempA = temp->tempAss + indexA;
                        if(tempA->timelineIndex == timeLineActive )
                        {
                            zIndex = tempA->zIndex;
                            boneID = tempA->parentBoneID;
                            break;
                        }
                    }
                    
                    Assert(zIndex >= 0 && zIndex < temp->countAss );
                    Assert(boneID >= 0 && boneID < temp->countBones );
                    PieceAss* ass = temp->ass + zIndex;
                    ass->boneID = boneID;
                    
                    i32 fileIndex = 0;
                    Assert(GetXMLValuei(&currentTag, "file", &fileIndex ) );
                    i32 folderIndex = 0;
                    Assert(GetXMLValuei(&currentTag, "folder", &folderIndex ) );
                    
                    
                    
                    u32 sourceSpriteIndex = 0;
                    for(i32 dirIndex = 0; dirIndex < folderIndex; ++dirIndex )
                    {
                        sourceSpriteIndex += folderSpriteCount[dirIndex];
                    }
                    sourceSpriteIndex += fileIndex;
                    SpriteInfo* source = tempSprites + sourceSpriteIndex;
                    
                    
                    u32 definitiveSpriteIndex = 0;
                    b32 toAdd = true;
                    for(u32 spriteIndex = 0; spriteIndex < definitiveSpriteCount; ++spriteIndex)
                    {
                        SpriteInfo* test = result.spriteInfos + spriteIndex;
                        if(StrEqual(source->name, test->name))
                        {
                            toAdd = false;
                            definitiveSpriteIndex = spriteIndex;
                        }
                    }
                    
                    if(toAdd)
                    {
                        definitiveSpriteIndex = definitiveSpriteCount++;
                        SpriteInfo* dest = result.spriteInfos + definitiveSpriteIndex;
                        *dest = *source;
                    }
                    
                    
                    ass->boneOffset = V2(x, y )  * (PLAYER_VIEW_WIDTH_IN_WORLD_METERS / 1920.0f );
                    ass->additionalZOffset = 0;
                    ass->angle = angle;
                    ass->scale = V2(scaleX, scaleY );
                    ass->alpha = alpha;
                    ass->spin = currentSpin;
                    ass->timeLineIndex = timeLineActive;
                    ass->spriteIndex = definitiveSpriteIndex;
                }
            }
        }
        
    }
    
    u32 boneIndexToCopy = 0;
    u32 assIndexToCopy = 0;
    
    Assert(definitiveSpriteCount < ArrayCount(result.spriteInfos ) );
    result.spriteInfoCount = definitiveSpriteCount;
    
    for(u32 frameIndex = 0; frameIndex < result.frameCount; frameIndex++ )
    {
        Assert(frameIndex < ArrayCount(result.frames ) );
        
        TempFrame* source = frames + frameIndex;
        FrameData* dest = result.frames + frameIndex;
        
        dest->timelineMS = SafeTruncateToU16(source->timeLine );
        dest->firstBoneIndex = boneIndexToCopy;
        dest->firstAssIndex = assIndexToCopy;
        
        for(i32 boneIndex = 0; boneIndex < source->countBones; boneIndex++ )
        {
            Assert(boneIndexToCopy < boneCount );
            Bone* sourceBone = source->bones + boneIndex;
            
            if(sourceBone->timeLineIndex > 0 )
            {
                Bone* destBone = result.bones + boneIndexToCopy++;
                *destBone = *sourceBone;
                dest->countBones++;
            }
        }
        
        for(i32 assIndex = 0; assIndex < source->countAss; assIndex++ )
        {
            Assert(assIndexToCopy < assCount );
            PieceAss* sourceAss = source->ass + assIndex;
            if(sourceAss->timeLineIndex > 0 )
            {
                PieceAss* destAss = result.ass + assIndexToCopy++;
                *destAss = *sourceAss;
                dest->countAss++;;
            }
        }
    }
    
    free(frames );
    
    Assert(result.durationMS );
    return result;
}

internal u32 CountAnimationInFile(char* path, char* filename )
{
    u32 result = 0;
    char completeName[256];
    sprintf(completeName, "%s/%s", path, filename );
    EntireFile animation = ReadFile(completeName );
    //Assert(animation.content );
    if(animation.content )
    {
        // TODO(Leonardo ): robustness!
        i32 startOffset = 134;
        char* start = (char* ) animation.content + startOffset;
        
        b32 mainLineActive = false;
        b32 loading = false;
        b32 ended = false;
        i32 timeLineActive = 0;
        
        i32 frameCount = 0;
        TempFrame* tempFrame = 0;
        i32 realFrameIndex = 0;
        i32 currentSpin = 0;
        
        while(start != 0 && !ended )
        {
            XMLTag currentTag = {};
            start = ReadXMLTag(start, &currentTag );
            
            if(StrEqual(currentTag.title, "animation" ) )
            {
                ++result;
            }
        }
    }
    
    free(animation.content );
    return result;
}

internal void GetAnimationName(char* path, char* filename, u32 animationIndex, char* output, u32 outputLength )
{
    u32 currentIndex = 0;
    char completeName[256];
    sprintf(completeName, "%s/%s", path, filename );
    EntireFile animation = ReadFile(completeName );
    //Assert(animation.content );
    if(animation.content )
    {
        // TODO(Leonardo ): robustness!
        i32 startOffset = 134;
        char* start = (char* ) animation.content + startOffset;
        
        b32 mainLineActive = false;
        b32 loading = false;
        b32 ended = false;
        i32 timeLineActive = 0;
        
        i32 frameCount = 0;
        TempFrame* tempFrame = 0;
        i32 realFrameIndex = 0;
        i32 currentSpin = 0;
        
        while(start != 0 && !ended )
        {
            XMLTag currentTag = {};
            start = ReadXMLTag(start, &currentTag );
            
            if(StrEqual(currentTag.title, "animation" ) )
            {
                if(currentIndex++ == animationIndex )
                {
                    char* name = GetXMLValue(&currentTag, "name" );
                    Assert(StrLen(name ) < outputLength );
                    StrCpy(name, StrLen(name ), output );
                }
            }
        }
    }
    
    free(animation.content );
}

FILE* out;


internal void BeginAssetType(Assets* assets, u32 type )
{
    Assert(assets->DEBUGAssetType == 0 );
    assets->DEBUGAssetType = assets->types + type;
    assets->DEBUGAssetType->ID = type;
    assets->DEBUGAssetType->firstAssetIndex = assets->countAssets;
    assets->DEBUGAssetType->onePastLastAssetIndex = assets->countAssets;
    assets->currentStringHashID = 0;
    
    currentAssets_ = assets;
}

struct AddedAsset
{
    u32 ID;
    AssetSource* source;
    PakAsset* dest;
};

internal AddedAsset AddAsset(Assets* assets)
{
    AddedAsset result = {};
    result.ID = assets->countAssets++;
    u32 assetIndex = assets->DEBUGAssetType->onePastLastAssetIndex++;
    result.source = assets->assetSources + assetIndex;
    
    result.dest = assets->assets + assetIndex;
    result.dest->firstTagIndex = assets->countTags;
    result.dest->onePastLastTagIndex = assets->countTags;
    assets->assetIndex = result.ID;
    
    return result;
    
}

internal BitmapId AddBitmapAsset(char* path, char* filename, u64 stringHash = 0, r32 alignX = 0.5f, r32 alignY = 0.5f)
{
    Assets* assets = currentAssets_;
    AddedAsset asset = AddAsset(assets );
    asset.source->type = Pak_bitmap;
    StrCpy(filename, StrLen(filename ), asset.source->bitmap.filename, ArrayCount(asset.source->bitmap.filename ) );
    StrCpy(path, StrLen(path), asset.source->bitmap.path, ArrayCount(asset.source->bitmap.path ) );
    
    asset.dest->bitmap.align[0] = alignX;
    asset.dest->bitmap.align[1] = alignY;
    asset.dest->typeHashID = stringHash;
    asset.dest->nameHashID = 0;
    
    LoadedBitmap bitmap = LoadBitmap(path, filename, false);
    asset.dest->bitmap.nativeHeight = bitmap.downsampleFactor * bitmap.height * (PLAYER_VIEW_WIDTH_IN_WORLD_METERS / 1920.0f);
    free(bitmap.free );
    
    
    BitmapId result = { asset.ID };
    return result;
}

internal BitmapId AddCharacterAsset(LoadedFont* font, u32 codePoint )
{
    Assets* assets = currentAssets_;
    AddedAsset asset = AddAsset(assets );
    
    asset.source->type = Pak_fontGlyph;
    asset.source->glyph.codePoint = codePoint;
    asset.source->glyph.font = font;
    
    // NOTE(Leonardo ): alignment is set later!
    asset.dest->typeHashID = 0;
    asset.dest->nameHashID = 0;
    asset.dest->bitmap.align[0] = 0.0f;
    asset.dest->bitmap.align[1] = 0.0f;
    asset.dest->bitmap.nativeHeight = 0.0f;
    
    BitmapId result = {asset.ID};
    
    u32 glyphIndex = font->glyphsCount++;
    Assert(font->glyphsCount < font->maximumGlyphsCount );
    PakGlyph* glyph = font->glyphs + glyphIndex;
    glyph->unicodeCodePoint = codePoint;
    glyph->bitmapId = result;
    
    font->glyphIndexForCodePoint[codePoint] = glyphIndex;
    
    if(codePoint >= font->onePastHighestCodePoint )
    {
        font->onePastHighestCodePoint = codePoint + 1;
    }
    return result;
}

internal FontId AddFontAsset(LoadedFont* font )
{
    Assets* assets = currentAssets_;
    AddedAsset asset = AddAsset(assets );
    
    asset.source->type = Pak_font;
    asset.source->font.font = font;
    
    asset.dest->typeHashID = 0;
    asset.dest->nameHashID = 0;
    asset.dest->font.glyphsCount = font->glyphsCount;
    asset.dest->font.ascenderHeight = font->ascenderHeight;
    asset.dest->font.descenderHeight = font->descenderHeight;
    asset.dest->font.externalLeading = font->externalLeading;
    asset.dest->font.onePastHighestCodePoint = font->onePastHighestCodePoint;
    
    FontId result = { asset.ID };
    return result;
}


internal AnimationId AddAnimationAsset(char* path, char* filename, u32 animationIndex, u64 stringHashID = 0, u32 loopingType = 0, u16 startingPreparationTimeLine = 0, u16 endingPreparationTimeLine = 0,
                                       u16 spawningMS = 0,
                                       u32 waitingLoopingType = 0,
                                       u16 startingWaitingTimeLine = 0, u16 endingWaitingTimeLine = 0,b32 singleCycle = false, r32 firstBarrier = 0.0f,
                                       r32 secondBarrier = 0.0f,
                                       r32 thirdBarrier = 0.0f )
{
    Assets* assets = currentAssets_;
    AddedAsset asset = AddAsset(assets );
    
    asset.source->type = Pak_animation;
    asset.dest->typeHashID = stringHashID;
    asset.dest->nameHashID = stringHashID;
    StrCpy(path, StrLen(path ), asset.source->animation.path, ArrayCount(asset.source->animation.path ) );
    StrCpy(filename, StrLen(filename ), asset.source->animation.filename, ArrayCount(asset.source->animation.filename ) );
    asset.source->animation.animationIndex = animationIndex;
    asset.source->animation.header.spawningMS = spawningMS;
    asset.source->animation.header.loopingType = loopingType;
    asset.source->animation.header.startingPreparationTimeLine = startingPreparationTimeLine;
    asset.source->animation.header.endingPreparationTimeLine = endingPreparationTimeLine;
    asset.source->animation.header.waitingLoopingType = waitingLoopingType;
    asset.source->animation.header.startingWaitingTimeLine = startingWaitingTimeLine;
    asset.source->animation.header.endingWaitingTimeLine = endingWaitingTimeLine;
    asset.source->animation.header.singleCycle = singleCycle;
    asset.source->animation.header.barriers[0] = firstBarrier;
    asset.source->animation.header.barriers[1] = secondBarrier;
    asset.source->animation.header.barriers[2] = thirdBarrier;
    
    AnimationId result = { asset.ID };
    return result;
}

internal void AddEveryAnimationThatStartsWith(char* path, u64 hashID, char* animName)
{
    PlatformFileGroup fileGroup = Win32GetAllFilesBegin(PlatformFile_animation, path);
    for(u32 fileIndex = 0; fileIndex < fileGroup.fileCount; ++fileIndex )
    {
        PlatformFileHandle handle = Win32OpenNextFile(&fileGroup, path);
        char* fileName = handle.name;
        u32 animationCount = CountAnimationInFile(path, fileName);
        for(u32 animationIndex = 0; animationIndex < animationCount; ++animationIndex )
        {
            char animationName[32];
            GetAnimationName(path, fileName, animationIndex, animationName, sizeof(animationName));
            if(StrEqual(StrLen(animName), animationName, animName))
            {
                AddAnimationAsset(path, fileName, animationIndex, hashID);
            }
        }
        Win32CloseHandle(&handle );
    }
    Win32GetAllFilesEnd(&fileGroup );
}

internal SoundId AddSoundAsset(char* filename, u64 stringHash, i32 firstSampleIndex = 0, i32 sampleCount = 0)
{
    char* soundName = AdvanceToLastSlash(filename);
    
    Assets* assets = currentAssets_;
    AddedAsset asset = AddAsset(assets );
    
    asset.source->type = Pak_sound;
    asset.source->sound.firstSampleIndex = firstSampleIndex;
    StrCpy(filename, StrLen(filename), asset.source->sound.filename, ArrayCount(asset.source->bitmap.filename));
    asset.dest->typeHashID = stringHash;
    asset.dest->nameHashID = StringHash(soundName);
    
    asset.dest->sound.sampleCount = sampleCount;
    asset.dest->sound.chain = Chain_none;
    
    SoundId result = {asset.ID};
    return result;
}


inline void EndAssetType()
{
    Assets* assets = currentAssets_;
    Assert(assets->DEBUGAssetType );
    assets->countAssets = assets->DEBUGAssetType->onePastLastAssetIndex;
    assets->DEBUGAssetType = 0;
    
    currentAssets_ = 0;
}

inline void AddTag(u32 ID, r32 value )
{
    Assets* assets = currentAssets_;
    Assert(assets->assetIndex );
    
    PakAsset* dest = assets->assets + assets->assetIndex;
    dest->onePastLastTagIndex++;
    
    PakTag* tag = assets->tags + assets->countTags++;
    tag->ID = ID;
    tag->value = value;
}

inline void AddLabel(char* label, u32 labelLength, r32 value)
{
    u32 hash = (u32) (StringHash(label, labelLength) >> 32);
    u32 hashIndex = (hash & (LABEL_HASH_COUNT - 1)) + Tag_count;
    
    AddTag(hashIndex, value);
}

internal void WritePak(Assets* assets, char* fileName_)
{
    char outputpak[128];
    FormatString(outputpak, sizeof(outputpak), "assets/%s", fileName_);
    
    if(assets->countAssets > 1)
    {
        out = fopen(outputpak, "wb");
        
        if(out )
        {
            PAKHeader header = {};
            header.magicValue = PAK_MAGIC_NUMBER;
            header.version = PAK_VERSION;
            
            header.tagCount = assets->countTags;
            header.assetTypeCount = Asset_count + HASHED_ASSET_SLOTS;
            header.assetcount = assets->countAssets;
            
            u32 tagArraySize = header.tagCount * sizeof(PakTag );
            u32 assetArrayTypeSize = header.assetTypeCount * sizeof(PakAssetType );
            u32 assetArraySize = header.assetcount * sizeof(PakAsset ) ;
            
            header.tagOffset = sizeof(PAKHeader );
            header.assetTypeOffset = header.tagOffset + tagArraySize;
            header.assetOffset = header.assetTypeOffset + assetArrayTypeSize;
            
            fwrite(&header, sizeof(PAKHeader ), 1, out );
            fwrite(&assets->tags, tagArraySize, 1, out );
            fwrite(&assets->types, assetArrayTypeSize, 1, out );
            
            fseek(out, assetArraySize, SEEK_CUR );
            
            for(u32 assetIndex = 1; assetIndex < header.assetcount; assetIndex++ )
            {
                AssetSource* source = assets->assetSources + assetIndex;
                PakAsset* dest = assets->assets + assetIndex;
                dest->dataOffset = ftell(out );
                
                switch(source->type )
                {
                    case Pak_bitmap:
                    {
                        LoadedBitmap bitmap = LoadBitmap(source->bitmap.path, source->bitmap.filename, true );
                        dest->bitmap.dimension[0] = bitmap.width;
                        dest->bitmap.dimension[1] = bitmap.height;
                        
                        fwrite(bitmap.pixels, bitmap.width * bitmap.height * sizeof(u32 ), 1, out );
                        free(bitmap.free );
                    } break;
                    
                    case Pak_font:
                    {
                        LoadedFont* font = source->font.font;
                        
                        FinalizeFontKernings(font);
                        
                        u32 glyphsSize = font->glyphsCount * sizeof(PakGlyph );
                        fwrite(font->glyphs, glyphsSize, 1, out );
                        
                        u8* horizontalAdvancePtr = (u8* ) font->horizontalAdvancement;
                        for(u32 glyphIndex = 0; glyphIndex < font->glyphsCount; glyphIndex++ )
                        {
                            
                            u32 horizontalAdvanceSliceSize = sizeof(r32 ) * font->glyphsCount; 
                            fwrite(horizontalAdvancePtr, horizontalAdvanceSliceSize, 1, out );
                            horizontalAdvancePtr += sizeof(r32 ) * font->maximumGlyphsCount;
                        }
                        
                        FreeFont(font );
                    } break;
                    
                    case Pak_fontGlyph:
                    {
                        LoadedBitmap bitmap = LoadGlyph(source->glyph.font, source->glyph.codePoint, dest );
                        
                        dest->bitmap.dimension[0] = bitmap.width;
                        dest->bitmap.dimension[1] = bitmap.height;
                        fwrite(bitmap.pixels, bitmap.width * bitmap.height * sizeof(u32 ), 1, out );
                        
                        free(bitmap.free );
                    } break;
                    
                    case Pak_sound:
                    {
                        LoadedSound sound = LoadWAV(source->sound.filename, source->sound.firstSampleIndex, dest->sound.sampleCount);
                        dest->sound.sampleCount = sound.countSamples;
                        dest->sound.channelCount = sound.countChannels;
                        
                        for(u32 channelIndex = 0; channelIndex < sound.countChannels; channelIndex++ )
                        {
                            fwrite(sound.samples[channelIndex], sound.countSamples * sizeof(i16 ), 1, out); 
                        }
                        
                        free(sound.free );
                    } break;
                    
                    case Pak_animation:
                    {
                        LoadedAnimation animation = LoadAnimation(source->animation.path, source->animation.filename, source->animation.animationIndex);
                        
                        u32 countTotalBones = 0;
                        u32 countTotalAss = 0;
                        
                        for(u32 frameIndex = 0; frameIndex < animation.frameCount; frameIndex++)
                        {
                            FrameData* data = animation.frames + frameIndex;
                            
                            countTotalBones += data->countBones;
                            countTotalAss += data->countAss;
                        }
                        
                        source->animation.header.durationMS = animation.durationMS;
                        source->animation.header.nameHash = animation.stringHashID;
                        dest->nameHashID = animation.stringHashID;
                        //source->animation.header.singleCycle = animation.singleCycle;
                        if(!dest->typeHashID)
                        {
                            dest->typeHashID = animation.stringHashID;
                        }
                        
                        dest->animation.spriteCount = animation.spriteInfoCount;
                        
                        dest->animation.frameCount = animation.frameCount;
                        dest->animation.boneCount = countTotalBones;
                        dest->animation.assCount = countTotalAss;
                        
                        Assert(source->animation.header.durationMS > 0);
                        fwrite(&source->animation.header, sizeof(AnimationHeader), 1, out);
                        fwrite(animation.spriteInfos, sizeof(SpriteInfo) * animation.spriteInfoCount, 1, out);
                        fwrite(animation.frames, sizeof(FrameData) * animation.frameCount, 1, out);
                        fwrite(animation.bones, countTotalBones * sizeof(Bone), 1, out);
                        fwrite(animation.ass, countTotalAss * sizeof(PieceAss), 1, out);
                        
                        free(animation.bones);
                        free(animation.ass);
                        free(animation.free);
                    } break;
                    
                    InvalidDefaultCase;
                }
            }
            
            fseek(out, (u32 ) header.assetOffset, SEEK_SET );
            fwrite(&assets->assets, header.assetcount * sizeof(PakAsset), 1, out);
            
            fclose(out );
            
            
            PlatformFile uncompressed = DEBUGWin32ReadFile(outputpak);
            
            uLong uncompressedSize = (uLong) uncompressed.size;
            uLong compressedLen = compressBound(uncompressedSize);
            
            u8* compressed = (u8*) malloc((size_t) compressedLen + 4);
            
            Assert(uncompressedSize <= 0xffffffff);
            *((u32*) compressed) = uncompressedSize;
            
            int cmp_status = compress(compressed + 4, &compressedLen, (const unsigned char *) uncompressed.content, uncompressedSize);
            Assert(cmp_status == Z_OK);
            
            
            DEBUGWin32FreeFile(&uncompressed);
            DEBUGWin32WriteFile(outputpak, compressed, compressedLen + 4);
            free(compressed);
        }
        else
        {
            InvalidCodePath;
        }
    }
}

internal void InitializeAssets(Assets* assets )
{
    assets->countTags = 1;
    assets->countAssetType = 0;
    assets->DEBUGAssetType = 0;
    assets->countAssets = 1;
    assets->assetIndex = 0;
    
    for(u32 assetType = 0; assetType < Asset_count + HASHED_ASSET_SLOTS; assetType++ )
    {
        PakAssetType* type = assets->types + assetType;
        type->ID = 0;
        type->firstAssetIndex = 0;
        type->onePastLastAssetIndex = 0;
    }
}

internal void AddEveryFileWithAssetIndex(char* path, PlatformFileHandle handle, u32 additionalAssetIndex)
{
    char completeName[256];
    sprintf(completeName, "%s/%s", path, handle.name );
    EntireFile animation = ReadFile(completeName );
    //Assert(animation.content );
    if(animation.content )
    {
        // TODO(Leonardo ): robustness!
        i32 startOffset = 134;
        char* start = (char* ) animation.content + startOffset;
        
        b32 mainLineActive = false;
        b32 loading = false;
        b32 ended = false;
        i32 timeLineActive = 0;
        
        i32 frameCount = 0;
        TempFrame* tempFrame = 0;
        i32 realFrameIndex = 0;
        i32 currentSpin = 0;
        
        while(start != 0 && !ended )
        {
            XMLTag currentTag = {};
            start = ReadXMLTag(start, &currentTag );
            
            if(StrEqual(currentTag.title, "file" ) )
            {
                char* fileName = GetXMLValues(&currentTag, "name" );
                char* slash = AdvanceToLastSlash(fileName);
                
                // NOTE(Leonardo): we add only the character "pieces"
                if(slash == fileName)
                {
                    u32 fileNameLength = 0;
                    for(char* test = fileName; *test; ++test )
                    {
                        if(*test == '.' )
                        {
                            break;
                        }
                        ++fileNameLength;
                    }
                    
                    u64 hashID = StringHash(fileName, fileNameLength );
                    u32 hashIndex =  hashID & (HASHED_ASSET_SLOTS - 1 );
                    u32 assetIndex = Asset_count + hashIndex;
                    
                    Vec2 pivot;
                    GetXMLValuef(&currentTag, "pivot_x", &pivot.x );
                    GetXMLValuef(&currentTag, "pivot_y", &pivot.y );
                    
                    if(assetIndex == additionalAssetIndex )
                    {
                        AddBitmapAsset("objects", fileName, hashID, pivot.x, pivot.y );
                    }
                }
            }
        }
    }
    
    free(animation.content );
}

inline void GetNameWithoutPoint(char* dest, u32 destLength, char* source)
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

inline void AddTagsBasedOnName(char* path, char* filename, u32 animationIndex)
{
    char animationName[32];
    GetAnimationName(path, filename, animationIndex, animationName, sizeof(animationName));
    
    char* parse = animationName;
    
    b32 ground = false;
    b32 open = false;
    
    while(*parse)
    {
        if(*parse == '_')
        {
            char* tagName = parse + 1;
            if(StrEqual(StrLen("open"), "open", tagName))
            {
                open = true;
            }
            else if(StrEqual(StrLen("ground"), "ground", tagName))
            {
                ground = true;
            }
        }
        
        ++parse;
    }
    
    if(open)
    {
        AddTag(Tag_ObjectState, ObjectState_Open);
    }
    else if(ground)
    {
        AddTag(Tag_ObjectState, ObjectState_Ground);
    }
    else
    {
        AddTag(Tag_ObjectState, ObjectState_None);
    }
}


inline char* StringPresentInFile(char* file, char* token, b32 limitToSingleList)
{
    char* result = 0;
    Tokenizer tokenizer = {};
    tokenizer.at = (char*) file;
    
    b32 parsing = true;
    i32 level = 0;
    
    while(parsing)
    {
        Token t = GetToken(&tokenizer);
        switch(t.type)
        {
            case Token_Identifier:
            {
                if(TokenEquals(t, token))
                {
                    if(RequireToken(&tokenizer, Token_EqualSign))
                    {
                        if(RequireToken(&tokenizer, Token_OpenParen))
                        {
                            result = tokenizer.at;
                        }
                    }
                    parsing = false;
                }
            } break;
            
            case Token_String:
            {
                Token string = Stringize(t);
                if(TokenEquals(string, token))
                {
                    result = tokenizer.at;
                    if(RequireToken(&tokenizer, Token_EqualSign))
                    {
                        if(RequireToken(&tokenizer, Token_OpenParen))
                        {
                            result = tokenizer.at;
                        }
                    }
                    parsing = false;
                }
            } break;
            
            case Token_OpenParen:
            {
                ++level;
            } break;
            
            case Token_CloseParen:
            {
                --level;
                if(limitToSingleList && level < 0)
                {
                    parsing = false;
                }
            } break;
            
            case Token_EndOfFile:
            {
                parsing = false;
            } break;
        }
    }
    
    return result;
}

inline void RemoveSpaces(char* dest, char* source)
{
    for(char* s = source; *s; ++s)
    {
        char copy = *s;
        if(copy == ' ')
        {
            copy = '_';
        }
        
        *dest++ = copy;
    }
    *dest = 0;
}

internal void AddLabelsFromFile(PlatformFile* labelsFile, char* folderName, char* assetName)
{
    char folderNameNoWhiteSpaces[64];
    Assert(StrLen(folderName) < ArrayCount(folderNameNoWhiteSpaces));
    RemoveSpaces(folderNameNoWhiteSpaces, folderName);
    char* folder = StringPresentInFile((char*) labelsFile->content, folderNameNoWhiteSpaces, false);
    Assert(folder);
    char* ptr = StringPresentInFile(folder, assetName, true);
    Assert(ptr);
    
    Tokenizer tokenizer = {};
    tokenizer.at = ptr;
    
    b32 parsing = true;
    while(parsing)
    {
        Token t = GetToken(&tokenizer);
        
        switch(t.type)
        {
            case Token_String:
            case Token_Identifier:
            {
                if(t.type == Token_String)
                {
                    t = Stringize(t);
                }
                
                if(TokenEquals(t, "labels"))
                {
                    if(RequireToken(&tokenizer, Token_EqualSign))
                    {
                        if(RequireToken(&tokenizer, Token_OpenParen))
                        {
                            Token empty = GetToken(&tokenizer);
                            if(empty.type == Token_Identifier && TokenEquals(empty, "empty"))
                            {
                                AdvanceToNextToken(&tokenizer, Token_CloseBraces);
                                
                                while(true)
                                {
                                    Token labelToken = GetToken(&tokenizer);
                                    if(labelToken.type == Token_CloseParen)
                                    {
                                        break;
                                    }
                                    else
                                    {
                                        if(RequireToken(&tokenizer, Token_Identifier) &&
                                           RequireToken(&tokenizer, Token_EqualSign))
                                        {
                                            Token labelName = Stringize(GetToken(&tokenizer));
                                            
                                            if(RequireToken(&tokenizer, Token_Comma) &&
                                               RequireToken(&tokenizer, Token_Identifier) &&
                                               RequireToken(&tokenizer, Token_EqualSign))
                                            {
                                                Token labelValue = GetToken(&tokenizer);
                                                
                                                AddLabel(labelName.text, labelName.textLength, (r32) atof(labelValue.text));
                                            }
                                        }
                                        
                                        AdvanceToNextToken(&tokenizer, Token_CloseBraces);
                                    }
                                }
                            }
                        }
                    }
                }
                
                parsing = false;
            } break;
        }
    }
}





struct BitmapFileHandle
{
    char name[64];
    u32 assetIndex;
    u64 ID;
};

internal void WriteBitmaps(char* folder, char* name, PlatformFile* labelsFile)
{
    Assets assets_;
    Assets* assets = &assets_;
    InitializeAssets(assets);
    
    char completePath[128];
    FormatString(completePath, sizeof(completePath), "%s/%s", folder, name);
    
    
	char* hashName = name;
	if(hashName[0] == '#')
	{
		++hashName;
	}
    u64 hashID = StringHash(hashName);
    
    
    u32 hashIndex =  hashID & (HASHED_ASSET_SLOTS - 1);
    u32 assetIndex = Asset_count + hashIndex;
    PlatformFileGroup bitmapGroup = Win32GetAllFilesBegin(PlatformFile_image, completePath);
    if(bitmapGroup.fileCount)
    {
        BeginAssetType(assets, assetIndex);
        for(u32 imageIndex = 0; imageIndex < bitmapGroup.fileCount; ++imageIndex)
        {
            PlatformFileHandle bitmapHandle = Win32OpenNextFile(&bitmapGroup, completePath);
            AddBitmapAsset(completePath, bitmapHandle.name, hashID);
            
            if(labelsFile)
            {
                AddLabelsFromFile(labelsFile, name, bitmapHandle.name);
            }
            
            Win32CloseHandle(&bitmapHandle);
        }
        EndAssetType();
    }
    
    Win32GetAllFilesEnd(&bitmapGroup);
    
    
    char subfolderPath[128];
    FormatString(subfolderPath, sizeof(subfolderPath), "%s/side", completePath);
    PlatformFileGroup sideGroup = Win32GetAllFilesBegin(PlatformFile_image, subfolderPath);
    
    if(sideGroup.fileCount)
    {
        BitmapFileHandle* handles = (BitmapFileHandle*) malloc(sizeof(BitmapFileHandle) * sideGroup.fileCount);
        
        for(u32 fileIndex = 0; fileIndex < sideGroup.fileCount; ++fileIndex)
        {
            PlatformFileHandle handle = Win32OpenNextFile(&sideGroup, subfolderPath);
            BitmapFileHandle* bitmap = handles + fileIndex;
            
            char* limbName = handle.name;
            char nameWithoutPoint[64];
            GetNameWithoutPoint(nameWithoutPoint, ArrayCount(nameWithoutPoint), limbName);
            
            FormatString(bitmap->name, sizeof(bitmap->name), "%s", limbName);
            
            bitmap->ID = StringHash(nameWithoutPoint);
            u32 limbIndex =  bitmap->ID & (HASHED_ASSET_SLOTS - 1);
            bitmap->assetIndex = Asset_count + limbIndex;
            
        }
        
        Win32GetAllFilesEnd(&sideGroup);
        
        for(u32 additionalAssetIndex = Asset_count; additionalAssetIndex < (Asset_count + HASHED_ASSET_SLOTS); ++additionalAssetIndex)
        {
            BeginAssetType(assets, additionalAssetIndex);
            for(u32 fileIndex = 0; fileIndex < sideGroup.fileCount; ++fileIndex)
            {
                BitmapFileHandle* bitmap = handles + fileIndex;
                if(bitmap->assetIndex == additionalAssetIndex)
                {
                    AddBitmapAsset(subfolderPath, bitmap->name, bitmap->ID);
                    r32 firstHashHalfValue = (r32) (hashID >> 32);
                    r32 secondHashHalfValue = (r32) (hashID & 0xFFFFFFFF);
                    AddTag(Tag_firstHashHalf, firstHashHalfValue);
                    AddTag(Tag_secondHashHalf, secondHashHalfValue);
                }
            }
            EndAssetType();
        }
    }
    
    
    char pakName[128];
    FormatString(pakName, sizeof(pakName), "%sB.pak", name);
    WritePak(assets, pakName);
}


internal void WriteAnimations(char* folder, char* name)
{
    Assets assets_;
    Assets* assets = &assets_;
    InitializeAssets(assets);
    
    
    char completePath[128];
    FormatString(completePath, sizeof(completePath), "%s/%s/side", folder, name);
    
    u64 hashID = StringHash(name);
    
    BeginAssetType(assets, Asset_rig);
    AddEveryAnimationThatStartsWith(completePath, hashID, "rig");
    EndAssetType();
    
    BeginAssetType(assets, Asset_standing);
    AddEveryAnimationThatStartsWith(completePath, hashID, "idle");
    EndAssetType();
    
    BeginAssetType(assets, Asset_moving);
    AddEveryAnimationThatStartsWith(completePath, hashID, "walk");
    EndAssetType();
    
    
    BeginAssetType(assets, Asset_attacking);
    AddEveryAnimationThatStartsWith(completePath, hashID, "attack");
    EndAssetType();
    
    BeginAssetType(assets, Asset_eating);
    AddEveryAnimationThatStartsWith(completePath, hashID, "eat");
    EndAssetType();
    
    
    BeginAssetType(assets, Asset_casting);
    AddEveryAnimationThatStartsWith(completePath, hashID, "cast");
    EndAssetType();
    
    
    BeginAssetType(assets, Asset_equipmentRig );
    AddEveryAnimationThatStartsWith(completePath, hashID, "equipmentmap");
    EndAssetType();
    
    
    char pakName[128];
    FormatString(pakName, sizeof(pakName), "%sA.pak", name);
    WritePak(assets, pakName);
}

internal void WriteEquipmentMaps()
{
    Assets assets_;
    Assets* assets = &assets_;
    InitializeAssets(assets);
    
    BeginAssetType(assets, Asset_equipmentMap);
    
    char* equipmentPath = "definition/equipment";
    
    PlatformFileGroup fileGroup = Win32GetAllFilesBegin(PlatformFile_animation, equipmentPath);
    for(u32 fileIndex = 0; fileIndex < fileGroup.fileCount; ++fileIndex )
    {
        PlatformFileHandle handle = Win32OpenNextFile(&fileGroup, equipmentPath );
        char* filename = handle.name;
        if(!StrEqual(StrLen("autosave"), filename, "autosave"))
        {
            u32 animationCount = CountAnimationInFile(equipmentPath, filename );
            for(u32 animationIndex = 0; animationIndex < animationCount; ++animationIndex )
            {
                AddAnimationAsset(equipmentPath, filename, animationIndex );
                AddTagsBasedOnName(equipmentPath, filename, animationIndex);
            }
        }
        Win32CloseHandle(&handle );
    }
    
    Win32GetAllFilesEnd(&fileGroup );
    EndAssetType();
    
    WritePak(assets, "equipmentmaps.pak" );
}

internal void OutputFoldersAutocompleteFile(char* filename, char* path)
{
    char* outputPath = "assets";
    char completePath[128];
    FormatString(completePath, sizeof(completePath), "%s/%s.autocomplete", outputPath, filename);
    
    
    char* buffer = (char*) malloc(MegaBytes(2));
    char* writeHere = buffer;
    
    PlatformSubdirNames* subdir = (PlatformSubdirNames* ) malloc(sizeof(PlatformSubdirNames));
    Win32GetAllSubdirectoriesName(subdir, path);
    for(u32 subdirIndex = 0; subdirIndex < subdir->subDirectoryCount; ++subdirIndex)
    {
        char* folderName = subdir->subdirs[subdirIndex];
        if(!StrEqual(folderName, ".") && !StrEqual(folderName, ".."))
        {
            writeHere += sprintf(writeHere, "\"%s\",", folderName);
        }
    }
    free(subdir);
    
    DEBUGWin32WriteFile(completePath, buffer, StrLen(buffer));
    
    free(buffer);
}

inline char* AddFolderToFile(char* addHere, char* fileEnd, char* folder, char* params)
{
    u32 sizeToEnd = (u32) (fileEnd - addHere);
    char toAdd[128];
    FormatString(toAdd, sizeof(toAdd), "\"%s\" = (%s),", folder, params);
    
    u32 roomToMake = StrLen(toAdd);
    Assert(roomToMake <= sizeToEnd);
    
    memcpy(addHere + roomToMake, addHere, sizeToEnd);
    memcpy(addHere, toAdd, roomToMake);
    
    char* result = addHere + roomToMake - 2;
    return result;
}

inline void AddAssetToFile(char* addHere, char* fileEnd, char* tag, char* assetName, b32 labeled)
{
    u32 sizeToEnd = (u32) (fileEnd - addHere);
    
    char toAdd[256];
    
    if(labeled)
    {
        FormatString(toAdd, sizeof(toAdd), "{%s = \"%s\", labels = (#empty = {name = \"invalid\", value = 0.0})},", tag, assetName);
    }
    else
    {
        FormatString(toAdd, sizeof(toAdd), "{%s = \"%s\"},", tag, assetName);
    }
    u32 roomToMake = StrLen(toAdd);
    Assert(roomToMake <= sizeToEnd);
    
    memcpy(addHere + roomToMake, addHere, sizeToEnd);
    memcpy(addHere, toAdd, roomToMake);
}

internal void WriteAssetDefinitionFile(char* path, char* filename, char* definitionParams)
{
    char* assetPath = "assets";
    
    char assetDest[512];
    FormatString(assetDest, sizeof(assetDest), "%s/%s", assetPath, filename);
    
    char oldPath[512];
    FormatString(oldPath, sizeof(oldPath), "%s/%s", path, filename);
    
    
    u32 newFileSize = MegaBytes(4);
    char* newFile = (char*) malloc(newFileSize);
    memset(newFile, 0, newFileSize);
    char* endFile = newFile + newFileSize;
    
    
    PlatformFile oldFile = DEBUGWin32ReadFile(oldPath);
    if(oldFile.content && oldFile.size <= newFileSize)
    {
        memcpy(newFile, oldFile.content, oldFile.size);
        DEBUGWin32FreeFile(&oldFile);
    }
    
    
    PlatformSubdirNames* subdir = (PlatformSubdirNames*) malloc(sizeof(PlatformSubdirNames));
    Win32GetAllSubdirectoriesName(subdir, path);
    for(u32 subdirIndex = 0; subdirIndex < subdir->subDirectoryCount; ++subdirIndex)
    {
        char* folderName = subdir->subdirs[subdirIndex];
        if(!StrEqual(folderName, ".") && !StrEqual(folderName, ".."))
        {
            char folderNameNoWhiteSpaces[64];
            Assert(StrLen(folderName) < ArrayCount(folderNameNoWhiteSpaces));
            RemoveSpaces(folderNameNoWhiteSpaces, folderName);
            char* folderPtr = StringPresentInFile(newFile, folderNameNoWhiteSpaces, false);
            
            if(!folderPtr)
            {
                folderPtr = AddFolderToFile(newFile, endFile, folderNameNoWhiteSpaces, definitionParams);
            }
            else
            {
                Tokenizer paramT = {};
                paramT.at = folderPtr;
                
                while(true)
                {
                    Token p = GetToken(&paramT);
                    if(p.type == Token_Pound)
                    {
                        Token value = GetToken(&paramT);
                        folderPtr = paramT.at + 1;
                    }
                    else
                    {
                        break;
                    }
                }
            }
            
            
            char folderPath[512];
            FormatString(folderPath, sizeof(folderPath), "%s/%s", path, folderName);
            
            PlatformFileGroup soundGroup = Win32GetAllFilesBegin(PlatformFile_sound, folderPath);
            for(u32 soundIndex = 0; soundIndex < soundGroup.fileCount; ++soundIndex)
            {
                PlatformFileHandle soundHandle = Win32OpenNextFile(&soundGroup, folderPath);
                
                if(!StringPresentInFile(folderPtr, soundHandle.name, true))
                {
                    AddAssetToFile(folderPtr, endFile, "soundName", soundHandle.name, false);
                }
                
                Win32CloseHandle(&soundHandle);
            }
            Win32GetAllFilesEnd(&soundGroup);
            
            
            
            PlatformFileGroup imageGroup = Win32GetAllFilesBegin(PlatformFile_image, folderPath);
            for(u32 imageIndex = 0; imageIndex < imageGroup.fileCount; ++imageIndex)
            {
                PlatformFileHandle imageHandle = Win32OpenNextFile(&imageGroup, folderPath);
                
                if(!StringPresentInFile(folderPtr, imageHandle.name, true))
                {
                    AddAssetToFile(folderPtr, endFile, "componentName", imageHandle.name, true);
                }
                
                Win32CloseHandle(&imageHandle);
            }
            Win32GetAllFilesEnd(&imageGroup);
        }
        
    }
    free(subdir);
    DEBUGWin32WriteFile(assetDest, newFile, StrLen(newFile));
}

internal void WriteComponents()
{
    char* componentsPath = "definition/components";
    char* assetFile = "components.fad";
    char* definitionParams = "#cantBeDeleted";
    
    WriteAssetDefinitionFile(componentsPath, assetFile, definitionParams);
    
    OutputFoldersAutocompleteFile("component", componentsPath);
    
    PlatformSubdirNames* subdir = (PlatformSubdirNames*) malloc(sizeof(PlatformSubdirNames));
    Win32GetAllSubdirectoriesName(subdir, componentsPath);
    
    
    char labelsPath[64];
    FormatString(labelsPath, sizeof(labelsPath), "assets/%s", assetFile);
    PlatformFile labelsFile = DEBUGWin32ReadFile(labelsPath);
    Assert(labelsFile.content);
    
    for(u32 subdirIndex = 0; subdirIndex < subdir->subDirectoryCount; ++subdirIndex)
    {
        char* folderName = subdir->subdirs[subdirIndex];
        
        if(!StrEqual(folderName, ".") && !StrEqual(folderName, ".."))
        {
            WriteBitmaps(componentsPath, folderName, &labelsFile);
        }
    }
    
    DEBUGWin32FreeFile(&labelsFile);
    free(subdir);
}


internal void WriteSpaces()
{
    Assets assets_;
    Assets* assets = &assets_;
    InitializeAssets(assets);
    
    BeginAssetType(assets, Asset_emptySpace);
    
    char emptySpacePath[64];
    FormatString(emptySpacePath, sizeof(emptySpacePath), "%s/%s", "definition/equipment", "emptySpace");
    
    PlatformFileGroup fileGroup = Win32GetAllFilesBegin(PlatformFile_image, emptySpacePath);
    Assert(fileGroup.fileCount == 1);
    for(u32 fileIndex = 0; fileIndex < fileGroup.fileCount; ++fileIndex )
    {
        PlatformFileHandle handle = Win32OpenNextFile(&fileGroup, emptySpacePath);
        AddBitmapAsset(emptySpacePath, handle.name);
        
        u8 dimX = 1;
        u8 dimY = 1;
        AddTag(Tag_dimX, dimX);
        AddTag(Tag_dimY, dimY);
    }
    
    Win32GetAllFilesEnd(&fileGroup );
    EndAssetType();
    
    WritePak(assets, "forgemptyspaces.pak" );
}

internal void WriteLeafs()
{
    char* leafPath = "definition/leafs";
    
    Assets assets_;
    Assets* assets = &assets_;
    InitializeAssets(assets);
    
    BeginAssetType(assets, Asset_leaf);
    AddBitmapAsset(leafPath, "leaf.png", 0, 0.5f, 0);
    EndAssetType();
    
    WritePak(assets, "forgleafs.pak" );
}

internal void WriteUI()
{
    char* UIPath = "definition/UI";
    
    Assets assets_;
    Assets* assets = &assets_;
    InitializeAssets(assets);
    
    BeginAssetType(assets, Asset_scrollUI);
    AddBitmapAsset(UIPath, "scrollicon.png", 0, 0.5f, 0.5f);
    EndAssetType();
    
    BeginAssetType(assets, Asset_UISphere);
    AddBitmapAsset(UIPath, "UISphere.png", 0, 0.5f, 0.5f);
    EndAssetType();
    
    BeginAssetType(assets, Asset_UISphereBounds);
    AddBitmapAsset(UIPath, "UISpherebounds.png", 0, 0.5f, 0.5f);
    EndAssetType();
    
    BeginAssetType(assets, Asset_BookPage);
    AddBitmapAsset(UIPath, "bookpage.png");
    EndAssetType();
    
    BeginAssetType(assets, Asset_BookElement);
    AddBitmapAsset(UIPath, "element.png");
    EndAssetType();
    
    BeginAssetType(assets, Asset_Bookmark);
    AddBitmapAsset(UIPath, "bookmark.png");
    EndAssetType();
    
    BeginAssetType(assets, Asset_Icon);
    char* iconsPath = "definition/UI/icons";
    PlatformFileGroup fileGroup = Win32GetAllFilesBegin(PlatformFile_image, iconsPath);
    for(u32 fileIndex = 0; fileIndex < fileGroup.fileCount; ++fileIndex)
    {
        PlatformFileHandle handle = Win32OpenNextFile(&fileGroup, iconsPath);
        char nameWithoutPoint[64];
        GetNameWithoutPoint(nameWithoutPoint, ArrayCount(nameWithoutPoint), handle.name);
        u64 stringHashID = StringHash(nameWithoutPoint);
        AddBitmapAsset(iconsPath, handle.name, stringHashID);
    }
    Win32GetAllFilesEnd(&fileGroup);
    EndAssetType();
    
    WritePak(assets, "forgUI.pak");
}


internal void RecursiveWriteBitmaps(char* path)
{
    PlatformSubdirNames* subdir = (PlatformSubdirNames*) malloc(sizeof(PlatformSubdirNames));
    Win32GetAllSubdirectoriesName(subdir, path);
    
    for(u32 subdirIndex = 0; subdirIndex < subdir->subDirectoryCount; ++subdirIndex)
    {
        char* folderName = subdir->subdirs[subdirIndex];
        char nextPath[256];
        FormatString(nextPath, sizeof(nextPath), "%s/%s", path, folderName);
        
        
        if(!StrEqual(folderName, ".") && !StrEqual(folderName, ".."))
        {
            WriteBitmaps(path, folderName, 0);
            RecursiveWriteBitmaps(nextPath);
        }
    }
    free(subdir);
}

internal void WriteAnimationAutocompleteFile(char* path, char* skeletonName)
{
    char completePath[128];
    FormatString(completePath, sizeof(completePath), "%s/%s/side", path, skeletonName);
    
    char* outputPath = "assets";
    
    char output[128];
    FormatString(output, sizeof(output), "%s/%s.autocomplete", outputPath, skeletonName);
    
    
    char* buffer = (char*) malloc(MegaBytes(2));
    char* writeHere = buffer;
    
    
    PlatformFileGroup animationGroup = Win32GetAllFilesBegin(PlatformFile_animation, completePath);
    for(u32 fileIndex = 0; fileIndex < animationGroup.fileCount; ++fileIndex)
	{
        PlatformFileHandle fileHandle = Win32OpenNextFile(&animationGroup, completePath);
		char* fileName = fileHandle.name;
		u32 animationCount = CountAnimationInFile(completePath, fileName);
		for(u32 animationIndex = 0; animationIndex < animationCount; ++animationIndex)
		{
			char animationName[32];
			GetAnimationName(completePath, fileName, animationIndex, animationName, sizeof(animationName));
			writeHere += sprintf(writeHere, "\"%s\",", animationName);
		}
        Win32CloseHandle(&fileHandle);
    }
    
    Win32GetAllFilesEnd(&animationGroup);
    
    DEBUGWin32WriteFile(output, buffer, StrLen(buffer));
    free(buffer);
}

internal void WriteBitmapsAndAnimations()
{
    char* bitmapPath = "definition/root";
    RecursiveWriteBitmaps(bitmapPath);
    
    
    char* animationPath = "definition/animation";
    PlatformSubdirNames* subdir = (PlatformSubdirNames* ) malloc(sizeof(PlatformSubdirNames ) );
    Win32GetAllSubdirectoriesName(subdir, animationPath);
    
    for(u32 subdirIndex = 0; subdirIndex < subdir->subDirectoryCount; ++subdirIndex)
    {
		char* skeletonName = subdir->subdirs[subdirIndex];
        if(!StrEqual(skeletonName, ".") && !StrEqual(skeletonName, ".."))
        {
            WriteAnimations(animationPath, skeletonName);
            WriteAnimationAutocompleteFile(animationPath, skeletonName);
        }
    }
    free(subdir);
    
    WriteComponents();
    WriteEquipmentMaps();
    WriteSpaces();
    WriteLeafs();
    WriteUI();
}




internal void WriteSounds(PlatformFile labelsFile, char* folder, char* name)
{
    Assets assets_;
    Assets* assets = &assets_;
    InitializeAssets(assets);
    
    char completePath[128];
    FormatString(completePath, sizeof(completePath), "%s/%s", folder, name);
    
    
    char* outputPath = "assets";
    char autocompletePath[128];
    FormatString(autocompletePath, sizeof(autocompletePath), "%s/%s.autocomplete", outputPath, name);
    
    
    char* buffer = (char*) malloc(MegaBytes(2));
    char* writeHere = buffer;
    
    
    u64 hashID = StringHash(name);
    
    u32 hashIndex =  hashID & (HASHED_ASSET_SLOTS - 1);
    u32 assetIndex = Asset_count + hashIndex;
    PlatformFileGroup soundGroup = Win32GetAllFilesBegin(PlatformFile_sound, completePath);
    if(soundGroup.fileCount)
    {
        BeginAssetType(assets, assetIndex);
        for(u32 soundIndex = 0; soundIndex < soundGroup.fileCount; ++soundIndex)
        {
            PlatformFileHandle soundHandle = Win32OpenNextFile(&soundGroup, completePath);
            
            char completeSoundName[256];
            FormatString(completeSoundName, sizeof(completeSoundName), "%s/%s", completePath, soundHandle.name);
            
            
            AddSoundAsset(completeSoundName, hashID);
            //AddLabelsFromFile(labelsFile, soundHandle.name);
            
            writeHere += sprintf(writeHere, "\"%s\",", soundHandle.name);
            
            Win32CloseHandle(&soundHandle);
        }
        EndAssetType();
    }
    Win32GetAllFilesEnd(&soundGroup);
    
    DEBUGWin32WriteFile(autocompletePath, buffer, StrLen(buffer));
    free(buffer);
    
    char pakName[128];
    FormatString(pakName, sizeof(pakName), "%sS.pak", name);
    WritePak(assets, pakName);
}


internal void RecursiveWriteSounds(PlatformFile labelsFile, char* path)
{
    PlatformSubdirNames* subdir = (PlatformSubdirNames* ) malloc(sizeof(PlatformSubdirNames ) );
    Win32GetAllSubdirectoriesName(subdir, path);
    
    for(u32 subdirIndex = 0; subdirIndex < subdir->subDirectoryCount; ++subdirIndex)
    {
        char* folderName = subdir->subdirs[subdirIndex];
        char nextPath[256];
        FormatString(nextPath, sizeof(nextPath), "%s/%s", path, folderName);
        
        
        if(!StrEqual(folderName, ".") && !StrEqual(folderName, ".."))
        {
            WriteSounds(labelsFile, path, folderName);
            RecursiveWriteSounds(labelsFile, nextPath);
        }
    }
    free(subdir);
}

internal void WriteMusic()
{
    Assets assets_;
    Assets* assets = &assets_;
    InitializeAssets(assets );
    
    BeginAssetType(assets, Asset_music);
    
    u32 tenSeconds = 10 * 48000;
    u32 totalSamples = tenSeconds * 10;
    
    SoundId lastMusic = {};
    for(u32 firstSampleIndex = 0; 
        firstSampleIndex < totalSamples; 
        firstSampleIndex += tenSeconds )
    {
        u32 lastSampleIndex = tenSeconds;
        u32 remainingSamples = totalSamples - firstSampleIndex;
        if(remainingSamples < tenSeconds )
        {
            lastSampleIndex = remainingSamples;
        }
        SoundId thisMusic = AddSoundAsset("definition/music/music_test.wav", 0, firstSampleIndex, lastSampleIndex);
        if(lastMusic.value)
        {
            assets->assets[lastMusic.value].sound.chain = Chain_next;
        }
        
        lastMusic = thisMusic;
        
    }
    
    EndAssetType();
    WritePak(assets, "musicS.pak" );
    
}



internal void WriteSounds()
{
    char* assetFile = "sound.fad";
    char* soundPath = "definition/sound";
    char databaseFile[512];
    FormatString(databaseFile, sizeof(databaseFile), "%s/%s", soundPath, assetFile);
    
    OutputFoldersAutocompleteFile("soundType", soundPath);
    
    char* definitionParams = "#cantBeDeleted #playSound";
    WriteAssetDefinitionFile(soundPath, assetFile, definitionParams);
    
    PlatformFile database = DEBUGWin32ReadFile(databaseFile);
    RecursiveWriteSounds(database, soundPath);
    DEBUGWin32FreeFile(&database);
    
    char* eventFile = "soundEvents.fad";
    char* eventPath = "definition/soundEvents";
    char* eventDestPath = "assets";
    
    char eventSourceCompletePath[128];
    FormatString(eventSourceCompletePath, sizeof(eventSourceCompletePath), "%s/%s", eventPath, eventFile);
    char eventDestCompletePath[128];
    FormatString(eventDestCompletePath, sizeof(eventDestCompletePath), "%s/%s", eventDestPath, eventFile);
    
    PlatformFile events = DEBUGWin32ReadFile(eventSourceCompletePath);
    DEBUGWin32WriteFile(eventDestCompletePath, events.content, events.size);
    DEBUGWin32FreeFile(&events);
}


internal void WriteFonts()
{
    Assets assets_;
    Assets* assets = &assets_;
    InitializeAssets(assets );
    
    LoadedFont fonts[2] = {
        LoadFont("c:/windows/fonts/arial.ttf", "Arial", 64 ),
        LoadFont("c:/windows/fonts/courier.ttf", "Courier New", 20 ),
    };
    
    BeginAssetType(assets, Asset_glyph );
    
    for(u32 fontIndex = 0;
        fontIndex < ArrayCount(fonts );
        fontIndex++ )
    {
        for(u32 codePoint = ' ';
            codePoint < '~';
            codePoint++ )
        {
            AddCharacterAsset(fonts + fontIndex, codePoint );
        }
    }
    EndAssetType();
    
    
    BeginAssetType(assets, Asset_font);
    AddFontAsset(fonts + 0);
    AddTag(Tag_fontType, (r32 ) Font_default );
    AddFontAsset(fonts + 1);
    AddTag(Tag_fontType, (r32) Font_debug);
    EndAssetType();
    
    WritePak(assets, "forgivenessF.pak" );
}



int main(int argc, char** argv )
{
    
#if 0    
	DeleteAll("assets", "*.fad");
	DeleteAll("assets", "*.pak");
	DeleteAll("assets", "*.autocomplete");
#endif
    
    WriteMusic();
    WriteSounds();
    WriteFonts();
    WriteBitmapsAndAnimations();
}


