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


internal void SpawnParticles(ParticleCache* cache, ParticleEffect* effect, Vec3 atPInit, u32 particle4xCount)
{
    effect->particle4xCount += particle4xCount;
    RandomSequence* entropy = &cache->particleEntropy;
    Vec3 atP = atPInit - cache->deltaParticleP;
    
    ParticleEmitter* emitter = &effect->emitter;
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
inline void UpdateAndRenderParticle4x(GameModeWorld* worldMode, ParticleUpdater* updater, Particle_4x* A, RenderGroup* group, V3_4x frameDisplacement, r32 dt, b32 sine)
{
    switch(updater->type)
    {
        case ParticleUpdater_Sine:
        {
            __m128 dt4x = MMSetExpr(dt);
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
#if 0
            if( particle->P.z < 0 )
            {
                particle->P.z = 0;
            }
            
            if(color.a > 0.9f)
            {
                color.a = 0.9f * Clamp01MapToRange( 1.0f, color.a, 0.9f );
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
        
        
        
#if 0        
        if(system->bitmapID.value)
        {
            ObjectTransform transform = UprightTransform();
            transform.angle = angle;
            PushBitmap(renderGroup, transform, system->bitmapID, P, height, V2(1.0f, 1.0f),  color, lights);
        }
        else
#endif
        {
            u32 C = StoreColor(color);
            
            r32 cos = Cos(angle);
            r32 sin = Sin(angle);
            Vec3 XAxisHalf = height * 0.5f * (cos * group->gameCamera.X + sin * group->gameCamera.Y);
            Vec3 YAxisHalf = height * 0.5f * (-sin * group->gameCamera.X + cos * group->gameCamera.Y);
            
            Vec4 P0 = V4(P - XAxisHalf - YAxisHalf, 0);
            Vec4 P1 = V4(P + XAxisHalf - YAxisHalf, 0);
            Vec4 P2 = V4(P + XAxisHalf + YAxisHalf, 0);
            Vec4 P3 = V4(P - XAxisHalf + YAxisHalf, 0);
            
            Vec2 UV = {};
            ReservedVertexes vertexes = ReserveQuads(group, 1);
            PushQuad(group, group->whiteTexture, lights, &vertexes,
                     P0, UV, C,
                     P1, UV, C,
                     P2, UV, C,
                     P3, UV, C, 0);
        }
    }
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
            ParticleUpdater* updater = &effect->updater;
            UpdateAndRenderParticle4x(worldMode, updater, particle, group, frameDisplacement, dt, false);
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

inline void InitEmitter(ParticleEmitter* emitter)
{
    emitter->type = ParticleEmitter_Standard;
    
    
    emitter->lifeTime = 2.0f;
    emitter->lifeTimeV = 0.0f;
    
    emitter->startPV = V3(0.1f, 0.1f, 0.1f);
    
    emitter->dP = V3(0, 0, 1);
    emitter->dPV = V3(0.1f, 0.1f, 0.1f);
    
    emitter->C = V4(1.0f, 0.6f, 0.0f, 1.0f);
    emitter->CV = V4(0.1f, 0.1f, 0.0f, 0.0f);
    
    emitter->dC = emitter->C * (1.0f / emitter->lifeTime);
    emitter->dCV = V4(0, 0, 0, 0);
    
    emitter->angle = 45.0f;
    emitter->angleV = 90.0f;
    
    emitter->height = 0.07f;
    emitter->heightV = 0.01f;
}

inline void InitUpdater(ParticleUpdater* updater)
{
    updater->type = ParticleUpdater_Standard;
    
    updater->unitDP = {};
    updater->ddP = {};
    updater->UpVector = {};
    updater->ddC = {};
    updater->lerpVel4x = {};
    updater->lerpAlpha4x = {};
}

inline ParticleEffect* GetNewParticleEffect(ParticleCache* cache)
{
    ParticleEffect* result;
    FREELIST_ALLOC(result, cache->firstFreeEffect, PushStruct(&cache->pool, ParticleEffect));
    
    result->active = true;
    
    InitEmitter(&result->emitter);
    InitUpdater(&result->updater);
    
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