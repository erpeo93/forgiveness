#pragma once

enum FontType
{
    Font_default,
    Font_debug,
};

printTable(noPrefix) enum TagId
{
    Tag_SkeletonSkinFirstHalf,
    Tag_SkeletonSkinSecondHalf,
    
    Tag_codepoint,
    
    Tag_shotIndex,
    Tag_layerIndex,
    
    Tag_fontType,
    
    Tag_count,
};

#define LABEL_HASH_COUNT (Mega(1)) // NOTE(Leonardo): has to be a power of 2!


struct VisualLabel
{
    u32 ID;
    r32 value;
    
    union
    {
        VisualLabel* next;
        VisualLabel* nextFree;
    };
};

#define HASHED_ASSET_SLOT_COUNT 32
inline u32 GetAssetIndex(u64 assetHash)
{
    u32 hashIndex =  assetHash & (HASHED_ASSET_SLOT_COUNT - 1 );
    u32 result = hashIndex + 1;
    Assert(result);
    return result;
}


#define ASSET_LEAF "asset_leaf"
#define ASSET_FLOWER "asset_flower"
#define ASSET_FRUIT "asset_fruit"
#define ASSET_TRUNK "asset_trunk"
#define ASSET_GROUND "asset_ground"
#define ASSET_PARTICLE "asset_particle"
#define Asset_emptySpace "asset_emptyspace"
#define Asset_scrollUI "asset_scrollUI"
#define Asset_BookPage "asset_bookPage"
#define Asset_BookElement "asset_bookelement"
#define Asset_Bookmark "asset_bookmark"
#define Asset_music "asset_music"
#define Asset_font "asset_font"
#define Asset_rig "asset_rig"
#define Asset_standing "asset_standing"
#define Asset_standingDragging "asset_standingDragging"
#define Asset_moving "asset_moving"
#define Asset_movingDragging "asset_movingDragging"
#define Asset_attacking "asset_attacking"
#define Asset_eating "asset_eating"
#define Asset_casting "asset_casting"
#define Asset_swimming "asset_swimming"
#define Asset_rolling "asset_rolling"
#define Asset_protecting "asset_protecting"

#define AssetSpecial_Glyph HASHED_ASSET_SLOT_COUNT
#define AssetSpecial_Count 2