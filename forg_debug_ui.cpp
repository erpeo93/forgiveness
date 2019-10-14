inline b32 InteractionsAreEqual(DebugInteraction A, DebugInteraction B)
{
    b32 result = (DebugIDAreEqual(A.ID, B.ID) && 
                  (A.type == B.type) && 
                  (A.target == B.target) && 
                  (A.generic == B.generic));
    return result;
}

inline b32 InteractionIsHot(DebugCollationState* collation, DebugInteraction B)
{
    b32 result = InteractionsAreEqual(collation->hotInteraction, B);
    
    if(B.type == DebugInteraction_none)
    {
        result = false;
    }
    return result;
}

inline DebugInteraction SetUInt32Interaction(DebugID ID, u32* target, u32 value)
{
    DebugInteraction result = {};
    result.ID = ID;
    result.type = DebugInteraction_setUInt32;
    result.target = target;
    result.UInt32 = value;
    
    return result;
}

inline DebugInteraction  SetPointerInteraction(DebugID ID, void** target, void* value)
{
    DebugInteraction result = {};
    result.ID = ID;
    result.type = DebugInteraction_setPointer;
    result.target = target;
    result.pointer = value;
    
    return result;
}

inline Rect2 TextOp(RenderGroup* group, EditorLayout* layout, char* string, Vec2 p, TextOperation op, Vec4 color = V4(1.0f, 1.0f, 1.0f, 1.0f), r32 scale = 1.0f)
{
    
    Rect2 result = InvertedInfinityRect2();
    if(layout->font)
    {
        result = PushText_(group, layout->fontID, layout->font, layout->fontInfo, 
                           string, V3(p.x, p.y, 0), layout->fontScale, 
                           op, color, false, true);
    }
    
    return result;
}

inline void TextLineAt(RenderGroup* group, EditorLayout* layout, char* string, Vec2 p, Vec4 color = V4(1.0f, 1.0f, 1.0f, 1.0f), r32 scale = 1.0f)
{
    TextOp(group, layout, string, p, TextOp_draw, color, scale);
}

inline Rect2 TextSize(RenderGroup* group, EditorLayout* layout, char* string)
{
    Rect2 result = TextOp(group, layout, string, V2(0, 0), TextOp_getSize);
    return result;
}

inline r32 GetLineAdvance(EditorLayout* layout)
{
    r32 result = layout->fontScale * GetLineAdvance(layout->fontInfo);
    return result;
}

inline r32 GetBaseline(EditorLayout* layout)
{
    r32 result = layout->fontScale * GetStartingLineY(layout->fontInfo);
    return result;
}

inline Layout BeginLayout(DebugState* debugState, EditorLayout* l, DebugCollationState* collation, Vec2 mouseP, Vec2 upperLeftCorner)
{
    Layout layout = {};
    return layout;
}

inline void EndLayout(Layout* layout)
{
    
}

inline LayoutElement BeginRectElement(Layout* layout, Vec2* dim)
{
    LayoutElement result = {};
    result.layout = layout;
    result.dim = dim;
    
    return result;
}

inline void MakeElementSizable(Layout* layout, LayoutElement* element)
{
    element->size = element->dim;
}

inline void DefaultInteraction(LayoutElement* element, DebugInteraction interaction)
{
    element->interaction = interaction;
}

inline void AdvanceElement(EditorLayout* layout, Rect2 elRect)
{
    layout->nextYDelta = Min(layout->nextYDelta, elRect.min.y - layout->At.y);
    if(layout->noLineFeed)
    {
        layout->At.x = elRect.max.x + layout->spacingX;
    }
    else
    {
        layout->At.y += layout->nextYDelta - layout->spacingY;
        layout->lineInitialized = false;
    }
}

inline void EndElement(DebugRenderGroup* group, LayoutElement* element)
{
    Layout* layout = element->layout;
    DebugState* debugState = layout->debugState;
    DebugCollationState* collation = layout->collation;
    
    if(!layout->lineInitialized)
    {
        layout->At.x = layout->baseCorner.x + layout->depth * layout->lineAdvance;
        layout->nextYDelta = 0;
        layout->lineInitialized = true;
    }
    
    r32 sizeHandlePixels = 4.0f;
    Vec2 frame = {};
    
    if(element->size)
    {
        frame.x = sizeHandlePixels;
        frame.y = sizeHandlePixels;
    }
    
    Vec2 totalDim = *element->dim + 2.0f * frame;
    
    Vec2 totalMinCorner = V2(layout->At.x, layout->At.y - totalDim.y);
    Vec2 totalMaxCorner = totalMinCorner + totalDim;
    
    Vec2 interiorMinCorner = totalMinCorner + frame;
    Vec2 interiorMaxCorner = interiorMinCorner + *element->dim;
    
    Rect2 totalBounds = RectMinMax(totalMinCorner, totalMaxCorner);
    element->bounds = RectMinMax(interiorMinCorner, interiorMaxCorner);
    
    if(element->interaction.type && PointInRect(element->bounds, layout->mouseP))
    {
        collation->nextHotInteraction = element->interaction;
    }
    
    if(element->size)
    {
        Rect2 sizeBounds = AddRadius(RectMinMax(V2(interiorMaxCorner.x, totalMinCorner.y),
                                                V2(totalMaxCorner.x, interiorMinCorner.y)), V2(4.0f, 4.0f));
        DebugInteraction sizeInteraction = {};
        sizeInteraction.type = DebugInteraction_resize;
        sizeInteraction.P = element->size;
        
        PushRect(group, FlatTransform(), sizeBounds, InteractionIsHot(collation, sizeInteraction) ? V4(1.0f, 1.0f, 0.0f, 1.0f) : V4(1.0f, 1.0f, 1.0f, 1.0f));
        
        if(PointInRect(sizeBounds, layout->mouseP))
        {
            collation->nextHotInteraction = sizeInteraction;
        }
    }
    
    AdvanceElement(layout, totalBounds);
}

internal Vec2 BasicTextElement(RenderGroup* group, EditorLayout* layout, char* text, DebugInteraction itemInteraction, Vec4 color = V4(0.8f, 0.8f, 0.8f, 1.0f), Vec4 hotColot = V4(1.0f, 1.0f, 1.0f, 1.0f), r32 border = 0.0f, Vec4 backdropColor = V4(0, 0, 0, 0))
{
    
    DebugState* debugState = layout->debugState;
    Rect2 textBounds = TextSize(group, layout->debugState, text);
    Vec2 dim = V2(GetDim(textBounds).x, layout->lineAdvance);
    dim += 2.0f * V2(border, border);
    
    LayoutElement element = BeginRectElement(layout, &dim);
    DefaultInteraction(&element, itemInteraction);
    EndElement(&element);
    
    if(backdropColor.w > 0)
    {
        PushRect(&debugState->renderGroup, FlatTransform(), element.bounds, backdropColor);
    }
    
    b32 hot = InteractionIsHot(layout->collation, itemInteraction);
    TextLineAt(debugState, text, V2(element.bounds.min.x + border, element.bounds.max.y - border - GetStartingLineY(debugState->debugFontInfo)), hot ? hotColot : color);
    
    return dim;
}

internal void BeginRow(Layout* layout)
{
    ++layout->noLineFeed;
    LayoutElement result = {};
}

internal void ActionButton(Layout* layout, char* name, DebugInteraction interaction)
{
    Vec2 dim = BasicTextElement(layout, name, interaction, V4(0.5f, 0.5f, 0.5f, 1.0f), V4(1.0f, 1.0f, 1.0f, 1.0f), 4.0f, V4(0.0f, 0.5f, 1.0f, 1.0f));
    
}

internal void BooleanButton(Layout* layout, char* name, b32 highlight, DebugInteraction interaction)
{
    Vec2 dim = BasicTextElement(layout, name, interaction, highlight ? V4(1.0f, 1.0f, 1.0f, 1.0f) : V4(0.5f, 0.5f, 0.5f, 1.0f), V4(1.0f, 1.0f, 1.0f, 1.0f), 4.0f, V4(0.0f, 0.5f, 1.0f, 1.0f));
}

internal void EndRow(Layout* layout)
{
    Assert(layout->noLineFeed > 0);
    --layout->noLineFeed;
    
    AdvanceElement(layout, RectMinMax(layout->At, layout->At));
}

internal void AddTooltip(DebugState* debugState, char* string)
{
    Layout* layout = &debugState->mouseTextLayout;
    RenderGroup* renderGroup = &debugState->renderGroup;
    
    Rect2 textBounds = TextSize(layout->debugState, string);
    Vec2 dim = V2(GetDim(textBounds).x, layout->lineAdvance);
    
    LayoutElement element = BeginRectElement(layout, &dim);
    EndElement(&element);
    
    TransientClipRect(&debugState->renderGroup, GetScreenRect(&debugState->renderGroup));
    TextLineAt(debugState, string, V2(element.bounds.min.x, element.bounds.max.y - GetStartingLineY(debugState->debugFontInfo)));
}