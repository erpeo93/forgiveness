internal void ImportAnimationGeneralParamsTab(TaxonomySlot* slot, EditorElement* root)
{
    slot->animationIn3d = ToB32(GetValue(root, "animationIn3d"));        
    slot->animationFollowsVelocity = ToB32(GetValue(root, "animationFollowsVelocity"));        
    slot->modelTypeID = StringHash(GetValue(root, "modelType"));
    slot->modelNameID = StringHash(GetValue(root, "modelName"));
    slot->modelOffset = ToV3(GetStruct(root, "offset"));
    slot->modelColoration = ToV4Color(GetStruct(root, "coloration"));
    slot->modelScale = ToV3(GetStruct(root, "scale"), V3(1, 1, 1));
}