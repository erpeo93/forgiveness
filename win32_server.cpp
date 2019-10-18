#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <process.h>
#include <Tlhelp32.h>
#include <winbase.h>
#include <DbgHelp.h>
#include <stdio.h>
#include <time.h>

#include "forg_base.h"
#define LL_NET_IMPLEMENTATION
#include "ll_net.h"

#include "forg_platform.h"
global_variable PlatformAPI platformAPI;
#include "forg_pool.h"
#include "forg_debug_interface.h"
#include "forg_token.h"
#include "forg_shared.h"
#include "forg_intrinsics.h"
#include "forg_math.h"
#include "forg_pool.cpp"

#include "win32_forg.h"
#include "win32_file.cpp"
#include "win32_thread.cpp"

#include "win32_process.cpp"



#pragma comment( lib, "wsock32.lib" )

global_variable Win32MemoryBlock globalMemorySentinel;
global_variable TicketMutex memoryMutex;
global_variable b32 globalRecording;
global_variable b32 globalPlayingBack;
global_variable HANDLE recordingHandle;
global_variable HANDLE playingBackHandle;

#define SERVER_FPS 60.0f
#define SERVER_MSEC_PER_FRAME 1000.0f / SERVER_FPS

inline b32 IsInLoop()
{
    b32 result = false;
    if(globalRecording || globalPlayingBack)
    {
        result = true;
    }
    
    return result;
}

#include "win32_memory_callback.cpp"
internal void ClearBlocksByMask( u64 mask )
{
    for( Win32MemoryBlock* block = globalMemorySentinel.next; block != &globalMemorySentinel; block = block->next )
    {
        if( ( block->loopingFlags & mask ) == mask )
        {
            Win32FreeMemoryBlock( block );
        }
        else
        {
            block->loopingFlags = 0;
        }
    }
    
}

#if FORGIVENESS_INTERNAL
internal void SendLoopMessage(ServerState* server, b32 justStarted)
{
    InvalidCodePath;
    
#if 0    
    Assert( server->currentPlayerIndex = 2 );
    ServerPlayer* player = server->debugPlayer;
    b32 result = false;
    
    
    unsigned char buff_[1024];
    unsigned char* buff = ForgPackHeader( buff_, Type_InputRecording);
    buff +=pack( buff, "l", justStarted );
    
    u32 totalSize = ForgEndPacket( buff_, buff );
    b32 sended = Win32SendData(&player->connection, ( char* ) buff_, totalSize);
    Assert(sended);
#endif
    
}

internal void BeginInputPlayback( ServerState* server )
{
    ClearBlocksByMask( LoopingFlag_AllocatedDuringLooping ); 
    playingBackHandle = CreateFile( "test.fms", GENERIC_READ, FILE_SHARE_READ, 0, OPEN_ALWAYS, 0, 0 );
    if( playingBackHandle )
    {
        for( ;; )
        {
            Win32SavedMemoryBlock block;
            
            DWORD ignored;
            ReadFile( playingBackHandle, &block, sizeof( block ), &ignored, 0 );
            if( block.basePointer )
            {
                void* basePointer = ( void* ) block.basePointer;
                Assert( block.size < U32_MAX );
                ReadFile( playingBackHandle, basePointer, ( u32 ) block.size, &ignored, 0 );
            }
            else
            {
                break;
            }
        }
        
        globalPlayingBack = true;
        SendLoopMessage( server, true );
    }
}

internal void EndInputPlayback(ServerState* server)
{
    ClearBlocksByMask(LoopingFlag_FreedDuringLooping);
    CloseHandle(playingBackHandle);
    globalPlayingBack = false;
    SendLoopMessage(server, false);
}


internal void Win32WritePlayerState(ServerState* server, ActionRequest* source)
{
    InvalidCodePath;
#if 0    
    Assert(globalRecording && recordingHandle);
    DWORD bytesWritten;
    WriteFile(recordingHandle, source, sizeof(ActionRequest), &bytesWritten, 0);
    Assert( bytesWritten == sizeof(ActionRequest));
#endif
    
}

internal void Win32ReadPlayerState(ServerState* server, ActionRequest* dest)
{
    InvalidCodePath;
    
#if 0    
    Assert(globalPlayingBack && playingBackHandle);
    DWORD bytesRead;
    ReadFile(playingBackHandle, dest, sizeof(ActionRequest), &bytesRead, 0);
    if(bytesRead == 0)
    {
        EndInputPlayback(server);
        BeginInputPlayback(server);
    }
    else
    {
        Assert(bytesRead == sizeof(ActionRequest));
    }
#endif
    
}

internal void Win32InputRecordingHandlePlayer(ServerState* server, ActionRequest* sourceDest)
{
    InvalidCodePath;
    
#if 0    
    if(globalRecording)
    {
        Assert(!globalPlayingBack);
        Win32WritePlayerState(server, sourceDest);
    }
    
    if(globalPlayingBack)
    {
        Win32ReadPlayerState(server, sourceDest);
    }
#endif
    
}

internal void StartInputRecording()
{
    globalRecording = true;
    recordingHandle = CreateFile("test.fms", GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    Win32MemoryBlock* sentinel = &globalMemorySentinel;
    BeginTicketMutex(&memoryMutex);
    
    for(Win32MemoryBlock* block = sentinel->next; block != sentinel; block = block->next)
    {
        if(!(block->block.flags & PlatformMemory_NotRestored))
        {
            Win32SavedMemoryBlock toSave;
            void* basePointer = block->block.base;
            toSave.basePointer = (u64) basePointer;
            toSave.size = block->block.size;
            
            DWORD bytesWritten;
            WriteFile(recordingHandle, &toSave, sizeof(Win32SavedMemoryBlock), &bytesWritten, 0);
            Assert( bytesWritten == sizeof( Win32SavedMemoryBlock ) );
            
            Assert( toSave.size < U32_MAX );
            WriteFile( recordingHandle, basePointer, ( u32 ) toSave.size, &bytesWritten, 0 );
            Assert( bytesWritten == toSave.size );
        }
    }
    EndTicketMutex( &memoryMutex );
    
    Win32SavedMemoryBlock lastOne = {};
    
    DWORD bytesWritten;
    WriteFile( recordingHandle, &lastOne, sizeof( Win32SavedMemoryBlock ), &bytesWritten, 0 );
    Assert( bytesWritten == sizeof( Win32SavedMemoryBlock ) );
}

internal void EndInputRecording()
{
    globalRecording = false;
    CloseHandle( recordingHandle );
}

INPUT_RECORDING_COMMAND(Win32DispatchInputRecordingCommand)
{
    if( recording )
    {
        if( !globalPlayingBack )
        {
            if( !globalRecording )
            {
                StartInputRecording();
            }
            else
            {
                EndInputRecording();
                if( startAutomatically )
                {
                    BeginInputPlayback( server );
                }
            }
        }
    }
    else
    {
        if( !globalRecording )
        {
            if( !globalPlayingBack )
            {
                BeginInputPlayback( server );
            }
            else
            {
                EndInputPlayback( server );
            }
        }
    }
}
#endif

struct ServerFunctions
{
    HMODULE serverDLL;
    FILETIME lastWriteTime;
    server_simulate_worlds* SimulateWorlds;
#if FORGIVENESS_INTERNAL
    server_frame_end* ServerFrameEnd;
#endif
};

internal void FreeServerCode( ServerFunctions* functions )
{
    FreeLibrary(functions->serverDLL);
    functions->serverDLL = {};
    
    functions->SimulateWorlds = 0;
    
#if FORGIVENESS_INTERNAL
    functions->ServerFrameEnd = 0;
#endif
}

ServerFunctions LoadServerCode( char* DLLName, char* tempDLLName, char* lockName )
{
    ServerFunctions result = {};
    
    char* toLoad = DLLName;
    
#if FORGIVENESS_INTERNAL
    CopyFile( DLLName, tempDLLName, 0 );
    toLoad = tempDLLName;
#endif
    
    WIN32_FILE_ATTRIBUTE_DATA fileAttributes;
    WIN32_FILE_ATTRIBUTE_DATA ignored;
    if(!GetFileAttributesEx( lockName, GetFileExInfoStandard, &ignored))
    {
        if(GetFileAttributesEx( DLLName, GetFileExInfoStandard, &fileAttributes))
        {
            result.lastWriteTime = fileAttributes.ftLastWriteTime;
        }
        HMODULE serverDLL = LoadLibrary(toLoad);
        if(serverDLL)
        {
            result.serverDLL = serverDLL;
            result.SimulateWorlds = ( server_simulate_worlds* ) GetProcAddress( serverDLL, "SimulateWorlds" );
            
#if FORGIVENESS_INTERNAL
            result.ServerFrameEnd = ( server_frame_end* ) GetProcAddress( serverDLL, "ServerFrameEnd" );
#endif
        }
    }
    
    return result;
}

#if FORGIVENESS_INTERNAL
global_variable DebugTable globalDebugTable_;
DebugTable* globalDebugTable = &globalDebugTable_;
#endif

int main( int argc, char* argv[] )
{
    SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER) CreateMiniDump);
    
    Win32ThreadStartup fastStartups[5] = {};
    PlatformWorkQueue fastQueue = {};
    Win32MakeQueue(&fastQueue, ArrayCount(fastStartups), fastStartups);
    
    Win32ThreadStartup slowStartups[2] = {};
    PlatformWorkQueue slowQueue = {};
    Win32MakeQueue(&slowQueue, ArrayCount(slowStartups), slowStartups);
    
    globalMemorySentinel.next = &globalMemorySentinel;
    globalMemorySentinel.prev = &globalMemorySentinel;
    
    PlatformServerMemory* memory = (PlatformServerMemory*) malloc(sizeof(PlatformServerMemory));
    memset(memory, 0, sizeof(PlatformServerMemory));
    memory->api.AllocateMemory = Win32AllocateMemory;
    memory->api.DeallocateMemory = Win32DeallocateMemory;
    
#if FORGIVENESS_INTERNAL
    memory->api.PlatformInputRecordingCommand = Win32DispatchInputRecordingCommand;
    memory->api.PlatformInputRecordingHandlePlayer = Win32InputRecordingHandlePlayer;
    memory->api.DEBUGMemoryStats = Win32GetMemoryStats;
    memory->debugTable = globalDebugTable;
#endif
    memory->api.GetAllSubdirectories = Win32GetAllSubdirectories;
    memory->api.CloseFile = Win32CloseFile;
    memory->api.GetAllFilesBegin = Win32GetAllFilesBegin;
    memory->api.OpenFile = Win32OpenFile;
    memory->api.GetAllFilesEnd = Win32GetAllFilesEnd;
    memory->api.ReadFromFile = Win32ReadFromFile;
    memory->api.FileError = Win32FileError;
    memory->api.ReplaceFile = Win32ReplaceFile;
    
    memory->api.DEBUGExecuteSystemCommand = DEBUGWin32ExecuteCommand;
    memory->api.DEBUGGetProcessState = DEBUGWin32GetProcessState;
    
    memory->api.DeleteFiles = Win32DeleteFiles;
    
    memory->api.net = Win32NetworkAPI;
    
    memory->api.CompleteQueueWork = Win32CompleteQueueWork;
    memory->api.PushWork = Win32PushWork;
    
    memory->fastQueue = &fastQueue;
    memory->slowQueue = &slowQueue;
    
    platformAPI = memory->api;
    
    char* DLLName = "forg_server.dll";
    char* tempDLLName = "forg_server_temp.dll";
    char* lockName = "lockS.tmp";
    char ExeFullPath[MAX_PATH];
    GetModuleFileName( 0, ExeFullPath, sizeof( ExeFullPath ) );
    
    char DLLFullName[MAX_PATH];
    char tempDLLFullName[MAX_PATH];	
    char lockFullName[MAX_PATH];	
    Win32BuildFullPath( ExeFullPath, DLLName, DLLFullName, sizeof( DLLFullName ) );
    Win32BuildFullPath( ExeFullPath, tempDLLName, tempDLLFullName, sizeof( tempDLLFullName ) );
    Win32BuildFullPath( ExeFullPath, lockName, lockFullName, sizeof( lockFullName ) );
    
    ServerFunctions functions = LoadServerCode( DLLFullName, tempDLLFullName, lockFullName );
    
    u32 universeIndex = 0;
    
    b32 editor = false;
    printf("server starting\n");
    if(argc > 1)
    {
        printf("editor mode!\n");
        editor = true;
    }
    
    if(Win32InitNetwork())
    {
        clock_t start = clock();
        r32 secondElapsed = 0;
        
        while(true)
        {
#if FORGIVENESS_INTERNAL
            BEGIN_BLOCK("check dll");
            WIN32_FILE_ATTRIBUTE_DATA DLLFileAttributes;
            if(GetFileAttributesEx(DLLFullName, GetFileExInfoStandard, &DLLFileAttributes))
            {
                FILETIME DLLLastWriteTime = DLLFileAttributes.ftLastWriteTime;
                if(CompareFileTime( &functions.lastWriteTime, &DLLLastWriteTime) != 0)
                {
                    FreeServerCode(&functions);
                    while(!functions.serverDLL)
                    {
                        functions = LoadServerCode(DLLFullName, tempDLLFullName, lockFullName);
                    }
                }
            }
            END_BLOCK();
#endif
            
            if(functions.SimulateWorlds)
            {
                functions.SimulateWorlds(memory, secondElapsed);
            }
            
            
            BEGIN_BLOCK("sleep");
            clock_t end = clock();
            r32 MSecondElapsed = (r32) (((end - start) * 1000.0f)  / CLOCKS_PER_SEC);
            
            while(MSecondElapsed <  SERVER_MSEC_PER_FRAME)
            {
                end = clock();
                MSecondElapsed = (r32) (((end - start) * 1000.0f)  / CLOCKS_PER_SEC);
            }
            start = end;
            secondElapsed = MSecondElapsed / 1000.0f;
            memory->elapsedTime = secondElapsed;
            END_BLOCK();
            
            FRAME_MARKER(secondElapsed);
#if FORGIVENESS_INTERNAL
            if(functions.ServerFrameEnd)
            {
                functions.ServerFrameEnd(memory);
            }
#endif
        }
        
        WSACleanup();
    }
}
