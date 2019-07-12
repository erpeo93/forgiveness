inline void ParsePlantLevelParams(PlantLevelParams* destParams, EditorElement* levelParams)
{
    if(levelParams)
    {
        destParams->curveRes = ToU8(GetValue(levelParams, "curveRes"));
        destParams->curveBack = ToR32(GetValue(levelParams, "curveBack"));
        destParams->curve = ToR32(GetValue(levelParams, "curve"));
        destParams->curveV = ToR32(GetValue(levelParams, "curveV"));
        
        destParams->segSplits = ToR32(GetValue(levelParams, "segSplits"));
        destParams->baseSplits = ToR32(GetValue(levelParams, "baseSplits"));
        
        destParams->splitAngle = ToR32(GetValue(levelParams, "splitAngle"));
        destParams->splitAngleV = ToR32(GetValue(levelParams, "splitAngleV"));
        
        destParams->branches = ToR32(GetValue(levelParams, "branches"));
        destParams->branchesV = ToR32(GetValue(levelParams, "branchesV"));
        destParams->downAngle = ToR32(GetValue(levelParams, "downAngle"));destParams->downAngleV = ToR32(GetValue(levelParams, "downAngleV"));
        destParams->rotate = ToR32(GetValue(levelParams, "rotate"));destParams->rotateV = ToR32(GetValue(levelParams, "rotateV"));
        destParams->lengthCoeff = ToR32(GetValue(levelParams, "lengthCoeff"));
        destParams->lengthCoeffV = ToR32(GetValue(levelParams, "lengthCoeffV"));
        destParams->taper = ToR32(GetValue(levelParams, "taper"));
        destParams->radiousMod = ToR32(GetValue(levelParams, "radiousMod"), 1.0f);
        destParams->clonePercRatio = ToR32(GetValue(levelParams, "clonePercRatio"), 0.5f);
        destParams->clonePercRatioV = ToR32(GetValue(levelParams, "clonePercRatioV"), 0.0f);
        
        destParams->baseYoungColor = ToV4Color(GetStruct(levelParams, "baseYoungColor"));
        destParams->topYoungColor = ToV4Color(GetStruct(levelParams, "topYoungColor"));
        destParams->baseOldColor = ToV4Color(GetStruct(levelParams, "baseOldColor"));
        destParams->topOldColor = ToV4Color(GetStruct(levelParams, "topOldColor"));
        
        destParams->radiousIncreaseSpeedBeforeClones = ToR32(GetValue(levelParams, "radiousSpeedBeforeClones"));
        destParams->lengthIncreaseSpeedBeforeClones = ToR32(GetValue(levelParams, "lengthSpeedBeforeClones"));
        
        destParams->radiousIncreaseSpeedAfterClones = ToR32(GetValue(levelParams, "radiousSpeedAfterClones"));
        destParams->lengthIncreaseSpeedAfterClones = ToR32(GetValue(levelParams, "lengthSpeedAfterClones"));
        
        destParams->normClonesSpawnAtLength = Clamp01(ElemR32(levelParams, "normClonesSpawnAtLength"));
        
        destParams->leafCount = Min(MAX_LFF_PER_STEM, ToU8(GetValue(levelParams, "leafCount")));
        destParams->allLeafsAtStemLength = ToR32(GetValue(levelParams, "allLeafsAtStemLength"), 0.5f);
        
        destParams->flowerCount = Min(MAX_LFF_PER_STEM, ToU8(GetValue(levelParams, "flowerCount")));
        destParams->allFlowersAtStemLength = ToR32(GetValue(levelParams, "allFlowersAtStemLength"), 0.5f);
        
        destParams->fruitCount = Min(MAX_LFF_PER_STEM, ToU8(GetValue(levelParams, "fruitCount")));
        destParams->allFruitsAtStemLength = ToR32(GetValue(levelParams, "allFruitsAtStemLength"), 0.5f);
    }
    else
    {
        *destParams = {};
    }
}


inline void ParseLFFSeasonParams(PlantLFFSeasonParams* params, EditorElement* root)
{
    if(root)
    {
        params->aliveColor = ToV4Color(GetStruct(root, "color"));
        params->deadColor = ToV4Color(GetStruct(root, "color"));
        params->colorV = ToV4Color(GetStruct(root, "colorV"));
        
        params->scale = ToV2(GetStruct(root, "scale"));
        params->scaleV = ToV2(GetStruct(root, "scaleV"));
        
        params->densityAtMidSeason = ElemR32(root, "densityAtMidSeason");
    }
}

inline void ParsePlantLFFParams(PlantLFFParams* params, EditorElement* root)
{
    if(root)
    {
        params->dimSpeed = ToR32(GetValue(root, "dimSpeed"));
        params->offsetSpeed = ToR32(GetValue(root, "offsetSpeed"));
        
        params->offsetV = ToV3(GetStruct(root, "offsetV"));
        params->angleV = ToR32(GetValue(root, "angleV"));
        
        params->windAngleV = ToR32(GetValue(root, "windAngleV"));
        params->windDirectionV = ToR32(GetValue(root, "windDirectionV"));
        
        params->bitmapHash = StringHash(GetValue(root, "imageName"));
        
        ParseLFFSeasonParams(params->seasons + Season_Autumn, GetStruct(root, "autumn"));
        ParseLFFSeasonParams(params->seasons + Season_Winter, GetStruct(root, "winter"));
        ParseLFFSeasonParams(params->seasons + Season_Spring, GetStruct(root, "spring"));
        ParseLFFSeasonParams(params->seasons + Season_Summer, GetStruct(root, "summer"));
    }
}

internal void ImportPlantDefinitionTab(TaxonomySlot* slot, EditorElement* root)
{
    if(slot->plantDefinition)
    {
        TAXTABLE_DEALLOC(slot->plantDefinition, PlantDefinition);
    }
    TAXTABLE_ALLOC(slot->plantDefinition, PlantDefinition);
    PlantDefinition* plant = slot->plantDefinition;
    
    plant->collides = ToB32(GetValue(root, "collides"));
    plant->shape = (PlantShape) GetValuePreprocessor(PlantShape, GetValue(root, "shape"));
    
    plant->growingCoeff = ToR32(GetValue(root, "growingCoeff"), 1.0f);
    
    plant->plantCount = ToU32(GetValue(root, "plantCount"));
    plant->plantCountV = ToR32(GetValue(root, "plantCountV"));
    
    plant->plantOffsetV = ToV2(GetStruct(root, "plantOffsetV"));
    plant->plantAngleZV = ToR32(GetValue(root, "plantAngleZV"));
    
    plant->attractionUp = ToR32(GetValue(root, "attractionUp"));
    
    plant->maxLevels = Min(4, ToU8(GetValue(root, "levels"), 1));
    plant->baseSize = ToR32(GetValue(root, "baseSize"));
    plant->scale = ToR32(GetValue(root, "scale"));
    plant->scaleV = ToR32(GetValue(root, "scaleV"));
    plant->scale_0 = ToR32(GetValue(root, "scale_0"));
    plant->scaleV_0 = ToR32(GetValue(root, "scaleV_0"));
    plant->ratio = ToR32(GetValue(root, "ratio"));
    plant->ratioPower = ToR32(GetValue(root, "ratioPower"));
    plant->flare = ToR32(GetValue(root, "flare"));
    
    ParsePlantLevelParams(plant->levelParams + 0, GetStruct(root, "level0"));
    ParsePlantLevelParams(plant->levelParams + 1, GetStruct(root, "level1"));
    ParsePlantLevelParams(plant->levelParams + 2, GetStruct(root, "level2"));
    ParsePlantLevelParams(plant->levelParams + 3, GetStruct(root, "level3"));
    
    ParsePlantLFFParams(&plant->leafParams, GetStruct(root, "leaf"));
    ParsePlantLFFParams(&plant->flowerParams, GetStruct(root, "flower"));
    ParsePlantLFFParams(&plant->fruitParams, GetStruct(root, "fruit"));
    
    plant->trunkColorV = ToV4Color(GetStruct(root, "trunkColorV"));
    plant->lobeDepth = ToR32(GetValue(root, "lobeDepth"));
    plant->lobes = ToR32(GetValue(root, "lobes"));
    plant->trunkStringHash = StringHash(GetValue(root, "trunkName"));
}