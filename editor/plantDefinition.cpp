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
        
        destParams->radiousIncreaseSpeed = ToR32(GetValue(levelParams, "radiousIncreaseSpeed"));
        destParams->lengthIncreaseSpeed = ToR32(GetValue(levelParams, "lengthIncreaseSpeed"));
        
        destParams->leafCount = Min(MAX_LEAFS_PER_STEM, ToU8(GetValue(levelParams, "leafCount")));
        destParams->allLeafsAtStemLength = ToR32(GetValue(levelParams, "allLeafsAtStemLength"), 0.5f);
    }
    else
    {
        *destParams = {};
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
    
    plant->leafColor = ToV4Color(GetStruct(root, "leafColor"));
    plant->leafColorV = ToV4Color(GetStruct(root, "leafColorV"));
    
    plant->leafDimSpeed = ToR32(GetValue(root, "leafDimSpeed"));
    plant->leafOffsetSpeed = ToR32(GetValue(root, "leafOffsetSpeed"));
    
    plant->leafScale = ToV2(GetStruct(root, "leafScale"));
    plant->leafScaleV = ToV2(GetStruct(root, "leafScaleV"));
    
    plant->leafOffsetV = ToV3(GetStruct(root, "leafOffsetV"));
    plant->leafAngleV = ToR32(GetValue(root, "leafAngleV"));
    
    plant->leafWindAngleV = ToR32(GetValue(root, "leafWindAngleV"));
    plant->leafWindDirectionV = ToR32(GetValue(root, "leafWindDirectionV"));
    
    plant->trunkColorV = ToV4Color(GetStruct(root, "trunkColorV"));
    plant->lobeDepth = ToR32(GetValue(root, "lobeDepth"));
    plant->lobes = ToR32(GetValue(root, "lobes"));
    
    plant->leafStringHash = StringHash(GetValue(root, "leafName"));
    plant->trunkStringHash = StringHash(GetValue(root, "trunkName"));
}