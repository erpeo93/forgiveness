#pragma once

struct ResizableArray
{
    MemoryPool p1;
    MemoryPool p2;
    MemoryPool* currentPool;
    
    
    u32 elementSize;
    u32 count;
    u32 maxCount;
    u8* memory;
};
