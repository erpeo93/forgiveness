#pragma once
#define DEBUG_FRAME_COUNT 256

extern DebugTable* debugTable;
struct DebugVariable;
struct DebugTree;

struct DebugEntryBlock
{
    Vec2 dim;
};

struct DebugViewProfileGraph
{
    DebugEntryBlock block;
    char* GUID;
};

struct DebugEntryCollapsible
{
    b32 expandedAltView;
    b32 expandedAlways;
};

enum DebugViewType
{
    DebugView_unknown,
    DebugView_basic,
    DebugView_inlineBlock,
    DebugView_collapsible,
};

struct DebugView
{
    DebugViewType type;
    
    DebugID ID;
    DebugView* nextInHash;
    union
    {
        DebugEntryBlock block;
        DebugViewProfileGraph profileGraph;
        DebugEntryCollapsible collapsible;
    };
};

struct DebugProfileNode
{
    struct DebugElement* element;
    struct DebugStoredEvent* firstChild;
    struct DebugStoredEvent* nextSameParent;
    u64 duration;
    u64 durationOfChildren;
    u64 parentRelativeClock;
    u32 reserved;
    u16 threadOrdinal;
    u16 coreIndex;
};

struct DebugStoredEvent
{
    u32 frameIndex;
    
    union
    {
        DebugEvent event;
        DebugProfileNode profileNode;
    };
    
    union
    {
        DebugStoredEvent* next;
        DebugStoredEvent* nextFree;
    };
};

struct DebugElementFrame
{
    DebugStoredEvent* oldestEvent;
    DebugStoredEvent* mostRecentEvent;
};

struct DebugElement
{
    char* originalGUID; // NOTE(Leonardo): can't be printed! might point into unloaded DLL!
    char* GUID;
    char* name;
    
    u32 fileNameCount;
    u32 lineNumber;
    
    DebugType type;
    
    b32 valueWasEdited;
    
    DebugElementFrame frames[DEBUG_FRAME_COUNT];
    DebugElement* nextInHash;
};

inline char* GetName(DebugElement* element) 
{
    char* result = element->name;
    return result;
}

struct DebugEventLink
{
    DebugEventLink* next;
    DebugEventLink* prev;
    
    DebugEventLink* firstChild;
    DebugEventLink* lastChild;
    
    char* name;
    DebugElement* element;
};

inline DebugEventLink* GetSentinel(DebugEventLink* from)
{
    DebugEventLink* result = ( DebugEventLink* ) ( &from->firstChild );
    return result;
}

inline b32 HasChildren(DebugEventLink* link)
{
    b32 result = ( link->firstChild != GetSentinel( link ) );
    return result;
}

struct DebugTree
{
    DebugEventLink* root;
    Vec2 P;
    
    DebugTree* next;
    DebugTree* prev;
};

struct DebugCounterState
{
    char* filename;
    char* blockName;
    u32 lineNumber;
};

struct DebugFrame
{
    u32 frameIndex;
    
    u64 beginClock;
    u64 endClock;
    
    r32 secondsElapsed;
    r32 frameBarScale;
    
    u32 storedEventCount;
    u32 profileBlockCount;
    u32 dataBlockCount;
    
    DebugStoredEvent* rootProfileNode;
};

struct OpenDebugBlock
{
    u32 startingFrameIndex;
    DebugElement* element;
    
    u64 beginClock;
    DebugStoredEvent* node;
    DebugEventLink* group;
    
    union
    {
        OpenDebugBlock* parent;
        OpenDebugBlock* nextFree;
    };
};

struct DebugThread
{
    u32 laneIndex;
    u32 ID;
    OpenDebugBlock* firstOpenCodeBlock;
    OpenDebugBlock* firstOpenDataBlock;
    
    union
    {
        DebugThread* next;
        DebugThread* nextFree;
    };
};

#include "forg_debug_ui.h"

struct DebugCollationState
{
    b32 paused;
    u32 totalFrameCount;
    u32 viewingFrameOrdinal;
    u32 mostRecentFrameOrdinal;
    u32 collatingFrameOrdinal;
    u32 oldestFrameOrdinal;
    DebugFrame frames[DEBUG_FRAME_COUNT];
    
    DebugElement* rootProfileElement;
    u32 frameBarLaneCount;
    DebugThread* firstThread;
    
    DebugEventLink* rootGroup;
    DebugEventLink* profileGroup;
    DebugTree sentinelTree;
    
    DebugView* viewHash[4096];
    DebugElement* elements[1024];
    
    DebugInteraction interaction;
    DebugInteraction hotInteraction;
    DebugInteraction nextHotInteraction;
    
    u32 countSelectedIDs;
    DebugID selectedID[32];
    DebugID editingID;
    
    char* rootInfo;
    
    DebugPlatformMemoryStats memStats;
};

struct DebugState
{
    b32 ALTUI;
    
    Vec2 mousePOffsetFromCenter;
    
    MemoryPool debugPool;
    MemoryPool perFramePool;
    
    Vec2 layoutOffset;
    
    Vec2 lastMouseP;
    Layout mouseTextLayout;
    
    DebugThread* firstFreeThread;
    OpenDebugBlock* firstFreeBlock;
    DebugStoredEvent* firstFreeStoredEvent;
    
    DebugCollationState clientState;
};

struct DebugStatistic
{
    r64 min;
    r64 max;
    r64 sum;
    r64 avg;
    u32 count;
};

inline b32 DebugIDAreEqual( DebugID A, DebugID B )
{
    b32 result = ( A.value[0] == B.value[0] ) && ( A.value[1] == B.value[1] );
    return result;
}

inline b32 IsNull( DebugID ID )
{
    b32 result = ( ID.value[0] == 0 ) && ( ID.value[1] == 0 );
    return result;
}

enum DebugElementOp
{
    DebugElementOp_AddToGroup = ( 1 << 1 ),
    DebugElementOp_CreateHierarchy = ( 1 << 2 ),
};
