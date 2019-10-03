#pragma once
struct AUID
{
    void* p1;
    void* p2;
    void* p3;
};

inline AUID auID(void* p1)
{
    AUID result = {};
    result.p1 = p1;
    
    return result;
}

inline AUID auID(void* p1, void* p2)
{
    AUID result = {};
    result.p1 = p1;
    result.p2 = p2;
    
    return result;
}

inline AUID auID(void* p1, void* p2, void* p3)
{
    AUID result = {};
    result.p1 = p1;
    result.p2 = p2;
    result.p3 = p3;
    
    return result;
}

#define MAX_UNDOREDO_SIZE 32
struct AUIDData
{
    AUID ID;
    
    Rect2 dim;
    char before[MAX_UNDOREDO_SIZE];
    
    union
    {
        b32 show;
        b32 active;
        u32 optionIndex;
        
        struct
        {
            r32 height;
            r32 speed;
            r32 time;
            Enumerator skin;
        };
        struct
        {
            b32 coldEdit;
            r32 speed;
            b32 increasing;
        };
    };
    
    AUIDData* next;
};

struct AUIDStorage
{
    AUIDData* data[1024];
};

internal b32 AreEqual(AUID i1, AUID i2)
{
    b32 result = (i1.p1 == i2.p1 && 
                  i1.p2 == i2.p2 &&
                  i1.p3 == i2.p3);
    return result;
}

enum UndoRedoType
{
    UndoRedo_Copy,
    UndoRedo_Add,
    UndoRedo_Delete
};


struct UndoRedoCopy
{
    u32 sizeBefore;
    char before[MAX_UNDOREDO_SIZE];
    u32 sizeAfter;
    char after[MAX_UNDOREDO_SIZE];
    void* ptr;
};

struct UndoRedoAdd
{
    ArrayCounter* counter;
    void* fieldPtr;
    void* oldPtr;
    void* newPtr;
};

struct UndoRedoDelete
{
    ArrayCounter* counter;
    void* deletedElement;
    void* deletedElementPtr;
    void* lastElementPtr;
    u32 elementSize;
};

struct UndoRedoRecord
{
    UndoRedoType type;
    
    union
    {
        UndoRedoCopy copy;
        UndoRedoAdd add;
        UndoRedoDelete del;
    };
	
	union
	{
		UndoRedoRecord* next;
		UndoRedoRecord* nextFree;
	};
	UndoRedoRecord* prev;
};

enum EditorTabs
{
    EditorTab_Assets,
    EditorTab_Misc,
    
    EditorTab_Count,
};

struct EditorUIContext
{
    AUID nextHot;
    AUID hot;
    AUID interactive;
    AUIDStorage storage;
    PlatformInput* input;
    MemoryPool* pool;
    
    Assets* assets;
    struct SoundState* soundState;
    struct PlayingSound* playingSound;
    
    UniversePos playerP;
    
    r32 fontScale;
    Vec2 offset;
    
    b32 renderEntityBounds;
    b32 showEditor;
    EditorTabs activeTab;
    
    char keyboardBuffer[32];
    
    UndoRedoRecord* firstFreeCommand;
    UndoRedoRecord undoRedoSentinel;
    UndoRedoRecord* currentCommand;
};

struct EditorLayout
{
    EditorUIContext* context;
    Vec4 defaultColoration;
    
    char* buffer;
    u32 bufferSize;
    
    Vec2 currentP;
    Vec2 lastP;
    Vec2 rawP;
    
    FontId fontID;
    PAKFont* font;
    r32 fontScale;
    r32 horizontalAdvance;
    r32 standardButtonDim;
    
    struct RenderGroup* group;
    
    Vec2 mouseP;
    Vec2 deltaMouseP;
};


enum EditorTextFlags
{
    EditorText_StartingSpace = (1 << 0),
    EditorText_OnTop = (1 << 1),
    EditorText_DarkBackground = (1 << 2)
};

#define EditorRole_Everyone 0xffffffff
printFlags(noPrefix) enum EditorRole
{
    EditorRole_SoundDesigner = (1 << 1),
    EditorRole_Composer = (1 << 2),
    EditorRole_GameDesigner = (1 << 3),
    EditorRole_Writer = (1 << 4),
    EditorRole_Animator = (1 << 5),
    EditorRole_Artist = (1 << 6),
    EditorRole_WebDeveloper = (1 << 7),
    EditorRole_3DModeller = (1 << 8),
};