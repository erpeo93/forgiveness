#pragma once

#include "asset_generated.h"

struct AssetID
{
    u16 type;
    u16 subtype;
    u32 index;
};

struct Font
{
    struct PakGlyph* glyphs;
    r32* horizontalAdvance;
    i32 offsetRebaseGlyphs;
    u16* unicodeMap;
};

struct Sound
{
    u32 countChannels;
    i32 countSamples;
    i16* samples[2];
};

enum AssetState
{
    Asset_unloaded,
    Asset_queued,
    Asset_loaded,
    Asset_locked,
};

struct AssetFile
{
    PlatformFileHandle handle;
    PAKHeader header;
};

struct AssetMemoryHeader
{
    AssetMemoryHeader* next;
    AssetMemoryHeader* prev;
    
    AssetID ID;
    u32 totalSize;
    union
    {
        Bitmap bitmap;
        Font font;
        Animation animation;
        Sound sound;
        VertexModel model;
        void* dataContent;
    };
};

struct AssetLRULink
{
    AssetLRULink* next;
    AssetLRULink* prev;
};

struct Asset
{
    AssetLRULink LRU;
    
    RenderTexture textureHandle;
    AssetMemoryHeader* header;
    PakAsset paka;
    u32 fileIndex;
    
    
    u32 state;
};

struct AssetMemoryBlock
{
    AssetMemoryBlock* prev;
    AssetMemoryBlock* next;
    u64 size;
    u64 flags;
};


struct AssetSubtypeArray
{
    u32 assetCount;
    Asset* assets;
};

struct AssetArray
{
    u32 subtypeCount;
    AssetSubtypeArray* subtypes;
};

struct Assets
{
    u32 lock;
    
    PlatformTextureOpQueue* textureQueue;
    
    AssetMemoryBlock blockSentinel;
    AssetMemoryHeader assetSentinel;
    
    u32 fileCount;
    AssetFile* files;
    
    
    u32 whitePixel;
    u32 nextFreeTextureHandle;
    u32 maxTextureHandleIndex;
    
    u32 nextFreeSpecialTextureHandle;
    u32 maxSpecialTextureHandleIndex;
    
    AssetLRULink LRUSentinel;
    AssetLRULink specialLRUSentinel;
    AssetLRULink lockedLRUSentinel;
    
    AssetArray assets[AssetType_Count];
};

internal void LoadBitmap(Assets* assets, AssetID ID, b32 immediate);
internal void LoadFont(Assets* assets, AssetID ID, b32 immediate );
internal void LoadSound(Assets* assets, AssetID ID);
internal void LoadAnimation( Assets* assets, AssetID ID);


inline void PrefetchBitmap(Assets* assets, AssetID ID, b32 immediate = false) { LoadBitmap(assets, ID, immediate);}
inline void PrefetchFont(Assets* assets, AssetID ID) { LoadFont( assets, ID, false ); }
inline void PrefetchSound( Assets* assets, AssetID ID ) { LoadSound( assets, ID ); }
inline void PrefetchAnimation( Assets* assets, AssetID ID ) { LoadAnimation( assets, ID ); }



Bitmap* GetBitmap(Assets* assets, AssetID ID);
Font* GetFont(Assets* assets, AssetID ID);
Sound* GetSound(Assets* assets, AssetID ID);
Animation* GetAnimation(Assets* assets, AssetID ID);

PakBitmap* GetBitmapInfo(Assets* assets, AssetID ID);
PakFont* GetFontInfo(Assets* assets, AssetID ID);
PakSound* GetSoundInfo(Assets* assets, AssetID ID);
PakSkeleton* GetSkeletonInfo(Assets* assets, AssetID ID);
