#pragma once

struct DebugTable;

#define GAME_FRAME_END( name ) void name( PlatformClientMemory* memory, PlatformInput* input, GameRenderCommands* commands )
typedef GAME_FRAME_END( game_frame_end );

struct DebugID
{
    void* value[2];
};

#if FORGIVENESS_INTERNAL
enum DebugType
{
    DebugType_unknown,
    DebugType_frameMarker,
    DebugType_beginCodeBlock,
    DebugType_endCodeBlock,
    
    DebugType_beginDataBlock,
    DebugType_endDataBlock,
    
    DebugType_b32,
    DebugType_r32,
    DebugType_u32,
    DebugType_i32,
    DebugType_Vec2,
    DebugType_Vec3,
    DebugType_Vec4,
    DebugType_Rect2,
    DebugType_Rect3,
    DebugType_BitmapId,
    DebugType_SoundId,
    DebugType_FontId,
    DebugType_ContextSlot,
    
    DebugType_ThreadIntervalGraph,
    DebugType_FrameBarGraph,
    DebugType_LastFrameInfo,
    DebugType_MemoryRemaining,
    DebugType_FrameSlider,
    DebugType_TopClockList,
};

struct ContextSlot;
struct DebugEvent
{
    u64 clock;
    char* GUID;
    char* name;
    u32 threadID;
    u16 coreIndex;
    
    void* pointer;
    u8 type;
    
    // NOTE(Leonardo): when you add member to this union, check that
    //the size of that member is _less_ than the biggest member of the union,
    //otherwise go modify the network code that sends debugEvent over the network
    union
    {
        b32 Value_b32;
        i32 Value_i32;
        u32 Value_u32;
        r32 Value_r32;
        Vec2 Value_Vec2;
        Vec3 Value_Vec3;
        Vec4 Value_Vec4;
        Rect2 Value_Rect2;
        Rect3 Value_Rect3;
        BitmapId Value_BitmapId;
        SoundId Value_SoundId;
        FontId Value_FontId;
        ContextSlot* Value_ContextSlot;
        
        DebugID debugID;
        u64 overNetwork[2];
    };
};

struct SavedNameSlot
{
    u64 pointer;
    char GUID[1024];
    char name[1024];
    
    SavedNameSlot* next;
};

struct DebugTable
{
    u64 overNetworkEdit[2];
    void* pointerToIgnore;
    DebugEvent editEvent;
    u32 recordIncrement;
    
    u32 currentEventArrayIndex;
    u64 volatile eventArrayIndex_EventIndex;
    DebugEvent eventArray[2][65536 * 8];
    
    b32 serverFinished;
    u32 currentServerEventArrayIndex;
    u32 serverEventCount[2];
    DebugEvent serverEvents[2][65536];
    DebugPlatformMemoryStats serverStats;
    
    MemoryPool tempPool;
    SavedNameSlot* nameSlots[1024];
};

extern DebugTable* globalDebugTable;
extern DebugEvent* DEBUGGlobalEditEvent;

#define DEBUGValueSetEventData_( typeInit )\
inline void DEBUGValueSetEventData( DebugEvent* event, typeInit ignored, void* value )\
{\
    event->type = DebugType_##typeInit;\
    if( globalDebugTable->editEvent.GUID == event->GUID )\
    {\
        *( typeInit * ) value = globalDebugTable->editEvent.Value_##typeInit;\
    }\
    event->Value_##typeInit = *( typeInit * ) value;\
}

DEBUGValueSetEventData_( r32 );
DEBUGValueSetEventData_( u32 );
DEBUGValueSetEventData_( i32 );
DEBUGValueSetEventData_( Vec2 );
DEBUGValueSetEventData_( Vec3 );
DEBUGValueSetEventData_( Vec4 );
DEBUGValueSetEventData_( Rect2 );
DEBUGValueSetEventData_( Rect3 );
DEBUGValueSetEventData_( BitmapId );
DEBUGValueSetEventData_( SoundId );
DEBUGValueSetEventData_( FontId );


#define UniqueFileLineCounterString__( file, line, counter, name ) file "|" #line "|" #counter //"|" name
#define UniqueFileLineCounterString_( file, line, counter, name ) UniqueFileLineCounterString__( file, line, counter, name )
#define DEBUG_NAME( name ) UniqueFileLineCounterString_( __FILE__, __LINE__, __COUNTER__, name )

#define DEBUGSetEventRecording( enabled ) globalDebugTable->recordIncrement = ( enabled ) ? 1 : 0

#define RecordDebugEvent( typeInit, guid, nameInit )\
u64 arrayIndex_eventIndex =AtomicIncrementU64( &globalDebugTable->eventArrayIndex_EventIndex, globalDebugTable->recordIncrement );\
u32 eventIndex = ( arrayIndex_eventIndex & 0xffffffff );\
Assert( eventIndex < ArrayCount( globalDebugTable->eventArray[0] ) );\
DebugEvent* event = globalDebugTable->eventArray[arrayIndex_eventIndex >> 32] + eventIndex;\
event->clock = __rdtsc();\
event->type = ( u8 ) typeInit;\
event->GUID = guid;\
event->threadID = GetThreadID();\
event->name = nameInit;

struct DebugDataBlock
{
    DebugDataBlock( char* guid, char* name )
    {
        RecordDebugEvent( DebugType_beginDataBlock, guid, name );
    }
    ~DebugDataBlock()
    {
        RecordDebugEvent( DebugType_endDataBlock, DEBUG_NAME( "end data block" ), "end data block" );
    }
};

#define DEBUG_DATA_BLOCK( name ) DebugDataBlock dataBlock__##name( DEBUG_NAME( #name ), #name )

#define FRAME_MARKER( secondsElapsedInit ) \
{\
    RecordDebugEvent( DebugType_frameMarker, DEBUG_NAME( "frame marker" ), "frame marker" );\
    event->Value_r32 = secondsElapsedInit;\
}

inline void DebugEditEvent( char* GUID, DebugEvent* event );
#define DEBUG_VALUE( value ) \
{\
    RecordDebugEvent( DebugType_unknown, DEBUG_NAME( #value ), #value );\
    DEBUGValueSetEventData( event, value, &value );\
}

#define DEBUG_B32( value ) \
{\
    RecordDebugEvent( DebugType_unknown, DEBUG_NAME( #value ), #value );\
    DEBUGValueSetEventData( event, ( i32 ) 0, &value );\
    event->type = DebugType_b32;\
}

#define DEBUG_UI_ELEMENT( type, name )\
{\
    RecordDebugEvent( type, #name, #name );\
}

#define BEGIN_BLOCK_( guid, name ) \
{\
    RecordDebugEvent( DebugType_beginCodeBlock, guid, name );\
}

#define END_BLOCK_( guid, name ) \
{\
    RecordDebugEvent( DebugType_endCodeBlock, guid, name );\
}

#define BEGIN_BLOCK( name ) BEGIN_BLOCK_( DEBUG_NAME( name ), name )
#define END_BLOCK() END_BLOCK_( DEBUG_NAME( "END_BLOCK" ), "END_BLOCK" )

#define TIMED_BLOCK__( guid, name ) TimedBlock timedBlock_##number( guid, name )
#define TIMED_BLOCK_(guid, name ) TIMED_BLOCK__( guid, name )
#define TIMED_BLOCK( name ) TIMED_BLOCK_( DEBUG_NAME( name ), name )
#define TIMED_FUNCTION() TIMED_BLOCK_( DEBUG_NAME( __FUNCTION__ ), __FUNCTION__ )

struct TimedBlock
{
    TimedBlock( char* GUID, char* name )
    {
        BEGIN_BLOCK_( GUID, name )
    }
    
    ~TimedBlock()
    {
        END_BLOCK();
    }
};


struct Entity;
global_variable Entity* globalHoverEntity;
inline void SetHover( Entity* entity )
{
    globalHoverEntity = entity;
}

#define DEBUG_UI_ENABLED 1
inline DebugID DEBUG_POINTER_ID( void* ptr ) { DebugID ID = { ptr }; return ID; }
internal void DEBUG_HIT( DebugID ID );
internal b32 DEBUG_HIGHLIGHTED( DebugID ID, Vec4* color );
internal b32 DEBUG_EDITING( DebugID ID );
internal b32 DEBUG_REQUESTED( DebugID ID );
internal void DEBUG_TEXT(char* text, Vec2 screenP, Vec4 color);
internal void DEBUG_RESET_EDITING();

#else
inline DebugID DEBUG_POINTER_ID( void* ptr ) { DebugID NullID = {}; return NullID; }
#define TIMED_BLOCK( ... )
#define TIMED_FUNCTION(...)
#define BEGIN_BLOCK( ... )
#define END_BLOCK( ... )
#define DEBUG_DATA_BLOCK( ... )
#define DEBUG_B32( ... )
#define FRAME_MARKER(...)
#define DEBUG_VALUE(...)
#define DEBUG_UI_ENABLED 0
#define DEBUG_HIT( ... ) 0
#define DEBUG_HIGHLIGHTED( ... ) 0
#define DEBUG_REQUESTED( ... ) 0
#define DEBUG_EDITING( ... ) 0
#define DEBUG_TEXT( ... )
#define DEBUG_RESET_EDITING( ... )

#define SetHover( ... )
#define DEBUGSetEventRecording( ... )
#endif
