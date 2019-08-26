#pragma once
enum RarityType
{
    Rarity_Common,
    Rarity_UnCommon,
    Rarity_Rare,
    Rarity_VeryRare,
    Rarity_Legendary,
    Rarity_Unique,
    
    Rarity_Count,
};

printTable(noPrefix) enum ObjectState
{
    ObjectState_Default,
    ObjectState_Ground,
    ObjectState_Open,
    ObjectState_GroundOpen,
    
    ObjectState_Count,
};

struct LayoutPieceParams
{
    b32 valid;
    Vec4 color;
    Vec3 parentOffset;
    r32 parentAngle;
    Vec2 scale;
    r32 alpha;
    Vec2 pivot;
};

#define MAX_COMPONENT_PER_MODULE 8
struct LayoutPiece
{
    u64 componentHashID;
    u8 index;
    
    LayoutPieceParams params[ObjectState_Count];
    
    char name[32];
	LayoutPiece* parent;
    u32 ingredientCount;
    u32 ingredientTaxonomies[MAX_COMPONENT_PER_MODULE];
    u32 ingredientQuantities[MAX_COMPONENT_PER_MODULE];
    
    union
    {
        LayoutPiece* next;
        LayoutPiece* nextFree;
    };
};

inline LayoutPieceParams* GetParams(LayoutPiece* piece, ObjectState state)
{
    LayoutPieceParams* params = piece->params + state;
    if(!params->valid)
    {
        params = piece->params + ObjectState_Default;
    }
    
    return params;
}

struct ObjectLayout
{
    char name[32];
    u64 nameHashID;
    
    u32 pieceCount;
    LayoutPiece* firstPiece;
    
    union
    {
        ObjectLayout* next;
        ObjectLayout* nextFree;
    };
};