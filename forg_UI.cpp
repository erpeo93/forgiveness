#include "forg_UIcommon.cpp"
#include "forg_book.cpp"

inline Vec2 UIFollowingP(UIButton* button, r32 separator)
{
    Vec2 buttonP = button->textP + V2(button->textDim.x + separator, 0);
    return buttonP;
    
}

inline UIButton UIBtn(UIState* UI, Vec2 P, EditorLayout* layout, Vec4 color, char* text)
{
    UIButton result;
    
    Rect2 bounds = GetUIOrthoTextBounds(UI, text, layout->fontScale, P);
    result.textP = P;
    result.textDim = GetDim(bounds);
    
    Vec2 boundsDim = result.textDim * 1.1f;
    Vec2 boundsCenter = GetCenter(bounds);
    
    result.bounds = RectCenterDim(boundsCenter, boundsDim);
    result.fontScale = layout->fontScale;
    result.Z = layout->additionalZBias;
    
    result.color = color;
    result.text = text;
    
    return result;
}

inline void UIButtonInteraction(UIButton* button, UIInteraction interaction)
{
    button->interaction = interaction;
}

inline Rect2 UIDrawButton(UIState* UI, PlatformInput* input, UIButton* button, r32 alpha = 1.0f)
{
    ObjectTransform buttonTranform = FlatTransform();
    buttonTranform.additionalZBias = button->Z + 0.001f;
    
    Vec4 buttonColor = button->color;
    buttonColor.a *= alpha;
    
    if(PointInRect(button->bounds, UI->relativeScreenMouse) && button->interaction.actionCount)
    {
        UIAddInteraction(UI, input, mouseLeft, button->interaction);
    }
    else
    {
        buttonColor.a *= 0.2f;
    }
    
    PushRect(UI->group, buttonTranform, button->bounds, buttonColor);
    PushUIOrthoText(UI, button->text, button->fontScale, button->textP, V4(1, 1, 1, alpha), button->Z + 0.002f);
    
    
    return button->bounds;
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
                    UIAddStandardAction(&autoInteraction, UI_Trigger, block->names[nameIndex], ColdPointer(UI->keyboardBuffer), ColdPointer(block->names[nameIndex]));
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
        UIAddInteraction(UI, input, actionDown, UISetValueInteraction(UI_Trigger, &UI->currentAutocompleteSelectedIndex, nextIndex));
        UIAddInteraction(UI, input, actionUp, UISetValueInteraction(UI_Trigger, &UI->currentAutocompleteSelectedIndex, prevIndex));
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

inline UIAutocomplete* UIFindAutocomplete(UIState* UI, EditorElement* grandParent, EditorElement* parent, char* name)
{
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
    else if(StrEqual(name, "type"))
    {
        if(grandParent && grandParent->type == EditorElement_List)
        {
            hash = StringHash(grandParent->elementName);
        }
    }
    
	UIAutocomplete* result = UIFindAutocomplete(UI, hash);
    
    return result;
}

inline UIAutocomplete* UIAddAutocomplete(UIState* UI, char* name)
{
    Assert(UI->autocompleteCount < ArrayCount(UI->autocompletes));
    UIAutocomplete* result = UI->autocompletes + UI->autocompleteCount++;
    result->hash = StringHash(name);
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
    UIAutocomplete* autocomplete = UIAddAutocomplete(UI, autocompleteName);
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
        
        UIAutocomplete* autocomplete = UIAddAutocomplete(UI, nameNoPoint);
        
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
        result.color = V4(0, 1, 0, 1);
        if(!UI->activeLabel)
        {
            if(PointInRect(result.bounds, UI->relativeScreenMouse))
            {
                UIInteraction mouseInteraction = {};
                if(StrEqual(text, "true"))
                {
                    result.color = V4(1, 0, 1, 1);
                    UIAddStandardAction_(&mouseInteraction, UI_Trigger, sizeof(root->value), ColdPointer(root->value), ColdPointer(UI->falseString));
                    UIAddReloadElementAction(&mouseInteraction, UI_Trigger, widget->root);
                    
                }
                else if(StrEqual(text, "false"))
                {
                    result.color = V4(1, 0, 1, 1);
                    UIAddStandardAction_(&mouseInteraction, UI_Trigger, sizeof(root->value), ColdPointer(root->value), ColdPointer(UI->trueString));
                    UIAddReloadElementAction(&mouseInteraction, UI_Trigger, widget->root);
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
                            UIAutocomplete* autocomplete = UIFindAutocomplete(UI, parents.grandParents[0], parents.father, root->name);
                            if(autocomplete)
                            {
                                canEdit = true;
                            }
                        }
                    }
                    
                    if(canEdit)
                    {
                        result.color = V4(1, 0, 1, 1);
                        mouseInteraction = UISetValueInteraction(UI_Click, &UI->active, root);
                        UIAddSetValueAction(&mouseInteraction, UI_Click, &UI->activeParent, parents.father);
                        UIAddSetValueAction(&mouseInteraction, UI_Click, &UI->activeGrandParent, parents.grandParents[0]);
                        UIAddSetValueAction(&mouseInteraction, UI_Click, &UI->currentAutocompleteSelectedIndex, -1);   UIAddClearAction(&mouseInteraction, UI_Click, ColdPointer(UI->keyboardBuffer), sizeof(UI->keyboardBuffer));
                        
                        
                        if(root->type == EditorElement_Real)
                        {
                            UIAddStandardAction_(&mouseInteraction, UI_Trigger, sizeof(root->value), ColdPointer(UI->realDragging), ColdPointer(root->value));
                            UIAddOffsetStringEditorElement(&mouseInteraction, UI_Idle, ColdPointer(root->value), ColdPointer(&UI->deltaScreenMouseP.y), 0.01f);
                            UIAddUndoRedoAction(&mouseInteraction, UI_Release, UndoRedoDelayedString(widget, root->value, sizeof(root->value), root->value, ColdPointer(root->value)));
                            UIAddReloadElementAction(&mouseInteraction, UI_Idle, widget->root);
                            UIAddReloadElementAction(&mouseInteraction, UI_Release, widget->root);
                            if(StrEqual(widget->name, "Editing Tabs"))
                            {
                                UIAddRequestAction(&mouseInteraction, UI_Release, SendDataFileRequest());
                            }
                            
                        }
                    }
                }
                
                UIAddInteraction(UI, input, mouseLeft, mouseInteraction);
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
            UIAddUndoRedoAction(&confirmInteraction, UI_Trigger, UndoRedoString(widget, root->value, sizeof(root->value), root->value, UI->keyboardBuffer));
            
            UIAddStandardAction(&confirmInteraction, UI_Trigger, root->value, ColdPointer(root->value), ColdPointer(UI->keyboardBuffer));
            UIAddSetValueAction(&confirmInteraction, UI_Trigger, &UI->active, 0);    
            UIAddSetValueAction(&confirmInteraction, UI_Trigger, &UI->activeParent, 0);    
            UIAddSetValueAction(&confirmInteraction, UI_Trigger, &UI->activeGrandParent, 0);    
            
            
            UIAddReloadElementAction(&confirmInteraction, UI_Trigger, widget->root);
            if(StrEqual(widget->name, "Editing Tabs"))
            {
                UIAddRequestAction(&confirmInteraction, UI_Trigger, SendDataFileRequest());
            }
            
            UIAddInteraction(UI, input, confirmButton, confirmInteraction);
        }
        
        if(root->type == EditorElement_String)
        {
            UIAutocomplete* autocomplete = UIFindAutocomplete(UI, parents.grandParents[0], parents.father, root->name);
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

inline Rect2 UIRenderEditorTree(UIState* UI, EditorWidget* widget, EditorLayout* layout, EditorElementParents parents, EditorElement* parent_, EditorElement* root_, PlatformInput* input, b32 canDelete)
{
    for(u32 parentIndex = ArrayCount(parents.grandParents) - 1; parentIndex > 0; --parentIndex)
    {
        parents.grandParents[parentIndex] = parents.grandParents[parentIndex - 1];
    }
    
    parents.grandParents[0] = parents.father;
    parents.father = parent_;
    
    
    EditorElement* father = parents.father;
    EditorElement* grandFather = parents.grandParents[0];
    
    Rect2 totalResult = InvertedInfinityRect2();
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
            if(root->type == EditorElement_List)
            {
                FormatString(name, sizeof(name), "%s |-|", root->name);
            }
            else
            {
                FormatString(name, sizeof(name), "%s", root->name);
            }
            
            
            
            Rect2 nameBounds = GetUIOrthoTextBounds(UI, name, layout->fontScale, layout->P);
            Vec4 nameColor = V4(1, 1, 1, 1);
            
            b32 nameHot = false;
            char* nameToShow = name;
            b32 showName = (nameToShow[0]);
            
            if(PointInRect(nameBounds, UI->relativeScreenMouse))
            {
                nameColor = V4(1, 1, 0, 1);
                u32 finalFlags = IsSet(root, EditorElem_Expanded) ? (root->flags & ~EditorElem_Expanded) : (root->flags | EditorElem_Expanded);
                
                UIAddInteraction(UI, input, mouseLeft, UISetValueInteraction(UI_Trigger, &root->flags, finalFlags));
                
                if(!UI->activeLabel && !UI->active && father && (father->flags & EditorElem_LabelsEditable))
                {
                    nameColor = V4(1, 0, 0, 1);
                    UIInteraction labelInteraction = UISetValueInteraction(UI_Trigger, &UI->activeLabel, root);
                    UIAddClearAction(&labelInteraction, UI_Trigger, ColdPointer(UI->keyboardBuffer), sizeof(UI->keyboardBuffer));
                    
                    UIAddInteraction(UI, input, mouseRight, labelInteraction);
                }
                nameHot = true;
            }
            
            if(root == UI->activeLabel)
            {
                nameToShow = UI->showBuffer;
                
                UIInteraction confirmInteraction = {};
                UIAddUndoRedoAction(&confirmInteraction, UI_Trigger, UndoRedoString(widget, root->name, sizeof(root->name), root->name, UI->keyboardBuffer));
                UIAddStandardAction(&confirmInteraction, UI_Trigger, root->name, ColdPointer(root->name), ColdPointer(UI->keyboardBuffer));
                UIAddSetValueAction(&confirmInteraction, UI_Trigger, &UI->activeLabel, 0);    
                UIAddClearAction(&confirmInteraction, UI_Trigger, ColdPointer(UI->keyboardBuffer), sizeof(UI->keyboardBuffer));
                
                UIAddReloadElementAction(&confirmInteraction, UI_Trigger, widget->root);
                if(StrEqual(widget->name, "Editing Tabs"))
                {
                    UIAddRequestAction(&confirmInteraction, UI_Trigger, SendDataFileRequest());
                }
                
                UIAddInteraction(UI, input, confirmButton, confirmInteraction);
            }
            
            Vec2 nameP = layout->P;
            result = nameBounds;
            
            if(showName)
            {
                layout->P += V2(0, -layout->childStandardHeight);
            }
            
            Vec2 lineStartP = nameP + V2(0.2f * layout->nameValueDistance, -0.2f * layout->childStandardHeight);
            Vec3 lineEndOffset = V3(0.2f * layout->nameValueDistance, 1.2f * layout->childStandardHeight, 0);
            
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
                    
                    UIAddTabResult addTab = UIAddTabValueInteraction(UI, widget, input, parents, root, valueP, layout, text);
                    
                    PushUIOrthoText(UI, text, layout->fontScale, valueP, addTab.color, layout->additionalZBias);
                    result = Union(result, addTab.bounds);
                    
                } break;
                
                case EditorElement_List:
                {
                    if(IsSet(root, EditorElem_Expanded))
                    {
                        Vec3 verticalStartP = V3(lineStartP, 0);
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
                        
                        if(root == UI->copying)
                        {
                            Vec4 outlineColor = V4(0, 0, 1, 1);
                            r32 thickness = 2;
                            ObjectTransform nameTranform = FlatTransform();
                            nameTranform.additionalZBias = layout->additionalZBias;
                            PushRectOutline(UI->group, nameTranform, nameBounds, outlineColor, thickness);
                        }
                        else
                        {
                            if(!UI->hotStructThisFrame && PointInRect(nameBounds, UI->relativeScreenMouse))
                            {
                                UI->hotStructThisFrame = true;
                                UI->hotStructBounds = nameBounds;
                                UI->hotStructZ = layout->additionalZBias;
                                UI->hotStructColor = V4(0, 1, 0, 1);
                                UI->hotStruct = root;
                                UI->hotWidget = widget;
                                
                                UIAddInteraction(UI, input, copyButton, UISetValueInteraction(UI_Trigger, &UI->copying, root));
                                if(UI->copying)
                                {
                                    b32 matches = StrEqual(root->name, UI->copying->name);
                                    if(matches)
                                    {
                                        UIAddInteraction(UI, input, pasteButton, UISetValueInteraction(UI_Trigger, &root->flags, root->flags | EditorElem_Pasted));
                                    }
                                    else
                                    {
                                        UI->hotStructColor.a = 0;
                                    }
                                }
                            }
                        }
                        
                        b32 moveHorizontally = (root->firstInList && root->firstInList->name[0]);
                        if(moveHorizontally)
                        {
                            layout->P += V2(layout->nameValueDistance, 0);
                        }
                        b32 canDeleteElements = !IsSet(root, EditorElem_CantBeDeleted);
                        result = Union(result, UIRenderEditorTree(UI, widget, layout, parents, root, root->firstInList, input, canDeleteElements));
                        
                        if(moveHorizontally)
                        {
                            layout->P -= V2(layout->nameValueDistance, 0);
                        }
                        
                        Vec3 verticalEndP = V3(layout->P, 0) + lineEndOffset;
                        PushLine(UI->group, V4(1, 1, 1, 1), verticalStartP, verticalEndP, 1);
                        
                        
                        if(root->emptyElement)
                        {
                            UIButton addButton = UIBtn(UI, nameP + V2(GetDim(nameBounds).x, 0) + V2(30, 0), layout, V4(1, 0, 0, 1), "add");
                            UIInteraction addInteraction = UIAddEmptyElementToListInteraction(UI_Trigger, widget, root);
                            
                            if(StrEqual(widget->name, "Editing Tabs"))
                            {
                                UIAddRequestAction(&addInteraction, UI_Trigger, SendDataFileRequest());
                            }
                            
                            UIAddReloadElementAction(&addInteraction, UI_Trigger, widget->root);
                            UIButtonInteraction(&addButton, addInteraction);
                            result = Union(result, UIDrawButton(UI, input, &addButton));
                        }
                    }
                } break;
                
                case EditorElement_Struct:
                {
                    if(IsSet(root, EditorElem_Expanded) || !root->name[0])
                    {
                        Vec3 verticalStartP = V3(lineStartP, 0);
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
                        Rect2 structBounds = UIRenderEditorTree(UI, widget, layout, parents, root, root->firstValue, input, false);
                        
                        if(canDelete)
                        {
                            if(root != UI->copying)
                            {
                                UIButton deleteButton = UIBtn(UI, GetCenter(structBounds) + V2(30, 0) +0.5f * V2(GetDim(structBounds).x, 0), layout, V4(1, 0, 0, 1), "delete");
                                UIButtonInteraction(&deleteButton, UISetValueInteraction(UI_Trigger, &root->flags, (root->flags | EditorElem_Deleted)));
                                structBounds = Union(structBounds, UIDrawButton(UI, input, &deleteButton));
                            }
                        }
                        
                        if(root->firstValue && father && (father->flags & EditorElem_PlaySoundButton))
                        {
                            UIButton playButton = UIBtn(UI, GetCenter(structBounds) + V2(70, 0) +0.5f * V2(GetDim(structBounds).x, 0), layout, V4(0, 1, 0, 1), "play");
                            
                            u64 soundTypeHash = StringHash(father->name);
                            u64 soundNameHash = StringHash(root->firstValue->value);
                            
                            UIButtonInteraction(&playButton, UIPlaySoundInteraction(UI_Trigger, soundTypeHash, soundNameHash));
                            structBounds = Union(structBounds, UIDrawButton(UI, input, &playButton));
                        }
                        if(root->firstValue && father && (father->flags & EditorElem_PlayEventSoundButton))
                        {
                            UIButton playButton = UIBtn(UI, GetCenter(structBounds) + V2(70, 0) +0.5f * V2(GetDim(structBounds).x, 0), layout, V4(0, 1, 0, 1), "play");
                            
                            char* soundType = GetValue(root, "soundType");
                            char* soundName = GetValue(root, "sound");
                            u64 soundTypeHash = StringHash(soundType);
                            u64 soundNameHash = StringHash(soundName);
                            
                            UIButtonInteraction(&playButton, UIPlaySoundInteraction(UI_Trigger, soundTypeHash, soundNameHash));
                            structBounds = Union(structBounds, UIDrawButton(UI, input, &playButton));
                        }
                        else if(root->firstValue && father && (father->flags & EditorElem_PlayEventButton))
                        {
                            UIButton playButton = UIBtn(UI, GetCenter(structBounds) + V2(20, 0) +0.5f * V2(GetDim(structBounds).x, 0), layout, V4(0, 1, 0, 1), "play");
                            
                            u64 eventNameHash = StringHash(root->name);
                            
                            UIButtonInteraction(&playButton, UIPlaySoundEventInteraction(UI_Trigger, eventNameHash));
                            structBounds = Union(structBounds, UIDrawButton(UI, input, &playButton));
                        }
                        
                        r32 padding = 5;
                        layout->P += V2(0, -padding);
                        Rect2 realBounds = AddRadius(structBounds, V2(padding, padding));
                        
                        
                        r32 thickness = 1.0f;
                        Vec4 outlineColor = V4(1, 1, 1, 1);
                        if(root == UI->copying)
                        {
                            thickness = 2.0f;
                            outlineColor = V4(0, 0, 1, 1);
                        }
                        else
                        {
                            if(!UI->hotStructThisFrame && PointInRect(realBounds, UI->relativeScreenMouse))
                            {
                                UI->hotStructThisFrame = true;
                                UI->hotStructBounds = realBounds;
                                UI->hotStructZ = layout->additionalZBias;
                                UI->hotStructColor = V4(0, 1, 0, 1);
                                UI->hotStruct = root;
                                UI->hotWidget = widget;
                                
                                UIAddInteraction(UI, input, copyButton, UISetValueInteraction(UI_Trigger, &UI->copying, root));
                                if(UI->copying)
                                {
                                    b32 matches = true;
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
                                    
                                    if(matches)
                                    {
                                        UIAddInteraction(UI, input, pasteButton, UISetValueInteraction(UI_Trigger, &root->flags, root->flags | EditorElem_Pasted));
                                    }
                                    else
                                    {
                                        UI->hotStructColor.a = 0;
                                    }
                                }
                            }
                        }
                        
                        if(!canDelete)
                        {
                            if(UI->hotStructThisFrame && root == UI->hotStruct)
                            {
                                UIInteraction dragInteraction = UISetValueInteraction(UI_Trigger, &UI->dragging, root);
                                UIAddSetValueAction(&dragInteraction, UI_Trigger, &UI->draggingParent, father);
                                UIAddReleaseDragAction(&dragInteraction, UI_Release);
                                UIAddInteraction(UI, input, mouseRight, dragInteraction);
                            }
                        }
                        
                        result = Union(result, realBounds);
                        layout->P += V2(-layout->nameValueDistance, 0);
                        layout->P += V2(0, -padding);
                        
                        Vec3 verticalEndP = V3(layout->P, 0) + lineEndOffset;
                        PushLine(UI->group, V4(1, 1, 1, 1), verticalStartP, verticalEndP, 1);
                        
                        ObjectTransform structTranform = FlatTransform();
                        structTranform.additionalZBias = layout->additionalZBias;
                        
                        PushRectOutline(UI->group, structTranform, realBounds, outlineColor, thickness);
                    }
                } break;
                
                case EditorElement_Taxonomy:
                {
                    TaxonomySlot* slot = GetSlotForTaxonomy(UI->table, root->taxonomy);
                    
                    if(slot->name[0] == '#')
                    {
                        nameToShow++;
                    }
                    
                    if(slot->editorChangeCount || UIChildModified(UI->table, slot))
                    {
                        nameColor = V4(1, 0, 0, 1);
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
                                char* cancText = "canc";
                                UIInteraction cancReviveInteraction = SendRequestInteraction(UI_Trigger, DeleteTaxonomyRequest(root->taxonomy));
                                
                                if(root->name[0] == '#')
                                {
                                    cancText = "revive";
                                    cancReviveInteraction = SendRequestInteraction(UI_Trigger, ReviveTaxonomyRequest(root->taxonomy));
                                }
                                
                                UIButton deleteButton = UIBtn(UI, startingPos, layout, V4(1, 0, 0, 1), cancText);
                                UIButtonInteraction(&deleteButton, cancReviveInteraction);
                                
                                result = Union(result, UIDrawButton(UI, input, &deleteButton));
                                
                                instantiateP = UIFollowingP(&deleteButton, buttonSeparator);
                            }
                            
                            UIButton instantiateButton = UIBtn(UI, instantiateP, layout, V4(0, 0, 1, 1), "place");
                            
                            UIInteraction instantiateInteraction = NullInteraction();
                            
                            r32 instantiateAlpha = 0.2f;
                            if(root->name[0] != '#')
                            {
                                instantiateAlpha = 1.0f;
                                instantiateInteraction = SendRequestInteraction(UI_Click, InstantiateTaxonomyRequest(root->taxonomy, V3(1, 0, 0)));
                                UIAddSetValueAction(&instantiateInteraction, UI_Idle, &UI->instantiatingTaxonomy, root->taxonomy);
                                UIAddSetValueAction(&instantiateInteraction, UI_Release, &UI->instantiatingTaxonomy, 0); 
                            }
                            
                            UIButtonInteraction(&instantiateButton, instantiateInteraction);
                            result = Union(result, UIDrawButton(UI, input, &instantiateButton, instantiateAlpha));
                            
                            
                            
                            UIButton editButton = UIBtn(UI, UIFollowingP(&instantiateButton, buttonSeparator), layout, V4(0, 1, 0, 1), "edit");
                            
                            UIInteraction editInteraction = NullInteraction();
                            r32 editAlpha = 0.2f;
                            
                            if(true)//root->name[0] != '#')
                            {
                                editAlpha = 1.0f;
                                editInteraction = SendRequestInteraction(UI_Trigger, EditRequest(root->taxonomy));
                            }
                            
                            UIButtonInteraction(&editButton, editInteraction);
                            result = Union(result, UIDrawButton(UI, input, &editButton, editAlpha));
                        }
                        
                        
                        Vec3 verticalStartP = V3(lineStartP, 0);
                        
                        layout->P += V2(layout->nameValueDistance, 0);
                        
                        result = Union(result, UIRenderEditorTree(UI, widget, layout, parents, root, root->firstChild, input, false));
                        
                        layout->P += V2(-layout->nameValueDistance, 0);
                        
                        Vec3 verticalEndP = V3(layout->P, 0) + lineEndOffset;
                        PushLine(UI->group, V4(1, 1, 1, 1), verticalStartP, verticalEndP, 1);
                    }
                } break;
                
                case EditorElement_EmptyTaxonomy:
                {
                    Vec4 addColor = V4(1, 1, 0.5f, 1);
                    if(root != UI->active)
                    {
                        char* text = "Add new...";
                        nameBounds = GetUIOrthoTextBounds(UI, text, layout->fontScale, nameP); 
                        layout->P += V2(0, -layout->childStandardHeight);
                        
                        if(PointInRect(nameBounds, UI->relativeScreenMouse))
                        {
                            UIInteraction mouseInteraction = UISetValueInteraction(UI_Trigger, &UI->active, root);
                            UIAddSetValueAction(&mouseInteraction, UI_Trigger, &UI->activeParent, father); 
                            UIAddSetValueAction(&mouseInteraction, UI_Trigger, &UI->activeGrandParent, grandFather); 
                            UIAddClearAction(&mouseInteraction, UI_Trigger, ColdPointer(UI->keyboardBuffer), sizeof(UI->keyboardBuffer));
                            UIAddInteraction(UI, input, mouseLeft, mouseInteraction);
                            addColor = V4(1, 0, 0, 1);
                        }
                        
                        result = GetUIOrthoTextBounds(UI, text, layout->fontScale, nameP);
                        PushUIOrthoText(UI, text, layout->fontScale, nameP, addColor, layout->additionalZBias);
                    }
                    else
                    {
                        layout->P += V2(0, -layout->childStandardHeight);
                        char* text = UI->showBuffer;
                        addColor = V4(1, 0, 0, 1);
                        
                        if(UI->bufferValid)
                        {
                            addColor = V4(0, 1, 0, 1);
                            UIInteraction buttonInteraction = SendRequestInteraction(UI_Trigger, AddTaxonomyRequest(root->parentTaxonomy, UI->keyboardBuffer));
                            UIAddSetValueAction(&buttonInteraction, UI_Trigger, &UI->active, 0);    
                            UIAddSetValueAction(&buttonInteraction, UI_Trigger, &UI->activeParent, 0);    
                            UIAddSetValueAction(&buttonInteraction, UI_Trigger, &UI->activeGrandParent, 0);    
                            UIAddInteraction(UI, input, confirmButton, buttonInteraction);
                        }
                        
                        result = GetUIOrthoTextBounds(UI, text, layout->fontScale, nameP);
                        PushUIOrthoText(UI, text, layout->fontScale, nameP, addColor, layout->additionalZBias);
                    }
                } break;
                
                case EditorElement_Animation:
                {
                    Vec3 P = V3(layout->P + 2.0f * V2(0, layout->childStandardHeight), 0);
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
                        r32 scale = ToR32(GetValue(pause, "scale"));
                        
                        r32 speed = ToR32(GetValue(pause, "speed"));
                        r32 timeToAdvance = play ? input->timeToAdvance : 0;
                        
                        
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
                        
                        AnimationOutput output =  PlayAndDrawAnimation(UI->worldMode, UI->group, V4(-1, -1, -1, -1), &test, animationScale, 0, animationBase, 0, V4(1, 1, 1, 1), 0, 0, InvertedInfinityRect2(), 1, true, showBones, showBitmaps, timer, nameHashID);
                        
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
                        originTransform.additionalZBias = 30.0f;
                        PushRect(UI->group, originTransform, P, V2(6, 6), V4(1, 0, 0, 1));
                        
                        if(play)
                        {
                            PlaySoundForAnimation(UI->worldMode, UI->group->assets, animationSlot, nameHashID, oldTimer, timer);
                        }
                    }
                } break;
            }
            
            if(showName)
            {
                PushUIOrthoText(UI, nameToShow, layout->fontScale, nameP, nameColor, layout->additionalZBias);
            }
            
            totalResult = Union(totalResult, result);
        }
    }
    
    return totalResult;
}

inline void UIRenderEditor(UIState* UI, PlatformInput* input)
{
    RenderGroup* group = UI->group;
    GameRenderCommands* commands = group->commands;
    
    r32 width = (r32) commands->settings.width;
    r32 height = (r32) commands->settings.height;
    
    SetCameraTransform(group, Camera_Orthographic, 0.0f, V3(2.0f / width, 0.0f, 0.0f), V3(0.0f, 2.0f / width, 0.0f), V3( 0, 0, 1));
    
    UIAddInteraction(UI, input, undo, UIUndoInteraction(UI_Trigger));
    UIAddInteraction(UI, input, redo, UIRedoInteraction(UI_Trigger));
    
    if(UI->instantiatingTaxonomy)
    {
        Vec3 mouseOffset = UI->worldMouseP;
        UIAddInteraction(UI, input, mouseRight,SendRequestInteraction(UI_Trigger, InstantiateTaxonomyRequest(UI->instantiatingTaxonomy, mouseOffset)));
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
    
    
    UIInteraction esc = UISetValueInteraction(UI_Trigger, &UI->active, 0);
    UIAddSetValueAction(&esc, UI_Trigger, &UI->activeParent, 0);
    UIAddSetValueAction(&esc, UI_Trigger, &UI->activeGrandParent, 0);
    UIAddSetValueAction(&esc, UI_Trigger, &UI->activeLabel, 0);
    UIAddClearAction(&esc, UI_Trigger, ColdPointer(UI->keyboardBuffer), sizeof(UI->keyboardBuffer));
    UIAddSetValueAction(&esc, UI_Trigger, &UI->copying, 0);
    UIAddSetValueAction(&esc, UI_Trigger, &UI->currentAutocompleteSelectedIndex, -1);
    UIAddInteraction(UI, input, escButton, esc);
    
    
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
        FormatString(UI->showBuffer, sizeof(UI->showBuffer), "%s|", UI->keyboardBuffer);
    }
    else
    {
        FormatString(UI->showBuffer, sizeof(UI->showBuffer), "%s", UI->keyboardBuffer);
    }
    
    UI->bufferValid = false;
    
    if(UI->keyboardBuffer[0] && UI->activeLabel)
    {
        UI->bufferValid = true;
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
                UIAutocomplete* options = UIFindAutocomplete(UI, UI->activeGrandParent, UI->activeParent, UI->active->name);
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
                    UI->bufferValid = true;
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
            Vec2 widgetP = resizeP + V2(20, -8);
            Vec2 resizeDim = V2(8, 8);
            Rect2 rect = RectCenterDim(resizeP, resizeDim);
            Vec4 color = V4(1, 1, 1, 1);
            if(PointInRect(rect, UI->relativeScreenMouse))
            {
                UIInteraction moveInteraction = {};
                UIAddStandardAction(&moveInteraction, UI_Idle, Vec2, ColdPointer(&widget->permanent.P),ColdPointer(&UI->relativeScreenMouse));
                UIAddInteraction(UI, input, mouseLeft, moveInteraction);
                color = V4(1, 0, 0, 1);
            }
            
            ObjectTransform sizeTranform = FlatTransform();
            sizeTranform.additionalZBias = widgetZ;
            
            PushRect(UI->group, sizeTranform, rect, color);
            
            
            EditorLayout* layout = &widget->layout;
            layout->P = widgetP;
            layout->additionalZBias = widgetZ;
            
            Rect2 widgetTitleBounds = GetUIOrthoTextBounds(UI, widget->name, layout->fontScale, widgetP);
            
            r32 widgetAlpha = widget->permanent.expanded ? 1.0f : 0.1f;
            Vec4 widgetColor = V4(0, 1, 0, widgetAlpha);
            if(PointInRect(widgetTitleBounds, UI->relativeScreenMouse))
            {
                widgetColor = V4(1, 0, 0, 1);
                UIAddInteraction(UI, input, mouseLeft, UISetValueInteraction(UI_Trigger, &widget->permanent.expanded, !widget->permanent.expanded));
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
                            
                            
                            UIButton leftButton = UIBtn(UI, leftP, &widget->layout, V4(1, 0, 0, 1), " << ");
                            UIButtonInteraction(&leftButton, UISetValueInteraction(UI_Trigger, &UI->editingTabIndex, UI->editingTabIndex - 1));
                            UIDrawButton(UI, input, &leftButton);
                            
                            Vec2 rightP = UIFollowingP(&leftButton, 10.0f);
                            UIButton rightButton = UIBtn(UI, rightP, &widget->layout, V4(1, 0, 0, 1), " >> ");
                            UIButtonInteraction(&rightButton, UISetValueInteraction(UI_Trigger, &UI->editingTabIndex, UI->editingTabIndex + 1));
                            UIDrawButton(UI, input, &rightButton);
                            
                            
                            
                            Vec2 saveP = UIFollowingP(&rightButton, 30.0f);
                            
                            r32 buttonAlpha = 0.2f;
                            
                            UIInteraction saveInteraction = NullInteraction();
                            if(editingSlot->editorChangeCount)
                            {
                                buttonAlpha = 1.0f;
                                saveInteraction = SendRequestInteraction(UI_Trigger, SaveTaxonomyTabRequest());
                            }
                            
                            char saveText[128];
                            
                            char* name = editingSlot->name[0] == '#' ? editingSlot->name + 1 : editingSlot->name;
                            FormatString(saveText, sizeof(saveText), "Save %s", name);
                            UIButton saveButton = UIBtn(UI, saveP, &widget->layout, V4(1, 0, 0, 1), saveText);
                            UIButtonInteraction(&saveButton, saveInteraction);
                            UIDrawButton(UI, input, &saveButton, buttonAlpha);
                            
                            u32 parentTaxonomy = GetParentTaxonomy(UI->table, editingSlot->taxonomy);
                            if(parentTaxonomy)
                            {
                                Vec2 editParentP = UIFollowingP(&saveButton, 30.0f);
                                UIButton editParentButton = UIBtn(UI, editParentP, &widget->layout, V4(1, 0, 0, 1), "Edit Parent");
                                
                                
                                UIButtonInteraction(&editParentButton, SendRequestInteraction(UI_Trigger, EditRequest(parentTaxonomy))); 
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
                        UIButton reloadButton = UIBtn(UI, reloadP, &widget->layout, V4(1, 0, 0, 1), " RELOAD ASSETS ");
                        
                        UIInteraction reloadInteraction = NullInteraction();
                        r32 reloadAlpha = 0.2f;
                        
                        if(!UI->reloadingAssets && !UI->patchingLocalServer)
                        {
                            reloadInteraction = SendRequestInteraction(UI_Trigger, ReloadAssetsRequest());
                            reloadAlpha = 1.0f;
                        }
                        UIButtonInteraction(&reloadButton, reloadInteraction);
                        UIDrawButton(UI, input, &reloadButton, reloadAlpha);
                        
                        
                        
                        Vec2 patchCheckP = UIFollowingP(&reloadButton, 20);
                        UIButton patchCheckButton = UIBtn(UI, patchCheckP, &widget->layout, V4(1, 0, 0, 1), " PATCH CHECK ");
                        
                        r32 checkAlpha = 0.2f;
                        UIInteraction checkInteraction = NullInteraction();
                        if(!UI->reloadingAssets && !UI->patchingLocalServer && !UIChildModified(UI->table, &UI->table->root))
                        {
                            checkInteraction = SendRequestInteraction(UI_Trigger, PatchCheckRequest());
                            checkAlpha = 1.0f;
                        }
                        UIButtonInteraction(&patchCheckButton, checkInteraction);
                        UIDrawButton(UI, input, &patchCheckButton, checkAlpha);
                        
                        
                        
                        Vec2 patchP = UIFollowingP(&patchCheckButton, 20);
                        UIButton patchButton = UIBtn(UI, patchP, &widget->layout, V4(1, 0, 0, 1), " PATCH SERVER ");
                        
                        r32 patchAlpha = 0.2f;
                        UIInteraction patchInteraction = NullInteraction();
                        if(!UI->reloadingAssets && !UI->patchingLocalServer && !UI->table->errorCount)
                        {
                            patchInteraction = SendRequestInteraction(UI_Trigger, PatchServerRequest());
                            patchAlpha = 1.0f;
                        }
                        UIButtonInteraction(&patchButton, patchInteraction);
                        UIDrawButton(UI, input, &patchButton, patchAlpha);
                        
                    } break;
                    
                    case EditorWidget_SoundEvents:
                    {
                        Vec2 saveP = widgetTitleBounds.min + V2(GetDim(widgetTitleBounds).x, 0) + V2(20.0f, 0);
                        
                        UIInteraction saveInteraction = NullInteraction();
                        
                        UIButton saveButton = UIBtn(UI, saveP, &widget->layout, V4(1, 0, 0, 1.0f), " SAVE ");
                        
                        r32 buttonAlpha = 0.2f;
                        if(widget->changeCount)
                        {
                            buttonAlpha = 1.0f;
                            saveInteraction =SendRequestInteraction(UI_Trigger, SaveAssetFadFileRequest("soundEvents", widget));
                            UIAddReloadElementAction(&saveInteraction, UI_Trigger, UI->table->soundEventsRoot);
                        }
                        
                        
                        UIButtonInteraction(&saveButton, saveInteraction);
                        UIDrawButton(UI, input, &saveButton, buttonAlpha);
                    } break;
                    
                    case EditorWidget_Components:
                    {
                        Vec2 saveP = widgetTitleBounds.min + V2(GetDim(widgetTitleBounds).x, 0) + V2(20.0f, 0);
                        
                        UIInteraction saveInteraction = NullInteraction();
                        
                        UIButton saveButton = UIBtn(UI, saveP, &widget->layout, V4(1, 0, 0, 1.0f), " SAVE ");
                        
                        r32 buttonAlpha = 0.2f;
                        if(widget->changeCount)
                        {
                            buttonAlpha = 1.0f;
                            saveInteraction =SendRequestInteraction(UI_Trigger, SaveAssetFadFileRequest("components", widget));
                        }
                        
                        
                        UIButtonInteraction(&saveButton, saveInteraction);
                        UIDrawButton(UI, input, &saveButton, buttonAlpha);
                    } break;
                }
            }
            
            
            layout->P.y -= layout->childStandardHeight;
            
            Rect2 widgetBounds = InvertedInfinityRect2();
            if(widget->permanent.expanded && widget->root)
            {
                EditorElementParents parents = {};
                widgetBounds = UIRenderEditorTree(UI, widget, layout, parents, 0, widget->root, input, false);
            }
            
            ObjectTransform widgetBoundsTransform = FlatTransform();
            widgetBoundsTransform.additionalZBias = widgetZ - 0.001f;
            
            PushRect(UI->group, widgetBoundsTransform, widgetBounds, V4(0, 0, 0, 0.5f));
            if(PointInRect(widgetBounds, UI->relativeScreenMouse))
            {
                widget->permanent.P.y -= input->mouseWheelOffset * 10.0f;
            }
            
            widgetZ += 1.0f;
        }
        
        if(widget->changeCount)
        {
            input->allowedToQuit = false;
        }
    }
    
    if(input->allowedToQuit && UIChildModified(UI->table, &UI->table->root))
    {
        input->allowedToQuit = false;
    }
    
    if(UI->hotStructThisFrame)
    {
        ObjectTransform structBoundsTranform = FlatTransform();
        structBoundsTranform.additionalZBias = UI->hotStructZ;
        
        PushRectOutline(UI->group, structBoundsTranform, UI->hotStructBounds, UI->hotStructColor, 2);
    }
    
    if(UI->dragging)
    {
        EditorLayout layout = UI->widgets[0].layout;
        layout.P = UI->relativeScreenMouse;
        layout.additionalZBias = 10.0f;
        
        EditorElementParents parents = {};
        
        EditorElement* oldNext = UI->dragging->next;
        UIRenderEditorTree(UI, 0, &layout, parents, 0, UI->dragging, input, false);
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
    
    if(UI->draggingEntity.taxonomy && !UI->player->animation.output.nearestCompatibleSlot.slotCount)
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

inline EquipInfo PossibleToEquip(TaxonomyTable* table, ClientEntity* entity, Object* object, SlotPlacement placement)
{
    u32 objectTaxonomy = GetObjectTaxonomy(table, object);
    EquipInfo result = PossibleToEquip_(table, entity->taxonomy, entity->equipment, objectTaxonomy, object->status, SlotPlacement_Both);
    
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
                    
                    EquipInfo equipInfo = PossibleToEquip(table, UI->player, focusObject, SlotPlacement_Both);
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
                    if(equipInfo.slotCount)
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
                    
                    
                    UIInteraction mouseInteraction = ScrollableListRequestInteraction(UI_Click, &UI->possibleObjectActions);
                    
                    UIRequest request = UIStandardInventoryRequest(Swap, container->identifier, objectIndex);
                    UIAddRequestAction(&mouseInteraction, UI_KeptPressed, request);
                    UIAddObjectToEntityAction(&mouseInteraction, UI_KeptPressed, ColdPointer(&UI->draggingEntity), focusObject);
                    UIAddInteraction(UI, input, mouseLeft, mouseInteraction);
                }
                else if(UI->draggingEntity.taxonomy && 
                        UI->draggingEntity.objects.objectCount == 0)
                {
                    UI->toUpdateList = 0;
                    UI->toRenderList = 0;
                    
                    UIInteraction swapInteraction = {};
                    UIRequest request = UIStandardInventoryRequest(Swap, container->identifier, objectIndex);
                    
                    UIAddRequestAction(&swapInteraction, UI_Trigger, request);
                    UIAddObjectToEntityAction(&swapInteraction, UI_Trigger, ColdPointer(&UI->draggingEntity), focusObject);
                    
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
                    UIAddRequestAction(&swapInteraction, UI_Trigger, request);
                    UIAddClearAction(&swapInteraction, UI_Trigger, ColdPointer(&UI->draggingEntity), sizeof(UI->draggingEntity));
                    
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

inline void UIAddChild(TaxonomyTable* table, EditorElement* element, EditorElementType type, char* name, char* value)
{
    EditorElement* newElement;
    FREELIST_ALLOC(newElement, table->firstFreeElement, PushStruct(&table->pool, EditorElement));
    
    
    newElement->type = type;
    FormatString(newElement->name, sizeof(newElement->name), "%s", name);
    FormatString(newElement->value, sizeof(newElement->value), "%s", value);
    
    newElement->next = element->firstChild;
    element->firstChild = newElement;
}

inline EditorWidget* StartWidget(UIState* UI, EditorWidgetType widget, u32 necessaryRole, char* name)
{
    EditorWidget* result = UI->widgets + widget;
    *result = {};
    result->permanent.expanded = true;
    result->necessaryRole = necessaryRole;
    result->layout.fontScale = 0.42f;
    result->layout.nameValueDistance = 50;
    result->layout.childStandardHeight = 30;
    FormatString(result->name, sizeof(result->name), name);
    
    return result;
}

inline void ResetUI(UIState* UI, GameModeWorld* worldMode, RenderGroup* group, ClientEntity* player, PlatformInput* input, r32 fontCoeff)
{
    UI->table = worldMode->table;
    if(!UI->initialized)
    {
        UI->initialized = true;
        if(worldMode->editingEnabled)
        {
            UI->uneditableTabRoot.type = EditorElement_String;
            FormatString(UI->uneditableTabRoot.name, sizeof(UI->uneditableTabRoot.name), "YOU CAN'T EDIT THIS");
            
            
            EditorWidget* taxonomyTree = StartWidget(UI, EditorWidget_TaxonomyTree, 0xffffffff, "Taxonomy Tree");
            taxonomyTree->permanent.P = V2(-800, -100);
            
            
            EditorWidget* taxonomyEditing = StartWidget(UI, EditorWidget_EditingTaxonomyTabs, 0xffffffff, "Editing Tabs");
            taxonomyEditing->permanent.P = V2(100, -100);
            
            
            EditorElement* animationStruct;
            FREELIST_ALLOC(animationStruct, UI->table->firstFreeElement, PushStruct(&UI->table->pool, EditorElement));
            animationStruct->type = EditorElement_Struct;
            
            
            
            EditorElement* animationRoot;
            FREELIST_ALLOC(animationRoot, UI->table->firstFreeElement, PushStruct(&UI->table->pool, EditorElement));
            animationRoot->type = EditorElement_Animation;
            
            EditorElement* animationActionTimer;
            FREELIST_ALLOC(animationActionTimer, UI->table->firstFreeElement, PushStruct(&UI->table->pool, EditorElement));
            animationActionTimer->type = EditorElement_Struct;
            UIAddChild(UI->table, animationActionTimer, EditorElement_Real, "time", "0.0");
            UIAddChild(UI->table, animationActionTimer, EditorElement_String, "animationName", "rig");
            
            EditorElement* playButton;
            FREELIST_ALLOC(playButton, UI->table->firstFreeElement, PushStruct(&UI->table->pool, EditorElement));
            playButton->type = EditorElement_Struct;
            UIAddChild(UI->table, playButton, EditorElement_String, "autoplay", "false");
            UIAddChild(UI->table, playButton, EditorElement_Real, "speed", "1.0");
            UIAddChild(UI->table, playButton, EditorElement_Real, "scale", "50.0");
            UIAddChild(UI->table, playButton, EditorElement_String, "showBones", "false");
            UIAddChild(UI->table, playButton, EditorElement_String, "showBitmaps", "true");
            
            
            playButton->next = animationActionTimer;
            animationRoot->next = playButton;
            animationStruct->firstValue = animationRoot;
            
            
            EditorWidget* animation = StartWidget(UI, EditorWidget_Animation, 0xffffffff, "Animation");
            animation->permanent.P = V2(-300, -300);
            animation->root = animationRoot;
            
            
            EditorWidget* soundDatabase = StartWidget(UI, EditorWidget_SoundDatabase, EditorRole_SoundDesigner, "Sound Database");
            soundDatabase->permanent.P = V2(300, 200);
            soundDatabase->root = UI->table->soundNamesRoot;
            
            EditorWidget* componentDatabase = StartWidget(UI, EditorWidget_Components, EditorRole_GameDesigner, "Visual Components");
            componentDatabase->permanent.P = V2(300, 200);
            componentDatabase->root = UI->table->componentsRoot;
            
            
            EditorWidget* actions = StartWidget(UI, EditorWidget_GeneralButtons, 0xffffffff, "Actions:");
            actions->permanent.P = V2(200, 100);
            
            
            EditorWidget* soundEvents = StartWidget(UI, EditorWidget_SoundEvents, EditorRole_SoundDesigner, "Sound Events");
            soundEvents->permanent.P = V2(-200, 100);
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
        
        TagVector matchVector = {};
        TagVector weightVector = {};
        weightVector.E[Tag_fontType] = 1.0f;
        matchVector.E[Tag_fontType] = ( r32 ) Font_default;
        UI->fontId = GetMatchingFont(group->assets, Asset_font, &matchVector, &weightVector);
        
        UI->scrollIconID = GetMatchingBitmap(group->assets, Asset_scrollUI, 0, &matchVector, &weightVector);
        
        
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
        
        
        
        
        UIAddAutocompleteFromTaxonomy(UI, "equipment");
        UIAddAutocompleteFromTaxonomy(UI, "root", "ingredient");
        UIAddAutocompleteFromTable(UI, SlotName, "slot");
        UIAddAutocompleteFromTable(UI, EntityAction, "action");
        UIAddAutocompleteFromTable(UI, SoundContainerType, "soundCType");
        UIAddAutocompleteFromFiles(UI);
        
        FormatString(UI->trueString, sizeof(UI->trueString), "true");
        FormatString(UI->falseString, sizeof(UI->falseString), "false");
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
    
    if(myPlayer->openedContainerID != UI->openedContainerID)
    {
        UI->openedContainerID = myPlayer->openedContainerID;
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
    if(UI->mode == UIMode_Equipment && overlapping->identifier == myPlayer->openedContainerID)
    {
        UIAddInteraction(UI, input, mouseLeft, UISetValueInteraction(UI_Trigger, &UI->nextMode, UIMode_Loot));
    }
    else
    {
        char entityName[64];
        GetUIName(entityName, sizeof(entityName), UI->table, overlapping);
        
        UIMarkListToUpdateAndRender(UI, possibleOverlappingActions);
        if(UI->worldMode->editingEnabled && input->altDown)
        {
            if(!IsPlant(UI->table, overlapping->taxonomy))
            {
                UIRequest editRequest = EditRequest(overlapping->taxonomy);
                UIAddPossibility(&UI->possibleOverlappingActions, "edit", entityName, editRequest);
                
                UIRequest deleteRequest = DeleteRequest(overlapping->identifier);
                UIAddPossibility(&UI->possibleOverlappingActions, "delete", entityName, deleteRequest);
                
                UIRequest impersonateRequest = ImpersonateRequest(overlapping->identifier);
                UIAddPossibility(&UI->possibleOverlappingActions, "impersonate", entityName, impersonateRequest);
                
                UIInteraction mouseInteraction = ScrollableListRequestInteraction(UI_Click, &UI->possibleOverlappingActions);
                UIAddInteraction(UI, input, mouseLeft, mouseInteraction);
            }
        }
        else
        {
            for(u32 actionIndex = Action_Attack; actionIndex < Action_Count; ++actionIndex)
            {
                if(myPlayer->overlappingPossibleActions[actionIndex])
                {
                    UIRequest actionRequest = StandardActionRequest(actionIndex, overlapping->identifier);
                    UIAddPossibility(&UI->possibleOverlappingActions, MetaTable_EntityAction[actionIndex], entityName, actionRequest);
                }
            }
            
            UIInteraction actionListInteraction = {};
            actionListInteraction.flags |= UI_MaintainWhenModeChanges;
            UIAddScrollableTargetInteraction(&actionListInteraction, &UI->possibleOverlappingActions, output);
            UIAddInvalidCondition(&actionListInteraction, u32,ColdPointerDataOffset(myPlayer->targetPossibleActions, OffsetOf(UIInteractionData, actionIndex)), Fixed((b32)false), 0);                      
            UIAddInvalidCondition(&actionListInteraction, u32,ColdPointer(&output->desiredAction), Fixed((u32)Action_Attack), UI_Ended);
            UIAddInvalidCondition(&actionListInteraction, b32,ColdPointer(&UI->movingWithKeyboard), Fixed((b32)true), 0);
            
            UIAddInteraction(UI, input, mouseLeft, actionListInteraction);
            
            if(myPlayer->overlappingPossibleActions[Action_Cast])
            {
                UIInteraction castInteraction = {};
                UIAddStandardTargetInteraction(&castInteraction, output, Action_Cast, overlapping->identifier);
                UIAddInvalidCondition(&castInteraction, u32,ColdPointer(myPlayer->targetPossibleActions + Action_Cast), Fixed(false));
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
    UIAddInteraction(UI, input, moveLeft, UISetValueInteraction(UI_Idle, &output.inputAcc.x, -1.0f), UIPriority_Standard, &UI->movementGroup);
    UIAddInteraction(UI, input, moveRight, UISetValueInteraction(UI_Idle, &output.inputAcc.x, 1.0f), UIPriority_Standard, &UI->movementGroup);
    UIAddInteraction(UI, input, moveDown, UISetValueInteraction(UI_Idle, &output.inputAcc.y, -1.0f), UIPriority_Standard, &UI->movementGroup);
    UIAddInteraction(UI, input, moveUp, UISetValueInteraction(UI_Idle, &output.inputAcc.y, 1.0f), UIPriority_Standard, &UI->movementGroup);
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
                            
                            UIInteraction mouseInteraction = SendRequestInteraction(UI_Click, EditRequest(overlapping->taxonomy));
                            UIAddInteraction(UI, input, mouseLeft, mouseInteraction);
                        }
                        else
                        {
                            PushUITooltip(UI, "inventory", V4(1, 0, 0, 1));
                            
                            UIAddInteraction(UI, input, mouseLeft, UISetValueInteraction(UI_Trigger, &UI->nextMode, UIMode_Equipment));
                            UIAddInteraction(UI, input, mouseRight, UISetValueInteraction(UI_Trigger, &UI->nextMode, UIMode_Book));
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
                    if(overlapping->identifier == myPlayer->overlappingIdentifier)
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
                UIResetListIndex(UI, possibleOverlappingActions);
                UIInteraction mouseMovement = {};
                UIAddStandardAction(&mouseMovement, UI_Click, Vec3, ColdPointer(&UI->deltaMouseP), ColdPointer(&UI->worldMouseP));
                UIAddStandardAction(&mouseMovement, UI_Idle, u32, ColdPointer(&UI->mouseMovement), Fixed(UIMouseMovement_MouseDir));
                UIAddInvalidCondition(&mouseMovement, b32, ColdPointer(&UI->movingWithKeyboard), Fixed(true), UI_Ended);
                
                if(false)
                {
                    UIAddStandardAction(&mouseMovement, UI_Click, b32, ColdPointer(&UI->reachedPosition), Fixed(false));
                    UIAddStandardAction(&mouseMovement, UI_Click | UI_Retroactive, u32, ColdPointer(&UI->mouseMovement), Fixed(UIMouseMovement_ToMouseP));
                    UIAddInvalidCondition(&mouseMovement, b32, ColdPointer(&UI->reachedPosition), Fixed(true), UI_Ended);
                }
                
                UIAddInteraction(UI, input, mouseLeft, mouseMovement);
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
            UIAddInteraction(UI, input, mouseRight, UISetValueInteraction(UI_Trigger, &UI->nextMode, UIMode_Equipment));
            
            ClientEntity* containerEntityC = GetEntityClient(worldMode, UI->openedContainerID);
            if(containerEntityC && GetFocusObject(containerEntityC))
            {
                UIHandleContainer(UI, containerEntityC, input, true);
            }
            else
            {
                UIAddInteraction(UI, input, mouseLeft, UISetValueInteraction(UI_Trigger, &UI->nextMode, UIMode_None));
            }
            
            Rect2 playerScreenBounds = ProjectOnScreenCameraAligned(UI->group, player->P, UI->player->animation.bounds);
            Rect2 containerScreenBounds = ProjectOnScreenCameraAligned(UI->group, containerEntityC->P, containerEntityC->animation.bounds);
        } break;
        
        case UIMode_Equipment:
        {
            UI->zoomLevel = 4.2f;
            
            b32 specialEquipmentMode = false;
            b32 onlyOpenedContainerAllowed = false;
            
            UIAddInteraction(UI, input, mouseRight, UISetValueInteraction(UI_Trigger, &UI->nextMode, UI->previousMode));
            
            UIResetListPossibility(UI, possibleObjectActions);
            UIMarkListToUpdateAndRender(UI, possibleObjectActions);
            
            EquipmentSlot* slots = player->equipment;       
            u32 focusSlot = UI->player->animation.output.focusSlots.slots[0];
            u64 focusEquipmentID = player->equipment[focusSlot].ID;
            
            UIAddInteraction(UI, input, mouseLeft, UISetValueInteraction(UI_Trigger, &UI->nextMode, UIMode_None));
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
                                UIRequest pickRequest = UIStandardSlotRequest(Disequip, focusSlot, pick.reference.containerID, pick.reference.objectIndex);
                                UIAddPossibility(&UI->possibleObjectActions, "disequip", equipmentName, pickRequest);
                            }
                            else
                            {
                                UIRequest dropRequest = UIStandardSlotRequest(Disequip, focusSlot, 0, 0);
                                UIAddPossibility(&UI->possibleObjectActions, "drop", equipmentName, dropRequest);
                            }
                        }
                        
                    }
                    
                    UIInteraction mouseInteraction = ScrollableListRequestInteraction(UI_Click, &UI->possibleObjectActions);
                    UIAddInteraction(UI, input, mouseLeft, mouseInteraction);
                    
                    if(UI->player->animation.output.focusSlots.slotCount)
                    {
                        if(!UI->draggingEntity.taxonomy)
                        {
                            UIRequest dragEquipmentRequest = {};
                            dragEquipmentRequest.requestCode = UIRequest_DragEquipment;
                            dragEquipmentRequest.slot = focusSlot;
                            
                            UIInteraction dragEquipmentInteraction = mouseInteraction;
                            UIAddRequestAction(&dragEquipmentInteraction, UI_KeptPressed, dragEquipmentRequest);
                            UIAddStandardAction(&dragEquipmentInteraction, UI_KeptPressed, ClientEntity, ColdPointer(&UI->draggingEntity), ColdPointer(equipmentFocus));
                            
                            UIAddInteraction(UI, input, mouseLeft, dragEquipmentInteraction);
                        }
                        else
                        {
                            ObjectReference freeSlot = HasFreeSpace(equipmentFocus);
                            if(!UI->draggingEntity.objects.objectCount && freeSlot.objectIndex >= 0)
                            {
                                UIInteraction swapInteraction = {};
                                UIRequest request = UIStandardInventoryRequest(Swap, freeSlot.containerID, freeSlot.objectIndex);
                                UIAddRequestAction(&swapInteraction, UI_Trigger, request);
                                UIAddClearAction(&swapInteraction, UI_Trigger, ColdPointer(&UI->draggingEntity), sizeof(UI->draggingEntity));
                                
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
                    if(focusSlot)
                    {
                        specialEquipmentMode = false;
                        UIRequest equipDraggingRequest;
                        equipDraggingRequest.requestCode = UIRequest_EquipDragging;
                        equipDraggingRequest.slot = focusSlot;
                        
                        UIInteraction equipDraggingInteraction = {};
                        UIAddRequestAction(&equipDraggingInteraction, UI_Trigger, equipDraggingRequest);
                        UIAddClearAction(&equipDraggingInteraction, UI_Trigger, ColdPointer(&UI->draggingEntity), sizeof(UI->draggingEntity));
                        
                        UIAddInteraction(UI, input, mouseLeft, equipDraggingInteraction);
                    }
                    else
                    {
                        UIRequest dropRequest = UIStandardInventoryRequest(Drop, 0, 0);
                        UIInteraction dropInteraction = {};
                        UIAddRequestAction(&dropInteraction, UI_Trigger, dropRequest);
                        UIAddClearAction(&dropInteraction, UI_Trigger, ColdPointer(&UI->draggingEntity), sizeof(UI->draggingEntity));
                        
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
                       overlap->identifier != myPlayer->identifier &&
                       (!onlyOpenedContainerAllowed || overlap->identifier == myPlayer->openedContainerID))
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
                        UIAddInteraction(UI, input, switchButton, UISetValueInteraction(UI_Trigger, &UI->possibleTargets.currentIndex, UI->possibleTargets.currentIndex + 1));
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
                        if(overlapping->identifier == myPlayer->overlappingIdentifier)
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
                UIAddInteraction(UI, input, mouseLeft, UISetValueInteraction(UI_Trigger, &UI->nextMode, UIMode_None));
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
    
    UIOverdrawSkillSlots(UI, alpha, input);
    
    
    WrapScrollableList(&UI->possibleTargets);
    UpdateScrollableList(UI, UI->toUpdateList, scrollOffset);
    UIRenderList(UI, UI->toRenderList);
    
    if(worldMode->editingEnabled)
    {
        UIRenderEditor(UI, input);
    }
    
    UIRenderTooltip(UI);
    
    for(u32 buttonIndex = 0; buttonIndex < MAX_BUTTON_COUNT; ++buttonIndex)
    {
        UIInteraction* interaction = UI->hotInteractions + buttonIndex;
        if(interaction->priority != UIPriority_NotValid)
        {
            PlatformButton* button = input->buttons + buttonIndex;
            if(Pressed(button))
            {
                UIResetActiveInteractions(UI, buttonIndex, interaction->excludeFromReset);
                UIDispatchInteraction(UI, interaction, UI_Trigger, input->timeToAdvance);
                UI->activeInteractions[buttonIndex] = *interaction;
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