#pragma once
struct ImageReference
{
    u64 typeHash;
    GameProperties properties;
};

struct StandardImageComponent
{
    ShadowComponent shadow;
    ImageReference entity;
};

struct MagicQuadComponent
{
    u32 color;
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
    
    ShadowComponent shadow;
    u64 typeHash;
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
    ShadowComponent shadow;
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
