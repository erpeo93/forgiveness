
enum LoopingFlags
{
    LoopingFlag_AllocatedDuringLooping = ( 1 << 0 ),
    LoopingFlag_FreedDuringLooping = ( 1 << 1 ),
};

struct Win32MemoryBlock
{
    PlatformMemoryBlock block;
    Win32MemoryBlock* prev;
    Win32MemoryBlock* next;
    u64 loopingFlags;
};

struct Win32SavedMemoryBlock
{
    u64 basePointer;
    u64 size;
};
