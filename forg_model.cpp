inline VertexModel CreateModel(MemoryPool* pool, ColoredVertex* vertexes, u32 vertexCount, ModelFace* faces, u32 faceCount)
{
    VertexModel result;
    result.vertexCount = vertexCount;
    result.vertexes = PushArray(pool, ColoredVertex, vertexCount);
    
    result.faceCount = faceCount;
    result.faces = PushArray(pool, ModelFace, faceCount);
    
    for(u32 vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex)
    {
        result.vertexes[vertexIndex] = vertexes[vertexIndex];
    }
    
    for(u32 faceIndex = 0; faceIndex < faceCount; ++faceIndex)
    {
        result.faces[faceIndex] = faces[faceIndex];
    }
    
    return result;
}


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

inline void CreateVertex(ColoredVertex* vertexes, u32 vertexIndex, ColoredVertex* v0, ColoredVertex* v1, r32 offsetCoeff, RandomSequence* seq)
{
    ColoredVertex* newVertex = vertexes + vertexIndex;
    
    Vec3 halfFrom0To1 = v0->P + 0.5f * (v1->P - v0->P);
    Vec3 P01 = halfFrom0To1 + (Normalize(halfFrom0To1) - halfFrom0To1) * RandomUni(seq) * offsetCoeff;
    
    newVertex->P = P01;
    newVertex->color = Lerp(v0->color, 0.5f, v1->color);
}




// NOTE(Leonardo): API
internal void GenerateRock(ClientRock* dest, VertexModel* model, u32 iterationCount, MemoryPool* tempPool, RandomSequence* seq, Vec4 color)
{
    u32 finalFaceCount = model->faceCount;
    for(u32 iterationIndex = 0; iterationIndex < iterationCount; ++iterationIndex)
    {
        finalFaceCount *= 4;
    }
    Assert(finalFaceCount <= ArrayCount(dest->faces));
    
    ModelFace* tempFaces = PushArray(tempPool, ModelFace, finalFaceCount);
    ModelFace* definitiveFaces = dest->faces;
    
    ModelFace* ping = tempFaces;
    ModelFace* pong = definitiveFaces;
    
    for(u32 startFaceIndex = 0; startFaceIndex < model->faceCount; ++startFaceIndex)
    {
        tempFaces[startFaceIndex] = model->faces[startFaceIndex];
    }
    
    
    u32 maxVertexCountPossible = finalFaceCount * 3;
    ColoredVertex* tempVertexes = PushArray(tempPool, ColoredVertex, maxVertexCountPossible);
    
    for(u32 startVertexIndex = 0; startVertexIndex < model->vertexCount; ++startVertexIndex)
    {
        ColoredVertex* vertex = tempVertexes + startVertexIndex;
        *vertex = model->vertexes[startVertexIndex];
        vertex->P.y *= RandomRangeFloat(seq, 0.33f, 0.66f);
        vertex->P.z *= RandomRangeFloat(seq, 0.5f, 1.0f);
        vertex->color = color;
        
        r32 offset = RandomBil(seq) * 0.05f;
        vertex->color.r += offset;
        vertex->color.g += offset;
        vertex->color.b += offset;
        
        vertex->color = Clamp01(vertex->color);
    }
    
    
    
    u16 faceCount = SafeTruncateToU16(model->faceCount);
    u16 vertexCount = SafeTruncateToU16(model->vertexCount);
    
    VertexHash* vertexHash = PushStruct(tempPool, VertexHash);
    
    for(u32 iterationIndex = 0; iterationIndex < iterationCount; ++iterationIndex)
    {
        r32 offsetCoeff = 1.0f / (iterationIndex + 2);
        for(u32 faceIndex = 0; faceIndex < faceCount; ++faceIndex)
        {
            ModelFace* sourceFace = ping + faceIndex;
            ModelFace* destFace = pong + (faceIndex * 4);
            
            u16 I0 = sourceFace->i0;
            u16 I1 = sourceFace->i1;
            u16 I2 = sourceFace->i2;
            
            ColoredVertex* v0 = tempVertexes + I0;
            ColoredVertex* v1 = tempVertexes + I1;
            ColoredVertex* v2 = tempVertexes + I2;
            
            u16 I01;
            if(!GetAndDeleteVertexIndexBetween(vertexHash, I0, I1, &I01))
            {
                I01 = vertexCount++;
                AddToVertexHash(vertexHash, I0, I1, I01, tempPool);
                CreateVertex(tempVertexes, I01, v0, v1, offsetCoeff, seq);
            }
            
            u16 I12;
            if(!GetAndDeleteVertexIndexBetween(vertexHash, I1, I2, &I12))
            {
                I12 = vertexCount++;
                AddToVertexHash(vertexHash, I1, I2, I12, tempPool);
                CreateVertex(tempVertexes, I12, v1, v2, offsetCoeff, seq);
            }
            
            
            u16 I20;
            if(!GetAndDeleteVertexIndexBetween(vertexHash, I2, I0, &I20))
            {
                I20 = vertexCount++;
                AddToVertexHash(vertexHash, I2, I0, I20, tempPool);
                CreateVertex(tempVertexes, I20, v2, v0, offsetCoeff, seq);
            }
            
            
            ModelFace* face01 = destFace + 0;
            face01->i0 = I0;
            face01->i1 = I01;
            face01->i2 = I20;
            
            ModelFace* face12 = destFace + 1;
            face12->i0 = I1;
            face12->i1 = I12;
            face12->i2 = I01;
            
            ModelFace* face20 = destFace + 2;
            face20->i0 = I2;
            face20->i1 = I20;
            face20->i2 = I12;
            
            ModelFace* face012 = destFace + 3;
            face012->i0 = I01;
            face012->i1 = I12;
            face012->i2 = I20;
        }
        
        ModelFace* temp = ping;
        ping = pong;
        pong = temp;
        
        faceCount *= 4;
    }
    
    
    if((iterationCount % 2) == 0)
    {
        for(u32 faceIndex = 0; faceIndex < faceCount; ++faceIndex)
        {
            definitiveFaces[faceIndex] = tempFaces[faceIndex];
        }
    }
    dest->faceCount = finalFaceCount;
    
    Assert(vertexCount <= ArrayCount(dest->vertexes));
    ColoredVertex* definitiveVertexes = dest->vertexes;
    dest->vertexCount = vertexCount;
    for(u32 vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex)
    {
        definitiveVertexes[vertexIndex] = tempVertexes[vertexIndex];
    }
}


