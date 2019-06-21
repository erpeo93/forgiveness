internal void ImportTileParamsTab(TaxonomySlot* slot, EditorElement* root)
{
    
    slot->groundPointMaxOffset = ToR32(GetValue(root, "groundPointMaxOffset"));
    slot->chunkynessWithSame = ToR32(GetValue(root, "chunkynessSame"), 0.5f);
    slot->chunkynessWithOther = ToR32(GetValue(root, "chunkynessOther"), 0.5f);
    slot->groundPointPerTile = ToR32(GetValue(root, "pointsPerTile"));
    slot->groundPointPerTileV = ToR32(GetValue(root, "pointsPerTileV"));
    slot->tileColor = ToV4Color(GetElement(root, "color"));
    slot->colorDelta = ToV4Color(GetElement(root, "colorDelta"), V4(0, 0, 0, 0));
    slot->tileBorderColor = ToV4Color(GetElement(root, "borderColor"), V4(0, 0, 0, 0));
    slot->tilePointsLayout = GetValuePreprocessor(TilePointsLayout, GetValue(root, "tileLayout"));
    slot->colorRandomness = ToR32(GetValue(root, "colorRandomness"), 0.0f);
    slot->tileNoise = ParseNoiseParams(GetStruct(root, "noise"));
}