inline PlantSegment* GetFreePlantSegment(GameModeWorld* worldMode)
{
    PlantSegment* segment;
    FREELIST_ALLOC(segment, worldMode->firstFreePlantSegment, PushStruct(&worldMode->entityPool, PlantSegment));
    return segment;
}

inline PlantStem* GetFreePlantStem(GameModeWorld* worldMode)
{
    PlantStem* stem;
    FREELIST_ALLOC(stem, worldMode->firstFreePlantStem, PushStruct(&worldMode->entityPool, PlantStem));
    return stem;
}

inline r32 ShapeRatio(PlantShape shape, r32 ratio)
{
    ratio = Clamp01(ratio);
    r32 result;
    
    switch(shape)
    {
        case PlantShape_Conical:
        {
            result = 0.2f + 0.8f * ratio;
        } break;
        
        case PlantShape_Spherical:
        {
            result = 0.2f + 0.8f * Sin(PI32 * ratio);
        } break;
        
        case PlantShape_Hemispherical:
        {
            result = 0.2f + 0.8f * Sin(0.5f * PI32 * ratio);
        } break;
        
        case PlantShape_Cylindrical:
        {
            result = 1.0f;
        } break;
        
        case PlantShape_TaperedCylindrical:
        {
            result = 0.5f + 0.5f * ratio;
        } break;
        
        case PlantShape_Flame:
        {
            if(ratio <= 0.7f)
            {
                result = ratio / 0.7f;
            }
            else
            {
                result = (1.0f - ratio) / 0.3f;
            }
        } break;
        
        case PlantShape_InverseConical:
        {
            result = 1.0f - 0.8f * ratio;
        } break;
        
        case PlantShape_TendFlame:
        {
            if(ratio <= 0.7f)
            {
                result = 0.5f + 0.5f * ratio / 0.7f;
            }
            else
            {
                result = 0.5f + 0.5f * (1.0f - ratio) / 0.3f;
            }
        } break;
        
        default:
        {
            result = ratio;
        }
    }
    
    return result;
}

inline r32 GetRadiousAtZ(r32 taper, r32 flare, r32 stemLength, r32 stemRadious, r32 Z)
{
    r32 result;
    r32 unitTaper;
    
    if(taper >= 0 && taper < 1)
    {
        unitTaper = taper;
    }
    else if(taper >= 1 && taper < 2)
    {
        unitTaper = 2.0f - taper;
    }
    else
    {
        unitTaper = 0;
    }
    
    r32 taperZ = stemRadious * (1.0f - (unitTaper * Z));
    
    if(taper >= 0 && taper < 1)
    {
        result = taperZ;
    }
    else
    {
        r32 Z2 = (1.0f - Z) * stemLength;
        
        r32 depth = taper - 2.0f;
        if(taper < 2 || (Z2 < taperZ))
        {
            depth = 1;
        }
        
        r32 Z3 = Z2;
        if(taper >= 2)
        {
            Z3 = Abs(Z2 - 2.0f * taperZ * Floor(Z2 / (2.0f * taperZ) + 0.5f));
        }
        
        
        if(taper < 2 && Z3 >= taperZ)
        {
            result = taperZ;
        }
        else
        {
            result = (1.0f - depth) * taperZ + depth * SquareRoot(Square(taperZ) - Square(Z3 - taperZ));
        }
    }
    
    if(flare)
    {
        r32 flareExp = Max(0.0f, 1.0f - 8.0f * Z);
        r32 flareZ = flare * (Pow(100.0f, flareExp) - 1.0f) / 101.0f;
        result *= flareZ;
    }
    
    return result;
}

inline Vec4 GetColorAtZ(PlantDefinition* definition, PlantLevelParams* params, r32 stemLife, r32 stemNoise, r32 Z)
{
    Vec4 result = BiLerp(params->baseYoungColor, params->topYoungColor, params->baseOldColor, params->topOldColor, Z, stemLife);
    result += stemNoise * definition->trunkColorV;
    
    if(definition->lobes)
    {
        result += Sin(definition->lobes * Z * PI32) * definition->trunkColorV;
    }
    
    return result;
}

inline void UpdatePlantStem(GameModeWorld* worldMode, ClientPlant* plant, PlantDefinition* definition, PlantStem* stem, u8 recursiveLevel, m4x4 originOrientation, r32 startingZ, r32 timeToUpdate)
{
    PlantLevelParams* levelParams = definition->levelParams + recursiveLevel;
    PlantLevelParams* nextLevelParams = levelParams;
    if(recursiveLevel < (definition->maxLevels - 1))
    {
        nextLevelParams = definition->levelParams + (recursiveLevel + 1);
    }
    
    m4x4 segmentOrientation = originOrientation * ZRotation(DegToRad(stem->parentAngleZ)) * YRotation(DegToRad(stem->parentAngleY)) ;
    
    
    r32 segmentLength = stem->totalLength / levelParams->curveRes;
    r32 segmentUnitZ = 1.0f / levelParams->curveRes;
    
    r32 segmentBaseZ = startingZ;
    r32 segmentTopZ = startingZ + segmentUnitZ;
    
    r32 stemActualLength = 0;
    for(PlantSegment* segment = stem->root; segment;)
    {
        PlantSegment* next = segment->next;
        m4x4 topOrientation = segmentOrientation * YRotation(segment->angleY);
        
        for(PlantStem* child = segment->childs; child; child = child->next)
        {
            UpdatePlantStem(worldMode, plant, definition, child, recursiveLevel + 1, topOrientation, 0, timeToUpdate);
        }
        
        for(PlantStem* clone = segment->clones; clone; clone = clone->next)
        {
            UpdatePlantStem(worldMode, plant, definition, clone, recursiveLevel, topOrientation, segmentTopZ, timeToUpdate);
        }
        
		r32 oldZ = segmentBaseZ + segment->lengthCoeff * segmentUnitZ;
        r32 oldLenghtCoeff = segment->lengthCoeff;
        
        segment->lengthCoeff += levelParams->lengthIncreaseSpeed * timeToUpdate;
        segment->radiousCoeff += levelParams->radiousIncreaseSpeed * timeToUpdate;
        
        segment->lengthCoeff = Min(segment->lengthCoeff, 1.0f);
        segment->radiousCoeff = Min(segment->radiousCoeff, 1.0f);
        
		r32 newZ = segmentBaseZ + segment->lengthCoeff * segmentUnitZ;
        
        stemActualLength += segmentLength * segment->lengthCoeff;
        if(recursiveLevel < definition->maxLevels)
        {
            if(oldZ <= stem->nextChildZ && newZ > stem->nextChildZ)
            {
                Assert(!segment->next && !segment->clones);
                r32 childUnitZ = 1.0f / stem->numberOfChilds;
                
                while(stem->nextChildZ <= newZ)
                {
                    r32 parentStemZ = stem->nextChildZ;
                    r32 parentSegmentZ = (parentStemZ - segmentBaseZ) / segmentUnitZ;
                    Assert(parentStemZ <= 1.0f);
                    
                    r32 offsetChild = parentStemZ * stem->totalLength;
                    r32 trunkLength = plant->trunk.totalLength;
                    
                    
                    r32 angleY;
                    if(levelParams->downAngleV >= 0)
                    {
                        angleY = levelParams->downAngle + RandomBil(&plant->sequence) * levelParams->downAngleV;
                    }
                    else
                    {
                        angleY = levelParams->downAngle + RandomBil(&plant->sequence) * (levelParams->downAngleV * (1.0f - 2.0f * ShapeRatio(PlantShape_Conical, (stem->totalLength - offsetChild) / (stem->totalLength - plant->lengthBase))));
                    }
                    
                    r32 rotation;
                    if(levelParams->rotate >= 0)
                    {
                        rotation = levelParams->rotate + RandomBil(&plant->sequence) * levelParams->rotateV;
                    }
                    else
                    {
                        rotation = 180.0f + (-levelParams->rotate) + RandomBil(&plant->sequence) * levelParams->rotateV;
                        
                    }
                    stem->childsCurrentAngle += rotation;
                    r32 angleZ = stem->childsCurrentAngle;
                    
                    
                    PlantStem* childStem = GetFreePlantStem(worldMode);
                    *childStem = {};
                    
                    childStem->root = GetFreePlantSegment(worldMode);
                    *childStem->root = {};
                    
                    childStem->segmentCount = 1;
                    
                    childStem->probabliltyToClone = 1.0f;
                    childStem->parentAngleY = angleY;
                    childStem->parentAngleZ = angleZ;
                    childStem->parentSegmentZ = parentSegmentZ;
                    childStem->parentStemZ = parentStemZ;
                    
                    r32 lengthChildMax = nextLevelParams->lengthCoeff + RandomBil(&plant->sequence) * nextLevelParams->lengthCoeffV;
                    if(recursiveLevel == 0)
                    {
                        r32 divisor = 1.0f;
                        if(trunkLength > plant->lengthBase)
                        {
                            divisor=  trunkLength - plant->lengthBase;
                        }
                        
                        childStem->totalLength = stem->totalLength * lengthChildMax * ShapeRatio(definition->shape, (trunkLength - offsetChild) / divisor);
                    }
                    else
                    {
                        childStem->totalLength = lengthChildMax * (stem->totalLength - 0.6f * offsetChild);
                    }
                    
                    childStem->baseRadious = stem->baseRadious * Pow(childStem->totalLength / stem->totalLength, definition->ratioPower);
                    
                    r32 radiousAtOriginatingPoint = GetRadiousAtZ(levelParams->taper, definition->flare, stem->totalLength, stem->baseRadious, childStem->parentStemZ);
                    childStem->baseRadious = Min(childStem->baseRadious, radiousAtOriginatingPoint);
                    
                    
                    
                    r32 lengthChild = childStem->totalLength;
                    r32 lengthParent = stem->totalLength;
                    if(recursiveLevel == 0)
                    {
                        childStem->numberOfChilds = RoundReal32ToU32(nextLevelParams->branches * (lengthChild / lengthParent) / lengthChildMax);
                    }
                    else
                    {
                        childStem->numberOfChilds = RoundReal32ToU32(nextLevelParams->branches * (1.0f - 0.5f * offsetChild / lengthParent));
                    }
                    
                    childStem->trunkNoise = RandomBil(&plant->sequence);
                    r32 childChildUnitZ = 1.0f / childStem->numberOfChilds;
                    childStem->nextChildZ = RandomRangeFloat(&plant->sequence, 0.5f * childChildUnitZ, childChildUnitZ - 0.01f);
                    
                    
                    childStem->childsCurrentAngle = 0;
                    childStem->additionalCurveBackAngle = 0;
                    
                    FREELIST_INSERT(childStem, segment->childs);
                    
                    
                    stem->nextChildZ += childUnitZ;
                }
            }
            
        }
		
        if(oldLenghtCoeff < 1.0f && (segment->lengthCoeff >= 1.0f))
        {
            if(stem->segmentCount < levelParams->curveRes)
            {
                u32 numberOfClones = 1;
                if(RandomUni(&plant->sequence) <= stem->probabliltyToClone)
                {
                    r32 splitCountReal = (stem->segmentCount == 1) ? levelParams->baseSplits : levelParams->segSplits;
                    
                    
                    r32 errorUse = RandomRangeFloat(&plant->sequence, 0.5f * plant->cloneAccumulatedError[recursiveLevel], plant->cloneAccumulatedError[recursiveLevel]);
                    
                    
                    numberOfClones = RoundReal32ToU32(splitCountReal + errorUse);
                    
                    plant->cloneAccumulatedError[recursiveLevel] -= errorUse;
                    plant->cloneAccumulatedError[recursiveLevel] += (levelParams->segSplits - (r32) numberOfClones);
                    
                    numberOfClones += 1;
                }
                
                u8 actualCurveRes = levelParams->curveRes - 1;
                if(numberOfClones == 1)
                {
                    Assert(!segment->clones);
                    Assert(!segment->next);
                    
                    PlantSegment* newSegment = GetFreePlantSegment(worldMode);
                    *newSegment = {};
                    
                    if(levelParams->curveBack == 0)
                    {
                        newSegment->angleY = levelParams->curve / actualCurveRes;
                    }
                    else
                    {
                        if(levelParams->curveBack > 0)
                        {
                            if(stem->segmentCount < actualCurveRes / 2)
                            {
                                newSegment->angleY = (levelParams->curve / (actualCurveRes / 2));
                            }
                            else
                            {
                                newSegment->angleY = (levelParams->curveBack / (actualCurveRes / 2));
                            }
                        }
                        else
                        {
                            InvalidCodePath;
                        }
                    }
                    
                    if(levelParams->curveV >= 0)
                    {
                        newSegment->angleY += RandomBil(&plant->sequence) * (levelParams->curveV / actualCurveRes);
                    }
                    else
                    {
                        InvalidCodePath;
                    }
                    newSegment->angleY += stem->additionalCurveBackAngle / actualCurveRes;
                    
                    segment->next = newSegment;
                    ++stem->segmentCount;
                }
                else
                {
                    r32 zComponent = (segmentOrientation * V3(0, 0, 1)).z;
                    r32 declination = ArcCos(zComponent);
                    
                    r32 runningAngle = 0;
                    r32 rotatingAngle = 0;
                    
#if 0                    
                    if(numberOfClones == 2)
                    {
                        rotatingAngle = 20.0f + 0.75f * (30.0f + Abs(declination - 90.0f)) * Square(RandomUni(&plant->sequence));
                        
                        if(RandomUni(&plant->sequence) < 0.5f)
                        {
                            rotatingAngle = -rotatingAngle;
                        }
                    }
                    else
#endif
                    
                    {
                        rotatingAngle = 360.0f / numberOfClones;
                    }
                    
                    for(u32 cloneIndex = 0; cloneIndex < numberOfClones; ++cloneIndex)
                    {
                        PlantStem* clonedStem = GetFreePlantStem(worldMode);
                        clonedStem->root = GetFreePlantSegment(worldMode);
                        *clonedStem->root = {};
                        
                        r32 angleY = (levelParams->splitAngle + RandomBil(&plant->sequence) * levelParams->splitAngleV) - declination;
                        
                        angleY += stem->additionalCurveBackAngle / actualCurveRes;
                        angleY = Max(angleY, 0);
                        clonedStem->segmentCount = stem->segmentCount + 1;
                        
                        clonedStem->probabliltyToClone = stem->probabliltyToClone * (levelParams->clonePercRatio + RandomBil(&plant->sequence) * levelParams->clonePercRatioV);
                        clonedStem->parentAngleY = angleY;
                        clonedStem->parentAngleZ = runningAngle;
                        clonedStem->parentStemZ = stem->parentStemZ;
                        clonedStem->parentSegmentZ = 1.0f;
                        
                        clonedStem->totalLength = stem->totalLength;
                        clonedStem->baseRadious = stem->baseRadious;
                        
                        clonedStem->trunkNoise = stem->trunkNoise;
                        clonedStem->nextChildZ = stem->nextChildZ;
                        clonedStem->numberOfChilds = stem->numberOfChilds;
                        clonedStem->childsCurrentAngle = stem->childsCurrentAngle;
                        clonedStem->additionalCurveBackAngle = -angleY;
                        
                        FREELIST_INSERT(clonedStem, segment->clones);
                        
                        runningAngle += rotatingAngle;
                    }
                }
            }
		}
        
        
        segmentOrientation = topOrientation;
        segment = next;
        
        segmentBaseZ = segmentTopZ;
        segmentTopZ += segmentUnitZ;
    }
    
    stem->lengthNormZ = Clamp01(stemActualLength / stem->totalLength);
    Assert(Normalized(stem->lengthNormZ));
    
    
    
    u32 leafToUpdate = levelParams->leafCount *  Ceil(Clamp01MapToRange(0, stem->lengthNormZ, levelParams->allLeafsAtStemLength));
    Assert(leafToUpdate < MAX_LEAFS_PER_STEM);
    for(u32 leafIndex = 0; leafIndex < leafToUpdate; ++leafIndex)
    {
        Leaf* leaf = stem->leafs + leafIndex;
        
        if(!leaf->dimCoeff)
        {
            leaf->renderingRandomization = RandomBil(&plant->sequence);
        }
        
        leaf->dimCoeff += definition->leafDimSpeed * timeToUpdate;
        leaf->offsetCoeff += definition->leafOffsetSpeed * timeToUpdate;
        
        leaf->dimCoeff = Min(leaf->dimCoeff, 1.0f);
        leaf->offsetCoeff = Min(leaf->offsetCoeff, 1.0f);
    }
}

inline void UpdatePlant(GameModeWorld* worldMode, PlantDefinition* definition, ClientPlant* plant, r32 timeToUpdate)
{
    PlantStem* trunk = &plant->trunk;
    if(!trunk->root)
    {
        plant->scale = definition->scale + RandomBil(&plant->sequence) * definition->scaleV;
        plant->lengthBase = definition->baseSize * plant->scale;
        
        PlantLevelParams* levelParams = definition->levelParams + 0;
        
        trunk->root = GetFreePlantSegment(worldMode);
        *trunk->root = {};
        
        trunk->segmentCount = 1;
        
        trunk->probabliltyToClone = 1.0f;
		trunk->parentAngleY = 0;
        trunk->parentAngleZ = 0;
        trunk->parentStemZ = 0;
        trunk->parentSegmentZ = 0;
        
        trunk->totalLength =  plant->scale * (definition->levelParams->lengthCoeff + RandomBil(&plant->sequence) * levelParams->lengthCoeffV);
        trunk->baseRadious = trunk->totalLength * definition->ratio * (definition->scale_0 + RandomBil(&plant->sequence) * definition->scaleV_0);
        
        trunk->numberOfChilds = RoundReal32ToU32(levelParams->branches);
        
        trunk->trunkNoise = RandomBil(&plant->sequence);
        r32 childUnitZ = 1.0f /trunk->numberOfChilds;
        trunk->nextChildZ = RandomRangeFloat(&plant->sequence, 0.5f * childUnitZ, childUnitZ - 0.01f);
        trunk->childsCurrentAngle = 0;
        
        trunk->additionalCurveBackAngle = 0;
    }
    
    UpdatePlantStem(worldMode, plant, definition, trunk, 0, Identity(), 0, timeToUpdate);
}


internal void RenderStem(RenderGroup* group, Vec4 lightIndexes, ClientPlant* plant, PlantDefinition* definition, PlantStem* stem, Vec3 stemP, u8 recursiveLevel, m4x4 originOrientation, r32 startingZ)
{
    m4x4 baseOrientation = originOrientation * ZRotation(DegToRad(stem->parentAngleZ)) * YRotation(DegToRad(stem->parentAngleY));
    
    Vec3 segmentBaseP = stemP;
    
    PlantLevelParams* levelParams = definition->levelParams + recursiveLevel;
    
    r32 segmentLength = stem->totalLength / levelParams->curveRes;
    r32 segmentUnitZ = 1.0f / levelParams->curveRes;
    
    r32 baseZ = startingZ;
    r32 topZ = baseZ + segmentUnitZ;
    
    Vec3 topP = stemP;
    
    r32 stemLifeNorm = stem->lengthNormZ;
    for(PlantSegment* segment = stem->root; segment; segment = segment->next)
    {
        r32 baseRadious = GetRadiousAtZ(levelParams->taper, definition->flare, stem->totalLength, stem->baseRadious, baseZ) * segment->radiousCoeff;
        r32 topRadious = GetRadiousAtZ(levelParams->taper, definition->flare, stem->totalLength, stem->baseRadious, topZ) * segment->radiousCoeff;
        r32 length = segmentLength * segment->lengthCoeff;
        Vec4 baseColor = GetColorAtZ(definition, levelParams, stemLifeNorm, stem->trunkNoise, baseZ);
        Vec4 topColor = GetColorAtZ(definition, levelParams, stemLifeNorm, stem->trunkNoise, topZ);
        
        Vec3 bottomXAxis = GetColumn(baseOrientation, 0);
        Vec3 bottomYAxis = GetColumn(baseOrientation, 1);
        Vec3 bottomZAxis = GetColumn(baseOrientation, 2);
        
        m4x4 topOrientation = baseOrientation * YRotation(DegToRad(segment->angleY));
        Vec3 topXAxis = GetColumn(topOrientation, 0);
        Vec3 topYAxis = GetColumn(topOrientation, 1);
        Vec3 topZAxis = GetColumn(topOrientation, 2);
        
        topP = segmentBaseP + length * topZAxis;
        PushTrunkatedPyramid(group, plant->trunkBitmap, segmentBaseP, topP, 4, bottomXAxis, bottomYAxis, bottomZAxis, topXAxis, topYAxis, topZAxis, baseRadious, topRadious, length, baseColor, topColor, lightIndexes, 0);
        
        for(PlantStem* child = segment->childs; child; child = child->next)
        {
            Vec3 childP = segmentBaseP + child->parentSegmentZ * segmentLength * topZAxis;
            RenderStem(group, lightIndexes, plant, definition, child, childP, recursiveLevel + 1, topOrientation, 0);
        }
        
        for(PlantStem* clone = segment->clones; clone; clone = clone->next)
        {
			Vec3 cloneP = topP;
            RenderStem(group, lightIndexes, plant, definition, clone, cloneP, recursiveLevel, topOrientation, topZ);
        }
        
        segmentBaseP = topP;
        baseOrientation = topOrientation;
        
        baseZ = topZ;
        topZ += segmentUnitZ;
    }
    
    for(u32 leafIndex = 0; leafIndex < levelParams->leafCount; ++leafIndex)
    {
        Leaf* leaf = stem->leafs + leafIndex;
        
        Vec2 leafScale = definition->leafScale + leaf->renderingRandomization * definition->leafScaleV;
        Vec2 scale = leaf->dimCoeff * leafScale;
        
        Vec3 leafP = topP + V3(0, 0, 0.1f * leafIndex) + leaf->offsetCoeff * leaf->renderingRandomization * definition->leafOffsetV;
        Vec4 leafColor = definition->leafColor + leaf->renderingRandomization * definition->leafColorV;
        leafColor = Clamp01(leafColor);
        
        
        ObjectTransform leafTransform = UprightTransform();
        leafTransform.angle = leaf->renderingRandomization * definition->leafAngleV;
        
        PushBitmap(group, leafTransform, plant->leafBitmap, leafP, 0, scale, leafColor, lightIndexes);
    }
}




internal void UpdateAndRenderPlant(GameModeWorld* worldMode, RenderGroup* group, Vec4 lightIndexes, PlantDefinition* definition, ClientPlant* plant, Vec3 plantP, r32 timeToUpdate)
{
    timeToUpdate *= worldMode->UI->plantGrowingCoeff;
    r32 targetUpdateTime = 1.0f;
    
    while((plant->serverAge - plant->age) >= targetUpdateTime)
    {
        UpdatePlant(worldMode, definition, plant, targetUpdateTime);
        plant->age += targetUpdateTime;
    }
#if 0
    for( u32 plantIndex = 0; plantIndex < params->plantCount; ++plantIndex )
    {
        r32 offsetX = RandomRangeFloat( &seq, params->minOffset, params->maxOffset );
        r32 offsetY = RandomRangeFloat( &seq, params->minOffset, params->maxOffset );
        Vec3 plantP = entityC->P + V3( offsetX, offsetY, 0 );
    }
#endif
    
    RenderStem(group, lightIndexes, plant, definition, &plant->trunk, plantP, 0, Identity(), 0);
}
