#if 0

struct Vorton
{
    ???
};

internal void CalculateVelocityGrid()
{
    for( everyVorton )
    {
        for( everyOtherVorton )
        {
            ... do stuff
                gridcell->velocity += ??;
        }
    }
}

internal void Advect()
{
    for( everyVorton )
    {
        vorton->position += Interpolate( velocityGrid, vorton->position );
    }
    
    for( everyParticle )
    {
        particle->position += Interpolate( velocityGrid, particle->position );
    }
}

#endif

#define MMSetExpr(expr) _mm_set_ps(expr, expr, expr, expr)


internal void SpawnWaterRipples(ParticleCache* cache, Vec3 atPInit, Vec3 dP, r32 lifeTime)
{
    ParticleSystem* system = &cache->waterRippleSystem;
    RandomSequence* entropy = &cache->particleEntropy;
    
    lifeTime *= RandomRangeFloat(entropy, 0.7f, 1.3f);
    
    atPInit.x +=  RandomBil(entropy) * 0.5f;
    atPInit.y +=  RandomBil(entropy) * 0.5f;
    atPInit.z +=  RandomBil(entropy) * 0.5f;
    
    V3_4x atP = ToV3_4x( atPInit - cache->deltaParticleP );
    for( u32 newParticle = 0; newParticle < 1; newParticle++ )
    {
        u32 i = system->nextParticle4++;
        
        if( system->nextParticle4 >= MAX_PARTICLE_COUNT_4 )
        {
            system->nextParticle4 = 0;
        }
        
        Particle_4x *A = system->particles + i;
        
        A->P.x = atP.x;
        A->P.y = atP.y;
        A->P.z = atP.z;
        
        A->dP.x = _mm_set_ps( dP.x + RandomBil(entropy) * 0.0f, dP.x + RandomBil(entropy) * 0.0f, dP.x + RandomBil(entropy) * 0.0f, dP.x + RandomBil(entropy) * 0.0f);
        A->dP.y = _mm_set_ps( dP.y  + RandomBil(entropy) * 0.0f, dP.x + RandomBil(entropy) * 0.0f, dP.x + RandomBil(entropy) * 0.0f, dP.x + RandomBil(entropy) * 0.00f);
        A->dP.z = _mm_set_ps( dP.z  + RandomBil(entropy) * 0.0f, dP.z  + RandomBil(entropy) * 0.0f, dP.z  + RandomBil(entropy) * 0.0f, dP.z  + RandomBil(entropy) * 0.0f);
        
        A->ddP.x = MMSetExpr( 0.0f );
        A->ddP.y = MMSetExpr( 0.0f );
        A->ddP.z = MMSetExpr( 0.0f );
        
        A->C.r = MMSetExpr( 0.9f - RandomUni(entropy) * 0.05f);
        A->C.g = MMSetExpr( 1.0f - RandomUni(entropy) * 0.05f);
        A->C.b = MMSetExpr( 0.9f - RandomUni(entropy) * 0.05f);
        A->C.a = MMSetExpr( 1.0f );
        
        A->dC.r = MMSetExpr( -1.0f / lifeTime );
        A->dC.g = MMSetExpr( -1.0f / lifeTime );
        A->dC.g = MMSetExpr( -1.0f / lifeTime );
        A->dC.a = MMSetExpr( -1.0f / lifeTime );
        
        r32 startingAngle = DegToRad(0.0f);
        A->angle4x = _mm_set_ps(DegToRad(startingAngle + RandomBil(entropy) * 0.0f), DegToRad(startingAngle + RandomBil(entropy) * 0.0f), DegToRad(startingAngle + RandomBil(entropy) * 0.0f), DegToRad(startingAngle + RandomBil(entropy) * 0.0f));
        A->height4x = MMSetExpr(0.25f + (0.13f * RandomUni(entropy)));
    }
}




internal void SpawnAsh(ParticleCache* cache, Vec3 atPInit, Vec3 dP, r32 lifeTime, Vec4 ashColor, u32 particleCount4x, r32 ashParticleViewPercentage, r32 dim)
{
    ParticleSystem* system = &cache->ashSystem;
    RandomSequence* entropy = &cache->particleEntropy;
    
    lifeTime *= RandomRangeFloat(entropy, 0.7f, 1.3f);
    
    atPInit.x +=  RandomBil(entropy) * 0.1f;
    atPInit.y +=  RandomBil(entropy) * 0.1f;
    atPInit.z +=  RandomBil(entropy) * 0.1f;
    
    Vec3 maxDisplacement = V3(0.3f, 0.3f, 0.3f);
    
    V3_4x atP = ToV3_4x(atPInit - cache->deltaParticleP);
    for(u32 newParticle = 0; newParticle < particleCount4x; newParticle++)
    {
        u32 i = system->nextParticle4++;
        
        if( system->nextParticle4 >= MAX_PARTICLE_COUNT_4 )
        {
            system->nextParticle4 = 0;
        }
        
        Particle_4x *A = system->particles + i;
        
        A->P.x = atP.x;
        A->P.y = atP.y;
        A->P.z = atP.z;
        
        A->dP.x = _mm_set_ps( dP.x + RandomBil(entropy) * 0.1f, dP.x + RandomBil(entropy) * 0.1f, dP.x + RandomBil(entropy) * 0.1f, dP.x + RandomBil(entropy) * 0.1f);
        A->dP.y = _mm_set_ps( dP.y  + RandomBil(entropy) * 0.1f, dP.x + RandomBil(entropy) * 0.1f, dP.x + RandomBil(entropy) * 0.1f, dP.x + RandomBil(entropy) * 0.1f);
        A->dP.z = _mm_set_ps( dP.z  + RandomBil(entropy) * 0.4f, dP.z  + RandomBil(entropy) * 0.4f, dP.z  + RandomBil(entropy) * 0.4f, dP.z  + RandomBil(entropy) * 0.4f);
        
        A->ddP.x = MMSetExpr(0.0f);
        A->ddP.y = MMSetExpr(0.0f);
        A->ddP.z = MMSetExpr(0.0f);
        
        
        A->C.r = MMSetExpr(ashColor.r - RandomUni(entropy) * 0.05f);
        A->C.g = MMSetExpr(ashColor.g + RandomBil(entropy) * 0.05f);
        A->C.b = MMSetExpr(ashColor.b);
        
        r32 alpha[4] = {};
        
        for(u32 particleIndex = 0; particleIndex < ArrayCount(alpha); ++particleIndex)
        {
            if(RandomUni(entropy) < ashParticleViewPercentage)
            {
                alpha[particleIndex] = ashColor.a;
            }
        }
        
        A->C.a = _mm_set_ps(alpha[0], alpha[1], alpha[2], alpha[3]);
        
        A->dC.r = MMSetExpr( 0 / lifeTime );
        A->dC.g = MMSetExpr( 0 / lifeTime );
        A->dC.b = MMSetExpr( 0 / lifeTime);
        A->dC.a = MMSetExpr(-1.0f / lifeTime);
        
        r32 startingAngle = DegToRad(45.0f);
        A->angle4x = _mm_set_ps(DegToRad(startingAngle + RandomBil(entropy) * 90.0f), DegToRad(startingAngle + RandomBil(entropy) * 90.0f), DegToRad(startingAngle + RandomBil(entropy) * 90.0f), DegToRad(startingAngle + RandomBil(entropy) * 90.0f));
        A->height4x = MMSetExpr(dim);
    }
}


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

internal void SpawnFire(ParticleCache* cache, Vec3 atPInit, Vec3 dP, r32 lifeTime)
{
    ParticleSystem* system = &cache->fireSystem;
    RandomSequence* entropy = &cache->particleEntropy;
    
    lifeTime *= RandomRangeFloat(entropy, 0.7f, 1.3f);
    
    atPInit.x +=  RandomBil(entropy) * 0.1f;
    atPInit.y +=  RandomBil(entropy) * 0.1f;
    atPInit.z +=  RandomBil(entropy) * 0.1f;
    
    V3_4x atP = ToV3_4x( atPInit - cache->deltaParticleP );
    for( u32 newParticle = 0; newParticle < 1; newParticle++ )
    {
        u32 i = system->nextParticle4++;
        
        if( system->nextParticle4 >= MAX_PARTICLE_COUNT_4 )
        {
            system->nextParticle4 = 0;
        }
        
        Particle_4x *A = system->particles + i;
        
        A->P.x = atP.x;
        A->P.y = atP.y;
        A->P.z = atP.z;
        
        A->dP.x = _mm_set_ps( dP.x + RandomBil(entropy) * 0.1f, dP.x + RandomBil(entropy) * 0.1f, dP.x + RandomBil(entropy) * 0.1f, dP.x + RandomBil(entropy) * 0.1f);
        A->dP.y = _mm_set_ps( dP.y  + RandomBil(entropy) * 0.1f, dP.x + RandomBil(entropy) * 0.1f, dP.x + RandomBil(entropy) * 0.1f, dP.x + RandomBil(entropy) * 0.1f);
        A->dP.z = _mm_set_ps( dP.z  + RandomBil(entropy) * 0.4f, dP.z  + RandomBil(entropy) * 0.4f, dP.z  + RandomBil(entropy) * 0.4f, dP.z  + RandomBil(entropy) * 0.4f);
        
        A->ddP.x = MMSetExpr( 0.0f );
        A->ddP.y = MMSetExpr( 0.0f );
        A->ddP.z = MMSetExpr( 0.0f );
        
        A->C.r = MMSetExpr( 1.0f - RandomUni(entropy) * 0.12f);
        A->C.g = MMSetExpr( 0.6f + RandomBil(entropy) * 0.05f);
        A->C.b = MMSetExpr( 0.0f );
        A->C.a = MMSetExpr( 1.0f );
        
        A->dC.r = MMSetExpr( -1.0f / lifeTime );
        A->dC.g = MMSetExpr( -1.0f / lifeTime );
        A->dC.b = MMSetExpr( 0.0f );
        A->dC.a = MMSetExpr( -1.0f / lifeTime );
        
        r32 startingAngle = DegToRad(45.0f);
        A->angle4x = _mm_set_ps(DegToRad(startingAngle + RandomBil(entropy) * 90.0f), DegToRad(startingAngle + RandomBil(entropy) * 90.0f), DegToRad(startingAngle + RandomBil(entropy) * 90.0f), DegToRad(startingAngle + RandomBil(entropy) * 90.0f));
        A->height4x = MMSetExpr(0.07f);
    }
}

internal void SpawnWater( ParticleCache* cache, Vec3 atPInit, Vec3 dP, r32 lifeTime )
{
    ParticleSystem* system = &cache->waterSystem;
    RandomSequence* entropy = &cache->particleEntropy;
    
    V3_4x atP = ToV3_4x( atPInit - cache->deltaParticleP );
    for( u32 newParticle = 0; newParticle < 1; newParticle++ )
    {
        u32 i = system->nextParticle4++;
        if( system->nextParticle4 >= MAX_PARTICLE_COUNT_4 )
        {
            system->nextParticle4 = 0;
        }
        
        Particle_4x *A = system->particles + i;
        
        A->P.x = atP.x;
        A->P.y = atP.y;
        A->P.z = atP.z;
        
        A->dP.x = MMSetExpr( dP.x );
        A->dP.y = MMSetExpr( dP.y );
        A->dP.z = MMSetExpr( dP.z );
        
        A->ddP.x = MMSetExpr( 0.0f );
        A->ddP.y = MMSetExpr( 0.0f );
        A->ddP.z = MMSetExpr( 0.0f );
        
        A->C.r = MMSetExpr( 0.0f );
        A->C.g = MMSetExpr( 0.0f );
        A->C.b = MMSetExpr( 1.0f );
        A->C.a = MMSetExpr( 1.0f );
        
        A->dC.r = MMSetExpr( 0.0f );
        A->dC.g = MMSetExpr( 0.0f );
        A->dC.b = MMSetExpr( -1.0f / lifeTime );
        A->dC.a = MMSetExpr( -1.0f / lifeTime );
    }
}

internal void SpawnSteam( ParticleCache* cache, Vec3 atPInit, Vec3 dP, r32 lifeTime )
{
    ParticleSystem* system = &cache->steamSystem;
    RandomSequence* entropy = &cache->particleEntropy;
    
    V3_4x atP = ToV3_4x( atPInit - cache->deltaParticleP );
    for( u32 newParticle = 0; newParticle < 1; newParticle++ )
    {
        u32 i = system->nextParticle4++;
        
        if( system->nextParticle4 >= MAX_PARTICLE_COUNT_4 )
        {
            system->nextParticle4 = 0;
        }
        
        Particle_4x *A = system->particles + i;
        
        A->P.x = atP.x;
        A->P.y = atP.y;
        A->P.z = atP.z;
        
        A->dP.x = MMSetExpr( dP.x );
        A->dP.y = MMSetExpr( dP.y );
        A->dP.z = MMSetExpr( dP.z );
        
        A->ddP.x = MMSetExpr( 0.0f );
        A->ddP.y = MMSetExpr( 0.0f );
        A->ddP.z = MMSetExpr( 0.0f );
        
        A->C.r = MMSetExpr( 0.0f );
        A->C.g = MMSetExpr( 1.0f );
        A->C.b = MMSetExpr( 0.0f );
        A->C.a = MMSetExpr( 1.0f );
        
        A->dC.r = MMSetExpr( 0.0f );
        A->dC.g = MMSetExpr( -1.0f / lifeTime );
        A->dC.b = MMSetExpr( 0.0f );
        A->dC.a = MMSetExpr( -1.0f / lifeTime );
    }
}

internal void SpawnFluidParticles(ParticleCache* particleCache, FluidSpawnType type, Vec3 P)
{
    switch(type)
    {
        case FluidSpawn_Fire:
        {
            
#if 0                                
            for( u32 rayIndex = 0; rayIndex < ArrayCount( fluid->rays ); ++rayIndex )
            {
                FluidRay* ray = fluid->rays + rayIndex;
                if( ray->lengthPercentage )
                {
                    r32 speed = 0.5f;
                    Vec3 particleDirection = fluid->direction * fluidVectors[rayIndex];
                    Vec3 velocity = particleDirection * speed;
                    
                    r32 rayMaxLenght = fluid->length * ray->lengthPercentage;
                    r32 lifeTimeFull = GetRayLength( rayMaxLenght, ray ) / Length( velocity );
                    if( lifeTimeFull )
                    {
                        for( u32 fireIndex = 0; fireIndex < ( u32 ) 1; ++fireIndex )
                        {
                            r32 runningLifeTime = 1.0f;
                            for( u32 segmentIndex = 0; segmentIndex < SEGMENT_COUNT; ++segmentIndex )
                            {
                                FluidRaySegment* segment = ray->segments + segmentIndex;
                                runningLifeTime *= segment->intensityPercentage;
                                
                                r32 lifeTime = runningLifeTime * lifeTimeFull;
                                if( lifeTime )
                                {
                                    SpawnFire( particleCache, entity->P + V3( 0, 0, 0 ), velocity, lifeTime );
                                }
                                else
                                {
                                    break;
                                }
                            }
                        }
                    }
                }
            }
#else
            r32 runningLifeTime = 1.0f;
            
            r32 speed = 0.5f;
            Vec3 particleDirection = V3(0, 0, 1);
            Vec3 velocity = particleDirection * speed;
            
            r32 lifeTime = 2.0f;
            SpawnFire( particleCache, P, velocity, lifeTime );
#endif
        } break;
        
        case FluidSpawn_Water:
        {
            
#if 0            
            for( u32 rayIndex = 0; rayIndex < ArrayCount( fluid->rays ); ++rayIndex )
            {
                FluidRay* ray = fluid->rays + rayIndex;
                if( ray->lengthPercentage )
                {
                    r32 speed = 2.0f;
                    Vec3 particleDirection = fluid->direction * fluidVectors[rayIndex];
                    Vec3 velocity = particleDirection * speed;
                    
                    r32 rayMaxLenght = fluid->length * ray->lengthPercentage;
                    r32 lifeTimeFull = GetRayLength( rayMaxLenght, ray ) / Length( velocity );
                    
                    if( lifeTimeFull )
                    {
                        for( u32 waterIndex = 0; waterIndex < ( u32 ) fluid->intensity; ++waterIndex )
                        {
                            r32 runningLifeTime = 1.0f;
                            for( u32 segmentIndex = 0; segmentIndex < SEGMENT_COUNT; ++segmentIndex )
                            {
                                FluidRaySegment* segment = ray->segments + segmentIndex;
                                runningLifeTime *= segment->intensityPercentage;
                                
                                r32 lifeTime = runningLifeTime * lifeTimeFull;
                                if( lifeTime )
                                {
                                    SpawnWater( particleCache, P + V3( 0, 0, 0 ), velocity, lifeTime );
                                }
                                else
                                {
                                    break;
                                }
                            }
                        }
                    }
                }
            }
#endif
            
        } break;
        
        case FluidSpawn_Steam:
        {
            
#if 0            
            for( u32 rayIndex = 0; rayIndex < ArrayCount( fluid->rays ); ++rayIndex )
            {
                FluidRay* ray = fluid->rays + rayIndex;
                if( ray->lengthPercentage )
                {
                    r32 speed = 2.0f;
                    Vec3 particleDirection = fluid->direction * fluidVectors[rayIndex];
                    Vec3 velocity = particleDirection * speed;
                    
                    r32 rayMaxLenght = fluid->length * ray->lengthPercentage;
                    r32 lifeTimeFull = GetRayLength( rayMaxLenght, ray ) / Length( velocity );
                    
                    if( lifeTimeFull )
                    {
                        for( u32 steamIndex = 0; steamIndex < ( u32 ) fluid->intensity; ++steamIndex )
                        {
                            r32 runningLifeTime = 1.0f;
                            for( u32 segmentIndex = 0; segmentIndex < SEGMENT_COUNT; ++segmentIndex )
                            {
                                FluidRaySegment* segment = ray->segments + segmentIndex;
                                runningLifeTime *= segment->intensityPercentage;
                                
                                r32 lifeTime = runningLifeTime * lifeTimeFull;
                                if( lifeTime )
                                {
                                    SpawnSteam( particleCache, P + V3( 0, 0, 0 ), velocity, lifeTime );
                                }
                                else
                                {
                                    break;
                                }
                            }
                        }
                    }
                }
            }
#endif
        } break;
    }
}

inline Vec4 GetLightIndexes(GameModeWorld* worldMode, Vec3 P);
internal void UpdateAndRenderSystem( GameModeWorld* worldMode, ParticleSystem* system, RandomSequence* entropy, r32 dt, Vec3 frameDisplacementInit, RenderGroup* renderGroup )
{
    GameRenderCommands* commands = renderGroup->commands;
    Vec3 N = V3(0, 0, 0);
    Vec2 UV = V2(0, 0);
    
    V3_4x frameDisplacement = ToV3_4x( frameDisplacementInit );
    
    u32 particle4xTriangleCount = 8; // NOTE(Leonardo): two triangles * every particle;
    for( u32 particleIndex = 0; particleIndex < MAX_PARTICLE_COUNT_4; particleIndex++ )
    {
        Particle_4x* A = system->particles + particleIndex;
        //TODO(leonardo): update particle position using velocity calculated via the velocity grid
        A->P += frameDisplacement;
        A->P += ((0.5f * Square(dt) * A->ddP) + (dt * A->dP));
        A->dP += dt * A->ddP;
        A->C += dt * A->dC;
        A->C = Clamp01(A->C);
#if 0
        if( particle->P.z < 0 )
        {
            particle->P.z = 0;
        }
        
        if( color.a > 0.9f )
        {
            color.a = 0.9f * Clamp01MapToRange( 1.0f, color.a, 0.9f );
        }
#endif
        TexturedQuadsCommand* entry = GetCurrentQuads(renderGroup, 4);
        if(entry)
        {
            TexturedVertex* vertBatch = commands->vertexArray + commands->vertexCount;
            commands->vertexCount += (particle4xTriangleCount * 3);
            
            for( u32 subIndex = 0; subIndex < 4; ++subIndex )
            {
                Vec3 P = 
                { 
                    M(A->P.x, subIndex),
                    M(A->P.y, subIndex),
                    M(A->P.z, subIndex)
                };
                Vec4 lightIndexes = GetLightIndexes(worldMode, P);
                
                Vec4 color = 
                {
                    M(A->C.r, subIndex),
                    M(A->C.g, subIndex),
                    M(A->C.b, subIndex),
                    M(A->C.a, subIndex)
                };
                
                r32 angle = M(A->angle4x, subIndex);
                r32 height = M(A->height4x, subIndex);
                
				if(system->bitmapID.value)
				{
					ObjectTransform transform = UprightTransform();
					transform.angle = angle;
					PushBitmap(renderGroup, transform, system->bitmapID, P, height, V2(1.0f, 1.0f),  color, lightIndexes);
				}
				else
				{
					u32 C = StoreColor(color);
                    
					r32 cos = Cos(angle);
					r32 sin = Sin(angle);
					Vec3 XAxisHalf = height * 0.5f * (cos * renderGroup->gameCamera.X + sin * renderGroup->gameCamera.Y);
					Vec3 YAxisHalf = height * 0.5f * (-sin * renderGroup->gameCamera.X + cos * renderGroup->gameCamera.Y);
                    
					Vec4 P0 = V4(P - XAxisHalf - YAxisHalf, 0);
					Vec4 P1 = V4(P + XAxisHalf - YAxisHalf, 0);
					Vec4 P2 = V4(P + XAxisHalf + YAxisHalf, 0);
					Vec4 P3 = V4(P - XAxisHalf + YAxisHalf, 0);
                    
				    PushQuad(renderGroup, renderGroup->whiteTexture, lightIndexes,
                             P0, UV, C,
                             P1, UV, C,
                             P2, UV, C,
                             P3, UV, C, 0);
				}
            }
        }
    }
}

internal void UpdateAndRenderSineSystem(GameModeWorld* worldMode, ParticleSystem* system, RandomSequence* entropy, r32 dt, Vec3 frameDisplacementInit, RenderGroup* renderGroup)
{
    __m128 dt4x = MMSetExpr(dt);
    
    GameRenderCommands* commands = renderGroup->commands;
    Vec3 N = V3(0, 0, 0);
    Vec2 UV = V2(0, 0);
    
    V3_4x frameDisplacement = ToV3_4x(frameDisplacementInit);
    
    u32 particle4xTriangleCount = 8; // NOTE(Leonardo): two triangles * every particle;
    for( u32 particleIndex = 0; particleIndex < MAX_PARTICLE_COUNT_4; particleIndex++ )
    {
        Particle_4x* A = system->particles + particleIndex;
        
        A->startP += frameDisplacement;
        A->lerp4x = _mm_add_ps(A->lerp4x, _mm_mul_ps(dt4x, A->lerpVel4x));
        A->lerpColorAlpha = _mm_add_ps(A->lerpColorAlpha, _mm_mul_ps(dt4x, A->lerpVel4x));
        A->lerpColorAlpha = _mm_min_ps(A->lerpColorAlpha, MMSetExpr(PI32));
        
        __m128 sin4x = _mm_set_ps(Sin(M(A->lerp4x, 0)), 
                                  Sin(M(A->lerp4x, 1)), 
                                  Sin(M(A->lerp4x, 2)), 
                                  Sin(M(A->lerp4x, 3)));
        
        V3_4x P4x = A->startP + A->lerp4x * A->unitDP + sin4x * A->UpVector;
        
        TexturedQuadsCommand* entry = GetCurrentQuads(renderGroup, 4);
        if(entry)
        {
            TexturedVertex* vertBatch = commands->vertexArray + commands->vertexCount;
            commands->vertexCount += (particle4xTriangleCount * 3);
            
            for( u32 subIndex = 0; subIndex < 4; ++subIndex )
            {
                Vec3 P = 
                { 
                    M(P4x.x, subIndex),
                    M(P4x.y, subIndex),
                    M(P4x.z, subIndex)
                };
                Vec4 lightIndexes = GetLightIndexes(worldMode, P);
                
                Vec4 color = 
                {
                    M(A->startC.r, subIndex),
                    M(A->startC.g, subIndex),
                    M(A->startC.b, subIndex),
                    M(A->startC.a, subIndex)
                };
                
                r32 alpha = Sin(M(A->lerpColorAlpha, subIndex));
                color.a = alpha;
                
                u32 C = StoreColor(color);
                
                r32 angle = M(A->angle4x, subIndex);
                
                r32 cos = Cos(angle);
                r32 sin = Sin(angle);
                r32 height = M(A->height4x, subIndex);
                
                
                
                Vec3 XAxisHalf = height * 0.5f * (cos * renderGroup->gameCamera.X + sin * renderGroup->gameCamera.Y);
                Vec3 YAxisHalf = height * 0.5f * (-sin * renderGroup->gameCamera.X + cos * renderGroup->gameCamera.Y);
                
                Vec4 P0 = V4(P - XAxisHalf - YAxisHalf, 0);
                Vec4 P1 = V4(P + XAxisHalf - YAxisHalf, 0);
                Vec4 P2 = V4(P + XAxisHalf + YAxisHalf, 0);
                Vec4 P3 = V4(P - XAxisHalf + YAxisHalf, 0);
                
                
                PushQuad(renderGroup, renderGroup->whiteTexture, lightIndexes,
                         P0, UV, C,
                         P1, UV, C,
                         P2, UV, C,
                         P3, UV, C, 0);
            }
        }
    }
}

internal void InitParticleCache( ParticleCache* particleCache, Assets* assets )
{
    particleCache->particleEntropy = Seed( 1234 );
    
	particleCache->waterRippleSystem.transform = UprightTransform();
    particleCache->waterRippleSystem.bitmapID = GetFirstBitmap(assets, Asset_waterRipple);
    //particleCache->waterRippleSystem.bitmapID = {};
    particleCache->waterRippleSystem.nextParticle4 = 0;
    
    particleCache->ashSystem.transform = UprightTransform();
    particleCache->ashSystem.bitmapID = {};
    particleCache->ashSystem.nextParticle4 = 0;
    
    particleCache->ashSineSystem.transform = UprightTransform();
    particleCache->ashSineSystem.bitmapID = {};
    particleCache->ashSineSystem.nextParticle4 = 0;
    
    particleCache->fireSystem.transform = UprightTransform();
    particleCache->fireSystem.bitmapID = {};
    particleCache->fireSystem.nextParticle4 = 0;
    
    particleCache->waterSystem.transform = UprightTransform();
	particleCache->waterSystem.bitmapID = {};
    particleCache->waterSystem.nextParticle4 = 0;
    
    particleCache->steamSystem.transform = UprightTransform();
    particleCache->steamSystem.bitmapID = {};
    particleCache->steamSystem.nextParticle4 = 0;
    
    particleCache->deltaParticleP = {};
}

internal void UpdateAndRenderParticleSystems(GameModeWorld* worldMode, ParticleCache* particleCache, r32 dt, RenderGroup* group)
{
    Vec3 frameDisplacement = particleCache->deltaParticleP;
    UpdateAndRenderSystem(worldMode, &particleCache->fireSystem, &particleCache->particleEntropy, dt, frameDisplacement, group);
    UpdateAndRenderSystem(worldMode, &particleCache->waterSystem, &particleCache->particleEntropy, dt, frameDisplacement, group);
    
    UpdateAndRenderSystem(worldMode, &particleCache->waterRippleSystem, &particleCache->particleEntropy, dt, frameDisplacement, group);
    
    UpdateAndRenderSystem(worldMode, &particleCache->steamSystem, &particleCache->particleEntropy, dt, frameDisplacement, group);
    UpdateAndRenderSystem(worldMode, &particleCache->ashSystem, &particleCache->particleEntropy, dt, frameDisplacement, group);
    
    UpdateAndRenderSineSystem(worldMode, &particleCache->ashSineSystem, &particleCache->particleEntropy, dt, frameDisplacement, group);
}
