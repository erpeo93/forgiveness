internal void EditStruct(EditorLayout* layout, String structName, void* structPtr)
{
    ReservedSpace ignored = {};
    StructOperation(layout, structName, 0, structPtr, FieldOperation_Edit, 0, &ignored);
}

internal Rect2 EditorTextDraw(EditorLayout* layout, Vec4 color, u32 flags, char* format, ...)
{
    va_list argList;
    va_start(argList, format);
    FormatStringList(layout->buffer, layout->bufferSize, format, argList);
    va_end(argList);
    
    r32 z = (flags & EditorText_OnTop) ? 1.0f : 0.0f;
    b32 startingSpace = (flags & EditorText_StartingSpace);
    
    Vec3 P = V3(layout->currentP, z);
    Rect2 textDim = InvertedInfinityRect2();
    char* text = layout->buffer;
    textDim = GetTextDim(layout->group, layout->fontID, text, P, layout->fontScale, startingSpace);
    
    if(flags & EditorText_DarkBackground)
    {
        Vec2 padding = layout->fontScale * V2(4, 8);
        textDim.min -= padding;
        textDim.max += padding;
        PushRect(layout->group, FlatTransform(), textDim, V4(0.02f, 0.02f, 0.02f, 1.0f));
    }
    
    PushText(layout->group, layout->fontID, text, P, layout->fontScale, color, startingSpace);
    
    layout->lastP = layout->currentP;
    layout->currentP.x = textDim.max.x;
    
    return textDim;
}


r32 RawHeight(EditorLayout* layout)
{
    r32 result = layout->fontScale * GetLineAdvance(layout->font);
    return result;
}

internal void NextRaw(EditorLayout* layout)
{
    layout->rawP.y -= RawHeight(layout);
    layout->lastP = layout->currentP;
    
    layout->currentP = layout->rawP;
}

internal void SetRawP(EditorLayout* layout, r32 X)
{
    layout->rawP.x = X;
}

internal void Push(EditorLayout* layout)
{
    layout->rawP.x += layout->fontScale * layout->horizontalAdvance;
    layout->lastP = layout->currentP;
    layout->currentP = layout->rawP;
}

internal void Nest(EditorLayout* layout)
{
    Push(layout);
    NextRaw(layout);
}

internal void Pop(EditorLayout* layout)
{
    layout->rawP.x -= layout->fontScale * layout->horizontalAdvance;
    layout->lastP = layout->currentP;
    layout->currentP = layout->rawP;
}

internal Vec4 StandardTextColor()
{
    Vec4 result = V4(1, 1, 1, 1);
    return result;
}

internal Vec4 DefaultEditorStringColor()
{
    Vec4 result = V4(1, 0, 1, 1);
    return result;
}
internal Vec2 ButtonDim(EditorLayout* layout)
{
    Vec2 result = layout->fontScale * V2(2.0f * layout->standardButtonDim, layout->standardButtonDim);
    return result;
}

internal Vec2 CollapsibleDim(EditorLayout* layout)
{
    Vec2 result = layout->fontScale * 0.5f * V2(layout->standardButtonDim, layout->standardButtonDim);
    
    return result;
}

inline b32 IsHotAUID(EditorUIContext* context, AUID ID)
{
    b32 result = AreEqual(context->hot, ID);
    return result;
}


#define GetUIButton(context, button) &(context->input->button)
#define UIPressed(context, button) Pressed(GetUIButton(context, button))
#define UIReleased(context, button) Released(GetUIButton(context, button))

#define HotAUIDAndPressed(context, ID, button) HotAUIDAndPressed_(context, ID, GetUIButton(context, button))
internal b32 HotAUIDAndPressed_(EditorUIContext* context, AUID ID, PlatformButton* button)
{
    b32 result = IsHotAUID(context, ID) && Pressed(button);
    return result;
    
}

inline void SetNextHotAUID(EditorUIContext* context, AUID ID)
{
    context->nextHot = ID;
}

inline void SetInteractiveAUID(EditorUIContext* context, AUID ID)
{
    context->interactive = ID;
}

inline void EndInteraction(EditorUIContext* context)
{
    context->interactive = {};
}

inline b32 IsInteractiveAUID(EditorUIContext* context, AUID ID)
{
    b32 result = AreEqual(context->interactive, ID);
    return result;
}

inline u32 AUIDHashIndex(AUIDStorage* storage, AUID ID)
{
    Assert(IsPowerOf2(ArrayCount(storage->data)));
    
    u32 rawIndex = (u32) ((u64) ID.p1 + (u64) ID.p2 + (u64) ID.p3);
    u32 result = rawIndex & (ArrayCount(storage->data) - 1);
    
    return result;
}

internal AUIDData* GetAUIDData(EditorUIContext* context, AUID ID)
{
    AUIDStorage* storage = &context->storage;
    
    AUIDData* result = 0;
    
    u32 hashIndex = AUIDHashIndex(storage, ID);
    for(AUIDData* data = storage->data[hashIndex]; data; data = data->next)
    {
        if(AreEqual(data->ID, ID))
        {
            result = data;
            break;
        }
    }
    
    if(!result)
    {
        result = PushStruct(context->pool, AUIDData);
        result->ID = ID;
        result->next = storage->data[hashIndex];
        storage->data[hashIndex] = result;
    }
    
    
    return result;
}


internal Rect2 EditorElementName(EditorLayout* layout, char* name, b32 colon = true)
{
    Vec4 color = V4(1, 1, 0.5f, 1);
    Rect2 result = EditorTextDraw(layout, color, EditorText_StartingSpace, name);
    
    if(colon)
    {
        EditorTextDraw(layout, color, 0, ":");
    }
    
    return result;
}

internal Rect2 ShowString(EditorLayout* layout, char* name, char* string, u32 flags, Vec4 color = DefaultEditorStringColor())
{
    if(name){EditorElementName(layout, name);}
    Rect2 result = EditorTextDraw(layout, color, flags, "%s", string);
    return result;
}

#define ShowName(layout, name)if(name){EditorElementName(layout, name);}
#define ShowStandard(layout, color, format, ...) EditorTextDraw(layout, color, EditorText_StartingSpace, format, ##__VA_ARGS__)

internal b32 EditString(EditorLayout* layout, char* name, char* string, AUID ID, StringArray options, char* outputBuffer, u32 outputLength)
{
    b32 result = false;
    b32 showOptions = false;
    
    AUIDData* data = GetAUIDData(layout->context, ID);
    Vec4 color = DefaultEditorStringColor();
    if(options.count)
    {
        if(PointInRect(data->dim, layout->mouseP))
        {
            SetNextHotAUID(layout->context, ID);
            color = V4(1, 1, 0, 1);
        }
        
        
        if(HotAUIDAndPressed(layout->context, ID, mouseLeft))
        {
            SetInteractiveAUID(layout->context, ID);
        }
        
        if(IsInteractiveAUID(layout->context, ID))
        {
            color = V4(0, 1, 0, 1);
            
            i32 optionIndex = (i32) data->optionIndex;
            if(UIPressed(layout->context, actionUp))
            {
                --optionIndex;
            }
            
            if(UIPressed(layout->context, actionDown))
            {
                ++optionIndex;
            }
            
            optionIndex = Wrap(0, optionIndex, (i32) options.count); 
            Assert(optionIndex >= 0);
            
            data->optionIndex = (u32) optionIndex;
            
            showOptions = true;
            if(UIPressed(layout->context, confirmButton))
            {
                EndInteraction(layout->context);
                FormatString(outputBuffer, outputLength, "%s", options.strings[data->optionIndex]);
                result = true;
            }
        }
    }
    
    data->dim = ShowString(layout, name, string, EditorText_StartingSpace, color);
    
    if(showOptions)
    {
        EditorLayout optionLayout = *layout;
        SetRawP(&optionLayout, layout->lastP.x);
        
        for(u32 optionIndex = 0; optionIndex < options.count; ++optionIndex)
        {
            NextRaw(&optionLayout);
            Vec4 optionColor = (data->optionIndex == optionIndex) ? V4(0, 0, 1, 1) : DefaultEditorStringColor();
            ShowString(&optionLayout, 0, options.strings[optionIndex], EditorText_OnTop | EditorText_StartingSpace | EditorText_DarkBackground, optionColor);
        }
    }
    
    return result;
}

internal b32 EditString(EditorLayout* layout, char* name, char* string, StringArray options, char* output, u32 outputLength)
{
    b32 result = EditString(layout, name, string, auID(string), options, output, outputLength);
    return result;
}


internal void EditU32(EditorLayout* layout, char* name, u32* number)
{
    AUID ID = auID(number);
    
    Vec4 color = V4(1, 0, 0, 1);
    
    ShowName(layout, name);
    ShowStandard(layout, color, "%d", *number);
}

internal void EditU16(EditorLayout* layout, char* name, u16* number)
{
    AUID ID = auID(number);
    
    Vec4 color = V4(1, 0, 0, 1);
    ShowName(layout, name);
    ShowStandard(layout, color, "%d", *number);
}


internal void EditR32(EditorLayout* layout, char* name, r32* number)
{
    AUID ID = auID(number);
    AUIDData* data = GetAUIDData(layout->context, ID);
    
    
    Vec4 color = V4(1, 0.5f, 0, 1);
    if(PointInRect(data->dim, layout->mouseP))
    {
        color = V4(1, 0.0f, 1.0f, 1.0f);
        SetNextHotAUID(layout->context, ID);
    }
    
    if(HotAUIDAndPressed(layout->context, ID, mouseLeft))
    {
        SetInteractiveAUID(layout->context, ID);
    }
    
    
    if(IsInteractiveAUID(layout->context, ID))
    {
        if(UIReleased(layout->context, mouseLeft))
        {
            EndInteraction(layout->context);
        }
        else
        {
            data->speed += 0.005f * layout->deltaMouseP.x;
            data->speed = Clamp(0.05f, data->speed, 3.0f);
            r32 delta = data->speed * layout->deltaMouseP.y;
            *number += delta;
        }
    }
    
    
    
    ShowName(layout, name);
    Rect2 elementRect = ShowStandard(layout, color, "%f", *number);
    data->dim = elementRect;
}

internal void EditVec2(EditorLayout* layout, char* name, Vec2* v)
{
    AUID ID = auID(v);
    
    EditorElementName(layout, name);
    EditR32(layout, "x", &v->x);
    EditR32(layout, "y", &v->y);
}

internal void EditVec3(EditorLayout* layout, char* name, Vec3* v)
{
    AUID ID = auID(v);
    
    EditorElementName(layout, name);
    EditR32(layout, "x", &v->x);
    EditR32(layout, "y", &v->y);
    EditR32(layout, "z", &v->z);
}

internal void EditVec4(EditorLayout* layout, char* name, Vec4 * v)
{
    AUID ID = auID(v);
    
    EditorElementName(layout, name);
    EditR32(layout, "x", &v->x);
    EditR32(layout, "y", &v->y);
    EditR32(layout, "z", &v->z);
    EditR32(layout, "w", &v->w);
}


internal b32 EditorButton(EditorLayout* layout, Vec2 rawOffset, Vec2 buttonDim, char* name, AUID ID, Vec4 color = V4(1, 0, 0, 1))
{
    b32 result = false;
    Vec2 buttonMin = layout->currentP + Hadamart(rawOffset, V2(RawHeight(layout), RawHeight(layout)));
    Rect2 button = RectMinDim(buttonMin, buttonDim);
    
    if(PointInRect(button, layout->mouseP))
    {
        SetNextHotAUID(layout->context, ID);
    }
    else
    {
        color.a = 0.5f * color.a;
    }
    
    if(HotAUIDAndPressed(layout->context, ID, mouseLeft))
    {
        result = true;
    }
    
    layout->currentP.x += 1.2f* buttonDim.x;
    
    PushRect(layout->group, FlatTransform(), button, color);
    if(name)
    {
        PushTextEnclosed(layout->group, layout->fontID, name, button, layout->fontScale, StandardTextColor());
    }
    
    return result;
}

internal b32 StandardEditorButton(EditorLayout* layout, char* name, AUID ID, Vec4 color = V4(1, 0, 0, 1))
{
    Vec2 buttonDim = ButtonDim(layout);
    b32 result = EditorButton(layout, V2(0.25f, -0.1f), buttonDim, name, ID);
    return result;
}

internal b32 EditorCollapsible(EditorLayout* layout, char* string, AUID ID)
{
    AUIDData* data = GetAUIDData(layout->context, ID);
    
    Vec4 collapsibleColor = data->show ? V4(0, 1, 0, 1) : V4(0, 0, 1, 1);
    Vec2 collapseDim = CollapsibleDim(layout);
    if(EditorButton(layout, V2(0, 0), collapseDim, 0, ID, collapsibleColor))
    {
        data->show = !data->show;
    }
    
    if(string)
    {
        EditorElementName(layout, string, false);
    }
    
    b32 result = data->show;
    
    return result;
}

internal b32 EditorCollapsible(EditorLayout* layout, char* string)
{
    b32 result = EditorCollapsible(layout, string, auID(string));
    return result;
}

internal void RenderAndEditAsset(EditorLayout* layout, Assets* assets, AssetID ID)
{
    GetGameAssetResult get = GetGameAsset(assets, ID);
    PAKAsset* info = get.info;
    
    b32 showAssetInfo = EditorCollapsible(layout, info->sourceName);
    switch(ID.type)
    {
        case AssetType_Image:
        {
            if(get.derived)
            {
                
            }
            else
            {
            }
        } break;
        
        case AssetType_Sound:
        {
            AUID auid = auID(info, "sound");
            
            if(StandardEditorButton(layout, "play", auid))
            {
                EditorUIContext* UI = layout->context;
                if(UI->playingSound)
                {
                    ChangeVolume(UI->soundState, UI->playingSound, 0, V2(0, 0));
                }
                UI->playingSound = PlaySound(UI->soundState, assets, ID, 0);
            }
        } break;
        
        case AssetType_Font:
        {
            
        } break;
        
        case AssetType_Model:
        {
            
        } break;
        
        case AssetType_Skeleton:
        {
            
        } break;
        
        case AssetType_Invalid:
        case AssetType_Count:
        {
            InvalidCodePath;
        } break;
        
        default:
        {
        } break;
    }
    
    if(showAssetInfo)
    {
        switch(ID.type)
        {
            case AssetType_Image:
            {
                if(get.derived)
                {
                    
                }
                else
                {
                }
            } break;
            
            case AssetType_Sound:
            {
                Nest(layout);
                PAKSound* sound = &info->sound;
                ShowName(layout, "total samples");
                ShowStandard(layout, StandardTextColor(), "%d", sound->sampleCount);
                
                ShowName(layout, "channels");
                ShowStandard(layout, StandardTextColor(), "%d", sound->channelCount);
                Pop(layout);
            } break;
            
            case AssetType_Font:
            {
                
            } break;
            
            case AssetType_Model:
            {
                
            } break;
            
            case AssetType_Skeleton:
            {
                
            } break;
            
            case AssetType_Invalid:
            case AssetType_Count:
            {
                InvalidCodePath;
            } break;
            
            default:
            {
                Asset* asset = GetGameAsset(assets, ID).asset;
                if(asset)
                {
                    MetaAsset metaAssetType = metaAsset_assetType[ID.type];
                    String structName = {};
                    structName.ptr = metaAssetType.name;
                    structName.length = StrLen(metaAssetType.name);
                    
                    Nest(layout);
                    EditStruct(layout, structName, asset->data);
                    Pop(layout);
                }
                else
                {
                    LoadDataFile(assets, ID);
                }
            } break;
        }
        
        
        
        
        Nest(layout);
        if(EditorCollapsible(layout, "labels", auID(info, "labels")))
        {
            Push(layout);
            for(u32 labelIndex = 0; labelIndex < ArrayCount(info->labels); ++labelIndex)
            {
                PAKLabel* label = info->labels + labelIndex;
                if(label->label)
                {
                    NextRaw(layout);
                    
                    char* labelType = GetMetaLabelTypeName(label->label);
                    char* labelValue = GetMetaLabelValueName(label->label, label->value);
                    
                    StringArray options = GetLabelValueList(label->label);
                    
                    char outputBuffer[32];
                    if(EditString(layout, labelType, labelValue, auID(info, "labels", (void*) labelIndex), options, outputBuffer, sizeof(outputBuffer)))
                    {
                        Token newValue = Tokenize(outputBuffer);
                        u16 value = ExistMetaLabelValue(label->label, newValue);
                        label->value = value;
                    }
                }
            }
            Pop(layout);
        }
        Pop(layout);
    }
}

internal void RenderEditAssetFile(EditorLayout* layout, Assets* assets, u32 fileIndex)
{
    PAKFileHeader* file = GetFileInfo(assets, fileIndex);
    
    b32 showAssetData = EditorCollapsible(layout, file->name);
    
    AUID saveID = auID(file, "saveButton");
    if(EditorButton(layout, V2(0.25f, -0.1f), ButtonDim(layout), "save", saveID))
    {
        WritebackAssetFileToFileSystem(assets, file->assetType, file->assetSubType, "../server/assets/raw");
    }
    
    if(showAssetData)
    {
        Push(layout);
        NextRaw(layout);
        
        char* assetType = GetAssetTypeName(file->assetType);
        char* assetSubtype = GetAssetSubtypeName(file->assetType, file->assetSubType);
        
        ShowString(layout, "type", assetType, EditorText_StartingSpace);
        ShowString(layout, "subtype", assetSubtype, EditorText_StartingSpace);
        
        NextRaw(layout);
        
        ShowName(layout, "standard");
        ShowStandard(layout, StandardTextColor(), "%d", file->standardAssetCount);
        ShowName(layout, "derived");
        ShowStandard(layout, StandardTextColor(), "%d", file->derivedAssetCount);
        
        NextRaw(layout);
        
        if(EditorCollapsible(layout, "assets", auID(&file->magicValue)))
        {
            Push(layout);
            u16 totalAssetCount = file->standardAssetCount + file->derivedAssetCount;
            for(u16 assetIndex = 0; assetIndex < totalAssetCount; ++assetIndex)
            {
                NextRaw(layout);
                AssetID ID = {};
                ID.type = file->assetType;
                ID.subtype = file->assetSubType;
                ID.index = assetIndex;
                
                RenderAndEditAsset(layout, assets, ID);
            }
            Pop(layout);
        }
        
        Pop(layout);
    }
}

internal EditorLayout StandardLayout(MemoryPool* pool, RenderGroup* group, EditorUIContext* context, Vec2 mouseP, Vec2 deltaMouseP, Vec4 defaultColoration = V4(1, 1, 1, 1), r32 fontScale = 1.0f, r32 horizontalAdvance = 100.0f)
{
    EditorLayout result = {};
    
    RandomSequence seq = {};
    AssetLabels labels = {};
    FontId fontID = QueryAssets(group->assets, AssetType_Font, AssetFont_game, &seq, &labels);
    PAKFont* font = GetFontInfo(group->assets, fontID);
    
    
    
    result.context = context;
    
    result.defaultColoration = defaultColoration;
    
    result.bufferSize = KiloBytes(64);
    result.buffer = (char*) PushSize(pool, result.bufferSize);
    
    result.currentP = context->offset;
    result.rawP = result.currentP;
    result.lastP = result.rawP;
    
    result.fontID = fontID;
    result.font = font;
    result.fontScale = fontScale;
    result.horizontalAdvance = horizontalAdvance;
    result.standardButtonDim = 0.5f * horizontalAdvance;
    
    result.mouseP = mouseP;
    result.deltaMouseP = deltaMouseP;
    result.group = group;
    
    return result;
}

internal void RenderEditor(RenderGroup* group, EditorUIContext* context, Vec2 mouseP, Vec2 deltaMouseP)
{
    MemoryPool editorPool = {};
    
    SetOrthographicTransformScreenDim(group);
    
    if(Pressed(&context->input->actionLeft))
    {
        context->offset.x -= 10.0f;
    }
    
    if(Pressed(&context->input->actionRight))
    {
        context->offset.x += 10.0f;
    }
    
    context->offset.y -= 10.0f * context->input->mouseWheelOffset;
    
    context->nextHot = {};
    
    EditorLayout layout = StandardLayout(&editorPool, group, context, mouseP, deltaMouseP, V4(1, 1, 1, 1), 0.5f);
    
    for(u32 fileIndex = 0; fileIndex < group->assets->fileCount; ++fileIndex)
    {
        RenderEditAssetFile(&layout, group->assets, fileIndex);
        NextRaw(&layout);
    }
    
    
    context->hot = context->nextHot;
    
    if(Pressed(&context->input->escButton))
    {
        context->interactive = {};
    }
    
    Clear(&editorPool);
}
