#pragma once

#define ASSETS_PATH "assets"
#define RELOAD_PATH "assets/reload"
#define RELOAD_SEND_PATH "assets/reload_send"
#define TIMESTAMP_PATH "assets/timestamps"
#define ASSETS_RAW_PATH "assets/raw"
#define PROPERTIES_PATH "../properties"
#define WRITEBACK_PATH "../server/assets/raw"

#define TEST_FILE_PREFIX "__test__"

#define MARKUP_FILE_NAME "markup"

#define SKELETON_FLIPPED "flipped"
#define ANIMATION_PROPERTY_SYNC_THREESOLD "syncThreesold"
#define ANIMATION_PROPERTY_PREPARATION_THREESOLD "preparationThreesold"

#define IMAGE_PROPERTY_ALIGN_X "alignX"
#define IMAGE_PROPERTY_ALIGN_Y "alignY"
#define IMAGE_ATTACHMENT_POINT "attachmentPoint"
#define IMAGE_GROUP_NAME "groupName"


#define INVALID_PROPERTY_VALUE 0xffff
#define INVALID_ASSET_SUBTYPE 0xffff
struct MetaPropertyList
{
    char name[64];
    u16 propertyCount;
    char** properties;
};

#include "asset_generated.h"

introspection() struct AssetID
{
    u16 type;
    u32 subtypeHashIndex;
    u16 index;
};

typedef AssetID BitmapId;
typedef AssetID FontId;
typedef AssetID ModelId;
typedef AssetID SkeletonId;
typedef AssetID SoundId;

enum GamePropertyFlags
{
    GameProperty_Optional = (1 << 0),
};

typedef PAKProperty GameProperty;
#define GameProp(property, value) GameProp_(Property_##property, value)
internal GameProperty GameProp_(u16 property, u16 value)
{
    GameProperty result;
    result.property = property;
    result.value = value;
    
    return result;
}

struct GameProperties
{
    GameProperty properties[MAX_PROPERTIES_PER_ASSET];
    u32 flags[MAX_PROPERTIES_PER_ASSET];
};

inline b32 AreEqual(GameProperty p1, GameProperty p2)
{
    b32 result = ((p1.property == p2.property) && (p1.value == p2.value));
    return result;
}

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
    
    r32 nativeHeight;
    r32 widthOverHeight;
    
    PAKAttachmentPoint* attachmentPoints;
    PAKGroupName* groupNames;
    
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
    i32 parentIndex;
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
    i32 boneIndex;
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
    u64 nameHash;
    char name[32];
    b32 placeHolder;
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

struct ColoredBitmap
{
    Vec2 pivot;
    Bitmap* bitmap;
    Vec4 coloration;
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
    u64 subtypeHash;
};


struct AssetLabel
{
    char name[16];
};

introspection() struct GroundColorationArrayTest
{
    u32 p1 MetaDefault("2");
    u32 p2 MetaDefault("3");
    
    GameProperty property;
};

introspection() struct ground_coloration
{
    Vec4 color MetaDefault("V4(1, 0, 1, 1)");
    
    ArrayCounter testCounter MetaCounter(a1);
    GroundColorationArrayTest* a1;
    
    GameAssetType asset MetaDefault("{AssetType_Font, 0}") MetaFixed(type);
    
    ArrayCounter propertyCount MetaCounter(properties);
    GameProperty* properties;
};

introspection() struct tile_definition
{
    GameAssetType asset MetaDefault("{AssetType_Image, 0}") MetaFixed(type);
    GameProperty property;
    Vec4 color MetaDefault("V4(1, 1, 1, 1)");
};

introspection() struct TileMapping
{
    tile_definition tile;
    r32 weight;
};

introspection() struct ground_generator
{
    ArrayCounter tileTypeCount MetaCounter(tiles);
    TileMapping* tiles;
};

introspection() struct EntityRef
{
    u32 subtypeHashIndex;
    u16 index;
};





















enum AssetState
{
    Asset_unloaded,
    Asset_queued,
    Asset_preloaded,
    Asset_loaded,
    Asset_locked,
    Asset_loadedNoData,
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

#define ASSETS_PER_BLOCK 64
struct AssetBlock
{
    Asset assets[ASSETS_PER_BLOCK];
    
    union
    {
        AssetBlock* next;
        AssetBlock* nextFree;
    };
};

struct AssetSubtypeArray
{
    u16 standardAssetCount;
    u16 derivedAssetCount;
    
    AssetBlock* firstAssetBlock;
    u32 fileIndex;
    u64 hash;
    
    AssetSubtypeArray* next;
};

struct AssetArray
{
    AssetSubtypeArray* subtypes[32];
};

struct AssetFile
{
    PlatformFileHandle handle;
    u32 size;
    b32 valid;
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
    AssetLRULink LRUFreeSentinel;
    AssetLRULink specialLRUSentinel;
    AssetLRULink specialLRUFreeSentinel;
    AssetLRULink lockedLRUSentinel;
    
    u32 fileCount;
    u32 maxFileCount;
    AssetFile* files;
    
    u32 whitePixel;
    u32 nextFreeTextureHandle;
    u32 maxTextureHandleIndex;
    
    u32 nextFreeSpecialTextureHandle;
    u32 maxSpecialTextureHandleIndex;
    
    AssetArray assets[AssetType_Count];
    AssetBlock* firstFreeAssetBlock;
    MemoryPool* pool;
    
    TicketMutex fileMutex;
};


inline b32 IsValid(AssetID ID)
{
    b32 result = (ID.type != AssetType_Invalid);
    return result;
}


#define GetValueFromNames(names, value) GetValueFromNames_(names, ArrayCount(names), value)
internal u16 GetValueFromNames_(char** names, u16 nameCount, char* value)
{
    u16 result = 0xffff;
    for(u16 nameIndex = 0; nameIndex < nameCount; ++nameIndex)
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

internal u16 GetValueFromNames_(char** names, u16 nameCount, Token value)
{
    u16 result = 0xffff;
    for(u16 nameIndex = 0; nameIndex < nameCount; ++nameIndex)
    {
        char* name = names[nameIndex];
        if(StrEqual(value.text, value.textLength, name, StrLen(name)))
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
    u16 result = GetValueFromNames(metaAsset_assetType, type);
    
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
