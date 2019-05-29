#define MAXIMUM_SKILL_SLOTS MAX_PASSIVE_SKILLS_ACTIVE

enum TextOperation
{
    TextOp_draw,
    TextOp_getSize,
};

enum UIMode
{
    UIMode_None,
    UIMode_Loot,
    UIMode_Equipment,
    UIMode_Book,
    UIMode_Combat,
};

enum UIRequestType
{
    UIRequest_None,
    UIRequest_Equip,
    UIRequest_Disequip,
    UIRequest_Pick,
    UIRequest_Drop,
    UIRequest_Swap,
    UIRequest_EquipDragging,
    UIRequest_DragEquipment,
    UIRequest_Learn,
    UIRequest_Consume,
    UIRequest_Craft,
    UIRequest_Open,
    UIRequest_Close,
    UIRequest_CustomTargetP,
    UIRequest_Edit,
    UIRequest_EditTab,
    UIRequest_AddTaxonomy,
    UIRequest_DeleteTaxonomy,
    UIRequest_ReviveTaxonomy,
    UIRequest_InstantiateTaxonomy,
    UIRequest_InstantiateTaxonomyPtr,
    UIRequest_MovePlayerInOtherRegion,
    UIRequest_DeleteEntity,
    UIRequest_ImpersonateEntity,
    UIRequest_SaveAssetFile,
    UIRequest_ReloadAssets,
    UIRequest_PatchServer,
    UIRequest_PatchCheck,
    UIRequest_SaveTaxonomyTab,
    UIRequest_RegenerateWorldChunks,
};

struct UIRequest
{
    UIRequestType requestCode;
    union
    {
        struct
        {
            u32 action;
            u64 identifier;
        };
        
        struct
        {
            u64 containerID;
        };
        
        struct
        {
            u64 containerEntityID;
            u8 objectIndex;
        };
        
        struct
        {
            u64 sourceContainerID;
            u8 sourceObjectIndex;
            
            u64 destContainerID;
            u8 destObjectIndex;
        };
        
        struct
        {
            u32 slot;
            u64 destContainerID;
            u8 destObjectIndex;
        };
        
        struct 
        {
            u32 taxonomy;
            GenerationData gen;
        };
        
        struct
        {
            u32 parentTaxonomy;
            char taxonomyName[32];
        };
        
        struct
        {
            u32 slotIndex;
        };
        
        struct
        {
            u64 IDToClose;
        };
        
        struct
        {
            u64 identifier;
        };
        
        struct
        {
            char fileName[32];
            EditorWidget* widget;
        };
        
        struct
        {
            u32 taxonomy;
            Vec3 offset;
        };
        
        
        struct
        {
            u32 taxonomy;
            Vec3* offsetPtr;
        };
        
        struct
        {
            Vec3 customTargetP;
        };
        
        u32 seed;
    };
};

struct UIScrollableElement
{
    char name[64];
    union
    {
        struct ClientEntity* entity;
        UIRequest request;
    };
};

struct UIScrollableList
{
    u32 currentIndex;
    u32 possibilityCount;
    UIScrollableElement possibilities[32];
};

struct UIOwnedIngredient
{
    b32 needToBeDrawn;
    u32 taxonomy;
    u16 quantity;
};

enum UIInteractionPriority
{
    UIPriority_NotValid,
    UIPriority_Low,
    UIPriority_Standard,
    UIPriority_High,
};

#define AddButton(group, buttonName) (group)->buttonIndexes[(group)->buttonCount++] = (OffsetOf(PlatformInput, buttonName) - OffsetOf(PlatformInput, buttons)) / sizeof(PlatformButton)
struct UIInteractionButtonGroup
{
    u32 buttonCount;
    u32 buttonIndexes[8];
};

enum UIMemoryPairFlags
{
    UI_Trigger = (1 << 1),
    UI_Idle = (1 << 2),
    UI_Release = (1 << 3),
    UI_KeptPressed = (1 << 4),
    UI_Click = (1 << 5),
    UI_Retroactive = (1 << 6),
    UI_Activated = (1 << 7),
    UI_HasTimer = (1 << 8),
};

enum UIInteractionFlags
{
    UI_Ended = (1 << 1),
    UI_MaintainWhenModeChanges = (1 << 2),
};


enum UIMemoryReferenceType
{
    UIMemory_Fixed,
    UIMemory_ColdPointer,
    UIMemory_DynamicPointerOffset,
    UIMemory_ColdPointerDynamicIndex,
    UIMemory_ColdPointerDataIndex,
    UIMemory_DataPointer,
};

struct UIMemoryReference
{
    UIMemoryReferenceType type;
    
    u32 offset;
    union
    {
        struct
        {
            EditorElementParents fixed;
        };
        
        struct
        {
            void* ptr;
        };
        
        struct
        {
            void** ptrPtr;
        };
        
        struct
        {
            void* ptr;
            u32* indexPtr;
            u32 elementSize;
        };
        
        struct
        {
            void* ptr;
            u32 elementSize;
        };
    };
};

struct UIMemoryPair
{
    UIMemoryReference source;
    UIMemoryReference dest;
    u32 size;
};

inline UIMemoryPair UIMemPair(UIMemoryReference source, UIMemoryReference dest, u32 size)
{
    UIMemoryPair result = {};
    result.source = source;
    result.dest = dest;
    result.size = size;
}

inline UIMemoryReference Fixed_(void* source, u32 size)
{
    UIMemoryReference result;
    result.type = UIMemory_Fixed;
    Assert(size <= sizeof(result.fixed));
    memcpy(&result.fixed, source, size);
    
    return result;
}

#define FixedDefinition(type)\
inline UIMemoryReference Fixed(type value){UIMemoryReference result = Fixed_(&value, sizeof(type)); return result;}

FixedDefinition(u32);
FixedDefinition(u64);
FixedDefinition(b32);


inline UIMemoryReference ColdPointer(void* pointer, u32 offset = 0)
{
    UIMemoryReference result;
    result.type = UIMemory_ColdPointer;
    result.ptr = pointer;
    result.offset = offset;
    
    return result;
}

inline UIMemoryReference DynamicPointer(void** ptrPtr, u32 offset = 0)
{
    UIMemoryReference result;
    result.type = UIMemory_DynamicPointerOffset;
    result.ptrPtr = ptrPtr;
    result.offset = offset;
    
    return result;
}

#define ColdPointerDynamicOffset(ptr, indexPtr, ...) ColdPointerDynamicOffset_(ptr, indexPtr, sizeof(ptr[0]), __VA_ARGS__)
inline UIMemoryReference ColdPointerDynamicOffset_(void* ptr, u32* indexPtr, u32 elementSize, u32 elementOffset = 0)
{
    UIMemoryReference result;
    result.type = UIMemory_ColdPointerDynamicIndex;
    
    result.ptr = ptr;
    result.indexPtr = indexPtr;
    result.elementSize = elementSize;
    result.offset = elementOffset;
    
    return result;
}


#define ColdPointerDataOffset(ptr, dataOffset) ColdPointerDataOffset_(ptr, sizeof(ptr[0]), dataOffset)
inline UIMemoryReference ColdPointerDataOffset_(void* ptr, u32 elementSize, u32 dataOffset)
{
    UIMemoryReference result;
    result.type = UIMemory_ColdPointerDataIndex;
    
    result.ptr = ptr;
    result.elementSize = elementSize;
    result.offset = dataOffset;
    
    return result;
}

struct UIInteractionData
{
    u32 actionIndex;
    u64 identifier;
    UIRequest request;
    Object object;
    char editorElementValue[32];
};

#define UIDataPointer(field) UIDataPointer_(OffsetOf(UIInteractionData, field))
inline UIMemoryReference UIDataPointer_(u32 offset)
{
    UIMemoryReference result;
    result.type = UIMemory_DataPointer;
    
    result.offset = offset;
    return result;
}

inline void* GetValue(UIMemoryReference reference, UIInteractionData* data)
{
    void* result = 0;
    switch(reference.type)
    {
        case UIMemory_Fixed:
        {
            result = &reference.fixed;
        } break;
        
        case UIMemory_ColdPointer:
        {
            result = ((u8*) reference.ptr + reference.offset);
        } break;
        
        case UIMemory_DynamicPointerOffset:
        {
            void* source = *reference.ptrPtr;
            if(source)
            {
                result = (u8*) source + reference.offset;
            }
        } break;
        
        case UIMemory_ColdPointerDynamicIndex:
        {
            void* source = reference.ptr;
            u32 index = *reference.indexPtr;
            result = (u8*) source + (index * reference.elementSize + reference.offset);
        } break;
        
        case UIMemory_ColdPointerDataIndex:
        {
            void* source = reference.ptr;
            u32 index = *(u32*)((u8*) data + reference.offset);
            result = (u8*) source + (index * reference.elementSize);
        } break;
        
        case UIMemory_DataPointer:
        {
            result = (u8*) data + reference.offset;
        } break;
    }
    
    return result;
}

inline void Copy(UIMemoryPair* pair, UIInteractionData* data)
{
    void* source = GetValue(pair->source, data);
    if(source)
    {
        void* dest = GetValue(pair->dest, data);
        memcpy(dest, source, pair->size);
    }
    
}

inline b32 Check(UIMemoryPair* pair, UIInteractionData* data)
{
    b32 result = false;
    
    void* source = GetValue(pair->source, data);
    void* dest = GetValue(pair->dest, data);
    
    if(source && dest)
    {
        result = memcmp(dest, source, pair->size);
    }
    return result;
}


enum UndoRedoCommandType
{
    UndoRedo_StringCopy,
    UndoRedo_DelayedStringCopy,
    UndoRedo_AddElement,
    UndoRedo_DeleteElement,
    UndoRedo_Paste,
    UndoRedo_EditTaxonomy,
};


struct UndoRedoCommand
{
    UndoRedoCommandType type;
    EditorWidget* widget;
    
    union
    {
        struct
        {
            char oldString[32];
            char newString[32];
            char* ptr;
            u32 maxPtrSize;
        };
        
        struct
        {
            char oldString[32];
            UIMemoryReference newDelayedString;
            char* newPtr;
            u32 newMaxPtrSize;
        };
        
        struct
        {
            EditorElement* list;
            EditorElement* added;
        };
        
        struct
        {
            EditorElement** prevNextPtr;
            EditorElement* deleted;
        };
        
        struct
        {
            EditorElement* dest;
            EditorElement oldElem;
            EditorElement newElem;
        };
        
        struct
        {
            u32 oldTaxonomy;
            u32 newTaxonomy;
            u32 oldTabIndex;
        };
    };
    
    
    union
    {
        UndoRedoCommand* next;
        UndoRedoCommand* nextFree;
    };
    UndoRedoCommand* prev;
};


inline UndoRedoCommand UndoRedoString(EditorWidget* widget, char* ptr, u32 ptrSize, char* oldString, char* newString)
{
    UndoRedoCommand result = {};
    result.widget = widget;
    result.type = UndoRedo_StringCopy;
    result.ptr = ptr;
    result.maxPtrSize = ptrSize;
    FormatString(result.oldString, sizeof(result.oldString), "%s", oldString);
    FormatString(result.newString, sizeof(result.newString), "%s", newString);
    
    return result;
}

inline UndoRedoCommand UndoRedoDelayedString(EditorWidget* widget, char* ptr, u32 ptrSize, char* oldString, UIMemoryReference newString)
{
    UndoRedoCommand result = {};
    result.widget = widget;
    result.type = UndoRedo_DelayedStringCopy;
    result.newPtr = ptr;
    result.newMaxPtrSize = ptrSize;
    FormatString(result.oldString, sizeof(result.oldString), "%s", oldString);
    result.newDelayedString = newString;
    
    return result;
}

inline UndoRedoCommand UndoRedoDelete(EditorWidget* widget, EditorElement** prevNextPtr, EditorElement* deleted)
{
    UndoRedoCommand result = {};
    result.widget = widget;
    result.type = UndoRedo_DeleteElement;
    result.prevNextPtr = prevNextPtr;
    result.deleted = deleted;
    
    return result;
}

inline UndoRedoCommand UndoRedoAdd(EditorWidget* widget, EditorElement* list, EditorElement* added)
{
    UndoRedoCommand result = {};
    result.widget = widget;
    result.type = UndoRedo_AddElement;
    result.list = list;
    result.added = added;
    
    return result;
}

inline UndoRedoCommand UndoRedoPaste(EditorWidget* widget, EditorElement* dest, EditorElement oldElem, EditorElement newElem)
{
    UndoRedoCommand result = {};
    result.widget = widget;
    result.type = UndoRedo_Paste;
    result.dest = dest;
    result.oldElem = oldElem;
    result.newElem = newElem;
    
    return result;
}

inline UndoRedoCommand UndoRedoEditTaxonomy(u32 oldTaxonomy, u32 oldTabIndex, u32 newTaxonomy)
{
    UndoRedoCommand result = {};
    result.widget = 0;
    result.type = UndoRedo_EditTaxonomy;
    result.oldTaxonomy = oldTaxonomy;
    result.newTaxonomy = newTaxonomy;
    result.oldTabIndex = oldTabIndex;
    
    return result;
}

enum UIInteractionActionType
{
    UIInteractionAction_Copy,
    UIInteractionAction_SendRequest,
    UIInteractionAction_ObjectToEntity,
    UIInteractionAction_Clear,
    UIInteractionAction_SendRequestDirectly,
    UIInteractionAction_OffsetRealEditor,
    UIInteractionAction_OffsetUnsignedEditor,
    UIInteractionAction_OffsetReal,
    UIInteractionAction_AddEmptyEditorElement,
    UIInteractionAction_PlaySound,
    UIInteractionAction_ShowLabeledBitmap,
    UIInteractionAction_ReloadElement,
    UIInteractionAction_PlaySoundEvent,
    UIInteractionAction_PlaySoundContainer,
    UIInteractionAction_EquipInAnimationWidget,
    UIInteractionAction_ReleaseDragging,
    UIInteractionAction_UndoRedoCommand,
    UIInteractionAction_Undo,
    UIInteractionAction_Redo,
    UIInteractionAction_CopyToClipboard,
    UIInteractionAction_PasteFromClipboard,
};

struct UIInteractionAction
{
    UIInteractionActionType type;
    u32 flags;
    union
    {
        UIMemoryPair copy;
        UIMemoryReference request;
        UIMemoryReference objectToEntity;
        UIMemoryPair clear;
        UIRequest directRequest;
        
        struct
        {
            UIMemoryReference value;
            UIMemoryReference offset;
            r32 speed;
        };
        
        struct
        {
            u64 soundTypeHash;
            u64 soundNameHash;
            EditorElement* params;
        };
        
        struct
        {
            u64 eventNameHash;
        };
        
        struct
        {
            u64 containerNameHash;
        };
        
        struct
        {
            u64 componentNameHash;
            u64 bitmapNameHash;
            Vec4 coloration;
        };
        
        struct
        {
            UIMemoryReference list;
            UIMemoryReference widget;
        };
        
        struct
        {
            UIMemoryReference grandParent;
            UIMemoryReference root;
        };
        
        struct
        {
            UIMemoryReference current;
            EditorElementParents parents;
        };
        
        struct
        {
            EditorWidget* clipboardWidget;
            char* clipboardBuffer;
            u32 clipboardSize;
        };
        
        UIMemoryReference toReload;
        UndoRedoCommand undoRedo;
    };
    
    r32 timer;
    r32 targetTimer;
    
    union
    {
        UIInteractionAction* next;
        UIInteractionAction* nextFree;
    };
};

struct UIInteractionCheck
{
    u32 flags;
    UIMemoryPair check;
};

struct UIInteraction
{
    u32 flags;
    UIInteractionPriority priority;
    UIInteractionButtonGroup* excludeFromReset;
    
    UIInteractionAction* firstAction;
    UIInteractionAction* lastAction;
    
    u32 checkCount;
    UIInteractionCheck checks[4];
    
    UIInteractionData data;
};

struct UIOutput
{
    b32 forceVoronoiRegeneration;
    r32 facingDirection;
    Vec3 inputAcc;
    
    b32 actionCanBeDone;
    u32 desiredAction;
    u64 targetEntityID;
    u64 overlappingEntityID;
    
    b32 animationGhostAllowed;
};

enum UIMouseMovementType
{
    UIMouseMovement_None,
    UIMouseMovement_ToMouseP,
    UIMouseMovement_MouseDir,
};

struct UISkill
{
    b32 active;
    u32 taxonomy;
};

struct UIButton
{
    b32 enabled;
    
    Rect2 bounds;
    r32 Z;
    UIInteraction interaction;
    Vec4 color;
    
    Vec2 textP;
    Vec2 textDim;
    r32 fontScale;
    char* text;
};

struct UIAutocompleteBlock
{
    u32 count;
    char names[32][32];
    
    union
    {
        UIAutocompleteBlock* next;
        UIAutocompleteBlock* nextFree;
    };
};

struct UIAutocomplete
{
    u64 hash;
    u64 additionalHash;
    UIAutocompleteBlock* firstBlock;
};



struct UIState
{
    b32 initialized;
    b32 previousFrameWasAllowedToQuit;
    
    RenderGroup* group;
    TaxonomyTable* table;
    struct ClientEntity* player;
    struct GameModeWorld* worldMode;
    struct ClientPlayer* myPlayer;
    
    r32 modeTimer;
    UIMode nextMode;
    UIMode mode;
    UIMode previousMode;
    
    UIMouseMovementType mouseMovement;
    Vec3 worldMouseP;
    Vec3 deltaMouseP;
    
    
    b32 reachedPosition;
    b32 movingWithKeyboard;
    Vec2 relativeScreenMouse;
    Vec2 screenMouseP;
    Vec2 oldScreenMouseP;
    Vec2 deltaScreenMouseP;
    
    UIInteractionButtonGroup movementGroup;
    UIInteraction hotInteractions[MAX_BUTTON_COUNT];
    UIInteraction activeInteractions[MAX_BUTTON_COUNT];
    
    UIScrollableList* toUpdateList;
    UIScrollableList* toRenderList;
    
    UIScrollableList possibleTargets;
    UIScrollableList possibleOverlappingActions;
    UIScrollableList possibleObjectActions;
    
    r32 fontScale;
    FontId fontId;
    Font* font;
    PakFont* fontInfo;
    
    BitmapId scrollIconID;
    
    
    
    
    
    Vec2 additionalCameraOffset;
    r32 zoomLevel;
    
    
    u64 lockedInventoryID1;
    Rect2 screenBoundsInventory1;
    
    u64 lockedInventoryID2;
    Rect2 screenBoundsInventory2;
    
    r32 focusContainerTimer;
    
    u64 openedContainerID;
    ClientEntity draggingEntity;
    b32 animationGhostAllowed;
    UIOwnedIngredient ingredientHashMap[64];
    
    
    r32 skillSlotTimeout;
    r32 skillSlotMaxTimeout;
    r32 skillFadeInSlotTimeout;
    
    i32 activeSkillSlotIndex;
    UISkill skills[MAXIMUM_SKILL_SLOTS];
    
    
    u32 currentElementBookIndex;
    u32 totalBookmarkCount;
    u32 bookmarkCount[UIBookmark_Count];
    UIBookmark bookmarks[128];
    UIBookmark* hotBookmark;
    
    u32 currentBookModeIndex;
    BookMode bookModes[UIBook_Count];
    
    BookElementsBlock* firstFreeElementBlock;
    MemoryPool bookElementsPool;
    
    BookReference* firstFreeReference;
    MemoryPool bookReferencePool;
    
    MemoryPool interactionPool;
    UIInteractionAction* firstFreeInteractionAction;
    
    char tooltipText[128];
    b32 prefix;
    b32 suffix;
    
    
    b32 reloadingAssets;
    b32 patchingLocalServer;
    
    EditorElement uneditableTabRoot;
    i32 editingTabIndex;
    u32 editingTaxonomy;
    
    u32 instantiatingTaxonomy;
    
    b32 showCursor;
    r32 runningCursorTimer;
    
    EditorElement* editorTaxonomyTree;
    
    b32 bufferValid;
    char realDragging[32];
    char keyboardBuffer[32];
    char showBuffer[64];
    EditorElement* active;
    EditorElementParents activeParents;
    
    EditorElement* activeLabel;
    EditorElement* activeLabelParent;
    EditorWidget* activeWidget;
    
    MemoryPool fixedWidgetsElementPool;
    EditorElement* emptyFixedElement;
    
    EditorElement* dragging;
    EditorElement* draggingParent;
    
    r32 saveWidgetLayoutTimer;
    r32 autosaveWidgetTimer;
    EditorWidget widgets[EditorWidget_Count];
    
    EditorElement* copying;
    
    r32 pastedTimeLeft;
    EditorElement* pasted;
    
    b32 hotStructThisFrame;
    Rect2 hotStructBounds;
    r32 hotStructZ;
    Vec4 hotStructColor;
    EditorElement* hotStruct;
    EditorWidget* hotWidget;
    r32 fakeDistanceFromPlayer;
    
    char trueString[32];
    char falseString[32];
    
    i32 currentAutocompleteSelectedIndex;
    u32 autocompleteCount;
    UIAutocomplete autocompletes[64];
    MemoryPool autocompletePool;
    UIAutocompleteBlock* firstFreeAutocompleteBlock;
    
    
    u32 editorUnionLayoutCount;
    EditorUnionLayout unionLayouts[64];
    
    
    
    MemoryPool undoRedoPool;
    UndoRedoCommand* firstFreeUndoRedoCommand;
    UndoRedoCommand undoRedoSentinel;
    UndoRedoCommand* current;
    b32 canRedo;
    
    ClientEntity fakeEquipment[Slot_Count];
    
    r32 backspacePressedTime;
    
    b32 showGroundOutline;
    b32 uniformGroundColors;
    GroundViewMode groundViewMode;
    Vec3 cameraOffset;
    u32 chunkApron;
    
    UIOutput output;
};