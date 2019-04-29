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

struct PlantStem
{
    PlantSegment* root;
    
    u8 segmentCount;
    
    m4x4 orientation;
    
    r32 parentStemZ;
    r32 parentSegmentZ;
    
    r32 totalLength;
    r32 baseRadious;
    
    u32 numberOfChilds;
    r32 childsCurrentAngle;
    
    r32 additionalCurveBackAngle;
    
    union
    {
        PlantStem* next;
        PlantStem* nextFree;
    };
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
    
    r32 length;
    r32 lengthV;
    
    r32 taper;
    
    r32 lengthIncreaseSpeed;
    r32 radiousIncreaseSpeed;
    
    r32 createClonesLengthNorm;
    r32 createChildsLengthNorm;
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

struct PlantDefinition
{
    PlantShape shape;
    r32 baseSize;
    
    r32 scale;
    r32 scaleV;
    
    r32 scale_0;
    r32 scaleV_0;
    
    r32 ratio;
    r32 ratioPower;
    
    r32 flare;
    
    u8 maxLevels;
    PlantLevelParams levelParams[4];
    
    union
    {
        PlantDefinition* next;
        PlantDefinition* nextFree;
    };
};
