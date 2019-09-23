#pragma once

struct ResizableArray
{
    u32 elementSize;
    u32 count;
    u32 maxCount;
    u8* memory;
};
