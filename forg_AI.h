#pragma once

#define MAX_DECADENCE_FACTOR 5.0f
#define MAX_REFRESH_FACTOR 5.0f

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

enum AIActionType
{
    AIAction_None,
    AIAction_Behavior,
    AIAction_StandardAction,
    AIAction_SpecialAction
};

enum EntitySpecialAction
{
    SpecialAction_MoveLeft,
    SpecialAction_MoveRight,
};

union AIActionData
{
    u32 behaviorTaxonomy;
	struct
	{
        u32 standardAction;
		u32 conceptTaxonomy;
	};
    EntitySpecialAction specialAction;
};

enum AIConditionType
{
    AICondition_TooFar,
    AICondition_TooNear,
    AICondition_OnSight,
    AICondition_DoingActionFor,
};

union AIConditionData
{
    r32 distance;
    u32 taxonomy;
    r32 time;
};

struct AICondition
{
    b32 negated;
    AIConditionType type;
    AIConditionData data;
};

struct AIStateMachineTransition
{
    u32 conditionCount;
    AICondition conditions[8];
    u32 destActionIndex;
};

struct AICommand
{
    AIActionType type;
    AIActionData data;
};

struct AIAction
{
    AICommand command;
    
    u32 transitionCount;
    AIStateMachineTransition transitions[8];
};

struct AIStateMachine
{
    u32 actionCount;
    AIAction actions[8];
};

#define MAX_CONCEPTS 16
struct Brain
{
    b32 valid;
    r32 timeSinceLastUpdate;
    
    ReachableTile reachableTiles[REACHABLE_TILEMAP_DIM * REACHABLE_TILEMAP_DIM];
    PathToReach path;
    
    u32 currentActionIndex;
    AIStateMachine* stateMachine;
    
    
    u32 savedTaxonomies[MAX_CONCEPTS];
    r32 bestScore[MAX_CONCEPTS];
    u64 bestTarget[MAX_CONCEPTS];
    
    
    u32 oldAction;
    u64 oldTargetID;
    
    r32 commandTime;
    AICommand currentCommand;
};

