internal void* GetInternal_(ResizableArray* array, u32 index)
{
    void* result = 0;
    if(index && index < array->count)
    {
        result = AdvanceVoidPtrBytes(array->memory, index * array->internalElementSize);
    }
    
    return result;
}

internal void* Get_(ResizableArray* array, u32 index)
{
    void* result = GetInternal_(array, index);
    if(result)
    {
        result = AdvanceVoidPtrBytes(result, array->internalElementSize - array->elementSize);
    }
    return result;
}

internal void Free_(ResizableArray* array, u32 index)
{
    void* toFree = GetInternal_(array, index);
    *(u32*) toFree = array->firstFree;
    array->firstFree = index;
}

internal b32 Deleted_(ResizableArray* array, u32 index)
{
    void* element = GetInternal_(array, index);
    b32 result = (*(u32*) element != 0);
    return result;
}

internal void* Acquire_(ResizableArray* array, u32* index, u32 max)
{
    Assert(array->count);
    Assert(array->count <= array->maxCount);
    
    u32 acquired = 0;
    if(array->firstFree > 0 && array->firstFree != 0xffffffff)
    {
        acquired = array->firstFree;
        void* element = GetInternal_(array, acquired);
        array->firstFree = *(u32*) element;
        *(u32*)element = 0;
    }
    else
    {
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
            u32 toCopySize = array->internalElementSize * array->count;
            
            array->maxCount = array->count * 2;
            array->currentPool = newPool;
            
            array->memory = PushSize(newPool, array->maxCount * array->internalElementSize);
            Copy(toCopySize, array->memory, temp);
            
            Clear(toFree);
        }
        
        Assert(array->count < array->maxCount);
        acquired = array->count++;
    }
    
    Assert(acquired);
    if(index)
    {
        Assert(acquired <= max);
        *index = (*index | acquired);
    }
    
    void* result = Get_(array, acquired);
    return result;
}

internal ResizableArray InitResizableArray_(u32 elementSize, u32 maxElementCount)
{
    ResizableArray result = {};
    result.internalElementSize = elementSize + sizeof(u32);
    result.elementSize = elementSize;
    result.count = 1;
    result.maxCount = maxElementCount;
    
    result.firstFree = 0xffffffff;
    
    result.currentPool = &result.p1;
    result.memory = PushSize(result.currentPool, result.internalElementSize * maxElementCount, NoClear());
    
    return result;
}