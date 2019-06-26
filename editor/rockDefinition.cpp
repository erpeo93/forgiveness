internal void ImportRockDefinitionTab(TaxonomySlot* slot, EditorElement* root)
{
    if(slot->rockDefinition)
    {
        if(slot->rockDefinition->firstPossibleMineral)
        {
            FREELIST_FREE(slot->rockDefinition->firstPossibleMineral, RockMineral,  taxTable_->firstFreeRockMineral);
        }
        
        FREELIST_DEALLOC(slot->rockDefinition, taxTable_->firstFreeRockDefinition);
        slot->rockDefinition = 0;
    }
    
    TAXTABLE_ALLOC(slot->rockDefinition, RockDefinition);
    RockDefinition* definition = slot->rockDefinition;
    
    definition->collides = ToB32(GetValue(root, "collides"));
    definition->modelTypeHash = StringHash(GetValue(root, "modelType"));
    definition->modelNameHash = StringHash(GetValue(root, "modelName"));
    definition->color = ToV4Color(GetStruct(root, "color"));
    definition->startingColorDelta = ToV4Color(GetStruct(root, "startingColorDelta"));
    definition->perVertexColorDelta = ToV4Color(GetStruct(root, "perVertexColorDelta"));
    definition->iterationCount = ToU32(GetValue(root, "iterations"));
    definition->minDisplacementY = ToR32(GetValue(root, "minDisplacementY"));
    definition->maxDisplacementY = ToR32(GetValue(root, "maxDisplacementY"));
    definition->minDisplacementZ = ToR32(GetValue(root, "minDisplacementZ"));
    definition->maxDisplacementZ = ToR32(GetValue(root, "maxDisplacementZ"));
    definition->smoothness = ToR32(GetValue(root, "smoothness"));
    definition->smoothnessDelta = ToR32(GetValue(root, "smoothnessDelta"));
    definition->scale = ToV3(GetStruct(root, "scale"));
    definition->scaleDelta = ToV3(GetStruct(root, "scaleDelta"));
    
    definition->percentageOfMineralVertexes = ToR32(GetValue(root, "percentageOfMineralVertexes"));
    definition->mineralCount = 0;
    EditorElement* minerals = GetList(root, "minerals");
    while(minerals)
    {
        RockMineral* mineral;
        TAXTABLE_ALLOC(mineral, RockMineral);
        
        mineral->lerp = ToR32(GetValue(minerals, "lerp"));
        mineral->lerpDelta = ToR32(GetValue(minerals, "lerpDelta"));
        mineral->color = ToV4Color(GetStruct(minerals, "color"));
        
        FREELIST_INSERT(mineral, definition->firstPossibleMineral);
        ++definition->mineralCount;
        
        minerals = minerals->next;
    }
    
    definition->renderingRocksCount = ToU32(GetValue(root, "renderingRocksCount"));
    definition->renderingRocksDelta = ToU32(GetValue(root, "renderingRocksDelta"));
    definition->renderingRocksRandomOffset = ToV3(GetStruct(root, "renderingRocksOffset"));
    definition->scaleRandomness = ToR32(GetValue(root, "scaleRandomness"));
    
}