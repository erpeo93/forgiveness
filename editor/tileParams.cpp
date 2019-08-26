internal void ImportTileParamsTab(TaxonomySlot* slot, EditorElement* root)
{
    slot->tileDefinition = PushStruct(&currentSlot_->pool, TileDefinition);
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
    
    tile->textureSplashCount = ToU32(GetValue(root, "textureSplashCount"), 1);
    tile->splashOffsetV = Clamp01(ElemR32(root, "splashOffsetV", 0.5f));
    tile->splashAngleV = Clamp01(ElemR32(root, "splashAngleV"));
    tile->splashMinScale = Clamp01(ElemR32(root, "splashMinScale", 1.0f));
    tile->splashMaxScale = Clamp01(ElemR32(root, "splashMaxScale", 1.0f));
    
    tile->splashCount = 0;
    EditorElement* splashes = GetList(root, "splashes");
    while(splashes)
    {
        char* name = GetValue(splashes, "splashName");
        r32 weight = ElemR32(splashes, "weight", 1.0f);
        if(name)
        {
            if(tile->splashCount < ArrayCount(tile->splashes))
            {
                TileSplash* splash = tile->splashes + tile->splashCount++;
                
                splash->nameHash = StringHash(name);
                splash->weight = weight;
                tile->totalWeights += weight;
            }
        }
        
        splashes = splashes->next;
    }
}