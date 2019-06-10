#if 0
internal void SpawnAshFromSourceToDest(ParticleCache* cache, Vec3 PStart, Vec3 PDest, Vec4 ashColor, u32 particleCount4x, r32 dim, r32 timeToArriveAtDest)
{
    ParticleSystem* system = &cache->ashSineSystem;
    RandomSequence* entropy = &cache->particleEntropy;
    
    r32 radiants = PI32;
    r32 timeToMakeRadiants = timeToArriveAtDest;
    
    Vec3 deltaP = PDest - PStart;
    deltaP = (1.0f / radiants) * deltaP;
    
    V3_4x PStart4x = ToV3_4x(PStart - cache->deltaParticleP);
    V3_4x unitDP = ToV3_4x(deltaP);
    
    Vec2 horizontalPlane = PDest.xy - PStart.xy;
    r32 zRotation = RadiantsBetweenVectors(V2(1, 0), horizontalPlane);
    
    Vec2 verticalPlane = V2(PDest.x, PDest.z) - V2(PStart.x, PStart.z);
    r32 yRotation = RadiantsBetweenVectors(V2(1, 0), verticalPlane);
    
    
    m4x4 matrix = ZRotation(zRotation) * YRotation(yRotation);
    
    Vec3 UpVector = GetColumn(matrix, 2);
    UpVector.z = Abs(UpVector.z);
    
    V3_4x UpVector4x = ToV3_4x(UpVector);
    
    for(u32 newParticle = 0; newParticle < particleCount4x; newParticle++)
    {
        u32 i = system->nextParticle4++;
        
        if( system->nextParticle4 >= MAX_PARTICLE_COUNT_4 )
        {
            system->nextParticle4 = 0;
        }
        
        Particle_4x *A = system->particles + i;
        
        
        __m128 Xs = _mm_set_ps(PStart.x + RandomBil(entropy) * 0.1f,
                               PStart.x + RandomBil(entropy) * 0.1f,
                               PStart.x + RandomBil(entropy) * 0.1f,
                               PStart.x + RandomBil(entropy) * 0.1f);
        
        __m128 Ys = _mm_set_ps(PStart.y + RandomBil(entropy) * 0.1f,
                               PStart.y + RandomBil(entropy) * 0.1f,
                               PStart.y + RandomBil(entropy) * 0.1f,
                               PStart.y + RandomBil(entropy) * 0.1f);
        
        __m128 Zs = _mm_set_ps(PStart.z + RandomBil(entropy) * 0.1f,
                               PStart.z + RandomBil(entropy) * 0.1f,
                               PStart.z + RandomBil(entropy) * 0.1f,
                               PStart.z + RandomBil(entropy) * 0.1f);
        
        A->startP.x = Xs;
        A->startP.y = Ys;
        A->startP.z = Zs;
        
        
        A->unitDP.x = unitDP.x;
        A->unitDP.y = unitDP.y;
        A->unitDP.z = unitDP.z;
        
        A->UpVector = UpVector4x;
        
        r32 radiantsPerSec = radiants / timeToMakeRadiants;
        
        
        A->startC.r = MMSetExpr(Clamp01(ashColor.r - RandomUni(entropy) * 0.05f));
        A->startC.g = MMSetExpr(Clamp01(ashColor.g - RandomUni(entropy) * 0.05f));
        A->startC.b = MMSetExpr(ashColor.b);
        A->startC.a = MMSetExpr(1.0f);
        
        A->lerp4x = MMSetExpr(0);
        A->lerpVel4x = _mm_set_ps(radiantsPerSec, radiantsPerSec, radiantsPerSec, radiantsPerSec);
        A->lerpColorAlpha = MMSetExpr(0);
        
        
        r32 startingAngle = DegToRad(45.0f);
        A->angle4x = _mm_set_ps(DegToRad(startingAngle + RandomBil(entropy) * 90.0f), DegToRad(startingAngle + RandomBil(entropy) * 90.0f), DegToRad(startingAngle + RandomBil(entropy) * 90.0f), DegToRad(startingAngle + RandomBil(entropy) * 90.0f));
        A->height4x = MMSetExpr(dim);
    }
}
#endif

#define MMSetRandomize(seq, value, magnitude) _mm_set_ps(value + RandomBil(seq) * magnitude, value + RandomBil(seq) * magnitude, value + RandomBil(seq) * magnitude, value + RandomBil(seq) * magnitude);


#if 0
struct ParticleEffectData
{
	Vec3 sourceP;
	Vec3 sourceVel;
	Vec3 sourceAcc;
	Rect3 sourceBounds3D;
	Rect2 sourceBounds2D;
    
	Vec3 targetP;
	Vec3 targetVel;
	Vec3 targetAcc;
	Rect3 targetBounds3D;
	Rect2 targetBounds2D;
};

inline void FillParticleEffectData(ParticleEffect* effect, Vec3 sourceP, Vec3 sourceVel, Vec3 sourceAcc, Rect3 sourceBounds, Vec3 targetP, Vec3 targetVel, Vec3 targetAcc, Rect3 targetBounds)
{
	effect->data.sourceP = sourceP;
	effect->data.sourceVel = sourceVel;
	effect->data.sourceAcc = sourceAcc;
	effect->data.sourceBounds = sourceBounds;
    
	effect->data.sourceP = sourceP;
	effect->data.sourceVel = sourceVel;
	effect->data.sourceAcc = sourceAcc;
	effect->data.sourceBounds = sourceBounds;
}
#endif

internal void SpawnParticles(ParticleCache* cache, ParticleEffect* effect, Vec3 atPInit, u32 particle4xCount)
{
    effect->particle4xCount += particle4xCount;
    RandomSequence* entropy = &cache->particleEntropy;
    Vec3 atP = atPInit - cache->deltaParticleP;
    
    ParticleEmitter* emitter = &effect->definition->emitter;
    for( u32 newParticle = 0; newParticle < particle4xCount; newParticle++ )
    {
        Particle_4x *A;
        FREELIST_ALLOC(A, cache->firstFreeParticle4x, PushStruct(&cache->pool, Particle_4x, AlignNoClear(128)));
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
                A->height4x = MMSetRandomize(entropy, emitter->height, emitter->heightV);
                
                FREELIST_INSERT(A, effect->firstParticle);
            } break;
            InvalidDefaultCase;
        }
    }
}


inline Lights GetLights(GameModeWorld* worldMode, Vec3 P);
inline void UpdateAndRenderParticle4x(GameModeWorld* worldMode, ParticlePhase* phase, ParticlePhase* followingPhase, r32 normPhaseTime, Particle_4x* A, RenderGroup* group, V3_4x frameDisplacement, r32 dt, b32 sine)
{
    ParticleUpdater* updater = &phase->updater;
    
    __m128 dt4x = MMSetExpr(dt);
    switch(updater->type)
    {
        case ParticleUpdater_Sine:
        {
            A->P += frameDisplacement;
            A->lerp4x = _mm_add_ps(A->lerp4x, _mm_mul_ps(dt4x, updater->lerpVel4x));
            A->C.a = _mm_add_ps(A->C.a, _mm_mul_ps(dt4x, updater->lerpAlpha4x));
            
            __m128 sin4x = _mm_set_ps(Sin(M(A->lerp4x, 0)), 
                                      Sin(M(A->lerp4x, 1)), 
                                      Sin(M(A->lerp4x, 2)), 
                                      Sin(M(A->lerp4x, 3)));
            
            A->P = A->lerp4x * updater->unitDP + sin4x * updater->UpVector;
        } break;
        
        case ParticleUpdater_Standard:
        {
            A->P += frameDisplacement;
            A->P += ((0.5f * Square(dt) * updater->ddP) + (dt * A->dP));
            A->dP += dt * updater->ddP;
            
            A->C += ((0.5f * Square(dt) * updater->ddC) + (dt * A->dC));
            A->dC += dt * updater->ddC;
            A->C = Clamp01(A->C);
            
            A->height4x += dt4x * updater->dHeight;
            A->angle4x += dt4x * updater->dAngle;
#if 0
            if( particle->P.z < 0 )
            {
                particle->P.z = 0;
            }
#endif
        } break;
        
        InvalidDefaultCase;
    }
    
    
    
    
    
    
    Vec3 lightP = 
    { 
        M(A->P.x, 0),
        M(A->P.y, 0),
        M(A->P.z, 0)
    };
    Lights lights = GetLights(worldMode, lightP);
    for(u32 subIndex = 0; subIndex < 4; ++subIndex)
    {
        Vec3 P = 
        { 
            M(A->P.x, subIndex),
            M(A->P.y, subIndex),
            M(A->P.z, subIndex)
        };
        
        Vec4 color = 
        {
            M(A->C.r, subIndex),
            M(A->C.g, subIndex),
            M(A->C.b, subIndex),
            M(A->C.a, subIndex)
        };
        
        r32 angle = M(A->angle4x, subIndex);
        r32 height = M(A->height4x, subIndex);
        
        
        
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
                PushBitmap(group, transform, followingUpdater->bitmapID, P, height, V2(1.0f, 1.0f),  newColor, lights);
            }
            PushBitmap(group, transform, updater->bitmapID, P, height, V2(1.0f, 1.0f),  color, lights);
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
                UpdateAndRenderParticle4x(worldMode, phase, followingPhase, normPhaseTime, particle, group, frameDisplacement, dt, false);
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