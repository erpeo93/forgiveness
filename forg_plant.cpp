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

inline r32 GetRadiousAtZ(r32 taper, r32 stemLength, r32 stemRadious, r32 Z)
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
    
    return result;
}

inline Vec4 GetSegmentColor()
{
    Vec4 result = V4(1, 1, 1, 1);
    return result;
}

inline void UpdatePlantStem(GameModeWorld* worldMode, ClientPlant* plant, PlantDefinition* definition, PlantStem* stem, u8 recursiveLevel, r32 timeToUpdate)
{
    PlantLevelParams* levelParams = definition->levelParams + recursiveLevel;
    PlantLevelParams* nextLevelParams = levelParams;
    if(recursiveLevel < (definition->maxLevels - 1))
    {
        nextLevelParams = definition->levelParams + (recursiveLevel + 1);
    }
    
    
    m4x4 segmentOrientation = stem->orientation;
    
    r32 segmentLength = stem->totalLength / levelParams->curveRes;
    r32 segmentUnitZ = segmentLength / stem->totalLength;
    r32 segmentBaseZ = 0;
    r32 segmentTopZ = segmentUnitZ;
    
    for(PlantSegment* segment = stem->root; segment;)
    {
        PlantSegment* next = segment->next;
        
        for(PlantStem* child = segment->childs; child; child = child->next)
        {
            UpdatePlantStem(worldMode, plant, definition, child, recursiveLevel + 1, timeToUpdate);
        }
        
        for(PlantStem* clone = segment->clones; clone; clone = clone->next)
        {
            UpdatePlantStem(worldMode, plant, definition, clone, recursiveLevel, timeToUpdate);
        }
        
        segmentOrientation = YRotation(segment->angleY) * segmentOrientation;
        
        b32 spawnedChilds = (segment->lengthCoeff >= levelParams->createChildsLengthNorm);
        b32 spawnedClones = (segment->lengthCoeff >= levelParams->createClonesLengthNorm);
        
        segment->lengthCoeff += levelParams->lengthIncreaseSpeed * timeToUpdate;
        segment->radiousCoeff += levelParams->radiousIncreaseSpeed * timeToUpdate;
        
        segment->lengthCoeff = Min(segment->lengthCoeff, 1.0f);
        segment->radiousCoeff = Min(segment->radiousCoeff, 1.0f);
        
        if(!spawnedClones && (segment->lengthCoeff >= levelParams->createClonesLengthNorm))
        {
            u32 numberOfClones = 3;
#if 0            
            segment->index == 0 ? levelParams->baseSplits : levelParams->segSplits;
            u32 numberOfClones = Compute(numberOfClonesReal);
#endif
            
            
            if(stem->segmentCount < levelParams->curveRes)
            {
                if(numberOfClones == 1)
                {
                    Assert(!segment->clones);
                    Assert(!segment->next);
                    
                    PlantSegment* newSegment = GetFreePlantSegment(worldMode);
                    *newSegment = {};
                    
					u8 actualCurveRes = levelParams->curveRes - 1;
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
                    if(numberOfClones == 2)
                    {
                        rotatingAngle = 20.0f + 0.75f * (30.0f + Abs(declination - 90.0f)) * Square(RandomUni(&plant->sequence));
                        
                        if(RandomUni(&plant->sequence) < 0.5f)
                        {
                            rotatingAngle = -rotatingAngle;
                        }
                    }
                    else
                    {
                        rotatingAngle = 360.0f / numberOfClones;
                    }
                    
                    for(u32 cloneIndex = 0; cloneIndex < numberOfClones; ++cloneIndex)
                    {
                        PlantStem* clonedStem = GetFreePlantStem(worldMode);
                        clonedStem->root = GetFreePlantSegment(worldMode);
                        *clonedStem->root = {};
                        clonedStem->root->angleY = 0;
                        
                        clonedStem->segmentCount = stem->segmentCount + 1;
                        
                        r32 angleY = (levelParams->splitAngle + RandomBil(&plant->sequence) * levelParams->splitAngleV) - declination;
                        clonedStem->orientation =  YRotation(angleY) * segmentOrientation;
                        angleY = Max(angleY, 0);
                        //* ZRotation(runningAngle);
                        
                        clonedStem->parentStemZ = stem->parentStemZ;
                        clonedStem->parentSegmentZ = 1.0f;
                        
                        clonedStem->totalLength = stem->totalLength;
                        clonedStem->baseRadious = stem->baseRadious;
                        
                        clonedStem->numberOfChilds = stem->numberOfChilds;
                        clonedStem->childsCurrentAngle = stem->childsCurrentAngle;
                        
                        clonedStem->additionalCurveBackAngle = -angleY;
                        
                        FREELIST_INSERT(clonedStem, segment->clones);
                        
                        runningAngle += rotatingAngle;
                    }
                }
            }
        }
        
        if(!spawnedChilds && (segment->lengthCoeff >= levelParams->createChildsLengthNorm))
        {
            Assert(!segment->childs);
            
            u32 numberOfChilds = 0;
#if 0            
            r32 numberOfChildsReal = parentStem->numberOfChilds / levelParams->curveRes;
            u32 numberOfChilds = Compute(numberOfChildsReal);
#endif
            
            for(u32 childIndex = 0; childIndex < numberOfChilds; ++childIndex)
            {
                r32 parentSegmentZ = RandomUni(&plant->sequence);
                r32 parentStemZ = segmentBaseZ + parentSegmentZ * segmentUnitZ;
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
                    rotation = 180.0f + levelParams->rotate + RandomBil(&plant->sequence) * levelParams->rotateV;
                    
                }
                stem->childsCurrentAngle += rotation;
                r32 angleZ = stem->childsCurrentAngle;
                
                
                PlantStem* childStem = GetFreePlantStem(worldMode);
                *childStem = {};
                
                childStem->root = GetFreePlantSegment(worldMode);
                *childStem->root = {};
                
                childStem->segmentCount = 1;
                childStem->orientation = YRotation(angleY) * ZRotation(angleZ) * segmentOrientation;
                
                childStem->parentSegmentZ = parentSegmentZ;
                childStem->parentStemZ = parentStemZ;
                
                r32 lengthChildMax = levelParams->length + RandomBil(&plant->sequence) * levelParams->lengthV;
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
                
                r32 radiousAtOriginatingPoint = GetRadiousAtZ(levelParams->taper, stem->totalLength, stem->baseRadious, childStem->parentStemZ);
                childStem->baseRadious = Min(childStem->baseRadious, radiousAtOriginatingPoint);
                
                
                
                r32 lengthChild = childStem->totalLength;
                r32 lengthParent = stem->totalLength;
                if(recursiveLevel == 0)
                {
                    childStem->numberOfChilds = RoundReal32ToU32(levelParams->branches * (lengthChild / lengthParent) / lengthChildMax);
                }
                else
                {
                    childStem->numberOfChilds = RoundReal32ToU32(levelParams->branches * (1.0f - 0.5f * offsetChild / lengthParent));
                }
                
                childStem->childsCurrentAngle = 0;
                childStem->additionalCurveBackAngle = 0;
                
                FREELIST_INSERT(childStem, segment->childs);
            }
        }
        
        segment = next;
        
        segmentBaseZ = segmentTopZ;
        segmentTopZ += segmentUnitZ;
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
        
        trunk->orientation = Identity();
        
        trunk->parentStemZ = 0;
        trunk->parentSegmentZ = 0;
        
        trunk->totalLength =  plant->scale * (definition->levelParams->length + RandomBil(&plant->sequence) * levelParams->lengthV);
        trunk->baseRadious = trunk->totalLength * definition->ratio * (definition->scale_0 + RandomBil(&plant->sequence) * definition->scaleV_0);
        
        trunk->numberOfChilds = 0;
        trunk->childsCurrentAngle = 0;
        
        trunk->additionalCurveBackAngle = 0;
    }
    
    UpdatePlantStem(worldMode, plant, definition, trunk, 0, timeToUpdate);
}


internal void RenderStem(RenderGroup* group, Vec4 lightIndexes, PlantDefinition* definition, PlantStem* stem, Vec3 stemP, u8 recursiveLevel, m4x4 originOrientation)
{
    m4x4 baseOrientation = baseOrientation;
    m4x4 topOrientation = stem->orientation;
    
    Assert(stem->root);
    Assert(stem->root->angleY == 0);
    
    Vec3 segmentBaseP = stemP;
    
    PlantLevelParams* levelParams = definition->levelParams + recursiveLevel;
    r32 segmentLength = stem->totalLength / levelParams->curveRes;
    r32 segmentUnitZ = segmentLength / stem->totalLength;
    
    r32 baseZ = 0;
    r32 topZ = segmentUnitZ;
    
    for(PlantSegment* segment = stem->root; segment; segment = segment->next)
    {
        r32 baseRadious = GetRadiousAtZ(levelParams->taper, stem->totalLength, stem->baseRadious, baseZ) * segment->radiousCoeff;
        r32 topRadious = GetRadiousAtZ(levelParams->taper, stem->totalLength, stem->baseRadious, topZ) * segment->radiousCoeff;
        r32 length = segmentLength * segment->lengthCoeff;
        Vec4 color = GetSegmentColor();
        
        Vec3 bottomXAxis = GetColumn(baseOrientation, 0);
        Vec3 bottomYAxis = GetColumn(baseOrientation, 1);
        Vec3 bottomZAxis = GetColumn(baseOrientation, 2);
        
        r32 rotationAngle = segment->next ? segment->next->angleY : 0;
        topOrientation = YRotation(DegToRad(rotationAngle)) * topOrientation;
        Vec3 topXAxis = GetColumn(topOrientation, 0);
        Vec3 topYAxis = GetColumn(topOrientation, 1);
        Vec3 topZAxis = GetColumn(topOrientation, 2);
        
        Vec3 topP = segmentBaseP + length * bottomZAxis;
        PushTrunkatedPyramid(group, segmentBaseP, topP, 4, bottomXAxis, bottomYAxis, bottomZAxis, topXAxis, topYAxis, topZAxis, baseRadious, topRadious, length, color, lightIndexes, 0);
        
        for(PlantStem* child = segment->childs; child; child = child->next)
        {
            Vec3 childP = segmentBaseP + child->parentSegmentZ * length * bottomZAxis;
            RenderStem(group, lightIndexes, definition, child, childP, recursiveLevel + 1, ?);
        }
        
        for(PlantStem* clone = segment->clones; clone; clone = clone->next)
        {
            Vec3 cloneP = segmentBaseP + clone->parentSegmentZ * length * bottomZAxis;
            RenderStem(group, lightIndexes, definition, clone, cloneP, recursiveLevel, topOrientation);
        }
        
        segmentBaseP = topP;
        baseOrientation = topOrientation;
        
        baseZ = topZ;
        topZ += segmentUnitZ;
    }
}




internal void UpdateAndRenderPlant(GameModeWorld* worldMode, RenderGroup* group, Vec4 lightIndexes, PlantDefinition* definition, ClientPlant* plant, Vec3 plantP, r32 timeToUpdate)
{
    timeToUpdate *= worldMode->UI->plantGrowingCoeff;
    r32 targetUpdateTime = 1.0f;
    
    plant->elapsedFromLastUpdate += timeToUpdate;
    if(plant->elapsedFromLastUpdate >= targetUpdateTime)
    {
        UpdatePlant(worldMode, definition, plant, plant->elapsedFromLastUpdate);
        plant->elapsedFromLastUpdate -= targetUpdateTime;
    }
#if 0
    for( u32 plantIndex = 0; plantIndex < params->plantCount; ++plantIndex )
    {
        r32 offsetX = RandomRangeFloat( &seq, params->minOffset, params->maxOffset );
        r32 offsetY = RandomRangeFloat( &seq, params->minOffset, params->maxOffset );
        Vec3 plantP = entityC->P + V3( offsetX, offsetY, 0 );
    }
#endif
    
    RenderStem(group, lightIndexes, definition, &plant->trunk, plantP, 0, Identity());
}
