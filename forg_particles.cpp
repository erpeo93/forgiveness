#define MMSetRandomize(seq, value, magnitude) _mm_set_ps(value + RandomBil(seq) * magnitude, value + RandomBil(seq) * magnitude, value + RandomBil(seq) * magnitude, value + RandomBil(seq) * magnitude);

inline void FillParticleEffectData(ParticleEffect* effect, Vec3 P, Vec3 destP)
{
	effect->data.P = P;
    effect->data.destP = destP;
}

internal void SpawnParticles(ParticleCache* cache, ParticleEffect* effect, r32 dt)
{
    RandomSequence* entropy = &cache->particleEntropy;
    Vec3 atP = effect->data.P - cache->deltaParticleP;
    
    ParticleEmitter* emitter = &effect->definition->emitter;
    
    
    r32 particleCount = emitter->particlesPerSec * dt + effect->spawnParticlesLeftOff;
    u32 particle4xCount = (u32) (particleCount / 4.0f);
    effect->spawnParticlesLeftOff = particleCount - (r32) (particle4xCount * 4);
    effect->particle4xCount += particle4xCount;
    
    for( u32 newParticle = 0; newParticle < particle4xCount; newParticle++ )
    {
        Particle_4x *A;
        FREELIST_ALLOC(A, cache->firstFreeParticle4x, PushStruct(&cache->pool, Particle_4x, AlignNoClear(128)));
        FREELIST_INSERT(A, effect->firstParticle);
        switch(emitter->type)
        {
            case ParticleEmitter_Standard:
            {
                r32 ttl = emitter->lifeTime + RandomBil(entropy) * emitter->lifeTimeV;
                A->ttl4x = MMSetExpr(ttl);
                
                A->P.x = MMSetRandomize(entropy, atP.x, emitter->startPV.x);
                A->P.y = MMSetRandomize(entropy, atP.y, emitter->startPV.y);
                A->P.z = MMSetRandomize(entropy, atP.z, emitter->startPV.z);
                
                A->dP.x = MMSetRandomize(entropy, emitter->dP.x, emitter->dPV.x);
                A->dP.y = MMSetRandomize(entropy, emitter->dP.y, emitter->dPV.y);
                A->dP.z = MMSetRandomize(entropy, emitter->dP.z, emitter->dPV.z);
                
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
            } break;
            
            InvalidDefaultCase;
        }
    }
}


inline Lights GetLights(GameModeWorld* worldMode, Vec3 P);
inline void UpdateAndRenderParticle4x(GameModeWorld* worldMode, ParticleEffectData* data, ParticlePhase* phase, ParticlePhase* followingPhase, r32 normPhaseTime, Particle_4x* A, RenderGroup* group, V3_4x frameDisplacement, r32 dt, b32 sine)
{
    ParticleUpdater* updater = &phase->updater;
    
    __m128 dt4x = MMSetExpr(dt);
    
    V3_4x finalParticleP = {};
    
    switch(updater->type)
    {
        case ParticleUpdater_Sine:
        {
            Vec3 deltaP = data->destP - data->P;
            V3_4x dP = ToV3_4x(deltaP);
            
            Vec2 horizontalPlane = deltaP.xy;
            r32 zRotation = RadiantsBetweenVectors(V2(1, 0), horizontalPlane);
            Vec2 verticalPlane = V2(deltaP.x, deltaP.z);
            r32 yRotation = RadiantsBetweenVectors(V2(1, 0), verticalPlane);
            m4x4 matrix = ZRotation(zRotation) * YRotation(yRotation);
            Vec3 upVector = GetColumn(matrix, 2);
            upVector.z = Abs(upVector.z);
            V3_4x upVector4x = ToV3_4x(upVector);
            
            r32 normPhase = Clamp01MapToRange(phase->ttlMax, M(A->ttl4x, 0), phase->ttlMin);
            r32 normRadiants = normPhase * M(updater->totalRadiants, 0);
            r32 radiantsSine = Sin(normRadiants);
            
            finalParticleP = A->P + normPhase * dP + radiantsSine * upVector4x;
            
            
            A->C += ((0.5f * Square(dt) * updater->ddC) + (dt * A->dC));
            A->dC += dt * updater->ddC;
            A->C = Clamp01(A->C);
            
            A->scaleX4x += dt4x * updater->dScaleX;
            A->scaleY4x += dt4x * updater->dScaleY;
            A->angle4x += dt4x * updater->dAngle;
        } break;
        
        case ParticleUpdater_Standard:
        {
            A->P += frameDisplacement;
            A->P += ((0.5f * Square(dt) * updater->ddP) + (dt * A->dP));
            A->dP += dt * updater->ddP;
            finalParticleP = A->P;
            
            A->C += ((0.5f * Square(dt) * updater->ddC) + (dt * A->dC));
            A->dC += dt * updater->ddC;
            A->C = Clamp01(A->C);
            
            A->scaleX4x += dt4x * updater->dScaleX;
            A->scaleY4x += dt4x * updater->dScaleY;
            A->angle4x += dt4x * updater->dAngle;
        } break;
        
        InvalidDefaultCase;
    }
    
    
    
    
    
    
    Vec3 lightP = 
    { 
        M(finalParticleP.x, 0),
        M(finalParticleP.y, 0),
        M(finalParticleP.z, 0)
    };
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
        
        
        
        if(updater->bitmapID.value)
        {
            ObjectTransform transform = UprightTransform();
            transform.angle = angle;
            
            if(updater->startDrawingFollowingBitmapAt && followingPhase)
            {
                r32 normLife = Clamp01MapToRange(updater->startDrawingFollowingBitmapAt, normPhaseTime, 1.0f);
                r32 newAlpha = Lerp(0, normLife, color.a);
                Vec4 newColor = color;
                newColor.a = newAlpha;
                
                ParticleUpdater* followingUpdater = &followingPhase->updater;
                PushBitmap(group, transform, followingUpdater->bitmapID, P, 0, V2(scaleX, scaleY),  newColor, lights);
            }
            PushBitmap(group, transform, updater->bitmapID, P, 9, V2(scaleX, scaleY),  color, lights);
        }
    }
}

inline ParticlePhase* GetPhase(ParticleEffect* effect, r32 ttl)
{
    ParticlePhase* result = 0;
    
    for(u32 phaseIndex = 0; phaseIndex < effect->definition->phaseCount; ++phaseIndex)
    {
        ParticlePhase* phase = effect->definition->phases + phaseIndex;
        if(phase->ttlMax >= ttl && phase->ttlMin <= ttl)
        {
            result = phase;
            break;
        }
    }
    return result;
}

inline ParticlePhase* GetFollowingPhase(ParticleEffect* effect, ParticlePhase* phaseIn)
{
    ParticlePhase* result = 0;
    
    r32 currentNearest = R32_MAX;
    for(u32 phaseIndex = 0; phaseIndex < effect->definition->phaseCount; ++phaseIndex)
    {
        ParticlePhase* phase = effect->definition->phases + phaseIndex;
        
        r32 delta = Abs(phase->ttlMax - phaseIn->ttlMin);
        if(phase != phaseIn && delta < currentNearest)
        {
            currentNearest = delta;
            result = phase;
        }
    }
    return result;
}


internal void UpdateAndRenderEffect(GameModeWorld* worldMode, ParticleCache* cache, ParticleEffect* effect, r32 dt, Vec3 frameDisplacementInit, RenderGroup* group)
{
    SpawnParticles(cache, effect, dt);
    
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
            ParticlePhase* phase = GetPhase(effect, ttl);
            if(phase)
            {
                ParticlePhase* followingPhase = GetFollowingPhase(effect, phase);
                r32 normPhaseTime = 1.0f - Clamp01MapToRange(phase->ttlMin, ttl, phase->ttlMax);
                UpdateAndRenderParticle4x(worldMode, &effect->data, phase, followingPhase, normPhaseTime, particle, group, frameDisplacement, dt, false);
            }
            particlePtr = &particle->next;
        }
    }
}

inline void FreeParticleEffect(ParticleEffect* effect)
{
    Assert(effect->particle4xCount == 0);
    Assert(!effect->firstParticle);
    Assert(effect->active);
    effect->active = false;
}


inline ParticleEffect* GetNewParticleEffect(ParticleCache* cache, ParticleEffectDefinition* definition)
{
    ParticleEffect* result;
    FREELIST_ALLOC(result, cache->firstFreeEffect, PushStruct(&cache->pool, ParticleEffect));
    
    result->active = true;
    result->data = {};
    result->spawnParticlesLeftOff = 0;
    result->definition = definition;
    Assert(result->particle4xCount == 0);
    Assert(!result->firstParticle);
    
    FREELIST_INSERT(result, cache->firstActiveEffect);
    
    return result;
}

internal void InitParticleCache( ParticleCache* particleCache, Assets* assets )
{
    particleCache->particleEntropy = Seed(1234);
    particleCache->deltaParticleP = {};
}

internal void UpdateAndRenderParticleEffects(GameModeWorld* worldMode, ParticleCache* particleCache, r32 dt, RenderGroup* group)
{
    Vec3 frameDisplacement = particleCache->deltaParticleP;
    for(ParticleEffect** effectPtr = &particleCache->firstActiveEffect; *effectPtr;)
    {
        ParticleEffect* effect = *effectPtr;
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