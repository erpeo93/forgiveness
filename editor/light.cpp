inline void AddLight(TaxonomySlot* slot, r32 minIntensity, r32 maxIntensity, char* pieceName, Vec3 color)
{
    slot->minLightIntensity = minIntensity;
    slot->maxLightIntensity = maxIntensity;
    slot->hasLight = true;
    slot->lightColor = color;
    slot->lightPieceHashID = 0;
    if(pieceName)
    {
        slot->lightPieceHashID = StringHash(pieceName);
    }
}


internal void ImportLightTab(TaxonomySlot* slot, EditorElement* root)
{
    char* min = GetValue(root, "minIntensity");
    char* max = GetValue(root, "maxIntensity");
    char* pieceName = GetValue(root, "animationPieceName");
    AddLight(slot, ToR32(min), ToR32(max), pieceName, V3(1, 1, 1));
}