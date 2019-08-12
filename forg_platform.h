#pragma once

#define introspection(...)
#define printTable(...)
#define printFlags(...)

#include <float.h>
#include <limits.h>
#include <stdio.h>

#if !defined(internal)
#define internal static
#endif
#define global_variable static
#define local_persist static

#define MAX_IMAGE_DIM 512

#if _MSC_VER
inline u32 AtomicCompareExchangeUint32( u32* toCheck, u32 newValue, u32 oldValue )
{
    u32 result = InterlockedCompareExchange( ( LONG volatile* ) toCheck, newValue, oldValue );
    return result;
}

inline void BusyWait( u32* lock )
{
    while( AtomicCompareExchangeUint32( lock, 1, 0 ) == 1 ){}
}

inline u32 AtomicIncrementU32( u32 volatile* value, u32 addend )
{
    // NOTE( Leonardo ): return the value that was there _Before_ the add!
    u32 result = ( InterlockedExchangeAdd( ( long* ) value, addend ) );
    return result;
}

inline u64 AtomicIncrementU64( u64 volatile* value, u64 addend )
{
    // NOTE( Leonardo ): return the value that was there _Before_ the add!
    u64 result = ( InterlockedExchangeAdd64( ( __int64* ) value, addend ) );
    return result;
}

inline u64 AtomicExchangeU64( u64 volatile* value, u64 changeTo  )
{
    u64 result = InterlockedExchange64( ( __int64* ) value, changeTo );
    return result;
}

inline u32 GetThreadID()
{
    u8* threadLocalStorage = ( u8* ) __readgsqword( 0x30 );
    u32 threadID = *( u32* ) ( threadLocalStorage + 0x48 );
    return threadID;
}

struct TicketMutex
{
    u64 volatile ticket;
    u64 volatile serving;
};

inline void BeginTicketMutex(TicketMutex* mutex)
{
    u64 ticket = AtomicIncrementU64(&mutex->ticket, 1);
    while(ticket != mutex->serving);{_mm_pause();}
}

inline void EndTicketMutex(TicketMutex* mutex)
{
    AtomicIncrementU64(&mutex->serving, 1);
}

#define CompletePastWritesBeforeFutureWrites _WriteBarrier(); _mm_sfence(); 
#define CompletePastReadsBeforeFutureReads _ReadBarrier(); 
#elif COMPILER_LLVM
// TODO( Leonardo ): intrinsics for other compilers!
#else
#endif


#define U32FromPointer( pointer ) ( ( u32 ) ( memory_index ) ( pointer ) )
#define PointerFromU32( type, value ) ( type* ) ( ( memory_index ) value )

#define OffsetOf( type, name ) ( u32 ) ( &( ( type* ) 0 )->name )

#pragma pack( push, 1 )
struct BitmapId
{
    u32 value;
    Vec4 coloration;
};

struct FontId
{
    u32 value;
};

struct AnimationId
{
    u32 value;
};

struct SoundId
{
    u32 value;
};

struct ModelId
{
    u32 value;
};

#pragma pack( pop )

introspection("hello param") struct Rect2
{
    Vec2 min;
    Vec2 max;
};

introspection("hello param") struct Rect3
{
    Vec3 min;
    Vec3 max;
};

struct m4x4
{
    // NOTE(Leonardo): stored as row-major! E[ROW][COLUMS] gives E[ROW][COLUMN]
    r32 E[4][4];
};

#define PI32 3.14f
#define TAU32 6.28f

#define KiloBytes( value ) ( ( value ) * 1024 )
#define MegaBytes( value ) ( ( value ) * 1024 * 1024 )
#define GigaBytes( value ) ( ( value ) * 1024 * 1024 * 1024 )
#define TeraBytes( value ) ( ( value ) * 1024 * 1024 * 1024 * 1024 )

#define Kilo(value) KiloBytes(value)
#define Mega(value) MegaBytes(value)
#define Giga(value) GigaBytes(value)
#define Tera(value) teraBytes(value)


#define Max( A , B ) ( ( ( A ) > ( B ) ) ? ( A ) : ( B ) )
#define Min( A , B ) ( ( ( A ) < ( B ) ) ? ( A ) : ( B ) )

#define ArrayCount( array ) ( sizeof( array ) / sizeof( ( array )[0] ) )

#define AlignPow2( value, n )  ( ( ( value )  + ( ( n ) - 1 ) ) & ~( ( n ) - 1 ) )  
#define Align16( value ) AlignPow2( value, 16 )
#define Align8( value ) AlignPow2( value, 8 )
#define Align4( value ) AlignPow2( value, 4 )

#define R32_MAX FLT_MAX
#define R32_MIN -FLT_MAX
#define I32_MAX INT_MAX 
#define I32_MIN INT_MIN
#define U32_MAX UINT_MAX
#define U16_MAX 0xffff
#define I16_MAX SHRT_MAX
#define I16_MIN SHRT_MIN

//
//
//
#define DLLIST_INSERT( sentinel, element ) \
(element)->next = (sentinel)->next; \
(element)->prev = (sentinel); \
(element)->prev->next = (element); \
(element)->next->prev = (element);

#define DLLIST_REMOVE(element) \
if((element)->next)\
{\
    (element)->prev->next = (element)->next; \
    (element)->next->prev = (element)->prev;\
    (element)->next = 0;\
    (element)->prev = 0;\
}

#define DLLIST_ISEMPTY(sentinel) ((sentinel)->next == (sentinel))

#define DLLIST_INSERT_AS_LAST( sentinel, element ) \
(element)->next = (sentinel); \
(element)->prev = (sentinel)->prev; \
(element)->prev->next = (element); \
(element)->next->prev = (element);


#define DLLIST_INIT( sentinel ) \
(sentinel)->next = ( sentinel );\
(sentinel)->prev = ( sentinel );

#define FREELIST_ALLOC( result, firstFreePtr, allocationCode ) if( ( result ) = ( firstFreePtr ) ) { ( firstFreePtr ) = ( result )->nextFree; } else{ ( result ) = allocationCode; } Assert( ( result ) != ( firstFreePtr ) ); 
#define FREELIST_DEALLOC( result, firstFreePtr ) Assert( ( result ) != ( firstFreePtr ) ); if( ( result ) ) { ( result )->nextFree = ( firstFreePtr ); ( firstFreePtr ) = ( result ); }
#define FREELIST_INSERT( newFirst, firstPtr ) Assert( ( firstPtr ) != ( newFirst ) ); ( newFirst )->next = (firstPtr); (firstPtr) = (newFirst);

#define FREELIST_INSERT_AS_LAST(newLast, firstPtr, lastPtr) \
if(lastPtr){(lastPtr)->next = newLast; lastPtr = newLast;} else{ (firstPtr) = (lastPtr) = dest; }


#define FREELIST_FREE( listPtr, type, firstFreePtr ) for( type* element = ( listPtr ); element; ) { Assert( element != element->next ); type* nextElement = element->next; FREELIST_DEALLOC( element, firstFreePtr ); element = nextElement; } ( listPtr ) = 0;
#define FREELIST_COPY( destList, type, toCopy, firstFree, pool, ... ){ type* currentElement_ = ( toCopy ); while( currentElement_ ) { type* elementToCopy_; FREELIST_ALLOC( elementToCopy_, ( firstFree ), PushStruct( ( pool ), type ) ); ##__VA_ARGS__; *elementToCopy_ = *currentElement_; FREELIST_INSERT( elementToCopy_, ( destList ) ); currentElement_ = currentElement_->next; } }
//
//
//

#define ZeroStruct(s) ZeroSize( sizeof(s), &(s))
#define ZeroArray(s, count) ZeroSize(count * sizeof(s), &(s))
inline void ZeroSize(u64 size, void* memory)
{
    u8* dest = (u8*) memory;
    while(size--)
    {
        *dest++ = 0;
    }
}

inline void* Copy(u64 size, void* destPointer, void* sourcePointer)
{
    u8* dest = (u8*) destPointer;
    u8* source = (u8*) sourcePointer;
    while(size--)
    {
        *dest++ = *source++;
    }
    return destPointer;
}




struct TexturedVertex
{
    Vec4 P;
    Vec3 N;
    Vec2 UV;
    u32 color;
    
    u16 lightStartingIndex;
    u16 lightEndingIndex;
    
    r32 modulationPercentage;
    u16 textureIndex;
};


struct HardwareRenderingSettings
{
    u32 renderWidth;
    u32 renderHeight;
};

struct GameRenderSettings
{
    u32 depthPeelCount;
    b32 multisamplingHint;
    u32 width;
    u32 height;
};

#define MAX_LIGHTS 1024
struct GameRenderCommands
{
    GameRenderSettings settings;
    
    Vec4 clearColor;
    
    u32 maxVertexCount;
    u32 vertexCount;
    TexturedVertex* vertexArray;
    
    u32 maxIndexCount;
    u32 indexCount;
    u16* indexArray;
    
    u32 maxBufferSize;
    u32 usedSize;
    u8* pushMemory;
    
    u32 bufferElementCount;
    
    u16 lightCount;
    Vec4 lightSource0[MAX_LIGHTS];
    Vec4 lightSource1[MAX_LIGHTS];
};

inline GameRenderCommands DefaultRenderCommands(u8* pushBuffer, u32 pushBufferSize, u32 width, u32 height, u32 maxVertexCount, u32 maxIndexCount,  TexturedVertex* vertexArray, u16* indexArray, Vec4 clearColor)
{
    GameRenderCommands result = {};
    result.settings.width = width;
    result.settings.height = height;
    result.settings.depthPeelCount = 4;
    result.settings.multisamplingHint = false;
    
    result.clearColor = clearColor;
    result.maxVertexCount = maxVertexCount;
    result.vertexArray = vertexArray;
    
    result.maxIndexCount = maxIndexCount;
    result.indexArray = indexArray;
    
    
    result.maxBufferSize = pushBufferSize;
    result.pushMemory = pushBuffer;
    
    return result;
}

inline b32 AreEqual(GameRenderSettings* A, GameRenderSettings* B)
{
    b32 result = 
        ( A->depthPeelCount == B->depthPeelCount &&
         A->multisamplingHint == B->multisamplingHint &&
         A->width == B->width &&
         A->height == B->height );
    
    return result;
}

struct PlatformSoundBuffer
{
    i16* samples;
    u32 sampleCount;
    i32 samplesPerSecond;
    i32 bytesPerSample;
};

struct PlatformButton
{
    b32 changedState[32];
    b32 endedDown[32];
};


#define MAX_BUTTON_COUNT 64

struct PlatformInput
{
    char* serverEXE;
    
    NetworkInterface* network;
    r32 timeToAdvance;
    
    r32 mouseX;
    r32 mouseY;
    
    r32 normalizedMouseX;
    r32 normalizedMouseY;
    
    r32 relativeMouseX;
    r32 relativeMouseY;
    
    i32 mouseWheelOffset;
    
    b32 isDown[0xff];
    b32 wasDown[0xff];
    b32 altDown, shiftDown, ctrlDown;
    union
    {
        PlatformButton buttons[MAX_BUTTON_COUNT];
        struct
        {
            PlatformButton moveUp;
            PlatformButton moveDown;
            PlatformButton moveRight;
            PlatformButton moveLeft;
            
            PlatformButton actionUp;
            PlatformButton actionDown;
            PlatformButton actionRight;
            PlatformButton actionLeft;
            
            PlatformButton escButton;
            PlatformButton exitButton;
            PlatformButton confirmButton;
            PlatformButton backButton;
            
            PlatformButton switchButton;
            PlatformButton inventoryButton;
            PlatformButton equipmentButton;
            
            PlatformButton mouseLeft;
            PlatformButton mouseRight;
            PlatformButton mouseCenter;
            
            PlatformButton editorButton;
            PlatformButton copyButton;
            PlatformButton pasteButton;
            PlatformButton pauseButton;
            PlatformButton undo;
            PlatformButton redo;
            
            
            PlatformButton slotButtons[10];
#if FORGIVENESS_INTERNAL
            PlatformButton debugButton1;
            PlatformButton debugButton2;
            PlatformButton debugCancButton;
            PlatformButton reloadButton;
#endif
        };
    };
    
    // NOTE(Leonardo): signals back to the platform layer
    b32 quitRequested;
    b32 allowedToQuit;
};

inline b32 IsDown( PlatformButton* button )
{
    b32 result = button->endedDown[0];
    return result;
}

inline b32 Pressed(PlatformButton* button, u32 frameIndex = 0)
{
    b32 result = button->endedDown[frameIndex] && button->changedState[frameIndex];
    return result;
}

inline b32 Released(PlatformButton* button, u32 frameIndex = 0)
{
    b32 result = !button->endedDown[frameIndex] && button->changedState[frameIndex];
    return result;
}

inline b32 Clicked(PlatformButton* button, u32 framesToLookBack)
{
    Assert(framesToLookBack < ArrayCount(button->endedDown) - 1);
    b32 result = false;
    if(Released(button, 0))
    {
        for(u32 frameIndex = 1; frameIndex < framesToLookBack; ++frameIndex)
        {
            if(Pressed(button, frameIndex))
            {
                result = true;
                break;
            }
        }
    }
    return result;
}

inline b32 DoubleClicked(PlatformButton* button, u32 framesToLookBack)
{
    Assert(framesToLookBack < ArrayCount(button->endedDown) - 1);
    b32 result = false;
    if(Released(button, 0))
    {
        u32 clickCount = 0;
        for(u32 frameIndex = 1; frameIndex < framesToLookBack; ++frameIndex)
        {
            if(Pressed(button, frameIndex))
            {
                if(++clickCount == 2)
                {
                    result = true;
                    break;
                }
            }
        }
    }
    return result;
}

inline b32 KeptDown(PlatformButton* button, u32 frameCount)
{
    frameCount = Min(frameCount, ArrayCount(button->endedDown));
    b32 result = true;
    for(u32 frameIndex = 0; frameIndex < frameCount; ++frameIndex)
    {
        if(!button->endedDown[frameIndex])
        {
            result = false;
            break;
        }
    }
    return result;
}

// NOTE(leonardo): multithreading
#define PLATFORM_WORK_CALLBACK( name ) void name( void* param )
typedef PLATFORM_WORK_CALLBACK( platform_work_callback );

struct PlatformWorkQueueEntry
{
    void* param;
    platform_work_callback* callback;
};

struct PlatformWorkQueue;

#define COMPLETE_QUEUE_WORK( name ) void name( PlatformWorkQueue* queue )
typedef COMPLETE_QUEUE_WORK( platform_complete_queue_work );

#define PUSH_QUEUE_WORK( name ) void name( PlatformWorkQueue* queue, platform_work_callback* callback, void* param )
typedef PUSH_QUEUE_WORK( platform_push_work );


struct PlatformTextureOpQueue
{
    TicketMutex mutex;
    struct TextureOp* first;
    struct TextureOp* last;
    struct TextureOp* firstFree;
};

// NOTE(leonardo): file I/O DEBUG
struct PlatformFile
{
    void* content;
    u32 size;
};

#define DEBUG_PLATFORM_FREE_FILE( name ) void name( PlatformFile* file )
typedef DEBUG_PLATFORM_FREE_FILE( platform_free_file );

#define DEBUG_PLATFORM_READ_FILE( name ) PlatformFile name( char* fileName )
typedef DEBUG_PLATFORM_READ_FILE( platform_read_entire_file );

#define DEBUG_PLATFORM_WRITE_FILE( name ) b32 name( char* fileName, void* content, u32 fileSize )
typedef DEBUG_PLATFORM_WRITE_FILE( platform_write_entire_file );





// NOTE(Leonardo): command execution
struct PlatformProcessHandle 
{
    u64 OSHandle;
};

struct PlatformProcessState
{
    b32 startedSuccessfully;
    b32 isRunning;
    i32 returnCode;
};

#define DEBUG_PLATFORM_EXECUTE_COMMAND( name ) PlatformProcessHandle name( char* path, char* command, char* arguments )
typedef DEBUG_PLATFORM_EXECUTE_COMMAND( platform_execute_command );

#define DEBUG_PLATFORM_GET_PROCESS_STATE( name ) PlatformProcessState name( PlatformProcessHandle handle )
typedef DEBUG_PLATFORM_GET_PROCESS_STATE( platform_get_process_state );

#define DEBUG_PLATFORM_EXISTS_PROCESS_WITH_NAME(name) b32 name(char* processName)
typedef DEBUG_PLATFORM_EXISTS_PROCESS_WITH_NAME(platform_exists_process_with_name);

#define DEBUG_PLATFORM_KILL_PROCESS_BY_NAME(name) void name(char* processName)
typedef DEBUG_PLATFORM_KILL_PROCESS_BY_NAME(platform_kill_process_by_name);

#define PLATFORM_ERROR_MESSAGE(name) void name(char* message)
typedef PLATFORM_ERROR_MESSAGE(platform_error_message);




typedef enum PlatformFileType
{
    PlatformFile_compressedAsset,
    PlatformFile_uncompressedAsset,
    PlatformFile_savedGame,
    PlatformFile_image,
    PlatformFile_animation,
    PlatformFile_sound,
    PlatformFile_model,
    PlatformFile_entityDefinition,
    PlatformFile_assetDefinition,
    PlatformFile_autocomplete,
    PlatformFile_all,
    
    PlatformFile_count,
} PlatformFileType;


// NOTE( Leonardo ): final file API
struct PlatformFileHandle
{
    b32 noErrors;
    u32 fileSize;
    char name[64];
    void* platform;
};

struct PlatformFileGroup
{
    u32 fileCount;
    void* platform;
};

struct PlatformSubdirNames
{
    u32 subDirectoryCount;
    char subdirs[32][1024];
};

#define PLATFORM_GET_ALL_FILE_BEGIN( name ) PlatformFileGroup name( PlatformFileType type, char* folderPath )
typedef PLATFORM_GET_ALL_FILE_BEGIN( platform_get_all_file_begin );

#define PLATFORM_GET_ALL_SUBDIRECTORIES_NAME( name ) void name( PlatformSubdirNames* output, char* folderPath )
typedef PLATFORM_GET_ALL_SUBDIRECTORIES_NAME( platform_get_all_subdirectories_name );

#define PLATFORM_OPEN_NEXT_FILE( name ) PlatformFileHandle name( PlatformFileGroup* group, char* folderPath )
typedef PLATFORM_OPEN_NEXT_FILE( platform_open_next_file );

#define PLATFORM_CLOSE_HANDLE_INTERNAL( name ) void name( PlatformFileHandle* handle )
typedef PLATFORM_CLOSE_HANDLE_INTERNAL( platform_close_handle_internal );

#define PLATFORM_GET_FILE_SIZE( name ) u32 name( PlatformFileHandle* handle )
typedef PLATFORM_GET_FILE_SIZE( platform_get_file_size );

#define PLATFORM_READ_FROM_FILE( name ) void name( PlatformFileHandle* handle, u64 offset, u32 size, void* dest )
typedef PLATFORM_READ_FROM_FILE( platform_read_from_file );

#define PLATFORM_GET_ALL_FILE_END( name ) void name( PlatformFileGroup* group )
typedef PLATFORM_GET_ALL_FILE_END( platform_get_all_file_end );

#define PLATFORM_FILE_ERROR( name ) void name( PlatformFileHandle* handle, char* error )
typedef PLATFORM_FILE_ERROR( platform_file_error );


#define PLATFORM_CREATE_FOLDER(name) b32 name(char* path)
typedef PLATFORM_CREATE_FOLDER(platform_create_folder);

#define PLATFORM_COPY_ALL_FILES(name) void name(char* source, char* dest)
typedef PLATFORM_COPY_ALL_FILES(platform_copy_all_files);

#define PLATFORM_DELETE_FOLDER_RECURSIVE(name) void name(char* path)
typedef PLATFORM_DELETE_FOLDER_RECURSIVE(platform_delete_folder_recursive);

#define PLATFORM_DELETE_FILE_WILDCARDS(name) void name(char* path, char* fileName)
typedef PLATFORM_DELETE_FILE_WILDCARDS(platform_delete_file_wildcards);

#define PLATFORM_MOVE_FILE_OR_FOLDER(name) void name(char* oldPath, char* newPath)
typedef PLATFORM_MOVE_FILE_OR_FOLDER(platform_move_file_or_folder);

#define PLATFORM_COPY_FILE_OR_FOLDER(name) void name(char* oldPath, char* newPath)
typedef PLATFORM_COPY_FILE_OR_FOLDER(platform_copy_file_or_folder);


inline b32 PlatformNoFileErrors( PlatformFileHandle* handle )
{
    b32 result = handle->noErrors;
    return result;
}



enum PlatformMemoryFlags
{
    PlatformMemory_NotRestored = ( 1 << 0 ),
    PlatformMemory_OverflowCheck = ( 1 << 1 ),
    PlatformMemory_UnderflowCheck = ( 1 << 2 ),
};

struct PlatformMemoryBlock
{
    unm size;
    u64 flags;
    u8* base;
    unm used;
    
    PlatformMemoryBlock* poolPrev;
};

#define PLATFORM_ALLOCATE_MEMORY( name ) PlatformMemoryBlock* name( memory_index size, u64 flags )
typedef PLATFORM_ALLOCATE_MEMORY( platform_allocate_memory );

#define PLATFORM_DEALLOCATE_MEMORY( name ) void name( PlatformMemoryBlock* block )
typedef PLATFORM_DEALLOCATE_MEMORY( platform_deallocate_memory );

#define PLATFORM_GET_CLIPBOARD(name) void name(char* buffer, u32 bufferLength)
typedef PLATFORM_GET_CLIPBOARD(platform_get_clipboard);

#define PLATFORM_SET_CLIPBOARD(name) void name(char* text, u32 textLength)
typedef PLATFORM_SET_CLIPBOARD(platform_set_clipboard);

struct DebugPlatformMemoryStats
{
    u32 blockCount;
    unm totalUsed;
    unm totalSize;
};

#define DEBUG_PLATFORM_MEMORY_STATS( name ) DebugPlatformMemoryStats name( void )
typedef DEBUG_PLATFORM_MEMORY_STATS( debug_platform_memory_stats );


#if FORG_SERVER
#if FORGIVENESS_INTERNAL
#define INPUT_RECORDING_COMMAND(name) void name( struct ServerState* server, b32 recording, b32 startAutomatically )
typedef INPUT_RECORDING_COMMAND( platform_input_recording_command );

#define INPUT_RECORDING_HANDLE_PLAYER( name ) void name( struct ServerState* server, struct ActionRequest* request )
typedef INPUT_RECORDING_HANDLE_PLAYER( platform_input_recording_handle_player );
#endif
#endif

struct PlatformAPI
{
    platform_get_all_file_begin* GetAllFilesBegin;
    platform_get_all_subdirectories_name* GetAllSubdirectoriesName;
    platform_open_next_file* OpenNextFile;
    platform_read_from_file* ReadFromFile;
    platform_get_all_file_end* GetAllFilesEnd;
    platform_close_handle_internal* CloseHandle;
    platform_file_error* FileError;
    
    platform_allocate_memory* AllocateMemory;
    platform_deallocate_memory* DeallocateMemory;
    platform_get_clipboard* GetClipboardText;
    platform_set_clipboard* SetClipboardText;
    debug_platform_memory_stats* DEBUGMemoryStats;
    
    platform_push_work* PushWork;
    platform_complete_queue_work* CompleteQueueWork;
    
    platform_read_entire_file* DEBUGReadFile;
    platform_write_entire_file* DEBUGWriteFile;
    platform_free_file* DEBUGFreeFile;
    
    platform_create_folder* CreateFolder;
    platform_copy_all_files* CopyAllFiles;
    platform_delete_folder_recursive* DeleteFolderRecursive;
    platform_delete_file_wildcards* DeleteFileWildcards;
    platform_move_file_or_folder* MoveFileOrFolder;
    platform_copy_file_or_folder* CopyFileOrFolder;
    
    NetworkAPI net;
    
    platform_execute_command* DEBUGExecuteSystemCommand;
    platform_get_process_state* DEBUGGetProcessState;
    platform_exists_process_with_name* DEBUGExistsProcessWithName;
    platform_kill_process_by_name* DEBUGKillProcessByName;
    
    platform_error_message* ErrorMessage;
    
#if FORGIVENESS_INTERNAL
    
#if FORG_SERVER
    platform_input_recording_command* PlatformInputRecordingCommand;
    platform_input_recording_handle_player* PlatformInputRecordingHandlePlayer;
#endif
#endif
    
    
};

struct PlatformServerMemory
{
    struct ServerState* server;
    // NOTE(Leonardo): this two are used for input recording and replaying!
    //void* bufferedStorage;
    //void* checkpoint;
    
#if FORGIVENESS_INTERNAL
    struct DebugTable* debugTable;
#endif
    
    PlatformAPI api;
};

struct PlatformClientMemory
{
    struct GameState* gameState;
    struct TranState* tranState;
    
#if FORGIVENESS_INTERNAL
    b32 DLLReloaded;
    struct DebugTable* debugTable;
    struct DebugState* debugState;
#endif
    
    PlatformAPI api;
    
    PlatformWorkQueue* highPriorityQueue;
    PlatformWorkQueue* lowPriorityQueue;
    PlatformTextureOpQueue textureQueue;
};

extern struct PlatformClientMemory* debugGlobalMemory;

#define GAME_UPDATE_AND_RENDER( name ) void name( PlatformClientMemory* memory, PlatformInput* input, GameRenderCommands* commands )
typedef GAME_UPDATE_AND_RENDER( game_update_and_render );

#define GAME_GET_SOUND_OUTPUT( name ) void name( PlatformClientMemory* memory, PlatformInput* input, PlatformSoundBuffer* soundBuffer )
typedef GAME_GET_SOUND_OUTPUT( game_get_sound_output );

#include "forg_pool.h"
#include "forg_debug_interface.h"