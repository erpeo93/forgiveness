#pragma once
struct ImageReference
{
    u64 typeHash;
    GameProperties properties;
    b32 emittors;
};

struct StandardImageComponent
{
    ImageReference entity;
};

struct SegmentImageComponent
{
    ImageReference entity;
};

struct MagicQuadComponent
{
    u32 color;
    Vec2 invUV;
    r32 alphaThreesold;
    ImageReference entity;
    AssetID bitmapID;
    Vec3 lateral;
    Vec3 up;
    Vec3 offset;
};

struct FrameByFrameAnimationComponent
{
    r32 runningTime;
    r32 speed;
    u64 typeHash;
    
    b32 emittors;
    b32 overridesPivot;
    Vec2 pivot;
};

struct MultipartAnimationComponent
{
	u16 staticCount;
    ImageReference staticParts[4];
    
	u16 frameByFrameCount;
	FrameByFrameAnimationComponent frameByFrameParts[4];
};

struct LayoutPiece
{
    u64 nameHash;
    r32 height;
    
    Vec4 color;
    
    u16 inventorySlotType;
    ImageReference image;
};

struct LayoutComponent
{
    Vec2 rootScale;
    r32 rootAngle;
    u64 rootHash;
    
    u32 pieceCount;
    LayoutPiece pieces[8];
    
    u32 openPieceCount;
    LayoutPiece openPieces[8];
    
    u32 usingPieceCount;
    LayoutPiece usingPieces[8];
    
    u32 equippedPieceCount;
    LayoutPiece equippedPieces[8];
};

enum LayoutContainerDrawMode
{
    LayoutContainerDraw_Standard,
    LayoutContainerDraw_Open,
    LayoutContainerDraw_Using,
    LayoutContainerDraw_Equipped
};
struct LayoutContainer
{
    LayoutContainerDrawMode drawMode;
    b32 storedObjectsDrawn[MAX_CONTAINER_OBJECT];
    b32 usingObjectsDrawn[MAX_USING_OBJECT];
    ContainerComponent* container;
    RecipeEssenceComponent* recipeEssences;
};
