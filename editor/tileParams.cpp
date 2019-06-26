internal void ImportTileParamsTab(TaxonomySlot* slot, EditorElement* root)
{
    if(slot->tileDefinition)
    {
        FREELIST_DEALLOC(slot->tileDefinition, taxTable_->firstFreeTileDefinition);
        slot->tileDefinition = 0;
    }
    
    TAXTABLE_ALLOC(slot->tileDefinition, TileDefinition);
    TileDefinition* tile = slot->tileDefinition;
    
    tile->groundPointMaxOffset = ToR32(GetValue(root, "groundPointMaxOffset"));
    tile->chunkynessWithSame = ToR32(GetValue(root, "chunkynessSame"), 0.5f);
    tile->chunkynessWithOther = ToR32(GetValue(root, "chunkynessOther"), 0.5f);
    tile->groundPointPerTile = ToR32(GetValue(root, "pointsPerTile"));
    tile->groundPointPerTileV = ToR32(GetValue(root, "pointsPerTileV"));
    tile->tileColor = ToV4Color(GetElement(root, "color"));
    tile->colorDelta = ToV4Color(GetElement(root, "colorDelta"), V4(0, 0, 0, 0));
    tile->tileBorderColor = ToV4Color(GetElement(root, "borderColor"), V4(0, 0, 0, 0));
    tile->tilePointsLayout = GetValuePreprocessor(TilePointsLayout, GetValue(root, "tileLayout"));
    tile->colorRandomness = ToR32(GetValue(root, "colorRandomness"), 0.0f);
    tile->tileNoise = ParseNoiseParams(GetStruct(root, "noise"));
}