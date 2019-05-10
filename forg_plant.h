#pragma once

struct PlantSegment
{
    r32 radiousCoeff;
    r32 lengthCoeff;
    
    r32 angleY;
    
    struct PlantStem* clones;
    struct PlantStem* childs;
    
    union
    {
        PlantSegment* next;
        PlantSegment* nextFree;
    };
};

struct Leaf
{
    b32 initialized;
    
    r32 dimCoeff;
    r32 offsetCoeff;
    
    r32 renderingRandomization;
    r32 colorRandomization;
    r32 windRandomization;
};

#define MAX_LEAFS_PER_STEM 16
struct PlantStem
{
    PlantSegment* root;
    
    u8 segmentCount;
    
    r32 probabliltyToClone;
    
    m4x4 orientation;
    r32 parentStemZ;
    r32 parentSegmentZ;
    
    r32 lengthNormZ;
    r32 totalLength;
    r32 baseRadious;
    r32 maxRadious;
    
    
    r32 trunkNoise;
    
    r32 nextChildZ;
    u32 numberOfChilds;
    r32 childsCurrentAngle;
    
    r32 additionalCurveBackAngle;
    
    Leaf leafs[MAX_LEAFS_PER_STEM];
    
    union
    {
        PlantStem* next;
        PlantStem* nextFree;
    };
};

#define MAX_PLANT_COUNT 8
struct PlantInstance
{
    u32 plantCount;
    
    Vec2 offsets[MAX_PLANT_COUNT];
    r32 angleZ[MAX_PLANT_COUNT];
    
    u32 trunkCount;
    PlantStem* firstTrunk;
};

struct PlantLevelParams
{
    u8 curveRes;
    r32 curveBack;
    r32 curve;
    r32 curveV;
    
    r32 segSplits;
    r32 baseSplits;
    
    r32 splitAngle;
    r32 splitAngleV;
    
    r32 branches;
    r32 branchesV;
    r32 downAngle;
    r32 downAngleV;
    r32 rotate;
    r32 rotateV;
    
    r32 lengthCoeff;
    r32 lengthCoeffV;
    
    r32 taper;
    
    r32 radiousMod;
    
    r32 clonePercRatio;
    r32 clonePercRatioV;
    
    Vec4 baseYoungColor;
    Vec4 topYoungColor;
    
    Vec4 baseOldColor;
    Vec4 topOldColor;
    
    r32 lengthIncreaseSpeed;
    r32 radiousIncreaseSpeed;
    
    u8 leafCount;
    r32 allLeafsAtStemLength;
};

printTable(noPrefix) enum PlantShape
{
    PlantShape_Conical,
    PlantShape_Spherical,
    PlantShape_Hemispherical,
    PlantShape_Cylindrical,
    PlantShape_TaperedCylindrical,
    PlantShape_Flame,
    PlantShape_InverseConical,
    PlantShape_TendFlame
};

#define MAX_LEVELS 4
struct PlantDefinition
{
    b32 collides;
    PlantShape shape;
    
    r32 growingCoeff;
    
    u32 plantCount;
    r32 plantCountV;
    
    Vec2 plantOffsetV;
    r32 plantAngleZV;
    
    r32 attractionUp;
    
    r32 baseSize;
    
    r32 scale;
    r32 scaleV;
    
    r32 scale_0;
    r32 scaleV_0;
    
    r32 ratio;
    r32 ratioPower;
    
    r32 flare;
    
    u8 maxLevels;
    PlantLevelParams levelParams[MAX_LEVELS];
    
    Vec4 leafColor;
    Vec4 leafColorV;
    
    r32 leafDimSpeed;
    r32 leafOffsetSpeed;
    
    Vec2 leafScale;
    Vec2 leafScaleV;
    
    Vec3 leafOffsetV;
    r32 leafAngleV;
    
    r32 leafWindAngleV;
    r32 leafWindDirectionV;
    
    Vec4 trunkColorV;
    
    r32 lobeDepth;
    r32 lobes;
    
    u64 leafStringHash;
    u64 trunkStringHash;
    
    union
    {
        PlantDefinition* next;
        PlantDefinition* nextFree;
    };
};


inline r32 GetPlantStandardTrunkLength(PlantDefinition* definition)
{
    r32 result =  definition->scale * definition->levelParams[0].lengthCoeff;
    return result;
}

inline r32 GetPlantStandardTrunkRadious(PlantDefinition* definition)
{
    r32 trunkLength = GetPlantStandardTrunkLength(definition);
    r32 result = trunkLength * definition->ratio * definition->scale_0;
    
    return result;
}
