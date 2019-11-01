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

struct LayoutPiece
{
    u64 nameHash;
    r32 height;
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
    u32 currentObjectIndex;
    ContainerMappingComponent* container;
};
