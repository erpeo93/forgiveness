struct FitTextRow
{
    Vec2 P;
    u32 characterCount;
};

struct FitTextResult
{
    FitTextRow row;
    r32 fontScale;
};


#if RESTRUCTURING
inline FitTextResult FitTextIntoRect(RenderGroup* group, UIFont* font, Rect2 rect, char* text, r32 desiredFontScale = 1.0f, r32 lineHeightCoeff = 1.2f)
{
    FitTextResult result = {};
    
    r32 fontScale = desiredFontScale;
    
    Vec2 rectDim = GetDim(rect);
    Vec2 rectCenter = GetCenter(rect);
    
    while(true)
    {
        r32 lineHeight = lineHeightCoeff * font->fontScale * fontScale;
        
        b32 fitsVertically = false;
        while(!fitsVertically)
        {
            Rect2 probe = TextOp(group, font->font, font->fontInfo, text, font->fontScale * fontScale, V3(rectCenter, 0), V4(1, 1, 1, 1), false, true);
            
            Vec2 probeDim = GetDim(probe);
            
            if(probeDim.y <= rectDim.y)
            {
                fitsVertically = true;
            }
            else
            {
                fontScale *= 0.9f;
            }
        }
        
        Rect2 probe = UITextOp(group, font->font, font->fontInfo, text, font->fontScale * fontScale, V3(rectCenter, 0), V4(1, 1, 1, 1), false, true);
        
        Vec2 probeDim = GetDim(probe);
        if(probeDim.x <= rectDim.x)
        {
            result.fontScale = fontScale;
            result.row.characterCount = StrLen(text);
            result.row.P = rectCenter;
            break;
        }
        else
        {
            fontScale *= 0.9f;
        }
    }
    
    return result;
}

struct FitProjectedModelResult
{
    Vec3 modelScale;
    Vec3 modelP;
};

inline FitProjectedModelResult FitProjectedModelIntoRect(UIState* UI, Vec3 P, Rect2 rect, ModelId MID, Vec3 desiredScale = V3(1, 1, 1))
{
    FitProjectedModelResult result = {};
    if(IsValid(MID))
    {
        PakModel* modelInfo = GetModelInfo(UI->group->assets, MID);
        
        Vec2 rectCenter = GetCenter(rect);
        Vec3 P3d = P + rectCenter.x * UI->group->gameCamera.X + rectCenter.y * UI->group->gameCamera.Y;
        
        r32 cameraZ;
        Rect2 projectedRef = ProjectOnScreenCameraAligned(UI->group, P, rect, &cameraZ);
        Vec2 refDim = GetDim(projectedRef);
        
        Vec3 refScale = modelInfo->dim;
        Vec3 scale = Hadamart(modelInfo->dim, desiredScale);
        while(true)
        {
            r32 ignoredCameraZ;
            Rect2 probeRect = ProjectOnScreen(UI->group, RectCenterDim(P3d, scale), &ignoredCameraZ);
            
            Vec2 probeDim = GetDim(probeRect);
            if(probeDim.x <= refDim.x && probeDim.y <= refDim.y)
            {
                result.modelScale = V3(scale.x / refScale.x, scale.y / refScale.y, scale.z /refScale.z);
                result.modelP = P3d;
                break;
            }
            else
            {
                scale *= 0.9f;
            }
        }
    }
    
    return result;
}
#endif
