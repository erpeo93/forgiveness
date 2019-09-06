#pragma once

#define ASSETS_PATH "assets"
#define LABELS_FILE_NAME "properties"


typedef PAKLabel GameLabel;

struct MetaAssetType
{
    u32 subtypeCount;
    char** names;
};

#define INVALID_LABEL_VALUE 0xffff
#define INVALID_ASSET_SUBTYPE 0xffff
struct MetaLabelList
{
    char name[64];
    u16 labelCount;
    char** labels;
};

#include "asset_generated.h"

struct AssetID
{
    u16 type;
    u16 subtype;
    u16 index;
};

typedef AssetID BitmapId;
typedef AssetID FontId;
typedef AssetID ModelId;
typedef AssetID SkeletonId;
typedef AssetID SoundId;

struct AssetLabels
{
    PAKLabel labels[MAX_LABEL_PER_ASSET];
};

struct RenderTexture
{
    u32 index;
    u16 width;
    u16 height;
};

struct Bitmap
{
    void* pixels;
    u16 width;
    u16 height;
    
    Vec2 pivot;
    r32 nativeHeight;
    r32 widthOverHeight;
    
    RenderTexture textureHandle;
};

struct Font
{
    u32* codepoints;
    r32* horizontalAdvance;
    u16* unicodeMap;
};

struct Sound
{
    u32 countChannels;
    i32 countSamples;
    i16* samples[2];
};

struct Bone
{
    u32 timeLineIndex;
    i32 id;
    Vec2 mainAxis;
    i32 parentID;
    r32 parentAngle;
    
    r32 finalAngle;
    Vec2 parentOffset;
    Vec2 finalOriginOffset;
    
    // NOTE(leonardo): if spin is -1, it means that we have to lerp the angle in the opposite way, means going clockwise instead of counter-clockwise
    i32 spin;
};

struct PieceAss
{
    u32 spriteIndex;
    u32 timeLineIndex;
    i32 boneID;
    Vec2 boneOffset;
    r32 additionalZOffset;
    r32 angle;
    Vec2 scale;
    Vec4 color;
    // NOTE(leonardo): if spin is -1, it means that we have to lerp the angle in the opposite way, means going clockwise instead of counter-clockwise
    i32 spin;
};

struct SpriteInfo
{
    Vec2 pivot;
    u64 stringHashID;
    u8 index;
    char name[32];
    u32 flags;
};

struct FrameData
{
    u32 firstBoneIndex;
    u8 countBones;
    u32 firstAssIndex;
    u8 countAss;
    u16 timelineMS;
};

struct Animation
{
    u32 spriteInfoCount;
    u32 frameCount;
    
    SpriteInfo* spriteInfos;
    FrameData* frames;
    Bone* bones;
    PieceAss* ass;
};

struct Skeleton
{
};


struct ColoredVertex
{
    Vec3 P;
    Vec4 color;
    Vec3 N;
};

struct ModelFace
{
    u16 i0;
    u16 i1;
    u16 i2;
};

struct VertexModel
{
    u32 vertexCount;
    u32 faceCount;
    
    Vec3 dim;
    
    ColoredVertex* vertexes;
    ModelFace* faces;
};

struct DataFile
{
    u32 rawSize;
};


introspection() struct GameAssetType
{
    u16 type;
    u16 subtype;
};

introspection() struct GroundColorationArrayTest
{
    u32 p1 MetaDefault("2");
    u32 p2 MetaDefault("3");
    
    GameLabel label;
};

introspection() struct ground_coloration
{
    Vec4 color MetaDefault("V4(1, 0, 1, 1)");
    
    ArrayCounter testCounter MetaCounter(a1);
    GroundColorationArrayTest* a1;
    
    GameAssetType asset MetaDefault("{AssetType_Font, AssetFont_debug}") MetaFixed(type);
    
    ArrayCounter labelCount MetaCounter(labels);
    GameLabel* labels;
};

























enum AssetState
{
    Asset_unloaded,
    Asset_queued,
    Asset_preloaded,
    Asset_loaded,
    Asset_locked,
};

struct AssetLRULink
{
    AssetLRULink* next;
    AssetLRULink* prev;
};

struct AssetMemoryBlock
{
    AssetMemoryBlock* prev;
    AssetMemoryBlock* next;
    u64 size;
    u64 flags;
};

struct SpecialTexture
{
    AssetLRULink LRU;
    RenderTexture textureHandle;
};

struct Asset
{
    AssetLRULink LRU;
    
    void* data;
    union
    {
        Bitmap bitmap;
        Font font;
        Skeleton skeleton;
        Animation animation;
        Sound sound;
        VertexModel model;
        DataFile dataFile;
    };
    
    RenderTexture textureHandle;
    
    PAKAsset paka;
    
    u32 state;
    
    Asset* next;
    Asset* prev;
};

struct AssetSubtypeArray
{
    u16 standardAssetCount;
    u16 derivedAssetCount;
    Asset* assets;
    u32 fileIndex;
};

struct AssetArray
{
    u32 subtypeCount;
    AssetSubtypeArray* subtypes;
};

struct AssetFile
{
    PlatformFileHandle handle;
    PAKFileHeader header;
};

struct Assets
{
    u32 lock;
    u32 threadsReading;
    
    PlatformTextureOpQueue* textureQueue;
    PlatformWorkQueue* loadQueue;
    
    u32 taskCount;
    TaskWithMemory* tasks;
    
    AssetMemoryBlock blockSentinel;
    Asset assetSentinel;
    
    AssetLRULink LRUSentinel;
    AssetLRULink specialLRUSentinel;
    AssetLRULink lockedLRUSentinel;
    
    
    u32 fileCount;
    AssetFile* files;
    
    u32 whitePixel;
    u32 nextFreeTextureHandle;
    u32 maxTextureHandleIndex;
    
    u32 nextFreeSpecialTextureHandle;
    u32 maxSpecialTextureHandleIndex;
    
    AssetArray assets[AssetType_Count];
};


inline b32 IsValid(AssetID ID)
{
    b32 result = (ID.type != AssetType_Invalid);
    return result;
}


#define GetValueFromNames(names, value) GetValueFromNames_(names, ArrayCount(names), value)
internal u32 GetValueFromNames_(char** names, u32 nameCount, char* value)
{
    u32 result = 0;
    for(u32 nameIndex = 0; nameIndex < nameCount; ++nameIndex)
    {
        char* name = names[nameIndex];
        if(StrEqual(value, name))
        {
            result = nameIndex;
            break;
        }
    }
    
    return result;
}

#define GetNameFromNames(names, value) GetNameFromNames_(names, ArrayCount(names), value)
internal char* GetNameFromNames_(char** names, u32 nameCount, u32 value)
{
    char* result = 0;
    if(value < nameCount)
    {
        result = names[value];
    }
    return result;
}

internal u16 GetMetaAssetType(char* type)
{
    u16 result = SafeTruncateToU16(GetValueFromNames(metaAsset_assetType, type));
    
    return result;
}

internal char* GetAssetTypeName(u16 type)
{
    char* result = 0;
    if(type < AssetType_Count)
    {
        result = GetNameFromNames(metaAsset_assetType, type);
    }
    else
    {
        InvalidCodePath;
    }
    
    return result;
}

internal u16 GetMetaAssetSubtype(u16 type, char* subtype)
{
    u16 result = 0;
    
    if(type < AssetType_Count)
    {
        MetaAssetType sub = metaAsset_subTypes[type];
        result = SafeTruncateToU16(GetValueFromNames_(sub.names, sub.subtypeCount, subtype));
    }
    
    return result;
}

internal char* GetAssetSubtypeName(u16 type, u16 subtype)
{
    char* result = 0;
    
    if(type < AssetType_Count)
    {
        MetaAssetType sub = metaAsset_subTypes[type];
        result = GetNameFromNames_(sub.names, sub.subtypeCount, subtype);
    }
    
    return result;
}
