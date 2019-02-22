#pragma once

#define MAX_DECADENCE_FACTOR 5.0f
#define MAX_REFRESH_FACTOR 5.0f

struct AICommand
{
    EntityAction action;
    u64 targetID;
};

struct AIDestination
{
    char* expression;
};

enum AIActionType
{
    AIAction_Command,
    AIAction_Destination,
    AIAction_Behavior,
};


struct AIConceptTargets
{
    u32 conceptTaxonomy;
    
    u32 targetCount;
    u64 targets[32];
};

struct AIAction
{
    AIActionType type;
    r32 importance;
    
    u32 associatedConcept;
    union
    {
        AICommand command;
        AIDestination destination;
        u32 behaviorTaxonomy;
    };
    
    u32 considerationCount;
    Consideration considerations[4];
};

enum AIBlackboardEntryType
{
    AIBlackboard_None,
    AIBlackboard_R32,
};

struct AIBlackboardEntry
{
    AIBlackboardEntryType type;
    u64 nameHash;
    
    union
    {
        r32 value_R32;
    };
};

struct AIBehavior
{
    u32 actionCount;
    AIAction actions[8];
    
    union
    {
        AIBehavior* next;
        AIBehavior* nextFree;
    };
};

struct BrainBehavior
{
    u32 taxonomy;
    
    AIBlackboardEntry blackboard[8];
    
    r32 importance;
    u32 activeActionIndex;
};


struct CachedExpression
{
    u64 cachedHashID;
    r32 value;
};

struct AIInfluenceMap
{
    u32 taxonomy;
    u8 values[256];
};


#define REACHABLE_TILEMAP_DIM 64
#define REACHABLE_TILEMAP_SPAN 16.0f

enum ReachableDirection
{
    ReachableDir_Left,
    ReachableDir_Right,
    ReachableDir_Up,
    ReachableDir_Down,
};

struct ReachableTile
{
    u8 directionToReachOrigin;
};

#define PATH_NODES 16
struct PathToReach
{
    u8 nodeCount;
    u16 X[PATH_NODES];
    u16 Y[PATH_NODES];
    Vec3 steps[PATH_NODES];
    Vec3 originP;
    
    u8 currentNodeIndex;
};


struct Brain
{
    b32 valid;
    r32 timeSinceLastUpdate;
    
    //CachedExpression cachedhashmap[64];
    //AIInfluenceMap EnemiesProximity;
    //AIInfluenceMap AlliesProximity;
    ReachableTile reachableTiles[REACHABLE_TILEMAP_DIM * REACHABLE_TILEMAP_DIM];
    
    
    u32 behaviorCount;
    BrainBehavior behaviorStack[8];
    
    AIConceptTargets conceptTargets[8];
    AIConceptTargets newConceptTargets[8];
    
    Mem memory;
    
    
    EntityAction oldAction;
    u64 oldTargetID;
    
    PathToReach path;
    AICommand desiredCommand;
};

