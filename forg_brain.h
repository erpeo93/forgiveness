#pragma once
#define LIGHT_RADIOUS_PERCENTAGE_MAINTAIN_DISTANCE 1.5f
enum BrainState
{
    BrainState_Wandering,
    BrainState_Fleeing,
    BrainState_Following,
    BrainState_Chasing,
    BrainState_Attacking
};


struct BrainComponent
{
    u16 type;
    
	GameCommand currentCommand;
    GameCommand inventoryCommand;
    CommandParameters commandParameters;
    b32 commandCompleted;
    
    EntityID ID;
    r32 time;
    Vec3 wanderDirection;
    u16 state;
};

introspection() struct BrainParams
{
    r32 hostileDistanceRadious MetaDefault("5.0f");
    r32 maintainDistanceRadious MetaDefault("5.0f");
    r32 scaryDistanceRadious MetaDefault("3.0f");
    r32 safeDistanceRadious MetaDefault("8.0f");
    r32 idleTimeWhenWandering;
    r32 wanderTargetTime MetaDefault("2.0f");
    
    EntityRef hostileType;
    EntityRef maintainDistanceType;
    EntityRef scaryType;
};