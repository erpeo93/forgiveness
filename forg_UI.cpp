#include "forg_UIcommon.cpp"
#include "forg_book.cpp"

inline Vec2 UIFollowingP(UIButton* button, r32 separator)
{
    Vec2 buttonP = button->textP + V2(button->textDim.x + separator, 0);
    return buttonP;
    
}

inline UIButton UIBtn(UIState* UI, Vec2 P, EditorLayout* layout, Vec4 color, char* text, b32 enabled = true, UIInteraction interaction = NullInteraction())
{
    UIButton result;
    
    Rect2 bounds = GetUIOrthoTextBounds(UI, text, layout->fontScale, P);
    result.textP = P;
    result.textDim = GetDim(bounds);
    
    Vec2 boundsDim = result.textDim * 1.1f;
    Vec2 boundsCenter = GetCenter(bounds);
    
    result.bounds = AddRadius(RectCenterDim(boundsCenter, boundsDim), V2(layout->padding, layout->padding));
    result.fontScale = layout->fontScale;
    result.Z = layout->additionalZBias;
    
    result.color = color;
    result.text = text;
    
    result.enabled = enabled;
    if(enabled)
    {
            result.interaction = interaction;
    }
    else
    {
        result.interaction = NullInteraction();
    }
    
    return result;
}

inline void UIButtonInteraction(UIButton* button, UIInteraction interaction)
{
    button->interaction = interaction;
}

struct DrawButtonResult
{
    Rect2 bounds;
    b32 hot;
};

inline DrawButtonResult UIDrawButton(UIState* UI, PlatformInput* input, UIButton* button)
{
    DrawButtonResult result = {};
    
    ObjectTransform buttonTranform = FlatTransform();
    buttonTranform.additionalZBias = button->Z + 0.001f;
    
    Vec4 buttonColor = button->color;
    
    r32 textAlpha = 1.0f;
    if(button->enabled)
    {
        if(PointInRect(button->bounds, UI->relativeScreenMouse))
        {
            UIAddInteraction(UI, input, mouseLeft, button->interaction);
            result.hot = true;
        }
        else
        {
            buttonColor.a *= 0.45f;
            textAlpha = 0.6f;
        }
        
   
    }
    else
    {
        textAlpha = 0.3f;
        buttonColor.a *= 0.07f;     
    }
        
    PushRect(UI->group, buttonTranform, button->bounds, buttonColor);
    PushRectOutline(UI->group, buttonTranform, button->bounds, V4(1, 1, 1, textAlpha), 1.4f);
    PushUIOrthoText(UI, button->text, button->fontScale, button->textP, V4(1, 1, 1, textAlpha), button->Z + 0.002f);
    
    result.bounds = button->bounds;
    return result;
}


inline void UIRenderAutocomplete(UIState* UI, PlatformInput* input, UIAutocomplete* autocomplete, EditorLayout* layout, Vec2 P, char* nameIn)
{
    UIAutocompleteBlock* block = autocomplete->firstBlock;
    
    
    i32 nameCount = 0;
    i32 runningIndex = 0;
    while(block)
    {
        for(u32 nameIndex = 0; nameIndex < block->count; ++nameIndex)
        {
            char* name = block->names[nameIndex];
            
            u32 count = StrLen(nameIn);
            if(StrEqual(count, name, nameIn))
            {
                Rect2 bounds = GetUIOrthoTextBounds(UI, name, layout->fontScale, P);
                
                Vec2 boundsMin = bounds.min - V2(2, 4);
                Vec2 dim = GetDim(bounds) + V2(layout->nameValueDistance, 0);
                dim.y = layout->childStandardHeight;
                
                
                Vec4 autocompleteColor = V4(1, 1, 1, 1);
                if(runningIndex++ == UI->currentAutocompleteSelectedIndex)
                {
                    autocompleteColor = V4(0, 1, 0, 1);
                    
                    UIInteraction autoInteraction = {};
                    UIAddStandardAction(UI, &autoInteraction, UI_Trigger, block->names[nameIndex], ColdPointer(UI->keyboardBuffer), ColdPointer(block->names[nameIndex]));
                    UIAddInteraction(UI, input, switchButton, autoInteraction);
                }
                
                PushUIOrthoText(UI, name, layout->fontScale, P, autocompleteColor, layout->additionalZBias + 1.1f);
                PushUIOrthoText(UI, nameIn, layout->fontScale, P, V4(0.5f, 0.5f, 0.5f, 1.0f), layout->additionalZBias + 1.11f);
                
                ObjectTransform autoTranform = FlatTransform();
                autoTranform.additionalZBias = layout->additionalZBias + 1.0f;
                
                PushRect(UI->group, autoTranform, RectMinDim(boundsMin, dim), V4(0, 0, 0, 1));
                PushRectOutline(UI->group, autoTranform, RectMinDim(boundsMin, dim), autocompleteColor, 1);
                P.y -= layout->childStandardHeight;
                
                ++nameCount;
            }
        }
        
        block = block->next;
    }
    
    
    if(nameCount)
    {
        i32 nextIndex = UI->currentAutocompleteSelectedIndex + 1;
        if(UI->currentAutocompleteSelectedIndex == (nameCount - 1))
        {
            nextIndex = 0;
        }
        
        i32 prevIndex = UI->currentAutocompleteSelectedIndex - 1;
        if(UI->currentAutocompleteSelectedIndex <= 0)
        {
            prevIndex = nameCount - 1;
        }
        UIAddInteraction(UI, input, actionDown, UISetValueInteraction(UI, UI_Trigger, &UI->currentAutocompleteSelectedIndex, nextIndex));
        UIAddInteraction(UI, input, actionUp, UISetValueInteraction(UI, UI_Trigger, &UI->currentAutocompleteSelectedIndex, prevIndex));
    }
    else
    {
        char* text = "no match";
        Rect2 bounds = GetUIOrthoTextBounds(UI, text, layout->fontScale, P);
        
        Vec2 boundsMin = bounds.min - V2(2, 4);
        Vec2 dim = GetDim(bounds) + V2(layout->nameValueDistance, 0);
        dim.y = layout->childStandardHeight;
        
        Vec4 color = V4(1, 0, 0, 1);
        
        PushUIOrthoText(UI, text, layout->fontScale, P, color, layout->additionalZBias + 1.1f);
        
        ObjectTransform autoTranform = FlatTransform();
        autoTranform.additionalZBias = layout->additionalZBias + 1.0f;
        
        PushRect(UI->group, autoTranform, RectMinDim(boundsMin, dim), V4(0, 0, 0, 1));
        PushRectOutline(UI->group, autoTranform, RectMinDim(boundsMin, dim), V4(1, 1, 1, 1), 1);
    }
}


inline UIAutocomplete* UIAddAutocomplete(UIState* UI, char* name)
{
    Assert(UI->autocompleteCount < ArrayCount(UI->autocompletes));
    UIAutocomplete* result = UI->autocompletes + UI->autocompleteCount++;
    result->hash = StringHash(name);
    return result;
}

inline void UIFreeAutocompleteOptions(UIState* UI, UIAutocomplete* autocomplete)
{
    FREELIST_FREE(autocomplete->firstBlock, UIAutocompleteBlock, UI->firstFreeAutocompleteBlock);
}

inline UIAutocomplete* UIFindAutocomplete(UIState* UI, u64 hash)
{
    UIAutocomplete* result = 0;
	for(u32 autoIndex = 0; autoIndex < UI->autocompleteCount; ++autoIndex)
    {
        UIAutocomplete* test = UI->autocompletes + autoIndex;
        if(hash == test->hash)
        {
            result = test;
            break;
        }
    }
    
	return result;
}

inline UIAutocomplete* UIFindAutocomplete(UIState* UI, char* name)
{
    UIAutocomplete* result = UIFindAutocomplete(UI, StringHash(name));
    return result;
}

inline void UIAddOption(UIState* UI, UIAutocomplete* autocomplete, char* option, u32 optionLength = 0)
{
    UIAutocompleteBlock* block = autocomplete->firstBlock;
    if(!block || (block->count == ArrayCount(block->names)))
    {
        FREELIST_ALLOC(block, UI->firstFreeAutocompleteBlock, PushStruct(&UI->autocompletePool, UIAutocompleteBlock));
        block->count = 0;
        block->next = autocomplete->firstBlock;
        autocomplete->firstBlock = block;
    }
    
    char* name = block->names[block->count++];
    
    if(!optionLength)
    {
        optionLength = StrLen(option);
    }
    FormatString(name, sizeof(block->names[0]), "%.*s", optionLength, option);
}

inline void UIAddTaxonomyToAutocomplete(UIState* UI, UIAutocomplete* autocomplete, TaxonomySlot* slot)
{
    UIAddOption(UI, autocomplete, slot->name);
    
	u32 validTaxonomies = slot->subTaxonomiesCount - slot->invalidTaxonomiesCount;
    for(u32 childIndex = 0; childIndex < validTaxonomies; ++childIndex)
    {
        TaxonomySlot* child = GetNthChildSlot(UI->table, slot, childIndex);
        UIAddTaxonomyToAutocomplete(UI, autocomplete, child);
    }
}

inline void UIAddAutocompleteFromTaxonomy(UIState* UI, char* name, char* autocompleteName = 0)
{
    if(!autocompleteName)
    {
        autocompleteName = name;
    }
    
    UIAutocomplete* autocomplete = UIFindAutocomplete(UI, autocompleteName);
    if(autocomplete)
    {
        UIFreeAutocompleteOptions(UI, autocomplete);    
    }
    else
    {
        autocomplete = UIAddAutocomplete(UI, autocompleteName);
    }
    
   
    TaxonomySlot* slot = NORUNTIMEGetTaxonomySlotByName(UI->table, name);
    
    for(u32 childIndex = 0; childIndex < slot->subTaxonomiesCount; ++childIndex)
    {
        TaxonomySlot* child = GetNthChildSlot(UI->table, slot, childIndex);
        UIAddTaxonomyToAutocomplete(UI, autocomplete, child);
    }
}

#define UIAddAutocompleteFromTable(UI, tableName, name) UIAddAutocompleteFromTable_(UI, MetaTable_##tableName, ArrayCount(MetaTable_##tableName), name)
inline void UIAddAutocompleteFromTable_(UIState* UI, char** values, u32 valueCount, char* name)
{
    UIAutocomplete* autocomplete = UIAddAutocomplete(UI, name);
    for(u32 valueIndex = 0; valueIndex < valueCount; ++valueIndex)
    {
        char* value = values[valueIndex];
        UIAddOption(UI, autocomplete, value);
    }
}



inline void UIAddAutocompleteFromFiles(UIState* UI)
{
    char* assetPath = "assets";
    PlatformFileGroup autoGroup = platformAPI.GetAllFilesBegin(PlatformFile_autocomplete, assetPath);
    for(u32 fileIndex = 0; fileIndex < autoGroup.fileCount; ++fileIndex)
    {
        char buffer[KiloBytes(2)];
        u32 bufferSize = sizeof(buffer);
        
        PlatformFileHandle handle = platformAPI.OpenNextFile(&autoGroup, assetPath);
        
        Assert(handle.fileSize <= bufferSize);
        platformAPI.ReadFromFile(&handle, 0, handle.fileSize, buffer);
        buffer[handle.fileSize] = 0;
        
        
        char nameNoPoint[64];
        GetNameWithoutPoint(nameNoPoint, sizeof(nameNoPoint), handle.name);
        
        
        UIAutocomplete* autocomplete = UIFindAutocomplete(UI, nameNoPoint);
        
        if(autocomplete)
        {
            UIFreeAutocompleteOptions(UI, autocomplete);
        }
        else
        {
            autocomplete = UIAddAutocomplete(UI, nameNoPoint);     
        }
       
        
        Tokenizer tokenizer = {};
        tokenizer.at = (char*) buffer;
        
        b32 parsing = true;
        while(parsing)
        {
            Token value = GetToken(&tokenizer);
            
            switch(value.type)
            {
                case Token_String:
                {
                    Token s = Stringize(value);
                    UIAddOption(UI, autocomplete, s.text, s.textLength);
                } break;
                
                case Token_Comma:
                {
                    
                } break;
                
                default:
                {
                    parsing = false;
                } break;
            }
        }
        platformAPI.CloseHandle(&handle);
    }
    platformAPI.GetAllFilesEnd(&autoGroup);
}



inline UIAutocomplete* UIFindAutocomplete(UIState* UI, EditorElementParents parents, char* name)
{
    EditorElement* grandGrandGrandGrandParent = parents.grandParents[3];
    EditorElement* grandGrandGrandParent = parents.grandParents[2];
    EditorElement* grandGrandParent = parents.grandParents[1];
    EditorElement* grandParent = parents.grandParents[0];
    EditorElement* parent = parents.father;
	u64 hash = StringHash(name);
	if(StrEqual(name, "animationName"))
	{
		TaxonomySlot* slot = GetSlotForTaxonomy(UI->table, UI->editingTaxonomy);
		hash = GetSkeletonForTaxonomy(UI->table, slot).hashID;
	}
    else if(StrEqual(name, "sound"))
    {
        Assert(parent);
        
        char* soundType = GetValue(parent, "soundType");
        hash = StringHash(soundType);
    }
    else if(StrEqual(name, "beachTaxonomy"))
    {
        hash = StringHash("tileType");
    }
    else if(StrEqual(name, "type"))
    {
        if(grandParent && grandParent->type == EditorElement_List)
        {
            hash = StringHash(grandParent->elementName);
        }
    }
    else if(StrEqual(name, "layoutName"))
    {
        UIAutocomplete* autocomplete = UIFindAutocomplete(UI, hash);
        UIFreeAutocompleteOptions(UI, autocomplete);
        
        char* equipmentName = GetValue(grandGrandParent, "equipment");
        TaxonomySlot* slot = NORUNTIMEGetTaxonomySlotByName(UI->table, equipmentName);
        
        for(ObjectLayout* layout = slot->firstLayout; layout; layout = layout->next)
        {
            UIAddOption(UI, autocomplete, layout->name);
        }
    }
	else if(StrEqual(name, "pieceName"))
	{
		UIAutocomplete* autocomplete = UIFindAutocomplete(UI, hash);
        UIFreeAutocompleteOptions(UI, autocomplete);
        
		char* equipmentName = GetValue(grandGrandGrandGrandParent, "equipment");
        char* layoutName = GetValue(grandGrandParent, "layoutName");
        
        if(equipmentName && layoutName)
        {
            TaxonomySlot* slot = NORUNTIMEGetTaxonomySlotByName(UI->table, equipmentName);
            
            for(ObjectLayout* layout = slot->firstLayout; layout; layout = layout->next)
            {
                if(StrEqual(layout->name, layoutName))
                {
                    for(LayoutPiece* piece = layout->firstPiece; piece; piece = piece->next)
                    {
                        if(!piece->parent)
                        {
                            UIAddOption(UI, autocomplete, piece->name);
                        }
                    }
                    break;	
                }
            }   
        }
        
        UIAddOption(UI, autocomplete, "all");
	}
    
	UIAutocomplete* result = UIFindAutocomplete(UI, hash);
    
    return result;
}

inline b32 SpecialHandling(char* name)
{
    b32 result = false;
    
    if(StrEqual(name, "leafName") ||
       StrEqual(name, "trunkName"))
    {
        result = true;
    }
    
    return result;
}

inline void AddConfirmActions(UIState* UI, EditorWidget* widget, UIInteraction* interaction, char* dest, u32 destSize)
{
	UIAddUndoRedoAction(UI, interaction, UI_Trigger, UndoRedoString(widget, dest, destSize, dest, UI->keyboardBuffer));            
	UIAddStandardAction_(UI, interaction, UI_Trigger, destSize, ColdPointer(dest), ColdPointer(UI->keyboardBuffer));
	UIAddSetValueAction(UI, interaction, UI_Trigger, &UI->activeLabel, 0); 
	UIAddSetValueAction(UI, interaction, UI_Trigger, &UI->activeLabelParent, 0); 
	UIAddSetValueAction(UI, interaction, UI_Trigger, &UI->active, 0);    
    UIAddClearAction(UI, interaction, UI_Trigger, ColdPointer(&UI->activeParents), sizeof(UI->activeParents));
	UIAddSetValueAction(UI, interaction, UI_Trigger, &UI->activeWidget, 0);    
    
    
	UIAddReloadElementAction(UI, interaction, UI_Trigger, widget->root);
	if(StrEqual(widget->name, "Editing Tabs"))
	{
		UIAddRequestAction(UI, interaction, UI_Trigger, SendDataFileRequest());
	}        
	UIAddClearAction(UI, interaction, UI_Trigger, ColdPointer(UI->keyboardBuffer), sizeof(UI->keyboardBuffer));
    
}


struct UIAddTabResult
{    
    Rect2 bounds;
    Vec4 color;
};

inline UIAddTabResult UIAddTabValueInteraction(UIState* UI, EditorWidget* widget, PlatformInput* input, EditorElementParents parents, EditorElement* root, Vec2 P, EditorLayout* layout, char* text)
{
    UIAddTabResult result = {};
    result.bounds = InvertedInfinityRect2();
    
    if(root != UI->active)
    {
       
        result.bounds = GetUIOrthoTextBounds(UI, text, layout->fontScale, P);
        result.color = V4(0.7f, 0.7f, 0, 1);
        if(!UI->activeLabel)
        {
            if(PointInRect(result.bounds, UI->relativeScreenMouse))
            {
                UIInteraction mouseInteraction = {};
                UIInteraction rightInteraction = {};
                
				if(UI->active && UI->bufferValid)
				{
					AddConfirmActions(UI, widget, &mouseInteraction, UI->active->value, sizeof(UI->active->value));
				}
                
                if(StrEqual(text, "true"))
                {
                    result.color = V4(1, 0, 1, 1);
                    UIAddStandardAction_(UI, &mouseInteraction, UI_Trigger, sizeof(root->value), ColdPointer(root->value), ColdPointer(UI->falseString));
                    UIAddReloadElementAction(UI, &mouseInteraction, UI_Trigger, widget->root);
                    
                }
                else if(StrEqual(text, "false"))
                {
                    result.color = V4(1, 0, 1, 1);
                    UIAddStandardAction_(UI, &mouseInteraction, UI_Trigger, sizeof(root->value), ColdPointer(root->value), ColdPointer(UI->trueString));
                    UIAddReloadElementAction(UI, &mouseInteraction, UI_Trigger, widget->root);
                }
                else
                {
                    
                    b32 canEdit = false;
                    if(root->type == EditorElement_Signed || root->type == EditorElement_Unsigned ||
                       root->type == EditorElement_Real)
                    {
                        canEdit = true;
                    }
                    else
                    {
                        if(IsSet(root, EditorElem_AlwaysEditable))
                        {
                            canEdit = true;
                        }
                        else
                        {
                            
                            if(SpecialHandling(root->name))
                               {
                                canEdit = true;   
                               }
                            else
                            {
                                UIAutocomplete* autocomplete = UIFindAutocomplete(UI, parents, root->name);
                                if(autocomplete)
                                {
                                    canEdit = true;
                                }   
                            }
                        }
                    }
                    
                    if(!UI->active && canEdit)
                    {
                        result.color = V4(1, 0, 1, 1);
                        mouseInteraction = UISetValueInteraction(UI, UI_Click, &UI->active, root);
                        UIAddSetValueAction(UI, &mouseInteraction, UI_Click, &UI->activeParents, parents);
                        UIAddSetValueAction(UI, &mouseInteraction, UI_Click, &UI->activeWidget, widget); 
                        UIAddSetValueAction(UI, &mouseInteraction, UI_Click, &UI->currentAutocompleteSelectedIndex, -1);   UIAddClearAction(UI, &mouseInteraction, UI_Click, ColdPointer(UI->keyboardBuffer), sizeof(UI->keyboardBuffer));
                        
                        
                        if(root->type == EditorElement_Real)
                        {
                            UIAddStandardAction_(UI, &mouseInteraction, UI_Trigger, sizeof(root->value), ColdPointer(UI->realDragging), ColdPointer(root->value));
                            UIAddOffsetStringRealEditorElement(UI, &mouseInteraction, UI_Idle, ColdPointer(root->value), ColdPointer(&UI->deltaScreenMouseP.y), 0.01f);
                            UIAddUndoRedoAction(UI, &mouseInteraction, UI_Release, UndoRedoDelayedString(widget, root->value, sizeof(root->value), root->value, ColdPointer(root->value)));
                            UIAddReloadElementAction(UI, &mouseInteraction, UI_Idle, widget->root);
                            UIAddReloadElementAction(UI, &mouseInteraction, UI_Release, widget->root);
                            if(StrEqual(widget->name, "Editing Tabs"))
                            {
                                UIAddRequestAction(UI, &mouseInteraction, UI_Release, SendDataFileRequest());
                            }
                            
                        }
                        
                        if(root->type == EditorElement_Unsigned)
                        {                            
                            UIAddOffsetStringUnsignedEditorElement(UI, &rightInteraction, UI_Click, ColdPointer(root->value), Fixed(1), 1);
                            UIAddUndoRedoAction(UI, &rightInteraction, UI_Click, UndoRedoDelayedString(widget, root->value, sizeof(root->value), root->value, ColdPointer(root->value)));
                            UIAddReloadElementAction(UI, &rightInteraction, UI_Click, widget->root);
                            if(StrEqual(widget->name, "Editing Tabs"))
                            {
                                UIAddRequestAction(UI, &rightInteraction, UI_Click, SendDataFileRequest());
                            }
                            
                        }
                    }
                }
                
                UIAddInteraction(UI, input, mouseLeft, mouseInteraction);
                UIAddInteraction(UI, input, mouseRight, rightInteraction);       
            }
        }
    }
    else
    {
        result.bounds = GetUIOrthoTextBounds(UI, UI->keyboardBuffer, layout->fontScale, P);
        result.color = V4(1, 0, 0, 1);
        if(UI->bufferValid)
        {
            result.color = V4(0, 1, 0, 1);
            
            UIInteraction confirmInteraction = {};
			AddConfirmActions(UI, widget, &confirmInteraction, root->value, sizeof(root->value));
            UIAddInteraction(UI, input, confirmButton, confirmInteraction);
        }
        
        if(root->type == EditorElement_String)
        {
            UIAutocomplete* autocomplete = UIFindAutocomplete(UI, parents, root->name);
            if(autocomplete)
            {
                UIRenderAutocomplete(UI, input, autocomplete, layout, P -V2(0, layout->childStandardHeight), UI->keyboardBuffer);
            }
        }   
    }
    
    return result;
}

inline b32 UIChildModified(TaxonomyTable* table, TaxonomySlot* slot)
{
    b32 result = false;
    for(u32 childIndex = 0; childIndex < slot->subTaxonomiesCount; ++childIndex)
    {
        TaxonomySlot* child = GetNthChildSlot(table, slot, childIndex);
        if(child->editorChangeCount || UIChildModified(table, child))
        {
            result = true;
            break;
        }
    }
    
    return result;
}

struct StructButtonsResult
{
    Rect2 completeBounds;
    b32 hot;
    Vec4 nameColor;
};


inline void DrawStructActionButton(UIState* UI, PlatformInput* input, StructButtonsResult* result, UIButton* button)
{
    DrawButtonResult btn = UIDrawButton(UI, input, button);
    
    result->completeBounds = Union(result->completeBounds, btn.bounds);
    
    if(btn.hot)
    {
        result->hot = true;
        result->nameColor = button->color;
    }
}

inline StructButtonsResult DrawStructButtons(UIState* UI, PlatformInput* input, EditorLayout* layout, Rect2 nameBounds, EditorElementParents parents, EditorElement* grandFather, EditorElement* father, EditorElement* root, b32 canDelete)
{
    StructButtonsResult result = {};
    result.completeBounds = nameBounds;
    
    
    Vec2 nameDim = GetDim(nameBounds);
    Vec2 buttonP = nameBounds.min + V2(nameDim.x + layout->nameValueDistance, 0.3f * nameDim.y);
    if(canDelete)
    {
        if(root != UI->copying)
        {
            Vec4 deleteColor = V4(1, 0, 0, 1);
            UIButton deleteButton = UIBtn(UI, buttonP, layout, deleteColor, "delete", true, UISetValueInteraction(UI, UI_Trigger, &root->flags, (root->flags | EditorElem_Deleted)));
            
     
            DrawStructActionButton(UI, input, &result, &deleteButton);            
            buttonP.x = result.completeBounds.max.x + layout->nameValueDistance;
        }
    }
    
    if(root->firstValue && father)
    {
        if(father->flags & EditorElem_PlaySoundButton)
        {
            u64 soundTypeHash = StringHash(father->name);
            u64 soundNameHash = StringHash(root->firstValue->value);
            
            UIButton playButton = UIBtn(UI, buttonP, layout, V4(0, 1, 0, 1), "play", true, UIPlaySoundInteraction(UI, UI_Trigger, soundTypeHash, soundNameHash, 0));
            
            DrawStructActionButton(UI, input, &result, &playButton);
        }
        else if(father->flags & EditorElem_PlayEventSoundButton)
        {
            
            char* soundType = GetValue(root, "soundType");
            char* soundName = GetValue(root, "sound");
            u64 soundTypeHash = StringHash(soundType);
            u64 soundNameHash = StringHash(soundName);
            
            UIButton playButton = UIBtn(UI, buttonP, layout, V4(0, 1, 0, 1), "play", true, UIPlaySoundInteraction(UI, UI_Trigger, soundTypeHash, soundNameHash, GetElement(root, "params")));
            
            DrawStructActionButton(UI, input, &result, &playButton);
        }
        else if(father->flags & EditorElem_PlayEventButton)
        {
            UIButton playButton = UIBtn(UI, buttonP, layout, V4(0, 1, 0, 1), "play");
            
            u64 eventNameHash = StringHash(root->name);
            
            UIButtonInteraction(&playButton, UIPlaySoundEventInteraction(UI, UI_Trigger, eventNameHash));
            DrawStructActionButton(UI, input, &result, &playButton);
        }
        else if(father->flags & EditorElem_PlayContainerButton)
        {
            UIButton playButton = UIBtn(UI, buttonP, layout, V4(0, 1, 0, 1), "play");
            
            UIButtonInteraction(&playButton, UIPlaySoundContainerInteraction(UI, UI_Trigger, root, parents));
            DrawStructActionButton(UI, input, &result, &playButton);
        }
        else if(father->flags & EditorElem_EquipInAnimationButton)
        {
            UIButton equipButton = UIBtn(UI, buttonP, layout, V4(0, 1, 0, 1), "try");
            
            UIButtonInteraction(&equipButton, UIEquipInAnimationWidgetInteraction(UI, UI_Trigger, grandFather, root));
            DrawStructActionButton(UI, input, &result, &equipButton);
        }
        
        else if(father->flags & EditorElem_ShowLabelBitmapButton)
        {
            UIButton showButton = UIBtn(UI,buttonP, layout, V4(0, 1, 0, 1), "view");
            
            EditorElement* grandGrandFather = parents.grandParents[1];
            u64 componentNameHash = StringHash(grandGrandFather->name);
            u64 bitmapNameHash = StringHash(GetValue(grandFather,
                                                     "componentName"));
            
            Vec4 coloration = ToV4Color(GetElement(root, "coloration"));
            UIButtonInteraction(&showButton, UIShowBitmapInteraction(UI, UI_Idle, componentNameHash, bitmapNameHash, coloration));
            
            DrawStructActionButton(UI, input, &result, &showButton);
        }
        
    }
    
    return result;
   
}

struct CopyPasteInteractionRes
{
    b32 propagateColor;
    Vec4 color;
};

inline  CopyPasteInteractionRes AddCopyPasteInteraction(UIState* UI, PlatformInput* input, EditorWidget* widget, EditorLayout* layout, EditorElement* root, b32 canDelete, Rect2 nameBounds)
{    
    CopyPasteInteractionRes result = {};
                            if(root == UI->copying)
                        {
        Vec4 copyColor = V4(0, 0, 1, 1);
        result.propagateColor = true;
        result.color = copyColor;
        PushRectOutline(UI->group, FlatTransform(layout->additionalZBias), nameBounds, copyColor, 2.0f);
                        
                        }
                        else
                        {
                            if(!UI->hotStructThisFrame && PointInRect(nameBounds, UI->relativeScreenMouse))
                            {
            Vec4 defaultColor = V4(0.7f, 0.7f, 0.0f, 1.0f);
            
            result.propagateColor = true;
            result.color = defaultColor;
            
                                UI->hotStructThisFrame = true;
                                UI->hotStructBounds = nameBounds;
                                UI->hotStructZ = layout->additionalZBias;
                                UI->hotStructColor = defaultColor;
                                UI->hotStruct = root;
                                UI->hotWidget = widget;
                                
                                UIAddInteraction(UI, input, copyButton, UISetValueInteraction(UI, UI_Trigger, &UI->copying, root));
                                if(UI->copying)
                                {
                                    b32 matches = false;
                
                if(UI->copying->type == root->type)
                {
                    switch(root->type)
                    {
                        case EditorElement_Struct:
                        {
                            matches = (true);
                            EditorElement* test = root->firstChild;
                            for(EditorElement* match = UI->copying->firstChild; match; match = match->next)
                            {
                                if(!test || !StrEqual(match->name, test->name))
                                {
                                    matches = false;
                                    break;
                                }
                                
                                test = test->next;
                            }
                            
                        } break;
                        
                        case EditorElement_List:
                        {
                        matches = StrEqual(root->name, UI->copying->name);
                        } break;
                    }
                }

                if(matches)
                {
                    result.propagateColor = true;
                    Vec4 canPasteColor = V4(0, 1, 0, 1);
                    
                    result.color = canPasteColor;
                    UI->hotStructColor = canPasteColor;
                                        UIAddInteraction(UI, input, pasteButton, UISetValueInteraction(UI, UI_Trigger, &root->flags, root->flags | EditorElem_Pasted));
                }
                
                                }
                            }
                        }
    return result;                        
}


inline void PushEditorLine(RenderGroup* group, b32 propagated, Vec4 color, Vec3 fromP, Vec3 toP, r32 thickness, r32 lineSegmentLength, r32 lineSpacing)
{
    if(propagated)
    {
        PushLine(group, color, fromP, toP, 1.5f * thickness);
    }
    else
    {
        PushDottedLine(group, color, fromP, toP, thickness, lineSegmentLength, lineSpacing);   
    }
}

struct Rect24C
{
    Rect2 rect;
    Vec4 c0, c1, c2, c3;
};

inline void SetColorsVertical(Rect24C* rect, Vec4 cStart, Vec4 cEnd)
{
    rect->c0 = rect->c3 = cStart;
    rect->c1 = rect->c2 = cEnd;
}


struct UIRenderTreeResult
{
    Rect2 bounds;
    r32 lineEndY;
};

inline UIRenderTreeResult UIRenderEditorTree(UIState* UI, EditorWidget* widget, EditorLayout* layout, EditorElementParents parents, EditorElement* parent_, b32 propagateLineColor, Vec4 parentSquareColor, EditorElement* root_, PlatformInput* input, b32 canDelete, b32 showNames = true, Vec2 parentNameP = {})
{
    UIRenderTreeResult totalResult = {};
    
    Assert(parents.grandParents[ArrayCount(parents.grandParents) - 1] == 0);
    for(u32 parentIndex = ArrayCount(parents.grandParents) - 1; parentIndex > 0; --parentIndex)
    {
        parents.grandParents[parentIndex] = parents.grandParents[parentIndex - 1];
    }
    
    parents.grandParents[0] = parents.father;
    parents.father = parent_;
    
    
    EditorElement* father = parents.father;
    EditorElement* grandFather = parents.grandParents[0];
    
    
    
    totalResult.bounds = InvertedInfinityRect2();
    totalResult.lineEndY = layout->P.y;
    
    u32 childIndex = 0;
    for(EditorElement* root = root_; root; root = root->next)
    {
		if(root->flags & EditorElem_DontRender)
		{
            root->flags &= ~EditorElem_DontRender;
		}
		else
		{
			if(IsSet(root, EditorElem_Pasted))
            {
                UI->pasted = root;
                UI->pastedTimeLeft = 2.0f;
                
                ClearFlags(root, EditorElem_Pasted);
                EditorElement oldElem = *root;
                
                root->flags = UI->copying->flags;
                switch(root->type)
                {
                    case EditorElement_Struct:
                    {
                        root->firstValue = CopyEditorElement(UI->table, UI->copying->firstValue);
                    } break;
                    
                    case EditorElement_List:
                    {
                        root->firstInList = CopyEditorElement(UI->table, UI->copying->firstInList);
                        
                        FormatString(root->elementName, sizeof(root->elementName), "%s", UI->copying->elementName);
                        if(UI->copying->emptyElement)
                        {
                            root->emptyElement = CopyEditorElement(UI->table, UI->copying->emptyElement);
                        }
                    }
                }
                
                EditorElement newElem = *root;
                UIAddUndoRedoCommand(UI, UndoRedoPaste(widget, root, oldElem, newElem));
                
            }
            
            Rect2 result = InvertedInfinityRect2();
            char name[128];
            
            b32 deleteName = false;
            if(father && father->type == EditorElement_List && !root->name[0])
            {
                FormatString(root->name, sizeof(root->name), "000%d", childIndex);
                deleteName = true;
            }
            if(root->type == EditorElement_List)
            {
                FormatString(name, sizeof(name), " [ ] %s", root->name);
            }
            else if(root->type == EditorElement_Struct)
            {
                FormatString(name, sizeof(name), "{ } %s", root->name);
            }
            else if(root->type == EditorElement_EmptyTaxonomy)
            {
                FormatString(name, sizeof(name), "Add new -%s-", father->name);
            }
            else if(root->type <= EditorElement_Real)
            {
                if(showNames)
                {
                    FormatString(name, sizeof(name), "%s =", root->name); 
                }
                else
                {
                    FormatString(name, sizeof(name), " ="); 
                }
               
            }
            else
            {
                FormatString(name, sizeof(name), "%s", root->name);
            }
            
            Vec3 parentAlignedP = V3(layout->P, layout->additionalZBias);
            if(father)
            {
                parentAlignedP = V3(layout->P.x - layout->nameValueDistance, layout->P.y, layout->additionalZBias);
            }
            
            
            Vec3 lineStartP = V3(layout->P, layout->additionalZBias);
            
            Rect2 nameTestBounds = GetUIOrthoTextBounds(UI, name, layout->fontScale, layout->P);
            
            u32 seed = (u32) StringHash(root->name) + childIndex;
            RandomSequence seq = Seed(seed);
            
            r32 squareDim = layout->squareDim;
            if(root->type < EditorElement_List)
            {
                squareDim *= 0.6f;
            }
            
            Vec4 nameColor = V4(1, 1, 1, 1);
            Vec4 squareLineColor;
            
            if(root == UI->copying)
            {
                squareLineColor = V4(0, 0, 1, 1);
                nameColor = squareLineColor;
            }
            else
            {
                if(propagateLineColor || root->type == EditorElement_EmptyTaxonomy)
                {
                    squareLineColor = parentSquareColor;
                    nameColor = parentSquareColor;
                }
                else
                {
                    squareLineColor = V4(0.6f * RandomUniV3(&seq), 1);
                    if(root->type < EditorElement_List)
                    {
                        if(root->type == EditorElement_String)
                        {
                            squareLineColor = V4(0, 0, 0.2f, 1.0f);
                        }
                        else
                        {
                            squareLineColor = V4(0, 0.2f, 0.0f, 1.0f);
                        }
                    }
                    
                }
                
            }

            Rect2 square = RectCenterDim(layout->P, V2(squareDim, squareDim));
            
            
            Vec4 internalSquareColor = V4(0, 0, 0, 1);
            b32 drawExpandedSign = (root->type == EditorElement_List || root->type == EditorElement_Struct || root->type == EditorElement_Taxonomy);
            if(PointInRect(square, UI->relativeScreenMouse) && drawExpandedSign)
            {
                //nameColor = V4(1, 1, 0, 1);
                u32 finalFlags = IsSet(root, EditorElem_Expanded) ? (root->flags & ~EditorElem_Expanded) : (root->flags | EditorElem_Expanded);
                
                UIAddInteraction(UI, input, mouseLeft, UISetValueInteraction(UI, UI_Trigger, &root->flags, finalFlags));
                
                internalSquareColor = V4(0.9f, 0.2f, 0.1f, 1.0f);
                
            }
            
            
			totalResult.lineEndY = GetCenter(square).y;
            Vec2 nameP = GetCenter(square) + V2(1.5f * GetDim(square).x, -0.4f * GetDim(nameTestBounds).y);
            
            if(!showNames)
            {
				nameP = parentNameP;
            }
            else
            {
            PushEditorLine(UI->group, propagateLineColor, parentSquareColor, parentAlignedP, lineStartP, layout->lineThickness, layout->lineSegmentLength, layout->lineSpacing);
                PushRect(UI->group, FlatTransform(layout->additionalZBias), square, squareLineColor);   
            }
            
            if(drawExpandedSign)
            {
                PushRect(UI->group, FlatTransform(layout->additionalZBias + 0.01f), Scale(square, 0.82f), internalSquareColor);                
                char* sign = IsSet(root, EditorElem_Expanded) ? "-" : "+";
                
                Vec2 insideDim = Hadamart(0.5f * GetDim(square), V2(0.7f, 0.8f));
                PushUIOrthoText(UI, sign, 0.6f * layout->fontScale, GetCenter(square) - insideDim, V4(1, 1, 1, 1), layout->additionalZBias + 0.01f);
            }
            
            Rect2 nameBounds = AddRadius(GetUIOrthoTextBounds(UI, name, layout->fontScale, nameP), V2(layout->padding, layout->padding));
            
           
            if(root == UI->pasted)
            {
                r32 lerp = Clamp01MapToRange(2.0f, UI->pastedTimeLeft, 0);
                nameColor = Lerp(V4(0, 1, 0, 1), lerp, nameColor);
            }
            
			char shadowLabel[64] = {};
            char* nameToShow = name;
            b32 showName = (nameToShow[0]);
            
            
            if(PointInRect(nameBounds, UI->relativeScreenMouse))
            {                
                if(!UI->activeLabel && !UI->active && father && (father->flags & EditorElem_LabelsEditable))
                {
                    if(!father->labelName[0] || UIFindAutocomplete(UI, parents, father->labelName))
                    {
                                         nameColor = V4(1, 0, 0, 1);
                    UIInteraction labelInteraction = UISetValueInteraction(UI, UI_Trigger, &UI->activeLabel, root);
                    UIAddSetValueAction(UI, &labelInteraction, UI_Trigger, &UI->activeLabelParent, father);
                    UIAddClearAction(UI, &labelInteraction, UI_Trigger, ColdPointer(UI->keyboardBuffer), sizeof(UI->keyboardBuffer));
                    
                    UIAddSetValueAction(UI, &labelInteraction, UI_Trigger, &UI->activeWidget, widget); 
                    UIAddInteraction(UI, input, mouseLeft, labelInteraction);   
                    }
                }
            }
            
            if(root == UI->activeLabel)
            {
                nameToShow = UI->showBuffer;
                
				if(UI->keyboardBuffer[0] == 0)
				{
                    switch(root->type)
                    {
                        case EditorElement_String:
                        {
                            FormatString(shadowLabel, sizeof(shadowLabel), "{ } %s", root->name);
                        } break;
                        
                        case EditorElement_List:
                        {
                            FormatString(shadowLabel, sizeof(shadowLabel), "[ ] %s", root->name);
                        } break;
                        
                        default:
                        {
                            FormatString(shadowLabel, sizeof(shadowLabel), "%s", root->name);
                        } break;
                    }
				}
                
                
                UIAutocomplete* autocomplete = UIFindAutocomplete(UI, parents, father->labelName);
                if(autocomplete)
                {
                    UIRenderAutocomplete(UI, input, autocomplete, layout, nameP -V2(0, layout->childStandardHeight), UI->keyboardBuffer);
                }
                
                
                if(UI->bufferValid)
                {
                    nameColor = V4(0, 1, 0, 1);
                    UIInteraction confirmInteraction = {};
                    AddConfirmActions(UI, widget, &confirmInteraction, root->name, sizeof(root->name));
                    UIAddInteraction(UI, input, confirmButton, confirmInteraction);
                }
                else
                {
                    nameColor = V4(1, 0, 0, 1);
                }
            }
            
            if(root == UI->active && root->type == EditorElement_EmptyTaxonomy)
            {
                nameToShow = UI->showBuffer;
            }
            
           
            
            result = Union(square, nameBounds);
            
            if(showName && showNames)
            {
                layout->P += V2(0, -layout->childStandardHeight);
            }
            
            
            switch(root->type)
            {
                case EditorElement_String:
                case EditorElement_Real:
                case EditorElement_Signed:
                case EditorElement_Unsigned:
                {
                    r32 xAdvance = Max(layout->nameValueDistance, GetDim(nameBounds).x + 10.0f);
                    Vec2 valueP = nameP + V2(xAdvance, 0);
                    char* text = (root == UI->active) ? UI->showBuffer : root->value;
                    
					if(root == UI->active && UI->keyboardBuffer[0] == 0)
					{
                        Vec4 shadowColor = V4(0.6f, 0.6f, 0.6f, 1.0f);
                        PushUIOrthoText(UI, root->value, layout->fontScale, valueP, shadowColor, layout->additionalZBias);
                        result = Union(result, GetUIOrthoTextBounds(UI, root->value, layout->fontScale, valueP));
					}
                    
                    UIAddTabResult addTab = UIAddTabValueInteraction(UI, widget, input, parents, root, valueP, layout, text);
                    
                    PushUIOrthoText(UI, text, layout->fontScale, valueP, addTab.color, layout->additionalZBias);
                    result = Union(result, AddRadius(addTab.bounds, V2(layout->padding, layout->padding)));
                    
                } break;
                
                case EditorElement_List:
                {
                    CopyPasteInteractionRes copyPaste = AddCopyPasteInteraction(UI, input, widget, layout, root, canDelete, nameBounds);

                    b32 propagateLineC = propagateLineColor;
                    if(copyPaste.propagateColor)
                    {
                        propagateLineC = true;
                        squareLineColor = copyPaste.color;
                    }
                    

                    if(IsSet(root, EditorElem_Expanded))
                    {
                        Vec3 verticalStartP = lineStartP;
                        for(EditorElement** elementPtr = &root->firstInList; *elementPtr;)
                        {
                            EditorElement* element = *elementPtr;
                            if(IsSet(element, EditorElem_Deleted))
                            {
                                UIAddUndoRedoCommand(UI, UndoRedoDelete(widget, elementPtr, element));
                                *elementPtr = element->next;
                                widget->needsToBeReloaded = true;
                            }
                            else
                            {
                                elementPtr = &element->next;
                            }
                        }
                        
                        
                        
                        if(root->emptyElement)
                        {
                            layout->P.y -= layout->childStandardHeight;
                            Vec2 addP = nameP + V2(layout->nameValueDistance, -layout->childStandardHeight);
                            
                            UIButton addButton = UIBtn(UI, addP, layout, V4(1, 0, 0, 1), "add");
                            UIInteraction addInteraction = UIAddEmptyElementToListInteraction(UI, UI_Trigger, widget, root);
                            
                            if(StrEqual(widget->name, "Editing Tabs"))
                            {
                                UIAddRequestAction(UI, &addInteraction, UI_Trigger, SendDataFileRequest());
                            }
                            
                            UIAddReloadElementAction(UI, &addInteraction, UI_Trigger, widget->root);
                            UIButtonInteraction(&addButton, addInteraction);
                            result = Union(result, UIDrawButton(UI, input, &addButton).bounds);
                            
                        }
                        
                        
                        
                        b32 moveHorizontally = (root->firstInList != 0);
                        if(moveHorizontally)
                        {
                            layout->P += V2(layout->nameValueDistance, 0);
                        }
                        
                        
                        
                        b32 canDeleteElements;
                        if(IsSet(root, EditorElem_AtLeastOneInList))
                        {
                            canDeleteElements = (root->firstInList && root->firstInList->next);
                            
                        }
                        else
                        {
                            canDeleteElements = !IsSet(root, EditorElem_CantBeDeleted);
                        }
                        
                        UIRenderTreeResult childs = UIRenderEditorTree(UI, widget, layout, parents, root, propagateLineC, squareLineColor, root->firstInList, input, canDeleteElements);
                        
                        result = Union(result, childs.bounds);
                        
                        if(moveHorizontally)
                        {
                            layout->P -= V2(layout->nameValueDistance, 0);
                        }
                        
                        Vec3 verticalEndP = V3(verticalStartP.x, childs.lineEndY, layout->additionalZBias);
                        
                        if(root->firstInList)
                        {
                            PushEditorLine(UI->group, propagateLineC, squareLineColor, verticalStartP, verticalEndP, layout->lineThickness, layout->lineSegmentLength, layout->lineSpacing);   
                        }
                        
                    }
                } break;
                
                case EditorElement_Struct:
                {
                    b32 propagateLineC = propagateLineColor;
                    b32 simpleValue = false;
                   
                    
                    if(root->firstValue && !root->firstValue->next && StrEqual(root->firstValue->name, "value"))
                    {
                        //layout->P.y = nameP.y;
                        simpleValue = true;
						Vec2 drawHere = nameP;
                        drawHere.x = nameBounds.max.x;
                        
                        UIRenderTreeResult childs = UIRenderEditorTree(UI, widget, layout, parents, root, propagateLineC, squareLineColor, root->firstValue, input, canDelete, false, drawHere);
                        
                        nameBounds.max.x = childs.bounds.max.x;
                    }
                    
                    
                    if(root != UI->activeLabel)
                    {
                        StructButtonsResult structButtons = DrawStructButtons(UI, input, layout, nameBounds, parents, grandFather, father, root, canDelete);
                        result = Union(result, structButtons.completeBounds);
                    
                        if(structButtons.hot)
                        {
                            nameColor = structButtons.nameColor;
                                propagateLineC = true;
                                squareLineColor = nameColor;   
                        }
                    }
                    
                    
                    CopyPasteInteractionRes copyPaste = AddCopyPasteInteraction(UI, input, widget, layout, root, canDelete, nameBounds);

                    if(copyPaste.propagateColor)
                    {
                        propagateLineC = true;
                        squareLineColor = copyPaste.color;
                    }
                    
                    if(!canDelete)
                    {
                        if(UI->hotStructThisFrame && root == UI->hotStruct)
                        {
                            UIInteraction dragInteraction = UISetValueInteraction(UI, UI_Trigger, &UI->dragging, root);
                            UIAddSetValueAction(UI, &dragInteraction, UI_Trigger, &UI->draggingParent, father);
                            UIAddReleaseDragAction(UI, &dragInteraction, UI_Release);
                            UIAddInteraction(UI, input, mouseRight, dragInteraction);
                        }
                    }
                    
                    if(simpleValue)
                    {
                        
                    }
                    else
                    {
                   
                    if(IsSet(root, EditorElem_Expanded) || !root->name[0])
                    {
                        Vec3 verticalStartP = lineStartP;
                        layout->P += V2(layout->nameValueDistance, 0);
                        
                        
                        if(father)
                        {
                            if(StrEqual(father->elementName, "soundCType") ||
                               StrEqual(father->elementName, "soundType"))
                            {
                                b32 showLabels = true;
                                EditorElement* pppp = parents.grandParents[0];
                                if(pppp)
                                {
                                    char* type = GetValue(pppp, "type");
                                    if(!StrEqual(type, "Labeled"))
                                    {
                                        showLabels = false;
                                    }
                                }
                                else
                                {
                                    showLabels = false;
                                }
                                
                                if(!showLabels)
                                {
                                    EditorElement* list = GetElement(root, "labels");
                                    if(list)
                                    {
                                        list->flags |= EditorElem_DontRender;
                                    }
                                }
                            }
                        }
                        
                        
                        UIRenderTreeResult childs = UIRenderEditorTree(UI, widget, layout, parents, root, propagateLineC, squareLineColor, root->firstValue, input, canDelete);
                                                

                        
                        result = Union(result, childs.bounds);
                        layout->P += V2(-layout->nameValueDistance, 0);
                        
                        Vec3 verticalEndP = V3(verticalStartP.x, childs.lineEndY, layout->additionalZBias);
                        PushEditorLine(UI->group, propagateLineC, squareLineColor, verticalStartP, verticalEndP, layout->lineThickness, layout->lineSegmentLength, layout->lineSpacing);
                                               
                      
                    }     
                    }
                   
                } break;
                
                case EditorElement_Taxonomy:
                {
                    TaxonomySlot* slot = GetSlotForTaxonomy(UI->table, root->taxonomy);
                    
                    if(slot->name[0] == '#')
                    {
                        nameColor = V4(0.2f, 0.2f, 0.2f, 1.0f);
                        nameToShow++;
                    }
                    
                    if(slot->editorChangeCount || UIChildModified(UI->table, slot))
                    {
                        nameColor = V4(1, 0, 0, 1);
                    }
                    
                    b32 propagateLineC = propagateLineColor;
                    if(PointInRect(nameBounds, UI->relativeScreenMouse))
                    {

                        nameColor = squareLineColor;
                        propagateLineC = true;
                    }
                    
                    if(IsSet(root, EditorElem_Expanded))
                    {
                        if(IsSet(root, EditorElem_Editable))
                        {
                            r32 buttonSeparator = 20;
                            Vec2 startingPos = nameP + V2(GetDim(nameBounds).x + 0.5f * layout->nameValueDistance, 0);
                            Vec2 instantiateP = startingPos;
                            
                            if(UI->worldMode->editorRoles & EditorRole_GameDesigner)
                            {
                                b32 cancActive = UI->previousFrameWasAllowedToQuit;
                                char* cancText = "canc";
                                UIInteraction cancReviveInteraction = SendRequestInteraction(UI, UI_Trigger, DeleteTaxonomyRequest(root->taxonomy));
                                
                                if(root->name[0] == '#')
                                {
                               
                                    cancText = "revive";
                                    cancReviveInteraction = SendRequestInteraction(UI, UI_Trigger, ReviveTaxonomyRequest(root->taxonomy));
                                }
                                
                                UIButton deleteButton = UIBtn(UI, startingPos, layout, V4(1, 0, 0, 1), cancText, cancActive, cancReviveInteraction);
                                
                                DrawButtonResult cancDraw = UIDrawButton(UI, input, &deleteButton);
                                if(cancDraw.hot)
                                {
                                    nameColor = V4(0, 1, 0, 1);
                                }
                                result = Union(result, cancDraw.bounds);
                                
                                instantiateP = UIFollowingP(&deleteButton, buttonSeparator);
                            }
                            
                            UIInteraction instantiateInteraction = SendRequestInteraction(UI, UI_Click, InstantiateTaxonomyRequest(root->taxonomy, V3(1, 0, 0)));
                            UIAddSetValueAction(UI, &instantiateInteraction, UI_Idle, &UI->instantiatingTaxonomy, root->taxonomy);
                            UIAddSetValueAction(UI, &instantiateInteraction, UI_Release, &UI->instantiatingTaxonomy, 0); 
                            
                            b32 active = (root->name[0] != '#' && IsSpawnable(UI->table, root->taxonomy));
                            UIButton instantiateButton = UIBtn(UI, instantiateP, layout, V4(0, 0, 1, 1), "place", active, instantiateInteraction);
                            
                            
                            DrawButtonResult placeDraw = UIDrawButton(UI, input, &instantiateButton);
                            if(placeDraw.hot)
                            {
                                nameColor = V4(0, 1, 0, 1);
                            }
                            result = Union(result, placeDraw.bounds);
                            
                            
                            UIInteraction editInteraction = SendRequestInteraction(UI, UI_Trigger,EditRequest(root->taxonomy));
                            UIButton editButton = UIBtn(UI, UIFollowingP(&instantiateButton, buttonSeparator), layout, V4(0, 1, 0, 1), "edit", true, editInteraction);
                                                        
                            DrawButtonResult editDraw = UIDrawButton(UI, input, &editButton);
                            result = Union(result, editDraw.bounds);
                            if(editDraw.hot)
                            {
                                nameColor = V4(0, 1, 0, 1);  
                            }
                        }
                        
                        
                        Vec3 verticalStartP = lineStartP;
                        
                        layout->P += V2(layout->nameValueDistance, 0);
                        
                        UIRenderTreeResult childs = UIRenderEditorTree(UI, widget, layout, parents, root, propagateLineC, squareLineColor, root->firstChild, input, false);
                        result = Union(result, childs.bounds);
                        
                        layout->P += V2(-layout->nameValueDistance, 0);
                        
                        Vec3 verticalEndP = V3(verticalStartP.x, childs.lineEndY, layout->additionalZBias);
                        PushEditorLine(UI->group, propagateLineC, squareLineColor, verticalStartP, verticalEndP, layout->lineThickness, layout->lineSegmentLength, layout->lineSpacing);
                    }
                } break;
                
                case EditorElement_EmptyTaxonomy:
                {
                    if(root != UI->active)
                    {                            
                        if(UI->previousFrameWasAllowedToQuit)
                        {
                        if(PointInRect(nameBounds, UI->relativeScreenMouse))
                        {
                            UIInteraction mouseInteraction = UISetValueInteraction(UI, UI_Trigger, &UI->active, root);
                            UIAddSetValueAction(UI, &mouseInteraction, UI_Trigger, &UI->activeParents, parents); 
                            UIAddSetValueAction(UI, &mouseInteraction, UI_Trigger, &UI->activeWidget, widget); 
                            UIAddClearAction(UI, &mouseInteraction, UI_Trigger, ColdPointer(UI->keyboardBuffer), sizeof(UI->keyboardBuffer));
                            UIAddInteraction(UI, input, mouseLeft, mouseInteraction);
                                nameColor = parentSquareColor;
                                nameColor = V4(1, 1, 0, 1);
                        }   
                        }
                        else
                        {
                            nameColor.a = 0.3f;
                        }
                    }
                    else
                    {
                        nameColor = V4(1, 0, 0, 1);
                        
                        if(UI->bufferValid)
                        {
                            nameColor = V4(0, 1, 0, 1);
                            UIInteraction buttonInteraction = SendRequestInteraction(UI, UI_Trigger, AddTaxonomyRequest(root->parentTaxonomy, UI->keyboardBuffer));
                            UIAddSetValueAction(UI, &buttonInteraction, UI_Trigger, &UI->active, 0);    
                            UIAddClearAction(UI, &buttonInteraction, UI_Trigger, ColdPointer(&UI->activeParents), sizeof(UI->activeParents));    
                            UIAddSetValueAction(UI, &buttonInteraction, UI_Trigger, &UI->activeWidget, 0);    
                            UIAddInteraction(UI, input, confirmButton, buttonInteraction);
                        }                        
                    }
                } break;
                
                case EditorElement_Animation:
                {
                    Vec3 P = V3(layout->P, layout->additionalZBias) + V3(layout->nameValueDistance, layout->childStandardHeight, 0);
                    P.x += 0.5f * layout->nameValueDistance;
                    if(UI->editingTaxonomy)
                    {
                        TaxonomySlot* animationSlot = GetSlotForTaxonomy(UI->table, UI->editingTaxonomy);
                        ClientEntity test = {};
                        test.taxonomy = UI->editingTaxonomy;
                        test.lifePoints = 1;
                        test.maxLifePoints = 1;
                        test.status = R32_MAX;
                        test.animation.cameInTime = R32_MAX;
                        
                        EditorElement* pause = root->next;
                        b32 play = ToB32(GetValue(pause, "autoplay"));
                        b32 showBones = ToB32(GetValue(pause, "showBones"));
                        b32 showBitmaps = ToB32(GetValue(pause, "showBitmaps"));
                        b32 showPivots = ToB32(GetValue(pause, "showPivots"));
                        b32 drawOpened = ToB32(GetValue(pause, "drawOpened"));
                        r32 scale = ToR32(GetValue(pause, "scale"));
                        
                        r32 speed = ToR32(GetValue(pause, "speed"));
                        r32 timeToAdvance = play ? input->timeToAdvance : 0;
                        
                        test.gen = RecipeIndexGenerationData(ToU32(GetValue(pause, "seed")));
                        EditorElement* animationElement = pause->next;
                        
                        char* animationName = GetValue(animationElement, "animationName");
                        u64 nameHashID = StringHash(animationName);
                        r32 timer = ToR32(GetValue(animationElement, "time"));
                        r32 oldTimer = timer;
                        
                        if(play)
                        {
                            timer += UI->worldMode->originalTimeToAdvance * speed;
                        }
                        
                        if(timer > 1.0f)
                        {
                            timer = 0;
                        }
                        timer = Clamp01(timer);
                        
                        EditorElement* timerElement = GetElement(animationElement, "time");
                        FormatString(timerElement->value, sizeof(timerElement->value), "%f", timer);
                        test.animation.totalTime = timer;
                        
                        Vec2 animationScale = V2(scale, scale);
                        
                        SkeletonInfo info = GetSkeletonForTaxonomy(UI->table, animationSlot);
                        
                        Vec3 animationBase = P;
                        animationBase.xy += Hadamart(info.originOffset, animationScale);
                                                
                        AnimationOutput output =  PlayAndDrawEntity(UI->worldMode, UI->group, V4(-1, -1, -1, -1), &test, animationScale, 0, animationBase, 0, V4(1, 1, 1, 1), drawOpened, 0, InvertedInfinityRect2(), 1, {true, showBones, !showBitmaps, showPivots, timer, nameHashID, UI->fakeEquipment});
                        
                        if(output.hotBoneIndex >= 0)
                        {
                            char tooltip[64];
                            FormatString(tooltip, sizeof(tooltip), "bone index: %d", output.hotBoneIndex);
                            PushUITooltip(UI, tooltip, V4(1, 1, 1, 1));
                        }
                        else if(output.hotAssIndex >= 0)
                        {
                            char tooltip[64];
                            FormatString(tooltip, sizeof(tooltip), "bitmap index: %d", output.hotAssIndex);
                            PushUITooltip(UI, tooltip, V4(1, 1, 1, 1));
                        }
                        
                        
                        ObjectTransform originTransform = FlatTransform();
                        originTransform.additionalZBias = layout->additionalZBias;
                        PushRect(UI->group, originTransform, P, V2(6, 6), V4(1, 0, 0, 1));
                        
                        if(play)
                        {
                            PlaySoundForAnimation(UI->worldMode, UI->group->assets, animationSlot, nameHashID, oldTimer, timer);
                        }
                    }
                } break;

				case EditorElement_ColorPicker:
				{
                    r32 baseColorRealHeight = 2 * layout->childStandardHeight;
                   
                   
                    r32 baseColorWidth = 10.0f * layout->nameValueDistance;
                    r32 baseColorHeight = 0.6f * baseColorRealHeight;
                    
                    Vec3 P = V3(layout->P, 0);
                    P.x += 0.5f * layout->nameValueDistance;
                                        
					Rect2 baseColorRect = RectMinDim(P.xy - 0.5f * V2(0, baseColorHeight), V2(baseColorWidth, baseColorHeight));
                    
                    Vec4 c0 = V4(1, 0, 0, 1);
                    Vec4 c1 = V4(1, 1, 0, 1);
                    Vec4 c2 = V4(0, 1, 0, 1);
                    Vec4 c3 = V4(0, 1, 1, 1);
                    Vec4 c4 = V4(0, 0, 1, 1);
                    Vec4 c5 = V4(1, 0, 1, 1);
                    
                    r32 colorWidth = baseColorWidth / 6.0f;
                    r32 colorHeight = baseColorHeight;
                    Rect24C colors[6];
                    
                    Vec2 startingRunningP = P.xy - 0.5f * V2(0, baseColorHeight);
                    Vec2 runningP = startingRunningP;
                    Vec2 colorDim = V2(colorWidth, colorHeight);
                    
                    colors[0].rect = RectMinDim(runningP, colorDim);
                    SetColorsVertical(colors + 0, c0, c1);
                    runningP.x += colorWidth;
                    
                    colors[1].rect = RectMinDim(runningP, colorDim);
                    SetColorsVertical(colors + 1, c1, c2);
                    runningP.x += colorWidth;
                    
                    colors[2].rect = RectMinDim(runningP, colorDim);
                    SetColorsVertical(colors + 2, c2, c3);
                    runningP.x += colorWidth;
                    
                    colors[3].rect = RectMinDim(runningP, colorDim);
                    SetColorsVertical(colors + 3, c3, c4);
                    runningP.x += colorWidth;
                    
                    colors[4].rect = RectMinDim(runningP, colorDim);
                    SetColorsVertical(colors + 4, c4, c5);
                    runningP.x += colorWidth;
                    
                    colors[5].rect = RectMinDim(runningP, colorDim);
                    SetColorsVertical(colors + 5, c5, c0);
                    runningP.x += colorWidth;
                    
                    
                    if(PointInRect(baseColorRect, UI->relativeScreenMouse))
                    {
                        UIInteraction baseInteraction = {};
                        
                        UIAddSetValueAction(UI, &baseInteraction, UI_Trigger, &root->scrolling, true);
                        UIAddSetValueAction(UI, &baseInteraction, UI_Release, &root->scrolling, false);

                        UIAddStandardAction(UI, &baseInteraction, UI_Idle, r32, ColdPointer(&root->relativeMouseX), ColdPointer(&UI->relativeScreenMouse.x));

                        UIAddInteraction(UI, input, mouseLeft, baseInteraction);
                    }

                    if(root->scrolling)
                    {
                        root->norm = (r32) (root->relativeMouseX - baseColorRect.min.x) / baseColorWidth;
                    }
                    root->norm = Clamp01(root->norm);
                    r32 norm = root->norm;
                    
                    
                    r32 baseColorLerp = norm * baseColorWidth;
                    
                    
                    Vec4 baseColor= c0;
                    
                    r32 runningWidth = 0;
                    b32 baseColorComputed = false;
                    
                    for(u32 rectIndex = 0; rectIndex < ArrayCount(colors); ++rectIndex)
                    {
                        Rect24C* rect = colors + rectIndex;
                        
                        r32 nextRunningWidth = runningWidth + colorWidth;
                        if(!baseColorComputed && baseColorLerp <= nextRunningWidth)
                        {
                            r32 lerp = Clamp01MapToRange(runningWidth, baseColorLerp, runningWidth + colorWidth);
                            baseColor = Lerp(rect->c0, lerp, rect->c1);
                            baseColorComputed = true;
                        }
                        
                        ObjectTransform baseTransform = FlatTransform(layout->additionalZBias);
                        PushRect4Colors(UI->group, baseTransform, rect->rect, rect->c0, rect->c1, rect->c2, rect->c3);
                        runningWidth = nextRunningWidth;
                    }

                    
                   
                    r32 markerWidth =8 * layout->fontScale;
                    Rect2 lineRect = RectMinDim(baseColorRect.min + V2(baseColorLerp - 0.5f * markerWidth, 0), V2(markerWidth, colorHeight));
                    
                    PushRect(UI->group, FlatTransform(layout->additionalZBias + 0.1f), lineRect, baseColor);
                    PushRectOutline(UI->group, FlatTransform(layout->additionalZBias + 0.1f), lineRect, V4(1, 1, 1, 1), 2 * layout->fontScale);
                    
                    
                    
                    
                    
                    r32 pickColorWidth = 10.0f * layout->nameValueDistance;
                    r32 pickColorRealHeight = 6 * layout->childStandardHeight;
                    r32 pickColorHeight = 0.8f * pickColorRealHeight;
                    
                    
                    layout->P.y -= layout->childStandardHeight;
                    layout->P.y -= 0.5f * pickColorRealHeight;
                    
                    
                    
                    
                    P = V3(layout->P, 0);
                    P.x += 0.5f * layout->nameValueDistance;
                                        
					Rect2 pickColorRect = RectMinDim(P.xy - 0.5f * V2(0, pickColorHeight), V2(pickColorWidth, pickColorHeight));
                    
                   
                    
                    
                    Vec3 pickColor = baseColor.rgb;
					if(PointInRect(pickColorRect, UI->relativeScreenMouse))
					{
                        UIInteraction pickInteraction = {};
                        
                        UIAddSetValueAction(UI, &pickInteraction, UI_Trigger, &root->scrolling2, true);
                        UIAddSetValueAction(UI, &pickInteraction, UI_Release, &root->scrolling2, false);
                        
                        UIAddStandardAction(UI, &pickInteraction, UI_Idle, Vec2, ColdPointer(&root->relativeMouseP), ColdPointer(&UI->relativeScreenMouse));
                        
                        UIAddInteraction(UI, input, mouseLeft, pickInteraction);
					}

                   
                    
                    
                    Vec3 black = V3(0, 0, 0);
                    Vec3 white = V3(1, 1, 1);
                    
                    Vec2 triangleCenter = GetCenter(pickColorRect);
                    r32 triangleSide = GetDim(pickColorRect).y;
                    r32 triangleHeight = triangleSide * SquareRoot(3.0f) / 2.0f;
                    
                    Vec2 colorOffset = V2(0, 0.5f * triangleHeight);
                    Vec2 blackOffset = V2(-0.5f * triangleSide, -0.5f * triangleHeight);
                    Vec2 whiteOffset = V2(0.5f * triangleSide, -0.5f * triangleHeight);
                    
                    Vec2 up = triangleCenter + colorOffset;
                    Vec2 left = triangleCenter + blackOffset;
                    Vec2 right = triangleCenter + whiteOffset;
                    

                    ReservedVertexes triangleVertex = ReserveTriangles(UI->group, 1);
                    r32 triangleZ = layout->additionalZBias;
                    PushTriangle(UI->group, UI->group->whiteTexture, V4(-1, -1, -1, -1), &triangleVertex, V4(up, triangleZ, 0), V4(pickColor, 1.0f), V4(left, triangleZ, 0), V4(black, 1.0f), V4(right, triangleZ, 0), V4(white, 1.0f), 0);
                    
                    
                    if(root->scrolling2)
                    {
                        root->distanceColor = Clamp01(Length(root->relativeMouseP - up) / triangleSide);
                                                      root->distanceBlack = Clamp01(Length(root->relativeMouseP - left) / triangleSide);
                                                      root->distanceWhite = Clamp01(Length(root->relativeMouseP - right) / triangleSide);
                    }
                              
                              
                              
                    r32 weightColor = 1.0f - root->distanceColor;
                    r32 weightBlack = 1.0f - root->distanceBlack;
                    r32 weightWhite = 1.0f - root->distanceWhite;
                                                      
                                          
                    Vec3 finalColorRGB = weightColor * pickColor + weightBlack * black + weightWhite * white * (1.0f / (weightColor + weightBlack + weightWhite));
                    
                    Vec4 finalColor = V4(finalColorRGB, 1.0f);
                                                     
                    Vec2 lerpedP = triangleCenter + weightColor * colorOffset + weightBlack * blackOffset + weightWhite * whiteOffset;
                    r32 lerpedDim = 30.0f * layout->fontScale;
                    if(root->zooming)
                    {
                        lerpedDim *= 5.0f;
                    }
                    
                    Rect2 pickMarkRect = RectCenterDim(lerpedP, V2(lerpedDim, lerpedDim));
                    if(PointInRect(pickMarkRect, UI->relativeScreenMouse))
                    {
                        UIInteraction zoomInteraction = {};
                        
                        UIAddSetValueAction(UI, &zoomInteraction, UI_Trigger, &root->zooming, true);
                        UIAddSetValueAction(UI, &zoomInteraction, UI_Release, &root->zooming, false);
                                             
                        UIAddInteraction(UI, input, mouseRight, zoomInteraction);
                    }
                    
                    PushRect(UI->group, FlatTransform(layout->additionalZBias + 0.1f), pickMarkRect, finalColor);
                    PushRectOutline(UI->group, FlatTransform(layout->additionalZBias + 0.1f), pickMarkRect, V4(1, 1, 1, 1), 1 * layout->fontScale);

                    
                    

                    layout->P.y -= 0.5f * pickColorRealHeight;
                    layout->P.y -= layout->childStandardHeight;
                    
                    EditorElement* colorElement = root->next;
                    Assert(colorElement);
                    
                    EditorElement* r = GetElement(colorElement, "r");
                    FormatString(r->value, sizeof(r->value), "%f", finalColor.r);
                    
                    EditorElement* g = GetElement(colorElement, "g");
                    FormatString(g->value, sizeof(g->value), "%f", finalColor.g);
                    
                    EditorElement* b = GetElement(colorElement, "b");
                    FormatString(b->value, sizeof(b->value), "%f", finalColor.b);
				} break;
                
                case EditorElement_GroundParams:
                {
                    EditorElement* paramsElement = root->next;
                    Assert(paramsElement);
                                       
                    UI->showGroundOutline = ToB32(GetValue(paramsElement, "showBorders"));
                    UI->uniformGroundColors = ToB32(GetValue(paramsElement, "uniformColors"));
                    UI->groundViewMode = (GroundViewMode) GetValuePreprocessor(GroundViewMode, GetValue(paramsElement, "viewType"));
                    UI->chunkApron = ToU32(GetValue(paramsElement, "chunkApron"));

                    if(UI->chunkApron >= 4)
                    {
                        UI->groundViewMode = Max(UI->groundViewMode, GroundView_Tile);
                    }
                    
                    if(UI->chunkApron >= 8)
                    {
                        //UI->groundViewMode = GroundView_Chunk;
                    }
                    
                    UI->cameraOffset = ToV3(GetElement(paramsElement, "cameraOffset"));
                } break;
            }
            
			if(shadowLabel[0])
			{
				Vec4 shadowColor = V4(0.6f, 0.6f, 0.6f, 1.0f);
				PushUIOrthoText(UI, shadowLabel, layout->fontScale, nameP, shadowColor, layout->additionalZBias);
			}
            if(showName)
            {
                PushUIOrthoText(UI, nameToShow, layout->fontScale, nameP, nameColor, layout->additionalZBias);
            }
            
            totalResult.bounds = Union(totalResult.bounds, result);
            
            ++childIndex;
            if(deleteName)
            {
                ZeroStruct(root->name);
            }
            
        }
    }
    
    return totalResult;
}

inline b32 ChangesMustBeSaved(u32 widgetIndex)
{
    b32 result = true;
    
    if(widgetIndex == EditorWidget_Animation ||
       widgetIndex == EditorWidget_SoundDatabase ||
       widgetIndex == EditorWidget_GeneralButtons || 
       widgetIndex == EditorWidget_ColorPicker ||
       widgetIndex == EditorWidget_GroundParams ||
       widgetIndex == EditorWidget_Misc)
    {
        result = false;
    }
    
    return result;
}

inline void UIRenderEditor(UIState* UI, PlatformInput* input)
{
    RenderGroup* group = UI->group;
    GameRenderCommands* commands = group->commands;
    
    r32 width = (r32) commands->settings.width;
    r32 height = (r32) commands->settings.height;
    
    SetCameraTransform(group, Camera_Orthographic, 0.0f, V3(2.0f / width, 0.0f, 0.0f), V3(0.0f, 2.0f / width, 0.0f), V3( 0, 0, 1));
    
    UIAddInteraction(UI, input, undo, UIUndoInteraction(UI, UI_Trigger));
    UIAddInteraction(UI, input, redo, UIRedoInteraction(UI, UI_Trigger));
    
    if(UI->instantiatingTaxonomy)
    {
        Vec3 mouseOffset = UI->worldMouseP;
        UIAddInteraction(UI, input, mouseRight,SendRequestInteraction(UI, UI_Trigger, InstantiateTaxonomyRequest(UI->instantiatingTaxonomy, mouseOffset)));
    }
    
    if(input->altDown)
    {
        UIInteraction moveInteraction = SendRequestInteraction(UI, UI_Trigger, MovePlayerRequest(V3(10, 0, 0)));
        UIAddInteraction(UI, input, moveRight, moveInteraction);
        
        moveInteraction = SendRequestInteraction(UI, UI_Trigger, MovePlayerRequest(V3(-10, 0, 0)));
        UIAddInteraction(UI, input, moveLeft, moveInteraction);
        
        moveInteraction = SendRequestInteraction(UI, UI_Trigger, MovePlayerRequest(V3(0, 10, 0)));
        UIAddInteraction(UI, input, moveUp, moveInteraction);
        
        moveInteraction = SendRequestInteraction(UI, UI_Trigger, MovePlayerRequest(V3(0, -10, 0)));
        UIAddInteraction(UI, input, moveDown, moveInteraction);
        
        
    }
    
    UI->saveWidgetLayoutTimer += UI->worldMode->originalTimeToAdvance;
    if(UI->saveWidgetLayoutTimer >= 10.0f)
    {
        UI->saveWidgetLayoutTimer = 0;
        
        WidgetPermanent toSave[EditorWidget_Count];
        
        for(u32 widgetIndex = 0; widgetIndex < EditorWidget_Count; ++widgetIndex)
        {
            toSave[widgetIndex] = UI->widgets[widgetIndex].permanent;
        }
        
        platformAPI.DEBUGWriteFile("widget", toSave, sizeof(toSave));
    }
    
    UI->autosaveWidgetTimer += UI->worldMode->originalTimeToAdvance;
    if(UI->autosaveWidgetTimer >= 2000000.0f)
    {
        UI->autosaveWidgetTimer = 0;
        
        for(u32 widgetIndex = 0; widgetIndex < EditorWidget_Count; ++widgetIndex)
        {
            EditorWidget* widget = UI->widgets + widgetIndex;
            if(ChangesMustBeSaved(widgetIndex) && widget->changeCount && widget->fileName)
            {
                widget->changeCount = 0;
                SendSaveAssetDefinitionFile(widget->fileName, widget->root);
            }
        }
        
        
        TaxonomySlot* editing = GetSlotForTaxonomy(UI->table, UI->editingTaxonomy);
        if(editing->editorChangeCount)
        {
            editing->editorChangeCount = 0;
            SendSaveTabRequest(UI->editingTaxonomy);
        }
        
    }
    
    
    Vec2 importantMessageP = V2(-900, +500);
    r32 importantMessageScale = 0.42f;
    Vec4 importantColor = V4(1, 0, 0, 1);
    
    if(UI->table->errorCount)
    {
        PushUIOrthoText(UI, "There were errors when loading the assets! check the editorError file", importantMessageScale, importantMessageP, importantColor);
        importantMessageP.y -= 40.0f;
    }
    
    if(UI->reloadingAssets)
    {
        PushUIOrthoText(UI, "Reloading Assets...", importantMessageScale, importantMessageP, importantColor);
    }
    else if(UI->patchingLocalServer)
    {
        char* patchText = "Patching Local Server...";
        if(UI->patchingLocalServer == 2)
        {
            patchText = "Checking if is possible to patch...";
        }
        
        PushUIOrthoText(UI, patchText, importantMessageScale, importantMessageP, importantColor);
    }
    
    UI->hotStructThisFrame = false;
    
    TaxonomySlot* editingSlot = GetSlotForTaxonomy(UI->table, UI->editingTaxonomy);
    UI->editingTabIndex = Wrap(0, UI->editingTabIndex, editingSlot->tabCount);
    
    EditorTab* editingTab = editingSlot->tabs + UI->editingTabIndex;
    EditorElement* editingRoot = editingTab->editable ? editingTab->root : &UI->uneditableTabRoot;
    
    EditorWidget* tabWidget = UI->widgets + 1;
    tabWidget->root = editingRoot;
    
    UI->widgets[0].root = UI->editorTaxonomyTree;
    UI->widgets[3].root = UI->table->soundNamesRoot;
    UI->widgets[5].root = UI->table->soundEventsRoot;
    
    
    UIInteraction esc = UISetValueInteraction(UI, UI_Trigger, &UI->active, 0);
    UIAddSetValueAction(UI, &esc, UI_Trigger, &UI->activeLabel, 0);
    UIAddSetValueAction(UI, &esc, UI_Trigger, &UI->activeLabelParent, 0);
    UIAddSetValueAction(UI, &esc, UI_Trigger, &UI->activeWidget, 0);
    UIAddClearAction(UI, &esc, UI_Trigger, ColdPointer(UI->keyboardBuffer), sizeof(UI->keyboardBuffer));
    UIAddClearAction(UI, &esc, UI_Trigger, ColdPointer(&UI->activeParents), sizeof(UI->activeParents));
    UIAddSetValueAction(UI, &esc, UI_Trigger, &UI->copying, 0);
    UIAddSetValueAction(UI, &esc, UI_Trigger, &UI->currentAutocompleteSelectedIndex, -1);
    UIAddInteraction(UI, input, escButton, esc);
    
	if(UI->bufferValid)
	{
		if(UI->active)
		{
            UIInteraction clickInteraction = {};
			AddConfirmActions(UI, UI->activeWidget, &clickInteraction, UI->active->value, sizeof(UI->active->value));
			UIAddInteraction(UI, input, mouseLeft, clickInteraction);
		}
		else if(UI->activeLabel)
		{
            UIInteraction clickInteraction = {};
			AddConfirmActions(UI, UI->activeWidget, &clickInteraction, UI->activeLabel->name, sizeof(UI->activeLabel->name));
			UIAddInteraction(UI, input, mouseLeft, clickInteraction);
            
		}
	}
#if 0
    if(UI->active)
	{
		if(ctrlDown)
		{
			How do we update the grandparent?
                UpOneLevel();
		}
		else if(shiftDown)
		{
			if(firstInList || firstValue)
			{
				DrillDown();
			}
		}
		else
		{
			if(editable(UI->active->next))
			{
				UIInteraction moveInteraction;
				if(bufferValid)
				{
					AddConfirmInteraction();
				}
                
				AddClearAction(keyboardBuffer);
				AddAction(moveDown, UI->active, UI->active->next);
                
				AddAction(moveUp, UI->active, UI->active->prev);
			}
		}
	}
#endif
    
    
    u32 current = StrLen(UI->keyboardBuffer);
    u32 maxToCopy = sizeof(UI->keyboardBuffer) - (current + 1);
    char* appendHere = UI->keyboardBuffer + current;
    
    if(IsDown(&input->backButton) && current > 0)
    {
        UI->backspacePressedTime += UI->worldMode->originalTimeToAdvance;
        
        if(Pressed(&input->backButton) || UI->backspacePressedTime >= 0.12f)
        {
            UI->backspacePressedTime = 0;
            *(--appendHere) = 0;
        }
    }
    else
    {
        for(u8 charIndex = 0; charIndex < 0xff; ++charIndex)
        {
            if(input->isDown[charIndex] && !input->wasDown[charIndex] && maxToCopy > 0)
            {
                *appendHere++ = charIndex;
                --maxToCopy;
                UI->currentAutocompleteSelectedIndex = -1;
            }
        }
    }
    
    
    if(UI->showCursor)
    {
        FormatString(UI->showBuffer, sizeof(UI->showBuffer), "%s_", UI->keyboardBuffer);
    }
    else
    {
        FormatString(UI->showBuffer, sizeof(UI->showBuffer), "%s", UI->keyboardBuffer);
    }
    
    UI->bufferValid = false;
    
    if(UI->keyboardBuffer[0] && UI->activeLabel)
    {
        UI->bufferValid = true;
        char* labelType = UI->activeLabelParent->labelName;
        if(labelType[0])
        {
            UI->bufferValid = false;
            UIAutocomplete* options = UIFindAutocomplete(UI, UI->activeParents, labelType);
            if(options)
            {
                UIAutocompleteBlock* block = options->firstBlock;
                while(block && !UI->bufferValid)
                {
                    for(u32 nameIndex = 0; nameIndex < block->count; ++nameIndex)
                    {
                        char* name = block->names[nameIndex];
                        if(StrEqual(name, UI->keyboardBuffer))
                        {
                            UI->bufferValid = true;
                            break;
                        }
                    }
                        
                    block = block->next;
                }
            }
        }
        else
        {
            UI->bufferValid = true; 
        }
    }
    else if(UI->keyboardBuffer[0] && UI->active)
    {
        switch(UI->active->type)
        {
            case EditorElement_EmptyTaxonomy:
            {
                ShortcutSlot* shortcut = GetShortcut(UI->table, UI->keyboardBuffer);
				char withPound[64];
				FormatString(withPound, sizeof(withPound), "#%s", UI->keyboardBuffer);
				ShortcutSlot* shortcutPound = GetShortcut(UI->table, withPound);
                
                UI->bufferValid = (shortcut == 0 && shortcutPound == 0);
            } break;
            
            case EditorElement_String:
            {
                if(SpecialHandling(UI->active->name))
                {
                   UI->bufferValid = true;
                }
                UIAutocomplete* options = UIFindAutocomplete(UI, UI->activeParents, UI->active->name);
                if(options)
                {
                    UIAutocompleteBlock* block = options->firstBlock;
                    while(block && !UI->bufferValid)
                    {
                        for(u32 nameIndex = 0; nameIndex < block->count; ++nameIndex)
                        {
                            char* name = block->names[nameIndex];
                            if(StrEqual(name, UI->keyboardBuffer))
                            {
                                UI->bufferValid = true;
                                break;
                            }
                        }
                        
                        block = block->next;
                    }
                }
                else
                {
                    //UI->bufferValid = true;
                }
            } break;
            
            case EditorElement_Real:
            {
                UI->bufferValid = true;
                
                b32 encounteredPoint = false;
                b32 signEncountered = false;
                
                char* test = UI->keyboardBuffer;
                if(*test == '-' || *test == '+')
                {
                    signEncountered = true;
                    ++test;
                }
                
                if(*test == '.')
                {
                    UI->bufferValid = false;
                }
                else
                {
                    for(; *test; ++test)
                    {
                        if(*test == '.')
                        {
                            if(encounteredPoint)
                            {
                                UI->bufferValid = false;
                                break;
                            }
                            else
                            {
                                encounteredPoint = true;
                            }
                        }
                        else if(*test < '0' || *test > '9')
                        {
                            UI->bufferValid = false;
                            break;
                        }
                    }
                }
            } break;
            
            case EditorElement_Signed:
            {
                UI->bufferValid = true;
                char* test = UI->keyboardBuffer;
                if(*test == '-' || *test == '+')
                {
                    ++test;
                }
                
                for(; *test; ++test)
                {
                    if(*test < '0' || *test > '9')
                    {
                        UI->bufferValid = false;
                        break;
                    }
                }
            } break;
            
            case EditorElement_Unsigned:
            {
                UI->bufferValid = true;
                for(char* test = UI->keyboardBuffer; *test; ++test)
                {
                    if(*test < '0' || *test > '9')
                    {
                        UI->bufferValid = false;
                        break;
                    }
                }
            } break;
            
            InvalidDefaultCase;
        }
        
    }
    
    
    r32 widgetZ = 0.0f;
    for(u32 widgetIndex = 0; widgetIndex < EditorWidget_Count; ++widgetIndex)
    {
        EditorWidget* widget = UI->widgets + widgetIndex;
       
        
        widget->permanent.fontSize = Clamp(0.3f, widget->permanent.fontSize, 3.0f);
        
        EditorLayout layout_ = widget->layout;
        layout_.fontScale *= widget->permanent.fontSize;
        layout_.nameValueDistance *= widget->permanent.fontSize;
        layout_.childStandardHeight *= widget->permanent.fontSize;
        layout_.squareDim *= widget->permanent.fontSize;
        layout_.lineThickness *= widget->permanent.fontSize;
        layout_.padding *= widget->permanent.fontSize;
        
        if(widget->necessaryRole & UI->worldMode->editorRoles)
        {
            if(widget->needsToBeReloaded)
            {
                TaxonomySlot* editing = GetSlotForTaxonomy(UI->table, UI->editingTaxonomy);
                Import(editing, widget->root);
                
                if(StrEqual(widget->name, "Editing Tabs"))
                {
                    SendNewTabMessage();
                    SendEditorElements(widget->root);
                    SendReloadEditingMessage(UI->editingTaxonomy, UI->editingTabIndex);
                }
                
                
                widget->needsToBeReloaded = false;
            }
            
            Vec2 resizeP = widget->permanent.P;
            Vec2 widgetP = resizeP + widget->permanent.fontSize * V2(20, -8);
            Vec2 moveDim = widget->permanent.fontSize * V2(8, 8);
            Rect2 rect = RectCenterDim(resizeP, moveDim);
            Vec4 color = V4(1, 1, 1, 1);
            if(PointInRect(rect, UI->relativeScreenMouse))
            {
                UIInteraction moveInteraction = {};
                UIAddStandardAction(UI, &moveInteraction, UI_Idle, Vec2, ColdPointer(&widget->permanent.P),ColdPointer(&UI->relativeScreenMouse));
                UIAddSetValueAction(UI, &moveInteraction, UI_Trigger, &widget->moving, true);
                UIAddSetValueAction(UI, &moveInteraction, UI_Release, &widget->moving, false);
                UIAddSetValueAction(UI, &moveInteraction, UI_Trigger, &widget->movingClipWidth, widget->oldClipWidth);
                UIAddSetValueAction(UI, &moveInteraction, UI_Trigger, &widget->movingClipHeight, widget->oldClipHeight);
                
                UIAddInteraction(UI, input, mouseLeft, moveInteraction);
                color = V4(1, 0, 0, 1);
            }
            
            ObjectTransform sizeTranform = FlatTransform();
            sizeTranform.additionalZBias = widgetZ;
            
            PushRect(UI->group, sizeTranform, rect, color);
            
            
            EditorLayout* layout = &layout_;
            layout->P = widgetP;
            layout->additionalZBias = widgetZ;
            
            Rect2 widgetTitleBounds = GetUIOrthoTextBounds(UI, widget->name, layout->fontScale, widgetP);
            
            r32 widgetAlpha = widget->permanent.expanded ? 1.0f : 0.1f;
            Vec4 widgetColor = V4(0, 1, 0, widgetAlpha);
            if(PointInRect(widgetTitleBounds, UI->relativeScreenMouse))
            {
                widgetColor = V4(1, 0, 0, 1);
                UIAddInteraction(UI, input, mouseLeft, UISetValueInteraction(UI, UI_Trigger, &widget->permanent.expanded, !widget->permanent.expanded));
            }
            PushUIOrthoText(UI, widget->name, layout->fontScale, widgetP, widgetColor, layout->additionalZBias);
            
            if(widget->permanent.expanded)
            {
                switch(widgetIndex)
                {
                    case EditorWidget_EditingTaxonomyTabs:
                    {
                        if(UI->editingTaxonomy)
                        {
                            Vec2 leftP = widgetTitleBounds.min + V2(GetDim(widgetTitleBounds).x, 0) + V2(20.0f, 0);
                            
                            
                            UIButton leftButton = UIBtn(UI, leftP, layout, V4(1, 0, 0, 1), " << ");
                            UIButtonInteraction(&leftButton, UISetValueInteraction(UI, UI_Trigger, &UI->editingTabIndex, UI->editingTabIndex - 1));
                            UIDrawButton(UI, input, &leftButton);
                            
                            Vec2 rightP = UIFollowingP(&leftButton, 10.0f);
                            UIButton rightButton = UIBtn(UI, rightP, layout, V4(1, 0, 0, 1), " >> ");
                            UIButtonInteraction(&rightButton, UISetValueInteraction(UI, UI_Trigger, &UI->editingTabIndex, UI->editingTabIndex + 1));
                            UIDrawButton(UI, input, &rightButton);
                            
                            
                            
                            Vec2 saveP = UIFollowingP(&rightButton, 30.0f);
                                                        
                            b32 saveActive = (editingSlot->editorChangeCount);
                                UIInteraction saveInteraction = SendRequestInteraction(UI, UI_Trigger, SaveTaxonomyTabRequest());
                            
                            char saveText[128];
                            
                            char* name = editingSlot->name[0] == '#' ? editingSlot->name + 1 : editingSlot->name;
                            FormatString(saveText, sizeof(saveText), "Save %s", name);
                            UIButton saveButton = UIBtn(UI, saveP, layout, V4(1, 0, 0, 1), saveText, saveActive, saveInteraction);
                            UIDrawButton(UI, input, &saveButton);
                            
                            u32 parentTaxonomy = GetParentTaxonomy(UI->table, editingSlot->taxonomy);
                            if(parentTaxonomy)
                            {
                                Vec2 editParentP = UIFollowingP(&saveButton, 30.0f);
                                UIButton editParentButton = UIBtn(UI, editParentP, layout, V4(1, 0, 0, 1), "Edit Parent");
                                
                                
                                UIButtonInteraction(&editParentButton, SendRequestInteraction(UI, UI_Trigger, EditRequest(parentTaxonomy))); 
                                UIDrawButton(UI, input, &editParentButton);
                            }
                        }
                    } break;
                    
                    case EditorWidget_SoundDatabase:
                    {
                    } break;
                    
                    
                    
                    case EditorWidget_GeneralButtons:
                    {
                        Vec2 reloadP = widgetTitleBounds.min + V2(GetDim(widgetTitleBounds).x, 0) + V2(20.0f, 0);
                        
                        b32 reloadActive = (!UI->reloadingAssets && !UI->patchingLocalServer);
                            UIInteraction reloadInteraction = SendRequestInteraction(UI, UI_Trigger, ReloadAssetsRequest());
                        UIButton reloadButton = UIBtn(UI, reloadP, layout, V4(1, 0, 0, 1), " RELOAD ASSETS ", reloadActive, reloadInteraction);
                        
                        UIDrawButton(UI, input, &reloadButton);
                        
                        
                        
                        b32 checkActive = (!UI->reloadingAssets && !UI->patchingLocalServer && !UIChildModified(UI->table, &UI->table->root));
                        UIInteraction checkInteraction = SendRequestInteraction(UI, UI_Trigger, PatchCheckRequest());
                        Vec2 patchCheckP = UIFollowingP(&reloadButton, 20);
                        UIButton patchCheckButton = UIBtn(UI, patchCheckP, layout, V4(1, 0, 0, 1), " PATCH CHECK ", checkActive, checkInteraction);
                        
                        UIDrawButton(UI, input, &patchCheckButton);
                        
                        
                        b32 patchActive = (!UI->reloadingAssets && !UI->patchingLocalServer && !UI->table->errorCount);
                        
                        UIInteraction patchInteraction = SendRequestInteraction(UI, UI_Trigger, PatchServerRequest());
                        Vec2 patchP = UIFollowingP(&patchCheckButton, 20);
                        UIButton patchButton = UIBtn(UI, patchP, layout, V4(1, 0, 0, 1), " PATCH SERVER ", patchActive, patchInteraction);
                        
                        UIDrawButton(UI, input, &patchButton);
                        
                        
                        EditorWidget* misc = UI->widgets + EditorWidget_Misc;
                        u32 seed = ToU32(GetValue(misc->root, "worldSeed"));
                        UIInteraction regenerateInteraction = SendRequestInteraction(UI, UI_Trigger, RegenerateWorldRequest(seed));
                        Vec2 regenerateP = UIFollowingP(&patchButton, 20);
                        UIButton regenerateButton = UIBtn(UI, regenerateP, layout, V4(1, 0, 0, 1), " RECREATE WORLD ", true, regenerateInteraction);
                        
                        UIDrawButton(UI, input, &regenerateButton);
                        
                        
                    } break;
                    
                    case EditorWidget_SoundEvents:
                    {
                        for(EditorElement* test = widget->root; test; test = test->next)
                        {
                            if(StrEqual(test->name, "test params"))
                            {
                                UI->fakeDistanceFromPlayer = ToR32(GetValue(test, "distanceFromPlayer"), 1);   
                                break;
                            }         
                        }
                        
                        Vec2 saveP = widgetTitleBounds.min + V2(GetDim(widgetTitleBounds).x, 0) + V2(20.0f, 0);
                        
                        UIInteraction saveInteraction =SendRequestInteraction(UI, UI_Trigger, SaveAssetFadFileRequest(widget));
                        UIAddReloadElementAction(UI, &saveInteraction, UI_Trigger, UI->table->soundEventsRoot);
                        b32 saveActive = (widget->changeCount);
                        
                        UIButton saveButton = UIBtn(UI, saveP, layout, V4(1, 0, 0, 1.0f), " SAVE ", saveActive, saveInteraction);
                                                
                        UIDrawButton(UI, input, &saveButton);
                    } break;
                    
                    case EditorWidget_Components:
                    {
                        Vec2 saveP = widgetTitleBounds.min + V2(GetDim(widgetTitleBounds).x, 0) + V2(20.0f, 0);
                        
                        UIInteraction saveInteraction = SendRequestInteraction(UI, UI_Trigger, SaveAssetFadFileRequest(widget));
                        b32 saveActive = (widget->changeCount);
                        
                        UIButton saveButton = UIBtn(UI, saveP, layout, V4(1, 0, 0, 1.0f), " SAVE ", saveActive, saveInteraction);
                        
                        UIDrawButton(UI, input, &saveButton);
                    } break;
                    
                    case EditorWidget_Misc:
                    {
                        UI->worldMode->currentPhase = (ForgDayPhase) GetValuePreprocessor(ForgDayPhase, GetValue(widget->root, "dayPhase"));
                        
                        UI->worldMode->windSpeed = ToR32(GetValue(widget->root, "windSpeed"));
                    } break;
                }
            }
            
            
            layout->P.y -= layout->childStandardHeight;

            
            r32 widgetMaxY = 0.5f * height + widget->permanent.P.y - 20;
            r32 widgetMinX = 0.5f * width + widget->permanent.P.x;
            
            if(widget->moving)
            {
                widget->permanent.resizeP.x = widget->permanent.P.x + widget->movingClipWidth;
                widget->permanent.resizeP.y = widget->permanent.P.y - widget->movingClipHeight;
            }
            
            r32 suggestedX = widget->permanent.resizeP.x - widget->permanent.P.x;
            r32 suggestedY = widget->permanent.P.y - widget->permanent.resizeP.y;

            r32 clipWidth = Max(suggestedX, 60);
            r32 clipHeight = Max(suggestedY, 60);
            
            
            r32 widgetMinY = widgetMaxY - clipHeight;
            r32 widgetMaxX = widgetMinX + clipWidth;
            
            Rect2i clipRect = RectMinMax((i32) widgetMinX, (i32) widgetMinY, (i32) widgetMaxX, (i32) widgetMaxY);
            widget->oldClipWidth = clipRect.maxX - clipRect.minX;
            widget->oldClipHeight = clipRect.maxY - clipRect.minY;
                        
            Rect2 clipRectReal = RectMinMax(V2((r32) clipRect.minX - 0.5f * width, (r32) clipRect.minY - 0.5f * height), V2((r32) clipRect.maxX - 0.5f * width, (r32) clipRect.maxY - 0.5f * height));
            
            
            PushClipRect(group, clipRect);            
            layout->P.y += widget->dataOffsetY;
            
            widget->maxDataY = (i32) (layout->P.y + 0.5f * height);
            
            if(widget->permanent.expanded && widget->root)
            {
                EditorElementParents parents = {};
                Rect2 widgetBounds = UIRenderEditorTree(UI, widget, layout, parents, 0, false, V4(1, 1, 1, 1), widget->root, input, false).bounds;
                
                widget->minDataY = (i32) (widgetBounds.min.y + 0.5f * height);
                
                
                
                r32 resizeDim = layout->squareDim;
                Vec4 resizeColor = V4(1, 1, 1, 1);
                
                Vec2 clipRectrightDown = V2(clipRectReal.max.x, clipRectReal.min.y);
                Vec2 resizeMin = V2(clipRectrightDown.x - resizeDim, clipRectrightDown.y);
                Vec2 resizeMax = V2(clipRectrightDown.x, clipRectrightDown.y + resizeDim);
                
                
                Rect2 resizeRect = RectMinMax(resizeMin, resizeMax);
                if(PointInRect(resizeRect, UI->relativeScreenMouse))
                {
                    resizeColor = V4(1, 0, 0, 1);
                    UIInteraction mouseInteraction = {};                    
                    
                    UIAddStandardAction(UI, &mouseInteraction, UI_Idle, r32, ColdPointer(&widget->permanent.resizeP.x), ColdPointer(&UI->relativeScreenMouse.x));
                    UIAddStandardAction(UI, &mouseInteraction, UI_Idle, r32,  ColdPointer(&widget->permanent.resizeP.y), ColdPointer(&UI->relativeScreenMouse.y));

                    
                    UIAddInteraction(UI, input, mouseLeft, mouseInteraction);
                }
                PushRect(group, FlatTransform(layout->additionalZBias), resizeRect, resizeColor);
                
                r32 boundsAlpha = 0.5f;
                
                
            if(PointInRect(clipRectReal, UI->relativeScreenMouse))
            {
                boundsAlpha = 0.8f;   
                if(input->ctrlDown)
                {
                    if(input->mouseWheelOffset)
                    {
                        widget->permanent.fontSize += (r32) input->mouseWheelOffset * 0.1f;
                        
                    }                    
                }
                else
                {
                    b32 allowScrolling = false;
                    if(input->mouseWheelOffset > 0)
                    {
                            if(widgetIndex == EditorWidget_Animation)
                            {
                                allowScrolling = true;
                            }
                            else
                            {
                                allowScrolling = (widget->maxDataY >= clipRect.maxY - 20);                                                       
                            }

                    }
                    else if(input->mouseWheelOffset < 0)
                    {
                        allowScrolling = (widget->minDataY <= (clipRect.minY + 20));                       
                    }
                    
                    
                    if(allowScrolling)
                    {
                        widget->dataOffsetY -= input->mouseWheelOffset * 10.0f;   
                    }
                }
            }
                boundsAlpha = 1.0f;
                
            ObjectTransform widgetBoundsTransform = FlatTransform();
            widgetBoundsTransform.additionalZBias = widgetZ - 0.001f;
            PushRect(UI->group, widgetBoundsTransform, clipRectReal, V4(0, 0, 0, boundsAlpha));
            PushRectOutline(UI->group, widgetBoundsTransform, clipRectReal, V4(1, 1, 1, boundsAlpha), 2.0f);
        
            }
                        
                
            widgetZ += 1.0f;
        }
        
        if(ChangesMustBeSaved(widgetIndex) && widget->changeCount)
        {
            input->allowedToQuit = false;
        }
        
        
        if(UI->hotStructThisFrame)
        {
            ObjectTransform structBoundsTranform = FlatTransform();
            structBoundsTranform.additionalZBias = UI->hotStructZ;
            
            PushRectOutline(UI->group, structBoundsTranform, UI->hotStructBounds, UI->hotStructColor, 2);
        }
        
        
        PushClipRect(group, RectMinMax(0, 0, (i32) width, (i32) height));
    }
    
    if(input->allowedToQuit && UIChildModified(UI->table, &UI->table->root))
    {
        input->allowedToQuit = false;
    }
    
    UI->pastedTimeLeft -= UI->worldMode->originalTimeToAdvance;
    if(UI->pastedTimeLeft <= 0)
    {
        UI->pasted = 0;
    }
    
    if(UI->dragging)
    {
        EditorLayout layout = UI->widgets[0].layout;
        layout.P = UI->relativeScreenMouse;
        layout.additionalZBias = 10.0f;
        
        EditorElementParents parents = {};
        
        EditorElement* oldNext = UI->dragging->next;
        UIRenderEditorTree(UI, 0, &layout, parents, 0, false, V4(1, 1, 1, 1), UI->dragging, input, false);
        UI->dragging->next = oldNext;
    }
}




inline Object* GetFocusObject(ClientEntity* entityC)
{
    Object* result = 0;
    i32 objIndex = entityC->animation.output.focusObjectIndex;
    if(objIndex >= 0)
    {
        u8 objectIndex = SafeTruncateToU8(objIndex);
        result = entityC->objects.objects + objectIndex;
    }
    return result;
}


inline void AddToAvailableIngredients(UIState* UI, Object* object)
{
    for(u32 slotIndex = 0; slotIndex < ArrayCount(UI->ingredientHashMap); ++slotIndex)
    {
        UIOwnedIngredient* hashSlot = UI->ingredientHashMap + slotIndex;
        if(hashSlot->taxonomy == object->taxonomy)
        {
            hashSlot->quantity += SafeTruncateToU16(object->quantity);
            break;
        }
    }
}

inline void UIRenderTooltip(UIState* UI)
{
    RenderGroup* group = UI->group;
    GameRenderCommands* commands = group->commands;
    
    r32 tooltipZ = 100;
    if(commands)
    {
        r32 width = (r32) commands->settings.width;
        r32 height = (r32) commands->settings.height;
        Vec2 tooltipP = UI->screenMouseP - 0.5f * V2(width, height);
        tooltipP.y += 30.0f;
        Vec2 centerTooltipP = tooltipP;
        
        SetCameraTransform(group, Camera_Orthographic, 0.0f, V3(2.0f / width, 0.0f, 0.0f), V3(0.0f, 2.0f / width, 0.0f), V3( 0, 0, 1));
        if(UI->font)
        {
            r32 tooltipScale = 0.42f;
            
            char* text = UI->tooltipText;
            Rect2 tooltipDim = UIOrthoTextOp(group, UI->font, UI->fontInfo, text, tooltipScale, V3(0, 0, 0), TextOp_getSize,V4(1, 1, 1, 1));
            Vec2 textDim = GetDim(tooltipDim);
            
            Vec2 textP = tooltipP;
            textP.x -= 0.5f * textDim.x;
            
            centerTooltipP.y += 0.5f * textDim.y;
            
            UIOrthoTextOp(group, UI->font, UI->fontInfo, text, tooltipScale, V3(textP, tooltipZ), TextOp_draw,V4(1, 1, 1, 1));
        }
        
        if(UI->prefix || UI->suffix)
        {
            BitmapId ID = UI->scrollIconID;
            r32 prefixHeight = 20.0f;
            
            Vec2 prefixP = centerTooltipP + V2(0, 28.0f);
            
            PushBitmap(group, FlatTransform(), ID, V3(prefixP, tooltipZ), prefixHeight, V2( 1.0f, 1.0f ), V4(0.0f, 0.0f, 0.0f, 1.0f));
            PushBitmap(group, FlatTransform(), ID, V3(prefixP, tooltipZ), prefixHeight, V2( 1.0f, 1.0f ), V4(1, 1, 1, 1));
            
            Vec2 suffixP = centerTooltipP - V2(0, 28.0f);
            
            ObjectTransform suffixTransform = FlatTransform();
            suffixTransform.angle = 180;
            
            PushBitmap(group, suffixTransform, ID, V3(suffixP, tooltipZ), prefixHeight, V2( 1.0f, 1.0f ), V4(0.0f, 0.0f, 0.0f, 1.0f));
            PushBitmap(group, suffixTransform, ID, V3(suffixP, tooltipZ), prefixHeight, V2( 1.0f, 1.0f ), V4(1, 1, 1, 1));
        }
    }
}

inline Rect2 OverdrawInventoryEntity(RenderGroup* group, GameModeWorld* worldMode,  ClientEntity* entity, Vec3 P, Vec2 objectDim)
{
    Rect2 result = InvertedInfinityRect2();
    
    Vec3 oldP = entity->P;
    r32 oldModulation = entity->modulationWithFocusColor;
    b32 oldFlip = entity->animation.flipOnYAxis;
    
    entity->P = P + V3(worldMode->cameraWorldOffset.xy, 0) + V3(worldMode->cameraEntityOffset, 0);
    entity->animation.flipOnYAxis = false;
    
    AnimationEntityParams params = ContainerEntityParams(true);
    params.bounds = RectCenterDim(V2(0, 0), objectDim);
    
    entity->animation.output = RenderEntity(group, worldMode, entity, 0, params);
    r32 ignored;
    result = ProjectOnScreenCameraAligned(group, entity->P, entity->animation.bounds, &ignored);
    
    entity->P = oldP;
    entity->animation.flipOnYAxis = oldFlip;
    
    return result;
}

inline void UIOverdrawInventoryView(UIState* UI)
{
    RenderGroup* group = UI->group;
    GameModeWorld* worldMode = UI->worldMode;
    
    ObjectTransform alphaRectTransform = FlatTransform();
    
    if(UI->mode == UIMode_Equipment)
    {
        Vec2 objectDim = V2(1.7f, 1.7f);
        ClientEntity* onTopEntity = 0;
        if(UI->lockedInventoryID1)
        {
            onTopEntity = GetEntityClient(worldMode, UI->lockedInventoryID1);
            if(onTopEntity)
            {
                UI->screenBoundsInventory1 =OverdrawInventoryEntity(group, worldMode, onTopEntity, V3(1.4f, 0.5f, 0), objectDim);
            }
        }
        
        if(UI->lockedInventoryID2)
        {
            onTopEntity = GetEntityClient(worldMode, UI->lockedInventoryID2);
            if(onTopEntity)
            {
                UI->screenBoundsInventory2 = OverdrawInventoryEntity(group, worldMode, onTopEntity, V3(-1.4f, 0.5f, 0), objectDim);
            }
        }
    }
    
    if(UI->draggingEntity.taxonomy && !IsValid(UI->player->animation.nearestCompatibleSlotForDragging))
    {
        UI->draggingEntity.animation.cameInTime = R32_MAX;
        Vec2 standardDraggingObjectDim = V2(1.5f, 1.5f);
        r32 zoomCoeff = UI->worldMode->cameraWorldOffset.z / UI->worldMode->defaultCameraZ;
        Vec2 finalDraggingObjectDim = standardDraggingObjectDim * zoomCoeff;
        
        Vec3 objectP = UI->worldMouseP;
        
        RenderEntity(UI->group, UI->worldMode, &UI->draggingEntity, objectP, finalDraggingObjectDim, UI->worldMode->cameraWorldOffset.z - 0.1f);
    }
}

#define ICON_DIM 0.4f;
inline void UIOverdrawSkillSlots(UIState* UI, r32 modulationAlpha, PlatformInput* input)
{
    
    RenderGroup* group = UI->group;
    
    r32 additionalZBias = 12.1f;
    
    BitmapId sphereID = GetFirstBitmap(group->assets, Asset_UISphere);
    BitmapId sphereBoundsID = GetFirstBitmap(group->assets, Asset_UISphereBounds);
    
    r32 iconDim = ICON_DIM;
    r32 sphereDim = iconDim * 2.0f;
    r32 slotDim = sphereDim * 1.0f;
    
    i32 slotCount = MAXIMUM_SKILL_SLOTS;
    Assert((slotCount % 2) == 0);
    Vec2 slotP = V2(-(slotCount / 2 * slotDim) + 0.5f * slotDim, -5.0f);
    
    for(i32 slotIndex = 0; slotIndex < slotCount; ++slotIndex)
    {
        UISkill* skill = UI->skills + slotIndex;
        u32 skillTaxonomy = skill->taxonomy;
        TaxonomySlot* slot = GetSlotForTaxonomy(UI->table, skillTaxonomy);
        
        b32 drawBounds = false;
        Vec4 color = V4(0, 0, 0, 0.2f);
        if(slotIndex == UI->activeSkillSlotIndex)
        {
            drawBounds = true;
            color.a = 0.5f;
        }
        
        if(slot->isPassiveSkill)
        {
            drawBounds = true;
            if(skill->active)
            {
                color.a = 0.5f;
            }
        }
        
        color.a *= modulationAlpha;
        PushUIBitmap(group, sphereID, slotP, sphereDim, 0, additionalZBias, V2(1.0f, 1.0f), color);
        
        
        if(skillTaxonomy)
        {
            BitmapId iconID = GetRecursiveIconId(UI->table, group->assets, skillTaxonomy);
            PushUIBitmap(group, iconID, slotP, iconDim, 0, additionalZBias, V2(1.0f, 1.0f), V4(1, 1, 1, color.a));
        }
        
        if(drawBounds)
        {
            Vec4 boundsColor = V4(1, 1, 1, modulationAlpha);
            if(slot->isPassiveSkill)
            {
                boundsColor = V4(1, 0, 0, modulationAlpha);
                if(!skill->active)
                {
                    boundsColor = V4(0.2f, 0, 0, modulationAlpha);
                }
            }
            
            
            PushUIBitmap(group, sphereBoundsID, slotP, sphereDim, 0, additionalZBias, V2(1.0f, 1.0f), boundsColor);
        }
        
        
        Vec3 skillP = GetWorldP(group, slotP);
        Rect2 worldRect = RectCenterDim(V2(0, 0), V2(iconDim, iconDim));
        Rect2 skillRect = ProjectOnScreenCameraAligned(group, skillP, worldRect);
        if(skillTaxonomy && PointInRect(skillRect, UI->screenMouseP))
        {
            PushUITooltip(UI, slot->name, V4(1, 1, 1, 1));
            
            if(Pressed(&input->mouseLeft))
            {
                if(slot->isPassiveSkill)
                {
                    b32 active = skill->active;
                    b32 activate = !active;
                    
                    SendPassiveSkillRequest(skill->taxonomy, activate);
                    skill->active = !skill->active;
                }
                else
                {
                    Assert(skill->active);
                    SendActiveSkillRequest(skill->taxonomy);
                    UI->activeSkillSlotIndex = slotIndex;
                }
                
                platformAPI.DEBUGWriteFile("skills", UI->skills, sizeof(UI->skills));
            }
        }
        
        slotP.x += slotDim;
    }
}

inline void RenderEssence(UIState* UI, RenderGroup* group, TaxonomySlot* slot, Vec2 P, Vec2 quantityOffset, r32 iconDim, r32 additionalZBias)
{
    BitmapId sphereID = GetFirstBitmap(group->assets, Asset_UISphere);
    BitmapId sphereBoundsID = GetFirstBitmap(group->assets, Asset_UISphereBounds);
    BitmapId iconID = GetRecursiveIconId(UI->table, group->assets, slot->taxonomy);
    
    
    r32 sphereDim = iconDim * 2.0f;
    PushUIBitmap(group, sphereID, P, sphereDim, 0, additionalZBias, V2(1.0f, 1.0f), V4(0, 0, 0, 0.7f));
    if(UIElementActive(UI, group, P, V2(iconDim, iconDim)))
    {
        PushUIBitmap(group, sphereBoundsID, P, sphereDim, 0, additionalZBias, V2(1.0f, 1.0f), V4(1, 1, 1, 1));
        PushUITooltip(UI, slot->name, V4(1, 1, 1, 1));
    }
    
    PushUIBitmap(group, iconID, P, iconDim, 0, additionalZBias, V2(1.0f, 1.0f), V4(1, 1, 1, 1));
    
    r32 quantityDim = 0.18f;
    u32 quantity = GetEssenceQuantity(UI, slot->taxonomy);
    char quantityStr[4];
    FormatString(quantityStr, sizeof(quantityStr), "%d", quantity);
    
    Vec2 textP = P + quantityOffset;
    PushUITextWithDimension(UI, quantityStr, textP, V2(quantityDim, quantityDim), V4(1, 1, 1, 1));
}

inline void UIOverdrawEssences(UIState* UI)
{
    RenderGroup* group = UI->group;
    TaxonomySlot* essencesSlot = NORUNTIMEGetTaxonomySlotByName(UI->table, "essences");
    
    r32 additionalZBias = 12.2f;
    
    r32 iconDim = ICON_DIM;
    Vec2 iconPLeft = V2(-9.2f, 0.0f);
    Vec2 iconPRight = V2(9.2f, 0.0f);
    r32 quantityOffset = 1.0f * iconDim + 0.1f;
    
    u32 half = essencesSlot->subTaxonomiesCount / 2;
    
    r32 slotDim = 2.0f * iconDim;
    r32 halfOffset = (half / 2) * slotDim;
    iconPLeft.y -= halfOffset;
    iconPRight.y -= halfOffset;
    
    
    for(u32 essenceIndex = 0; essenceIndex < half; ++essenceIndex)
    {
        TaxonomySlot* slot = GetNthChildSlot(UI->table, essencesSlot, essenceIndex);
        RenderEssence(UI, group, slot, iconPLeft, V2(quantityOffset, -0.0f), iconDim, additionalZBias);
        iconPLeft.y += slotDim;
    }
    
    for(u32 essenceIndex = half; essenceIndex < essencesSlot->subTaxonomiesCount; ++essenceIndex)
    {
        TaxonomySlot* slot = GetNthChildSlot(UI->table, essencesSlot, essenceIndex);
        RenderEssence(UI, group, slot, iconPRight, V2(-quantityOffset, -0.0f), iconDim, additionalZBias);
        iconPRight.y += slotDim;
    }
}

inline EquipInfo PossibleToEquip(TaxonomyTable* table, ClientEntity* entity, Object* object)
{
    u32 objectTaxonomy = GetObjectTaxonomy(table, object);
    EquipInfo result = PossibleToEquip_(table, entity->taxonomy, entity->equipment, objectTaxonomy, object->status);
    
    return result;
}

internal void UIHandleContainer(UIState* UI, ClientEntity* container, PlatformInput* input, b32 isLootContainer)
{
    TaxonomyTable* table = UI->table;
    if(container->objects.maxObjectCount)
    {
        i32 objIndex = container->animation.output.focusObjectIndex;
        Object* focusObject = GetFocusObject(container);
        if(focusObject)
        {
            u8 objectIndex = SafeTruncateToU8(objIndex);
            if(focusObject->taxonomy)
            {
                if(!UI->draggingEntity.taxonomy)
                {
                    UIMarkListToUpdateAndRender(UI, possibleObjectActions);
                    
                    char objectName[64];
                    GetUIName(objectName, sizeof(objectName), table, focusObject);
                    u32 taxonomy = GetObjectTaxonomy(table, focusObject);
                    
                    EquipInfo equipInfo = PossibleToEquip(table, UI->player, focusObject);
                    PickInfo pickInfo = PossibleToPick(UI->worldMode, table, UI->player, taxonomy, false);
                    EntityAction consumeAction = CanConsume(table, UI->player->taxonomy, taxonomy);
                    
                    if(consumeAction)
                    {
                        UIAddPossibility(&UI->possibleObjectActions, MetaTable_EntityAction[consumeAction], objectName, UIStandardInventoryRequest(Consume, container->identifier, objectIndex));
                    }
                    if(!isLootContainer && focusObject->status < 0)
                    {
                        UIAddPossibility(&UI->possibleObjectActions, "Craft", objectName, UIStandardInventoryRequest(Craft, container->identifier, objectIndex));
                    }
                    if(IsValid(equipInfo))
                    {
                        UIAddPossibility(&UI->possibleObjectActions, "equip", objectName, UIStandardInventoryRequest(Equip, container->identifier, objectIndex));
                    }
                    
                    if(taxonomy == table->recipeTaxonomy)
                    {
                        UIRequest learnRequest;
                        learnRequest.requestCode = UIRequest_Learn;
                        learnRequest.containerEntityID = container->identifier;
                        learnRequest.objectIndex = objectIndex;
                        
                        UIAddPossibility(&UI->possibleObjectActions, "learn", objectName, learnRequest);
                    }
                    
                    if(isLootContainer)
                    {
                        UIRequest pickRequest;
                        pickRequest.requestCode = UIRequest_Pick;
                        pickRequest.sourceContainerID = container->identifier;
                        pickRequest.sourceObjectIndex = objectIndex;
                        pickRequest.destContainerID = pickInfo.reference.containerID;
                        pickRequest.destObjectIndex = pickInfo.reference.objectIndex;
                        
                        UIAddPossibility(&UI->possibleObjectActions, "pick", objectName, pickRequest);
                    }
                    else
                    {
                        UIAddPossibility(&UI->possibleObjectActions, "drop", objectName, UIStandardInventoryRequest(Drop, container->identifier, objectIndex));
                    }
                    
                    
                    UIInteraction mouseInteraction = ScrollableListRequestInteraction(UI, UI_Click, &UI->possibleObjectActions);
                    
                    UIRequest request = UIStandardInventoryRequest(Swap, container->identifier, objectIndex);
                    UIAddRequestAction(UI, &mouseInteraction, UI_KeptPressed, request);
                    UIAddObjectToEntityAction(UI, &mouseInteraction, UI_KeptPressed, ColdPointer(&UI->draggingEntity), focusObject);
                    UIAddInteraction(UI, input, mouseLeft, mouseInteraction);
                }
                else if(UI->draggingEntity.taxonomy && 
                        UI->draggingEntity.objects.objectCount == 0)
                {
                    UI->toUpdateList = 0;
                    UI->toRenderList = 0;
                    
                    UIInteraction swapInteraction = {};
                    UIRequest request = UIStandardInventoryRequest(Swap, container->identifier, objectIndex);
                    
                    UIAddRequestAction(UI, &swapInteraction, UI_Trigger, request);
                    UIAddObjectToEntityAction(UI, &swapInteraction, UI_Trigger, ColdPointer(&UI->draggingEntity), focusObject);
                    
                    UIAddInteraction(UI, input, mouseLeft, swapInteraction);
                }
            }
            else 
            {
                UIResetListIndex(UI, possibleObjectActions);
                if(UI->draggingEntity.taxonomy && UI->draggingEntity.objects.objectCount == 0)
                {              
                    UIInteraction swapInteraction = {};
                    UIRequest request = UIStandardInventoryRequest(Swap, container->identifier, objectIndex);
                    UIAddRequestAction(UI, &swapInteraction, UI_Trigger, request);
                    UIAddClearAction(UI, &swapInteraction, UI_Trigger, ColdPointer(&UI->draggingEntity), sizeof(UI->draggingEntity));
                    
                    UIAddInteraction(UI, input, mouseLeft, swapInteraction);
                }
            }
        }
        else
        {
            UIResetListIndex(UI, possibleObjectActions);
        }
    }
}

internal EditorElement* BuildEditorTaxonomyTree(u32 editorRoles, TaxonomyTable* table, TaxonomySlot* slot)
{
    EditorElement* result;
    FREELIST_ALLOC(result, table->firstFreeElement, PushStruct(&table->pool, EditorElement));
    *result = {};
    result->type = EditorElement_Taxonomy;
    result->taxonomy = slot->taxonomy;
    FormatString(result->name, sizeof(result->name), "%s", slot->name);
    
    if(slot->taxonomy)
    {
        u32 parentTaxonomy = GetParentTaxonomy(table, slot->taxonomy);
        if(parentTaxonomy)
        {
            AddFlags(result, EditorElem_Editable);
        }
        
        if((editorRoles & EditorRole_GameDesigner) && slot->name[0] != '#')
        {
            EditorElement* empty;
            FREELIST_ALLOC(empty, table->firstFreeElement, PushStruct(&table->pool, EditorElement));
            *empty = {};
            empty->type = EditorElement_EmptyTaxonomy;
            empty->parentTaxonomy = slot->taxonomy;
            result->firstChild = empty;
        }
    }
    
    for(u32 childIndex = 0; childIndex < slot->subTaxonomiesCount; ++childIndex)
    {
        TaxonomySlot* childSlot = GetNthChildSlot(table, slot, childIndex);
        EditorElement* child = BuildEditorTaxonomyTree(editorRoles, table, childSlot);
        child->next = result->firstChild;
        result->firstChild = child;
    }
    
    
    return result;
}

inline EditorElement* UIAddChild(TaxonomyTable* table, EditorElement* element, EditorElementType type, char* name, char* value = 0)
{
    EditorElement* newElement;
    FREELIST_ALLOC(newElement, table->firstFreeElement, PushStruct(&table->pool, EditorElement));
    
    
    newElement->type = type;
    FormatString(newElement->name, sizeof(newElement->name), "%s", name);
    
    if(value)
    {
        FormatString(newElement->value, sizeof(newElement->value), "%s", value);
    }
    
    newElement->next = element->firstChild;
    element->firstChild = newElement;
    
    return newElement;
}

inline EditorWidget* StartWidget(UIState* UI, EditorWidgetType widget, Vec2 P, u32 necessaryRole, char* name, char* fileName = 0)
{
    EditorWidget* result = UI->widgets + widget;
    *result = {};
    result->permanent.expanded = true;
    result->permanent.fontSize = 1.0f;
    result->dataOffsetY = 0;
    result->permanent.P = P;
    result->permanent.resizeP = P;
    result->necessaryRole = necessaryRole;
    result->layout.fontScale = 0.42f;
    result->layout.nameValueDistance = 24;
    result->layout.childStandardHeight = 30;
    result->layout.squareDim = 12;
    result->layout.lineThickness = 1.1f;
    result->layout.lineSegmentLength = 6.0f;
    result->layout.lineSpacing = 2.0f;
    result->layout.padding = 4.0f;
    FormatString(result->name, sizeof(result->name), name);
    if(fileName)
    {
           FormatString(result->fileName, sizeof(result->fileName), fileName);
    }
    
    return result;
}

inline void ResetUI(UIState* UI, GameModeWorld* worldMode, RenderGroup* group, ClientEntity* player, PlatformInput* input, r32 fontCoeff, b32 loadTaxonomyAutocompletes, b32 loadAssetAutocompletes)
{
    UI->table = worldMode->table;
    if(!UI->initialized)
    {
        UI->initialized = true;
        
        if(worldMode->editingEnabled)
        {
            UI->uneditableTabRoot.type = EditorElement_String;
            FormatString(UI->uneditableTabRoot.name, sizeof(UI->uneditableTabRoot.name), "YOU CAN'T EDIT THIS");
            
            
            EditorWidget* taxonomyTree = StartWidget(UI, EditorWidget_TaxonomyTree, V2(-800, -100), 0xffffffff, "Taxonomy Tree");
            
            
#if 0            
			AddChild(recipeParameters);
			AddChild("taxonomy");
			AddChild("recipeIndex");
#endif
            
            
            EditorWidget* taxonomyEditing = StartWidget(UI, EditorWidget_EditingTaxonomyTabs, V2(100, -100),  0xffffffff, "Editing Tabs");
            
            
            EditorElement* animationStruct;
            FREELIST_ALLOC(animationStruct, UI->table->firstFreeElement, PushStruct(&UI->table->pool, EditorElement));
            animationStruct->type = EditorElement_Struct;
            
            
            
            EditorElement* animationRoot;
            FREELIST_ALLOC(animationRoot, UI->table->firstFreeElement, PushStruct(&UI->fixedWidgetsElementPool, EditorElement));
            animationRoot->type = EditorElement_Animation;
            
            
            EditorElement* animationActionTimer;
            FREELIST_ALLOC(animationActionTimer, UI->table->firstFreeElement, PushStruct(&UI->fixedWidgetsElementPool, EditorElement));
            animationActionTimer->type = EditorElement_Struct;
            UIAddChild(UI->table, animationActionTimer, EditorElement_Real, "time", "0.0");
            UIAddChild(UI->table, animationActionTimer, EditorElement_String, "animationName", "rig");
            
            EditorElement* playButton;
            FREELIST_ALLOC(playButton, UI->table->firstFreeElement, PushStruct(&UI->fixedWidgetsElementPool, EditorElement));
            playButton->type = EditorElement_Struct;
            UIAddChild(UI->table, playButton, EditorElement_String, "autoplay", "false");
            UIAddChild(UI->table, playButton, EditorElement_Real, "speed", "1.0");
            UIAddChild(UI->table, playButton, EditorElement_Real, "scale", "50.0");
            UIAddChild(UI->table, playButton, EditorElement_String, "showBones", "false");
            UIAddChild(UI->table, playButton, EditorElement_String, "showBitmaps", "true");
            UIAddChild(UI->table, playButton, EditorElement_String, "showPivots", "false");
            UIAddChild(UI->table, playButton, EditorElement_String, "drawOpened", "false");
            UIAddChild(UI->table, playButton, EditorElement_Unsigned, "seed", "1");
            
            
            playButton->next = animationActionTimer;
            animationRoot->next = playButton;
            animationStruct->firstValue = animationRoot;
            
            
            EditorWidget* animation = StartWidget(UI, EditorWidget_Animation, V2(-300, -300), 0xffffffff, "Animation");
            animation->root = animationRoot;

            
            EditorElement* colorPickerRoot;
            FREELIST_ALLOC(colorPickerRoot, UI->table->firstFreeElement, PushStruct(&UI->fixedWidgetsElementPool, EditorElement));
            colorPickerRoot->type = EditorElement_ColorPicker;
            
            
            EditorElement* color;
            FREELIST_ALLOC(color, UI->table->firstFreeElement, PushStruct(&UI->fixedWidgetsElementPool, EditorElement));
            color->type = EditorElement_Struct;
            FormatString(color->name, sizeof(color->name), "color");
            
            UIAddChild(UI->table, color, EditorElement_Real, "a", "1.0");
            UIAddChild(UI->table, color, EditorElement_Real, "b", "0.0");
            UIAddChild(UI->table, color, EditorElement_Real, "g", "0.0");
            UIAddChild(UI->table, color, EditorElement_Real, "r", "0.0");
            
            
            colorPickerRoot->next = color;
            
            EditorWidget* colorPicker = StartWidget(UI, EditorWidget_ColorPicker, V2(-300, -300), 0xffffffff, "Color Picker");
            colorPicker->root = colorPickerRoot;
            
            
            
            EditorElement* groundParamsRoot;
            FREELIST_ALLOC(groundParamsRoot, UI->table->firstFreeElement, PushStruct(&UI->fixedWidgetsElementPool, EditorElement));
            groundParamsRoot->type = EditorElement_GroundParams;
            
            
            EditorElement* params;
            FREELIST_ALLOC(params, UI->table->firstFreeElement, PushStruct(&UI->fixedWidgetsElementPool, EditorElement));
            params->type = EditorElement_Struct;
            FormatString(params->name, sizeof(params->name), "params");
            
            UIAddChild(UI->table, params, EditorElement_String, "showBorders", "false");
            UIAddChild(UI->table, params, EditorElement_String, "uniformColors", "false");
            UIAddChild(UI->table, params, EditorElement_String, "viewType", "Voronoi");
            UIAddChild(UI->table, params, EditorElement_Unsigned, "chunkApron", "2");
            EditorElement* offset = UIAddChild(UI->table, params, EditorElement_Struct, "cameraOffset");
            UIAddChild(UI->table, offset, EditorElement_Real, "x", "0.0");
            UIAddChild(UI->table, offset, EditorElement_Real, "y", "0.0");
            UIAddChild(UI->table, offset, EditorElement_Real, "z", "0.0");
            
            
            groundParamsRoot->next = params;
            
            EditorWidget* groundParams = StartWidget(UI, EditorWidget_GroundParams, V2(-400, -300),
                                                     0xffffffff, "Ground Params");
            groundParams->root = groundParamsRoot;
            
            EditorWidget* soundDatabase = StartWidget(UI, EditorWidget_SoundDatabase, V2(300,200), EditorRole_SoundDesigner, "Sound Database");
            soundDatabase->root = UI->table->soundNamesRoot;
            
            EditorWidget* componentDatabase = StartWidget(UI, EditorWidget_Components, V2(300, 200), EditorRole_GameDesigner, "Visual Components", "components");
            componentDatabase->root = UI->table->componentsRoot;
            
            
            EditorWidget* actions = StartWidget(UI, EditorWidget_GeneralButtons, V2(200, 100), 0xffffffff, "Actions:");
            
            
            EditorWidget* misc = StartWidget(UI, EditorWidget_Misc, V2(200, 100), 0xffffffff, "Miscellaneous");
            FREELIST_ALLOC(misc->root, UI->table->firstFreeElement, PushStruct(&UI->fixedWidgetsElementPool, EditorElement));
            misc->root->type = EditorElement_Struct;
            
            UIAddChild(UI->table, misc->root, EditorElement_String, "recipeTaxonomy", "objects");
            UIAddChild(UI->table, misc->root, EditorElement_Unsigned, "recipeIndex", "0");
            UIAddChild(UI->table, misc->root, EditorElement_Unsigned, "worldSeed", "0");
            UIAddChild(UI->table, misc->root, EditorElement_String, "dayPhase", "Day");
            UIAddChild(UI->table, misc->root, EditorElement_Real, "windSpeed", "1.0");
            
            
            
            EditorElement* ambientColor;
            FREELIST_ALLOC(ambientColor, UI->table->firstFreeElement, PushStruct(&UI->fixedWidgetsElementPool, EditorElement));
            ambientColor->type = EditorElement_Struct;
            FormatString(ambientColor->name, sizeof(ambientColor->name), "ambientColor");
            
            UIAddChild(UI->table, ambientColor, EditorElement_Real, "a", "1.0");
            UIAddChild(UI->table, ambientColor, EditorElement_Real, "b", "0.0");
            UIAddChild(UI->table, ambientColor, EditorElement_Real, "g", "0.0");
            UIAddChild(UI->table, ambientColor, EditorElement_Real, "r", "0.0");
            
            
            misc->root->next = ambientColor;
            
            
            
            EditorWidget* soundEvents = StartWidget(UI, EditorWidget_SoundEvents, V2(-200, 100), EditorRole_SoundDesigner, "Sound Events", "soundEvents");
            soundEvents->root = UI->table->soundEventsRoot;
            
            
            PlatformFile layoutFile = platformAPI.DEBUGReadFile("widget");
            if(layoutFile.content)
            {
                if(layoutFile.size == sizeof(WidgetPermanent) * EditorWidget_Count)
                {
                    WidgetPermanent* layouts = (WidgetPermanent*) layoutFile.content;
                    
                    for(u32 widgetIndex = 0; widgetIndex < EditorWidget_Count; ++widgetIndex)
                    {
                        EditorWidget* widget = UI->widgets + widgetIndex;
                        widget->permanent = layouts[widgetIndex];
                    }
                }
            }
            
            platformAPI.DEBUGFreeFile(&layoutFile);
            
            
            DLLIST_INIT(&UI->undoRedoSentinel);
            UI->current = &UI->undoRedoSentinel;
        }
        
        
        
        
        UI->chunkApron = 2;
        UI->activeSkillSlotIndex = -1;
        UI->skillSlotMaxTimeout = 10.0f;
        
        UIBookmarkCondition condition = BookModeCondition(UIBook_Recipes);
        UIBookmark* startingBookmark = UIAddBookmark(UI, UIBookmark_RightSide, condition);
        
        UIBookmarkCondition condition2 = BookModeCondition(UIBook_Skills);
        UIBookmark* skills = UIAddBookmark(UI, UIBookmark_RightSide, condition2);
        
        UIDispatchBookmarkCondition(UI, skills);
        
        UI->movementGroup.buttonCount = 0;
        AddButton(&UI->movementGroup, moveLeft);
        AddButton(&UI->movementGroup, moveRight);
        AddButton(&UI->movementGroup, moveUp);
        AddButton(&UI->movementGroup, moveDown);
        
        UI->group = group;
        UI->worldMode = worldMode;
        UI->myPlayer = &worldMode->player;
        
        TagVector matchVector = {};
        TagVector weightVector = {};
        weightVector.E[Tag_fontType] = 1.0f;
        matchVector.E[Tag_fontType] = ( r32 ) Font_default;
        UI->fontId = GetMatchingFont(group->assets, Asset_font, &matchVector, &weightVector);
        
        UI->scrollIconID = GetMatchingBitmap(group->assets, Asset_scrollUI, &matchVector, &weightVector);
        
        
        PlatformFile skillsFile = platformAPI.DEBUGReadFile("skills");
        if(skillsFile.content)
        {
            Copy(sizeof(UI->skills), UI->skills, skillsFile.content);
            platformAPI.DEBUGFreeFile(&skillsFile);
            
            for(u32 skillIndex = 0; skillIndex < MAXIMUM_SKILL_SLOTS; ++skillIndex)
            {
                UISkill* skill = UI->skills + skillIndex;
                if(skill->taxonomy)
                {
                    TaxonomySlot* slot = GetSlotForTaxonomy(UI->table, skill->taxonomy);
                    if(slot->isPassiveSkill && skill->active)
                    {
                        SendPassiveSkillRequest(skill->taxonomy, true);
                    }
                }
            }
        }
        
        
        
        UIAddAutocompleteFromTable(UI, SlotName, "slot");
        UIAddAutocompleteFromTable(UI, EntityAction, "action");
        UIAddAutocompleteFromTable(UI, SoundContainerType, "soundCType");
        UIAddAutocompleteFromTable(UI, ObjectState, "objectState");
        UIAddAutocompleteFromTable(UI, GroundViewMode, "viewType");
        UIAddAutocompleteFromTable(UI, TilePointsLayout, "tileLayout");
        UIAddAutocompleteFromTable(UI, PlantShape, "shape");
        UIAddAutocompleteFromTable(UI, ForgDayPhase, "dayPhase");
        
        UIAddAutocomplete(UI, "layoutName");
		UIAddAutocomplete(UI, "pieceName");
        
        UIAutocomplete* autocomplete = UIAddAutocomplete(UI, "soundParamName");
        UIAddOption(UI, autocomplete, "decibelOffset");
        UIAddOption(UI, autocomplete, "pitch");
        UIAddOption(UI, autocomplete, "delay");
        UIAddOption(UI, autocomplete, "toleranceDistance");
        UIAddOption(UI, autocomplete, "distanceFalloffCoeff");
        
        
        FormatString(UI->trueString, sizeof(UI->trueString), "true");
        FormatString(UI->falseString, sizeof(UI->falseString), "false");
    }
    
    
    if(loadTaxonomyAutocompletes)
    {
        UIAddAutocompleteFromTaxonomy(UI, "equipment");
        UIAddAutocompleteFromTaxonomy(UI, "root", "ingredient");
        UIAddAutocompleteFromTaxonomy(UI, "objects", "recipeTaxonomy");
        UIAddAutocompleteFromTaxonomy(UI, "tiles", "tileType");
        UIAddAutocompleteFromTaxonomy(UI, "root", "taxonomyName");
    }
    
    if(loadAssetAutocompletes)
    {
        UIAddAutocompleteFromFiles(UI);
    }
    
    UI->player = player;
    
    
    UI->runningCursorTimer += UI->worldMode->originalTimeToAdvance;
    r32 cursorInterval = 0.6f;
    
    u32 triggered = TruncateReal32ToU32(UI->runningCursorTimer / cursorInterval);
    UI->showCursor = ((triggered % 2) == 0);
    
    
    if(!UI->font)
    {
        UI->font = PushFont(group, UI->fontId);
        if(UI->font)
        {
            UI->fontInfo = GetFontInfo(group->assets, UI->fontId);
            
            for(u8 codepoint = 65; codepoint <= 122; ++codepoint)
            {
                BitmapId ID = GetBitmapForGlyph(group->assets, UI->font, UI->fontInfo, codepoint);
                PrefetchBitmap(group->assets, ID);
            }
        }
    }
    
    
    UI->prefix = false;
    UI->suffix = false;
    UI->tooltipText[0] = 0;
    
    UI->fontScale = 0.0042f * fontCoeff;
    Assert(UI->fontScale > 0);
    Assert(fontCoeff < 100000);
    
    UI->additionalCameraOffset = {};
    UI->zoomLevel = 1.0f;
    
    if(UI->myPlayer->openedContainerID != UI->openedContainerID)
    {
        UI->openedContainerID = UI->myPlayer->openedContainerID;
        if(UI->openedContainerID)
        {
            UI->nextMode = UIMode_Loot;
        }
        else
        {
            if(UI->mode == UIMode_Loot)
            {
                UI->nextMode = UIMode_None;
            }
        }
    }
    
    if(UI->mode == UIMode_None)
    {
        if(UI->draggingEntity.taxonomy)
        {
            SendDropRequest(0, 0);
            UI->draggingEntity = {};
        }
    }
    
    if(UI->mode == UIMode_Book)
    {
        for(u32 slotIndex = 0; slotIndex < ArrayCount(UI->ingredientHashMap); ++slotIndex)
        {
            UIOwnedIngredient* ingredient = UI->ingredientHashMap + slotIndex;
            ingredient->quantity = 0;
            if(!ingredient->needToBeDrawn)
            {
                ingredient->taxonomy = 0;
            }
            ingredient->needToBeDrawn = false;
        }
        
        for(u32 slotIndex = 0; slotIndex < Slot_Count; ++slotIndex)
        {
            u64 ID = player->equipment[slotIndex].ID;
            if(ID)
            {
                ClientEntity* equipment = GetEntityClient(UI->worldMode, ID);
                if(equipment)
                {
                    for(u32 objectIndex = 0; objectIndex < equipment->objects.maxObjectCount; ++objectIndex)
                    {
                        Object* object = equipment->objects.objects + objectIndex;
                        if(object->taxonomy && !IsRecipe(object))
                        {
                            AddToAvailableIngredients(UI, object);
                        }
                    }
                }
            }
        }
    }
    
    if(UI->nextMode != UI->mode)
    {
        UI->previousMode = UI->mode;
        UI->mode = UI->nextMode;
        
        for(u32 buttonIndex = 0; buttonIndex < MAX_BUTTON_COUNT; ++buttonIndex)
        {
            UI->hotInteractions[buttonIndex].priority = UIPriority_NotValid;
            UIInteraction* interaction = UI->activeInteractions + buttonIndex;
            if(!(interaction->flags & UI_MaintainWhenModeChanges))
            {
                interaction->priority = UIPriority_NotValid;
            }
        }
    }
    
    UI->toUpdateList = 0;
    UI->toRenderList = 0;
}

inline void HandleOverlappingInteraction(UIState* UI, UIOutput* output, PlatformInput* input, ClientEntity* overlapping)
{
    if(UI->mode == UIMode_Equipment && overlapping->identifier == UI->myPlayer->openedContainerID)
    {
        UIAddInteraction(UI, input, mouseLeft, UISetValueInteraction(UI, UI_Trigger, &UI->nextMode, UIMode_Loot));
    }
    else
    {
        char entityName[64];
        GetUIName(entityName, sizeof(entityName), UI->table, overlapping);
        
        UIMarkListToUpdateAndRender(UI, possibleOverlappingActions);
        if(UI->worldMode->editingEnabled && input->altDown)
        {
            UIRequest editRequest = EditRequest(overlapping->taxonomy);
            UIAddPossibility(&UI->possibleOverlappingActions, "edit", entityName, editRequest);
            
            UIRequest deleteRequest = DeleteRequest(overlapping->identifier);
            UIAddPossibility(&UI->possibleOverlappingActions, "delete", entityName, deleteRequest);
            
            if(!IsPlant(UI->table, overlapping->taxonomy))
            {                
                UIRequest impersonateRequest = ImpersonateRequest(overlapping->identifier);
                UIAddPossibility(&UI->possibleOverlappingActions, "impersonate", entityName, impersonateRequest);
                
            }
            
            UIInteraction mouseInteraction = ScrollableListRequestInteraction(UI, UI_Click, &UI->possibleOverlappingActions);
            UIAddInteraction(UI, input, mouseLeft, mouseInteraction);
        }
        else
        {
            for(u32 actionIndex = Action_Attack; actionIndex < Action_Count; ++actionIndex)
            {
                if(UI->myPlayer->overlappingPossibleActions[actionIndex])
                {
                    UIRequest actionRequest = StandardActionRequest(actionIndex, overlapping->identifier);
                    UIAddPossibility(&UI->possibleOverlappingActions, MetaTable_EntityAction[actionIndex], entityName, actionRequest);
                }
            }
            
            UIInteraction actionListInteraction = {};
            actionListInteraction.flags |= UI_MaintainWhenModeChanges;
            UIAddScrollableTargetInteraction(UI, &actionListInteraction, &UI->possibleOverlappingActions, output);
            UIAddInvalidCondition(&actionListInteraction, u32,ColdPointerDataOffset(UI->myPlayer->targetPossibleActions, OffsetOf(UIInteractionData, actionIndex)), Fixed((b32)false), 0);                      
            UIAddInvalidCondition(&actionListInteraction, u32,ColdPointer(&output->desiredAction), Fixed((u32)Action_Attack), UI_Ended);
            UIAddInvalidCondition(&actionListInteraction, b32,ColdPointer(&UI->movingWithKeyboard), Fixed((b32)true), 0);
            
            UIAddInteraction(UI, input, mouseLeft, actionListInteraction);
            
            if(UI->myPlayer->overlappingPossibleActions[Action_Cast])
            {
                UIInteraction castInteraction = {};
                UIAddStandardTargetInteraction(UI, &castInteraction, output, Action_Cast, overlapping->identifier);
                UIAddInvalidCondition(&castInteraction, u32,ColdPointer(UI->myPlayer->targetPossibleActions + Action_Cast), Fixed(false));
                UIAddInteraction(UI, input, mouseCenter, castInteraction);
            }
        }
        
    }
    
    if(UI->possibleOverlappingActions.possibilityCount == 1)
    {
        UIMarkListToUpdate(UI, possibleTargets);
    }
}

internal UIOutput UIHandle(UIState* UI, PlatformInput* input, Vec2 screenMouseP, ClientEntity** overlappingEntities, u32 maxOverlappingEntities)
{    
    input->allowedToQuit = true;
    
    i32 scrollOffset = input->mouseWheelOffset;
    
    UIOutput output = {};
    
    TaxonomyTable* table = UI->table;
    ClientEntity* player = UI->player;
    GameModeWorld* worldMode = UI->worldMode;
    
    UI->worldMouseP = worldMode->worldMouseP;
    UI->oldScreenMouseP = UI->screenMouseP;
    if(UI->screenMouseP.x != screenMouseP.x || UI->screenMouseP.y != screenMouseP.y)
    {
        UI->animationGhostAllowed = true;
        UI->screenMouseP = screenMouseP;
    }
    
    UI->deltaScreenMouseP = UI->screenMouseP - UI->oldScreenMouseP;
    
    UI->relativeScreenMouse = V2(input->relativeMouseX, input->relativeMouseY);
    
    
    UI->movingWithKeyboard = false;
    if(IsDown(&input->moveLeft) ||
       IsDown(&input->moveRight) ||
       IsDown(&input->moveUp) ||
       IsDown(&input->moveDown))
    {
        UI->movingWithKeyboard = true;
    }
    UIAddInteraction(UI, input, moveLeft, UISetValueInteraction(UI, UI_Idle, &output.inputAcc.x, -1.0f), UIPriority_Standard, &UI->movementGroup);
    UIAddInteraction(UI, input, moveRight, UISetValueInteraction(UI, UI_Idle, &output.inputAcc.x, 1.0f), UIPriority_Standard, &UI->movementGroup);
    UIAddInteraction(UI, input, moveDown, UISetValueInteraction(UI, UI_Idle, &output.inputAcc.y, -1.0f), UIPriority_Standard, &UI->movementGroup);
    UIAddInteraction(UI, input, moveUp, UISetValueInteraction(UI, UI_Idle, &output.inputAcc.y, 1.0f), UIPriority_Standard, &UI->movementGroup);
    switch(UI->mode)
    {
        case UIMode_None:
        {
            UI->lockedInventoryID1 = 0;
            UI->lockedInventoryID2 = 0;
            UIResetListPossibility(UI, possibleTargets);
            
            for(u32 overlappingIndex = 0; overlappingIndex < maxOverlappingEntities; ++overlappingIndex)
            {
                if(overlappingEntities[overlappingIndex])
                {
                    UIAddPossibility(&UI->possibleTargets, overlappingEntities[overlappingIndex]);
                }
            }
            
            if(UI->possibleOverlappingActions.currentIndex == (UI->possibleOverlappingActions.possibilityCount - 1))
            {
                if(scrollOffset > 0)
                {
                    ++UI->possibleTargets.currentIndex;
                    WrapScrollableList(&UI->possibleTargets);
                    scrollOffset = 0;
                }
            }
            
            
            
            UIScrollableElement* activeElement = GetActiveElement(&UI->possibleTargets);
            if(activeElement)
            {
                ClientEntity* overlapping = activeElement->entity;
                if(overlapping)
                {
                    if(overlapping->identifier == player->identifier)
                    {
                        if(UI->worldMode->editingEnabled && input->altDown)
                        {
                            TaxonomySlot* slot = GetSlotForTaxonomy(UI->table, overlapping->taxonomy);
                            char tooltip[64];
                            FormatString(tooltip, sizeof(tooltip), "edit %s", slot->name);
                            PushUITooltip(UI, tooltip, V4(1, 0, 0, 1));
                            
                            UIInteraction mouseInteraction = SendRequestInteraction(UI, UI_Click, EditRequest(overlapping->taxonomy));
                            UIAddInteraction(UI, input, mouseLeft, mouseInteraction);
                        }
                        else
                        {
                            PushUITooltip(UI, "inventory", V4(1, 0, 0, 1));
                            
                            UIAddInteraction(UI, input, mouseLeft, UISetValueInteraction(UI, UI_Trigger, &UI->nextMode, UIMode_Equipment));
                            UIAddInteraction(UI, input, mouseRight, UISetValueInteraction(UI, UI_Trigger, &UI->nextMode, UIMode_Book));
                            UIMarkListToUpdate(UI, possibleTargets);
                        }
                    }
                    else
                    {
                        output.overlappingEntityID = overlapping->identifier;
                    }
                    
                    if(overlapping->identifier != UI->player->targetID || UI->player->action == Action_None)
                    {
                        overlapping->modulationWithFocusColor = worldMode->modulationWithFocusColor;
                    }
                    
                    UIResetListPossibility(UI, possibleOverlappingActions);
                    if(overlapping->identifier == UI->myPlayer->overlappingIdentifier || (UI->worldMode->editingEnabled && input->altDown))
                    {
                        HandleOverlappingInteraction(UI, &output, input, overlapping);
                    }
                    else
                    {
                        UIResetListIndex(UI, possibleOverlappingActions);
                    }
                }
            }
            else
            {
                if(!input->altDown)
                {
                                    UIResetListIndex(UI, possibleOverlappingActions);
                UIInteraction mouseMovement = {};
                UIAddStandardAction(UI, &mouseMovement, UI_Click, Vec3, ColdPointer(&UI->deltaMouseP), ColdPointer(&UI->worldMouseP));
                UIAddStandardAction(UI, &mouseMovement, UI_Idle, u32, ColdPointer(&UI->mouseMovement), Fixed(UIMouseMovement_MouseDir));
                UIAddInvalidCondition(&mouseMovement, b32, ColdPointer(&UI->movingWithKeyboard), Fixed(true), UI_Ended);
                
                if(false)
                {
                    UIAddStandardAction(UI, &mouseMovement, UI_Click, b32, ColdPointer(&UI->reachedPosition), Fixed(false));
                    UIAddStandardAction(UI, &mouseMovement, UI_Click | UI_Retroactive, u32, ColdPointer(&UI->mouseMovement), Fixed(UIMouseMovement_ToMouseP));
                    UIAddInvalidCondition(&mouseMovement, b32, ColdPointer(&UI->reachedPosition), Fixed(true), UI_Ended);
                }
                
                UIAddInteraction(UI, input, mouseLeft, mouseMovement);
                }

            }
            
#if 0            
            if(fighting)
            {
                AddInteraction(mouseRight, Action_Protect);
            }
#endif
            
            switch(UI->mouseMovement)
            {
                case UIMouseMovement_ToMouseP:
                {
                    if(LengthSq(UI->deltaMouseP) < Square(0.6f))
                    {
                        UI->reachedPosition = true;
                        output.inputAcc = V3(0, 0, 0);
                    }
                    else
                    {
                        output.inputAcc = UI->deltaMouseP;
                    }
                } break;
                
                case UIMouseMovement_MouseDir:
                {
                    Vec3 dir = UI->worldMouseP - player->P;
                    output.inputAcc = dir;
                    output.desiredAction = Action_Move;
                } break;
            }
            UI->mouseMovement = UIMouseMovement_None;
        } break;
        
        case UIMode_Loot:
        {
            UIResetListPossibility(UI, possibleObjectActions);
            UIAddInteraction(UI, input, mouseRight, UISetValueInteraction(UI, UI_Trigger, &UI->nextMode, UIMode_Equipment));
            
            ClientEntity* containerEntityC = GetEntityClient(worldMode, UI->openedContainerID);
            if(containerEntityC && GetFocusObject(containerEntityC))
            {
                UIHandleContainer(UI, containerEntityC, input, true);
            }
            else
            {
                UIAddInteraction(UI, input, mouseLeft, UISetValueInteraction(UI, UI_Trigger, &UI->nextMode, UIMode_None));
            }
            
            Rect2 playerScreenBounds = ProjectOnScreenCameraAligned(UI->group, player->P, UI->player->animation.bounds);
            Rect2 containerScreenBounds = ProjectOnScreenCameraAligned(UI->group, containerEntityC->P, containerEntityC->animation.bounds);
        } break;
        
        case UIMode_Equipment:
        {
            UI->zoomLevel = 4.2f;
            
            b32 specialEquipmentMode = false;
            b32 onlyOpenedContainerAllowed = false;
            
            UIAddInteraction(UI, input, mouseRight, UISetValueInteraction(UI, UI_Trigger, &UI->nextMode, UI->previousMode));
            
            UIResetListPossibility(UI, possibleObjectActions);
            UIMarkListToUpdateAndRender(UI, possibleObjectActions);
            
            EquipmentSlot* slots = player->equipment;       
            
            EquipInfo focusSlot = UI->player->animation.output.focusSlots;
            
            EquipInfo compatibleWithDragging = {};
            
            ClientEntity* dragging = &UI->draggingEntity;
            u64 draggingID = 0;
            
            if(dragging->taxonomy)
            {
                draggingID = dragging->identifier;
                i16 currentHotAssIndex = player->animation.output.nearestAss;
                if(currentHotAssIndex >= 0)
                {
                    if(dragging->status >= 0)
                    {
                        EquipmentMapping canEquip = InventorySlotPresent(UI->table, player->taxonomy, dragging->taxonomy);
                        b32 found = false;
                        for(EquipmentLayout* layout = canEquip.firstEquipmentLayout; layout && !found; layout = layout->next)
                        {
                            for(EquipmentAss* ass = layout->firstEquipmentAss; ass; ass = ass->next)
                            {
                                if((i16) ass->assIndex == currentHotAssIndex)
                                {
                                    compatibleWithDragging = layout->slot;
                                    found = true;
                                    break;
                                }
                            }
                        }   
                    }
                }
                
            }
            
            
            
            player->animation.nearestCompatibleSlotForDragging = compatibleWithDragging;
            
            
            u64 focusEquipmentID = player->equipment[GetMainSlot(focusSlot)].ID;
            
            UIAddInteraction(UI, input, mouseLeft, UISetValueInteraction(UI, UI_Trigger, &UI->nextMode, UIMode_None));
            if(focusEquipmentID)
            {
                ClientEntity* equipmentFocus = GetEntityClient(worldMode, focusEquipmentID);
                if(equipmentFocus)
                {
                    if(UI->draggingEntity.taxonomy &&
                       (focusEquipmentID != UI->lockedInventoryID1 && 
                        focusEquipmentID != UI->lockedInventoryID2))
                    {
                        r32 maxPixelAllowed = 5;
                        Vec2 deltaScreenP = UI->screenMouseP - UI->oldScreenMouseP;
                        if(LengthSq(deltaScreenP) <= Square(maxPixelAllowed))
                        {
                            UI->focusContainerTimer += input->timeToAdvance;
                        }
                        else
                        {
                            UI->focusContainerTimer = 0;
                        }
                        
                        if(UI->focusContainerTimer >= 0.5f)
                        {
                            UI->focusContainerTimer = 0;
                            if(!UI->lockedInventoryID1)
                            {
                                UI->lockedInventoryID1 = focusEquipmentID;
                            }
                            else
                            {
                                UI->lockedInventoryID2 = focusEquipmentID;
                            }
                        }
                    }
                    else
                    {
                        UI->focusContainerTimer = 0;
                    }
                    
                    char equipmentName[64];
                    GetUIName(equipmentName, sizeof(equipmentName), UI->table, equipmentFocus);
                    
                    if(!UI->draggingEntity.taxonomy)
                    {
                        if(focusEquipmentID == UI->lockedInventoryID1 ||
                           focusEquipmentID == UI->lockedInventoryID2)
                        {
                            UIRequest closeRequest;
                            closeRequest.requestCode = UIRequest_Close;
                            closeRequest.IDToClose = focusEquipmentID;
                            UIAddPossibility(&UI->possibleObjectActions, "Close", equipmentName, closeRequest);
                        }
                        else
                        {
                            if(equipmentFocus->objects.maxObjectCount > 0)
                            {
                                UIRequest openRequest;
                                openRequest.requestCode = UIRequest_Open;
                                openRequest.containerID = equipmentFocus->identifier;
                                UIAddPossibility(&UI->possibleObjectActions, "open", equipmentName, openRequest);
                            }
                            
                            PickInfo pick = PossibleToPick(UI->worldMode, UI->table, UI->player, equipmentFocus->taxonomy, true);
                            if(equipmentFocus->objects.objectCount == 0 && pick.slot)
                            {
                                UIRequest pickRequest = UIStandardSlotRequest(Disequip, GetMainSlot(focusSlot), pick.reference.containerID, pick.reference.objectIndex);
                                UIAddPossibility(&UI->possibleObjectActions, "disequip", equipmentName, pickRequest);
                            }
                            else
                            {
                                UIRequest dropRequest = UIStandardSlotRequest(Disequip, GetMainSlot(focusSlot), 0, 0);
                                UIAddPossibility(&UI->possibleObjectActions, "drop", equipmentName, dropRequest);
                            }
                        }
                        
                    }
                    
                    UIInteraction mouseInteraction = ScrollableListRequestInteraction(UI, UI_Click, &UI->possibleObjectActions);
                    UIAddInteraction(UI, input, mouseLeft, mouseInteraction);
                    
                    if(IsValid(focusSlot))
                    {
                        if(!UI->draggingEntity.taxonomy)
                        {
                            UIRequest dragEquipmentRequest = {};
                            dragEquipmentRequest.requestCode = UIRequest_DragEquipment;
                            dragEquipmentRequest.slot = GetMainSlot(focusSlot);
                            
                            UIInteraction dragEquipmentInteraction = mouseInteraction;
                            UIAddRequestAction(UI, &dragEquipmentInteraction, UI_KeptPressed, dragEquipmentRequest);
                            UIAddStandardAction(UI, &dragEquipmentInteraction, UI_KeptPressed, ClientEntity, ColdPointer(&UI->draggingEntity), ColdPointer(equipmentFocus));
                            
                            UIAddInteraction(UI, input, mouseLeft, dragEquipmentInteraction);
                        }
                        else
                        {
                            ObjectReference freeSlot = HasFreeSpace(equipmentFocus);
                            if(!UI->draggingEntity.objects.objectCount && freeSlot.objectIndex >= 0)
                            {
                                UIInteraction swapInteraction = {};
                                UIRequest request = UIStandardInventoryRequest(Swap, freeSlot.containerID, freeSlot.objectIndex);
                                UIAddRequestAction(UI, &swapInteraction, UI_Trigger, request);
                                UIAddClearAction(UI, &swapInteraction, UI_Trigger, ColdPointer(&UI->draggingEntity), sizeof(UI->draggingEntity));
                                
                                UIAddInteraction(UI, input, mouseLeft, swapInteraction);
                            }
                            else
                            {
                                UIAddInteraction(UI, input, mouseLeft, NullInteraction());
                            }
                        }
                    }
                }
            }
            else
            {
                //if(!UI->lockedInventoryID1 && !UI->lockedInventoryID2)
                {
                    specialEquipmentMode = true;
                }
                if(UI->draggingEntity.taxonomy)
                {
                    onlyOpenedContainerAllowed = true;
                    if(IsValid(compatibleWithDragging))
                    {
                        specialEquipmentMode = false;
                        UIRequest equipDraggingRequest;
                        equipDraggingRequest.requestCode = UIRequest_EquipDragging;
                        equipDraggingRequest.slot = GetMainSlot(compatibleWithDragging);
                        
                        UIInteraction equipDraggingInteraction = {};
                        UIAddRequestAction(UI, &equipDraggingInteraction, UI_Trigger, equipDraggingRequest);
                        UIAddClearAction(UI, &equipDraggingInteraction, UI_Trigger, ColdPointer(&UI->draggingEntity), sizeof(UI->draggingEntity));
                        
                        UIAddInteraction(UI, input, mouseLeft, equipDraggingInteraction);
                    }
                    else
                    {
                        UIRequest dropRequest = UIStandardInventoryRequest(Drop, 0, 0);
                        UIInteraction dropInteraction = {};
                        UIAddRequestAction(UI, &dropInteraction, UI_Trigger, dropRequest);
                        UIAddClearAction(UI, &dropInteraction, UI_Trigger, ColdPointer(&UI->draggingEntity), sizeof(UI->draggingEntity));
                        
                        UIAddInteraction(UI, input, mouseLeft, dropInteraction);
                    }
                }
            }
            
            if(UI->lockedInventoryID1 && input->normalizedMouseX >= 0.5f)
            {
                ClientEntity* container = GetEntityClient(worldMode, UI->lockedInventoryID1);
                if(container)
                {
                    UIHandleContainer(UI, container, input, false);
                }
                
                if(PointInRect(UI->screenBoundsInventory1, UI->screenMouseP))
                {
                    specialEquipmentMode = false;
                }
            }
            else if(UI->lockedInventoryID2)
            {
                ClientEntity* container = GetEntityClient(worldMode, UI->lockedInventoryID2);
                if(container)
                {
                    UIHandleContainer(UI, container, input, false);
                }
                
                if(PointInRect(UI->screenBoundsInventory2, UI->screenMouseP))
                {
                    specialEquipmentMode = false;
                }
            }
            
            if(specialEquipmentMode)
            {
                UIResetListPossibility(UI, possibleTargets);
                for(u32 overlappingIndex = 0; overlappingIndex < maxOverlappingEntities; ++overlappingIndex)
                {
                    ClientEntity* overlap = overlappingEntities[overlappingIndex];
                    if(overlap && 
                       overlap->identifier != UI->myPlayer->identifier &&
                       (!onlyOpenedContainerAllowed || overlap->identifier == UI->myPlayer->openedContainerID))
                    {
                        UIAddPossibility(&UI->possibleTargets, overlappingEntities[overlappingIndex]);
                    }
                }
                
                UIScrollableElement* activeElement = GetActiveElement(&UI->possibleTargets);
                if(activeElement)
                {
                    ClientEntity* overlapping = activeElement->entity;
                    if(overlapping)
                    {
                        UIAddInteraction(UI, input, switchButton, UISetValueInteraction(UI, UI_Trigger, &UI->possibleTargets.currentIndex, UI->possibleTargets.currentIndex + 1));
                        UIResetListPossibility(UI, possibleOverlappingActions);
                        if(overlapping->identifier != player->identifier)
                        {
                            output.overlappingEntityID = overlapping->identifier;
                            if(overlapping->identifier != UI->player->targetID ||  UI->player->action == Action_None)
                            {
                                overlapping->modulationWithFocusColor = worldMode->modulationWithFocusColor;
                            }
                            
                        }        
                        
                        UIResetListPossibility(UI, possibleOverlappingActions);
                        if(overlapping->identifier == UI->myPlayer->overlappingIdentifier || (UI->worldMode->editingEnabled && input->altDown))
                        {
                            HandleOverlappingInteraction(UI, &output, input, overlapping);
                        }
                        else
                        {
                            UIResetListIndex(UI, possibleOverlappingActions);
                        }
                    }
                }
            }
            
        } break;
        
        
        case UIMode_Book:
        {
            b32 bookOnFocus = UpdateAndRenderBook(UI, input);
            
            if(!bookOnFocus)
            {
                UIAddInteraction(UI, input, mouseLeft, UISetValueInteraction(UI, UI_Trigger, &UI->nextMode, UIMode_None));
            }
            if(true)
            {
                UI->skillSlotTimeout = UI->skillSlotMaxTimeout;
                UIOverdrawEssences(UI);
            }
        } break;
    }
    
    
    
    u32 targetWrapCount = 1;
    i32 wrapHere = UI->activeSkillSlotIndex;
    i32 offset = 0;
    i32 startIndex = 0;
    
    if(input->mouseWheelOffset > 0)
    {
        startIndex = UI->activeSkillSlotIndex + 1;
        offset = 1;
        if(UI->activeSkillSlotIndex == -1)
        {
            wrapHere = 0;
            startIndex = wrapHere;
            targetWrapCount = 2;
        }
    }
    else if(input->mouseWheelOffset < 0)
    {
        startIndex = UI->activeSkillSlotIndex - 1;
        offset = -1;
        if(UI->activeSkillSlotIndex == -1)
        {
            wrapHere = MAXIMUM_SKILL_SLOTS - 1;
            startIndex = wrapHere;
            targetWrapCount = 2;
        }
    }
    
    if(offset)
    {
        UI->skillSlotTimeout = UI->skillSlotMaxTimeout;
    }
    else
    {
        if(UI->activeSkillSlotIndex != -1)
        {
            UISkill* skill = UI->skills + UI->activeSkillSlotIndex;
            if(skill->taxonomy)
            {
                TaxonomySlot* taxSlot = GetSlotForTaxonomy(UI->table, skill->taxonomy);
                if(taxSlot->isPassiveSkill)
                {
                    offset = 1;
                    startIndex = UI->activeSkillSlotIndex + 1;
                }
            }
        }
    }
    
    if(offset)
    {	
        u32 wrappedCount = 0;
        while(true)
        {
            startIndex = Wrap(0, startIndex, MAXIMUM_SKILL_SLOTS);
            if(startIndex == wrapHere)
            {
                if(++wrappedCount == targetWrapCount)
                {
                    UI->activeSkillSlotIndex = -1;
                    break;
                }
            }
            
            UISkill* skill = UI->skills + startIndex;
            u32 taxonomy = skill->taxonomy;
            TaxonomySlot* taxSlot = GetSlotForTaxonomy(UI->table, taxonomy);
            
            
            if(!taxonomy || !taxSlot->isPassiveSkill)
            {
                UI->activeSkillSlotIndex = (u32) startIndex;
                SendActiveSkillRequest(taxonomy);
                break;
            }
            startIndex += offset;
        }
    }
    
    if(LengthSq(output.inputAcc) > 0)
    {
        UI->mode = UIMode_None;
        UI->nextMode = UIMode_None;
    }
    
    r32 targetSkillSlotTime = 2.0f;
    UI->skillSlotTimeout -= input->timeToAdvance;
    
    r32 alpha = 1.0f;
    if(UI->skillSlotTimeout < targetSkillSlotTime)
    {
        alpha = Clamp01MapToRange(0, UI->skillSlotTimeout, targetSkillSlotTime);
    }
    
    //UIOverdrawSkillSlots(UI, alpha, input);
    
    
    WrapScrollableList(&UI->possibleTargets);
    UpdateScrollableList(UI, UI->toUpdateList, scrollOffset);
    UIRenderList(UI, UI->toRenderList);
    
    if(worldMode->editingEnabled)
    {
        UIRenderEditor(UI, input);
    }
    
    UIRenderTooltip(UI);
    
    UI->previousFrameWasAllowedToQuit = input->allowedToQuit;
    
    for(u32 buttonIndex = 0; buttonIndex < MAX_BUTTON_COUNT; ++buttonIndex)
    {
        UIInteraction* interaction = UI->hotInteractions + buttonIndex;
        if(interaction->priority != UIPriority_NotValid)
        {
            PlatformButton* button = input->buttons + buttonIndex;
            if(Pressed(button))
            {
                FREELIST_FREE(UI->activeInteractions[buttonIndex].firstAction, UIInteractionAction, UI->firstFreeInteractionAction);
                
                UIResetActiveInteractions(UI, buttonIndex, interaction->excludeFromReset);
                UIDispatchInteraction(UI, interaction, UI_Trigger, input->timeToAdvance);
                
               
                UI->activeInteractions[buttonIndex] = *interaction;
                
                interaction->firstAction = 0;
                interaction->lastAction = 0;
            }
            
            interaction->priority = UIPriority_NotValid;
        }
    }
    
    for(u32 buttonIndex = 0; buttonIndex < MAX_BUTTON_COUNT; ++buttonIndex)
    {
        UIInteraction* interaction = UI->activeInteractions + buttonIndex;
        if(interaction->priority != UIPriority_NotValid)
        {
            PlatformButton* button = input->buttons + buttonIndex;
            if(!(interaction->flags & UI_Ended))
            {
                UIDispatchInteraction(UI, interaction, UI_Idle, input->timeToAdvance);
                if(Clicked(button, 10))
                {
                    UIDispatchInteraction(UI, interaction, UI_Click, input->timeToAdvance);
                    interaction->flags |= UI_Ended;
                }
                else if(KeptDown(button, 12))
                {
                    UIDispatchInteraction(UI, interaction, UI_KeptPressed, input->timeToAdvance, true);
                }
                else if(Released(button))
                {
                    UIDispatchInteraction(UI, interaction, UI_Release, input->timeToAdvance);
                    interaction->flags |= UI_Ended;
                }
            }
            else
            {
                UIDispatchInteraction(UI, interaction, UI_Activated | UI_Retroactive, input->timeToAdvance);
            }
            
            if(!ConditionsSatisfied(interaction))
            {
                interaction->priority = UIPriority_NotValid;
            }
        }
    }
        
    return output;
}