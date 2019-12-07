#pragma once
#define GAME_FRAME_END(name) void name()
typedef GAME_FRAME_END(game_frame_end);

#define SERVER_FRAME_END(name) void name(PlatformServerMemory* memory)
typedef SERVER_FRAME_END(server_frame_end);

#if FORGIVENESS_INTERNAL
enum DebugType
{
    DebugType_frameMarker,
    DebugType_beginCodeBlock,
    DebugType_endCodeBlock,
};

struct DebugEvent
{
    u64 clock;
    char* GUID;
    char* name;
    
    u32 threadID;
    u16 coreIndex;
    u8 type;
    r32 Value_r32;
};

struct DebugTable
{
    u32 currentEventArrayIndex;
    u64 volatile eventArrayIndex_EventIndex;
    DebugEvent eventArray[2][65536 * 8];
    
    MemoryPool tempPool;
};

struct FlipTableResult
{
    u32 eventCount;
    DebugEvent* eventArray;
};

internal FlipTableResult FlipDebugTable(DebugTable* debugTable)
{
    debugTable->currentEventArrayIndex = !debugTable->currentEventArrayIndex;
    u64 arrayIndex_eventIndex = AtomicExchangeU64(&debugTable->eventArrayIndex_EventIndex, ((u64) debugTable->currentEventArrayIndex << 32));
    
    u32 eventArrayIndex = arrayIndex_eventIndex >> 32;
    Assert(eventArrayIndex <= 1);
    u32 eventCount = arrayIndex_eventIndex & 0xffffffff;
    DebugEvent* eventArray = debugTable->eventArray[eventArrayIndex];
    
    FlipTableResult result;
    result.eventCount = eventCount;
    result.eventArray = eventArray;
    
    return result;
}

extern DebugTable* globalDebugTable;
#define UniqueFileLineCounterString__(file, line, counter, name) file "|" #line "|" #counter //"|" name
#define UniqueFileLineCounterString_(file, line, counter, name) UniqueFileLineCounterString__(file, line, counter, name)
#define DEBUG_NAME(name) UniqueFileLineCounterString_(__FILE__, __LINE__, __COUNTER__, name)

#define RecordDebugEvent(typeInit, guid, nameInit)\
u64 arrayIndex_eventIndex =AtomicIncrementU64(&globalDebugTable->eventArrayIndex_EventIndex, 1);\
u32 eventIndex = (arrayIndex_eventIndex & 0xffffffff);\
Assert(eventIndex < ArrayCount(globalDebugTable->eventArray[0]));\
DebugEvent* event = globalDebugTable->eventArray[arrayIndex_eventIndex >> 32] + eventIndex;\
event->clock = __rdtsc();\
event->type = (u8) typeInit;\
event->GUID = guid;\
event->threadID = GetThreadID();\
event->name = nameInit;

#define FRAME_MARKER_(secondsElapsedInit) \
{\
    RecordDebugEvent(DebugType_frameMarker, DEBUG_NAME("frame marker"), "frame marker");\
    event->Value_r32 = secondsElapsedInit;\
}

#define BEGIN_BLOCK_(guid, name) \
{\
    RecordDebugEvent(DebugType_beginCodeBlock, guid, name);\
}

#define END_BLOCK_(guid, name) \
{\
    RecordDebugEvent(DebugType_endCodeBlock, guid, name);\
}

#define TIMED_BLOCK__(guid, name) TimedBlock timedBlock_##number(guid, name)
#define TIMED_BLOCK_(guid, name) TIMED_BLOCK__(guid, name)
#define BEGIN_BLOCK(name) BEGIN_BLOCK_(DEBUG_NAME(name), name)
#define END_BLOCK() END_BLOCK_(DEBUG_NAME("END_BLOCK"), "END_BLOCK")
#define TIMED_FUNCTION() TIMED_BLOCK_(DEBUG_NAME(__FUNCTION__), __FUNCTION__)
#define TIMED_BLOCK(name) TIMED_BLOCK_(DEBUG_NAME(name), name)
#define FRAME_MARKER(seconds) FRAME_MARKER_(seconds)


struct TimedBlock
{
    TimedBlock(char* GUID, char* name)
    {
        BEGIN_BLOCK_(GUID, name)
    }
    
    ~TimedBlock()
    {
        END_BLOCK();
    }
};

#else
#define TIMED_BLOCK(...)
#define TIMED_FUNCTION(...)
#define BEGIN_BLOCK(...)
#define END_BLOCK(...)
#define FRAME_MARKER(...)
#endif
