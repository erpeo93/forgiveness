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
    Assert(Normalized(ratio));
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

inline void UpdatePlantStem(GameModeWorld* worldMode, PlantDefinition* definition, PlantStem* stem, u8 recursiveLevel, r32 timeToUpdate)
{
    PlantLevelParams* levelParams = definition->levelParams + recursiveLevel;
    PlantLevelParams* nextLevelParams;
    
    if(recursiveLevel < (definition->maxLevels - 1))
    {
        nextLevelParams = definition->levelParams + (recursiveLevel + 1);
    }
    else
    {
        nextLevelParams = levelParams;
    }
    
    
    for(PlantSegment* segment = stem->root; segment;)
    {
        PlantSegment* next = segment->next;
        for(PlantStem* child = segment->childs; child; child = child->next)
        {
            UpdatePlantStem(worldMode, definition, child, recursiveLevel + 1, timeToUpdate);
        }
        
        for(PlantStem* clone = segment->clones; clone; clone = clone->next)
        {
            UpdatePlantStem(worldMode, definition, clone, recursiveLevel, timeToUpdate);
        }
        
        
        b32 spawnedChilds = (segment->lengthCoeff >= levelParams->createChildsLengthNorm);
        b32 spawnedClones = (segment->lengthCoeff >= levelParams->createClonesLengthNorm);
        
        segment->lengthCoeff += levelParams->lengthIncreaseSpeed * timeToUpdate;
        segment->radiousCoeff += levelParams->radiousIncreaseSpeed * timeToUpdate;
        
        segment->lengthCoeff = Min(segment->lengthCoeff, 1.0f);
        segment->radiousCoeff = Min(segment->radiousCoeff, 1.0f);
        
        
        if(!spawnedClones && (segment->lengthCoeff >= levelParams->createClonesLengthNorm))
        {
            r32 numberOfClonesReal = segment->index == 0 ? levelParams->baseSplits : levelParams->segSplits;
            u32 numberOfClones = Compute(numberOfClonesReal);
            
            
            if(numberOfClones == 1)
            {
                Assert(!segment->clones);
                Assert(!segment->next);
                
                if(stem->segmentCount < levelParams->curveRes)
                {
                    ++stem->segmentCount;
                    PlantSegment* newSegment = GetFreePlantSegment(worldMode);
                    *newSegment = {};
                    
                    if(levelParams->curveBack == 0)
                    {
                        newSegment->angleY = levelParams->curve / levelParams->curveRes;
                    }
                    else
                    {
                        if(levelParams->curveBack > 0)
                        {
                            if(segment->segmentIndex < levelParams->curveRes / 2)
                            {
                                newSegment->angleY = (levelParams->curve / (levelParams->curveRes / 2));
                            }
                            else
                            {
                                newSegment->angleY = (levelParams->curveBack / (levelParams->curveRes / 2));
                            }
                        }
                        else
                        {
                            InvalidCodePath;
                        }
                    }
                    
                    if(levelParams->curveV >= 0)
                    {
                        newSegment->angleY += RandomBil() * (levelParams->curveV / levelParams->curveRes);
                    }
                    else
                    {
                        InvalidCodePath;
                    }
                    newSegment->angleY += stem->additionalCurveBackSegmentAngle;
                    
                    
                    segment->next = newSegment;
                }
            }
            else
            {
                r32 declination = ?;
                
                r32 runningAngle = 0;
                r32 rotatingAngle = 0;
                if(cloneCount == 2)
                {
                    r32 rotatingAngle = 20.0f + 0.75f * (30.0f + Abs(declination - 90.0f)) * Square(RandomUni());
                    
                    if(RandomUni() < 0.5f)
                    {
                        rotatingAngle = -rotatingAngle;
                    }
                }
                else
                {
                    rotatingAngle = 360 / cloneCount;
                }
                
                for(u32 cloneIndex = 0; cloneIndex < cloneCount; ++cloneIndex)
                {
                    PlantStem* clonedStem = GetFreePlantStem(worldMode);
                    clonedStem->root = GetFreePlantStem(worldMode);
                    *clonedStem->root = {};
                    
                    clonedStem->root->angleY = 0;
                    clonedStem->segmentCount = stem->segmentCount;
                    
                    r32 angleY = (levelParams->splitAngle + RandomBil() * levelParams->splitAngleV) - declination;
                    stem->orientation = ZRotation(runningAngle) * segmentOrientation * RotateY(angleY);
                    
                    
                    clonedStem->parentStemZ = ?;
                    clonedStem->parentSegmentZ = ?;
                    
                    clonedStem->length = stem->length;
                    clonedStem->baseRadious = stem->baseRadious;
                    
                    clonedStem->childMaxCount = stem->childMaxCount;
                    clonedStem->childsCurrentAngle = stem->childsCurrentAngle;
                    
                    clonedStem->additionalCurveBackAngle = -angleY;
                    
                    FREELIST_INSERT(stem, segment->clones);
                    
                    
                    runningAngle += rotatingAngle;
                }
            }
        }
        
        if(!spawnedChilds && (segment->lengthCoeff >= levelParams->createChildsLengthNorm))
        {
            Assert(!segment->childs);
            
            r32 numberOfChildsReal = parentStem->numberOfChilds / levelParams->curveRes;
            u32 numberOfChilds = Compute(numberOfChildsReal);
            
            for(u32 childIndex = 0; childIndex < numberOfChilds; ++childIndex)
            {
                r32 angleY;
                if(levelParams->downAngleV >= 0)
                {
                    angleY = levelParams->downAngle + RandomBil() * levelParams->downAngleV;
                }
                else
                {
                    angleY = levelParams->downAngle + RandomBil() * (levelParams->downAngleV * (1.0f - 2.0f * ShapeRatio(0, (parentStem->length - childOffset) / (parentStem->length - baseLength))));
                }
                
                r32 rotation;
                if(levelParams->rotate >= 0)
                {
                    rotation = levelParams->rotate + RandomBil() * levelParams->rotateV;
                }
                else
                {
                    rotation = 180.0f + levelParams->rotate + RandomBil() * levelParams->rotateV;
                    
                }
                parentStem->childsCurrentAngle += rotation;
                r32 angleZ = parentStem->childsCurrentAngle;
                
                
                PlantStem* childStem = GetFreePlantStem(worldMode);
                
                childStem->root = GetFreePlantSegment(worldMode);
                *childStem->root = {};
                
                childStem->segmentCount = 0;
                childStem->orientation = segmentOrientation * RotateY(angleY);
                
                childStem->parentStemZ = ?;
                
                
                r32 lengthChildMax = levelParams->length + RandomBil() * length->lengthV;
                if(recursiveLevel == 0)
                {
                    r32 trunkLength = ?;
                    r32 scaleTree = definition->scale;
                    r32 baseLength = definition->baseSize * scaleTree;
                    
                    stem->length = parentStem->length * lengthChildMax * ShapeRatio(definition->shape, (trunkLength - offsetChild) / (trunkLength - baseLength));
                }
                else
                {
                    stem->length = lengthChildMax * (parentStem->length - 0.6f * offsetChild);
                }
                
                stem->baseRadious = parentStem->baseRadious * Pow(stem->length / parentStem->length, definition->ratioPower);
                
                r32 radiousAtOriginatingPoint = GetRadiousAtZ();
                stem->baseRadious = Min(stem->baseRadious, radiousAtOriginatingPoint);
                
                
                
                r32 lengthChild = stem->length;
                r32 lengthParent = parentStem->length;
                if(parentStem->recursiveLevel == 0)
                {
                    stem->numberOfChilds = levelParams->branches * (lengthChild / lengthParent) / lengthChildMax;
                }
                else
                {
                    stem->numberOfChilds = levelParams->branches * (1.0f - 0.5f * offsetChild / lengthParent);
                }
                
                childStem->childsCurrentAngle = 0;
                childStem->additionalCurveBackAngle = 0;
                
                FREELIST_INSERT(stem, segment->clones);
                
                runningAngle += rotatingAngle;
            }
        }
        
        segment = next;
    }
}

inline void UpdatePlant(GameModeWorld* worldMode, PlantDefinition* definition, ClientPlant* plant, r32 timeToUpdate)
{
    PlantStem* trunk = &plant->trunk;
    if(!trunk->root)
    {
        PlantLevelParams* levelParams = definition->levelParams + 0;
        plant->scale = definition->scale + RandomBil() * definition->scaleV;
        plant->lengthBase = definition->baseSize * plant->scale;
        
        trunk->length = plant->scale * (definition->scale_0 + RandomBil() * definition->scaleV_0);
        trunk->baseRadious = trunk->length * definition->ratio * definition->scale_0;
        trunk->orientation = Identity();
        trunk->root = GetFreePlantSegment(worldMode);
        *trunk->root = {};
    }
    
    UpdatePlantStem(worldMode, definition, trunk, 0, timeToUpdate);
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
    
    r32 result = 0.3f * coeff;
    return result;
}

inline Vec4 GetSegmentColor()
{
    Vec4 result = V4(1, 1, 1, 1);
    return result;
}

internal void RenderStem(RenderGroup* group, Vec4 lightIndexes, PlantDefinition* definition, PlantStem* stem, Vec3 stemP, m4x4 parentOrientation, u8 recursiveLevel)
{
    m4x4 stemOrientation = parentOrientation * stem->orientation;
    m4x4 segmentOrientation = stemOrientation;
    
    Vec3 segmentBaseP = stemP;
    
    PlantLevelParams* levelParams = definition->levelParams + recursiveLevel;
    r32 segmentLength = stem->length / levelParams->curveRes;
    
    r32 segmentZ = segmentLength / stem->length;
    r32 baseZ = 0;
    r32 topZ = segmentZ;
    
    for(PlantSegment* segment = stem->root; segment; segment = segment->next)
    {
        r32 baseRadious = GetRadiousAtZ(levelParams->taper, stem->length, stem->baseRadious, baseZ) * segment->radiousCoeff;
        r32 topRadious = GetRadiousAtZ(levelParams->taper, stem->length, stem->baseRadious, topZ) * segment->radiousCoeff;
        r32 length = segmentLength * segment->lengthCoeff;
        Vec4 color = GetSegmentColor();
        
        Vec3 bottomXAxis = GetColumn(segmentOrientation, 0);
        Vec3 bottomYAxis = GetColumn(segmentOrientation, 1);
        Vec3 bottomZAxis = GetColumn(segmentOrientation, 2);
        
        m4x4 topOrientation = segmentOrientation * YRotation(segment->angleY) * XRotation(segment->angleX);
        Vec3 topXAxis = GetColumn(topOrientation, 0);
        Vec3 topYAxis = GetColumn(topOrientation, 1);
        Vec3 topZAxis = GetColumn(topOrientation, 2);
        
        Vec3 topP = segmentBaseP + length * bottomZAxis;
        
        PushTrunkatedPyramid(group, segmentBaseP, 4, bottomXAxis, bottomYAxis, topZAxis, topXAxis, topYAxis, baseRadious, topRadious, length, color, lightIndexes, 0);
        
        for(PlantStem* child = segment->childs; child; child = child->next)
        {
            Vec3 childP = segmentBaseP + child->parentSegmentZ * bottomZAxis;
            RenderStem(group, lightIndexes, definition, child, childP, segmentOrientation, recursiveLevel + 1);
        }
        
        for(PlantStem* clone = segment->clones; clone; clone = clone->next)
        {
            Vec3 cloneP = segmentBaseP + clone->parentSegmentZ * bottomZAxis;
            RenderStem(group, lightIndexes, definition, clone, cloneP, segmentOrientation, recursiveLevel);
        }
        
        segmentBaseP = topP;
        segmentOrientation = topOrientation;
        
        baseZ = topZ;
        topZ += segmentZ;
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
    
    RenderStem(group, lightIndexes, definition, &plant->trunk, plantP, Identity());
}
