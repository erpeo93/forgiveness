#pragma once
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

struct GameRenderSettings
{
    u32 depthPeelCount;
    b32 multisamplingHint;
    u32 width;
    u32 height;
};

#define MAX_IMAGE_DIM 512
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
        (A->depthPeelCount == B->depthPeelCount &&
         A->multisamplingHint == B->multisamplingHint &&
         A->width == B->width &&
         A->height == B->height);
    
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
    
    struct NetworkInterface* network;
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

inline b32 IsDown(PlatformButton* button)
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
#define PLATFORM_WORK_CALLBACK(name) void name(void* param)
typedef PLATFORM_WORK_CALLBACK(platform_work_callback);

struct PlatformWorkQueueEntry
{
    void* param;
    platform_work_callback* callback;
};

struct PlatformWorkQueue;

#define COMPLETE_QUEUE_WORK(name) void name(PlatformWorkQueue* queue)
typedef COMPLETE_QUEUE_WORK(platform_complete_queue_work);

#define PUSH_QUEUE_WORK(name) void name(PlatformWorkQueue* queue, platform_work_callback* callback, void* param)
typedef PUSH_QUEUE_WORK(platform_push_work);


struct PlatformTextureOpQueue
{
    TicketMutex mutex;
    struct TextureOp* first;
    struct TextureOp* last;
    struct TextureOp* firstFree;
};




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

#define DEBUG_PLATFORM_EXECUTE_COMMAND(name) PlatformProcessHandle name(char* path, char* command, char* arguments)
typedef DEBUG_PLATFORM_EXECUTE_COMMAND(platform_execute_command);

#define DEBUG_PLATFORM_GET_PROCESS_STATE(name) PlatformProcessState name(PlatformProcessHandle handle)
typedef DEBUG_PLATFORM_GET_PROCESS_STATE(platform_get_process_state);

#define DEBUG_PLATFORM_EXISTS_PROCESS_WITH_NAME(name) b32 name(char* processName)
typedef DEBUG_PLATFORM_EXISTS_PROCESS_WITH_NAME(platform_exists_process_with_name);

#define DEBUG_PLATFORM_KILL_PROCESS_BY_NAME(name) void name(char* processName)
typedef DEBUG_PLATFORM_KILL_PROCESS_BY_NAME(platform_kill_process_by_name);

#define PLATFORM_ERROR_MESSAGE(name) void name(char* message)
typedef PLATFORM_ERROR_MESSAGE(platform_error_message);




typedef enum PlatformFileType
{
    PlatformFile_invalid = 0,
    PlatformFile_AssetPack = (1 << 1),
    PlatformFile_savedGame = (1 << 2),
    PlatformFile_png  = (1 << 3),
    PlatformFile_Coloration  = (1 << 4),
    PlatformFile_font  = (1 << 5),
    PlatformFile_skeleton  = (1 << 6),
    PlatformFile_sound  = (1 << 7),
    PlatformFile_model = (1 << 8),
    PlatformFile_data = (1 << 9),
    PlatformFile_markup = (1 << 10),
    PlatformFile_reloadedAsset = (1 << 11),
    PlatformFile_timestamp = (1 << 12),
    PlatformFile_properties = (1 << 13),
    
    PlatformFile_count,
} PlatformFileType;


// NOTE(Leonardo): final file API
struct PlatformFileHandle
{
    b32 noErrors;
    void* platform;
};

struct PlatformFileInfo
{
    u64 size;
    char* name;
    u64 timestamp;
    
    PlatformFileInfo* next;
};

struct PlatformFileGroup
{
    char* path;
    u32 fileCount;
    PlatformFileInfo* firstFileInfo;
    
    void* platform;
};

struct PlatformSubdirNames
{
    u32 count;
    char names[32][1024];
};

#define PLATFORM_GET_ALL_FILE_BEGIN(name) PlatformFileGroup name(u32 fileTypes, char* path)
typedef PLATFORM_GET_ALL_FILE_BEGIN(platform_get_all_file_begin);

#define PLATFORM_GET_ALL_SUBDIRECTORIES(name) void name(PlatformSubdirNames* output, char* folderPath)
typedef PLATFORM_GET_ALL_SUBDIRECTORIES(platform_get_all_subdirectories);

#define PLATFORM_OPEN_FILE(name) PlatformFileHandle name(PlatformFileGroup* group, PlatformFileInfo* info)
typedef PLATFORM_OPEN_FILE(platform_open_file);

#define PLATFORM_CLOSE_FILE(name) void name(PlatformFileHandle* handle)
typedef PLATFORM_CLOSE_FILE(platform_close_file);

#define PLATFORM_READ_FROM_FILE(name) void name(PlatformFileHandle* handle, u64 offset, u64 size, void* dest)
typedef PLATFORM_READ_FROM_FILE(platform_read_from_file);

#define PLATFORM_GET_ALL_FILE_END(name) void name(PlatformFileGroup* group)
typedef PLATFORM_GET_ALL_FILE_END(platform_get_all_file_end);

#define PLATFORM_FILE_ERROR(name) void name(PlatformFileHandle* handle, char* error)
typedef PLATFORM_FILE_ERROR(platform_file_error);

#define PLATFORM_DELETE_FILES(name) void name(PlatformFileType type, char* path)
typedef PLATFORM_DELETE_FILES(platform_delete_files);

#define PLATFORM_REPLACE_FILE(name) b32 name(PlatformFileType type, char* path, char* file, u8* content, u32 size)
typedef PLATFORM_REPLACE_FILE(platform_replace_file);


inline b32 PlatformNoFileErrors(PlatformFileHandle* handle)
{
    b32 result = handle->noErrors;
    return result;
}



enum PlatformMemoryFlags
{
    PlatformMemory_NotRestored = (1 << 0),
    PlatformMemory_OverflowCheck = (1 << 1),
    PlatformMemory_UnderflowCheck = (1 << 2),
};

struct PlatformMemoryBlock
{
    unm size;
    u64 flags;
    u8* base;
    unm used;
    
    PlatformMemoryBlock* poolPrev;
};

#define PLATFORM_ALLOCATE_MEMORY(name) PlatformMemoryBlock* name(memory_index size, u64 flags)
typedef PLATFORM_ALLOCATE_MEMORY(platform_allocate_memory);

#define PLATFORM_DEALLOCATE_MEMORY(name) void name(PlatformMemoryBlock* block)
typedef PLATFORM_DEALLOCATE_MEMORY(platform_deallocate_memory);

struct DebugPlatformMemoryStats
{
    u32 blockCount;
    unm totalUsed;
    unm totalSize;
};

#define DEBUG_PLATFORM_MEMORY_STATS(name) DebugPlatformMemoryStats name(void)
typedef DEBUG_PLATFORM_MEMORY_STATS(debug_platform_memory_stats);


#if FORG_SERVER
#if FORGIVENESS_INTERNAL
#define INPUT_RECORDING_COMMAND(name) void name(struct ServerState* server, b32 recording, b32 startAutomatically)
typedef INPUT_RECORDING_COMMAND(platform_input_recording_command);

#define INPUT_RECORDING_HANDLE_PLAYER(name) void name(struct ServerState* server, struct ActionRequest* request)
typedef INPUT_RECORDING_HANDLE_PLAYER(platform_input_recording_handle_player);
#endif
#endif

struct PlatformAPI
{
    // NOTE(Leonardo): file api
    platform_get_all_file_begin* GetAllFilesBegin;
    platform_get_all_subdirectories* GetAllSubdirectories;
    platform_open_file* OpenFile;
    platform_read_from_file* ReadFromFile;
    platform_get_all_file_end* GetAllFilesEnd;
    platform_close_file* CloseFile;
    platform_file_error* FileError;
    platform_delete_files* DeleteFiles;
    platform_replace_file* ReplaceFile;
    
    // NOTE(Leonardo): memory api
    platform_allocate_memory* AllocateMemory;
    platform_deallocate_memory* DeallocateMemory;
    
    // NOTE(Leonardo): multithread api
    platform_push_work* PushWork;
    platform_complete_queue_work* CompleteQueueWork;
    
    // NOTE(Leonardo): network api
    NetworkAPI net;
    
    // NOTE(Leonardo): process api
    platform_execute_command* DEBUGExecuteSystemCommand;
    platform_get_process_state* DEBUGGetProcessState;
    platform_exists_process_with_name* DEBUGExistsProcessWithName;
    platform_kill_process_by_name* DEBUGKillProcessByName;
    
    // NOTE(Leonardo): misc api
    debug_platform_memory_stats* DEBUGMemoryStats;
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
    r32 elapsedTime;
    
    PlatformWorkQueue* fastQueue;
    PlatformWorkQueue* slowQueue;
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

#define GAME_UPDATE_AND_RENDER(name) void name(PlatformClientMemory* memory, PlatformInput* input, GameRenderCommands* commands)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);

#define GAME_GET_SOUND_OUTPUT(name) void name(PlatformClientMemory* memory, PlatformInput* input, PlatformSoundBuffer* soundBuffer)
typedef GAME_GET_SOUND_OUTPUT(game_get_sound_output);


#ifdef FORG_SERVER
#define SERVER_SIMULATE_WORLDS( name ) void name(PlatformServerMemory* memory, r32 secondElapsed)
typedef SERVER_SIMULATE_WORLDS( server_simulate_worlds );
#endif
