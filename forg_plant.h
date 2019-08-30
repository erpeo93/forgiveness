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

struct LeafFlowerFruit
{
    b32 initialized;
    
    r32 dimCoeff;
    r32 offsetCoeff;
    
    r32 renderingRandomization;
    r32 densityRandomization;
    r32 colorRandomization;
    r32 windRandomization;
};

#define MAX_LFF_PER_STEM 16
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
    
    LeafFlowerFruit leafs[MAX_LFF_PER_STEM];
    LeafFlowerFruit flowers[MAX_LFF_PER_STEM];
    LeafFlowerFruit fruits[MAX_LFF_PER_STEM];
    
    union
    {
        PlantStem* next;
        PlantStem* nextFree;
    };
};

#define MAX_PLANT_COUNT 32
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
    
    r32 lengthIncreaseSpeedBeforeClones;
    r32 radiousIncreaseSpeedBeforeClones;
    
    r32 lengthIncreaseSpeedAfterClones;
    r32 radiousIncreaseSpeedAfterClones;
    
    r32 normClonesSpawnAtLength;
    
    u8 leafCount;
    r32 allLeafsAtStemLength;
    
    u8 flowerCount;
    r32 allFlowersAtStemLength;
    
    u8 fruitCount;
    r32 allFruitsAtStemLength;
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

struct PlantLFFSeasonParams
{
    Vec2 scale;
    Vec2 scaleV;
    Vec4 aliveColor;
    Vec4 deadColor;
    Vec4 colorV;
    
    r32 densityAtMidSeason;
};


struct PlantLFFParams
{
    Vec3 offsetV;
    r32 angleV;
    
    r32 windAngleV;
    r32 windDirectionV;
    
    r32 dimSpeed;
    r32 offsetSpeed;
    
    u64 bitmapHash;
    
    PlantLFFSeasonParams seasons[Season_Count];
};


inline PlantLFFSeasonParams* GetPreviousSeason(PlantLFFParams* params, WorldSeason season)
{
    Assert(season < Season_Count);
    
    u32 index = (season == 0) ? Season_Count - 1 : season - 1;
    PlantLFFSeasonParams* result = params->seasons + index;
    
    return result;
}

inline PlantLFFSeasonParams* GetSeason(PlantLFFParams* params, WorldSeason season)
{
    Assert(season < Season_Count);
    PlantLFFSeasonParams* result = params->seasons + season;
    
    return result;
}

inline PlantLFFSeasonParams* GetFollowingSeason(PlantLFFParams* params, WorldSeason season)
{
    Assert(season < Season_Count);
    
    u32 index = (season == Season_Count - 1) ? 0 : season + 1;
    PlantLFFSeasonParams* result = params->seasons + index;
    
    return result;
}

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
    
    PlantLFFParams leafParams;
    PlantLFFParams flowerParams;
    PlantLFFParams fruitParams;
    
    Vec4 trunkColorV;
    u32 trunkSubdivision;
    
    r32 lobeDepth;
    r32 lobes;
    
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

struct Plant
{
    b32 canRender;
    
    RandomSequence sequence;
    
    r32 cloneAccumulatedError[MAX_LEVELS];
    
    r32 scale;
    r32 lengthBase;
    
    PlantInstance plant;
    
    r32 age;
    r32 serverAge;
    
    r32 life;
    
    r32 leafDensity;
    r32 flowerDensity;
    r32 fruitDensity;
    
    AssetID leafBitmap;
    AssetID flowerBitmap;
    AssetID fruitBitmap;
    AssetID trunkBitmap;
    
    Plant* nextFree;
};
