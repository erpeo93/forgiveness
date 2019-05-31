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

struct CheckCollisionCurrent
{
    b32 isEntity;
    union
    {
        struct SimEntity* entity;
        struct RegionTile* tile;
    };
};
