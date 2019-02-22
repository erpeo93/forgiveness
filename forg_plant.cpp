/*   TODO
-fire effect
-roots
-flowers
-fruits

The server, for every plant type, defines at what month during the year the plant change phase: making leafs, making flowers, ecc ecc
and also the type: annual, biannual, undefined lifetime
-seasons! alternate leaf growth and fall
-simulate the accumulation of energy, and simulate the consuption of that energy in winter, updating the plant health.
*/


inline r32 GetMaxRadiousAtLevel( PlantParams* params, u8 level, u8 segment )
{
    Assert( level <= MAX_LEVELS );
    Assert( segment <= params->maxSegmentNumber[level] );
    
    r32 result = params->maxRadious[level];
    
    for( u32 segmentIndex = 0; segmentIndex < segment; ++segmentIndex )
    {
        result *= params->segmentBySegmentRadiousCoeff[level];
    }
    
    return result;
}

inline r32 GetSegmentLengthAtLevel( PlantParams* params, PlantSegmentType type, u8 level, u8 segment )
{
    Assert( level <= MAX_LEVELS );
    Assert( segment <= params->maxSegmentNumber[level] );
    
    r32 result = 0;
    if( type == PlantSegment_Branch )
    {
        result = params->segmentLengths[level];
        for( u32 segmentIndex = 0; segmentIndex < segment; ++segmentIndex )
        {
            result *= params->segmentBySegmentLengthCoeff[level];
        }
    }
    
    return result;
}

inline u8 GetMaxSegmentNumberAtLevel( PlantParams* params, u8 level, u8 parentSegment )
{
    Assert( level <= MAX_LEVELS );
    r32 result = params->maxSegmentNumber[level];
    
    if( level )
    {
        u8 parentLevel = level - 1;
        Assert( parentSegment <= params->maxSegmentNumber[parentLevel] );
        for( u32 segmentIndex = 0; segmentIndex < parentSegment; ++segmentIndex )
        {
            result *= params->segmentBySegmentBranchLengthCoeff[parentLevel];
        }
    }
    
    return ( u8 ) result;
}

inline r32 GetLeafSizeAtLevel( PlantParams* params, u8 level )
{
    Assert( level <= MAX_LEVELS );
    r32 result = params->leafSize[level];
    
    return result;
}

inline r32 GetWindEffectAtLevel( PlantParams* params, r32 radious, u8 level, u8 segment )
{
    r32 result = 0;
    
    //TODO(leonardo): use something more realistic than a linear function!
    r32 radiousNorm = 1.0f - ( radious * params->oneOverMaxRadious );
    
    r32 strength = params->plantStrength[level];
    for( u8 segmentIndex = 0; segmentIndex < segment; ++segmentIndex )
    {
        strength *= params->segmentBySegmentStrengthCoeff[level];
    }
    result = radiousNorm * ( 1.0f - strength );
    return result;
}


inline PlantSegment* GetFreeSegment(GameModeWorld* worldMode)
{
    PlantSegment* result = worldMode->firstFreePlantSegment;
    if(!result)
    {
        result = PushStruct(&worldMode->entityPool, PlantSegment);
    }
    else
    {
        worldMode->firstFreePlantSegment = result->nextFree;
    }
    return result;
}

inline PlantSegment* CreateNewSegment( PlantParams* params, GameModeWorld* worldMode, u32 seed, u32 position, PlantSegmentType type, PlantAngle angleX, PlantAngle angleY, r32 centerDelta, u8 level, u8 segmentIndex, u8 parentSegmentIndex )
{
    PlantSegment* newSegment = GetFreeSegment(worldMode);
    
    newSegment->type = type;
    newSegment->segmentIndex = segmentIndex;
    newSegment->parentSegmentIndex = parentSegmentIndex;
    newSegment->level = level;
    
    newSegment->angleX = angleX;
    newSegment->angleY = angleY;
    newSegment->centerDelta = centerDelta;
    
    newSegment->radious = 0;
    newSegment->length = 0;
    
    newSegment->leafsSeed = seed;
    newSegment->leafCount = 0;
    
    return newSegment;
    
}

inline void AddLeaf( PlantSegment* root, u32* leafCount )
{
    Leaf* newLeaf = root->leafs + root->leafCount++;
    ++*leafCount;
}

inline PlantAngle NewAngle( RandomSequence* sequence, RangeDistribution* distr )
{
    b32 negative = false;
    r32 angle = ThrowDices( sequence, distr );
    if( angle < 0 )
    {
        negative = true;
        angle = -angle;
    }
    
    PlantAngle result = {};
    result.negative = negative;
    result.absValue = angle;
    
    return result;
}

inline PlantAngle Sum( PlantAngle a1, PlantAngle a2 )
{
    PlantAngle result;
    r32 A1 = a1.negative ? -a1.absValue : a1.absValue;
    r32 A2 = a2.negative ? -a2.absValue : a2.absValue;
    
    r32 r = A1 + A2;
    
    result.negative = ( r < 0.0f );
    result.absValue = result.negative ? -r : r;
    
    return result;
}

internal void ApplyRule(GameModeWorld* worldMode, PlantSegment* root, RandomSequence* sequence, PlantParams* params, PlantRule* rule, u32* leafCount)
{
    for( u32 newSegmentIndex = 0; newSegmentIndex < ArrayCount( rule->newSegments ); ++newSegmentIndex )
    {
        NewSegment* newSegment = rule->newSegments + newSegmentIndex;
        if( newSegment->type )
        {
            u8 segmentIndex = ( newSegmentIndex == PlantChild_Top ) ? ( root->segmentIndex + 1 ) : 0;
            u8 parentSegmentIndex = ( newSegmentIndex == PlantChild_Top ) ? ( root->parentSegmentIndex ) : root->segmentIndex;
            
            PlantAngle angleX = NewAngle( sequence, &newSegment->angleXDistr );
            PlantAngle angleY = NewAngle( sequence, &newSegment->angleYDistr );
            r32 centerDelta = RandomBil(sequence) * params->maxCenterDelta;
            
            u32 segmentSeed;
            if( newSegmentIndex == PlantChild_Top )
            {
                PlantAngle a1 = root->angleX;
                a1.absValue *= 1.1f;
                
                PlantAngle a2 = root->angleY;
                a2.absValue *= 1.1f;
                
                angleX = Sum( angleX, a1 );
                angleY = Sum( angleY, a2 );
                segmentSeed = root->leafsSeed;
            }
            else
            {
                segmentSeed = GetNextUInt32( sequence );
            }
            
            angleX.phase = root->angleX.phase;
            angleY.phase = root->angleY.phase;
            
            root->childs[newSegmentIndex] = CreateNewSegment(params, worldMode, segmentSeed, newSegmentIndex, newSegment->type, angleX, angleY, centerDelta, root->level + newSegment->levelDelta, segmentIndex, parentSegmentIndex);
        }
    }
    
    root->radious += rule->radiousIncrement;
    root->radious = Min( root->radious, GetMaxRadiousAtLevel( params, root->level, root->segmentIndex ) );
    
    root->length = GetSegmentLengthAtLevel( params, root->type, root->level, root->segmentIndex );
    root->type = rule->newType;
    
    Assert( ( root->leafCount + rule->leafCountDelta ) <= ArrayCount( root->leafs ) );
    
    //Assert( rule->leafCountDelta == 1 || rule->leafCountDelta == 0 );
    for( u8 leafIndex = 0; leafIndex < rule->leafCountDelta; ++leafIndex )
    {
        AddLeaf( root, leafCount );
    }
}

internal void AdvancePlantLife(GameModeWorld* worldMode, PlantParams* params, PlantSegment* root, RandomSequence* sequence, u32* leafCount )
{
    if( root )
    {
        for( u32 childIndex = 0; childIndex < ArrayCount( root->childs ); ++childIndex )
        {
            AdvancePlantLife( worldMode, params, root->childs[childIndex], sequence, leafCount );
        }
        
        for( u32 leafIndex = 0; leafIndex < root->leafCount; ++leafIndex )
        {
            Leaf* leaf = root->leafs + leafIndex;
            leaf->present = true;
        }
        
        PlantRule* rule = 0;
        for( u32 ruleIndex = 0; ruleIndex < params->ruleCount; ++ruleIndex )
        {
            PlantRule* test = params->rules + ruleIndex;
            if( root->type == test->requestedType &&
               root->level < params->maxLevels &&
               root->segmentIndex < GetMaxSegmentNumberAtLevel( params, root->level, root->parentSegmentIndex ) )
            {
                b32 satisfied = true;
                
                for( u32 reqIndex = 0; reqIndex < test->specialRequirementCount; ++reqIndex )
                {
                    SpecialRuleRequirement* req = test->specialRequirement + reqIndex;
                    if( req->level == root->level && root->segmentIndex < req->minSegmentIndex )
                    {
                        satisfied = false;
                        break;
                    }
                }
                
                if( satisfied )
                {
                    rule = test;
                }
            }
        }
        
        if( rule )
        {
            ApplyRule( worldMode, root, sequence, params, rule, leafCount );
        }
    }
}

struct PlantRenderingParams
{
    r32 lerp;
    u32 plantIndex;
    r32 defaultLeafSize; 
    r32 plantHealth;
    Vec3 windDirection;
    r32 windStrength;
    
    r32 modulationPercentageWithFocusColor;
};

internal void RenderPlant( RenderGroup* group, Vec4 lightIndexes, PlantParams* params, PlantSegment* root, PlantSegment* futureRoot, Vec3 baseP, m4x4 coordinateSystem, r32 maxBottomRadious, PlantRenderingParams* renderingParams, r32 timeToUpdate );
internal void RenderLateralChild( RenderGroup* group, Vec4 lightIndexes, PlantParams* params, PlantSegment* child, PlantSegment* futureChild, Vec3 baseP, Vec3 topP, r32 radiousBottom, r32 radiousTop, Vec3 YAxis, b32 invert, PlantRenderingParams* renderingParams, r32 timeToUpdate )
{
    Vec3 axis = topP - baseP;
    
    // TODO(Leonardo): work this out!
    r32 maxChildBottomRadious = Min( 0.5f * Length( axis ), 0.5f * radiousTop );
    Vec3 renderP = baseP + (0.5f + child->centerDelta) * axis;
    
    if( invert )
    {
        axis = -axis;
    }
    
    Vec3 X = Normalize( axis );
    Vec3 Y = YAxis;
    Vec3 Z = Cross( X, Y );
    m4x4 matrix = Columns3x3( X, Y, Z );
    
    RenderPlant( group, lightIndexes, params, child, futureChild, renderP, matrix, maxChildBottomRadious, renderingParams, timeToUpdate );
}

inline r32 AdvancePhaseAndGetRadiants( PlantParams* params, Vec3 plantAxis, u8 level, u8 segmentIndex, r32 radious, r32 health, PlantAnglePhase* phase, Vec3 windDirection, r32 windStrength, r32 timeToUpdate)
{
    Assert(Normalized(windStrength));
    r32 influence = Dot(windDirection, plantAxis) * windStrength;
    influence = windStrength;
    
    r32 coeff = GetWindEffectAtLevel(params, radious, level, segmentIndex);
    Assert(Normalized(coeff));
    
    if(phase->arrivedAtMaxAngle)
    {
        phase->phase -= influence * coeff * timeToUpdate; //+ Random();
        if(phase->phase <= 0)
        {
            phase->arrivedAtMaxAngle = false;
        }
    }
    else
    {
        phase->phase += influence * coeff * timeToUpdate;// + Random();
        if(phase->phase >= 1.0f)
        {
            phase->arrivedAtMaxAngle = true;
        }
    }
    
    phase->phase = Clamp01(phase->phase);
    r32 maxAngleRotation = DegToRad(10.0f);
    r32 result = phase->phase * maxAngleRotation;
    return result;
}

inline r32 GetHealthAngle(PlantParams* params, r32 health)
{
    return 0;
}

internal void RenderPlant( RenderGroup* group, Vec4 lightIndexes, PlantParams* params, PlantSegment* root, PlantSegment* futureRoot, Vec3 baseP, m4x4 coordinateSystem, r32 maxBottomRadious,  PlantRenderingParams* rp, r32 timeToUpdate )
{
    r32 plantHealth = rp->plantHealth;
    r32 lerp = rp->lerp;
    Assert(Normalized(plantHealth));
    r32 sizeGeneralCoeff = 1.0f;
    
    if(root)
    {
        BEGIN_BLOCK("plant render setup");
        r32 height = Lerp(root->length, lerp, futureRoot->length);  
        r32 radiousBottom = Lerp(root->radious, lerp, futureRoot->radious);
        
        r32 radiousTopCoeff = 1.0f;
        if(radiousBottom > maxBottomRadious)
        {
            radiousTopCoeff = maxBottomRadious / radiousBottom;
            radiousBottom = maxBottomRadious;
        }
        
        Vec3 bottomXAxis = GetColumn(coordinateSystem, 0);
        Vec3 bottomYAxis = GetColumn(coordinateSystem, 1);
        Vec3 bottomZAxis = GetColumn(coordinateSystem, 2);
        
        
        b32 isColliding = false;
        b32 wasColliding = false;
        
        r32 angleX = 0;
        r32 angleY = 0;
        
        b32 negativeX = false;
        b32 negativeY = false;
        
        if( isColliding )
        {
            //phase += 0.2f;
        }
        else
        {
            if( wasColliding )
            {
                wasColliding = false;
                //arrivedAtMaxAngle = true;
            }
            
            
            if( root->segmentIndex == 0 )
            {
                //angleX = GetHealthAngle( params, plantHealth ) + AdvancePhaseAndGetRadiants( params, bottomYAxis, root->level, root->segmentIndex, root->radious, plantHealth, &root->angleX.phase, rp->windDirection, rp->windStrength, timeToUpdate );
                //angleY = GetHealthAngle( params, plantHealth ) + AdvancePhaseAndGetRadiants( params, bottomXAxis, root->level, root->segmentIndex, root->radious, plantHealth, &root->angleY.phase, rp->windDirection, rp->windStrength, timeToUpdate );
            }
        }
        
        r32 radiousTop;
        PlantSegment* topChild = root->childs[PlantChild_Top];
        PlantSegment* topChildFuture = futureRoot->childs[PlantChild_Top];
        if( topChild )
        {
            Assert( topChildFuture );
            
            r32 r1 = topChild->radious;
            r32 r2 = topChildFuture->radious;
            if( !topChild->radious )
            {
                r1 = 0.4f * root->radious;
                r2 = 0.4f * futureRoot->radious;
            }
            
            
            Assert( topChild->angleX.negative == topChildFuture->angleX.negative );
            Assert( topChild->angleY.negative == topChildFuture->angleY.negative );
            
            negativeX = topChild->angleX.negative;
            negativeY = topChild->angleY.negative;
            
            //angleX += Lerp( topChild->angleX.absValue, lerp, topChildFuture->angleX.absValue );
            //angleY += Lerp( topChild->angleY.absValue, lerp, topChildFuture->angleY.absValue );
            
            radiousTop = Lerp( r1, lerp, r2 );
            radiousTop *= radiousTopCoeff;
        }
        else
        {
            Assert( root->type == PlantSegment_Meristem );
            radiousTop = Lerp( root->radious * 0.5f, lerp, futureRoot->radious * 0.5f );
        }
        
        
        if( negativeX )
        {
            angleX = -angleX;
        }
        
        if( negativeY )
        {
            angleY = -angleY;
        }
        END_BLOCK();
        
        
        
        BEGIN_BLOCK("render plant matrix");
        m4x4 top = XRotation( angleX ) * YRotation( angleY ) * coordinateSystem;
        Vec3 topXAxis = GetColumn( top, 0 );
        Vec3 topYAxis = GetColumn( top, 1 );
        Vec3 topZAxis = GetColumn( top, 2 );
        END_BLOCK();
        
        
        BEGIN_BLOCK("actual plant rendering");
        // NOTE(Leonardo): current segment
        Vec4 color = Lerp( params->branchColorDead, plantHealth, params->branchColorAlive );      
        color = params->branchColorAlive;
        PushTrunkatedPyramid( group, baseP, 4, bottomXAxis, bottomYAxis, topZAxis, topXAxis, topYAxis, radiousBottom * sizeGeneralCoeff, radiousTop * sizeGeneralCoeff, height * sizeGeneralCoeff, color, lightIndexes, rp->modulationPercentageWithFocusColor );
        
        // NOTE(Leonardo): draw top child
        Vec3 topP = baseP + height * topZAxis;
        RenderPlant( group, lightIndexes, params, root->childs[PlantChild_Top], futureRoot->childs[PlantChild_Top], topP, top, radiousTop, rp, timeToUpdate );
        
        // NOTE(Leonardo): draw lateral childs
        if(root->childs[PlantChild_Right])
        {
            RenderLateralChild( group, lightIndexes, params, root->childs[PlantChild_Right], futureRoot->childs[PlantChild_Right], baseP, topP, radiousBottom, radiousTop, bottomYAxis, true, rp, timeToUpdate );
        }
        
        if(root->childs[PlantChild_Left])
        {
            RenderLateralChild( group, lightIndexes, params, root->childs[PlantChild_Left], futureRoot->childs[PlantChild_Left], baseP, topP, radiousBottom, radiousTop, bottomYAxis, false, rp, timeToUpdate );
        }
        
        
        if(root->childs[PlantChild_Up])
        {
            RenderLateralChild( group, lightIndexes, params, root->childs[PlantChild_Up], futureRoot->childs[PlantChild_Up], baseP, topP, radiousBottom, radiousTop, bottomXAxis, true, rp, timeToUpdate );
        }
        
        if(root->childs[PlantChild_Down])
        {
            RenderLateralChild( group, lightIndexes, params, root->childs[PlantChild_Down], futureRoot->childs[PlantChild_Down], baseP, topP, radiousBottom, radiousTop, bottomXAxis, false, rp, timeToUpdate );
        }
        END_BLOCK();
        
        
        
        BEGIN_BLOCK("leaf render");
        if(root->leafCount)
        {
            // NOTE(Leonardo): draw leafs
            RandomSequence leafSeq = Seed( root->leafsSeed + rp->plantIndex );
            r32 leafSize = GetLeafSizeAtLevel( params, root->level );
            r32 leafSizeWhenDead = 1.0f;
            r32 sizeNorm  = leafSize * rp->defaultLeafSize * Lerp( leafSizeWhenDead, plantHealth, 1.0f );    
            
            r32 leafRadious = params->leafRadious;// + 0.1f * RandomBil( &leafSeq );
            r32 leafLength = sizeNorm * params->leafLength;
            
            BitmapId leafID = GetFirstBitmap(group->assets, Asset_leaf);
            ObjectTransform leafTransform = UprightTransform();
            leafTransform.modulationPercentage = rp->modulationPercentageWithFocusColor;
            for( u8 leafIndex = 0; leafIndex < root->leafCount; ++leafIndex )
            {
                Leaf* leaf = root->leafs + leafIndex; 
                if( leaf->present )
                {
                    // TODO(Leonardo): lerp with future leaf size!
                    //Leaf* futureLeaf = futureRoot->leafs + leafIndex;
                    Vec3 leafP = topP;  
                    
                    Vec4 leafColor = params->leafColorAlive;
                    //leafColor.rgb += RandomizeBil(params->leafColorRandomization, &leafSeq);
                    
                    
                    Vec2 leafScale = V2(0.3f, 1);
#if 0                    
                    Vec2 leafScale = V2(RandomRangeFloat(&leafSeq, 0.5f, 2.0f), RandomRangeFloat(&leafSeq, 0.5f, 2.0f));
#endif
                    
                    leafTransform.angle = RandomRangeFloat(&leafSeq, -75, 75);
                    leafTransform.additionalZBias += 0.01f;
                    
                    PushBitmap(group, leafTransform, leafID, leafP, params->leafLength, leafScale, leafColor, lightIndexes);
                }
            }
        }
        
        END_BLOCK();
    }
}

internal u32 LooseLeafs( PlantSegment* root, u32 leafCount, u32* indexes, u32* leafCountTotal )
{
    u32 deleted = 0;
    if( root )
    {
        for( u32 leafIndex = 0; leafIndex < root->leafCount; ++leafIndex )
        {
            b32 toDelete = false;
            for( u32 testIndex = 0; testIndex < leafCount; ++testIndex )
            {
                if( indexes[testIndex] == *leafCountTotal )
                {
                    toDelete = true;
                    break;
                }
            }
            if( toDelete )
            {
                Leaf* leaf = root->leafs + leafIndex;
                if( leaf->present )
                {
                    leaf->present = false;
                    ++deleted;
                }
            }
            
            ++*leafCountTotal;
        }
        
        for( u32 posIndex = 0; posIndex < ArrayCount( root->childs ); ++posIndex )
        {
            deleted += LooseLeafs( root->childs[posIndex], leafCount, indexes, leafCountTotal );
        }
    }
    
    return deleted;
}




internal void UpdateAndRenderPlant(GameModeWorld* worldMode, RenderGroup* group, TileInfo tileInfo,  ClientEntity* entityC, PlantParams* params, r32 timeToUpdate)
{
    u64 id = entityC->identifier;
    r32 age = entityC->plantTotalAge;
    if(!entityC->plant)
    {
        ClientPlant* newPlant = worldMode->firstFreePlant;
        if(!newPlant)
        {
            newPlant = PushStruct(&worldMode->entityPool, ClientPlant);
        }
        else
        {
            worldMode->firstFreePlant = newPlant->nextFree;
        }
        entityC->plant = newPlant;
        
        ClientPlant* plant = entityC->plant;
        *plant = {};
        
        plant->root = GetFreeSegment(worldMode);
        *plant->root = {};
        plant->root->type = PlantSegment_Meristem;
        plant->root->leafsSeed = ( i32 ) id;
        plant->sequence = Seed( ( i32 ) id );
        
        plant->futureRoot = GetFreeSegment(worldMode);
        *plant->futureRoot = *plant->root;
        plant->futureRoot->leafsSeed = ( i32 ) id;
        
        if(params->isHerbaceous)
        {
            u32 leafCount = 4;
            for( u32 leafIndex = 0; leafIndex < leafCount; ++leafIndex )
            {
                AddLeaf( plant->root, &plant->leafCount );
                AddLeaf( plant->futureRoot, &plant->futureLeafCount );
            }
        }
        
        plant->futureSequence = Seed( ( i32 ) id );
        
        
        plant->defaultLeafSize = 0.0f;
        AdvancePlantLife( worldMode, params, plant->futureRoot, &plant->futureSequence, &plant->futureLeafCount );
    }
    
    ClientPlant* plant = entityC->plant;
    
    r32 percentageOfLeafsToLoose = 0;
    switch( entityC->plantStatus )
    {
        case PlantLife_NewBranches:
        {
            if(plant->oldStatus == PlantLife_LooseLeafs)
            {
                plant->defaultLeafSize = 0;
            }
        } break;
        
        case PlantLife_NewLeafs:
        {
            plant->defaultLeafSize = entityC->plantStatusPercentage;
        } break;
        
        case PlantLife_LooseLeafs:
        {
            if( plant->oldStatus != PlantLife_LooseLeafs )
            {
                plant->loosedLeafPercentage = 0.0f;
            }
            percentageOfLeafsToLoose = entityC->plantStatusPercentage - plant->loosedLeafPercentage;
        } break;
        
        case PlantLife_Quiescent:
        {
            
        } break;
    }
    plant->oldStatus = entityC->plantStatus;
    plant->oldStatusPercentage = entityC->plantStatusPercentage;
    
    percentageOfLeafsToLoose = Clamp01( percentageOfLeafsToLoose );
    if( percentageOfLeafsToLoose )
    {
        u32 indexes[128];
        u32 leafsToLoose = ( u32 ) ( percentageOfLeafsToLoose * plant->leafCount );
        Assert( leafsToLoose <= ArrayCount( indexes ) );
        
        RandomSequence looseSeq = Seed( leafsToLoose * plant->leafCount * 1023 );
        for( u32 leafIndex = 0; leafIndex < leafsToLoose; ++leafIndex )
        {
            indexes[leafIndex] = RandomChoice( &looseSeq, plant->leafCount );
        }
        
        u32 leafTotal = 0;
        u32 loosed = LooseLeafs( plant->root, leafsToLoose, indexes, &leafTotal );
        Assert( leafTotal == plant->leafCount );
        
        r32 loosedPercentage = SafeRatio1( ( r32 ) loosed, ( r32 ) plant->leafCount );
        plant->loosedLeafPercentage += loosedPercentage;
        Assert(plant->loosedLeafPercentage <= 1.0f);
    }
    
    r32 timeStepTime = 1.0f;
    
    u32 stepDoneServer = ( u32 ) ( age / timeStepTime );
    u32 stepDoneHere = plant->stepDone; 
    Assert(stepDoneServer >= stepDoneHere);
    
    u32 stepDelta = stepDoneServer - stepDoneHere;
    
    for( u32 stepIndex = 0; stepIndex < stepDelta; ++stepIndex )
    {
        AdvancePlantLife(worldMode, params, plant->root, &plant->sequence, &plant->leafCount );
        AdvancePlantLife(worldMode, params, plant->futureRoot, &plant->futureSequence, &plant->futureLeafCount );
    }
    
    plant->stepDone = stepDoneServer;
    r32 lerp = Clamp01MapToRange( 0.0f, fmodf( age, timeStepTime ), timeStepTime );
    
    RandomSequence seq = Seed( ( i32 ) id );
    Assert( params->plantCount > 0 );
    
    PlantRenderingParams renderingParams;
    renderingParams.lerp = lerp;
    renderingParams.defaultLeafSize = plant->defaultLeafSize;
    
    renderingParams.plantHealth = 1.0f;//Clamp01( Sin( runningPlantHealth ) );
    renderingParams.windDirection = V3( 1.0f, 0.0f, 0.0f );;
    renderingParams.windStrength = 1.0f;
    
    renderingParams.modulationPercentageWithFocusColor = entityC->modulationWithFocusColor;
    
    for( u32 plantIndex = 0; plantIndex < params->plantCount; ++plantIndex )
    {
        renderingParams.plantIndex = plantIndex;
        r32 offsetX = RandomRangeFloat( &seq, params->minOffset, params->maxOffset );
        r32 offsetY = RandomRangeFloat( &seq, params->minOffset, params->maxOffset );
        Vec3 plantP = entityC->P + V3( offsetX, offsetY, 0 );
        RenderPlant( group, tileInfo.lightIndexes, params, plant->root, plant->futureRoot, plantP, Identity(), R32_MAX,  &renderingParams, timeToUpdate );
    }
}
