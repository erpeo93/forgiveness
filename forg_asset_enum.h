#pragma once

enum FontType
{
    Font_default,
    Font_debug,
};

enum ObjectState
{
    ObjectState_None,
    ObjectState_Ground,
    ObjectState_Open,
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
    Tag_firstHashHalf,
    Tag_secondHashHalf,
    
    Tag_codepoint,
    
    // NOTE(Leonardo): these has to stay at exactly those indexes, 5 and 6!
    Tag_shotIndex,
    Tag_layerIndex,
    
    Tag_fontType,
    
    Tag_dimX,
    Tag_dimY,
    Tag_ObjectState,
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
    Asset_moving,
    Asset_attacking,
    Asset_eating,
    Asset_casting,
    Asset_equipmentRig,
    
    Asset_leaf,
    Asset_emptySpace,
    Asset_scrollUI,
    Asset_UISphere,
    Asset_UISphereBounds,
    Asset_BookPage,
    Asset_BookElement,
    Asset_Bookmark,
    Asset_Icon,
    
    Asset_count,
};