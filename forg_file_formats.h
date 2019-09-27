#pragma once
#pragma pack(push, 1)

#define PAK_CODE( a, b, c, d ) ((u32) ( ( a ) << 0 ) | ( u32 ) ( ( b ) << 8 ) | ( u32 ) ( ( c ) << 16 ) | ( u32 ) ( ( d ) << 24 ) ) 

#define PAK_MAGIC_NUMBER PAK_CODE('h', 'h', 'a', 'f')
#define PAK_VERSION 0

struct PAKFileHeader
{
    char name[32];
    
    u32 magicValue;
    u32 version;
    
    char type[32];
    char subtype[32];
    
    u16 standardAssetCount;
    u16 derivedAssetCount;
};

struct PAKProperty
{
    u16 property;
    u16 value;
};


struct PAKBitmap
{
    u64 nameHash;
    u32 dimension[2];
    r32 align[2];
    r32 nativeHeight;
    // NOTE( Leonardo ): data is:
    /*
    u32* pixels;
    */
};

struct PAKColoration
{
    char imageName[32];
    Color color;
    u16 bitmapIndex;
};

struct PAKFont
{
    u32 glyphCount;
    r32 ascenderHeight;
    r32 descenderHeight;
    r32 externalLeading;
    u32 onePastHighestCodePoint;
    u16 glyphAssetsFirstIndex;
    // NOTE( Leonardo ): data is:
    /*
     u32 codePoints[glyphsCount];
    r32 horizintalAdvance[glyphsCount][glyphsCount];
    */
};

struct PAKSound
{
    u32 sampleCount;
    u32 channelCount;
    
    i16 maxSampleValue;
    r32 decibelLevel;
    // NOTE( Leonardo ): data is:
    /*
    i16* samples[2];
    */
};

struct PAKAnimation
{
    u32 spriteCount;
    u32 frameCount;
    u32 boneCount;
    u32 assCount;
    
    u16 durationMS;
    
    
    u16 preparationThreesoldMS;
    u16 syncThreesoldMS;
    
    // NOTE( Leonardo ): data is:
    /*
    SpriteInfo sprites[spriteCount]
    FrameData frames[frameCount];
    Bone bones[boneCount];
    PieceAss ass[assCount];
    */
};

struct PAKSkeleton
{
    u16 animationCount;
    u16 animationAssetsFirstIndex;
};

struct PAKModel
{
    u32 vertexCount;
    u32 faceCount;
    Vec3 dim;
    
    // NOTE(Leonardo): data is:
    //ColoredVertex vertexes[vertexCount];
    //ModelFace faces[faceCount];
};


struct PAKDataFile
{
    u32 rawSize;
    // NOTE( Leonardo ): data is:
    /*
    u8 data[rawSize];
    */
};

#define MAX_PROPERTIES_PER_ASSET 8
struct PAKAsset
{
    char sourceName[32];
    u64 dataOffset;
    PAKProperty properties[MAX_PROPERTIES_PER_ASSET];
    
    union
    {
        PAKBitmap bitmap;
        PAKColoration coloration;
        PAKSound sound;
        PAKSkeleton skeleton;
        PAKAnimation animation;
        PAKFont font;
        PAKModel model;
        PAKDataFile dataFile;
    };
};

#pragma pack(pop)


