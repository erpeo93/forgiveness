
inline void UsesSkeleton(TaxonomySlot* slot, char* skeletonName, char* skinName, Vec4 defaultColoration, Vec2 originOffset)
{
    slot->skeletonHashID = StringHash(skeletonName);
    slot->skinHashID = StringHash(skinName);
    slot->defaultColoration = defaultColoration;
    slot->originOffset = originOffset;
}

internal void ImportSkeletonTab(TaxonomySlot* slot, EditorElement* root)
{
    char* skeleton = GetValue(root, "skeletonName");
    char* skin = GetValue(root, "skinName");
    Vec4 color = ToV4Color(GetStruct(root, "defaultColoration"));
    Vec2 originOffset = ToV2(GetStruct(root, "originOffset"));
    UsesSkeleton(slot, skeleton, skin, color, originOffset);
}