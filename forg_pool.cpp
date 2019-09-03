#define PushStruct( alloc, type, ... ) ( type* ) PushSize_( alloc, sizeof( type ), ##__VA_ARGS__ ) 
#define PushArray( alloc, type, count, ... ) ( type* ) PushSize_( alloc, count * sizeof( type ), ##__VA_ARGS__ )
#define PushSize( alloc, size, ... ) ( u8* ) PushSize_( alloc, size, ##__VA_ARGS__ )
#define PushCopy(alloc, size, source, ... ) Copy(size, PushSize_( alloc, size, ##__VA_ARGS__ ), source)

enum PushParamFlags
{
    PushParamFlag_Clear = ( 1 << 1 )
};

struct PushParams
{
    memory_index align;
    u32 flags;
};

inline PushParams DefaultPushParams()
{
    PushParams result = {};
    result.align = 4;
    result.flags = PushParamFlag_Clear;
    
    return result;
}

inline PushParams AlignClear( memory_index align )
{
    PushParams result = DefaultPushParams();
    result.align = align;
    
    return result;
}

inline PushParams AlignNoClear( memory_index align )
{
    PushParams result = DefaultPushParams();
    result.flags &= ~PushParamFlag_Clear;
    result.align = align;
    
    return result;
}

inline PushParams NoClear()
{
    PushParams result = DefaultPushParams();
    result.flags &= ~PushParamFlag_Clear;
    
    return result;
}

inline memory_index GetAlignmentOffset( MemoryPool* pool, PushParams params = DefaultPushParams() )
{
    memory_index alignMask = params.align - 1;
    memory_index misalignment = ( ( unm ) ( pool->currentBlock->base ) + pool->currentBlock->used ) & alignMask;
    memory_index result = 0;
    if( misalignment )
    {
        result = params.align - misalignment;
    }
    
    return result;
}

inline memory_index GetEffectiveSize( MemoryPool* pool, memory_index size, PushParams params = DefaultPushParams() )
{
    memory_index totalSize = size;
    memory_index alignmentOffset = GetAlignmentOffset( pool, params );
    totalSize += alignmentOffset;
    
    return totalSize;
}

internal void* PushSize_( MemoryPool* pool, memory_index size, PushParams params = DefaultPushParams() )
{
    void* result = 0;
    
    //pool->allocationFlags |= PlatformMemory_OverflowCheck;
    //pool->allocationFlags |= PlatformMemory_UnderflowCheck;
    
    unm totalSize = 0;
    if( pool->currentBlock )
    {
        totalSize = GetEffectiveSize( pool, size, params );
    }
    
    if( !pool->currentBlock || ( pool->currentBlock->used + totalSize ) > pool->currentBlock->size )
    {
        totalSize = size;
        if( pool->allocationFlags & ( PlatformMemory_OverflowCheck | PlatformMemory_UnderflowCheck ) )
        {
            pool->minimumBlockSize = 0;
            size = AlignPow2( size, params.align );
            
        }
        else if( !pool->minimumBlockSize )
        {
            pool->minimumBlockSize = MegaBytes( 1 );
        }
        
        totalSize = size;
        memory_index blockSize = Max( totalSize, pool->minimumBlockSize );
        PlatformMemoryBlock* newBlock = platformAPI.AllocateMemory(blockSize, pool->allocationFlags);
        
        newBlock->poolPrev = pool->currentBlock;
        pool->currentBlock = newBlock;
        
        ++pool->blockCount;
    }
    
    Assert( ( pool->currentBlock->used + totalSize ) <= pool->currentBlock->size );
    memory_index alignSize = GetAlignmentOffset( pool, params );
    result = ( void*) ( ( ( u8* ) pool->currentBlock->base ) + ( pool->currentBlock->used + alignSize ) );
    pool->currentBlock->used += totalSize;
    
    Assert( size == totalSize - alignSize );
    if( params.flags & PushParamFlag_Clear )
    {
        ZeroSize( size, result );
    }
    return result;
}

inline char* PushString(MemoryPool* pool, char* string)
{
    PushParams params = AlignNoClear(1);
    char* result = (char*) PushCopy(pool, StrLen(string) + 1, string, params);
    return result;
}

inline char* PushAndNullTerminate(MemoryPool* pool, char* string, u32 length)
{
    char* dest = (char*) PushSize_( pool, length + 1, NoClear());
    Copy( length, dest, string);
    dest[length] = 0;
    
    return dest;
}

inline char* PushNullTerminatedString(MemoryPool* pool, char* string)
{
    u32 length = StrLen(string);
    char* result = PushAndNullTerminate(pool, string, length);
    return result;
}

inline void CheckPool( MemoryPool* pool )
{
    Assert( pool->tempCount == 0 );
}

inline TempMemory BeginTemporaryMemory( MemoryPool* pool )
{
    TempMemory result = {};
    result.pool = pool;
    result.block = pool->currentBlock;
    result.used = pool->currentBlock ? pool->currentBlock->used : 0;
    pool->tempCount++;
    return result;
}

inline b32 FreeLastBlock( MemoryPool* pool )
{
    b32 result = false;
    PlatformMemoryBlock* free = pool->currentBlock;
    pool->currentBlock = free->poolPrev;
    
    if( !pool->currentBlock && pool->bootstrapped )
    {
        result = true;
    }
    
    platformAPI.DeallocateMemory(free);
    
    return result;
}

inline void EndTemporaryMemory(TempMemory memory)
{
    MemoryPool* pool = memory.pool;
    while(pool->currentBlock != memory.block)
    {
        FreeLastBlock(pool);
    }
    
    if(pool->currentBlock)
    {
        Assert(pool->currentBlock->used >= memory.used);
        pool->currentBlock->used = memory.used;
        Assert(pool->tempCount > 0);
    }
    
    --pool->tempCount;
}

inline TaskWithMemory* BeginTaskWithMemory(TaskWithMemory* tasks, u32 countTasks, b32 dependOnGameMode)
{
    TaskWithMemory* result = 0;
    for( u32 taskIndex = 0; 
        taskIndex < countTasks; 
        taskIndex++ )
    {
        TaskWithMemory* task = tasks + taskIndex;
        
        if(!task->beingUsed)
        {
            task->beingUsed = true;
            task->dependOnGameMode = dependOnGameMode;
            task->memoryFlush = BeginTemporaryMemory(&task->pool);
            result = task;
            break;
        }
    }
    return result;
}

inline void EndTaskWithMemory(TaskWithMemory* task)
{
    EndTemporaryMemory(task->memoryFlush);
    _WriteBarrier();
    task->beingUsed = false;
    task->dependOnGameMode = false;
}

inline void Clear( MemoryPool* pool )
{
    while( pool->currentBlock )
    {
        if( FreeLastBlock( pool ) )
        {
            break;
        }
    }
}

struct PoolBootstrapParams
{
    u64 flags;
    unm minimumBlockSize;
};

inline PoolBootstrapParams DefaultBootstrapParams()
{
    PoolBootstrapParams result = {};
    return result;
}

inline PoolBootstrapParams NonRestoredPool()
{
    PoolBootstrapParams result = DefaultBootstrapParams();
    result.flags = PlatformMemory_NotRestored;
    return result;
}

#define BootstrapPushStruct( type, member, ... ) ( type* ) BootstrapPushSize_( sizeof( type ), OffsetOf( type, member ), ##__VA_ARGS__ )
inline void* BootstrapPushSize_( unm size, unm offsetToPool, PoolBootstrapParams bootstrapParams = DefaultBootstrapParams(), PushParams params = DefaultPushParams() )
{
    MemoryPool bootstrap = {};
    bootstrap.bootstrapped = true;
    bootstrap.minimumBlockSize = bootstrapParams.minimumBlockSize;
    bootstrap.allocationFlags = bootstrapParams.flags;
    void* structPtr = PushSize_( &bootstrap, size, params );
    MemoryPool* dest = (MemoryPool*) ( ( u8* ) structPtr + offsetToPool );
    *dest = bootstrap;
    return structPtr;
}

inline Buffer PushBuffer(MemoryPool* pool, u32 size, PushParams params = DefaultPushParams())
{
    Buffer result;
    result.size = size;
    result.b = PushSize(pool, size, params);
    
    return result;
}

inline Buffer CopyBuffer(MemoryPool* pool, Buffer source, PushParams params = DefaultPushParams())
{
    Buffer result;
    result.b = (u8*) PushCopy(pool, source.size, source.b, params);
    result.size = source.size;
    
    return result;
}