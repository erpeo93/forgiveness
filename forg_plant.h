#pragma once

printTable(noPrefix) enum PlantLifeStatus
{
    PlantLife_NewBranches,
    PlantLife_NewLeafs,
    PlantLife_LooseLeafs,
    PlantLife_Quiescent,
    
    
#if 0
    PlantLife_Flowers,
    PlantLife_Fruits,
    PlantLife_LooseFlowers,
    PlantLife_LooseFruits,
#endif
    
    PlantLife_Count,
};

enum PlantSegmentType
{
    PlantSegment_Invalid,
    
    PlantSegment_Meristem,
    PlantSegment_Branch,
};

enum PlantChildPosition
{
    PlantChild_Top,
    PlantChild_Left,
    PlantChild_Right,
    
    PlantChild_Up,
    PlantChild_Down,
    
    PlantChild_Count,
};

struct PlantAnglePhase
{
    b32 arrivedAtMaxAngle;
    r32 phase;
};

struct Leaf
{
    b32 present;
    PlantAnglePhase phase;
};

struct PlantAngle
{
    b32 negative;
    r32 absValue;
    PlantAnglePhase phase;
};

struct PlantSegment
{
    PlantSegmentType type;
    
    u8 segmentIndex;
    u8 parentSegmentIndex;
    u8 level;
    PlantSegment* childs[PlantChild_Count];
    
    PlantAngle angleX;
    PlantAngle angleY;
    r32 centerDelta;
    
    r32 radious;
    r32 length;
    
    u32 leafsSeed;
    u32 leafCount;
    Leaf leafs[8];
    
    PlantSegment* nextFree;
};


struct NewSegment
{
    PlantSegmentType type;
    RangeDistribution angleXDistr;
    RangeDistribution angleYDistr;
    u8 levelDelta;
    u8 leafCount;
};

struct SpecialRuleRequirement
{
    u8 level;
    u8 minSegmentIndex;
};

struct PlantRule
{
    u32 specialRequirementCount;
    SpecialRuleRequirement specialRequirement[8];
    
    PlantSegmentType requestedType;
    PlantSegmentType newType;
    
    u8 leafCountDelta;
    
    NewSegment newSegments[PlantChild_Count];
    
    r32 radiousIncrement;
};

struct PlantPhysicalParams
{
    u8 maxZeroLevelSegmentNumber;
    r32 maxRootSegmentRadious;
    r32 maxRootSegmentLength;
};

#define MAX_LEVELS 8
struct PlantParams
{
    u8 maxLevels;
    
    r32 oneOverMaxRadious;
    
    u8 maxSegmentNumber[MAX_LEVELS];
    r32 maxRadious[MAX_LEVELS];
    r32 segmentLengths[MAX_LEVELS];
    r32 leafSize[MAX_LEVELS];
    r32 plantStrength[MAX_LEVELS];
    
    r32 segmentBySegmentRadiousCoeff[MAX_LEVELS];
    r32 segmentBySegmentLengthCoeff[MAX_LEVELS];
    r32 segmentBySegmentBranchLengthCoeff[MAX_LEVELS];
    r32 segmentBySegmentStrengthCoeff[MAX_LEVELS];
    
    u32 ruleCount;
    PlantRule rules[8];
    
    r32 maxCenterDelta;
    
    r32 leafMinAngle;
    r32 leafMaxAngle;
    
    Vec4 leafColorAlive;
    Vec4 leafColorDead;
    Vec3 leafColorRandomization;
    
    Vec4 branchColorAlive;
    Vec4 branchColorDead;
    
    r32 leafRadious;
    r32 leafLength;
    
    
    u8 plantCount;
    r32 minOffset;
    r32 maxOffset;
    
    b32 isHerbaceous;
    
    
    union
    {
        PlantParams* next;
        PlantParams* nextFree;
    };
};
