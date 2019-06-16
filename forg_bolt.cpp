inline b32 ValidVector(Vec3 original, Vec3 pick)
{
    b32 result = true;
    if(pick.x == 0.0f && pick.y == 0.0f && pick.z == 0.0f)
    {
        result = false;
    }
    else
    {
        r32 dot = Dot(Normalize(original), Normalize(pick));
        result = (dot != 1.0f); 
    }
    
    return result;
}

internal void UpdateAndRenderBolt(GameModeWorld* worldMode, BoltCache* cache, RenderGroup* group, BoltDefinition* definition, Bolt* bolt, r32 timeToAdvance)
{
    TempMemory subdivisionsMemory = BeginTemporaryMemory(&cache->pool);
    
    u32 subdivisions = definition->subdivisions;
    subdivisions = Min(subdivisions, MAX_BOLT_SUBDIVISIONS);
    Vec3* subdivisionPoints = PushArray(&cache->pool, Vec3, subdivisions);
    
    Vec3 deltaP = bolt->endP - bolt->startP;
    Vec3 deltaPerSegment = deltaP / (r32) subdivisions;
    
    
    b32 computeAnimationOffsets = (bolt->timeSinceLastAnimationTick >= definition->animationTick);
    if(computeAnimationOffsets)
    {
        bolt->timeSinceLastAnimationTick = 0.0f;
    }
    
    Lights lights = GetLights(worldMode, bolt->startP + 0.5f * deltaP);
    
    
    AddLightToGridNextFrame(worldMode, bolt->startP + 0.5f * deltaP, definition->lightColor, definition->lightIntensity);
    
    
    
    RandomSequence seq = Seed(bolt->seed);
    for(u32 subIndex = 0; subIndex < subdivisions - 1; ++subIndex)
    {
        Vec3 parallel = (r32) (subIndex + 1) * deltaPerSegment;
        
        Vec3 random = {};
        while(!ValidVector(parallel, random))
        {
            random = RandomBilV3(&seq);
        }
        Vec3 perpStructure = Cross(parallel, definition->magnitudoStructure * random);
        
        if(computeAnimationOffsets)
        {
            Vec3 randomAnimation = {};
            while(!ValidVector(parallel, randomAnimation))
            {
                randomAnimation = RandomBilV3(&bolt->animationSeq);
            }
            bolt->subdivisionAnimationOffsets[subIndex] = Cross(parallel, definition->magnitudoAnimation * randomAnimation);
        }
        
        subdivisionPoints[subIndex] = bolt->startP + parallel + perpStructure + bolt->subdivisionAnimationOffsets[subIndex];
    }
    subdivisionPoints[subdivisions - 1] = bolt->endP;
    
    
    
    
    Vec3 subStartP = bolt->startP;
    for(u32 subdivisionIndex = 0; subdivisionIndex < subdivisions; ++subdivisionIndex)
    {
        Vec3 subEndP = subdivisionPoints[subdivisionIndex];
        
        Vec4 color = definition->color;
        
        r32 segmentAlpha = 1.0f;
        if(bolt->ttl > (definition->ttl - definition->fadeinTime))
        {
            r32 fadeinAvailableTime = definition->fadeinTime;
            r32 segmentFadeinTime = fadeinAvailableTime / subdivisions;
            
            r32 segmentMinAlphaTTL = definition->ttl - subdivisionIndex * segmentFadeinTime;
            r32 segmentMaxAlphaTTL = definition->ttl - (subdivisionIndex + 1) * segmentFadeinTime;
            
            segmentAlpha = Clamp01MapToRange(segmentMinAlphaTTL, bolt->ttl, segmentMaxAlphaTTL);
        }
        else if(bolt->ttl < definition->fadeoutTime)
        {
            r32 fadeoutAvailableTime = definition->fadeoutTime;
            r32 segmentFadeoutTime = fadeoutAvailableTime / subdivisions;
            
            r32 segmentMaxAlphaTTL = definition->fadeoutTime - subdivisionIndex * segmentFadeoutTime;
            r32 segmentMinAlphaTTL = definition->fadeoutTime - (subdivisionIndex + 1) * segmentFadeoutTime;
            
            segmentAlpha = 1.0f - Clamp01MapToRange(segmentMaxAlphaTTL, bolt->ttl, segmentMinAlphaTTL);
        }
        
        color.a = segmentAlpha;
        
        PushLineSegment(group, group->whiteTexture, color, subStartP, subEndP, definition->thickness, lights);
        subStartP = subEndP;
    }
    
    if(bolt->ttl <= definition->lightStartTime && bolt->ttl >= definition->lightEndTime)
    {
        AddLightToGridNextFrame(worldMode, bolt->endP, definition->lightColor, definition->lightIntensity);
    }
    
    EndTemporaryMemory(subdivisionsMemory);
}

internal void UpdateAndRenderBolts(GameModeWorld* worldMode, BoltCache* cache, r32 timeToUpdate, RenderGroup* group)
{
    for(Bolt** boltPtr = &cache->firstBolt; *boltPtr;)
    {
        Bolt* bolt = *boltPtr;
        
        bolt->ttl -= timeToUpdate;
        bolt->timeSinceLastAnimationTick += timeToUpdate;
        
        if(bolt->ttl <= 0.0f)
        {
            *boltPtr = bolt->next;
            FREELIST_DEALLOC(bolt, cache->firstFreeBolt);
        }
        else
        {
            bolt->startP += cache->deltaP;
            bolt->endP += cache->deltaP;
            UpdateAndRenderBolt(worldMode, cache, group, &cache->definition, bolt, timeToUpdate);
            
            boltPtr = &bolt->next;
        }
    }
}

inline void SpawnBolt(BoltCache* cache, Vec3 startP, Vec3 endP)
{
    BoltDefinition* definition = &cache->definition;
    
    Bolt* bolt;
    FREELIST_ALLOC(bolt, cache->firstFreeBolt, PushStruct(&cache->pool, Bolt));
    bolt->ttl = definition->ttl;
    bolt->seed = GetNextUInt32(&cache->seedSource);
    bolt->timeSinceLastAnimationTick = R32_MAX;
    bolt->animationSeq = Seed(bolt->seed);
    bolt->startP = startP;
    bolt->endP = endP;
    
    FREELIST_INSERT(bolt, cache->firstBolt);
}

internal void InitBoltCache(BoltCache* cache, u32 seed)
{
    cache->seedSource = Seed(seed);
    
    
    BoltDefinition definition = {};
    definition.animationTick = 0.01f;
    definition.ttl = 0.3f;
    definition.fadeinTime = 0.1f;
    definition.fadeoutTime = 0.05f;
    definition.color = V4(1, 1, 0, 1);
    definition.thickness = 0.024f;
    definition.magnitudoStructure = 0.1f;
    definition.magnitudoAnimation = 0.03f;
    definition.subdivisions = 8;
    
    definition.lightColor = V3(1, 1, 1);
    definition.lightIntensity = 2.0f;
    definition.lightStartTime = 0.2f;
    definition.lightEndTime = 0.0f;
    
    cache->definition = definition;
    
}