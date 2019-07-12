#pragma once

struct PakGlyph;
struct Font
{
    PakGlyph* glyphs;
    r32* horizontalAdvance;
    i32 offsetRebaseGlyphs;
    u16* unicodeMap;
};

struct Sound
{
    u32 countChannels;
    i32 countSamples;
    i16* samples[2];
    
    SoundId nextToPlay;
};

enum AssetHeaderType
{
    AssetType_None,
    AssetType_Bitmap,
};

enum AssetState
{
    Asset_unloaded,
    Asset_queued,
    Asset_loaded,
};

struct TagVector
{
    r32 E[Tag_count];
};

struct LabelVector
{
    u32 labelCount;
    u32 IDs[16];
    r32 values[16];
};

struct AssetType
{
    u32 firstAssetIndex;
    u32 onePastLastAssetIndex;
};

struct AssetFile
{
    PlatformFileHandle handle;
    PAKHeader header;
    
    PakAssetType* assetTypes;
    u32 tagBase;
    
    u32 fontBitmapsOffsetIndex;
    
};

struct AssetMemoryHeader
{
    AssetMemoryHeader* next;
    AssetMemoryHeader* prev;
    
    AssetHeaderType assetType;
    u32 generationID;
    u32 assetIndex;
    u32 totalSize;
    union
    {
        Bitmap bitmap;
        Font font;
        Animation animation;
        Sound sound;
        VertexModel model;
    };
    
};

struct AssetLRULink
{
    AssetLRULink* next;
    AssetLRULink* prev;
};

struct TranState;
struct Asset
{
    AssetLRULink LRU;
    
    RenderTexture textureHandle;
    
    AssetMemoryHeader* header;
    PakAsset paka;
    u32 fileIndex;
    
    u32 state;
    b32 locked;
};

struct AssetMemoryBlock
{
    AssetMemoryBlock* prev;
    AssetMemoryBlock* next;
    u64 size;
    u64 flags;
};

// NOTE(Leonardo): for every action, we store the corresponding animation asset 
struct AssetMappings
{
    AssetTypeId assetID[Action_Count];
};

struct Assets
{
    PlatformTextureOpQueue* textureQueue;
    GameState* gameState;
    
    AssetMemoryBlock blockSentinel;
    AssetMemoryHeader assetSentinel;
    
    u32 fileCount;
    AssetFile* files;
    
    u32 tagCount;
    PakTag* tags;
    
    u32 assetCount;
    
    Asset* assets;
    
    AssetType types[Asset_count + HASHED_ASSET_SLOTS];
    
    u32 whitePixel;
    u32 nextFreeTextureHandle;
    u32 maxTextureHandleIndex;
    
    u32 nextFreeSpecialTextureHandle;
    u32 maxSpecialTextureHandleIndex;
    
    AssetLRULink LRUSentinel;
    AssetLRULink specialLRUSentinel;
    AssetLRULink lockedLRUSentinel;
};

internal void LoadBitmap( Assets* assets, BitmapId ID, b32 immediate );
internal void LoadFont( Assets* assets, FontId ID, b32 immediate );
internal void LoadSound( Assets* assets, SoundId ID );
internal void LoadAnimation( Assets* assets, AnimationId ID );


inline void PrefetchAllGroundBitmaps(Assets* assets)
{
    AssetType* type = assets->types + Asset_Ground;
    for(u32 assetIndex = type->firstAssetIndex; assetIndex < type->onePastLastAssetIndex; ++assetIndex)
    {
        LoadBitmap(assets, {assetIndex}, false);
    }
    
}


inline void PrefetchBitmap( Assets* assets, BitmapId ID, b32 immediate = false) { LoadBitmap( assets, ID, immediate); }
inline void PrefetchFont( Assets* assets, FontId ID ) { LoadFont( assets, ID, false ); }
inline void PrefetchSound( Assets* assets, SoundId ID ) { LoadSound( assets, ID ); }
inline void PrefetchAnimation( Assets* assets, AnimationId ID ) { LoadAnimation( assets, ID ); }


u32 GetRandomAsset_( Assets* assets, u32 assetID, RandomSequence* seq );
inline BitmapId GetRandomBitmap( Assets* assets, u32 assetID, RandomSequence* seq )
{
    BitmapId result = { GetRandomAsset_( assets, assetID, seq ), V4(1, 1, 1, 1) };
    return result;
}

inline SoundId GetRandomSound( Assets* assets, u32 assetID, RandomSequence* seq )
{
    SoundId result = { GetRandomAsset_( assets, assetID, seq ) };
    return result;
}

u32 GetFirstAsset_( Assets* assets, u32 assetID );
inline BitmapId GetFirstBitmap( Assets* assets, u32 assetID )
{
    BitmapId result = { GetFirstAsset_( assets, assetID ), V4(1, 1, 1, 1)};
    return result;
}

inline SoundId GetFirstSound( Assets* assets, u32 assetID )
{
    SoundId result = { GetFirstAsset_( assets, assetID ) };
    return result;
}

inline ModelId GetFirstModel(Assets* assets, u32 assetID)
{
    ModelId result = {GetFirstAsset_(assets, assetID)};
    
    return result;
}
struct MatchingAssetResult
{
	u32 assetIndex;
    b32 cloned;
    union
    {
        Vec4 clonedColoration;
    };
};

MatchingAssetResult GetMatchingAsset_( Assets* assets, u32 assetID, u64 stringHashID,
                                      TagVector* values, TagVector* weights, LabelVector* labels);

inline BitmapId GetMatchingBitmap_( Assets* assets, u32 assetID, u64 stringHashID,
                                   TagVector* values, TagVector* weights, LabelVector* labels = 0)
{
    MatchingAssetResult matching = GetMatchingAsset_( assets, assetID, stringHashID, values, weights, labels );
    
    if(!matching.cloned)
    {
        matching.clonedColoration = V4(1, 1, 1, 1);
    }
    
    BitmapId result {matching.assetIndex, matching.clonedColoration};
    
    return result;
}

inline BitmapId GetMatchingBitmap(Assets* assets, u32 assetIndex, TagVector* values, TagVector* weights, LabelVector* labels = 0)
{
    BitmapId result = GetMatchingBitmap_(assets, assetIndex, 0, values, weights, labels);
    return result;
}

inline BitmapId GetMatchingBitmapHashed(Assets* assets, u64 stringHashID, TagVector* values, TagVector* weights, LabelVector* labels = 0)
{
    u32 assetIndex = Asset_count + (stringHashID & (HASHED_ASSET_SLOTS - 1));
    BitmapId result = GetMatchingBitmap_(assets, assetIndex, stringHashID, values, weights, labels);
    return result;
}

inline FontId GetMatchingFont( Assets* assets, u32 assetID, 
                              TagVector* values, TagVector* weights, LabelVector* labels = 0)
{
    FontId result = {GetMatchingAsset_( assets, assetID, 0, values, weights, labels).assetIndex};
    return result;
}

inline SoundId GetMatchingSound( Assets* assets, u32 assetID, u64 stringHashID,
                                TagVector* values, TagVector* weights, LabelVector* labels = 0)
{
    SoundId result = {GetMatchingAsset_( assets, assetID, stringHashID, values, weights, labels).assetIndex};
    return result;
}

inline AnimationId GetMatchingAnimation( Assets* assets, u32 assetID, u64 stringHashID,
                                        TagVector* values, TagVector* weights, LabelVector* labels = 0)
{
    AnimationId result = {GetMatchingAsset_( assets, assetID, stringHashID, values, weights, labels).assetIndex};
    return result;
}

Bitmap* GetBitmap( Assets* assets, BitmapId ID, u32 generationID );
Font* GetFont( Assets* assets, FontId ID, u32 generationID );
Sound* GetSound( Assets* assets, SoundId ID, u32 generationID );
Animation* GetAnimation( Assets* assets, AnimationId ID, u32 generationID );

PakBitmap* GetBitmapInfo( Assets* assets, BitmapId ID );
PakFont* GetFontInfo( Assets* assets, FontId ID );
PakSound* GetSoundInfo( Assets* assets, SoundId ID );
PakAnimation* GetAnimationInfo( Assets* assets, AnimationId ID );
SoundId GetNextSoundInChain( Assets* assets, SoundId ID );
