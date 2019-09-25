internal UndoRedoRecord* FreeAndGetUndoRedoRecord(EditorUIContext* context)
{
    if(context->currentCommand->next != &context->undoRedoSentinel)
    {
        UndoRedoRecord* firstInList = context->currentCommand->next;
        UndoRedoRecord* lastInList = context->undoRedoSentinel.prev;
        
        lastInList->nextFree = context->firstFreeCommand;
        context->firstFreeCommand = firstInList;
        context->currentCommand->next = &context->undoRedoSentinel;
        context->undoRedoSentinel.prev = context->currentCommand;
    }
    
    UndoRedoRecord* record;
	FREELIST_ALLOC(record, context->firstFreeCommand, PushStruct(context->pool, UndoRedoRecord));
    return record;
}

#define AddUndoRedoCopyStruct(struct, context, before, ptr, after, ID) AddUndoRedoCopy(context, sizeof(struct), before, ptr, sizeof(struct), after, ID)
internal void AddUndoRedoCopy(EditorUIContext* context, u32 sizeBefore, void* before, void* ptr, u32 sizeAfter, void* after, AssetID ID)
{
    UndoRedoRecord* record = FreeAndGetUndoRedoRecord(context);
	if(IsValid(ID))
	{
        WritebackAssetToFileSystem(context->assets, ID, WRITEBACK_PATH, true);
	}
    
    record->type = UndoRedo_Copy;
    
    Assert(sizeBefore < sizeof(record->copy.before));
    Assert(sizeAfter < sizeof(record->copy.after));
    
    record->copy.sizeBefore = sizeBefore;
    record->copy.sizeAfter = sizeAfter;
    Copy(sizeBefore, record->copy.before, before);
    Copy(sizeAfter, record->copy.after, after);
    record->copy.ptr = ptr;
    
	DLLIST_INSERT_AS_LAST(&context->undoRedoSentinel, record);
	context->currentCommand = record;
}

internal void AddUndoRedoAdd(EditorUIContext* context, ArrayCounter* counter, void* fieldPtr, void* oldPtr, void* newPtr, AssetID ID)
{
    UndoRedoRecord* record = FreeAndGetUndoRedoRecord(context);
    
    
	if(IsValid(ID))
	{
        WritebackAssetToFileSystem(context->assets, ID, WRITEBACK_PATH, true);
	}
    
    record->type = UndoRedo_Add;
    
    record->add.counter = counter;
    record->add.fieldPtr = fieldPtr;
    record->add.oldPtr = oldPtr;
    record->add.newPtr = newPtr;
    
	DLLIST_INSERT_AS_LAST(&context->undoRedoSentinel, record);
	context->currentCommand = record;
}

internal void AddUndoRedoDelete(EditorUIContext* context, ArrayCounter* counter, void* deletedElement, void* deletedElementPtr, void* lastElementPtr, u32 elementSize, AssetID ID)
{
	if(IsValid(ID))
	{
        WritebackAssetToFileSystem(context->assets, ID, WRITEBACK_PATH, true);
	}
    UndoRedoRecord* record = FreeAndGetUndoRedoRecord(context);
    
    record->type = UndoRedo_Delete;
    
    record->del.counter = counter;
    record->del.deletedElement = PushSize(context->pool, elementSize);
    Copy(elementSize, record->del.deletedElement, deletedElement); 
    record->del.deletedElementPtr = deletedElementPtr;
    record->del.lastElementPtr = lastElementPtr;
    record->del.elementSize = elementSize;
    
	DLLIST_INSERT_AS_LAST(&context->undoRedoSentinel, record);
	context->currentCommand = record;
}

internal void Undo(EditorUIContext* context)
{
	if(context->currentCommand != &context->undoRedoSentinel)
	{
        Assert(context->currentCommand->next);
        Assert(context->currentCommand->prev);
        
		UndoRedoRecord* toExec = context->currentCommand;
        
        Assert(toExec->next);
        Assert(toExec->prev);
        
		switch(toExec->type)
		{
			case UndoRedo_Copy:
			{
				Copy(toExec->copy.sizeBefore, toExec->copy.ptr, toExec->copy.before);
			} break;
            
			case UndoRedo_Add:
			{
				if(toExec->add.fieldPtr)
				{
					Copy(sizeof(void*), toExec->add.fieldPtr, &(toExec->add.oldPtr));
				}
				--*toExec->add.counter;
			} break;
			
            case UndoRedo_Delete:
			{
				void* ptr = toExec->del.deletedElementPtr;
                void* endPtr = toExec->del.lastElementPtr;
                
                for(void* dest = endPtr; dest != ptr; dest = AdvanceVoidPtrBytes(dest, -(i32)toExec->del.elementSize))
                {
                    void* source = AdvanceVoidPtrBytes(dest, -(i32)toExec->del.elementSize);
                    Copy(toExec->del.elementSize, dest, source);
                }
                
				Copy(toExec->del.elementSize, ptr, toExec->del.deletedElement);
				++*toExec->del.counter;
			} break;
		}
        
		context->currentCommand = context->currentCommand->prev;
        
        Assert(context->currentCommand->next);
        Assert(context->currentCommand->prev);
	}
}

internal void Redo(EditorUIContext* context)
{
	if(context->currentCommand->next != &context->undoRedoSentinel)
	{
        Assert(context->currentCommand->next);
        Assert(context->currentCommand->prev);
        
		UndoRedoRecord* toExec = context->currentCommand->next;
        
        Assert(toExec->next);
        Assert(toExec->prev);
        
		switch(toExec->type)
		{
			case UndoRedo_Copy:
			{
				Copy(toExec->copy.sizeAfter, toExec->copy.ptr, toExec->copy.after);
			} break;
            
            case UndoRedo_Add:
			{
				if(toExec->add.fieldPtr)
				{
					Copy(sizeof(void*), toExec->add.fieldPtr, &(toExec->add.newPtr));
				}
				++*toExec->add.counter;
			} break;
            
            case UndoRedo_Delete:
			{
				void* ptr = toExec->del.deletedElementPtr;
                void* endPtr = toExec->del.lastElementPtr;
                
                for(void* dest = ptr; dest != endPtr; dest = AdvanceVoidPtrBytes(dest, toExec->del.elementSize))
                {
                    void* source = AdvanceVoidPtrBytes(dest, toExec->del.elementSize);
                    Copy(toExec->del.elementSize, dest, source);
                }
				--*toExec->del.counter;
			} break;
		}
		context->currentCommand = toExec;
        
        Assert(context->currentCommand->next);
        Assert(context->currentCommand->prev);
	}
}



internal void EditStruct(EditorLayout* layout, String structName, void* structPtr, AssetID ID)
{
    ReservedSpace ignored = {};
    StructOperation(layout, structName, structName, 0, structPtr, FieldOperation_Edit, 0, &ignored, false, ID);
}


internal Vec2 RectPadding(r32 fontScale)
{
    Vec2 result = fontScale * V2(4, 8);
    return result;
}

internal Vec2 ButtonPadding(r32 fontScale)
{
    Vec2 result = fontScale * V2(2, 2);
    return result;
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
        Vec2 padding = RectPadding(layout->fontScale);
        textDim = AddRadius(textDim, padding);
        PushRect(layout->group, FlatTransform(z), textDim, V4(0.02f, 0.02f, 0.02f, 1.0f));
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

#define StandardTextColor() V4(1, 1, 1, 1)
#define DefaultEditorStringColor() V4(1, 0, 1, 1)
#define StandardNumberColor() V4(0, 0.5f, 1.0f, 1)
#define HotNumberColor() V4(0, 1, 1.0f, 1)
#define StandardFloatColor() V4(0.2f, 0.2f, 1.0f, 1)
#define HotFloatColor() V4(1.0f, 0.5f, 1.0f, 1)

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


internal Rect2 EditorElementName_(EditorLayout* layout, char* name, b32 colon = true)
{
    Vec4 color = V4(1, 1, 0.5f, 1);
    Rect2 result = EditorTextDraw(layout, color, EditorText_StartingSpace, name);
    
    if(colon)
    {
        EditorTextDraw(layout, color, 0, " :");
    }
    
    return result;
}

#define ShowName(layout, name)if(name){EditorElementName_(layout, name, true);}
#define ShowNameNoColon(layout, name)if(name){EditorElementName_(layout, name, false);}
#define ShowStandard(layout, color, format, ...) EditorTextDraw(layout, color, EditorText_StartingSpace, format, ##__VA_ARGS__)

internal Rect2 ShowString(EditorLayout* layout, char* name, char* string, u32 flags, Vec4 color = DefaultEditorStringColor())
{
    ShowName(layout, name);
    Rect2 result = EditorTextDraw(layout, color, flags, "%s", string);
    return result;
}

internal Rect2 ShowLabel(EditorLayout* layout, char* name, Vec4 color = DefaultEditorStringColor())
{
    Rect2 result = ShowString(layout, 0, name, EditorText_StartingSpace, color);
    return result;
}

internal b32 EditString(EditorLayout* layout, char* name, char* string, AUID ID, StringArray options, char* outputBuffer, u32 outputLength, AssetID assetID)
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
            u32 size = StrLen(string) + 1;
            Assert(size < sizeof(data->before));
            Copy(size, data->before, string);
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

internal b32 EditString(EditorLayout* layout, char* name, char* string, StringArray options, char* output, u32 outputLength, AssetID assetID)
{
    b32 result = EditString(layout, name, string, auID(string), options, output, outputLength, assetID);
    return result;
}


internal b32 Edit_AssetLabel(EditorLayout* layout, char* name, AssetLabel* label, b32 isInArray, AssetID assetID)
{
    EditorUIContext* context = layout->context;
    
    b32 result = false;
    Assert(!isInArray);
    
    AUID ID = auID(label);
    AUIDData* data = GetAUIDData(context, ID);
    Vec4 color = DefaultEditorStringColor();
    
    if(!label->name[0])
    {
        FormatString(label->name, sizeof(label->name), "null");
    }
    
    if(PointInRect(data->dim, layout->mouseP))
    {
        SetNextHotAUID(context, ID);
        color = V4(1, 1, 0, 1);
    }
    
    if(HotAUIDAndPressed(context, ID, mouseLeft))
    {
        ZeroSize(sizeof(context->keyboardBuffer), context->keyboardBuffer);
        SetInteractiveAUID(context, ID);
        
        u32 size = StrLen(label->name) + 1;
        Assert(size < sizeof(data->before));
        Copy(size, data->before, label->name);
    }
    
    char* toShow = label->name;
    
    if(IsInteractiveAUID(context, ID))
    {
        u32 appendHere = StrLen(context->keyboardBuffer);
        if(appendHere && Pressed(&context->input->backButton))
        {
            context->keyboardBuffer[--appendHere] = 0;
        }   
        for(u8 c = 0; c < 0xff; ++c)
        {
            if(context->input->isDown[c] && !context->input->wasDown[c])
            {
                if(appendHere < sizeof(layout->context->keyboardBuffer))
                {
                    context->keyboardBuffer[appendHere++] = c;
                }
            }
        }
        
        color = V4(0, 1, 0, 1);
        if(layout->context->keyboardBuffer[0])
        {
            toShow = layout->context->keyboardBuffer;
            color = V4(0, 1, 1, 1);
        }
        
        if(UIPressed(layout->context, confirmButton))
        {
            EndInteraction(layout->context);
            FormatString(label->name, sizeof(label->name), "%s", layout->context->keyboardBuffer);
            result = true;
            
            AddUndoRedoCopy(layout->context, StrLen(data->before) + 1, data->before, label->name, StrLen(label->name) + 1, label->name, assetID);
        }
    }
    
    data->dim = ShowString(layout, name, toShow, EditorText_StartingSpace, color);
    
    return result;
}

internal b32 Edit_u32(EditorLayout* layout, char* name, u32* number, b32 isInArray, AssetID assetID)
{
    b32 result = false;
    
    AUID ID = auID(number);
    AUIDData* data = GetAUIDData(layout->context, ID);
    
    Vec4 color = StandardNumberColor();
    
    if(PointInRect(data->dim, layout->mouseP))
    {
        color = HotNumberColor();
        SetNextHotAUID(layout->context, ID);
    }
    
	if(HotAUIDAndPressed(layout->context, ID, mouseLeft))
    {
		data->speed = 0;
		data->increasing = true;
        Copy(sizeof(u32), data->before, number);
        SetInteractiveAUID(layout->context, ID);
        *number = ++*number;
    }
    
	if(HotAUIDAndPressed(layout->context, ID, mouseRight))
    {
		data->speed = 0;
		data->increasing = false;
        
        Copy(sizeof(u32), data->before, number);
        SetInteractiveAUID(layout->context, ID);
        if(*number > 0)
        {
            *number = --*number;
        }
    }
    
    
    if(IsInteractiveAUID(layout->context, ID))
    {
        r32 real = (r32) *number + data->speed;
		real = Max(real, 0);
		*number = RoundReal32ToU32(real);
        
		if(data->increasing)
		{
			data->speed += 0.02f;
			if(UIReleased(layout->context, mouseLeft))
			{
                AddUndoRedoCopy(layout->context, sizeof(u32), data->before, number, sizeof(u32), number, assetID);
				EndInteraction(layout->context);
                result = true;
			}
		}
        else
        {
            data->speed -= 0.02f;
			if(UIReleased(layout->context, mouseRight))
			{
                AddUndoRedoCopy(layout->context, sizeof(u32), data->before, number, sizeof(u32), number, assetID);
				EndInteraction(layout->context);
                result = true;
			}
        }
    }
    
    
    ShowName(layout, name);
    Rect2 elementRect = ShowStandard(layout, color, "%d", *number);
    data->dim = elementRect;
    
    
    return result;
}

internal b32 Edit_u16(EditorLayout* layout, char* name, u16* number, b32 isInArray, AssetID assetID)
{
    b32 result = false;
    
    AUID ID = auID(number);
    
    Vec4 color = V4(1, 0, 0, 1);
    ShowName(layout, name);
    ShowStandard(layout, color, "%d", *number);
    
    return result;
}

internal b32 Edit_b32(EditorLayout* layout, char* name, b32* flag, b32 isInArray, AssetID assetID)
{
    b32 result = false;
    
    AUID ID = auID(flag);
    AUIDData* data = GetAUIDData(layout->context, ID);
    
    Vec4 color = *flag ? V4(0, 0.7f, 0, 1) : V4(0.7f, 0, 0, 1);
    
    if(PointInRect(data->dim, layout->mouseP))
    {
        color = *flag ? V4(0, 1, 0, 1) : V4(1, 0, 0, 1);
        SetNextHotAUID(layout->context, ID);
    }
    
    if(HotAUIDAndPressed(layout->context, ID, mouseLeft))
    {
        b32 before = *flag;
        *flag = !*flag;
        AddUndoRedoCopy(layout->context, sizeof(b32), &before, flag, sizeof(b32), flag, assetID);
        
    }
    
    ShowName(layout, name);
    Rect2 elementRect = ShowStandard(layout, color, "%s", *flag ? "true" : "false");
    data->dim = elementRect;
    
    return result;
}

internal b32 Edit_r32(EditorLayout* layout, char* name, r32* number, b32 isInArray, AssetID assetID)
{
    b32 result = false;
    
    EditorUIContext* context = layout->context;
    AUID ID = auID(number);
    AUIDData* data = GetAUIDData(context, ID);
    
    
    
    Vec4 color = StandardFloatColor();
    if(PointInRect(data->dim, layout->mouseP))
    {
        color = HotFloatColor();
        SetNextHotAUID(context, ID);
    }
    
    if(HotAUIDAndPressed(context, ID, mouseLeft))
    {
        Copy(sizeof(r32), data->before, number);
		data->coldEdit = false;
        SetInteractiveAUID(context, ID);
    }
    
	if(HotAUIDAndPressed(context, ID, mouseRight))
    {
        Copy(sizeof(r32), data->before, number);
        ZeroSize(sizeof(context->keyboardBuffer), context->keyboardBuffer);
		data->coldEdit = true;
        SetInteractiveAUID(context, ID);
    }
    
    
	b32 coldEditValid = false;
    if(IsInteractiveAUID(context, ID))
    {
		if(data->coldEdit)
		{
			u32 appendHere = StrLen(context->keyboardBuffer);
			if(appendHere && Pressed(&context->input->backButton))
			{
				context->keyboardBuffer[--appendHere] = 0;
			}   
            
			for(u8 c = 0; c < 0xff; ++c)
			{
				b32 canParsePoint = !StringContains(context->keyboardBuffer, '.');
				b32 canParseSign = (appendHere == 0);
				if(context->input->isDown[c] && !context->input->wasDown[c])
				{
					b32 append = false;
					if(c >= '0' && c <= '9')
					{
						append = true;
					}
					else if(c == '.')
					{
						if(canParsePoint)
						{
							append = true;
						}
					}
					else if(c == '-' || c == '+')
					{
						if(canParseSign)
						{
							append = true;
						}
					}
                    
					if(appendHere < sizeof(layout->context->keyboardBuffer))
					{
						context->keyboardBuffer[appendHere++] = c;
					}
				}
			}
            
			if(appendHere > 0)
			{
				coldEditValid = true;
                color = V4(0, 1.0f, 0.0f, 1.0f);
			}
            else
            {
                color = V4(0, 0.7f, 0.0f, 1.0f);
            }
            
			if(UIPressed(layout->context, confirmButton))
			{
				EndInteraction(layout->context);
				*number = StringToR32(context->keyboardBuffer);
				result = true;
				AddUndoRedoCopy(context, sizeof(r32), data->before, number, sizeof(r32), number, assetID);
			}
		}
		else
		{
			if(UIReleased(layout->context, mouseLeft))
			{
				AddUndoRedoCopy(context, sizeof(r32), data->before, number, sizeof(r32), number, assetID);
                EndInteraction(context);
			}
			else
			{	
				data->speed += 0.005f * layout->deltaMouseP.x;
				data->speed = Clamp(0.01f, data->speed, 3.0f);
				r32 delta = data->speed * layout->deltaMouseP.y;
				*number += delta;
			}
		}
    }
    
    ShowName(layout, name);
    
    Rect2 elementRect = InvertedInfinityRect2();
	if(coldEditValid)
	{
		elementRect = ShowStandard(layout, color, "%s", context->keyboardBuffer);
	}
	else
	{
		elementRect = ShowStandard(layout, color, "%f", *number);
	}
    data->dim = elementRect;
    
    return result;
}

internal b32 Edit_Vec2(EditorLayout* layout, char* name, Vec2* v, b32 isInArray, AssetID assetID)
{
    b32 result = false;
    
    AUID ID = auID(v);
    
    ShowName(layout, name);
    Edit_r32(layout, "x", &v->x, false, assetID);
    Edit_r32(layout, "y", &v->y, false, assetID);
    
    return result;
}

internal b32 Edit_Vec3(EditorLayout* layout, char* name, Vec3* v, b32 isInArray, AssetID assetID)
{
    b32 result = false;
    
    AUID ID = auID(v);
    
    ShowName(layout, name);
    Edit_r32(layout, "x", &v->x, false, assetID);
    Edit_r32(layout, "y", &v->y, false, assetID);
    Edit_r32(layout, "z", &v->z, false, assetID);
    
    return result;
}

internal b32 Edit_Vec4(EditorLayout* layout, char* name, Vec4 * v, b32 isInArray, AssetID assetID)
{
    b32 result = false;
    
    AUID ID = auID(v);
    
    ShowName(layout, name);
    Edit_r32(layout, "x", &v->x, false, assetID);
    Edit_r32(layout, "y", &v->y, false, assetID);
    Edit_r32(layout, "z", &v->z, false, assetID);
    Edit_r32(layout, "w", &v->w, false, assetID);
    
    return result;
}

internal b32 Edit_Hash64(EditorLayout* layout, char* name, Hash64* hash, char* optionsName, b32 isInArray, AssetID assetID)
{
    b32 result = false;
    ShowString(layout, "autocomplete", optionsName, EditorText_StartingSpace);
    return result;
}

internal char* GetStringFromOptions(StringArray options, u32 value)
{
    char* result = 0;
    
    if(value < options.count)
    {
        result = options.strings[value];
    }
    
    return result;
}

internal Enumerator GetValueFromOptions(StringArray options, char* value)
{
    Enumerator result = {};
    
    for(u32 stringIndex = 0; stringIndex < options.count; ++stringIndex)
    {
        if(StrEqual(options.strings[stringIndex], value))
        {
            result = stringIndex;
            break;
        }
    }
    
    
    return result;
}

internal b32 Edit_Enumerator(EditorLayout* layout, char* name, Enumerator* enumerator, StringArray options, b32 isInArray, AssetID assetID)
{
    b32 result = false;
    
    char* currentValue = GetStringFromOptions(options, *enumerator);
    char output[64];
    if(EditString(layout, name, currentValue, options, output, sizeof(output), assetID))
    {
        Enumerator oldValue = *enumerator;
        Enumerator newValue = GetValueFromOptions(options, output);
        *enumerator = newValue;
        
        AddUndoRedoCopyStruct(Enumerator, layout->context, &oldValue, enumerator, &newValue, assetID);
        result = true;
    }
    return result;
}

internal b32 Edit_GameAssetType(EditorLayout* layout, char* name, GameAssetType* type, b32 typeEditable, b32 isInArray, AssetID assetID)
{
    b32 result = false;
    
    ShowNameNoColon(layout, name);
    char output[64];
    
    char* typeString = GetAssetTypeName(type->type);
    if(!typeString)
    {
        type->type = 0;
        typeString = GetAssetTypeName(type->type);
    }
    
    StringArray typeOptions = {};
    if(typeEditable)
    {
        typeOptions = GetAssetTypeList();
    }
    
    if(EditString(layout, "type", typeString, auID(&type->type), typeOptions, output, sizeof(output), assetID))
    {
        u16 newType = GetMetaAssetType(output);
        if(newType != type->type)
        {
            GameAssetType oldStruct = *type;
            type->type = newType;
            type->subtype = 0;
            AddUndoRedoCopyStruct(GameAssetType, layout->context, &oldStruct, type, type, assetID);
            
        }
    }
    
    char* subtypeString = GetAssetSubtypeName(type->type, type->subtype);
    if(!subtypeString)
    {
        type->subtype = 0;
        subtypeString = GetAssetSubtypeName(type->type, type->subtype);
    }
    
    StringArray subtypeOptions = GetAssetSubtypeList(type->type);
    if(EditString(layout, "subtype", subtypeString, auID(&type->subtype), subtypeOptions, output, sizeof(output), assetID))
    {
        u16 newSubtype = GetMetaAssetSubtype(type->type, output);
        if(newSubtype != INVALID_ASSET_SUBTYPE)
        {
            u16 oldType = type->subtype;
            type->subtype = newSubtype;
            AddUndoRedoCopyStruct(u16, layout->context, &oldType, &type->subtype, &newSubtype, assetID);
        }
    }
    
    return result;
}

internal b32 Edit_GameProperty(EditorLayout* layout, char* name, GameProperty* property, b32 isInArray, AssetID assetID)
{
    b32 result = false;
    
    ShowNameNoColon(layout, name);
    char output[64];
    
    char* typeString = GetMetaPropertyTypeName(property->property);
    if(!typeString)
    {
        property->property = 0;
        typeString = GetMetaPropertyTypeName(property->property);
    }
    
    StringArray typeOptions = GetPropertyTypeList();
    if(EditString(layout, "type", typeString, auID(&property->property), typeOptions, output, sizeof(output), assetID))
    {
        u16 newType = GetMetaPropertyType(Tokenize(output));
        if(newType != property->property)
        {
            GameProperty oldStruct = *property;
            property->property = newType;
            property->value = 0;
            AddUndoRedoCopyStruct(GameProperty, layout->context, &oldStruct, property, property, assetID);
        }
    }
    
    char* valueString = GetMetaPropertyValueName(property->property, property->value);
    if(!valueString)
    {
        property->value = 0;
        valueString = GetMetaPropertyValueName(property->property, property->value);
    }
    
    StringArray valueOptions = GetPropertyValueList(property->property);
    if(EditString(layout, "value", valueString, auID(&property->value), valueOptions, output, sizeof(output), assetID))
    {
        u16 newValue = ExistMetaPropertyValue(property->property, Tokenize(output));
        
        if(newValue != INVALID_PROPERTY_VALUE)
        {
            u16 oldValue = property->value;
            property->value = newValue;
            
            AddUndoRedoCopyStruct(u16, layout->context, &oldValue, &property->value, &newValue, assetID);
            
        }
    }
    
    if(isInArray)
    {
        if(StandardEditorButton(layout, "canc", auID(property, "cancButton"), V4(0, 0.5f, 1.0f, 1.0f)))
        {
            result = true;
        }
    }
    
    return result;
}



internal b32 EditorButton(EditorLayout* layout, Vec2 rawOffset, Vec2 buttonDim, char* name, AUID ID, b32 disabled = false, Vec4 color = V4(1, 0, 0, 1))
{
    b32 result = false;
    Vec2 buttonMin = layout->currentP + Hadamart(rawOffset, V2(RawHeight(layout), RawHeight(layout)));
    Rect2 button = RectMinDim(buttonMin, buttonDim);
    
    if(disabled)
    {
        color.a *= 0.2f;
    }
    else
    {
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
    }
    
    layout->currentP.x += 1.2f* buttonDim.x;
    PushRect(layout->group, FlatTransform(), AddRadius(button, ButtonPadding(layout->fontScale)), color);
    if(name)
    {
        Vec4 textColor = StandardTextColor();
        if(disabled)
        {
            textColor.a *= 0.2f;
        }
        PushTextEnclosed(layout->group, layout->fontID, name, button, layout->fontScale, textColor);
    }
    
    return result;
}

internal b32 StandardEditorButton(EditorLayout* layout, char* name, AUID ID, Vec4 color = V4(1, 0, 0, 1))
{
    Vec2 buttonDim = ButtonDim(layout);
    b32 result = EditorButton(layout, V2(0.25f, -0.1f), buttonDim, name, ID);
    return result;
}

internal b32 EditorCollapsible(EditorLayout* layout, char* name, AUID ID)
{
    AUIDData* data = GetAUIDData(layout->context, ID);
    
    Vec4 collapsibleColor = data->show ? V4(0, 1, 0, 1) : V4(0, 0, 1, 1);
    Vec2 collapseDim = CollapsibleDim(layout);
    if(EditorButton(layout, V2(0, 0), collapseDim, 0, ID, false, collapsibleColor))
    {
        data->show = !data->show;
    }
    
    ShowNameNoColon(layout, name);
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
    
    AUID saveID = auID(info, "saveButton");
    
    b32 disabled = (!get.asset);
    if(EditorButton(layout, V2(0.25f, -0.1f), ButtonDim(layout), "save", saveID, disabled))
    {
        WritebackAssetToFileSystem(assets, ID, WRITEBACK_PATH, false);
    }
    
    
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
                LoadDataFile(assets, ID, true);
                Asset* asset = GetGameAsset(assets, ID).asset;
                Assert(asset);
                
                
                char* metaAssetType = metaAsset_assetType[ID.type];
                String structName = {};
                structName.ptr = metaAssetType;
                structName.length = StrLen(metaAssetType);
                
                Nest(layout);
                EditStruct(layout, structName, asset->data, ID);
                Pop(layout);
            } break;
        }
        
        
        
        
        Nest(layout);
        if(EditorCollapsible(layout, "properties", auID(info, "properties")))
        {
            Push(layout);
            for(u32 propertyIndex = 0; propertyIndex < ArrayCount(info->properties); ++propertyIndex)
            {
                PAKProperty* property = info->properties + propertyIndex;
                NextRaw(layout);
                Edit_GameProperty(layout, 0, property, false, ID);
            }
            Pop(layout);
        }
        Pop(layout);
    }
}

internal void RenderEditAssetFile(EditorLayout* layout, Assets* assets, PAKFileHeader* header)
{
    b32 showAssetData = EditorCollapsible(layout, header->name);
    u16 type = GetMetaAssetType(header->type);
    u16 subtype = GetMetaAssetSubtype(type, header->subtype);
    
    if(showAssetData)
    {
        Push(layout);
        NextRaw(layout);
        
        ShowString(layout, "type", header->type, EditorText_StartingSpace);
        ShowString(layout, "subtype", header->subtype, EditorText_StartingSpace);
        
        NextRaw(layout);
        
        ShowName(layout, "standard");
        ShowStandard(layout, StandardTextColor(), "%d", header->standardAssetCount);
        ShowName(layout, "derived");
        ShowStandard(layout, StandardTextColor(), "%d", header->derivedAssetCount);
        
        NextRaw(layout);
        
        if(EditorCollapsible(layout, "assets", auID(&header->magicValue)))
        {
            Push(layout);
            u16 totalAssetCount = header->standardAssetCount + header->derivedAssetCount;
            for(u16 assetIndex = 0; assetIndex < totalAssetCount; ++assetIndex)
            {
                NextRaw(layout);
                AssetID ID = {};
                ID.type = type;
                ID.subtype = subtype;
                ID.index = assetIndex;
                
                RenderAndEditAsset(layout, assets, ID);
            }
            Pop(layout);
        }
        
        Pop(layout);
    }
}

internal EditorLayout StandardLayout(MemoryPool* pool, FontId ID, RenderGroup* group, EditorUIContext* context, Vec2 mouseP, Vec2 deltaMouseP, Vec4 defaultColoration = V4(1, 1, 1, 1), r32 fontScale = 1.0f, r32 horizontalAdvance = 100.0f)
{
    EditorLayout result = {};
    
    
    PAKFont* font = GetFontInfo(group->assets, ID);
    result.context = context;
    
    result.defaultColoration = defaultColoration;
    
    result.bufferSize = KiloBytes(64);
    result.buffer = (char*) PushSize(pool, result.bufferSize);
    
    result.currentP = context->offset + 0.45f * V2(- (r32) group->commands->settings.width, (r32) group->commands->settings.height);
    result.rawP = result.currentP;
    result.lastP = result.rawP;
    
    result.fontID = ID;
    result.font = font;
    result.fontScale = context->fontScale;
    result.horizontalAdvance = horizontalAdvance;
    result.standardButtonDim = 0.5f * horizontalAdvance;
    
    result.mouseP = mouseP;
    result.deltaMouseP = deltaMouseP;
    result.group = group;
    
    return result;
}

internal void RenderEditor(RenderGroup* group, GameModeWorld* worldMode, Vec2 deltaMouseP)
{
    EditorUIContext* context = &worldMode->editorUI;
    Vec2 mouseP = worldMode->relativeScreenMouseP;
    if(Pressed(&context->input->editorButton))
    {
        context->showEditor = !context->showEditor;
    }
    
    if(context->showEditor)
    {
        RandomSequence seq = {};
        GameProperties properties = {};
        FontId fontID = QueryFonts(group->assets, AssetFont_debug, &seq, &properties);
        if(IsValid(fontID))
        {
            MemoryPool editorPool = {};
            SetOrthographicTransformScreenDim(group);
            
            if(Pressed(&context->input->undo))
            {
                Undo(context);
            }
            
            if(Pressed(&context->input->redo))
            {
                Redo(context);
            }
#if 0        
            if(Pressed(&context->input->actionLeft))
            {
                context->offset.x -= 10.0f;
            }
            if(Pressed(&context->input->actionRight))
            {
                context->offset.x += 10.0f;
            }
#endif
            
            if(context->input->ctrlDown)
            {
                context->fontScale += 0.008f * context->input->mouseWheelOffset;
            }
            else
            {
                context->offset.y -= 10.0f * context->input->mouseWheelOffset;
            }
            context->fontScale = Max(context->fontScale, 0.4f);
            
            context->nextHot = {};
            EditorLayout layout = StandardLayout(&editorPool, fontID, group, context, mouseP, deltaMouseP, V4(1, 1, 1, 1));
            
            EditorTabs active = context->activeTab;
            
            AUID auid = auID(context, "left");
            if(StandardEditorButton(&layout, "<", auid))
            {
                i32 currentTab = (i32) active - 1;
                if(currentTab < 0)
                {
                    context->activeTab = (EditorTabs) (EditorTab_Count - 1);
                }
                else
                {
                    context->activeTab = (EditorTabs) currentTab;
                }
            }
            
            switch(active)
            {
                case EditorTab_Assets:
                {
                    ShowLabel(&layout, "Assets");
                } break;
                
                case EditorTab_Misc:
                {
                    ShowLabel(&layout, "Misc");
                } break;
            }
            
            auid = auID(context, "right");
            if(StandardEditorButton(&layout, ">", auid))
            {
                i32 currentTab = (i32) active + 1;
                if(currentTab == EditorTab_Count)
                {
                    context->activeTab = (EditorTabs) 0;
                }
                else
                {
                    context->activeTab = (EditorTabs) currentTab;
                }
            }
            
            NextRaw(&layout);
            NextRaw(&layout);
            
            switch(active)
            {
                case EditorTab_Assets:
                {
                    for(u32 fileIndex = 0; fileIndex < group->assets->fileCount; ++fileIndex)
                    {
                        AssetFile* file = GetAssetFile(group->assets, fileIndex);
                        PAKFileHeader* header = GetFileInfo(group->assets, fileIndex);
                        
                        AssetSubtypeArray* assets = GetAssetSubtypeForFile(group->assets, header);
                        if(assets)
                        {
                            RenderEditAssetFile(&layout, group->assets, header);
                            NextRaw(&layout);
                        }
                    }
                } break;
                
                case EditorTab_Misc:
                {
                    AUID miscID = auID(context, "resetGround");
                    if(StandardEditorButton(&layout, "Regenerate Ground", miscID))
                    {
                        for(u32 chunkIndex = 0; chunkIndex < ArrayCount(worldMode->chunks); ++chunkIndex)
                        {
                            for(WorldChunk* chunk = worldMode->chunks[chunkIndex]; chunk; chunk = chunk->next)
                            {
                                chunk->initialized = false;
                                FreeSpecialTexture(group->assets, &chunk->texture);
                            }
                        }
                    }
                    
                    NextRaw(&layout);
                    Edit_Vec3(&layout, "camera offset", &worldMode->additionalCameraOffset, 0, {});
                    NextRaw(&layout);
                    Edit_b32(&layout, "tile view", &worldMode->worldTileView, 0, {});
                    NextRaw(&layout);
                    Edit_b32(&layout, "chunk view", &worldMode->worldChunkView, 0, {});
                    NextRaw(&layout);
                    Edit_u32(&layout, "chunk apron", &worldMode->chunkApron, 0, {});
                } break;
            }
            
            context->hot = context->nextHot;
            if(Pressed(&context->input->escButton))
            {
                context->interactive = {};
            }
            Clear(&editorPool);
        }
    }
}
