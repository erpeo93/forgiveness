#pragma once
struct ImageReference
{
    u64 typeHash;
    GameProperties properties;
};

struct StandardImageComponent
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
    Vec4 lateral;
    Vec4 up;
    Vec3 offset;
};

struct FrameByFrameAnimationComponent
{
    r32 runningTime;
    r32 speed;
    u64 typeHash;
    
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
};

enum LayoutContainerDrawMode
{
    LayoutContainerDraw_Standard,
    LayoutContainerDraw_Open,
    LayoutContainerDraw_Using,
    
};
struct LayoutContainer
{
    LayoutContainerDrawMode drawMode;
    b32 storedObjectsDrawn[MAX_CONTAINER_OBJECT];
    b32 usingObjectsDrawn[MAX_USING_OBJECT];
    ContainerMappingComponent* container;
};
