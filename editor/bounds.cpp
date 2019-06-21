
inline void DefineBounds_(char* boundsHeight_, char* boundsRadious_, ForgBoundType type)
{
    TaxonomySlot* slot = currentSlot_;
    
    r32 boundsRadious = ToR32(boundsRadious_);
    r32 boundsHeight = ToR32(boundsHeight_);
    
    Vec3 min = V3(-boundsRadious, -boundsRadious, 0);
    Vec3 max = V3(boundsRadious, boundsRadious, boundsHeight);
    
    slot->boundType = type;
    slot->physicalBounds = RectMinMax(min, max);
}

inline void DefineBounds(char* boundsHeight, char* boundsRadious)
{
    DefineBounds_(boundsHeight, boundsRadious, ForgBound_Standard);
}

inline void DefineNullBounds(char* boundsHeight, char* boundsRadious)
{
    DefineBounds_(boundsHeight, boundsRadious, ForgBound_NonPhysical);
}


internal void ImportBoundsTab(TaxonomySlot* slot, EditorElement* root)
{
    char* exists = GetValue(root, "physical");
    b32 physical = ToB32(exists);
    
    char* height = GetValue(root, "height");
    char* radious = GetValue(root, "radious");
    
    if(physical)
    {
        DefineBounds(height, radious);
    }
    else
    {
        DefineNullBounds(height, radious);
    }
    
    
    slot->scaleDimBasedOnIntensity = ToB32(GetValue(root, "scaleDimBasedOnGenIntensity"));
    slot->scaleDimGenCoeffV = ToR32(GetValue(root, "scaleDimGenCoeff"));
}



