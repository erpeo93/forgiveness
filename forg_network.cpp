inline NetworkBuffer ForgAllocateNetworkBuffer(MemoryPool* pool, u32 size)
{
    NetworkBuffer result = {};
    result.size = size;
    result.buffer = PushArray(pool, u8, size);
    
    Assert(result.buffer);
    return result;
}
