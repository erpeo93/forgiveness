#pragma once

#define ASSETS_PATH "assets"
#define LABELS_FILE_NAME "properties"

struct MetaAsset
{
    char* name;
    u32 value;
};

struct MetaAssetType
{
    u32 subtypeCount;
    MetaAsset* enums;
};

#define INVALID_LABEL_VALUE 0xffff
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


#define EDITOR_COUNTER_STRING "array_counter"
introspection() struct GroundColorationArrayTest
{
    u32 p1 MetaDefault("2");
    u32 p2 MetaDefault("3");
};

introspection() struct ground_coloration
{
    Vec4 color MetaDefault("V4(1, 0, 1, 1)");
    
    u32 array_counter_a1;
    GroundColorationArrayTest* a1;
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


#define GetValueFromEnum(enums, value) GetValueFromEnum_(enums, ArrayCount(enums), value)
internal u32 GetValueFromEnum_(MetaAsset* enums, u32 enumCount, char* value)
{
    u32 result = 0;
    
    for(u32 enumIndex = 0; enumIndex < enumCount; ++enumIndex)
    {
        MetaAsset* enumValue = enums + enumIndex;
        if(StrEqual(value, enumValue->name))
        {
            result = enumIndex;
            break;
        }
    }
    
    return result;
}

#define GetNameFromEnum(enums, value) GetNameFromEnum_(enums, ArrayCount(enums), value)
internal char* GetNameFromEnum_(MetaAsset* enums, u32 enumCount, u32 value)
{
    char* result = 0;
    
    for(u32 enumIndex = 0; enumIndex < enumCount; ++enumIndex)
    {
        MetaAsset* enumValue = enums + enumIndex;
        if(enumValue->value == value)
        {
            result = enumValue->name;
            break;
        }
    }
    
    return result;
}

internal u32 GetMetaAssetType(char* type)
{
    u32 result = GetValueFromEnum(metaAsset_assetType, type);
    
    return result;
}

internal char* GetAssetTypeName(u32 type)
{
    char* result = 0;
    if(type < AssetType_Count)
    {
        result = GetNameFromEnum(metaAsset_assetType, type);
    }
    else
    {
        InvalidCodePath;
    }
    
    return result;
}


internal u32 GetMetaAssetSubtype(u32 type, char* subtype)
{
    u32 result = 0;
    
    if(type < AssetType_Count)
    {
        MetaAssetType sub = metaAsset_subTypes[type];
        result = GetValueFromEnum_(sub.enums, sub.subtypeCount, subtype);
    }
    
    return result;
}

internal char* GetAssetSubtypeName(u32 type, u32 subtype)
{
    char* result = 0;
    
    if(type < AssetType_Count)
    {
        MetaAssetType sub = metaAsset_subTypes[type];
        result = GetNameFromEnum_(sub.enums, sub.subtypeCount, subtype);
    }
    
    return result;
}
