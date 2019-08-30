#pragma once
#pragma pack(push, 1)

#define PAK_CODE( a, b, c, d ) ((u32) ( ( a ) << 0 ) | ( u32 ) ( ( b ) << 8 ) | ( u32 ) ( ( c ) << 16 ) | ( u32 ) ( ( d ) << 24 ) ) 

#define PAK_MAGIC_NUMBER PAK_CODE('h', 'h', 'a', 'f')
#define PAK_VERSION 0

struct PAKHeader
{
    u32 magicValue;
    u32 version;
    
    u32 assetType;
    u32 assetSubType;
    u32 assetcount;
    
    u64 assetOffset;
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

struct PakSound
{
    u32 sampleCount;
    u32 channelCount;
    u32 chain;
    
    i16 maxSampleValue;
    r32 decibelLevel;
    
    // NOTE( Leonardo ): data is:
    /*
    i16* samples[2];
    */
};

struct PakSkeleton
{
    u32 animationCount;
    
    // NOTE( Leonardo ): data is:
    /*
    u32 spriteCount[animationCount];
    u32 frameCount[animationCount];
    u32 boneCount[animationCount];
    u32 assCount[animationCount];
    AnimationHeader header[animationCount];
    FrameData* frames;
    Bone* bones;
    PieceAss* ass;
    */
};

struct PakModel
{
    u32 vertexCount;
    u32 faceCount;
    Vec3 dim;
    
    // NOTE(Leonardo): data is:
    //ColoredVertex* vertexes;
    //ModelFace* faces;
};


struct PakDataFile
{
    u32 colorCount;
    
    // NOTE(Leonardo): data is:
    //Vec3* colors;
};

struct PakAsset
{
    u64 dataOffset;
    union
    {
        PakBitmap bitmap;
        PakSound sound;
        PakSkeleton skeleton;
        PakFont font;
        PakModel model;
        PakDataFile data;
    };
};

#pragma pack(pop)


