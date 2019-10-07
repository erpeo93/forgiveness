inline void CreateRockVertex(RockDefinition* rockDefinition, ColoredVertex* vertexes, u32 vertexIndex, ColoredVertex* v0, ColoredVertex* v1, r32 offsetCoeff, RandomSequence* seq, Vec4 rockStartingColor)
{
    ColoredVertex* newVertex = vertexes + vertexIndex;
    
    Vec3 halfFrom0To1 = v0->P + 0.5f * (v1->P - v0->P);
    Vec3 P01 = halfFrom0To1 + (Normalize(halfFrom0To1) - halfFrom0To1) * RandomUni(seq) * offsetCoeff;
    newVertex->P = P01;
    
    r32 smoothness = rockDefinition->smoothness + RandomUni(seq) * rockDefinition->smoothnessDelta;
    smoothness = Clamp01(smoothness);
    r32 lerpValue = (smoothness * 0.5f);
    
    Vec4 modifiedColor = Lerp(v0->color, lerpValue, v1->color);
	Vec4 vanillaColor = rockStartingColor;
	r32 fallbackToStarting = 0;
    
	newVertex->color = Lerp(modifiedColor, fallbackToStarting, vanillaColor);
    
    
    if(RandomUni(seq) < rockDefinition->percentageOfMineralVertexes)
    {
        u32 mineralIndex = RandomChoice(seq, rockDefinition->mineralCount);
        RockMineral* mineral = rockDefinition->minerals + mineralIndex;
        r32 lerp = mineral->lerp + RandomBil(seq) * mineral->lerpDelta;
        newVertex->color = Lerp(newVertex->color, lerp, mineral->color);
    }
}




#if 0
// NOTE(Leonardo): API
internal void GenerateRock(RockComponent* dest, VertexModel* model, MemoryPool* tempPool, RandomSequence* seq, RockDefinition* rockDefinition)
{
    dest->dim = GetRockDim(rockDefinition, seq);
    
    m4x4 rotationMatrix = ZRotation(RandomUni(seq) * TAU32);
    
    u32 iterationCount = rockDefinition->iterationCount;
    u32 finalFaceCount = model->faceCount;
    
    iterationCount = Min(iterationCount, 4);
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
    
    Vec4 startColor = rockDefinition->color + RandomBil(seq) * rockDefinition->startingColorDelta;
    
    for(u32 startVertexIndex = 0; startVertexIndex < model->vertexCount; ++startVertexIndex)
    {
        ColoredVertex* vertex = tempVertexes + startVertexIndex;
        *vertex = model->vertexes[startVertexIndex];
        vertex->P.y *= RandomRangeFloat(seq, rockDefinition->minDisplacementY, rockDefinition->maxDisplacementY);
        vertex->P.z *= RandomRangeFloat(seq, rockDefinition->minDisplacementZ, rockDefinition->maxDisplacementZ);
        
        vertex->P = rotationMatrix * vertex->P;
        vertex->color = startColor + RandomBil(seq) * rockDefinition->perVertexColorDelta;
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
                CreateRockVertex(rockDefinition, tempVertexes, I01, v0, v1, offsetCoeff, seq, startColor);
            }
            
            u16 I12;
            if(!GetAndDeleteVertexIndexBetween(vertexHash, I1, I2, &I12))
            {
                I12 = vertexCount++;
                AddToVertexHash(vertexHash, I1, I2, I12, tempPool);
                CreateRockVertex(rockDefinition, tempVertexes, I12, v1, v2, offsetCoeff, seq, startColor);
            }
            
            
            u16 I20;
            if(!GetAndDeleteVertexIndexBetween(vertexHash, I2, I0, &I20))
            {
                I20 = vertexCount++;
                AddToVertexHash(vertexHash, I2, I0, I20, tempPool);
                CreateRockVertex(rockDefinition, tempVertexes, I20, v2, v0, offsetCoeff, seq, startColor);
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
    
    
    for(u32 faceIndex = 0; faceIndex < faceCount; ++faceIndex)
    {
        ModelFace* face = definitiveFaces + faceIndex;
        
        ColoredVertex* v0 = tempVertexes + face->i0;
        ColoredVertex* v1 = tempVertexes + face->i1;
        ColoredVertex* v2 = tempVertexes + face->i2;
        
        Vec3 N = Normalize(Cross(v1->P - v0->P, v2->P - v1->P));
        
        v0->N += N;
        v1->N += N;
        v2->N += N;
    }
    
    dest->faceCount = finalFaceCount;
    Assert(vertexCount <= ArrayCount(dest->vertexes));
    ColoredVertex* definitiveVertexes = dest->vertexes;
    dest->vertexCount = vertexCount;
    for(u32 vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex)
    {
        definitiveVertexes[vertexIndex] = tempVertexes[vertexIndex];
        definitiveVertexes[vertexIndex].N = Normalize(definitiveVertexes[vertexIndex].N);
    }
    
    r32 normalLerp = 1.0f - rockDefinition->normalSmoothness;
    for(u32 faceIndex = 0; faceIndex < faceCount; ++faceIndex)
    {
        ModelFace* face = definitiveFaces + faceIndex;
        
        ColoredVertex* v0 = definitiveVertexes + face->i0;
        ColoredVertex* v1 = definitiveVertexes + face->i1;
        ColoredVertex* v2 = definitiveVertexes + face->i2;
        
        Vec3 N = Normalize(Cross(v1->P - v0->P, v2->P - v1->P));
        
        v0->N = Lerp(v0->N, normalLerp, N);
        v1->N = Lerp(v1->N, normalLerp, N);
        v2->N = Lerp(v2->N, normalLerp, N);
    }
}
#endif
