#pragma once
struct MemoryPool
{
    b32 bootstrapped;
    PlatformMemoryBlock* currentBlock;
    unm minimumBlockSize;
    
    u32 tempCount;
    u32 blockCount;
    u64 allocationFlags;
};

struct TempMemory
{
    MemoryPool* pool;
    PlatformMemoryBlock* block;
    memory_index used;
};

struct TaskWithMemory
{
    MemoryPool pool;
    b32 beingUsed;
    b32 dependOnGameMode;
    TempMemory memoryFlush;
};
