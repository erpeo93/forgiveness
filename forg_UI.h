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
    UIRequest_Edit,
    UIRequest_EditTab,
    UIRequest_AddTaxonomy,
    UIRequest_DeleteTaxonomy,
    UIRequest_InstantiateTaxonomy,
    UIRequest_DeleteEntity,
    UIRequest_ImpersonateEntity,
    UIRequest_SaveAssetFile,
    UIRequest_ReloadAssets,
    UIRequest_PatchServer,
    UIRequest_SaveTaxonomyTab,
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
            u64 recipeIndex;
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
            EditorElement* root;
        };
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
    UIScrollableElement possibilities[16];
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
            u64 fixed;
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

enum UIInteractionActionType
{
    UIInteractionAction_Copy,
    UIInteractionAction_SendRequest,
    UIInteractionAction_ObjectToEntity,
    UIInteractionAction_Clear,
    UIInteractionAction_SendRequestDirectly,
    UIInteractionAction_OffsetV2,
    UIInteractionAction_AddEmptyEditorElement,
    UIInteractionAction_PlaySound,
    UIInteractionAction_ReloadElement,
    UIInteractionAction_PlaySoundEvent,
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
        };
        
        struct
        {
            u64 eventNameHash;
        };
        
        UIMemoryReference toReload;
    };
    
    r32 timer;
    r32 targetTimer;
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
    
    u32 actionCount;
    UIInteractionAction actions[8];
    
    u32 checkCount;
    UIInteractionCheck checks[4];
    
    UIInteractionData data;
};

struct UIOutput
{
    b32 forceVoronoiRegeneration;
    r32 facingDirection;
    Vec3 inputAcc;
    
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
    UIAutocompleteBlock* firstBlock;
};

struct UIState
{
    b32 initialized;
    
    RenderGroup* group;
    TaxonomyTable* table;
    struct ClientEntity* player;
    struct GameModeWorld* worldMode;
    
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
    
    
    char tooltipText[128];
    b32 prefix;
    b32 suffix;
    
    u32 chunkApron;
    
    
    b32 reloadingAssets;
    
    EditorElement uneditableTabRoot;
    i32 editingTabIndex;
    u32 editingTaxonomy;
    
    b32 showCursor;
    r32 runningCursorTimer;
    
    EditorElement* editorTaxonomyTree;
    
    b32 bufferValid;
    char keyboardBuffer[32];
    char showBuffer[64];
    EditorElement* active;
    EditorElement* activeParent;
    EditorElement* activeGrandParent;
    
    EditorElement* activeLabel;
    
    r32 saveWidgetLayoutTimer;
    u32 editorRoles;
    EditorWidget widgets[EditorWidget_Count];
    
    
    b32 hotStructThisFrame;
    EditorElement* copying;
    
    Rect2 hotStructBounds;
    r32 hotStructZ;
    Vec4 hotStructColor;
    
    char trueString[32];
    char falseString[32];
    
    i32 currentAutocompleteSelectedIndex;
    u32 autocompleteCount;
    UIAutocomplete autocompletes[64];
    
    EditorElement* soundNamesRoot;
    EditorElement* soundEventsRoot;
    
    MemoryPool autocompletePool;
    UIAutocompleteBlock* firstFreeAutocompleteBlock;
    
};