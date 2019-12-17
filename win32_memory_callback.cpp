inline void Win32FreeMemoryBlock(Win32MemoryBlock* block)
{
    BeginTicketMutex(&memoryMutex);
    block->prev->next = block->next;
    block->next->prev = block->prev;
    EndTicketMutex( &memoryMutex );
    
    BOOL result = VirtualFree( block, 0, MEM_RELEASE );
    Assert( result );
}

//PlatformMemoryBlock* name( memory_index size, u64 flags )
PLATFORM_ALLOCATE_MEMORY(Win32AllocateMemory)
{
    Assert( sizeof( Win32MemoryBlock ) == 64);
    
    unm pageSize = 4096;
    unm totalSize = size + sizeof( Win32MemoryBlock );
    unm baseOffset = sizeof( Win32MemoryBlock );
    unm protectOffset = 0;
    
    if( flags & PlatformMemory_UnderflowCheck )
    {
        totalSize = size + 2 * pageSize;
        baseOffset = 2 * pageSize;
        protectOffset = pageSize;
    }
    else if( flags & PlatformMemory_OverflowCheck )
    {
        unm sizeRounded = AlignPow2( size, pageSize );
        totalSize = size + 2 * pageSize;
        baseOffset = pageSize + sizeRounded - size;
        protectOffset = pageSize + sizeRounded;
    }
    
    Win32MemoryBlock* block = ( Win32MemoryBlock* ) VirtualAlloc( 0, totalSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE );
    Assert( block );
    block->block.base = ( u8* ) block + baseOffset;
    Assert( block->block.used == 0 );
    Assert( block->block.poolPrev == 0 );
    
    if( flags & ( PlatformMemory_UnderflowCheck | PlatformMemory_OverflowCheck ) )
    {
        DWORD oldProtect;
        BOOL protect = VirtualProtect( ( u8* ) block + protectOffset, pageSize, PAGE_NOACCESS, &oldProtect );
        Assert( protect );
    }
    
    Win32MemoryBlock* sentinel = &globalMemorySentinel;
    block->next = sentinel;
    
    BeginTicketMutex( &memoryMutex );
    block->prev = sentinel->prev;
    block->next->prev = block;
    block->prev->next = block;
    EndTicketMutex( &memoryMutex );
    
    block->block.size = size;
    block->block.flags = flags;
    block->loopingFlags = IsInLoop() ? LoopingFlag_AllocatedDuringLooping : 0;
    
    PlatformMemoryBlock* result = &block->block;
    return result;
}

//void name( PlatformMemoryBlock* block )
PLATFORM_DEALLOCATE_MEMORY(Win32DeallocateMemory)
{
    if(block)
    {
        Win32MemoryBlock* win32Block = ( Win32MemoryBlock* ) block;
        if(IsInLoop())
        {
            win32Block->loopingFlags = LoopingFlag_FreedDuringLooping;
        }
        else
        {
            Win32FreeMemoryBlock( win32Block );
        }
    }
}

DEBUG_PLATFORM_MEMORY_STATS( Win32GetMemoryStats )
{
    DebugPlatformMemoryStats stats = {};
    Win32MemoryBlock* sentinel = &globalMemorySentinel;
    
    BeginTicketMutex( &memoryMutex );
    for( Win32MemoryBlock* block = sentinel->next; block != sentinel; block = block->next )
    {
        Assert( block->block.size < U32_MAX );
        
        ++stats.blockCount;
        stats.totalSize += block->block.size;
        stats.totalUsed += block->block.used;
    }
    EndTicketMutex( &memoryMutex );
    
    return stats;
}