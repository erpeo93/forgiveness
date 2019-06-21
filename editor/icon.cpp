internal void ImportIconTab(TaxonomySlot* slot, EditorElement* root)
{
    slot->iconColor = ToV4Color(GetStruct(root, "standardColor"));
    slot->iconActiveColor = ToV4Color(GetStruct(root, "activeColor"));
    slot->iconHoverColor = ToV4Color(GetStruct(root, "hoverColor"));
    
    slot->iconModelTypeID = StringHash(GetValue(root, "modelType"));
    slot->iconModelNameID = StringHash(GetValue(root, "modelName"));
    slot->iconScale = ToV3(GetStruct(root, "modelScale"));
}