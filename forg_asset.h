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
    u32 assetID[Action_Count];
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
    
    AssetType types[HASHED_ASSET_SLOT_COUNT + AssetSpecial_Count];
    
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


inline void PrefetchAllGroundBitmaps(Assets* assets, b32 immediate)
{
    u64 assetHashID = StringHash(ASSET_GROUND);
    u32 assetI = GetAssetIndex(assetHashID);
    AssetType* type = assets->types + assetI;
    for(u32 assetIndex = type->firstAssetIndex; assetIndex < type->onePastLastAssetIndex; ++assetIndex)
    {
		Asset* asset = assets->assets + assetIndex;
        if(asset->paka.typeHashID == assetHashID)
		{
			LoadBitmap(assets, {assetIndex}, immediate);
		}
    }
    
}


inline void PrefetchBitmap( Assets* assets, BitmapId ID, b32 immediate = false) { LoadBitmap( assets, ID, immediate); }
inline void PrefetchFont( Assets* assets, FontId ID ) { LoadFont( assets, ID, false ); }
inline void PrefetchSound( Assets* assets, SoundId ID ) { LoadSound( assets, ID ); }
inline void PrefetchAnimation( Assets* assets, AnimationId ID ) { LoadAnimation( assets, ID ); }


u32 GetFirstAsset_( Assets* assets, char* assetName);
inline BitmapId GetFirstBitmap( Assets* assets, char* assetName)
{
    BitmapId result = { GetFirstAsset_(assets, assetName), V4(1, 1, 1, 1)};
    return result;
}

inline SoundId GetFirstSound(Assets* assets, char* assetName)
{
    SoundId result = {GetFirstAsset_(assets, assetName)};
    return result;
}

inline ModelId GetFirstModel(Assets* assets, char* assetName)
{
    ModelId result = {GetFirstAsset_(assets, assetName)};
    
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

inline BitmapId GetMatchingBitmap(Assets* assets, u32 assetID, u64 stringHashID,
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

inline BitmapId GetMatchingBitmap(Assets* assets, char* assetName, TagVector* values, TagVector* weights, LabelVector* labels = 0)
{
    u64 stringHashID = StringHash(assetName);
    u32 assetID = GetAssetIndex(stringHashID);
    
    BitmapId result = GetMatchingBitmap(assets, assetID, stringHashID, values, weights, labels);
    return result;
}

inline FontId GetMatchingFont( Assets* assets, char* fontName, TagVector* values, TagVector* weights, LabelVector* labels = 0)
{
    u64 stringHashID = StringHash(fontName);
    u32 assetID = GetAssetIndex(stringHashID);
    
    FontId result = {GetMatchingAsset_( assets, assetID, stringHashID, values, weights, labels).assetIndex};
    return result;
}

inline SoundId GetMatchingSound( Assets* assets, u32 assetID, u64 stringHashID,
                                TagVector* values, TagVector* weights, LabelVector* labels = 0)
{
    SoundId result = {GetMatchingAsset_( assets, assetID, stringHashID, values, weights, labels).assetIndex};
    return result;
}

inline AnimationId GetMatchingAnimation( Assets* assets, u64 assetNameHash, u64 stringHashID,
                                        TagVector* values, TagVector* weights, LabelVector* labels = 0)
{
    u32 assetID = GetAssetIndex(assetNameHash);
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
