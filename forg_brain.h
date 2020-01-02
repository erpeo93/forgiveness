#pragma once
enum BrainState
{
    BrainState_Wandering,
    BrainState_Fleeing,
    BrainState_Following,
    BrainState_Chasing,
    BrainState_Attacking
};

struct BrainDirection
{
    u16 action;
    r32 coeff;
    r32 sum;
};

#define DIRECTION_COUNT 8
#define DIRECTION_ANGLE (360.0f / (r32) DIRECTION_COUNT)
struct BrainComponent
{
    u16 type;
    
	GameCommand currentCommand;
    GameCommand inventoryCommand;
    CommandParameters commandParameters;
    b32 commandCompleted;
    
    r32 time;
    Vec3 wanderDirection;
    u16 state;
    
    EntityID targetID;
    BrainDirection idleDirection;
    BrainDirection directions[DIRECTION_COUNT];
};

introspection() struct BrainParams
{
    r32 hostileDistanceRadious MetaDefault("5.0f");
    r32 maintainDistanceRadious MetaDefault("5.0f");
    r32 maintainDistanceModulationCoeff MetaDefault("0.5f");
    r32 maintainDistanceModulationAdiacentCoeff MetaDefault("0.5f");
    r32 scaryDistanceRadious MetaDefault("3.0f");
    r32 safeDistanceRadious MetaDefault("8.0f");
    r32 idleTimeWhenWandering;
    r32 wanderTargetTime MetaDefault("2.0f");
    r32 safetyLightRadious MetaDefault("1.0f");
    EntityName hostileType;
    EntityName maintainDistanceType;
    EntityName scaryType;
};