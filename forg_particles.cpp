internal r32 GetSpeedNorm(ParticleEffectInstance* effect)
{
    r32 result = Clamp01(LengthSq(effect->speed) / Square(effect->maxSpeedMagnitudo));
    return result;
}


#define MMSetRandomize(seq, value, magnitude) _mm_set_ps(value + RandomBil(seq) * magnitude, value + RandomBil(seq) * magnitude, value + RandomBil(seq) * magnitude, value + RandomBil(seq) * magnitude);
#define MMSetRandomUni(seq) _mm_set_ps(RandomUni(seq), RandomUni(seq), RandomUni(seq), RandomUni(seq))

internal void SpawnParticles(ParticleCache* cache, ParticleEffectInstance* effect, r32 dt)
{
    RandomSequence* entropy = &cache->entropy;
    Vec3 baseP = effect->P - cache->deltaParticleP;
    
    ParticleEmitter* emitter = &effect->emitter;
    
    Vec3 atP = baseP;
    r32 particleCount = 0;
    
    Vec3 orientation = emitter->defaultOrientation;
    if(LengthSq(effect->speed) > 0)
    {
        orientation = effect->speed;
    }
    
    r32 speedNorm = GetSpeedNorm(effect);
    switch(emitter->emissionType)
    {
        case Emission_RateOverTime:
        {
            r32 speed = emitter->particlesPerSec;
            if(emitter->modulateWithVelocity)
            {
                speed *= speedNorm;
            }
            
            particleCount = speed * dt;
        } break;
        
        case Emission_BurstOverTime:
        {
            r32 speed = 1.0f;
            if(emitter->modulateWithVelocity)
            {
                speed *= speedNorm;
            }
            
            emitter->accumulatedTime += speed * dt;
            if(emitter->accumulatedTime >= emitter->targetAccumulatedTime)
            {
                particleCount = emitter->particleCount + RandomBil(entropy) * emitter->particleCountV;
                emitter->accumulatedTime = 0;
                emitter->targetAccumulatedTime = emitter->targetAccumulatedTimeRef + RandomBil(entropy) * emitter->targetAccumulatedTimeV;
            }
        } break;
        
        case Emission_Fixed:
        {
            r32 speed = 1.0f;
            if(emitter->modulateWithVelocity)
            {
                speed *= speedNorm;
            }
            
            particleCount = (speed * (emitter->particleCount + RandomBil(entropy) * emitter->particleCountV));
        } break;
    }
    
    switch(emitter->boundType)
    {
        case Bound_None:
        {
            
        } break;
        
        case Bound_Sphere:
        {
            if(emitter->outline)
            {
                atP = baseP + Normalize(RandomBilV3(entropy)) * emitter->radious * effect->boundsScale;
            }
            else
            {
                atP = baseP + Normalize(RandomBilV3(entropy)) * RandomUni(entropy) * emitter->radious * effect->boundsScale;
            }
        } break;
        
        case Bound_Rect:
        {
            if(emitter->outline)
            {
                r32 x = (RandomChoice(entropy, 2) == 0) ? 1.0f : -1.0f;
                r32 y = (RandomChoice(entropy, 2) == 0) ? 1.0f : -1.0f;
                r32 z = (RandomChoice(entropy, 2) == 0) ? 1.0f : -1.0f;
                atP = baseP + Hadamart(V3(x, y, z), emitter->rectDim * effect->boundsScale);
            }
            else
            {
                atP = baseP + Hadamart(RandomBilV3(entropy), RandomUni(entropy) * emitter->rectDim * effect->boundsScale);
            }
        } break;
    }
    
    
    particleCount += effect->spawnParticlesLeftOff;
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
        
        Vec3 dP;
        if(emitter->followOrientation < 0)
        {
            dP = Lerp(-orientation, emitter->followOrientation, emitter->dP);
        }
        else
        {
            dP = Lerp(emitter->dP, emitter->followOrientation, orientation);
        }
        
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
        result = effect->P + V3(0, 0, 1);
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
inline void UpdateAndRenderParticle4x(GameModeWorld* worldMode, ParticleEffectInstance* effect,ParticleUpdater* updater, u32 phaseIndex, r32 normPhaseTime, Particle_4x* A, RenderGroup* group, V3_4x frameDisplacement, r32 dt)
{
    V3_4x finalParticleP = {};
    V3_4x ddP = ToV3_4x(updater->finalAcceleration);
    
    A->P += frameDisplacement;
    A->P += ((0.5f * Square(dt) * ddP) + (dt * A->dP));
    A->dP += dt * ddP;
    finalParticleP = A->P;
    
    r32 speedNorm = GetSpeedNorm(effect);
    if(updater->updateColorOverLifetime)
    {
        r32 colorDT = dt;
        if(updater->modulateColorWithVelocity)
        {
            colorDT *= speedNorm;
        }
        __m128 dt4x = MMSetExpr(colorDT);
        V4_4x ddC = ToV4_4x(updater->ddC);
        A->C += ((0.5f * Square(colorDT) * ddC) + (colorDT * A->dC));
        A->dC += colorDT * ddC;
        A->C = Clamp01(A->C);
    }
    else
    {
        Vec4 color = Lerp(updater->colorIdle, speedNorm, updater->colorFullSpeed);
        
        A->C.r = MMSetExpr(color.r);
        A->C.g = MMSetExpr(color.g);
        A->C.b = MMSetExpr(color.b);
        A->C.a = MMSetExpr(color.a);
    }
    
    if(updater->updateScaleOverTime)
    {
        r32 scaleDT = dt;
        if(updater->modulateScaleWithVelocity)
        {
            scaleDT *= speedNorm;
        }
        __m128 dt4x = MMSetExpr(scaleDT);
        __m128 dScaleX = MMSetExpr(updater->dScaleX);
        __m128 dScaleY = MMSetExpr(updater->dScaleY);
        
        A->scaleX4x += dt4x * dScaleX;
        A->scaleY4x += dt4x * dScaleY;
    }
    else
    {
        r32 scaleX = Lerp(updater->scaleXIdle, speedNorm, updater->scaleXFullspeed);
        r32 scaleY = Lerp(updater->scaleYIdle, speedNorm, updater->scaleYFullspeed);
        
        A->scaleX4x = MMSetExpr(scaleX);
        A->scaleY4x = MMSetExpr(scaleY);
    }
    
    if(updater->updateAngleOvertime)
    {
        r32 angleDT = dt;
        if(updater->modulateAngleWithVelocity)
        {
            angleDT *= speedNorm;
        }
        __m128 dt4x = MMSetExpr(angleDT);
        __m128 dAngle = MMSetExpr(updater->dAngle);
        A->angle4x += dt4x * dAngle;
    }
    else
    {
        r32 angle = Lerp(updater->angleIdle, speedNorm, updater->angleFullspeed);
        A->angle4x = MMSetExpr(angle); 
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
        Vec4 windInfluences = effect->windInfluences;
        u8 windFrequency = effect->windFrequency;
        u8 seed = 0;
        Vec4 dissolvePercentages = V4(0, 0, 0, 0);
        r32 alphaThreesold = 0;
        b32 flat = false;
        Vec2 invUV = updater->textureInvUV;
        
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
            u32 C = RGBAPack8x4(color * 255.0f);
            
            r32 angle = M(A->angle4x, subIndex);
            r32 scaleX = M(A->scaleX4x, subIndex);
            r32 scaleY = M(A->scaleY4x, subIndex);
            
            r32 angleRad = DegToRad(angle);
            Vec3 XAxis = V3(Cos(angleRad), Sin(angleRad), 0.0f);
            Vec3 YAxis  = V3(Perp(XAxis.xy), 0.0f);
            
            Vec3 lateral = 0.5f * scaleX * (XAxis.x * magicLateralVector + XAxis.y * magicUpVector);
            Vec3 up = 0.5f * scaleY * (YAxis.x * magicLateralVector + YAxis.y * magicUpVector);
            
            PushMagicQuad(group, P, flat, lateral, up, invUV, C, *updater->texture, lights, 0, 0, 1, windInfluences, windFrequency, dissolvePercentages, alphaThreesold, seed);
        }
    }
}

internal void SetEffectParameters(ParticleEffectInstance* effect, Vec3 P, Vec3 speed, r32 scale)
{
    effect->P = P;
    effect->speed = speed;
    effect->boundsScale = scale;
}

internal void UpdateAndRenderEffect(GameModeWorld* worldMode, ParticleCache* cache, ParticleEffectInstance* effect, r32 dt, Vec3 frameDisplacementInit, RenderGroup* group)
{
    for(u32 phaseIndex = 0; phaseIndex < effect->phaseCount; ++phaseIndex)
    {
        ParticlePhase* phase = effect->phases + phaseIndex;
        ParticleUpdater* updater = &phase->updater;
        u32 subtype = GetAssetSubtype(group->assets, AssetType_Image, updater->asset.subtypeHash);
        
        GameProperties properties = {};
        for(u32 propertyIndex = 0; propertyIndex < updater->propertyCount; ++propertyIndex)
        {
            AddGamePropertyRaw(&properties, updater->properties[propertyIndex]);
        }
        
        updater->bitmapID = QueryBitmaps(group->assets, subtype, 0, &properties);
        updater->finalAcceleration = updater->ddP;
        if(effect->influencedByWind)
        {
            updater->finalAcceleration += worldMode->windStrength * worldMode->windDirection;
        }
        
        if(IsValid(updater->bitmapID))
        {
            updater->texture = 0;
            Bitmap* bitmap = GetBitmap(group->assets, updater->bitmapID).bitmap;
            if(bitmap)
            {
                LockAssetForCurrentFrame(group->assets, updater->bitmapID);
                updater->texture = &bitmap->textureHandle;
                updater->textureInvUV = GetInvUV(bitmap->width, bitmap->height);
            }
            else
            {
                LoadBitmap(group->assets, updater->bitmapID);
            }
        }
    }
    
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
            if(effect->subEffect)
            {
                for(u32 pIndex = 0; pIndex < 4; ++pIndex)
                {
                    Vec3 P = 
                    { 
                        M(particle->P.x, pIndex),
                        M(particle->P.y, pIndex),
                        M(particle->P.z, pIndex)
                    };
                    
                    Vec3 speed = 
                    { 
                        M(particle->dP.x, pIndex),
                        M(particle->dP.y, pIndex),
                        M(particle->dP.z, pIndex)
                    };
                    
                    SetEffectParameters(effect->subEffect, P, speed, 1.0f);
                    SpawnParticles(cache, effect->subEffect, 0);
                }
            }
            
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
                UpdateAndRenderParticle4x(worldMode, effect, updater, phaseIndex,normPhaseTime, particle, group, frameDisplacement, dt);
            }
            particlePtr = &particle->next;
        }
    }
}


inline void FreeParticleEffect(ParticleEffectInstance* effect)
{
    Assert(effect->active);
    effect->active = false;
    
    if(effect->subEffect)
    {
        effect->subEffect->active = false;
    }
}

inline ParticleEffectInstance* GetNewParticleEffect(ParticleCache* cache, ParticleEffect* definition)
{
    ParticleEffectInstance* result;
    FREELIST_ALLOC(result, cache->firstFreeEffect, PushStruct(cache->pool, ParticleEffectInstance));
    
    result->active = true;
    result->subEffect = 0;
    result->spawnParticlesLeftOff = 0;
    
    result->emitter = definition->emitter;
    
    result->emitter.emissionType = SafeTruncateToU16(ConvertEnumerator(ParticleEmissionType, definition->emitter.emission));
    result->emitter.boundType = SafeTruncateToU16(ConvertEnumerator(ParticleBoundType, definition->emitter.bound));
    
    
    result->P = {};
    result->speed = {};
    result->boundsScale = 0;
    result->maxSpeedMagnitudo = definition->maxSpeedMagnitudo;
    
    
    result->influencedByWind = definition->influencedByWind;
    result->windInfluences = definition->windInfluences;
    result->windFrequency = (u8) definition->windFrequency;
    
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
    
    if(definition->subPhaseCount > 0)
    {
        ParticleEffect subDefinition = {};
        subDefinition.maxSpeedMagnitudo = definition->subEffectSpeedMagnitudo;
        subDefinition.emitter = definition->subEmitter;
        subDefinition.phaseCount = definition->subPhaseCount;
        subDefinition.phases = definition->subPhases;
        
        result->subEffect = GetNewParticleEffect(cache, &subDefinition);
    }
    
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
    particleCache->entropy = Seed(1234);
    particleCache->deltaParticleP = {};
    particleCache->pool = pool;
}
