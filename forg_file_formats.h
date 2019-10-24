#pragma once
#pragma pack(push, 1)

#define PAK_CODE( a, b, c, d ) ((u32) ( ( a ) << 0 ) | ( u32 ) ( ( b ) << 8 ) | ( u32 ) ( ( c ) << 16 ) | ( u32 ) ( ( d ) << 24 ) ) 

#define PAK_MAGIC_NUMBER PAK_CODE('h', 'h', 'a', 'f')
#define PAK_VERSION 2

struct PAKFileHeader
{
    u32 version;
    u32 assetVersion;
    u32 magicValue;
    
    char name[32];
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

struct PAKAttachmentPoint
{
    char name[32];
    Vec2 alignment;
    Vec2 scale;
    r32 angle;
    r32 zOffset;
};

struct PAKGroupName
{
    char name[32];
};

#define PAK_BITMAP_VERSION 7
struct PAKBitmap
{
    u64 nameHash;
    u32 dimension[2];
    r32 align[2];
    r32 nativeHeight;
    u32 attachmentPointCount;
    u32 groupNameCount;
    // NOTE( Leonardo ): data is:
    /*
    u32* pixels;
    AttachmentPoint* points[64];
    PAKGroupName* groups[16];
    */
};

struct PAKColoration
{
    char imageName[32];
    Color color;
    u16 bitmapIndex;
};

#define PAK_FONT_VERSION 0
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

#define PAK_SOUND_VERSION 0
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

#define PAK_ANIMATION_VERSION 2
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
    b32 flippedByDefault;
    u16 animationCount;
    u16 animationAssetsFirstIndex;
};

#define PAK_MODEL_VERSION 0
struct PAKModel
{
    u32 vertexCount;
    u32 faceCount;
    Vec3 dim;
    
    // NOTE(Leonardo): data is:
    //ColoredVertex vertexes[vertexCount];
    //ModelFace faces[faceCount];
};


#define PAK_DATA_VERSION 0
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
    
    u64 propertyHash[MAX_PROPERTIES_PER_ASSET];
    u64 valueHash[MAX_PROPERTIES_PER_ASSET];
    PAKProperty runtime[MAX_PROPERTIES_PER_ASSET];
    
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


