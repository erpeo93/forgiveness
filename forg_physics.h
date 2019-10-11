#pragma once
struct MoveStep
{
    Vec3 delta;
};

struct MoveSpec
{
    r32 acceleration;
    r32 drag;
    
    u32 stepCount;
    Vec3* steps;
};

inline MoveSpec DefaultMoveSpec(r32 accelerationCoeff)
{
    MoveSpec moveSpec = {27.0f, -7.8f};
    moveSpec.acceleration *= accelerationCoeff;
    
    return moveSpec;
}

struct Wall
{
    r32 x;
    r32 deltax;
    r32 deltay;
    r32 deltaz;
    r32 px;
    r32 py;
    r32 pz;
    r32 miny;
    r32 maxy;
    
    r32 minz;
    r32 maxz;
    
    Vec3 normal;
};

struct SpatialPartitionEntityBlock
{
    u32 count;
	EntityID IDs[64];
	SpatialPartitionEntityBlock* next;
};

struct SpatialPartitionChunk
{
	u32 totalCount;
	SpatialPartitionEntityBlock* entities;
};

struct SpatialPartition
{
    i32 width;
    i32 height;
    r32 cellDim;
	SpatialPartitionChunk* chunks;
};

struct SpatialPartitionQuery
{
	SpatialPartition* partition;
    
	i32 minX;
	i32 minY;
	
	i32 maxX;
	i32 maxY;
    
	i32 currentX;
	i32 currentY;
    
	u32 currentIndex;
};
