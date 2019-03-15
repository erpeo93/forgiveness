#pragma once

#pragma pack( push, 1 )

#define PAK_CODE( a, b, c, d ) ( ( u32 ) ( ( a ) << 0 ) | ( u32 ) ( ( b ) << 8 ) | ( u32 ) ( ( c ) << 16 ) | ( u32 ) ( ( d ) << 24 ) ) 

#define PAK_MAGIC_NUMBER PAK_CODE( 'h', 'h', 'a', 'f' )
#define PAK_VERSION 0


struct PAKHeader
{
    u32 magicValue;
    u32 version;
    
    u32 tagCount;
    u32 assetTypeCount;
    u32 assetcount;
    
    u64 tagOffset;
    u64 assetTypeOffset;
    u64 assetOffset;
};

struct PakAssetType
{
    u32 ID;
    u32 firstAssetIndex;
    u32 onePastLastAssetIndex;
};

struct PakTag
{
    u32 ID;
    r32 value;
};

struct PakBitmap
{
    u32 dimension[2];
    r32 align[2];
    r32 nativeHeight;
    
    // NOTE( Leonardo ): data is:
    /*
    u32* pixels;
    */
};

struct PakGlyph
{
    u32 unicodeCodePoint;
    BitmapId bitmapId;
};

struct PakFont
{
    u32 glyphsCount;
    r32 ascenderHeight;
    r32 descenderHeight;
    r32 externalLeading;
    
    u32 onePastHighestCodePoint;
    
    // NOTE( Leonardo ): data is:
    /*
    PakGlyph codePoints[glyphsCount];
    r32* horizintalAdvance[glyphsCount][glyphsCount];
    */
};

enum SoundChain
{
    Chain_none,
    Chain_loop,
    Chain_next,
};

struct PakSound
{
    u32 sampleCount;
    u32 channelCount;
    u32 chain;
    
    // NOTE( Leonardo ): data is:
    /*
    i16* samples[2];
    */
};

struct PakAnimation
{
    u32 spriteCount;
    u32 frameCount;
    u32 boneCount;
    u32 assCount;
    
    // NOTE( Leonardo ): data is:
    /*
    AnimationHeader header;
    FrameData* frames;
    Bone* bones;
    PieceAss* ass;
    */
};

struct PakAsset
{
    u64 dataOffset;
    u32 firstTagIndex;
    u32 onePastLastTagIndex;
    u64 typeHashID;
    u64 nameHashID;
    
    union
    {
        PakBitmap bitmap;
        PakSound sound;
        PakAnimation animation;
        PakFont font;
    };
};

#pragma pack( pop )


