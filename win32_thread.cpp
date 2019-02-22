struct PlatformWorkQueue
{
    u32 volatile completionGoal;
    u32 volatile completedEntries;
    
    u32 volatile nextEntryToRead;
    u32 volatile nextEntryToWrite;
    HANDLE semaphoreHandle;
    PlatformWorkQueueEntry workEntries[128];
    
    b32 needsOpenGL;
};

struct WorkQueueWorkResult
{
    b32 exists;
    u32 index;
};

struct Win32ThreadStartup
{
    PlatformWorkQueue* queue;
    HGLRC openGLRC;
    HDC openGLDC;
};

internal void Win32DispatchJob( PlatformWorkQueueEntry* entry )
{
    entry->callback(entry->param);
}

internal WorkQueueWorkResult WorkToBeDone(PlatformWorkQueue* queue)
{
    WorkQueueWorkResult result = {};
    
    u32 oldNextEntryToRead = queue->nextEntryToRead;
    u32 newNextEntryToRead = ( oldNextEntryToRead + 1 ) % ArrayCount( queue->workEntries );
    
    if( oldNextEntryToRead != queue->nextEntryToWrite )
    {
        u32 index = InterlockedCompareExchange( ( LONG volatile* ) &queue->nextEntryToRead, newNextEntryToRead, oldNextEntryToRead );
        if( index == oldNextEntryToRead )
        {
            result.exists = true;
            result.index = index;
            CompletePastReadsBeforeFutureReads;
        }
    }
    return result;
}

internal b32 Win32DoWorkerWork(PlatformWorkQueue* queue)
{
    b32 result = false;
    WorkQueueWorkResult todo = WorkToBeDone( queue );
    if(todo.exists)
    {
        PlatformWorkQueueEntry* entry = queue->workEntries + todo.index;
        Win32DispatchJob( entry );
        InterlockedIncrement((LONG volatile*) &queue->completedEntries);
        result = true;
    }
    return result;
}

DWORD WINAPI ThreadProc(LPVOID lpParameter)
{
    Win32ThreadStartup* startup = ( Win32ThreadStartup* ) lpParameter;
    PlatformWorkQueue* queue = startup->queue;
    
    u32 testThreadID = GetThreadID();
    Assert( testThreadID == GetCurrentThreadId() );
    
#ifndef FORG_SERVER
    if(startup->openGLRC)
    {
        if(wglMakeCurrent( startup->openGLDC, startup->openGLRC))
        {
            
        }
        else
        {
            InvalidCodePath
        }
    }
#endif
    
    for(;;)
    {
        if( !Win32DoWorkerWork( queue ) )
        {
            WaitForSingleObject( queue->semaphoreHandle, INFINITE );
        }
    }
}

COMPLETE_QUEUE_WORK(Win32CompleteQueueWork)
{
    while(queue->completedEntries < queue->completionGoal)
    {
        Win32DoWorkerWork(queue); 
    }
    
    queue->completionGoal = 0;
    queue->completedEntries = 0;
    Assert(queue->nextEntryToWrite == queue->nextEntryToRead);
}

PUSH_QUEUE_WORK(Win32PushWork)
{
    PlatformWorkQueueEntry* nextEntry = queue->workEntries + queue->nextEntryToWrite;
    nextEntry->param = param;
    nextEntry->callback = callback;
    CompletePastWritesBeforeFutureWrites;
    
    ++queue->completionGoal;
    queue->nextEntryToWrite = ( queue->nextEntryToWrite + 1 ) % ArrayCount( queue->workEntries );
    
    ReleaseSemaphore( queue->semaphoreHandle, 1, 0 );
}


internal void Win32MakeQueue( PlatformWorkQueue* queue, u32 threadCount, Win32ThreadStartup* startups )
{
    queue->completionGoal = 0;
    queue->completedEntries = 0;
    queue->nextEntryToRead = 0;
    queue->nextEntryToWrite = 0;
    
    queue->semaphoreHandle = CreateSemaphoreEx( 0, 0, threadCount, 0, 0, SEMAPHORE_ALL_ACCESS );
    for( u32 threadIndex = 0; threadIndex < threadCount; threadIndex++ )
    {
        Win32ThreadStartup* startup = startups + threadIndex;
        startup->queue = queue;
        
        DWORD threadID;
        HANDLE threadHandle = CreateThread( 0, 0, ThreadProc, startup, 0, &threadID );
        CloseHandle( threadHandle );
    }
}


