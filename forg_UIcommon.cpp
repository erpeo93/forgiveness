enum TextOperation
{
    TextOp_draw,
    TextOp_getSize,
};

struct UIFont
{
    r32 fontScale;
    AssetID ID;
    Font* font;
    PAKFont* fontInfo;
    b32 drawShadow;
};

internal Rect2 UIOrthoTextOp(RenderGroup* group, AssetID fontID, Font* font, PAKFont* info, char* string, r32 fontScale, Vec3 P, TextOperation op, Vec4 color, b32 drawShadow)
{
    Rect2 result = InvertedInfinityRect2();
    
    if(font && info)
    {
        u32 prevCodePoint = 0;
        for( char* at = string; *at; at++)
        {
            u8 codePoint = *at;
            P.x += fontScale * GetHorizontalAdvanceForPair( font, info, prevCodePoint, codePoint );
            if( codePoint != ' ' )
            {
                BitmapId ID = GetBitmapForGlyph(group->assets, fontID, codePoint);
                
                if(IsValid(ID))
                {
                    PAKBitmap* glyphInfo = GetBitmapInfo(group->assets, ID);
                    r32 glyphHeight = fontScale * glyphInfo->dimension[1];
                    if( op == TextOp_draw )
                    {
                        if(drawShadow)
                        {
                            PushBitmap( group, FlatTransform(), ID, P + V3( 2.0f, -2.0f, -0.001f ), glyphHeight, V2( 1.0f, 1.0f ), V4( 0.0f, 0.0f, 0.0f, 1.0f ) );
                        }
                        
                        PushBitmap( group, FlatTransform(), ID, P, glyphHeight, V2( 1.0f, 1.0f ), color );
                    }
                    else
                    {
                        Assert( op == TextOp_getSize );
                        Bitmap* bitmap = GetBitmap( group->assets, ID);
                        if( bitmap )
                        {
                            BitmapDim dim = GetBitmapDim( bitmap, P, V3( 1.0f, 0.0f, 0.0f ), V3( 0.0f, 1.0f, 0.0f ), glyphHeight );
                            Rect2 glyphRect = RectMinDim(dim.P.xy, dim.size);
                            result = Union(result, glyphRect);
                            
                        }
                    }
                }
            }
            
            prevCodePoint = codePoint;
        }
        
    }
    
    return result;
}

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

inline Rect2 GetUIOrthoTextBounds(RenderGroup* group, UIFont* font, char* text, r32 fontScale, Vec2 screenP)
{
    Rect2 bounds = UIOrthoTextOp(group, font->ID, font->font, font->fontInfo, text, fontScale, V3(screenP, 0), TextOp_getSize, V4(1, 1, 1, 1), font->drawShadow);
    
    return bounds;
}

inline void PushUIOrthoText(RenderGroup* group, UIFont* font, char* text, r32 fontScale, Vec2 screenP, Vec4 color, r32 additionalZ = 0.0f)
{
    UIOrthoTextOp(group, font->ID, font->font, font->fontInfo, text, fontScale, V3(screenP, additionalZ), TextOp_draw, color, font->drawShadow);
}


inline void PushCenteredOrthoText(RenderGroup* group, UIFont* font, char* string, r32 fontScale, Vec2 screenP, r32 additionalZ, Vec4 color)
{
    Rect2 textRect = GetUIOrthoTextBounds(group, font, string, fontScale, screenP);
    Vec2 textCenter = GetCenter(textRect);
    Vec2 textDim = GetDim(textRect);
    
    UIOrthoTextOp(group, font->ID, font->font, font->fontInfo, string, fontScale, V3(screenP - (0.5f * textDim), additionalZ), TextOp_draw, color, font->drawShadow);
}

inline void PushUICenteredOrthoTextWithDimension(RenderGroup* group, UIFont* font, char* text, Vec2 screenP, Vec2 dim, r32 additionalZ, Vec4 color)
{
    Vec2 standardDim = GetDim(GetUIOrthoTextBounds(group, font, text, font->fontScale, screenP));
    r32 coeffX = dim.x / standardDim.x;
    r32 coeffY = dim.y / standardDim.y;
    r32 coeff = Min(coeffX, coeffY);
    PushCenteredOrthoText(group, font, text, font->fontScale * coeff, screenP, additionalZ, color);
}

#if RESTRUCTURING
internal Rect2 UITextOp(UIState* UI, RenderGroup* group, Font* font, PakFont* info, char* string, r32 fontScale, Vec2 PIn, Vec4 color, b32 drawShadow, b32 computeRect = false)
{
    Vec2 P = PIn;
    Rect2 result = InvertedInfinityRect2();
    if(font && info)
    {
        u8 prevCodePoint = 0;
        
        r32 lightZBias = UI->worldMode->cameraWorldOffset.z - 0.06f;
        r32 darkZBias = UI->worldMode->cameraWorldOffset.z - 0.07f;
        
        for( char* at = string; *at; at++)
        {
            u8 codePoint = *at;
            P.x += fontScale * GetHorizontalAdvanceForPair(font, info, prevCodePoint, codePoint);
            if( codePoint != ' ' )
            {
                BitmapId ID = GetBitmapForGlyph(group->assets, font, info, codePoint);
                if(IsValid(ID))
                {
                    PakBitmap* glyphInfo = GetBitmapInfo(group->assets, ID);
                    r32 glyphHeight = fontScale * glyphInfo->dimension[1];
                    if(computeRect)
                    {
                        Bitmap* bitmap = GetBitmap( group->assets, ID);
                        if(bitmap)
                        {
                            Vec3 worldP = GetWorldP(group, P);
                            BitmapDim dim = GetBitmapDim(bitmap, worldP, V3(1.0f, 0.0f, 0.0f), V3(0.0f, 1.0f, 0.0f), glyphHeight);
                            Rect2 glyphRect = RectMinDim(dim.P.xy, dim.size);
                            result = Union( result, glyphRect );
                        }
                    }
                    else
                    {
                        if(drawShadow)
                        {
                            PushUIBitmap(group, ID, P + V2(fontScale * 4.0f, 0), glyphHeight, 0, darkZBias, V2(1.0f, 1.0f), V4( 0.0f, 0.0f, 0.0f, 1.0f));
                        }
                        PushUIBitmap(group, ID, P + V2( 0, 0), glyphHeight, 0, lightZBias, V2( 1.0f, 1.0f), color);
                    }
                }
            }
            
            prevCodePoint = codePoint;
        }
    }
    
    return result;
}

inline Rect2 GetTextRect(UIState* UI, RenderGroup* group, Font* font, PakFont* info, char* string, r32 fontScale)
{
    Rect2 result = UITextOp(UI, group, font, info, string, fontScale, V2(0, 0), V4(1, 1, 1, 1), false, true);
    
    return result;
}

inline void PushCenteredText(UIState* UI, RenderGroup* group, Font* font, PakFont* info, char* string, r32 fontScale, Vec2 centerOffset, Vec4 color, b32 drawShadow)
{
    Rect2 textRect = GetTextRect(UI, group, font, info, string, fontScale);
    Vec2 textCenter = GetCenter(textRect);
    Vec2 textDim = GetDim(textRect);
    
    UITextOp(UI, group, font, info, string, fontScale, centerOffset - (0.5f * textDim),
             color, drawShadow, false);
}

inline void PushUIText_(UIState* UI, UIFont* font, char* text, Vec2 centerOffset, Vec4 color, r32 scale = 1.0f)
{
    PushCenteredText(UI, UI->group, font->font, font->fontInfo, text, font->fontScale * scale, centerOffset, color, font->drawShadow);
}

inline void PushUITooltip(UIState* UI, char* text, Vec4 color, b32 prefix = false, b32 suffix = false, r32 scale = 1.0f, b32 drawBackground = false)
{
    UI->prefix = prefix;
    UI->suffix = suffix;
    UI->drawTooltipBackground = drawBackground;
    FormatString(UI->tooltipText, sizeof(UI->tooltipText), "%s", text);
}

inline void PushUITextWithDimension(UIState* UI, UIFont* font, char* text, Vec2 centerOffset, Vec2 dim, Vec4 color)
{
    Vec2 standardDim = GetDim(GetTextRect(UI, UI->group, font->font, font->fontInfo, text, font->fontScale));
    r32 coeffX = dim.x / standardDim.x;
    r32 coeffY = dim.y / standardDim.y;
    r32 coeff = Min(coeffX, coeffY);
    PushUIText_(UI, font, text, centerOffset, color, coeff);
}
#endif
