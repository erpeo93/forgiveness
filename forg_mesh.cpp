struct VertexHashSlot
{
    b32 valid;
    
    u16 I0;
    u16 I1;
    u16 inBetweenIndex;
    
    VertexHashSlot* nextInHash;
};

struct VertexHash
{
    VertexHashSlot* slots[128];
};

inline u32 ComputeHashVertexIndex(u16 I0, u16 I1)
{
    u32 result = I0 * I1;
    return result;
}

inline b32 GetAndDeleteVertexIndexBetween(VertexHash* hash, u16 I0, u16 I1, u16* output)
{
    if(I1 < I0)
    {
        u16 temp = I0;
        I0 = I1;
        I1 = temp;
    }
    
    b32 result = false;
    
    u32 hashValue = ComputeHashVertexIndex(I0, I1);
    u32 index = hashValue & (ArrayCount(hash->slots) - 1);
    
    VertexHashSlot* slot = hash->slots[index];
    while(slot)
    {
        if(slot->valid && slot->I0 == I0 && slot->I1 == I1)
        {
            result = true;
            *output = slot->inBetweenIndex;
            slot->valid = false;
            
            break;
        }
        
        slot = slot->nextInHash;
    }
    
    return result;
}

inline void AddToVertexHash(VertexHash* hash, u16 I0, u16 I1, u16 I01, MemoryPool* tempPool)
{
    if(I1 < I0)
    {
        u16 temp = I0;
        I0 = I1;
        I1 = temp;
    }
    
    u32 hashValue = ComputeHashVertexIndex(I0, I1);
    u32 index = hashValue & (ArrayCount(hash->slots) - 1);
    VertexHashSlot* slot = hash->slots[index];
    VertexHashSlot* free = 0;
    
    while(slot)
    {
        if(!slot->valid)
        {
            free = slot;
            break;
        }
        
        slot = slot->nextInHash;
    }
    
    if(!free)
    {
        free = PushStruct(tempPool, VertexHashSlot);
        free->nextInHash = hash->slots[index];
        hash->slots[index] = free;
    }
    
    free->valid = true;
    free->I0 = I0;
    free->I1 = I1;
    free->inBetweenIndex = I01;
}



