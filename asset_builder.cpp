#define STB_IMAGE_IMPLEMENTATION 1
#include "stb_image.h"

#define PLAYER_VIEW_WIDTH_IN_WORLD_METERS 6.0f
#define DEFAULT_WIDTH 1920

#define USE_FONTS_FROM_WINDOWS 1
#if USE_FONTS_FROM_WINDOWS 
#include <Windows.h>
#else
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#endif

#pragma pack(push, 1)
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

#pragma pack(pop)


global_variable HDC globalFontDC;
global_variable VOID* Bits;

#define FONT_MAX_WIDTH 1024
#define FONT_MAX_HEIGHT 1024

#define MAX_FONT_GLYPHS 0x10ffff

internal LoadedFont LoadFont(char* path, char* filename, char* fontName, int height, u32 startingCodePoint, u32 endingCodePoint)
{
    LoadedFont result = {};
    
    char completePath[128];
    FormatString(completePath, sizeof(completePath), "%s/%s", path, filename);
    
    AddFontResourceExA(completePath, FR_PRIVATE, 0);
    result.win32Font = CreateFontA(height, 0, 0, 0,
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
    
    SelectObject(globalFontDC, result.win32Font );
    GetTextMetrics(globalFontDC, &result.metrics);
    
    result.ascenderHeight = ( r32 ) result.metrics.tmAscent;
    result.descenderHeight = ( r32 ) result.metrics.tmDescent;
    result.externalLeading = ( r32 ) result.metrics.tmExternalLeading;
    
    result.maximumGlyphsCount = 5000;
    result.minCodePoint = INT_MAX;
    result.maxCodePoint = 0;
    result.onePastHighestCodePoint = 0;
    
    u32 glyphsTableSize = sizeof(u32) * result.maximumGlyphsCount;
    result.glyphIndexForCodePoint = (u32*) malloc(glyphsTableSize);
    memset(result.glyphIndexForCodePoint, 0, glyphsTableSize);
    
    result.glyphs = (u32*) malloc(sizeof(u32) * result.maximumGlyphsCount);
    result.glyphs[0] = 0;
    result.glyphsCount = 1;
    
    for(u32 codePoint = startingCodePoint; codePoint <= endingCodePoint; ++codePoint)
    {
        Assert(codePoint < result.maximumGlyphsCount);
        Assert(result.glyphsCount < result.maximumGlyphsCount);
        
        u32 glyphIndex = result.glyphsCount++;
        result.glyphs[glyphIndex] = codePoint;
        result.glyphIndexForCodePoint[codePoint] = glyphIndex;
        
        if(codePoint >= result.onePastHighestCodePoint)
        {
            result.onePastHighestCodePoint = codePoint + 1;
        }
    }
    
    
    u32 kerningTableSize = result.maximumGlyphsCount * result.maximumGlyphsCount * sizeof(r32);
    result.horizontalAdvancement = (r32*) malloc(kerningTableSize);
    memset(result.horizontalAdvancement, 0, kerningTableSize);
    
    SelectObject(globalFontDC, result.win32Font);
    DWORD kerningPairsCount = GetKerningPairsW(globalFontDC, 0, 0);
    KERNINGPAIR* pairs = (KERNINGPAIR*) malloc( sizeof( KERNINGPAIR ) * kerningPairsCount );
    GetKerningPairsW( globalFontDC, kerningPairsCount, pairs );
    for(DWORD pairIndex = 0; pairIndex < kerningPairsCount; pairIndex++)
    {
        KERNINGPAIR* pair = pairs + pairIndex;
        if( pair->wFirst < result.maximumGlyphsCount &&
           pair->wSecond < result.maximumGlyphsCount )
        {
            u32 first = result.glyphIndexForCodePoint[pair->wFirst];
            u32 second = result.glyphIndexForCodePoint[pair->wSecond];
            
            if(first && second)
            {
                result.horizontalAdvancement[first * result.maximumGlyphsCount + second] += ( r32 ) pair->iKernAmount;
            }
        }
    }
    free(pairs);
    
    return result;
}


internal LoadedBitmap LoadGlyph(LoadedFont* font, u32 codePoint)
{
    LoadedBitmap result = {};
    
    u32 glyphIndex = font->glyphIndexForCodePoint[codePoint];
#if USE_FONTS_FROM_WINDOWS
    
    int MaxWidth = FONT_MAX_WIDTH;
    int MaxHeight = FONT_MAX_HEIGHT;
    
    SelectObject(globalFontDC, font->win32Font);
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
        
        result.pivot.x = 1.0f / (r32)result.width;
        result.pivot.y = (1.0f + (MaxY - (BoundHeight - font->metrics.tmDescent))) / (r32)result.height;
        
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
        
        if(otherGlyphIndex != 0)
        {
            font->horizontalAdvancement[otherGlyphIndex * font->maximumGlyphsCount + glyphIndex] += kerningChange;
        }
    }
#else
    EntireFile fontFile = ReadFile(filename);
    
    stbtt_fontinfo font;
    stbtt_InitFont( &font, ( u8* ) fontFile.content, stbtt_GetFontOffsetForIndex( ( u8* ) fontFile.content, 0));
    
    
    int width, height, XOffset, YOffset;
    u8* bitmap = stbtt_GetCodepointBitmap( &font, 0, stbtt_ScaleForPixelHeight( &font, 40.0f ), 
                                          codePoint, &width, &height, &XOffset, &YOffset );
    
    
    result.bitmap.pixels = malloc( sizeof( u32 ) * width * height );
    result.free = result.bitmap.pixels;
    result.bitmap.width = (i16) width;
    result.bitmap.height = (i16) height;
    
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

internal b32 EditorFile(PlatformFileInfo* file)
{
    b32 result = StrEqual(StrLen(TEST_FILE_PREFIX), file->name, TEST_FILE_PREFIX);
    return result;
}

internal b32 RelevantFile(PlatformFileInfo* file, PlatformFileGroup* group, b32 invalidWhenNotFound)
{
	b32 result = true;
    if(!EditorFile(file))
    {
        for(PlatformFileInfo* test = group->firstFileInfo; test; test = test->next)
        {
            if(EditorFile(test))
            {
                Assert(StrLen(test->name) > StrLen(TEST_FILE_PREFIX));
                char* name = file->name;
                char* testName = test->name + StrLen(TEST_FILE_PREFIX);
                
                if(StrEqual(testName, name))
                {
                    if(TimestampIsMoreRecent(test->timestamp, file->timestamp))
                    {
                        result = false;
                    }
                    break;
                }
            }
        }
    }
    else
    {
        b32 foundStandardFile = false;
        for(PlatformFileInfo* test = group->firstFileInfo; test; test = test->next)
        {
            if(!EditorFile(test))
            {
                Assert(StrLen(file->name) > StrLen(TEST_FILE_PREFIX));
                char* name = file->name + StrLen(TEST_FILE_PREFIX);
                char* testName = test->name;
                
                if(StrEqual(testName, name))
                {
                    foundStandardFile = true;
                    if(TimestampIsMoreRecent(test->timestamp, file->timestamp))
                    {
                        result = false;
                    }
                    break;
                }
            }
        }
        
        if(invalidWhenNotFound && !foundStandardFile)
        {
            result = false;
        }
    }
	
    return result;
}

internal u8* ReadEntireFile(MemoryPool* pool, PlatformFileGroup* group, PlatformFileInfo* info)
{
    PlatformFileHandle handle = platformAPI.OpenFile(group, info);
    u8* result = PushSize(pool, info->size, NoClear());
    platformAPI.ReadFromFile(&handle, 0, info->size, result);
    result[info->size] = 0;
    platformAPI.CloseFile(&handle);
    return result;
}

internal void ParseAttachmentPoint(PAKAttachmentPoint* point, Tokenizer* tokenizer)
{
    if(RequireToken(tokenizer, Token_OpenBraces))
    {
        Token t = GetToken(tokenizer);
        Assert(t.type == Token_String);
        t = Stringize(t);
        FormatString(point->name, sizeof(point->name), "%.*s", t.textLength, t.text);
        
        Token alignX = GetToken(tokenizer);
        Token alignY = GetToken(tokenizer);
        Token angle = GetToken(tokenizer);
        Token scaleX = GetToken(tokenizer);
        Token scaleY = GetToken(tokenizer);
        Token zOffset = GetToken(tokenizer);
        
        point->alignment.x = StringToR32(alignX.text);
        point->alignment.y = StringToR32(alignY.text);
        point->angle = StringToR32(angle.text);
        point->scale.x = StringToR32(scaleX.text);
        point->scale.y = StringToR32(scaleY.text);
        point->zOffset = StringToR32(zOffset.text);
        
        while(t.type != Token_CloseBraces && t.type != Token_EndOfFile && t.type != Token_SemiColon)
        {
            t = GetToken(tokenizer);
        }
    }
}

#define MAX_ATTACHMENT_POINT 64
internal void FillAttachmentPoints(LoadedBitmap* bitmap, char* name, MemoryPool* tempPool, PlatformFileGroup* markupFiles)
{
    u32 runningPointIndex = 0;
    bitmap->attachmentPointCount = MAX_ATTACHMENT_POINT;
    u32 attachmentTotalSize = sizeof(PAKAttachmentPoint) * MAX_ATTACHMENT_POINT;
    bitmap->attachmentPoints = (PAKAttachmentPoint*) malloc(attachmentTotalSize);
    memset(bitmap->attachmentPoints, 0, attachmentTotalSize);
    
    for(PlatformFileInfo* info = markupFiles->firstFileInfo; info; info = info->next)
    {
        if(RelevantFile(info, markupFiles, false))
        {
            u8* fileContent = ReadEntireFile(tempPool, markupFiles, info);
            Tokenizer tokenizer = {};
            tokenizer.at = (char*) fileContent;
            Token t = AdvanceToToken(&tokenizer, name);
            
            if(t.type != Token_EndOfFile)
            {
                if(RequireToken(&tokenizer, Token_Colon))
                {
                    while(true)
                    {
                        Token p = GetToken(&tokenizer);
                        
                        if(p.type == Token_Comma)
                        {
                        }
                        else if(p.type == Token_EndOfFile)
                        {
                            break;
                        }
                        else if(p.type == Token_SemiColon)
                        {
                        }
                        else
                        {
                            Token propertyName = p;
                            
                            if(TokenEquals(propertyName, IMAGE_ATTACHMENT_POINT))
                            {
                                if(RequireToken(&tokenizer, Token_EqualSign))
                                {
                                    if(runningPointIndex < MAX_ATTACHMENT_POINT)
                                    {
                                        PAKAttachmentPoint* point = bitmap->attachmentPoints + runningPointIndex++;
                                        ParseAttachmentPoint(point, &tokenizer);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

internal void ParseGroupName(PAKGroupName* group, Tokenizer* tokenizer)
{
    if(RequireToken(tokenizer, Token_OpenBraces))
    {
        Token t = GetToken(tokenizer);
        Assert(t.type == Token_String);
        t = Stringize(t);
        FormatString(group->name, sizeof(group->name), "%.*s", t.textLength, t.text);
        
        while(t.type != Token_CloseBraces && t.type != Token_EndOfFile && t.type != Token_SemiColon)
        {
            t = GetToken(tokenizer);
        }
    }
}

#define MAX_GROUP_NAMES 16
internal void FillGroupNames(LoadedBitmap* bitmap, char* name, MemoryPool* tempPool, PlatformFileGroup* markupFiles)
{
    u32 runningGroupIndex = 0;
    bitmap->groupNameCount = MAX_GROUP_NAMES;
    u32 groupNameTotalSize = sizeof(PAKGroupName) * MAX_GROUP_NAMES;
    bitmap->groupNames = (PAKGroupName*) malloc(groupNameTotalSize);
    memset(bitmap->groupNames, 0, groupNameTotalSize);
    
    for(PlatformFileInfo* info = markupFiles->firstFileInfo; info; info = info->next)
    {
        if(RelevantFile(info, markupFiles, false))
        {
            u8* fileContent = ReadEntireFile(tempPool, markupFiles, info);
            Tokenizer tokenizer = {};
            tokenizer.at = (char*) fileContent;
            Token t = AdvanceToToken(&tokenizer, name);
            
            if(t.type != Token_EndOfFile)
            {
                if(RequireToken(&tokenizer, Token_Colon))
                {
                    while(true)
                    {
                        Token p = GetToken(&tokenizer);
                        
                        if(p.type == Token_Comma)
                        {
                        }
                        else if(p.type == Token_EndOfFile)
                        {
                            break;
                        }
                        else if(p.type == Token_SemiColon)
                        {
                        }
                        else
                        {
                            Token propertyName = p;
                            if(TokenEquals(propertyName, IMAGE_GROUP_NAME))
                            {
                                if(RequireToken(&tokenizer, Token_EqualSign))
                                {
                                    if(runningGroupIndex < MAX_GROUP_NAMES)
                                    {
                                        PAKGroupName* group = bitmap->groupNames + runningGroupIndex++;
                                        ParseGroupName(group, &tokenizer);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

internal LoadedBitmap LoadImage(MemoryPool* tempPool, char* path, char* filename, PlatformFileGroup* markupFiles)
{
    LoadedBitmap result = {};
    char completeName[256];
    sprintf(completeName, "%s/%s", path, filename);
    
    FillAttachmentPoints(&result, filename, tempPool, markupFiles);
    FillGroupNames(&result, filename, tempPool, markupFiles);
#if STB_IMAGE_IMPLEMENTATION
    int x;
    int y;
    int n;
    stbi_set_flip_vertically_on_load(true);
    unsigned char *data = stbi_load(completeName, &x, &y, &n, 0);
    Assert( n == 4 );
    if( data )
    {
        u32* source = (u32*) data;
        for( i32 Y = 0; Y < y; ++Y )
        {
            for( i32 X = 0; X < x; X++ )
            {
                u32* sourcePixel = source + (Y * x) + X;
                Vec4 texel = V4( ( r32 ) ( *sourcePixel >> 0 &0xff ),
                                ( r32 ) ( *sourcePixel >> 8 & 0xff ),
                                ( r32 ) ( *sourcePixel >> 16 & 0xff ),
                                ( r32 ) ( *sourcePixel >> 24 & 0xff ) );
                
                texel = SRGB255ToLinear1(texel);
                texel.rgb *= texel.a;
                texel = Linear1ToSRGB255(texel);
                
                *sourcePixel = ((u32) texel.a << 24 | 
                                (u32) texel.r << 16 |
                                (u32) texel.g << 8 |
                                (u32) texel.b << 0);
            }
        }
        
        result.pixels = data;
        result.free = result.pixels;
        
        result.width = SafeTruncateToU16(x);
        result.height = SafeTruncateToU16(y);
        result.widthOverHeight = (r32 ) x / (r32 ) y;
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

internal b32 IsColorationFile(char* filename)
{
    b32 result = false;
    
    u32 point = FindFirstInString(filename, '.');
    char* extension = filename + point + 1;
    
    result = (StrEqual(extension, "color"));
    return result;
}

internal LoadedColoration LoadColoration(char* fileContent)
{
    LoadedColoration result = {};
    result.color = V4(1, 1, 1, 1);
    
    Tokenizer tokenizer = {};
    tokenizer.at = fileContent;
    
    
    b32 parsing = true;
    
    b32 parsedR = false;
    b32 parsedG = false;
    b32 parsedB = false;
    
    while(parsing)
    {
        Token t = GetToken(&tokenizer);
        
        switch(t.type)
        {
            case Token_EndOfFile:
            {
                parsing = false;
            } break;
            
            case Token_String:
            {
                t = Stringize(t);
            }
            case Token_Identifier:
            {
                FormatString(result.imageName, sizeof(result.imageName), "%.*s", t.textLength, t.text);
            } break;
            
            case Token_Number:
            
            {
                if(!parsedR)
                {
                    parsedR = true;
                    result.color.r = StringToR32(t.text);
                }
                else if(!parsedG)
                {
                    parsedG = true;
                    result.color.g = StringToR32(t.text);
                }
                else if(!parsedB)
                {
                    parsedB = true;
                    result.color.b = StringToR32(t.text);
                }
                else
                {
                    result.color.a = StringToR32(t.text);
                }
            } break;
        }
    }
    
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


internal LoadedSound LoadWAV(char* fileContent)
{
    LoadedSound result = {};
    
    Assert(fileContent);
    
    WAVEHeader* header = (WAVEHeader*) fileContent;
    Assert(header->id == WAVE_IDriff);
    Assert(header->waveID == WAVE_IDwave);
    
    void* samplesLeft = 0;
    void* samplesRight = 0;
    
    for(RiffIter iter = ParseChunkAt(header + 1, ( u8* ) ( header + 1) + header->size - 4 ); IsValid( iter ); iter = NextChunk(iter))
    {
        switch(GetType(iter))
        {
            case WAVE_IDfmt:
            {
                WAVEFormat* format = ( WAVEFormat* ) GetChunkData(iter); 
                
                Assert(format->format == 1 );
                Assert(format->blocksPerSec = 48000);
                Assert(format->bitsPerSample == 16 );
                Assert(format->blockSize == format->channels * 2);
                
                result.countChannels = format->channels;
            } break;
            
            case WAVE_IDdata:
            {
                u32 sampleDataSize = GetChunkDataSize(iter);
                u32 sampleCount = sampleDataSize / (result.countChannels * sizeof(i16));
                i16* samples = ( i16*) GetChunkData(iter);
                
                if(result.countChannels == 1)
                {
                    result.samples[0] = samples;
                    result.samples[1] = 0;
                }
                else
                {
                    // TODO( Leonardo ): stereo!
                    Assert(result.countChannels == 2);
                    
                    result.samples[0] = samples;
                    result.samples[1] = samples + sampleCount;
                    
                    for(u32 sampleIndex = 0;
                        sampleIndex < sampleCount;
                        ++sampleIndex)
                    {
                        i16 source = samples[2*sampleIndex];
                        samples[2*sampleIndex] = samples[sampleIndex];
                        samples[sampleIndex] = source;
                    }
                }
                
                i16 maxSampleValue = I16_MIN;
                for( u32 sampleIndex = 0; sampleIndex < sampleCount * result.countChannels; sampleIndex++ )
                {
                    i16 sampleValue = samples[sampleIndex];
                    
                    if(sampleValue > maxSampleValue)
                    {
                        maxSampleValue = sampleValue;
                    }
                }
                
                result.countChannels = 1;
                Assert(maxSampleValue > 0);
                result.maxSampleValue = maxSampleValue;
                result.decibelLevel = 20 * Log10((r32) maxSampleValue / (r32) I16_MAX);
                
                
                result.countSamples = sampleCount;
            } break;
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
    StrCopy( source, countChar, dest->name, 
            ArrayCount( dest->name ) );
    source = ++testChar;
    countChar = 0;
    while( *testChar++ != '\"' )
    {
        countChar++;
    }
    StrCopy( source, countChar, dest->value, 
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
    
    StrCopy( source, countChar, dest->title, ArrayCount( dest->title ) );
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


internal void ParseSoundTrigger(PAKAnimationSoundTrigger* trigger, Tokenizer* tokenizer)
{
    if(RequireToken(tokenizer, Token_OpenBraces))
    {
        AdvanceToNextToken(tokenizer, Token_EqualSign);
        trigger->property = Parse_GameProperty(tokenizer, {});
        AdvanceToNextToken(tokenizer, Token_EqualSign);
        trigger->timeline = Parse_u32(tokenizer, 0);
    }
}

#define MAX_SOUND_TRIGGERS 16
internal void FillSoundTriggers(LoadedAnimation* animation, MemoryPool* tempPool, PlatformFileGroup* markupFiles)
{
    char name[64];
    FormatString(name, sizeof(name), "animations_%s", animation->name);
    u32 runningTriggerIndex = 0;
    animation->triggerCount = MAX_GROUP_NAMES;
    u32 triggersTotalSize = sizeof(PAKAnimationSoundTrigger) * MAX_SOUND_TRIGGERS;
    animation->triggers = (PAKAnimationSoundTrigger*) malloc(triggersTotalSize);
    memset(animation->triggers, 0, triggersTotalSize);
    
    for(PlatformFileInfo* info = markupFiles->firstFileInfo; info; info = info->next)
    {
        if(RelevantFile(info, markupFiles, false))
        {
            u8* fileContent = ReadEntireFile(tempPool, markupFiles, info);
            Tokenizer tokenizer = {};
            tokenizer.at = (char*) fileContent;
            Token t = AdvanceToToken(&tokenizer, name);
            
            if(t.type != Token_EndOfFile)
            {
                if(RequireToken(&tokenizer, Token_Colon))
                {
                    while(true)
                    {
                        Token p = GetToken(&tokenizer);
                        
                        if(p.type == Token_Comma)
                        {
                        }
                        else if(p.type == Token_EndOfFile)
                        {
                            break;
                        }
                        else if(p.type == Token_SemiColon)
                        {
                        }
                        else
                        {
                            Token propertyName = p;
                            if(TokenEquals(propertyName, ANIMATION_SOUND_TRIGGER))
                            {
                                if(RequireToken(&tokenizer, Token_EqualSign))
                                {
                                    if(runningTriggerIndex < MAX_SOUND_TRIGGERS)
                                    {
                                        PAKAnimationSoundTrigger* trigger = animation->triggers + runningTriggerIndex++;
                                        ParseSoundTrigger(trigger, &tokenizer);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

internal LoadedAnimation LoadAnimation(MemoryPool* tempPool, char* fileContent, u16 animationIndex, PlatformFileGroup* markupFiles)
{
    LoadedAnimation result = {};
    
    u32 maxFrameCount = 64;
    TempFrame* frames = (TempFrame*) malloc(maxFrameCount * sizeof(TempFrame));
    memset(frames, 0, maxFrameCount * sizeof(TempFrame));
    
    result.frames = (FrameData*) malloc(maxFrameCount * sizeof(FrameData));
    
    u32 maxBoneCount = 1024;
    u32 maxAssCount = 1024;
    u32 maxSpriteCount = 64;
    
    result.bones = (Bone* ) malloc(maxBoneCount * sizeof(Bone));
    memset(result.bones, 0, maxBoneCount * sizeof(Bone));
    result.ass = (PieceAss* ) malloc(maxAssCount * sizeof(PieceAss));
    memset(result.ass, 0, maxAssCount * sizeof(PieceAss));
    result.spriteInfos = (SpriteInfo* ) malloc(maxSpriteCount * sizeof(SpriteInfo));
    memset(result.spriteInfos, 0, maxSpriteCount * sizeof(SpriteInfo));
    
    
    
    SpriteInfo tempSprites[64] = {};
    i32 currentFolderID = 0;
    u32 folderSpriteCount[8] = {};
    u32* currentfolderSpriteCounter = 0;
    u32 tempSpriteCount = 0;
    u32 definitiveSpriteCount = 0;
    u16 currentIndex = 0;
    
    
    Assert(fileContent);
    {
        // TODO(Leonardo ): robustness!
        i32 startOffset = 134;
        char* start = (char* ) fileContent + startOffset;
        
        b32 mainLineActive = false;
        b32 loading = false;
        b32 ended = false;
        b32 keyiedOnFirst = false;
        i32 timeLineActive = 0;
        char* activeTimelineName = 0;
        
        u32 frameCount = 0;
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
                    
                    char* pieceName = GetXMLValues(&currentTag, "name");
                    Assert(pieceName);
                    
                    char* pieceNameReal = AdvanceToLastSlash(pieceName);
                    TrimToFirstCharacter(tempSprite->name, sizeof(tempSprite->name), pieceNameReal, '.');
                    tempSprite->nameHash = StringHash(tempSprite->name);
                    
                    Vec2 pivot;
                    GetXMLValuef(&currentTag, "pivot_x", &pivot.x );
                    GetXMLValuef(&currentTag, "pivot_y", &pivot.y );
                    tempSprite->pivot = pivot;
                    tempSprite->placeHolder = (pieceName != pieceNameReal);
                    
                    i32 height;
                    GetXMLValuei(&currentTag, "height", &height);
                    tempSprite->height = height * (PLAYER_VIEW_WIDTH_IN_WORLD_METERS / DEFAULT_WIDTH);
                }
                else if(StrEqual(currentTag.title, "animation"))
                {
                    i32 length = 0; 
                    GetXMLValuei(&currentTag, "length", &length );
                    char* name = GetXMLValues(&currentTag, "name");
                    Assert(name);
                    
                    
                    char* looping = GetXMLValues(&currentTag, "looping");
                    //result.singleCycle = StrEqual(looping, "false");
                    
                    if(animationIndex == currentIndex++)
                    {
                        FormatString(result.name, sizeof(result.name), "%s", name);
                        loading = true;
                        result.durationMS = SafeTruncateToU16(length);
                        Assert(result.durationMS);
                        
                        u32 animationNameLength = 0;
                        char* animationNameTest = name;
                        while(*animationNameTest)
                        {
                            if(*animationNameTest++ == '_')
                            {
                                break;
                            }
                            
                            ++animationNameLength;
                        }
                        
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
                        Assert(frameCount < maxFrameCount);
                        tempFrame = frames + frameCount++;
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
                        for(u32 frameIndex = 0; frameIndex < frameCount; ++frameIndex )
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
                    GetXMLValuei(&currentTag, "timeline", &timeLineIndex );
                    
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
                    GetXMLValuei(&currentTag, "parent", &parentID );
                    Assert(parentID < tempFrame->countBones ); 
                    i32 timeLineIndex = 0;
                    GetXMLValuei(&currentTag, "timeline", &timeLineIndex );
                    i32 zIndex = 0;
                    GetXMLValuei(&currentTag, "z_index", &zIndex );
                    
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
                    GetXMLValuei(&currentTag, "id", &timeLineActive );
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
                    bone->parentOffset = V2(x, y ) * (PLAYER_VIEW_WIDTH_IN_WORLD_METERS / DEFAULT_WIDTH);
                    bone->parentAngle = angle;
                    bone->spin = currentSpin;
                    bone->id = boneID;
                    bone->parentIndex = parentID;
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
                    ass->boneIndex = boneID;
                    
                    i32 fileIndex = 0;
                    GetXMLValuei(&currentTag, "file", &fileIndex);
                    i32 folderIndex = 0;
                    GetXMLValuei(&currentTag, "folder", &folderIndex );
                    
                    
                    
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
                        Assert(definitiveSpriteCount < maxSpriteCount);
                        definitiveSpriteIndex = definitiveSpriteCount++;
                        SpriteInfo* dest = result.spriteInfos + definitiveSpriteIndex;
                        *dest = *source;
                    }
                    
                    
                    ass->boneOffset = V2(x, y )  * (PLAYER_VIEW_WIDTH_IN_WORLD_METERS / DEFAULT_WIDTH);
                    ass->additionalZOffset = 0;
                    ass->angle = angle;
                    ass->scale = V2(scaleX, scaleY );
                    ass->color = V4(1, 1, 1, alpha);
                    ass->spin = currentSpin;
                    ass->timeLineIndex = timeLineActive;
                    ass->spriteIndex = definitiveSpriteIndex;
                }
            }
        }
        
    }
    
    u32 boneIndexToCopy = 0;
    u32 assIndexToCopy = 0;
    
    result.spriteInfoCount = definitiveSpriteCount;
    for(u32 frameIndex = 0; frameIndex < result.frameCount; frameIndex++ )
    {
        TempFrame* source = frames + frameIndex;
        FrameData* dest = result.frames + frameIndex;
        
        dest->timelineMS = SafeTruncateToU16(source->timeLine );
        dest->firstBoneIndex = boneIndexToCopy;
        dest->firstAssIndex = assIndexToCopy;
        dest->countBones = 0;
        dest->countAss = 0;
        
        for(i32 boneIndex = 0; boneIndex < source->countBones; boneIndex++ )
        {
            Assert(boneIndexToCopy < maxBoneCount);
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
            Assert(assIndexToCopy < maxAssCount);
            PieceAss* sourceAss = source->ass + assIndex;
            if(sourceAss->timeLineIndex > 0 )
            {
                PieceAss* destAss = result.ass + assIndexToCopy++;
                *destAss = *sourceAss;
                dest->countAss++;;
            }
        }
    }
    
    free(frames);
    Assert(result.durationMS);
    
    FillSoundTriggers(&result, tempPool, markupFiles);
    return result;
}

internal u16 CountAnimations(char* fileContent)
{
    u16 result = 0;
    
    Assert(fileContent);
    // TODO(Leonardo ): robustness!
    i32 startOffset = 134;
    char* start = (char* ) fileContent + startOffset;
    
    while(start != 0)
    {
        XMLTag currentTag = {};
        start = ReadXMLTag(start, &currentTag);
        
        if(StrEqual(currentTag.title, "animation"))
        {
            ++result;
        }
    }
    return result;
}


struct FaceVertexData
{
    u16 vertexIndex;
};

inline FaceVertexData ParseFaceVertexData(Tokenizer* tokenizer)
{
    FaceVertexData result = {};
    Token v = GetToken(tokenizer);
    
    if(v.type == Token_Number)
    {
        result.vertexIndex = (u16) (atoi(v.text) - 1);
    }
    
    if(NextTokenIs(tokenizer, Token_Slash))
    {
        Token s0 = GetToken(tokenizer);
        Token textureIndex = GetToken(tokenizer);
        
        
        if(NextTokenIs(tokenizer, Token_Slash))
        {
            Token s1 = GetToken(tokenizer);
            Token normalIndex = GetToken(tokenizer);
        }
    }
    
    return result;
}

internal LoadedModel LoadModel(char* fileContent)
{
    LoadedModel result = {};
    u32 maxVertexCount = U16_MAX;
    u32 maxFaceCount = U16_MAX;
    
    result.vertexes = (ColoredVertex*) malloc(maxVertexCount * sizeof(ColoredVertex));
    result.faces = (ModelFace*) malloc(maxFaceCount * sizeof(ModelFace));
    
    Vec3 min = V3(R32_MAX, R32_MAX, R32_MAX);
    Vec3 max = V3(R32_MIN, R32_MIN, R32_MIN);
    
    Assert(fileContent);
    Tokenizer tokenizer = {};
    tokenizer.at = (char*) fileContent;
    
    b32 parsing = true;
    while(parsing)
    {
        Token t = GetToken(&tokenizer);
        switch(t.type)
        {
            case Token_Identifier:
            {
                if(TokenEquals(t, "v"))
                {
                    Assert(result.vertexCount < maxVertexCount);
                    ColoredVertex* dest = result.vertexes + result.vertexCount++;
                    
                    Token x = GetToken(&tokenizer);
                    Token y = GetToken(&tokenizer);
                    Token z = GetToken(&tokenizer);
                    
                    if(x.type == Token_Number && y.type == Token_Number && z.type == Token_Number)
                    {
                        dest->P.x = (r32) R32FromChar(x.text);
                        dest->P.y = (r32) R32FromChar(y.text);
                        dest->P.z = (r32) R32FromChar(z.text);
                        
                        dest->N = {};
                        
                        if(dest->P.x > max.x)
                        {
                            max.x = dest->P.x;
                        }
                        else if(dest->P.x < min.x)
                        {
                            min.x = dest->P.x;
                        }
                        
                        if(dest->P.y > max.y)
                        {
                            max.y = dest->P.y;
                        }
                        else if(dest->P.y < min.y)
                        {
                            min.y = dest->P.y;
                        }
                        
                        if(dest->P.z > max.z)
                        {
                            max.z = dest->P.z;
                        }
                        else if(dest->P.z < min.z)
                        {
                            min.z = dest->P.z;
                        }
                    }
                    else
                    {
                        InvalidCodePath;
                    }
                    
                    dest->color = V4(1, 1, 1, 1);
                }
                else if(TokenEquals(t, "f"))
                {
                    Assert(result.faceCount < maxFaceCount);
                    ModelFace* dest = result.faces + result.faceCount++;
                    
                    FaceVertexData d0 = ParseFaceVertexData(&tokenizer);
                    FaceVertexData d1 = ParseFaceVertexData(&tokenizer);
                    FaceVertexData d2 = ParseFaceVertexData(&tokenizer);
                    
                    dest->i0 = d0.vertexIndex;
                    dest->i1 = d1.vertexIndex;
                    dest->i2 = d2.vertexIndex;
                    
                }
            } break;
            
            case Token_EndOfFile:
            {
                parsing = false;
            } break;
        }
    }
    
    result.dim = max - min;
    return result;
}


internal void AddProperty(PAKAsset* asset, u64 propertyHash, u64 valueHash)
{
    b32 found = false;
    for(u32 propertyIndex = 0; propertyIndex < ArrayCount(asset->runtime); ++propertyIndex)
    {
        if(!asset->propertyHash[propertyIndex])
        {
            asset->propertyHash[propertyIndex] = propertyHash;
            asset->valueHash[propertyIndex] = valueHash;
            found = true;
            break;
        }
    }
    
    Assert(found);
}

internal void FillPAKProperty(PAKAsset* asset, Token property, Token value)
{
    if(TokenEquals(property, ANIMATION_PROPERTY_PING_PONG))
    {
        asset->animation.pingPongLooping = StringToB32(value.text);
    }
    else if(TokenEquals(property, ANIMATION_PROPERTY_SINGLE_CYCLE))
    {
        asset->animation.singleCycle = StringToB32(value.text);
    }
    else if(TokenEquals(property, ANIMATION_PROPERTY_LOOPING_BASELINE))
    {
        asset->animation.loopingBaselineMS = SafeTruncateToU16(StringToUInt32(value.text));
    }
    else if(TokenEquals(property, IMAGE_PROPERTY_ALIGN_X))
    {
        asset->bitmap.align[0] = (r32) StringToR32(value.text);
    }
    else if(TokenEquals(property, IMAGE_PROPERTY_ALIGN_Y))
    {
        asset->bitmap.align[1] = (r32) StringToR32(value.text);
    }
    else if(TokenEquals(property, IMAGE_PROPERTY_ALPHA_THREESOLD))
    {
        asset->bitmap.alphaThreesold = (r32) StringToR32(value.text);
    }
    else if(TokenEquals(property, IMAGE_PROPERTY_FLIPPED_BY_DEFAULT))
    {
        asset->bitmap.flippedByDefault = (b32) StringToB32(value.text);
    }
    else if(TokenEquals(property, SKELETON_FLIPPED))
    {
        asset->skeleton.flippedByDefault = (b32) StringToB32(value.text);
    }
    else if(TokenEquals(property, SKELETON_FLIPPED_BONE1))
    {
        asset->skeleton.flippedBone1 = StringToUInt32(value.text);
    }
    else if(TokenEquals(property, SKELETON_FLIPPED_BONE1_OFFSET))
    {
        asset->skeleton.flippedBone1ZOffset = StringToR32(value.text);
    }
    else if(TokenEquals(property, SKELETON_FLIPPED_BONE2))
    {
        asset->skeleton.flippedBone2 = StringToUInt32(value.text);
    }
    else if(TokenEquals(property, SKELETON_FLIPPED_BONE2_OFFSET))
    {
        asset->skeleton.flippedBone2ZOffset = StringToR32(value.text);
    }
    else
    {
        u64 propertyHash = StringHash(property.text, property.textLength);
        u64 valueHash = StringHash(value.text, value.textLength);
        
        Assert(propertyHash && valueHash);
        AddProperty(asset, propertyHash, valueHash);
    }
}


internal SavedTypeSubtypeCountHash* AddFileCountHash(TimestampHash* hash, char* typeSubtype, u32 fileCount, u32 markupCount)
{
    Assert(IsPowerOf2(ArrayCount(hash->hashSlots)));
    u64 hashRaw = StringHash(typeSubtype);
    u32 slotIndex = hashRaw & (ArrayCount(hash->countHashSlots) - 1);
    
    SavedTypeSubtypeCountHash* newHash = PushStruct(&hash->pool, SavedTypeSubtypeCountHash);
    FormatString(newHash->typeSubtype, sizeof(newHash->typeSubtype), "%s", typeSubtype);
    newHash->fileCount = fileCount;
    newHash->markupCount = markupCount;
    
    FREELIST_INSERT(newHash, hash->countHashSlots[slotIndex]);
    
    return newHash;
}

internal SavedTypeSubtypeCountHash* GetCorrenspodingFileCountHash(TimestampHash* hash, char* type, char* subtype)
{
    char typeSubtype[256];
    FormatString(typeSubtype, sizeof(typeSubtype), "%s_%s", type, subtype);
    
    Assert(IsPowerOf2(ArrayCount(hash->hashSlots)));
    u64 hashRaw = StringHash(typeSubtype);
    u32 slotIndex = hashRaw & (ArrayCount(hash->countHashSlots) - 1);
    
    
    SavedTypeSubtypeCountHash* result = 0;
    for(SavedTypeSubtypeCountHash* test = hash->countHashSlots[slotIndex]; test; test = test->next)
    {
        if(StrEqual(test->typeSubtype, typeSubtype))
        {
            result = test;
            break;
        }
    }
    
    if(!result)
    {
        result = AddFileCountHash(hash, typeSubtype, 0, 0);
    }
    return result;
}

internal void SaveFileCountHash(SavedTypeSubtypeCountHash* info, u32 fileCount, u32 markupCount)
{
    info->fileCount = fileCount;
    info->markupCount = markupCount;
    
    u8* fileContent = (u8*) info;
    u32 fileSize = sizeof(SavedTypeSubtypeCountHash);
    
    platformAPI.ReplaceFile(PlatformFile_timestamp, TIMESTAMP_PATH, info->typeSubtype, fileContent, fileSize, 0);
}

internal SavedFileInfoHash* AddFileDateHash(TimestampHash* hash, char* pathAndName, PlatformFileTimestamp timestamp)
{
    Assert(IsPowerOf2(ArrayCount(hash->hashSlots)));
    u64 hashRaw = StringHash(pathAndName);
    u32 slotIndex = hashRaw & (ArrayCount(hash->hashSlots) - 1);
    
    SavedFileInfoHash* newHash = PushStruct(&hash->pool, SavedFileInfoHash);
    FormatString(newHash->pathAndName, sizeof(newHash->pathAndName), "%s", pathAndName);
    ReplaceAll(newHash->pathAndName, '.', '_');
    ReplaceAll(newHash->pathAndName, '/', '_');
    
    newHash->timestamp = timestamp;
    
    FREELIST_INSERT(newHash, hash->hashSlots[slotIndex]);
    
    return newHash;
}

internal SavedFileInfoHash* GetCorrenspodingFileDateHash(TimestampHash* hash, char* path, char* name)
{
    char pathAndName[256];
    FormatString(pathAndName, sizeof(pathAndName), "%s/%s", path, name);
    ReplaceAll(pathAndName, '.', '_');
    ReplaceAll(pathAndName, '/', '_');
    
    Assert(IsPowerOf2(ArrayCount(hash->hashSlots)));
    u64 hashRaw = StringHash(pathAndName);
    u32 slotIndex = hashRaw & (ArrayCount(hash->hashSlots) - 1);
    
    
    SavedFileInfoHash* result = 0;
    for(SavedFileInfoHash* test = hash->hashSlots[slotIndex]; test; test = test->next)
    {
        if(StrEqual(test->pathAndName, pathAndName))
        {
            result = test;
            break;
        }
    }
    
    if(!result)
    {
        result = AddFileDateHash(hash, pathAndName, {});
    }
    return result;
}

internal void SaveFileDateHash(SavedFileInfoHash* info, PlatformFileTimestamp timestamp)
{
    info->timestamp = timestamp;
    
    u8* fileContent = (u8*) info;
    u32 fileSize = sizeof(SavedFileInfoHash);
    platformAPI.ReplaceFile(PlatformFile_timestamp, TIMESTAMP_PATH, info->pathAndName, fileContent, fileSize, 0);
}


internal void FillPAKAssetBaseInfo(FILE* out, MemoryPool* tempPool, PAKAsset* asset, char* name, PlatformFileGroup* markupFiles)
{
    if(StrEqual(StrLen(TEST_FILE_PREFIX), TEST_FILE_PREFIX, name))
    {
        name += StrLen(TEST_FILE_PREFIX);
    }
    FormatString(asset->sourceName, sizeof(asset->sourceName), "%s", name);
    asset->dataOffset = ftell(out);
    for(u32 propertyIndex = 0; propertyIndex < MAX_PROPERTIES_PER_ASSET; ++propertyIndex)
    {
        asset->propertyHash[propertyIndex] = 0;
        asset->valueHash[propertyIndex] = 0;
        asset->runtime[propertyIndex] = {};
    }
    
    for(PlatformFileInfo* info = markupFiles->firstFileInfo; info; info = info->next)
    {
        if(RelevantFile(info, markupFiles, false))
        {
            u8* fileContent = ReadEntireFile(tempPool, markupFiles, info);
            Tokenizer tokenizer = {};
            tokenizer.at = (char*) fileContent;
            Token t = AdvanceToToken(&tokenizer, name);
            
            if(t.type != Token_EndOfFile)
            {
                if(RequireToken(&tokenizer, Token_Colon))
                {
                    while(true)
                    {
                        Token p = GetToken(&tokenizer);
                        
                        if(p.type == Token_Comma)
                        {
                        }
                        else if(p.type == Token_EndOfFile)
                        {
                            break;
                        }
                        else if(p.type == Token_SemiColon)
                        {
                        }
                        else if(p.type == Token_Asterisk)
                        {
                            break;
                        }
                        else
                        {
                            Token propertyName = p;
                            if(RequireToken(&tokenizer, Token_EqualSign))
                            {
                                Token value = GetToken(&tokenizer);
                                FillPAKProperty(asset, p, value);
                            }
                        }
                    }
                }
            }
        }
    }
}

internal u32 GetFileTypes(AssetType type)
{
    u32 fileTypes = PlatformFile_invalid;
    switch(type)
    {
        case AssetType_Image:
        {
            fileTypes |= PlatformFile_png;
            fileTypes |= PlatformFile_Coloration;
        } break;
        
        case AssetType_Sound:
        {
            fileTypes |= PlatformFile_sound;
        } break;
        
        case AssetType_Font:
        {
            fileTypes |= PlatformFile_font;
        } break;
        
        case AssetType_Skeleton:
        {
            fileTypes |= PlatformFile_skeleton;
        } break;
        
        case AssetType_Count:
        {
            InvalidCodePath;
        } break;
        
        default:
        {
            fileTypes |= PlatformFile_data;
        } break;
    }
    
    return fileTypes;
}

internal u32 GetSavedVersion(char* basePath, char* type)
{
    char path[128];
    FormatString(path, sizeof(path), "%s/%s", basePath, type);
    u32 result = 0xffffffff;
    PlatformFileGroup group = platformAPI.GetAllFilesBegin(PlatformFile_AssetVersion, path);
    if(group.fileCount)
    {
        Assert(group.fileCount == 1);
        PlatformFileInfo* info = group.firstFileInfo;
        PlatformFileHandle handle = platformAPI.OpenFile(&group, info);
        
        platformAPI.ReadFromFile(&handle, 0, sizeof(u32), &result);
        platformAPI.CloseFile(&handle);
    }
    
    platformAPI.GetAllFilesEnd(&group);
    
    return result;
}

internal void SaveVersion(char* basePath, char* type)
{
    u32 version = MetaGetCurrentVersion(type);
    char outputPath[128];
    FormatString(outputPath, sizeof(outputPath), "%s/%s", basePath, type);
    platformAPI.ReplaceFile(PlatformFile_AssetVersion, outputPath, "version", (u8*) &version, sizeof(u32), PlatformFileReplace_Hidden);
}


internal u32 GetSavedPakVersion(char* basePath)
{
    char path[128];
    FormatString(path, sizeof(path), "%s", basePath);
    u32 result = 0xffffffff;
    PlatformFileGroup group = platformAPI.GetAllFilesBegin(PlatformFile_AssetVersion, path);
    if(group.fileCount)
    {
        Assert(group.fileCount == 1);
        PlatformFileInfo* info = group.firstFileInfo;
        PlatformFileHandle handle = platformAPI.OpenFile(&group, info);
        platformAPI.ReadFromFile(&handle, 0, sizeof(u32), &result);
        platformAPI.CloseFile(&handle);
    }
    
    platformAPI.GetAllFilesEnd(&group);
    
    return result;
}

internal void SavePakVersion(char* basePath)
{
    u32 version = PAK_VERSION;
    platformAPI.ReplaceFile(PlatformFile_AssetVersion, basePath, "version", (u8*) &version, sizeof(u32), PlatformFileReplace_Hidden);
}

internal void WritePak(TimestampHash* hash, char* basePath, char* sourceDir, char* sourceSubdir, char* outputPath, b32 checkVersion)
{
    MemoryPool tempPool = {};
    
    u16 type = GetMetaAssetType(sourceDir);
    
    char source[128];
    char filename[128];
    
    FormatString(source, sizeof(source), "%s/%s/%s", basePath, sourceDir, sourceSubdir);
    FormatString(filename, sizeof(filename), "%s_%s", sourceDir, sourceSubdir);
    
    PAKFileHeader header = {};
    
    FormatString(header.name, sizeof(header.name), "%s.upak", filename);
    header.magicValue = PAK_MAGIC_NUMBER;
    header.version = PAK_VERSION;
    header.assetVersion = MetaGetCurrentVersion(sourceDir);
    
    b32 changedVersion = false;
    if(checkVersion)
    {
        u32 oldPakVersion = GetSavedPakVersion(basePath);
        u32 oldVersion = GetSavedVersion(basePath, sourceDir);
        changedVersion = (oldPakVersion != header.version || oldVersion != header.assetVersion);
    }
    
    FormatString(header.type, sizeof(header.type), "%s", sourceDir);
    FormatString(header.subtype, sizeof(header.subtype), "%s", sourceSubdir);
    header.standardAssetCount = 0;
    header.derivedAssetCount = 0;
    
    u32 fileTypes = GetFileTypes((AssetType) type);
    
    u32 startingFontCodepoint = ' ';
    u32 endingFontCodepoint = '~';
    
    PlatformFileGroup fileGroup = platformAPI.GetAllFilesBegin(fileTypes, source);
    
    u16 standardAssetCount = 0;
    u16 derivedAssetCount = 0;
    
    b32 updatedFiles = false;
    
    u32 validFileCount = 0;
    u32 validMarkupCount = 0;
    
    for(PlatformFileInfo* info = fileGroup.firstFileInfo; info; info = info->next)
    {
		if(RelevantFile(info, &fileGroup, true))
		{
            ++validFileCount;
            
			TempMemory fileMemory = BeginTemporaryMemory(&tempPool);
            PlatformFileHandle handle = platformAPI.OpenFile(&fileGroup, info);
            
            SavedFileInfoHash* saved = GetCorrenspodingFileDateHash(hash, source, info->name);
            if(!AreEqual(saved->timestamp, info->timestamp))
            {
                updatedFiles = true;
            }
            
            u16 standard = 1;
            u16 derived = 0;
            
            switch(type)
            {
                case AssetType_Font:
                {
                    derived = SafeTruncateToU16(endingFontCodepoint - startingFontCodepoint);
                } break;
                
                case AssetType_Skeleton:
                {
                    u8* tempData = PushSize(&tempPool, info->size, NoClear());
                    platformAPI.ReadFromFile(&handle, 0, info->size, tempData);
                    derived = CountAnimations((char*) tempData);
                } break;
                
                case AssetType_Image:
                {
                    if(IsColorationFile(info->name))
                    {
                        standard = 0;
                        derived = 0;
                        
                        u8* tempData = PushSize(&tempPool, info->size, NoClear());
                        platformAPI.ReadFromFile(&handle, 0, info->size, tempData);
                        LoadedColoration coloration = LoadColoration((char*) tempData);
                        b32 found = false;
                        for(PlatformFileInfo* testInfo = fileGroup.firstFileInfo; testInfo; testInfo = testInfo->next)
                        {
                            if(!IsColorationFile(testInfo->name))
                            {
                                if(StrEqual(testInfo->name, coloration.imageName))
                                {
                                    found = true;
                                    break;
                                }
                            }
                        }
                        
                        derived = 1;
                    }
                } break;
                
                default:
                {
                } break;
            }
            
            Assert(standardAssetCount < U16_MAX / 2);
            Assert(derivedAssetCount < U16_MAX / 2);
            
            standardAssetCount += standard;
            derivedAssetCount += derived;
            
            platformAPI.CloseFile(&handle);
            EndTemporaryMemory(fileMemory);
		}
    }
    
    header.standardAssetCount = standardAssetCount;
    header.derivedAssetCount = derivedAssetCount;
    
    
    PlatformFileGroup markupFiles = platformAPI.GetAllFilesBegin(PlatformFile_markup, source);
    for(PlatformFileInfo* info = markupFiles.firstFileInfo; info; info = info->next)
    {
        if(RelevantFile(info, &markupFiles, false))
        {
            ++validMarkupCount;
            SavedFileInfoHash* saved = GetCorrenspodingFileDateHash(hash, source, info->name);
            if(!AreEqual(saved->timestamp, info->timestamp))
            {
                updatedFiles = true;
            }
        }
    }
    
    u16 assetCount = header.standardAssetCount + header.derivedAssetCount;
    
    b32 changedFiles = false;
    
    char* typeStr = GetAssetTypeName(type);
    char* subtypeStr = sourceSubdir;
    
    SavedTypeSubtypeCountHash* savedCount = GetCorrenspodingFileCountHash(hash, typeStr, subtypeStr);
    if(validFileCount != savedCount->fileCount || 
       validMarkupCount != savedCount->markupCount)
    {
        changedFiles = true;
    }
    
    
    
    
    if(assetCount && (updatedFiles || changedFiles || changedVersion))
    {
        FILE* out = fopen("temp.tmp", "wb");
        if(out)
        {
            fwrite(&header, sizeof(PAKFileHeader), 1, out);
            
            u32 assetArraySize = assetCount * sizeof(PAKAsset);
            fseek(out, assetArraySize, SEEK_CUR);
            
            PAKAsset* pakAssets = PushArray(&tempPool, PAKAsset, assetCount, NoClear());
            u16 runningDerivedAssetIndex = derivedAssetCount ? header.standardAssetCount : 0;
            u16 runningAssetIndex = 0;
            
            for(PlatformFileInfo* info = fileGroup.firstFileInfo; info; info = info->next)
            {
				if(RelevantFile(info, &fileGroup, true))
				{
					TempMemory fileMemory = BeginTemporaryMemory(&tempPool);
                    u8* fileContent = ReadEntireFile(&tempPool, &fileGroup, info);
                    
                    Assert(runningAssetIndex < assetCount);
                    Assert(runningDerivedAssetIndex <= assetCount);
                    
                    PAKAsset* dest = pakAssets + runningAssetIndex++;
                    switch(type)
                    {
                        case AssetType_Image:
                        {
                            if(IsColorationFile(info->name))
                            {
                                LoadedColoration coloration = LoadColoration((char*) fileContent);
                                b32 found = false;
                                u16 bitmapIndex = 0;
                                for(PlatformFileInfo* testInfo = fileGroup.firstFileInfo; testInfo; testInfo = testInfo->next)
                                {
                                    if(!IsColorationFile(testInfo->name))
                                    {
                                        if(StrEqual(testInfo->name, coloration.imageName))
                                        {
                                            found = true;
                                            break;
                                        }
                                        
                                        ++bitmapIndex;
                                    }
                                }
                                
                                if(true)
                                {
                                    --runningAssetIndex;
                                    PAKAsset* derivedAsset = pakAssets + runningDerivedAssetIndex++;
                                    FillPAKAssetBaseInfo(out, &tempPool, derivedAsset, info->name, &markupFiles);
                                    
                                    Assert(StrLen(coloration.imageName) < sizeof(derivedAsset->coloration.imageName));
                                    FormatString(derivedAsset->coloration.imageName, sizeof(derivedAsset->coloration.imageName), "%s", coloration.imageName);
                                    derivedAsset->coloration.color = coloration.color;
                                    derivedAsset->coloration.bitmapIndex = bitmapIndex;
                                }
                            }
                            else
                            {
                                dest->bitmap.align[0] = 0.5f;
                                dest->bitmap.align[1] = 0.5f;
                                dest->bitmap.alphaThreesold = 0;
                                
                                FillPAKAssetBaseInfo(out, &tempPool, dest, info->name, &markupFiles);
                                LoadedBitmap bitmap = LoadImage(&tempPool, source, info->name, &markupFiles);
                                
                                char nameNoPoint[64];
                                TrimToFirstCharacter(nameNoPoint, sizeof(nameNoPoint), info->name, '.');
                                
                                dest->bitmap.nameHash = StringHash(nameNoPoint);
                                dest->bitmap.attachmentPointCount = bitmap.attachmentPointCount;
                                dest->bitmap.groupNameCount = bitmap.groupNameCount;
                                
                                dest->bitmap.dimension[0] = bitmap.width;
                                dest->bitmap.dimension[1] = bitmap.height;
                                
                                fwrite(bitmap.attachmentPoints, bitmap.attachmentPointCount * sizeof(PAKAttachmentPoint), 1, out);
                                fwrite(bitmap.groupNames, bitmap.groupNameCount * sizeof(PAKGroupName), 1, out);
                                fwrite(bitmap.pixels, bitmap.width * bitmap.height * sizeof(u32 ), 1, out);
                                
                                stbi_image_free(bitmap.free);
                                free(bitmap.attachmentPoints);
                                free(bitmap.groupNames);
                            }
                        } break;
                        
                        case AssetType_Sound:
                        {
                            FillPAKAssetBaseInfo(out, &tempPool, dest, info->name, &markupFiles);
                            LoadedSound sound = LoadWAV((char*) fileContent);
                            dest->sound.sampleCount = sound.countSamples;
                            dest->sound.channelCount = sound.countChannels;
                            dest->sound.maxSampleValue = sound.maxSampleValue;
                            dest->sound.decibelLevel = sound.decibelLevel;
                            
                            for(u32 channelIndex = 0; channelIndex < sound.countChannels; channelIndex++ )
                            {
                                fwrite(sound.samples[channelIndex], sound.countSamples * sizeof(i16), 1, out); 
                            }
                        } break;
                        
#if 0                        
                        case AssetType_Model:
                        {
                            FillPAKAssetBaseInfo(out, &tempPool, dest, info->name, &markupFiles);
                            LoadedModel model = LoadModel((char*) fileContent);
                            
                            dest->model.vertexCount = model.vertexCount;
                            dest->model.faceCount = model.faceCount;
                            dest->model.dim = model.dim;
                            fwrite(model.vertexes, sizeof(ColoredVertex) * model.vertexCount, 1, out);
                            fwrite(model.faces, sizeof(ModelFace) * model.faceCount, 1, out);
                            
                            free(model.vertexes);
                            free(model.faces);
                        } break;
#endif
                        
                        case AssetType_Font:
                        {
                            FillPAKAssetBaseInfo(out, &tempPool, dest, info->name, &markupFiles);
                            // TODO(Leonardo): how can we pass the font name here?
                            char fontName[128];
                            TrimToFirstCharacter(fontName, sizeof(fontName), info->name, '.');
                            LoadedFont font = LoadFont(source, info->name, fontName, 64, startingFontCodepoint, endingFontCodepoint);
                            
                            dest->font.glyphCount = font.glyphsCount;
                            dest->font.ascenderHeight = font.ascenderHeight;
                            dest->font.descenderHeight = font.descenderHeight;
                            dest->font.externalLeading = font.externalLeading;
                            dest->font.onePastHighestCodePoint = font.onePastHighestCodePoint;
                            dest->font.glyphAssetsFirstIndex = runningDerivedAssetIndex;
                            
                            u32 glyphsSize = font.glyphsCount * sizeof(u32);
                            fwrite(font.glyphs, glyphsSize, 1, out);
                            
                            
                            LoadedBitmap* glyphBitmaps = (LoadedBitmap*) malloc(sizeof(LoadedBitmap) * font.glyphsCount);
                            
                            for(u32 glyphIndex = 0; glyphIndex < font.glyphsCount; ++glyphIndex)
                            {
                                u32 codePoint = font.glyphs[glyphIndex];
                                if(codePoint)
                                {
                                    glyphBitmaps[glyphIndex] = LoadGlyph(&font, codePoint);
                                }
                            }
                            
                            u8* horizontalAdvancePtr = (u8*) font.horizontalAdvancement;
                            for(u32 glyphIndex = 0; glyphIndex < font.glyphsCount; glyphIndex++ )
                            {
                                u32 horizontalAdvanceSliceSize = sizeof(r32) * font.glyphsCount; 
                                fwrite(horizontalAdvancePtr, horizontalAdvanceSliceSize, 1, out);
                                horizontalAdvancePtr += sizeof(r32) * font.maximumGlyphsCount;
                            }
                            
                            for(u32 glyphIndex = 0; glyphIndex < font.glyphsCount; ++glyphIndex)
                            {
                                u32 codePoint = font.glyphs[glyphIndex];
                                if(codePoint)
                                {
                                    LoadedBitmap* bitmap = glyphBitmaps + glyphIndex;
                                    PAKAsset* derivedAsset = pakAssets + runningDerivedAssetIndex++;
                                    FillPAKAssetBaseInfo(out, &tempPool, derivedAsset, "ignored", &markupFiles);
                                    derivedAsset->bitmap.align[0] = bitmap->pivot.x;
                                    derivedAsset->bitmap.align[1] = bitmap->pivot.y;
                                    derivedAsset->bitmap.dimension[0] = bitmap->width;
                                    derivedAsset->bitmap.dimension[1] = bitmap->height;
                                    derivedAsset->bitmap.alphaThreesold = 0;
                                    
                                    fwrite(bitmap->pixels, bitmap->width * bitmap->height * sizeof(u32), 1, out);
                                    free(bitmap->free);
                                }
                            }
                            
                            
                            free(glyphBitmaps);
                            free(font.glyphs);
                            free(font.horizontalAdvancement);
                            free(font.glyphIndexForCodePoint);
                        } break;
                        
                        case AssetType_Skeleton:
                        {
                            FillPAKAssetBaseInfo(out, &tempPool, dest, info->name, &markupFiles);
                            u16 animationCount = CountAnimations((char*) fileContent);
                            dest->skeleton.animationCount = animationCount;
                            dest->skeleton.animationAssetsFirstIndex = runningDerivedAssetIndex;
                            
                            for(u16 animationIndex = 0; animationIndex < animationCount; ++animationIndex)
                            {
                                PAKAsset* derivedAsset = pakAssets + runningDerivedAssetIndex++;
                                LoadedAnimation animation = LoadAnimation(&tempPool, (char*) fileContent, animationIndex, &markupFiles);
                                
                                char trimmedFilename[128];
                                TrimToFirstCharacter(trimmedFilename, sizeof(trimmedFilename), info->name, '.');
                                
                                char animationName[128];
                                FormatString(animationName, sizeof(animationName), "%s_%s", trimmedFilename, animation.name);
                                
                                FillPAKAssetBaseInfo(out, &tempPool, derivedAsset, animationName, &markupFiles);
                                
                                u32 countTotalBones = 0;
                                u32 countTotalAss = 0;
                                for(u32 frameIndex = 0; frameIndex < animation.frameCount; frameIndex++)
                                {
                                    FrameData* data = animation.frames + frameIndex;
                                    countTotalBones += data->countBones;
                                    countTotalAss += data->countAss;
                                }
                                
                                derivedAsset->animation.durationMS = animation.durationMS;
                                Assert(derivedAsset->animation.durationMS > 0);
                                
                                derivedAsset->animation.spriteCount = animation.spriteInfoCount;
                                derivedAsset->animation.frameCount = animation.frameCount;
                                derivedAsset->animation.boneCount = countTotalBones;
                                derivedAsset->animation.assCount = countTotalAss;
                                derivedAsset->animation.triggerCount = animation.triggerCount;
                                
                                
                                fwrite(animation.spriteInfos, sizeof(SpriteInfo) * animation.spriteInfoCount, 1, out);
                                fwrite(animation.frames, sizeof(FrameData) * animation.frameCount, 1, out);
                                fwrite(animation.bones, countTotalBones * sizeof(Bone), 1, out);
                                fwrite(animation.ass, countTotalAss * sizeof(PieceAss), 1, out);
                                fwrite(animation.triggers, animation.triggerCount * sizeof(PAKAnimationSoundTrigger), 1, out);
                                free(animation.spriteInfos);
                                free(animation.frames);
                                free(animation.bones);
                                free(animation.triggers);
                            }
                        } break;
                        
                        case AssetType_Count:
                        {
                            InvalidCodePath;
                        }
                        default:
                        {
                            FillPAKAssetBaseInfo(out, &tempPool, dest, info->name, &markupFiles);
                            dest->dataFile.rawSize = SafeTruncateUInt64ToU32(info->size);
                            fwrite(fileContent, info->size, 1, out);
                        } break;
                    }
                    
                    EndTemporaryMemory(fileMemory);
				}
            }
            
            BeginTicketMutex(&hash->fileMutex);
            fseek(out, (u32) sizeof(PAKFileHeader), SEEK_SET);
            fwrite(pakAssets, assetCount * sizeof(PAKAsset), 1, out);
            fseek(out, 0, SEEK_END);
            fclose(out);
            
            PlatformFileGroup tempGroup = platformAPI.GetAllFilesBegin(PlatformFile_Temp, ".");
            Assert(tempGroup.fileCount == 1);
            PlatformFileInfo* tempInfo = tempGroup.firstFileInfo;
            PlatformFileHandle handle = platformAPI.OpenFile(&tempGroup, tempInfo);
            
            u8* content = PushSize(&tempPool, tempInfo->size, NoClear());
            platformAPI.ReadFromFile(&handle, 0, tempInfo->size, content);
            
            platformAPI.ReplaceFile(PlatformFile_AssetPack, outputPath, 
                                    filename, content, SafeTruncateUInt64ToU32(tempInfo->size), 0);
            platformAPI.CloseFile(&handle);
            platformAPI.GetAllFilesEnd(&tempGroup);
            
            
            EndTicketMutex(&hash->fileMutex);
            
            for(PlatformFileInfo* info = fileGroup.firstFileInfo; info; info = info->next)
            {
                SavedFileInfoHash* saved = GetCorrenspodingFileDateHash(hash, source, info->name);
                if(!AreEqual(saved->timestamp, info->timestamp))
                {
                    SaveFileDateHash(saved, info->timestamp);
                }
            }
            
            for(PlatformFileInfo* info = markupFiles.firstFileInfo; info; info = info->next)
            {
                SavedFileInfoHash* saved = GetCorrenspodingFileDateHash(hash, source, info->name);
                if(!AreEqual(saved->timestamp, info->timestamp))
                {
                    SaveFileDateHash(saved, info->timestamp);
                }
            }
            
            if(validFileCount != savedCount->fileCount || 
               validMarkupCount != savedCount->markupCount)
            {
                SaveFileCountHash(savedCount, validFileCount, validMarkupCount);
            }
        }
        else
        {
            printf("couldn't open temp file!\n");
        }
    }
    platformAPI.GetAllFilesEnd(&fileGroup);
    platformAPI.GetAllFilesEnd(&markupFiles);
    
    Clear(&tempPool);
}



internal void BuildAssets(TimestampHash* hash, char* sourcePath, char* destPath)
{
    PlatformSubdirNames subdir;
    platformAPI.GetAllSubdirectories(&subdir, sourcePath);
    for(u32 subdirIndex = 0; subdirIndex < subdir.count; ++subdirIndex)
    {
        char* subdirName = subdir.names[subdirIndex];
        char sourceAssetTypePath[128];
        FormatString(sourceAssetTypePath, sizeof(sourceAssetTypePath), "%s/%s", sourcePath, subdirName);
        
        PlatformSubdirNames subsubdir;
        platformAPI.GetAllSubdirectories(&subsubdir, sourceAssetTypePath);
        
        for(u32 subsubDirIndex = 0; subsubDirIndex < subsubdir.count; ++subsubDirIndex)
        {
            char* subsubDirName = subsubdir.names[subsubDirIndex];
            WritePak(hash, sourcePath, subdirName, subsubDirName, destPath, true);
        }
        
        SaveVersion(sourcePath, subdirName);
    }
    
    SavePakVersion(sourcePath);
}

internal b32 AutomaticReload(u16 type)
{
    b32 result = true;
    if(type == AssetType_Image ||
       type == AssetType_Sound)
    {
        result = false;
    }
    
    return result;
}

internal void WatchReloadFileChanges(TimestampHash* hash, char* sourcePath, char* destPath, char* destSendPath)
{
    PlatformSubdirNames subdir;
    platformAPI.GetAllSubdirectories(&subdir, sourcePath);
    for(u32 subdirIndex = 0; subdirIndex < subdir.count; ++subdirIndex)
    {
        char* subdirName = subdir.names[subdirIndex];
        char sourceAssetTypePath[128];
        FormatString(sourceAssetTypePath, sizeof(sourceAssetTypePath), "%s/%s", sourcePath, subdirName);
        PlatformSubdirNames subsubdir;
        platformAPI.GetAllSubdirectories(&subsubdir, sourceAssetTypePath);
        
        for(u32 subsubDirIndex = 0; subsubDirIndex < subsubdir.count; ++subsubDirIndex)
        {
            b32 updatedStandardFiles = false;
            b32 updatedTestFiles = false;
            
            char* subsubDirName = subsubdir.names[subsubDirIndex];
            char fullpath[128];
            FormatString(fullpath, sizeof(fullpath), "%s/%s/%s", sourcePath, subdirName, subsubDirName);
            
            u16 type = GetMetaAssetType(subdirName);
            
            u32 fileTypes = GetFileTypes((AssetType)type);
            PlatformFileGroup fileGroup = platformAPI.GetAllFilesBegin(fileTypes, fullpath);
            
            
            u32 currentFileCount = 0;
            u32 currentMarkupFileCount = 0;
            
            for(PlatformFileInfo* info = fileGroup.firstFileInfo; info; info = info->next)
            {
                if(RelevantFile(info, &fileGroup, true))
                {
                    ++currentFileCount;
                    SavedFileInfoHash* infoHash = GetCorrenspodingFileDateHash(hash, fullpath, info->name);
                    if(!AreEqual(info->timestamp, infoHash->timestamp))
                    {
                        if(EditorFile(info))
                        {
                            updatedTestFiles = true;
                        }
                        else
                        {
                            updatedStandardFiles = true;
                        }
                    }
                }
            }
            platformAPI.GetAllFilesEnd(&fileGroup);
            
            PlatformFileGroup markupFiles = platformAPI.GetAllFilesBegin(PlatformFile_markup, fullpath);
            for(PlatformFileInfo* info = markupFiles.firstFileInfo; info; info = info->next)
            {
                if(RelevantFile(info, &markupFiles, false))
                {
                    ++currentMarkupFileCount;
                    SavedFileInfoHash* infoHash = GetCorrenspodingFileDateHash(hash, fullpath, info->name);
                    if(!AreEqual(info->timestamp, infoHash->timestamp))
                    {
                        if(EditorFile(info))
                        {
                            updatedTestFiles = true;
                        }
                        else
                        {
                            updatedStandardFiles = true;
                        }
                    }
                }
            }
            
            platformAPI.GetAllFilesEnd(&markupFiles);
            
            char* typeStr = GetAssetTypeName(type);
            char* subtypeStr = subsubDirName;
            
            SavedTypeSubtypeCountHash* countHash = GetCorrenspodingFileCountHash(hash, typeStr, subtypeStr);
            
            b32 numberOfFilesChanged =(currentFileCount != countHash->fileCount || currentMarkupFileCount != countHash->markupCount);
            
            if(updatedStandardFiles || updatedTestFiles || numberOfFilesChanged)
            {
                if(AutomaticReload(type) || !updatedTestFiles)
                {
                    char* path = (updatedStandardFiles || numberOfFilesChanged) ? destSendPath : destPath;
                    
                    WritePak(hash, sourcePath, subdirName, subsubDirName, path, false);
                }
            }
        }
    }
}

inline void ReadCompressFile(ServerState* server, GameFile* file, u32 uncompressedSize, u8* uncompressedContent)
{
    Clear(&file->pool);
    file->uncompressedSize = uncompressedSize;          
    file->compressedSize = compressBound(file->uncompressedSize);
    file->content = PushSize(&file->pool, file->compressedSize);
    u32 cmp_status = compress(file->content, (mz_ulong*) &file->compressedSize, (const unsigned char*) uncompressedContent, file->uncompressedSize);
    Assert(cmp_status == Z_OK);
    
    u32 pakVersion = *(u32*) uncompressedContent;
    PAKFileHeader* header = (PAKFileHeader*) uncompressedContent;
    file->type = GetMetaAssetType(header->type);
    
    file->valid = IsValidPAKVersion(pakVersion) && IsValidPAKAssetVersion(header);
    
    Assert(StrLen(header->subtype) <= ArrayCount(file->subtype));
    FormatString(file->subtype, sizeof(file->subtype), "%s", header->subtype);
    file->dataHash = DataHash((char*) uncompressedContent, uncompressedSize);
}

internal GameFile* AddNewServerFile(ServerState* server, MemoryPool* tempPool, PlatformFileGroup* group, PlatformFileInfo* info)
{
    Assert(server->fileCount < server->maxFileCount);
    GameFile* file = server->files + server->fileCount++;
    
    PlatformFileHandle handle = platformAPI.OpenFile(group, info);
    TempMemory fileMemory = BeginTemporaryMemory(tempPool);
    u8* uncompressedContent = (u8*) PushSize(tempPool, info->size);
    platformAPI.ReadFromFile(&handle, 0, info->size, uncompressedContent);
    ReadCompressFile(server, file, SafeTruncateUInt64ToU32(info->size), uncompressedContent);
    EndTemporaryMemory(fileMemory);
    platformAPI.CloseFile(&handle);
    
    return file;
}

internal void ProcessReloadedFile(ServerState* server, MemoryPool* pool, PlatformFileGroup* group, PlatformFileInfo* info, b32 sendToPlayers)
{
    TempMemory fileMemory = BeginTemporaryMemory(pool);
    
    PlatformFileHandle handle = platformAPI.OpenFile(group, info);
    PAKFileHeader header;
    platformAPI.ReadFromFile(&handle, 0, sizeof(PAKFileHeader), &header);
    
    u16 type = GetMetaAssetType(header.type);
    u64 subtypeHash = StringHash(header.subtype);
    
    u32 fileIndex = 0;
    GameFile* file = 0;
    
    for(u32 testIndex = 0; testIndex < server->fileCount; ++testIndex)
    {
        GameFile* test = server->files + testIndex;
        if(test->type == type && StringHash(test->subtype) == subtypeHash)
        {
            file = test;
            fileIndex = testIndex;
            break;
        }
    }
    
    char nameNoExtension[128];
    TrimToFirstCharacter(nameNoExtension, sizeof(nameNoExtension), info->name, '.');
    
    b32 deleteFile = true;
    if(file)
    {
        if(file->counter == 0)
        {
            u8* uncompressedContent = (u8*) PushSize(pool, info->size);
            platformAPI.ReadFromFile(&handle, 0, info->size, uncompressedContent);
            
            
            u32 destFileIndex = 0;
            AssetFile* destFile = CloseAssetFileFor(server->assets, type, subtypeHash, &destFileIndex);
            Assert(destFile);
            ReopenReloadAssetFile(server->assets, destFile, destFileIndex, type, header.subtype, uncompressedContent, SafeTruncateUInt64ToU32(info->size), pool);
            
            ReadCompressFile(server, file, SafeTruncateUInt64ToU32(info->size), uncompressedContent);
            
        }
        else
        {
            sendToPlayers = false;
            deleteFile = false;
        }
    }
    else
    {
        file = AddNewServerFile(server, pool, group, info);
        InitLoadAssetFile(server->assets, &server->gamePool, group, info);
    }
    
    
    
    if(sendToPlayers)
    {
        for(CompIterator iter = FirstComponent(server, PlayerComponent); 
            IsValid(iter); iter = Next(iter))
        {
            PlayerComponent* player = GetComponentRaw(server, iter, PlayerComponent);
            if(player->connectionSlot)
            {
                FileToSend* toSend;
                FREELIST_ALLOC(toSend, server->firstFreeToSendFile, PushStruct(&server->gamePool, FileToSend));
                
                toSend->acked = false;
                toSend->playerIndex = player->runningFileIndex++;
                toSend->serverFileIndex = fileIndex;
                toSend->sendingOffset = 0;
                
                ++file->counter;
                FREELIST_INSERT(toSend, player->firstReloadedFileToSend);
                QueueFileHeader(player, toSend->playerIndex, file->type, file->subtype, file->uncompressedSize, file->compressedSize);
            }
        }
    }
    
    
    EndTemporaryMemory(fileMemory);
    
    BeginTicketMutex(&server->fileHash.fileMutex);
    platformAPI.CloseFile(&handle);
    if(deleteFile)
    {
        char* path = sendToPlayers ? RELOAD_SEND_PATH : RELOAD_PATH;
        platformAPI.ReplaceFile(PlatformFile_AssetPack, path, 
                                nameNoExtension, 0, 0, 0);
    }
    EndTicketMutex(&server->fileHash.fileMutex);
}

PLATFORM_WORK_CALLBACK(WatchForFileChanges)
{
    TimestampHash* hash = (TimestampHash*) param;
    while(true)
    {
        WatchReloadFileChanges(hash, ASSETS_RAW_PATH, RELOAD_PATH, RELOAD_SEND_PATH);
    }
}
