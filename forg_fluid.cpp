/* TODO

-fluid "definition" rules: direction schema, intensity schema, ecc
-fluid "interaction" rules: fuel, viscosity, ecc
-vortons client-side
-gravity server side
-server angle interpolation
-wind effect server-side
-fluid-tile interaction rules
-fluid-entities interaction rules
-storage optimization
-brain or something to handle entities with multiple fluids attached.
-client beautification

*/


inline u32 FluidHashIndex( SimRegion* region, u64 id1, u32 segmentIndex1, u64 id2, u32 segmentIndex2, u32 tileX, u32 tileY, i32 Z )
{
    u32 result = ( u32 ) ( id1 + id2 + segmentIndex1 + segmentIndex2 + tileX + tileY + ( u32 ) Z ) & ( region->fluidHashCount - 1 );
    return result;
}

inline void AddToFluidHash( SimRegion* region, MemoryPool* pool, u64 id1, u32 segmentIndex1, u64 id2, u32 segmentIndex2, u32 entityIndex, RegionTile* tile, i32 Z )
{
    u32 index = FluidHashIndex( region, id1, segmentIndex1, id2, segmentIndex2, tile->X, tile->Y, Z );
    RegionFluidHash* newHash = PushStruct( pool, RegionFluidHash );
    
    newHash->id1 = id1;
    newHash->segmentIndex1 = segmentIndex1;
    newHash->id2 = id2;
    newHash->segmentIndex2 = segmentIndex2;
    newHash->entityIndex = entityIndex;
    newHash->tileX = tile->X;
    newHash->tileY = tile->Y;
    newHash->Z = Z;
    
    newHash->nextInHash = region->fluidHash[index];
    region->fluidHash[index] = newHash;
}

inline SimEntity* GetFluid( SimRegion* region, u64 id1, u32 segmentIndex1, u64 id2, u32 segmentIndex2, RegionTile* tile, i32 Z )
{
    SimEntity* result = 0;
    
    u32 index = FluidHashIndex( region, id1, segmentIndex1, id2, segmentIndex2, tile->X, tile->Y, Z );
    RegionFluidHash* test = region->fluidHash[index];
    
    while( test )
    {
        if( test->id1 == id1 && test->id2 == id2 && 
           test->segmentIndex1 == segmentIndex1 && test->segmentIndex2 == segmentIndex2 &&
           test->tileX == tile->X && test->tileY == tile->Y && test->Z == Z )
        {
            result = GetRegionEntity(region, test->entityIndex);
            break;
        }
        
        test = test->nextInHash;
    }
    
    return result;
}

struct FluidInteractionRule
{
    u32 placeHolder;
};

inline FluidInteractionRule* ShouldInteract( SimRegion* region, u32 f1, u32 f2 )
{
    FluidInteractionRule* result = 0;
    if( f1 == FluidSpawn_Fire && f2 == FluidSpawn_Water )
    {
        result = ( FluidInteractionRule* ) 1;
    }
#if 0
    Hash( f1, f2 );
    
    result = region->server->generationData.fluidRules;
#endif
    
    return result;
}

inline r32 FluidEffectiveness( u32 fluid1, u32 fluid2 )
{
    r32 result = 0.0f;
    if( fluid1 == FluidSpawn_Fire && fluid2 == FluidSpawn_Water )
    {
        result = 1.0f;
    }
    
    return result;
}

inline r32 U8ToUnilateral( u8 value )
{
    r32 result = ( r32 ) value / ( r32 ) 0xff;
    return result;
}

inline u8 UnilateralToU8( r32 value )
{
    Assert( Normalized( value ) );
    u8 result = ( u8 ) ( ( value * 0xff ) / ( r32 ) 0xff );
    return result;
}

internal void ResetRay( FluidRay* ray )
{
    for( u32 segmentIndex = 0; segmentIndex < SEGMENT_COUNT; ++segmentIndex )
    {
        FluidRaySegment* segment = ray->segments + segmentIndex;
        if( !segment->touched )
        {
            segment->intensityPercentage = 1.0f;
        }
        segment->touched = false;
    }
}

inline void  TruncatePercentages( FluidRay* ray, r32 coeff, u32 segmentIndex )
{
    if( segmentIndex < ( SEGMENT_COUNT - 1 ) )
    {
        FluidRaySegment* segment = ray->segments + ( segmentIndex + 1 );
        segment->touched = true;
        segment->intensityPercentage *= coeff;
    }
}

enum LinesOverlapPossibilities
{
    Overlap_None,
    Overlap_EndsInsideMe,
    Overlap_StartsInsideMe,
    Overlap_CompletelyInsideMe,
};

struct LinesOverlapResult
{
    LinesOverlapPossibilities overlap;
    Vec3 deltaP;
};

inline LinesOverlapResult LinesOverlap( Vec3 start1, Vec3 dir1, r32 length1, Vec3 start2, Vec3 dir2, r32 length2 )
{
    LinesOverlapResult result = {};
    
    Vec3 toProject = start2 - start1;
    r32 projectedLength = Dot( toProject, dir1 );
    b32 startsInside = ( projectedLength >= 0 && projectedLength < length1 );
    
    result.deltaP = start2 - ( start1 + dir1 * projectedLength );
    
    Vec3 end2 = start2 + dir2 * length2;
    toProject = end2 - start1;
    projectedLength = Dot( toProject, dir1 );
    b32 endsInside = ( projectedLength >= 0 && projectedLength < length1 );
    
    
    if( startsInside || endsInside )
    {
        result.overlap = Overlap_StartsInsideMe;
        if( startsInside && endsInside )
        {
            result.overlap = Overlap_CompletelyInsideMe;
        }
        else if( endsInside )
        {
            result.overlap = Overlap_EndsInsideMe;
        }
    }
    return result;
}

#define EPSILON 0.01f
inline r32 GetMinDistanceSq( Vec3 p1, Vec3 q1, Vec3 p2, Vec3 q2, r32* s, r32* t, Vec3* c1, Vec3* c2 )
{
    Vec3 d1 = q1 - p1; // Direction vector of segment S1
    Vec3 d2 = q2 - p2; // Direction vector of segment S2
    Vec3 r = p1 - p2;
    float a = Dot(d1, d1); // Squared length of segment S1, always nonnegative
    float e = Dot(d2, d2); // Squared length of segment S2, always nonnegative
    float f = Dot(d2, r);
    // Check if either or both segments degenerate into points
    if (a <= EPSILON && e <= EPSILON) 
    {
        // Both segments degenerate into points
        *s = *t = 0.0f;
        *c1 = p1;
        *c2 = p2;
        return LengthSq( p1 - p2 );
    }
    
    if (a <= EPSILON) 
    {
        // First segment degenerates into a point
        *s = 0.0f;
        // s = 0 => t = (b*s + f) / e = f / e
        *t = Clamp(f / e, 0.0f, 1.0f);
    }
    else
    {
        float c = Dot(d1, r);
        if (e <= EPSILON)
        {
            // Second segment degenerates into a point
            *t = 0.0f;
            *s = Clamp(-c / a, 0.0f, 1.0f); // t = 0 => s = (b*t - c) / a = -c / a
        }
        else
        {
            // The general nondegenerate case starts here
            float b = Dot(d1, d2);
            float denom = a*e-b*b; // Always nonnegative
            // If segments not parallel, compute closest point on L1 to L2 and
            // clamp to segment S1. Else pick arbitrary s (here 0)
            if (denom != 0.0f) 
            {
                *s = Clamp((b*f - c*e) / denom, 0.0f, 1.0f);
            }
            else 
            {
                *s = 0.0f;
            }
            
            
            // Compute point on L2 closest to S1(s) using
            // t = Dot((P1 + D1*s) - P2,D2) / Dot(D2,D2) = (b*s + f) / e
            *t = (b*(*s ) + f) / e;
            // If t in [0,1] done. Else clamp t, recompute s for the new value
            // of t using s = Dot((P2 + D2*t) - P1,D1) / Dot(D1,D1)= (t*b - c) / a
            // and clamp s to [0, 1]
            if (*t < 0.0f)
            {
                *t = 0.0f;
                *s = Clamp(-c / a, 0.0f, 1.0f);
            } 
            else if (*t > 1.0f)
            {
                *t = 1.0f;
                *s = Clamp((b - c) / a, 0.0f, 1.0f);
            }
        }
    }
    *c1 = p1 + d1 * ( *s );
    *c2 = p2 + d2 * ( *t );
    return LengthSq( *c1 - *c2 );
}

inline r32 GetMinDistanceSq( Vec3 p1, Vec3 q1, Vec3 p2, Vec3 q2 )
{
    r32 ign1, ign2;
    Vec3 ign3, ign4;
    r32 result = GetMinDistanceSq( p1, q1, p2, q2, &ign1, &ign2, &ign3, &ign4 );
    return result;
}

inline u32 GetIndex( Vec3 clashP, r32 radious )
{
    r32 segmentLength = radious / ( SEGMENT_COUNT - 1 );
    u32 result = TruncateReal32ToU32( Length( clashP ) / segmentLength );
    
    Assert( result < SEGMENT_COUNT );
    return result;
}


inline RegionTile* GetRegionTile( SimRegion* region, Vec2 tileXY );
internal void HandleFluidInteraction( SimRegion* region, Vec3 fluidP, Fluid* fluid, u64 i1, Vec3 testP, Fluid* test, u64 i2 )
{
    for( u32 rayIndex1 = 0; rayIndex1 < ArrayCount( fluid->rays ); ++rayIndex1 )
    {
        FluidRay* ray = fluid->rays + rayIndex1;
        if( ray->lengthPercentage && ray->segments[0].intensityPercentage )
        {       
            Vec3 myDirNorm = fluid->direction * fluidVectors[rayIndex1];
            
            r32 rayMaxLength = fluid->length * ray->lengthPercentage;
            r32 rayLength = GetRayLength( rayMaxLength, ray );
            
            Vec3 myDir = myDirNorm * rayLength;
            
            for( u32 rayIndex2 = 0; rayIndex2 < ArrayCount( test->rays ); ++rayIndex2 )
            {
                FluidRay* hisRay = test->rays + rayIndex2;
                if( hisRay->lengthPercentage && hisRay->segments[0].intensityPercentage )
                {
                    b32 collide = false;
                    Vec3 clashP = {};
                    u32 myStartingIndex = 0;
                    u32 hisStartingIndex = 0;
                    
                    Vec3 hisDirNorm = test->direction * fluidVectors[rayIndex2];
                    
                    r32 hisRayMaxLength = test->length * hisRay->lengthPercentage;
                    r32 hisRayLength = GetRayLength( hisRayMaxLength, hisRay );
                    Vec3 hisDir = hisDirNorm * hisRayLength;
                    
                    u32 toAddCount = 0;
                    m4x4 directionsToAdd[2];
                    
                    r32 myCoeff = 1.0f;
                    r32 hisCoeff = 1.0f;
                    
                    r32 dot = Dot( myDirNorm, hisDirNorm );
                    if( dot == 1.0f )
                    {
                        LinesOverlapResult overlap = LinesOverlap( fluidP, myDirNorm, rayLength, testP, hisDirNorm, hisRayLength );
                        if( overlap.overlap )
                        {
                            Vec3 myMaxP = testP - overlap.deltaP;
                            Vec3 hisMaxP = testP;
                            
                            r32 maxDistanceSq = Square( 1.0f );
                            
                            r32 ignored1;
                            r32 ignored2;
                            Vec3 ign1;
                            Vec3 ign2;
                            r32 distanceSq = GetMinDistanceSq( fluidP, myMaxP, testP, hisMaxP, &ignored1, &ignored2, &ign1, &ign2 );
                            if( distanceSq < maxDistanceSq )
                            {
                                toAddCount = 1;
                                directionsToAdd[0] = fluid->direction;
                                
                                r32 myEffectiveness = FluidEffectiveness( fluid->type, test->type );
                                r32 hisEffectiveness = 1.0f - myEffectiveness;
                                
                                if( overlap.overlap == Overlap_EndsInsideMe )
                                {
                                    myMaxP = fluidP;
                                    hisMaxP = fluidP + overlap.deltaP;
                                }
                                
                                myCoeff = myEffectiveness;
                                hisCoeff = hisEffectiveness;
                                
                                Vec3 myClashP = myMaxP;
                                Vec3 hisClashP = hisMaxP;
                                
                                clashP = myClashP + ( 0.5f * overlap.deltaP );                           
                                
                                myStartingIndex = GetIndex( myClashP - fluidP, rayMaxLength );
                                hisStartingIndex = GetIndex( ( myClashP + overlap.deltaP ) - testP, hisRayMaxLength );
                                
                                collide = true;
                            }
                        }
                    }
                    else if( dot == -1.0f )
                    {
                        LinesOverlapResult overlap = LinesOverlap( fluidP, myDirNorm, rayLength, testP, hisDirNorm, hisRayLength );
                        if(  overlap.overlap )
                        {
                            Vec3 myMaxP = fluidP + myDir;
                            Vec3 hisMaxP = testP + hisDir;
                            
                            r32 maxDistanceSq = Square( 1.0f );
                            
                            r32 ignored1;
                            r32 ignored2;
                            Vec3 ign1;
                            Vec3 ign2;
                            r32 distanceSq = GetMinDistanceSq( fluidP, myMaxP, testP, hisMaxP, &ignored1, &ignored2, &ign1, &ign2 );
                            if( distanceSq < maxDistanceSq )
                            {
                                toAddCount = 2;
                                directionsToAdd[0] = fluid->direction;
                                directionsToAdd[1] = test->direction;
                                
                                r32 myEffectiveness = FluidEffectiveness( fluid->type, test->type );
                                r32 hisEffectiveness = 1.0f - myEffectiveness;
                                
                                if( overlap.overlap == Overlap_StartsInsideMe )
                                {
                                    myMaxP = testP - overlap.deltaP;
                                    hisMaxP = fluidP;
                                }
                                else if( overlap.overlap == Overlap_CompletelyInsideMe )
                                {
                                    myMaxP = testP - overlap.deltaP;
                                }
                                
                                // TODO(Leonardo): put them on the same line!
                                Vec3 deltaEffect = ( myMaxP + overlap.deltaP ) - hisMaxP;
                                
                                Vec3 myClashP = myMaxP - deltaEffect * hisEffectiveness;
                                Vec3 hisClashP = hisMaxP + deltaEffect * myEffectiveness;
                                
                                myCoeff = myEffectiveness;
                                hisCoeff = hisEffectiveness;
                                
                                clashP = myClashP + ( 0.5f * overlap.deltaP ) - hisEffectiveness * deltaEffect;                           
                                
                                myStartingIndex = GetIndex( myClashP - fluidP, rayMaxLength );
                                hisStartingIndex = GetIndex( ( myClashP + overlap.deltaP ) - testP, hisRayMaxLength );
                                
                                collide = true;
                            }
                        }
                    }
                    else
                    {
                        Vec3 myMaxP = fluidP + myDir;
                        Vec3 hisMaxP = testP + hisDir;
                        
                        r32 maxDistanceSq = Square( 1.0f );
                        
                        r32 p1;
                        r32 p2;
                        Vec3 c1;
                        Vec3 c2;
                        
                        r32 distanceSq = GetMinDistanceSq( fluidP, myMaxP, testP, hisMaxP, &p1, &p2, &c1, &c2 );
                        if( distanceSq < maxDistanceSq )
                        {
                            toAddCount = 1;
                            directionsToAdd[0] = fluid->direction;
                            
                            r32 myEffectiveness = FluidEffectiveness( fluid->type, test->type );
                            r32 hisEffectiveness = 1.0f - myEffectiveness;
                            
                            myCoeff = myEffectiveness;
                            hisCoeff = hisEffectiveness;
                            
                            clashP = ( c1 + c2 ) * 0.5f;
                            
                            myStartingIndex = GetIndex( c1 - fluidP, rayMaxLength );
                            hisStartingIndex = GetIndex( c2 - testP, hisRayMaxLength );
                            
                            collide = true;
                        }
                    }
                    
                    if( collide )
                    {
                        TruncatePercentages( ray, myCoeff, myStartingIndex );
                        TruncatePercentages( hisRay, hisCoeff, hisStartingIndex );
                        
                        u32 taxonomy = NORUNTIMEGetTaxonomySlotByName( region->taxTable, "steam" )->taxonomy;
                        RegionTile* tile = GetRegionTile( region, clashP.xy );
                        SimEntity* steam = GetFluid( region, i1, myStartingIndex, i2, hisStartingIndex, tile, ( i32 ) clashP.z );
                        
                        if(!steam)
                        {
                            AddEntity(region, clashP, taxonomy, NullGenerationData(),  FluidDirection(i1, myStartingIndex, i2, hisStartingIndex));
                        }
                        else
                        {
                            FluidComponent* fluidComp = Fluid(region, steam);
                            fluidComp->fluid.type = FluidSpawn_Steam;
                            fluidComp->fluid.length = 1.0f;
                            fluidComp->fluid.intensity = 0.5f * ( fluid->intensity + test->intensity );
                            fluidComp->fluid.fuel += 2.0f * region->timeToUpdate;
                            fluidComp->fluid.rays[0].lengthPercentage = 1.0f;
                            fluidComp->fluid.direction = directionsToAdd[0];
                        }
                    }
                }
            }
        }
    }
}

internal void HandleFluid( SimRegion* region, SimEntity* entity )
{
    FluidComponent* fluidComp = Fluid(region, entity);
    Fluid* fluid = &fluidComp->fluid;
    fluid->fuel -= 1.0f * region->timeToUpdate;
    
    if( fluid->fuel <= 0.0f )
    {
        AddFlags( entity, Flag_deleted );
    }
    else
    { 
        for( u32 rayIndex = 0; rayIndex < ArrayCount( fluid->rays ); ++rayIndex )
        {
            FluidRay* ray = fluid->rays + rayIndex;
            ResetRay( ray );
        }
        
        for(SimEntityBlock* entityBlock = region->firstEntityBlock; entityBlock; entityBlock = entityBlock->next)
        {
            for(u32 entityIndex = 0; entityIndex < entityBlock->entityCount; ++entityIndex)
            {
                SimEntity* testEntity = entityBlock->entities + entityIndex;
                if(testEntity->IDs[Component_Fluid])
                {
                    FluidComponent* testFluid = Fluid(region, testEntity);
                    Fluid* test = &testFluid->fluid;
                    if( test->type && testEntity != entity )
                    {
                        FluidInteractionRule* rule = ShouldInteract( region, fluid->type, test->type );
                        if( rule )
                        {
                            HandleFluidInteraction( region, entity->P, fluid, entity->identifier, testEntity->P, test, testEntity->identifier );
                        }
                    }
                }
            }
        }
        
        if( fluid->type == FluidSpawn_Water )
        {
            for( u32 rayIndex = 0; rayIndex < ArrayCount( fluid->rays ); ++rayIndex )
            {
                FluidRay* ray = fluid->rays + rayIndex;
                if( ray->lengthPercentage && ray->segments[0].intensityPercentage )
                {       
                    Vec3 myDirNorm = fluid->direction * fluidVectors[rayIndex];
                    
                    r32 rayMaxLength = fluid->length * ray->lengthPercentage;
                    r32 rayLength = GetRayLength( rayMaxLength, ray );
                    
                    Vec3 rayMaxP = entity->P + myDirNorm * rayLength;
                    if( rayLength )
                    {
                        for( u32 tileY = 0; tileY < region->gridSide; ++tileY )
                        {
                            for( u32 tileX = 0; tileX < region->gridSide; ++tileX )
                            {
                                RegionTile* tile = region->tiles + ( tileY * region->gridSide ) + tileX;
                                
                                r32 maxDistanceSq = Square( 0.2f );
                                r32 distanceSq = GetMinDistanceSq( entity->P, rayMaxP, tile->P, tile->P );
                                if( distanceSq < maxDistanceSq )
                                {
                                    tile->waterAmount += 2.1f * region->timeToUpdate;
                                    tile->dirty = true;
                                    fluid->fuel -= 0.1f * region->timeToUpdate;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

