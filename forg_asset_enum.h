#pragma once

enum FontType
{
    Font_default,
    Font_debug,
};

printTable(noPrefix) enum Material
{
    Material_None,
    Material_Standard,
    Material_Special,
    Material_Count,
};

printTable(noPrefix) enum TagId
{
    Tag_none,
    
    Tag_direction,
    Tag_skeletonFirstHalf,
    Tag_skeletonSecondHalf,
    
    Tag_skinFirstHalf,
    Tag_skinSecondHalf,
    
    Tag_codepoint,
    
    Tag_shotIndex,
    Tag_layerIndex,
    
    Tag_fontType,
    
    Tag_dimX,
    Tag_dimY,
    Tag_Material,
    
    Tag_count,
};

#define LABEL_HASH_COUNT (MegaBytes(1)) // NOTE(Leonardo): has to be a power of 2!


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


#define HASHED_ASSET_SLOTS 32
printTable(noPrefix) enum AssetTypeId
{
    Asset_none,
    Asset_music,
    
    Asset_font,
    Asset_glyph,
    
    Asset_openingCutscene = 19,
    
    Asset_rig,
    Asset_standing,
    Asset_standingDragging,
    Asset_moving,
    Asset_movingDragging,
    Asset_attacking,
    Asset_eating,
    Asset_casting,
    Asset_swimming,
    Asset_rolling,
    Asset_protecting,
    
    Asset_AnimationLast,
    
    Asset_ground,
    Asset_leaf,
    Asset_flower,
    Asset_fruit,
    Asset_trunk,
    Asset_waterRipple,
    Asset_emptySpace,
    Asset_scrollUI,
    Asset_BookPage,
    Asset_BookElement,
    Asset_Bookmark,
    Asset_Particle,
    
    Asset_count,
};