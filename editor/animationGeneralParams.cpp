inline void UsesSkeleton(TaxonomySlot* slot, char* skeletonName, char* skinName, Vec4 defaultColoration, Vec2 originOffset, b32 flippedOnYAxis)
{
    char skeletonSkin[128];
    FormatString(skeletonSkin, sizeof(skeletonSkin), "%s%s", skeletonName, skinName);
    slot->skeletonSkinHashID = StringHash(skeletonSkin);
    slot->skeletonHashID = StringHash(skeletonName);
    slot->skinHashID = StringHash(skinName);
    slot->defaultColoration = defaultColoration;
    slot->originOffset = originOffset;
    slot->flippedOnYAxis = flippedOnYAxis;
}

internal void ImportAnimationGeneralParamsTab(TaxonomySlot* slot, EditorElement* root)
{
    slot->animationIn3d = ToB32(GetValue(root, "animationIn3d"));        
    slot->animationFollowsVelocity = ToB32(GetValue(root, "animationFollowsVelocity"));        
    slot->modelTypeID = StringHash(GetValue(root, "modelType"));
    slot->modelNameID = StringHash(GetValue(root, "modelName"));
    slot->modelOffset = ToV3(GetStruct(root, "offset"));
    slot->modelColoration = ToV4Color(GetStruct(root, "coloration"));
    slot->modelScale = ToV3(GetStruct(root, "scale"), V3(1, 1, 1));
    
    char* skeleton = GetValue(root, "skeletonName");
    char* skin = GetValue(root, "skinName");
    Vec4 color = ToV4Color(GetStruct(root, "defaultColoration"));
    Vec2 originOffset = ToV2(GetStruct(root, "originOffset"));
    b32 flippedOnYAxis = ToB32(GetValue(root, "flippedOnYAxis"), false);
    UsesSkeleton(slot, skeleton, skin, color, originOffset, flippedOnYAxis);
}