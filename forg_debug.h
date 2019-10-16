#pragma once
#define DEBUG_FRAME_COUNT 256

struct DebugElement
{
    char* GUID;
    char* name;
    struct DebugProfileNode* nodes[DEBUG_FRAME_COUNT];
    
    DebugElement* nextInHash;
};

struct DebugProfileNode
{
    DebugProfileNode* firstChild;
    DebugProfileNode* nextSameParent;
    
    DebugElement* element;
    
    u64 duration;
    u64 durationOfChildren;
    u64 parentRelativeClock;
    u16 threadOrdinal;
    u16 coreIndex;
    
    union
    {
        DebugProfileNode* next;
        DebugProfileNode* nextFree;
    };
};


struct DebugEntry
{
    DebugElement* element;
    
    union
    {
        DebugEntry* next;
        DebugEntry* nextFree;
    };
};

struct DebugFrame
{
    u32 frameIndex;
    
    u64 beginClock;
    u64 endClock;
    
    r32 secondsElapsed;
    
    DebugProfileNode* rootProfileNode;
    
    u32 entryCount;
    DebugEntry* firstEntry;
};

struct OpenDebugBlock
{
    u64 beginClock;
    DebugProfileNode* currentNode;
    
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
    OpenDebugBlock* currentOpenCodeBlock;
    
    union
    {
        DebugThread* next;
        DebugThread* nextFree;
    };
};

struct DebugCollationState
{
    u32 totalFrameCount;
    
    b32 paused;
    
    u32 viewingFrameOrdinal;
    u32 mostRecentFrameOrdinal;
    u32 collatingFrameOrdinal;
    u32 oldestFrameOrdinal;
    
    DebugFrame frames[DEBUG_FRAME_COUNT];
    
    DebugElement* currentRootElement;
    DebugElement* elements[1024];
    
    u32 threadCount;
    DebugThread* firstThread;
};


enum ProfilerViewType
{
    Profiler_Threads,
    Profiler_Frames,
    Profiler_TopList,
};

struct DebugState
{
    b32 profiling;
    MemoryPool debugPool;
    
    DebugThread* firstFreeThread;
    OpenDebugBlock* firstFreeBlock;
    DebugProfileNode* firstFreeProfileNode;
    DebugEntry* firstFreeEntry;
    
    ProfilerViewType profilerType;
    DebugCollationState clientState;
    DebugCollationState serverState;
    
    Vec2 frameSliderDim;
    Vec2 profilerDim;
    
    b32 showServerProfiling;
    
};

struct DebugStatistic
{
    r64 min;
    r64 max;
    r64 sum;
    r64 avg;
    u32 count;
};