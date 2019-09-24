internal void* Acquire_(ResizableArray* array, u32* index)
{
    Assert(array->count < array->maxCount);
    Assert(array->count);
    
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

internal void* GetOrAcquire_(ResizableArray* array, u32 index)
{
    Assert(index < array->maxCount);
    array->count = index + 1;
    void* result = Get_(array, index);
    if(!result)
    {
        InvalidCodePath;
        // TODO(Leonardo): reallocate entire array!
    }
    return result;
}

internal ResizableArray InitResizableArray_(MemoryPool* pool, u32 elementSize, u32 maxElementCount)
{
    ResizableArray result = {};
    result.elementSize = elementSize;
    result.count = 1;
    result.maxCount = maxElementCount;
    result.memory = PushSize(pool, elementSize * maxElementCount, NoClear());
    
    return result;
}