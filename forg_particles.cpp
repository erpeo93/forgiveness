#define MMSetRandomize(seq, value, magnitude) _mm_set_ps(value + RandomBil(seq) * magnitude, value + RandomBil(seq) * magnitude, value + RandomBil(seq) * magnitude, value + RandomBil(seq) * magnitude);
#define MMSetRandomUni(seq) _mm_set_ps(RandomUni(seq), RandomUni(seq), RandomUni(seq), RandomUni(seq))

internal void SpawnParticles(ParticleCache* cache, ParticleEffectInstance* effect, r32 dt)
{
    RandomSequence* entropy = &cache->particleEntropy;
    Vec3 atP = effect->P - cache->deltaParticleP;
    
    ParticleEmitter* emitter = &effect->emitter;
    
    
    r32 particleCount = emitter->particlesPerSec * dt + effect->spawnParticlesLeftOff;
    u32 particle4xCount = (u32) (particleCount / 4.0f);
    effect->spawnParticlesLeftOff = particleCount - (r32) (particle4xCount * 4);
    effect->particle4xCount += particle4xCount;
    
    for( u32 newParticle = 0; newParticle < particle4xCount; newParticle++ )
    {
        Particle_4x *A;
        FREELIST_ALLOC(A, cache->firstFreeParticle4x, PushStruct(cache->pool, Particle_4x, AlignNoClear(128)));
        FREELIST_INSERT(A, effect->firstParticle);
        
        r32 ttl = emitter->lifeTime + RandomBil(entropy) * emitter->lifeTimeV;
        A->ttl4x = MMSetExpr(ttl);
        A->randomUni4x = MMSetRandomUni(entropy);
        
        A->P.x = MMSetRandomize(entropy, atP.x, emitter->startPV.x);
        A->P.y = MMSetRandomize(entropy, atP.y, emitter->startPV.y);
        A->P.z = MMSetRandomize(entropy, atP.z, emitter->startPV.z);
        
        Vec3 dP = Lerp(emitter->dP, emitter->lerpWithUpVector, effect->UpVector);
        
        A->dP.x = MMSetRandomize(entropy, dP.x, emitter->dPV.x);
        A->dP.y = MMSetRandomize(entropy, dP.y, emitter->dPV.y);
        A->dP.z = MMSetRandomize(entropy, dP.z, emitter->dPV.z);
        
        A->C.r = MMSetRandomize(entropy,  emitter->C.r, emitter->CV.r);
        A->C.g = MMSetRandomize(entropy,  emitter->C.g, emitter->CV.g);
        A->C.b = MMSetRandomize(entropy,  emitter->C.b, emitter->CV.b);
        A->C.a = MMSetRandomize(entropy,  emitter->C.a, emitter->CV.a);
        
        A->dC.r = MMSetRandomize(entropy, emitter->dC.r, emitter->dCV.r);
        A->dC.g = MMSetRandomize(entropy, emitter->dC.g, emitter->dCV.g);
        A->dC.b = MMSetRandomize(entropy, emitter->dC.b, emitter->dCV.b);
        A->dC.a = MMSetRandomize(entropy, emitter->dC.a, emitter->dCV.a);
        
        r32 angle = DegToRad(emitter->angle);
        r32 angleV = DegToRad(emitter->angleV);
        A->angle4x = MMSetRandomize(entropy, angle, angleV); 
        A->scaleX4x = MMSetRandomize(entropy, emitter->scaleX, emitter->scaleXV);
        A->scaleY4x = MMSetRandomize(entropy, emitter->scaleY, emitter->scaleYV);
    }
}

inline Vec3 GetPhaseEndP(ParticleEffectInstance* effect, Vec3 startP, u32 phaseIndex)
{
    Vec3 result;
    
    Assert(phaseIndex < effect->phaseCount);
    ParticleUpdater* updater = &effect->phases[phaseIndex].updater;
	if(phaseIndex == effect->phaseCount - 1)
    {
        result = effect->P + effect->UpVector;
    }
    else
    {
        result = startP;
    }
	return result;
}

inline Vec3 GetPhaseStartP(ParticleEffectInstance* effect, u32 phaseIndex)
{
    ParticleUpdater* updater = &effect->phases[phaseIndex].updater;
	Vec3 result = effect->P;
	if(phaseIndex == 0)
	{
		result = effect->P;
	}
	else
	{
        Vec3 prevStartP = GetPhaseStartP(effect, phaseIndex - 1);
		result = GetPhaseEndP(effect, prevStartP, phaseIndex);
	}
    
	return result;
}

inline ParticlePhase* GetPhase(ParticleEffectInstance* effect, r32 ttl, u32* phaseIndexOut, r32* normPhaseTime)
{
    ParticlePhase* result = 0;
    
    r32 runningTTL = 0;
    for(u32 phaseIndex = 0; phaseIndex < effect->phaseCount; ++phaseIndex)
    {
        ParticlePhase* phase = effect->phases + phaseIndex;
        r32 nextTTL = runningTTL + phase->ttl;
        if(ttl >= runningTTL && ttl < nextTTL)
        {
            *normPhaseTime = Clamp01MapToRange(runningTTL, ttl, nextTTL);
            result = phase;
            *phaseIndexOut = phaseIndex;
            break;
        }
        
        runningTTL = nextTTL;
    }
    return result;
}

inline Lights GetLights(GameModeWorld* worldMode, Vec3 P);
inline void UpdateAndRenderParticle4x(GameModeWorld* worldMode, ParticleEffectInstance* effect,ParticlePhase* phase, ParticleUpdater* updater, u32 phaseIndex, r32 normPhaseTime, Particle_4x* A, RenderGroup* group, V3_4x frameDisplacement, r32 dt)
{
    __m128 dt4x = MMSetExpr(dt);
    
    V3_4x finalParticleP = {};
    
    if(updater->sineUpdater)
    {
        Vec3 startP = GetPhaseStartP(effect, phaseIndex);
        Vec3 destP = GetPhaseEndP(effect, startP, phaseIndex);
        
        Vec3 deltaP = destP - startP;
        V3_4x dP = ToV3_4x(deltaP);
        
        Vec2 horizontalPlane = deltaP.xy;
        r32 zRotation = RadiantsBetweenVectors(V2(1, 0), horizontalPlane);
        Vec2 verticalPlane = V2(deltaP.x, deltaP.z);
        r32 yRotation = RadiantsBetweenVectors(V2(1, 0), verticalPlane);
        m4x4 matrix = ZRotation(zRotation) * YRotation(yRotation);
        Vec3 upVector = GetColumn(matrix, 2);
        upVector.z = Abs(upVector.z);
        
        r32 particleAngle = 0;
        if(updater->sineSubdivisions > 1)
        {
            r32 subdivisionAngle = TAU32 / (r32) updater->sineSubdivisions;
            u32 subIndex = RoundReal32ToU32(M(A->randomUni4x, 0) * (updater->sineSubdivisions - 1));
            particleAngle = subIndex * subdivisionAngle;
        }
        r32 angle = particleAngle + effect->updaterAngle;
        
        upVector = RotateVectorAroundAxis(upVector, deltaP, angle);
        V3_4x upVector4x = ToV3_4x(upVector);
        
        r32 normRadiants = normPhaseTime * M(updater->totalRadiants, 0);
        r32 radiantsSine = Sin(normRadiants);
        
        A->P += frameDisplacement;
        finalParticleP = A->P + normPhaseTime * dP + radiantsSine * upVector4x;
        
        V4_4x ddC = ToV4_4x(updater->ddC);
        __m128 dScaleX = MMSetExpr(updater->dScaleX);
        __m128 dScaleY = MMSetExpr(updater->dScaleY);
        __m128 dAngle = MMSetExpr(updater->dAngle);
        
        A->C += ((0.5f * Square(dt) * ddC) + (dt * A->dC));
        A->dC += dt * ddC;
        A->C = Clamp01(A->C);
        
        A->scaleX4x += dt4x * dScaleX;
        A->scaleY4x += dt4x * dScaleY;
        A->angle4x += dt4x * dAngle;
    }
    else
    {
        V4_4x ddC = ToV4_4x(updater->ddC);
        V3_4x ddP = ToV3_4x(updater->finalAcceleration);
        __m128 dScaleX = MMSetExpr(updater->dScaleX);
        __m128 dScaleY = MMSetExpr(updater->dScaleY);
        __m128 dAngle = MMSetExpr(updater->dAngle);
        
        A->P += frameDisplacement;
        A->P += ((0.5f * Square(dt) * ddP) + (dt * A->dP));
        A->dP += dt * ddP;
        finalParticleP = A->P;
        
        A->C += ((0.5f * Square(dt) * ddC) + (dt * A->dC));
        A->dC += dt * ddC;
        A->C = Clamp01(A->C);
        
        A->scaleX4x += dt4x * dScaleX;
        A->scaleY4x += dt4x * dScaleY;
        A->angle4x += dt4x * dAngle;
    }
    
    Vec3 lightP = 
    { 
        M(finalParticleP.x, 0),
        M(finalParticleP.y, 0),
        M(finalParticleP.z, 0)
    };
    
    if(updater->texture && IsValid(updater->texture))
    {
        Lights lights = GetLights(worldMode, lightP);
        for(u32 subIndex = 0; subIndex < 4; ++subIndex)
        {
            Vec3 P = 
            { 
                M(finalParticleP.x, subIndex),
                M(finalParticleP.y, subIndex),
                M(finalParticleP.z, subIndex)
            };
            
            Vec4 color = 
            {
                M(A->C.r, subIndex),
                M(A->C.g, subIndex),
                M(A->C.b, subIndex),
                M(A->C.a, subIndex)
            };
            
            r32 angle = M(A->angle4x, subIndex);
            r32 scaleX = M(A->scaleX4x, subIndex);
            r32 scaleY = M(A->scaleY4x, subIndex);
            
            r32 angleRad = DegToRad(angle);
            Vec3 XAxis = V3(Cos(angleRad), Sin(angleRad), 0.0f);
            Vec3 YAxis  = V3(Perp(XAxis.xy), 0.0f);
            
            Vec4 lateral = 0.5f * scaleX * (XAxis.x * magicLateralVector + XAxis.y * magicUpVector);
            Vec4 up = 0.5f * scaleY * (YAxis.x * magicLateralVector + YAxis.y * magicUpVector);
            
            u32 C = RGBAPack8x4(color * 255.0f);
            
            Vec4 windInfluences = V4(0.1f, 0.1f, 0.1f, 0.1f);
            PushMagicQuad(group, Slice_Standard, V4(P, 0), lateral, up, C, *updater->texture, lights, 0, 0, 1, windInfluences);
        }
    }
}

internal void UpdateAndRenderEffect(GameModeWorld* worldMode, ParticleCache* cache, ParticleEffectInstance* effect, r32 dt, Vec3 frameDisplacementInit, RenderGroup* group)
{
    for(u32 phaseIndex = 0; phaseIndex < effect->phaseCount; ++phaseIndex)
    {
        ParticlePhase* phase = effect->phases + phaseIndex;
        ParticleUpdater* updater = &phase->updater;
        updater->finalAcceleration = updater->ddP + worldMode->windStrength * worldMode->windDirection;
        if(IsValid(updater->bitmapID))
        {
            updater->texture = 0;
            Bitmap* bitmap = GetBitmap(group->assets, updater->bitmapID).bitmap;
            if(bitmap)
            {
                LockAssetForCurrentFrame(group->assets, updater->bitmapID);
                updater->texture = &bitmap->textureHandle;
            }
            else
            {
                LoadBitmap(group->assets, updater->bitmapID);
            }
        }
    }
    
	effect->updaterAngle += effect->dAngleSineUpdaters * dt;
    if(effect->active)
    {
        SpawnParticles(cache, effect, dt);
    }
    
    V3_4x frameDisplacement = ToV3_4x(frameDisplacementInit);
    for(Particle_4x** particlePtr = &effect->firstParticle; *particlePtr;)
    {
        Particle_4x* particle = *particlePtr;
        
        particle->ttl4x = _mm_sub_ps(particle->ttl4x, MMSetExpr(dt));
        
        if(M(particle->ttl4x, 0) <= 0.0f)
        {
            Assert(effect->particle4xCount > 0);
            --effect->particle4xCount;
            
            *particlePtr = particle->next;
            FREELIST_DEALLOC(particle, cache->firstFreeParticle4x);
        }
        else
        {
            r32 ttl = M(particle->ttl4x, 0);
            u32 phaseIndex;
            r32 normPhaseTime = 0;
            ParticlePhase* phase = GetPhase(effect, ttl, &phaseIndex, &normPhaseTime);
            if(phase)
            {
                ParticleUpdater* updater = &phase->updater;
                u32 subtype = GetAssetSubtype(group->assets, AssetType_Image, updater->asset.subtypeHash);
                updater->bitmapID = QueryBitmaps(group->assets, subtype, 0, 0);
                
                UpdateAndRenderParticle4x(worldMode, effect, phase, updater, phaseIndex,normPhaseTime, particle, group, frameDisplacement, dt);
            }
            particlePtr = &particle->next;
        }
    }
}

internal void SetPosition(ParticleEffectInstance* effect, Vec3 P)
{
    effect->P = P;
}


inline void FreeParticleEffect(ParticleEffectInstance* effect)
{
    Assert(effect->active);
    effect->active = false;
}

inline ParticleEffectInstance* GetNewParticleEffect(ParticleCache* cache, ParticleEffect* definition, Vec3 startP, Vec3 UpVector)
{
    ParticleEffectInstance* result;
    FREELIST_ALLOC(result, cache->firstFreeEffect, PushStruct(cache->pool, ParticleEffectInstance));
    
    result->active = true;
    SetPosition(result, startP);
    result->UpVector = UpVector;
    result->spawnParticlesLeftOff = 0;
    
    result->emitter = definition->emitter;
    result->dAngleSineUpdaters = definition->dAngleSineUpdaters;
    result->phaseCount = 0;
    for(u32 phaseIndex = 0; phaseIndex < definition->phaseCount; ++phaseIndex)
    {
        if(result->phaseCount < ArrayCount(result->phases))
        {
            ParticlePhase* phase = result->phases + result->phaseCount++;
            *phase = definition->phases[phaseIndex];
        }
    }
    Assert(result->particle4xCount == 0);
    Assert(!result->firstParticle);
    
    FREELIST_INSERT(result, cache->firstActiveEffect);
    
    return result;
}

internal void UpdateAndRenderParticleEffects(GameModeWorld* worldMode, ParticleCache* particleCache, r32 dt, RenderGroup* group)
{
    Vec3 frameDisplacement = particleCache->deltaParticleP;
    for(ParticleEffectInstance** effectPtr = &particleCache->firstActiveEffect; *effectPtr;)
    {
        ParticleEffectInstance* effect = *effectPtr;
        if(!effect->active && effect->particle4xCount == 0)
        {
            *effectPtr = effect->next;
            FREELIST_DEALLOC(effect, particleCache->firstFreeEffect);
        }
        else
        {
            UpdateAndRenderEffect(worldMode, particleCache, effect, dt, frameDisplacement, group);
            effectPtr = &effect->next;
        }
    }
}

internal void InitParticleCache(ParticleCache* particleCache, Assets* assets, MemoryPool* pool)
{
    particleCache->particleEntropy = Seed(1234);
    particleCache->deltaParticleP = {};
    particleCache->pool = pool;
}
