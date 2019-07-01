inline PlantSegment* GetFreePlantSegment(GameModeWorld* worldMode)
{
    PlantSegment* segment;
    
    BeginTicketMutex(&worldMode->plantMutex);
    FREELIST_ALLOC(segment, worldMode->firstFreePlantSegment, PushStruct(worldMode->persistentPool, PlantSegment));
    
    EndTicketMutex(&worldMode->plantMutex);
    return segment;
}

inline PlantStem* GetFreePlantStem(GameModeWorld* worldMode)
{
    PlantStem* stem;
    
    BeginTicketMutex(&worldMode->plantMutex);
    FREELIST_ALLOC(stem, worldMode->firstFreePlantStem, PushStruct(worldMode->persistentPool, PlantStem));
    EndTicketMutex(&worldMode->plantMutex);
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

inline r32 GetRadiousAtZ(r32 taper, r32 flare, r32 lobeDepth, r32 lobes, r32 stemLength, r32 stemRadious, r32 Z)
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
        r32 flareZ = flare * (Pow(100.0f, flareExp) - 1.0f) / 100.0f + 1.0f;
        result *= flareZ;
    }
    
    
    if(lobeDepth)
    {
        r32 lobeZ = 1.0f + lobeDepth * Sin(lobes * Z * TAU32);
        result *= lobeZ;
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
    
    result = Clamp01(result);
    return result;
}

inline void GetWindAnglesAtZ(r32 time, r32 curveRes, r32 stemBaseRadious, r32 stemLength, r32 radiousZ, r32 Z, r32* angleX, r32* angleY)
{
    r32 windSpeed = 10.0f;
    r32 windGust = 5.0f;
    
    r32 swayOffsetX = 10.0f;
    r32 swayOffsetY = 10.0f;
    
    r32 a0 = 4.0f * stemLength * (1.0f - Z) / radiousZ;
    r32 a1 = windSpeed / 50.0f * a0;
    r32 a2 = windGust / 50.0f * a0 + a1 / 2.0f;
    
    r32 bx = swayOffsetX + stemBaseRadious / stemLength * time / 15.0f;
    r32 by = swayOffsetY + stemBaseRadious / stemLength * time / 15.0f;
    
    *angleX = (a1 * Sin(bx) + a2 * Sin(0.7f * bx)) / curveRes;
    *angleY = (a1 * Sin(by) + a2 * Sin(0.7f * by)) / curveRes;
}


inline r32 GetStemMinUpdateTime(PlantDefinition* definition, PlantStem* stem, u8 recursiveLevel)
{
    r32 result = R32_MAX;
    
    PlantLevelParams* levelParams = definition->levelParams + recursiveLevel;
    for(PlantSegment* segment = stem->root; segment; segment = segment->next)
    {
        if(segment->lengthCoeff < 1.0f)
        {
            r32 remainingDelta = 1.0f - segment->lengthCoeff;
            r32 timeToArriveAt1 = remainingDelta / levelParams->lengthIncreaseSpeed;
            result = Min(result, timeToArriveAt1);
        }
        
        for(PlantStem* child = segment->childs; child; child = child->next)
        {
            result = Min(result, GetStemMinUpdateTime(definition, child, recursiveLevel + 1));
        }
        
        for(PlantStem* clone = segment->clones; clone; clone = clone->next)
        {
            result = Min(result, GetStemMinUpdateTime(definition, clone, recursiveLevel));
        }
    }
    
    return result;
}

inline void InitLFF(PlantLFFParams* params, LeafFlowerFruit* lff, r32 segmentWindRandomization, RandomSequence* seq)
{
    lff->initialized = true;
    lff->renderingRandomization = RandomBil(seq);
    lff->densityRandomization = RandomBil(seq);
    lff->colorRandomization = RandomBil(seq);
    r32 lffRandomization = RandomBil(seq);
    lff->windRandomization = Lerp(segmentWindRandomization, params->windDirectionV, lffRandomization);
}

inline void UpdateLFF(PlantLFFParams* params, LeafFlowerFruit* lff, r32 timeToUpdate)
{
    lff->dimCoeff += params->dimSpeed * timeToUpdate;
    lff->offsetCoeff += params->offsetSpeed * timeToUpdate;
    lff->dimCoeff = Min(lff->dimCoeff, 1.0f);
    lff->offsetCoeff = Min(lff->offsetCoeff, 1.0f);
}z

inline void InitAndUpdateLFF(PlantStem* stem, PlantLFFParams* params, LeafFlowerFruit* lffs, u32 levelCount, r32 allAtStemLength, RandomSequence* seq, r32 timeToUpdate)
{
    u32 lffToUpdate = levelCount *  Ceil(Clamp01MapToRange(0, stem->lengthNormZ, allAtStemLength));
    Assert(lffToUpdate < MAX_LFF_PER_STEM);
    r32 segmentWindRandomization = 0;
    for(u32 lffIndex = 0; lffIndex < levelCount; ++lffIndex)
    {
        LeafFlowerFruit* lff = lffs + lffIndex;
        if(!lff->initialized)
        {
            if(lffIndex == 0)
            {
                segmentWindRandomization = RandomBil(seq);
            }
            InitLFF(params, lff, segmentWindRandomization, seq);
        }
    }
    
    for(u32 lffIndex = 0; lffIndex < lffToUpdate; ++lffIndex)
    {
        LeafFlowerFruit* lff = lffs + lffIndex;
        UpdateLFF(params, lff, timeToUpdate);
    }
}


inline void UpdatePlantStem(GameModeWorld* worldMode, ClientPlant* plant, PlantDefinition* definition, PlantStem* trunk, PlantStem* stem, u8 recursiveLevel, m4x4 originOrientation, r32 startingZ, r32 timeToUpdate)
{
    PlantLevelParams* levelParams = definition->levelParams + recursiveLevel;
    PlantLevelParams* nextLevelParams = levelParams;
    if(recursiveLevel < (definition->maxLevels - 1))
    {
        nextLevelParams = definition->levelParams + (recursiveLevel + 1);
    }
    
    m4x4 segmentOrientation = originOrientation * stem->orientation;
    
    
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
            UpdatePlantStem(worldMode, plant, definition, trunk, child, recursiveLevel + 1, topOrientation, 0, timeToUpdate);
        }
        
        for(PlantStem* clone = segment->clones; clone; clone = clone->next)
        {
            UpdatePlantStem(worldMode, plant, definition, trunk, clone, recursiveLevel, topOrientation, segmentTopZ, timeToUpdate);
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
                    r32 trunkLength = trunk->totalLength;
                    
                    
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
                    
                    childStem->orientation = ZRotation(DegToRad(angleZ)) * YRotation(DegToRad(angleY));
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
                    
                    r32 radiousAtOriginatingPoint = GetRadiousAtZ(levelParams->taper, definition->flare, definition->lobeDepth, definition->lobes, stem->totalLength, stem->baseRadious, childStem->parentStemZ);
                    radiousAtOriginatingPoint *= levelParams->radiousMod;
                    childStem->maxRadious = radiousAtOriginatingPoint;
                    
                    
                    
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
                    
                    for(u32 leafIndex = 0; leafIndex < ArrayCount(childStem->leafs); ++leafIndex)
                    {
                        childStem->leafs[leafIndex] = {};
                    }
                    
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
                    Assert(numberOfClones < 0xff);
                    
                    plant->cloneAccumulatedError[recursiveLevel] -= errorUse;
                    plant->cloneAccumulatedError[recursiveLevel] += (levelParams->segSplits - (r32) numberOfClones);
                    plant->cloneAccumulatedError[recursiveLevel] = Max(0, plant->cloneAccumulatedError[recursiveLevel]);
                    
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
                    
                    r32 declination = ArcCos((segmentOrientation * V3(0, 0, 1)).z);
                    r32 orientation = ArcCos((segmentOrientation * V3(0, 1, 0)).z);
                    r32 curveupsegment = RadToDeg(definition->attractionUp * declination * Cos(orientation) / actualCurveRes);
                    
                    
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
                        
                        clonedStem->orientation = ZRotation(DegToRad(runningAngle)) * YRotation(DegToRad(angleY));
                        clonedStem->parentStemZ = stem->parentStemZ;
                        clonedStem->parentSegmentZ = 1.0f;
                        
                        clonedStem->totalLength = stem->totalLength;
                        clonedStem->baseRadious = stem->baseRadious;
                        clonedStem->maxRadious = stem->maxRadious;
                        
                        clonedStem->trunkNoise = stem->trunkNoise;
                        clonedStem->nextChildZ = stem->nextChildZ;
                        clonedStem->numberOfChilds = stem->numberOfChilds;
                        clonedStem->childsCurrentAngle = stem->childsCurrentAngle;
                        clonedStem->additionalCurveBackAngle = -angleY;
                        
                        for(u32 leafIndex = 0; leafIndex < ArrayCount(clonedStem->leafs); ++leafIndex)
                        {
                            clonedStem->leafs[leafIndex] = {};
                        }
                        
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
    
    
    InitAndUpdateLFF(stem, &definition->leafParams, stem->leafs, levelParams->leafCount, levelParams->allLeafsAtStemLength, &worldMode->leafFlowerFruitSequence, timeToUpdate);
    InitAndUpdateLFF(stem, &definition->flowerParams, stem->flowers, levelParams->flowerCount, levelParams->allFlowersAtStemLength, &worldMode->leafFlowerFruitSequence, timeToUpdate);
    InitAndUpdateLFF(stem, &definition->fruitParams, stem->fruits, levelParams->fruitCount, levelParams->allFruitsAtStemLength, &worldMode->leafFlowerFruitSequence, timeToUpdate);
}

inline void UpdatePlant(GameModeWorld* worldMode, PlantDefinition* definition, ClientPlant* plant, r32 timeToUpdate)
{
    for(PlantStem* trunk = plant->plant.firstTrunk; trunk; trunk = trunk->next)
    {
        UpdatePlantStem(worldMode, plant, definition, trunk, trunk, 0, Identity(), 0, timeToUpdate);
    }
}

struct PlantRenderingParams
{
    Lights lights;
    r32 modulationWithFocusColor;
    WorldSeason season;
    r32 lerpWithFollowingSeason;
};

inline void RenderLFF(RenderGroup* group, ClientPlant* plant, Vec3 P, PlantLFFParams* params,LeafFlowerFruit* lff, BitmapId BID, PlantRenderingParams renderingParams, r32 windTime)
{
    PlantLFFSeasonParams* season = GetSeason(params, renderingParams.season);
    PlantLFFSeasonParams* followingSeason = GetFollowingSeason(params, renderingParams.season);
    
    r32 lerp = renderingParams.lerpWithFollowingSeason;
    
    Vec2 seasonScale = Lerp(season->scale, lerp, followingSeason->scale);
    Vec2 seasonScaleV = Lerp(season->scaleV, lerp, followingSeason->scaleV);
    
    Vec2 scale = seasonScale + lff->renderingRandomization * seasonScaleV;
    scale *= lff->dimCoeff;
    
    Vec4 seasonAliveColor = Lerp(season->aliveColor, lerp, followingSeason->aliveColor);
    Vec4 seasonDeadColor = Lerp(season->deadColor, lerp, followingSeason->deadColor);
    Vec4 seasonColorV = Lerp(season->colorV, lerp, followingSeason->colorV);
    
    Vec4 referenceColor = Lerp(seasonDeadColor, plant->life, seasonAliveColor);
    Vec4 color = referenceColor + lff->colorRandomization * seasonColorV;
    color = Clamp01(color);
    
    
    ObjectTransform lffTransform = UprightTransform();
    
    r32 lffAngle = lff->renderingRandomization * Abs(params->angleV);
    if(params->angleV < 0 && Cos(DegToRad(lffAngle)) < 0)
    {
        lffTransform.flipOnYAxis = true;
    }
    
    
    lffAngle += Sin(windTime + TAU32 * lff->windRandomization) * params->windAngleV;
    
    lffTransform.angle = lffAngle;
    lffTransform.modulationPercentage = renderingParams.modulationWithFocusColor;
    
    PushBitmap(group, lffTransform, BID, P, 0, scale, color, renderingParams.lights);
}

internal void RenderStem(RenderGroup* group, PlantRenderingParams renderingParams, r32 windTime, ClientPlant* plant, PlantDefinition* definition, PlantStem* stem, Vec3 stemP, u8 recursiveLevel, m4x4 originOrientation, r32 startingZ)
{
    m4x4 baseOrientation = originOrientation * stem->orientation;
    
    Vec3 segmentBaseP = stemP;
    
    PlantLevelParams* levelParams = definition->levelParams + recursiveLevel;
    
    r32 segmentLength = stem->totalLength / levelParams->curveRes;
    r32 segmentUnitZ = 1.0f / levelParams->curveRes;
    
    r32 baseZ = startingZ;
    r32 topZ = baseZ + segmentUnitZ;
    
    Vec3 topP = stemP;
    
    r32 stemLifeNorm = stem->lengthNormZ;
    
    b32 drawBase = false;
    b32 drawTop = false;
    
    for(PlantSegment* segment = stem->root; segment; segment = segment->next)
    {
        if(!segment->next)
        {
            drawTop = true;
        }
        
        r32 baseRadious = GetRadiousAtZ(levelParams->taper, definition->flare, definition->lobeDepth, definition->lobes, stem->totalLength, stem->baseRadious, baseZ) * segment->radiousCoeff;
        r32 topRadious = GetRadiousAtZ(levelParams->taper, definition->flare, definition->lobeDepth, definition->lobes, stem->totalLength, stem->baseRadious, topZ) * segment->radiousCoeff;
        
        baseRadious = Min(baseRadious, stem->maxRadious);
        topRadious = Min(topRadious, stem->maxRadious);
        
        r32 length = segmentLength * segment->lengthCoeff;
        Vec4 baseColor = GetColorAtZ(definition, levelParams, stemLifeNorm, stem->trunkNoise, baseZ);
        Vec4 topColor = GetColorAtZ(definition, levelParams, stemLifeNorm, stem->trunkNoise, topZ);
        
        Vec3 bottomXAxis = GetColumn(baseOrientation, 0);
        Vec3 bottomYAxis = GetColumn(baseOrientation, 1);
        Vec3 bottomZAxis = GetColumn(baseOrientation, 2);
        
        r32 windAngleX = 0;
        r32 windAngleY = 0;
        //GetWindAnglesAtZ(windTime, levelParams->curveRes, stem->baseRadious, stem->totalLength, topRadious, topZ, &windAngleX, &windAngleY);
        
        m4x4 topOrientation = baseOrientation * XRotation(DegToRad(windAngleX)) * YRotation(DegToRad(segment->angleY + windAngleY));
        
        Vec3 topXAxis = GetColumn(topOrientation, 0);
        Vec3 topYAxis = GetColumn(topOrientation, 1);
        Vec3 topZAxis = GetColumn(topOrientation, 2);
        
        topP = segmentBaseP + length * topZAxis;
        
        if(IsValid(plant->trunkBitmap))
        {
            PushTrunkatedPyramid(group, plant->trunkBitmap, segmentBaseP, topP, 4, bottomXAxis, bottomYAxis, bottomZAxis, topXAxis, topYAxis, topZAxis, baseRadious, topRadious, length, baseColor, topColor, renderingParams.lights, renderingParams.modulationWithFocusColor, drawBase, drawTop);
        }
        
        for(PlantStem* child = segment->childs; child; child = child->next)
        {
            Vec3 childP = segmentBaseP + child->parentSegmentZ * segmentLength * topZAxis;
            RenderStem(group, renderingParams, windTime, plant, definition, child, childP, recursiveLevel + 1, topOrientation, 0);
        }
        
        for(PlantStem* clone = segment->clones; clone; clone = clone->next)
        {
            Vec3 cloneP = topP;
            RenderStem(group, renderingParams, windTime, plant, definition, clone, cloneP, recursiveLevel, topOrientation, topZ);
        }
        
        segmentBaseP = topP;
        baseOrientation = topOrientation;
        
        baseZ = topZ;
        topZ += segmentUnitZ;
    }
    
    
    if(IsValid(plant->leafBitmap))
    {
        for(u32 leafIndex = 0; leafIndex < levelParams->leafCount; ++leafIndex)
        {
            LeafFlowerFruit* lff = stem->leafs + leafIndex;
            r32 leafTargetDensity = BilateralToUnilateral(lff->densityRandomization);
            if(plant->leafDensity > leafTargetDensity)
            {
                Vec3 P = topP + V3(0, 0, 0.005f * leafIndex) + lff->offsetCoeff * lff->renderingRandomization * definition->leafParams.offsetV;
                RenderLFF(group, plant, P, &definition->leafParams, lff, plant->leafBitmap, renderingParams, windTime);
            }
        }
    }
    
    if(IsValid(plant->flowerBitmap))
    {
        for(u32 flowerIndex = 0; flowerIndex < levelParams->flowerCount; ++flowerIndex)
        {
            LeafFlowerFruit* lff = stem->flowers + flowerIndex;
            r32 flowerTargetDensity = BilateralToUnilateral(lff->renderingRandomization);
            if(plant->flowerDensity > flowerTargetDensity)
            {
                Vec3 P = topP + V3(0, 0, 0.005f * flowerIndex) + lff->offsetCoeff * lff->renderingRandomization * definition->flowerParams.offsetV;
                RenderLFF(group, plant, P, &definition->flowerParams, lff, plant->flowerBitmap, renderingParams, windTime);
            }
        }
    }
    
    
    if(IsValid(plant->fruitBitmap))
    {
        for(u32 fruitIndex = 0; fruitIndex < levelParams->fruitCount; ++fruitIndex)
        {
            LeafFlowerFruit* lff = stem->fruits + fruitIndex;
            r32 fruitTargetDensity = BilateralToUnilateral(lff->renderingRandomization);
            if(plant->fruitDensity > fruitTargetDensity)
            {
                Vec3 P = topP + V3(0, 0, 0.005f * fruitIndex) + lff->offsetCoeff * lff->renderingRandomization * definition->fruitParams.offsetV;
                RenderLFF(group, plant, P, &definition->fruitParams, lff, plant->fruitBitmap, renderingParams, windTime);
            }
        }
    }
    
}


internal void SyncPlantWithServer(GameModeWorld* worldMode, PlantDefinition* definition, ClientPlant* plant)
{
    r32 targetUpdateTime = 1.0f;
    r32 longUpdateTime = 10.0f;
    b32 granularUpdate = true;
    
    while(true)
    {
        r32 delta = plant->serverAge - plant->age;
        if(delta >= longUpdateTime)
        {
            r32 plantTimer = R32_MAX;
            for(PlantStem* trunk = plant->plant.firstTrunk; trunk; trunk = trunk->next)
            {
                plantTimer = Min(plantTimer, GetStemMinUpdateTime(definition, trunk, 0));
            }
            
            if(plantTimer < R32_MAX)
            {
                plantTimer = Min(plantTimer, delta);
                UpdatePlant(worldMode, definition, plant, plantTimer);
                plant->age += plantTimer;
            }
            else
            {
                granularUpdate = false;
                break;
            }
        }
        else
        {
            break;
        }
        
        
    }
    
    if(granularUpdate)
    {
        while((plant->serverAge - plant->age) >= targetUpdateTime)
        {
            UpdatePlant(worldMode, definition, plant, targetUpdateTime);
            plant->age += targetUpdateTime;
        }
    }
}

struct SyncPlantWork
{
    GameModeWorld* worldMode;
    PlantDefinition* definition;
    ClientPlant* plant;
    TaskWithMemory* task;
};


PLATFORM_WORK_CALLBACK(SyncPlantCallback)
{
    SyncPlantWork* work = (SyncPlantWork*) param;
    
    SyncPlantWithServer(work->worldMode, work->definition, work->plant);
    
    CompletePastWritesBeforeFutureWrites;
    work->plant->canRender = true;
    EndTaskWithMemory(work->task);
}


internal void UpdateAndRenderPlant(GameModeWorld* worldMode, RenderGroup* group, PlantRenderingParams renderingParams, PlantDefinition* definition, ClientPlant* plant, Vec3 plantP)
{
    if(!plant->plant.trunkCount)
    {
        GameState* gameState = worldMode->gameState;
        TaskWithMemory* task = BeginTaskWithMemory(gameState->tasks, ArrayCount(gameState->tasks), false);
        if(task)
        {
            for(u32 levelIndex = 0; levelIndex < MAX_LEVELS; ++levelIndex)
            {
                plant->cloneAccumulatedError[levelIndex] = RandomUni(&plant->sequence);
            }
            
            plant->plant.plantCount = Max(1, definition->plantCount + RoundReal32ToU32(RandomBil(&plant->sequence) *definition->plantCountV));
            for(u32 plantIndex = 0; plantIndex < plant->plant.plantCount; ++plantIndex)
            {
                plant->plant.offsets[plantIndex] = Hadamart(RandomBilV2(&plant->sequence), definition->plantOffsetV);
                plant->plant.angleZ[plantIndex] = RandomBil(&plant->sequence) * definition->plantAngleZV;
            }
            
            
            plant->scale = definition->scale + RandomBil(&plant->sequence) * definition->scaleV;
            plant->lengthBase = definition->baseSize * plant->scale;
            
            PlantLevelParams* levelParams = definition->levelParams + 0;
            
            
            plant->plant.trunkCount = 1;
            for(u32 trunkIndex = 0; trunkIndex < plant->plant.trunkCount; ++trunkIndex)
            {
                PlantStem* trunk = GetFreePlantStem(worldMode);
                
                trunk->root = GetFreePlantSegment(worldMode);
                *trunk->root = {};
                
                trunk->segmentCount = 1;
                
                trunk->probabliltyToClone = 1.0f;
                trunk->orientation = Identity();
                trunk->parentStemZ = 0;
                trunk->parentSegmentZ = 0;
                
                trunk->totalLength =  plant->scale * (definition->levelParams->lengthCoeff + RandomBil(&plant->sequence) * levelParams->lengthCoeffV);
                trunk->baseRadious = trunk->totalLength * definition->ratio * (definition->scale_0 + RandomBil(&plant->sequence) * definition->scaleV_0);
                trunk->maxRadious = R32_MAX;
                
                trunk->numberOfChilds = RoundReal32ToU32(levelParams->branches);
                
                trunk->trunkNoise = RandomBil(&plant->sequence);
                r32 childUnitZ = 1.0f /trunk->numberOfChilds;
                trunk->nextChildZ = RandomRangeFloat(&plant->sequence, 0.5f * childUnitZ, childUnitZ - 0.01f);
                trunk->childsCurrentAngle = RandomUni(&plant->sequence) * 360.0f;
                
                trunk->additionalCurveBackAngle = 0;
                
                FREELIST_INSERT(trunk, plant->plant.firstTrunk);
            }
            
            SyncPlantWork* work = PushStruct(&task->pool, SyncPlantWork);
            work->worldMode = worldMode;
            work->definition = definition;
            work->plant = plant;
            work->task = task;
            
            platformAPI.PushWork(gameState->slowQueue, SyncPlantCallback, work);
        }
    }
    
    
    if(plant->canRender)
    {
        SyncPlantWithServer(worldMode, definition, plant);
        
        PlantInstance* instance = &plant->plant;
        for( u32 plantIndex = 0; plantIndex < instance->plantCount; ++plantIndex)
        {
            Vec2 offset = instance->offsets[plantIndex];
            r32 angleZ = instance->angleZ[plantIndex];
            
            Vec3 finalPlantP = plantP + V3(offset, 0);
            m4x4 rotation = ZRotation(DegToRad(angleZ));
            
            
            for(PlantStem* trunk = plant->plant.firstTrunk; trunk; trunk = trunk->next)
            {
                RenderStem(group, renderingParams, worldMode->windTime, plant, definition, trunk, finalPlantP, 0, rotation, 0);
            }
        }
    }
}
