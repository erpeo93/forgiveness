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


#define REACHABLE_GRID_DIM 32
struct ReachableCell
{
    b32 reachable;
    //b32 anglesOccluded[4];
    u16 shortestDirection;
};

struct ReachableQueueElement
{
    Vec3 offset;
    ReachableCell* cell;
    ReachableQueueElement* next;
};

struct ReachableQueue
{
    ReachableQueueElement* first;
    ReachableQueueElement* last;
    
    MemoryPool* pool;
};

struct ReachableMapComponent
{
    ReachableCell cells[REACHABLE_GRID_DIM][REACHABLE_GRID_DIM];
};

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
    EntityID attackerID;
    BrainDirection idleDirection;
    BrainDirection directions[DIRECTION_COUNT];
    
    UniversePos homeP;
    ReachableMapComponent* reachableMap;
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
    
    ArrayCounter hostileCount MetaCounter(hostileTypes);
    EntityName* hostileTypes;
    
    ArrayCounter maintainDistanceCount MetaCounter(maintainDistanceTypes);
    EntityName* maintainDistanceTypes;
    
    ArrayCounter scaryCount MetaCounter(scaryTypes);
    EntityName* scaryTypes;
    
    r32 minHomeDistance;
    r32 maxHomeDistance;
    
    r32 maxDistanceFromHomeChase MetaDefault("10.0f");
    r32 reachableCellDim MetaDefault("0.5f");
    
    b32 fearsLight;
};