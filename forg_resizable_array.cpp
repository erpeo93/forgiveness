internal void* Acquire_(ResizableArray* array, u32* index)
{
    Assert(array->count);
    Assert(array->count <= array->maxCount);
    
    if(array->count == array->maxCount)
    {
        MemoryPool* newPool = &array->p2;
        MemoryPool* toFree = &array->p1;
        if(array->currentPool == &array->p2)
        {
            newPool = &array->p1;
            toFree = &array->p2;
        }
        
        u8* temp = array->memory;
        u32 toCopySize = array->elementSize * array->count;
        
        array->maxCount = array->count * 2;
        array->currentPool = newPool;
        
        array->memory = PushSize(newPool, array->maxCount * array->elementSize);
        Copy(toCopySize, array->memory, temp);
        
        Clear(toFree);
    }
    
    Assert(array->count < array->maxCount);
    u32 newIndex = array->count++;
    if(index)
    {
        *index = newIndex;
    }
    
    void* result = AdvanceVoidPtrBytes(array->memory, newIndex * array->elementSize);
    return result;
}

internal void* Get_(ResizableArray* array, u32 index)
{
    void* result = 0;
    if(index && index < array->count)
    {
        result = AdvanceVoidPtrBytes(array->memory, index * array->elementSize);
    }
    
    return result;
}

internal ResizableArray InitResizableArray_(u32 elementSize, u32 maxElementCount)
{
    ResizableArray result = {};
    result.elementSize = elementSize;
    result.count = 1;
    result.maxCount = maxElementCount;
    
    result.currentPool = &result.p1;
    result.memory = PushSize(result.currentPool, elementSize * maxElementCount, NoClear());
    
    return result;
}