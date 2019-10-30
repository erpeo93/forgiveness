inline r32 LerpAnglesWithSpin(i32 spin, r32 angle1, r32 angle2, r32 lerp )
{
    r32 result = 0;
    if(spin == -1)
    {
        r32 reverseLerp = 0;
        if(angle2 > angle1)
        {
            reverseLerp = Lerp(angle2, lerp, (angle1 + 360.0f)) - angle2;
        }
        else
        {
            reverseLerp = Lerp(angle2, lerp, angle1) - angle2;
        }
        result = angle1 - reverseLerp;
    }
    else
    {
        if(angle2 < angle1)
        {
            result = Lerp(angle1, lerp, (angle2 + 360.0f));
        }
        else
        {
            result = Lerp(angle1, lerp, angle2);
        }
    }
    
    return result;
}


internal Bone BlendBones_(Bone* b1, r32 lerp, Bone* b2)
{
    TIMED_FUNCTION();
    Assert(b1->timeLineIndex == b2->timeLineIndex);
    Assert(b1->parentIndex == b2->parentIndex);
    Bone result = {};
    
    result.id = b1->id;
    result.parentIndex = b1->parentIndex;
    
    result.parentAngle = LerpAnglesWithSpin(b1->spin, b1->parentAngle, b2->parentAngle, lerp);
    result.parentOffset = Lerp(b1->parentOffset, lerp, b2->parentOffset);
    result.finalAngle = result.parentAngle;
    
    return result;
}

internal PieceAss BlendAss_(PieceAss* p1, r32 lerp, PieceAss* p2)
{
    TIMED_FUNCTION();
    Assert(p1->boneIndex == p2->boneIndex);
    PieceAss result = {};
    
    result.spriteIndex = lerp <= 0.5f ? p1->spriteIndex : p2->spriteIndex;
    result.boneIndex = p1->boneIndex;
    result.color = Lerp(p1->color, lerp, p2->color);
    result.spin = p1->spin;
    result.angle = LerpAnglesWithSpin(result.spin, p1->angle, p2->angle, lerp);
    result.scale = Lerp(p1->scale, lerp, p2->scale);
    result.boneOffset = Lerp(p1->boneOffset, lerp, p2->boneOffset);
    
    return result;
}

inline Bone* FindBone(Animation* animation, u32 timelineIndex, u32 startingFrameIndex, b32 negativeDelta, u32* timelineMS)
{
    Bone* result = 0;
    u32 test = startingFrameIndex;
    while(test != 0)
    {
        FrameData* testFrame = animation->frames + test;
        Bone* firstTestBone = animation->bones + testFrame->firstBoneIndex;
        for(u32 testBoneIndex = 0; testBoneIndex < testFrame->countBones; ++testBoneIndex)
        {
            Bone* testBone = firstTestBone + testBoneIndex;
            if(testBone->timeLineIndex == timelineIndex)
            {
                result = testBone;
                *timelineMS = testFrame->timelineMS;
                break;
            }
        }
        if(result)
        {
            break;
        }
        else
        {
            if(negativeDelta)
            {
                --test;
            }
            else
            {
                if(++test >= animation->frameCount)
                {
                    test = 0;
                }
            }
        }
    }
    
    return result;
}

inline PieceAss* FindAss(Animation* animation, u32 timeLineIndex, u32 startingFrameIndex, b32 negativeDelta, u32* timelineMS)
{
    PieceAss* result = 0;
    
    u32 test = startingFrameIndex;
    while(test != 0)
    {
        FrameData* testFrame = animation->frames + test;
        PieceAss* firstTestAss = animation->ass + testFrame->firstAssIndex;
        for(u32 testAssIndex = 0; testAssIndex < testFrame->countAss; ++testAssIndex)
        {
            PieceAss* testAss = firstTestAss + testAssIndex;
            if(testAss->timeLineIndex == timeLineIndex)
            {
                result = testAss;
                *timelineMS = testFrame->timelineMS;
                break;
            }
        }
        
        if(negativeDelta)
        {
            --test;
        }
        else
        {
            if(++test >= animation->frameCount)
            {
                test = 0;
            }
        }
    }
    
    return result;
}

struct BlendedFrame
{
    u32 boneCount;
    Bone* bones;
    
    u32 assCount;
    PieceAss* ass;
};

internal void BlendFrames_(MemoryPool* tempPool, PAKAnimation* animationInfo, Animation* animation, BlendedFrame* in, u32 lowerFrameIndex, u32 timelineMS, u32 upperFrameIndex)
{
    FrameData* reference = animation->frames + 0;
    in->boneCount = reference->countBones;
    in->assCount = reference->countAss;
    
    in->bones = PushArray(tempPool, Bone, in->boneCount);
    in->ass = PushArray(tempPool, PieceAss, in->assCount);
    
    Bone* firstRefBone = animation->bones + reference->firstBoneIndex;
    for(u32 boneIndex = 0; boneIndex < in->boneCount; boneIndex++)
    {
        u32 lowerTimeLine = reference->timelineMS;
        u32 upperTimeLine = reference->timelineMS + animationInfo->durationMS;
        
        Bone* refBone = firstRefBone + boneIndex;
        Bone* b1 = FindBone(animation, refBone->timeLineIndex, lowerFrameIndex, true, &lowerTimeLine);
        if(!b1)
        {
            b1 = refBone;
        }
        
        Bone* b2 = FindBone(animation, refBone->timeLineIndex, upperFrameIndex, false, &upperTimeLine);
        if(!b2)
        {
            b2 = refBone;
        }
        
        Assert(upperTimeLine >= lowerTimeLine);
        Assert(timelineMS >= lowerTimeLine);
        
        u32 exceedingLower = timelineMS - lowerTimeLine;
        r32 lerp = SafeRatio1((r32) exceedingLower, (r32) (upperTimeLine - lowerTimeLine));
        
        Bone* dest = in->bones + boneIndex;
        *dest = BlendBones_(b1, lerp, b2);
    }
    
    
    PieceAss* firstRefAss = animation->ass + reference->firstAssIndex;
    for(u32 assIndex = 0; assIndex < in->assCount; assIndex++)
    {
        u32 lowerTimeLine = reference->timelineMS;
        u32 upperTimeLine = reference->timelineMS + animationInfo->durationMS;
        
        PieceAss* refAss = firstRefAss + assIndex;
        PieceAss* a1 = FindAss(animation, refAss->timeLineIndex, lowerFrameIndex, true, &lowerTimeLine);
        if(!a1)
        {
            a1 = refAss;
        }
        
        PieceAss* a2 = FindAss(animation, refAss->timeLineIndex, upperFrameIndex, false, &upperTimeLine);
        if(!a2)
        {
            a2 = refAss;
        }
        
        Assert(upperTimeLine >= lowerTimeLine);
        Assert(timelineMS >= lowerTimeLine);
        
        u32 exceedingLower = timelineMS - lowerTimeLine;
        r32 lerp = SafeRatio1((r32) exceedingLower, (r32) (upperTimeLine - lowerTimeLine));
        
        PieceAss* dest = in->ass + assIndex;
        *dest = BlendAss_(a1, lerp, a2);
        Assert(dest->spriteIndex < animation->spriteInfoCount);
    }
}

internal Vec2 CalculateFinalBoneOffset_(Bone* frameBones, i32 countBones, Bone* bone, AnimationParams* params)
{
    Assert(bone->parentIndex < countBones);
    Vec2 baseOffset = V2(0, 0);
    
    Vec2 XAxis;
    Vec2 YAxis;
    
    Vec2 offsetFromParentScale = V2(1, 1);
    if(bone->parentIndex >= 0)
    {
        Bone* parent = frameBones + bone->parentIndex;
        Assert(parent->id <= bone->id);
        
        baseOffset = parent->finalOriginOffset;
        XAxis = parent->mainAxis;
        YAxis = Perp(XAxis);
    }
    else
    {
        r32 rad = DegToRad(params->angle);
        XAxis = V2(Cos(rad), Sin(rad));
        YAxis = Perp(XAxis);
    }
    
    Vec2 parentOffset = Hadamart(bone->parentOffset, offsetFromParentScale) * params->scale;
    Vec2 result = baseOffset + (parentOffset.x * XAxis + parentOffset.y * YAxis);
    
    return result;
}

internal AnimationPiece* GetAnimationPiecesAndAdvanceState(MemoryPool* tempPool, PAKAnimation* animationInfo, Animation* animation,
                                                           AnimationComponent* component, AnimationParams* params, u32* animationPieceCount, b32 render)
{
    u32 animTimeMod = 0;
    if(render)
    {
        r32 oldTime = component->time;
        r32 newTime = oldTime + params->elapsedTime;
        component->time = newTime;
        u32 timeline = (u32) (component->time * 1000.0f);
        animTimeMod = timeline % animationInfo->durationMS;
    }
    
    u32 lowerFrameIndex = 0;
    for(u32 frameIndex = 0; frameIndex < animation->frameCount; frameIndex++)
    {
        FrameData* data = animation->frames + frameIndex;
        if(animTimeMod >= data->timelineMS)
        {
            lowerFrameIndex = frameIndex;
        }
        else
        {
            break;
        }
    }
    u32 upperFrameIndex = (lowerFrameIndex + 1);
    if(upperFrameIndex == animation->frameCount)
    {
        upperFrameIndex = 0;
    }
    
    BlendedFrame blended = {};
    BlendFrames_(tempPool, animationInfo, animation, &blended, lowerFrameIndex, animTimeMod, upperFrameIndex);
    
    for(u32 boneIndex = 0; boneIndex < blended.boneCount; boneIndex++)
    {
        Bone* bone = blended.bones + boneIndex;
        if(bone->parentIndex != -1)
        {
            Bone* parent = blended.bones + bone->parentIndex;
            bone->finalAngle += parent->finalAngle;
        }
        else
        {
            bone->finalAngle += params->angle;
        }
        r32 totalAngleRad = DegToRad(bone->finalAngle);
        bone->finalOriginOffset = CalculateFinalBoneOffset_(blended.bones, blended.boneCount, bone, params);
        
        Vec2 scale = V2(1.0f, 1.0f);
#if 0        
        if(boneAlt->valid)
        {
            scale = boneAlt->scale;
        }
#endif
        
        bone->mainAxis = Hadamart(scale, V2(Cos(totalAngleRad), Sin(totalAngleRad))); 
    }
    
    *animationPieceCount = blended.assCount;
    AnimationPiece* result = PushArray(tempPool, AnimationPiece, blended.assCount);
    
    r32 zOffset = 0.0f;
    for(u32 assIndex = 0; assIndex < blended.assCount; ++assIndex)
    {
        AnimationPiece* dest = result + assIndex;
        
        PieceAss* ass = blended.ass + assIndex;
        SpriteInfo* sprite = animation->spriteInfos + ass->spriteIndex;
        
        dest->pivot = sprite->pivot;
        dest->nameHash = sprite->nameHash;
        dest->placeHolder = sprite->placeHolder;
        
        Assert(ass->boneIndex >= 0);
        Bone* parentBone = blended.bones + ass->boneIndex;
        Vec2 boneXAxis = parentBone->mainAxis;
        Vec2 boneYAxis = Perp(boneXAxis);
        
        Vec2 boneOffset = ass->boneOffset * params->scale;
        
        Vec2 offsetFromBone = boneOffset.x * boneXAxis + boneOffset.y * boneYAxis;
        dest->originOffset = V3(parentBone->finalOriginOffset + offsetFromBone, zOffset + ass->additionalZOffset);
        
        dest->angle = parentBone->finalAngle + ass->angle;
        dest->scale = ass->scale;
        dest->color = ass->color;
        
        zOffset += 0.01f;
    }
    
    
#if 0    
    r32 waitingForSyncThreesold = 0.8f;
    if(timeline >= header->durationMS || (state->stopAtNextBarrier && !RequiresAnimationSync((EntityAction) state->action)) || state->waitingForSyncTimer >= waitingForSyncThreesold)
    {
        StartNextAction(state);
    }
#endif
    
#if 0    
    if(state->stopAtNextBarrier)
    {
        // NOTE(Leonardo): if the animation has no barriers, interrupt it immediately!
        ended = true;
        
        u32 oldTimeMod = (u32) (oldAnimationTime * 1000.0f) % header->durationMS;
        for(u32 barrierIndex = 0; barrierIndex < ArrayCount(header->barriers); barrierIndex++)
        {
            r32 realBarrier = header->barriers[barrierIndex];
            Assert(Normalized(realBarrier));
            
            u32 barrier =  (u32) (realBarrier * header->durationMS);
            if(barrier > 0)
            {
                ended = false;
                if(barrier == header->durationMS)
                {
                    animTimeMod = timeline;
                }
                
                if(oldTimeMod <= barrier && animTimeMod >= barrier)
                {
                    ended = true;
                    animTimeMod = barrier;
                    break;
                }
            }
        }
    }
#endif
    
    return result;
}

inline Vec2 GetBoneAxisOffsetfromXY(Bone* bone, Vec2 alignedOffset)
{
    Vec2 result = {};
    
    Vec2 normMain = Normalize(bone->mainAxis);
    Vec2 perpNormMain = Perp(normMain);
    result.x = Dot(normMain, alignedOffset); 
    result.y = Dot(perpNormMain, alignedOffset);
    
    return result;
}

inline r32 ArrangeObjects(u8 gridDimX,u8 gridDimY, Vec3 originalGridDim)
{
    r32 result = Min(originalGridDim.x / gridDimX, originalGridDim.y / gridDimY);
    return result;
}


#if 0
inline void ApplyAssAlterations(PieceAss* ass, AssAlteration* assAlt, Bone* parentBone, Vec4* proceduralColor)
{
    if(assAlt->valid)
    {
        ass->boneOffset += GetBoneAxisOffsetfromXY(parentBone, assAlt->boneOffset);
        ass->scale = Hadamart(ass->scale, assAlt->scale);
        
        if(assAlt->specialColoration)
        {
            *proceduralColor = assAlt->color;
        }
    }
    
}
#endif

internal r32 GetModulationPercentageAndResetFocus(GameModeWorld* worldMode, EntityID ID)
{
    r32 result = 0;
    InteractionComponent* interaction = GetComponent(worldMode, ID, InteractionComponent);
    if(interaction && interaction->isOnFocus)
    {
        result = 0.7f;
    }
    return result;
}

internal Rect2 RenderLayout(GameModeWorld* worldMode, RenderGroup* group, Vec3 P, ObjectTransform transform, LayoutComponent* layout, u32 seed, Lights lights, struct LayoutContainer* container);
internal void RenderAttachmentPoint(GameModeWorld* worldMode, RenderGroup* group, Vec3 P, u64 hash, ObjectTransform transform, ObjectMapping* mappings, u32 mappingCount, b32* alreadyRendered, Lights lights)
{
    Assert(hash);
    for(u32 mappingIndex = 0; mappingIndex < mappingCount; ++mappingIndex)
    {
        if(!alreadyRendered[mappingIndex])
        {
            ObjectMapping* mapping = mappings + mappingIndex;
            if(IsValid(mapping->ID) && (hash == mapping->slotHash || hash == mapping->pieceHash))
            {
                alreadyRendered[mappingIndex] = true;
                
                EntityID equipmentID = mapping->ID;
                
                BaseComponent* equipmentBase = GetComponent(worldMode, equipmentID, BaseComponent);
                InteractionComponent* equipmentInteraction = GetComponent(worldMode, equipmentID, InteractionComponent);
                LayoutComponent* equipmentLayout = GetComponent(worldMode, equipmentID, LayoutComponent);
                
                if(equipmentBase && equipmentLayout)
                {
                    ObjectTransform finalTransform = transform;
                    finalTransform.angle += equipmentLayout->rootAngle;
                    finalTransform.scale = Hadamart(finalTransform.scale, equipmentLayout->rootScale);
                    
                    if(equipmentInteraction && equipmentInteraction->isOnFocus)
                    {
                        finalTransform.modulationPercentage = GetModulationPercentageAndResetFocus(worldMode, equipmentID);
                    }
                    
                    LayoutContainer container = {};
                    //container.container = GetComponent(worldMode, equipmentID, ContainerMappingComponent);
                    
                    equipmentBase->projectedOnScreen = RenderLayout(worldMode, group, P, finalTransform, equipmentLayout, equipmentBase->seed, lights, &container);
                }
                break;
            }
        }
    }
}

internal void RenderObjectMappings(GameModeWorld* worldMode, RenderGroup* group,
                                   PAKBitmap* bitmapInfo, BitmapId BID, BitmapDim dim, ObjectTransform transform, ObjectMapping* mappings, u32 mappingCount, b32* alreadyRendered, Lights lights)
{
    for(u32 attachmentPointIndex = 0; attachmentPointIndex < bitmapInfo->attachmentPointCount; ++attachmentPointIndex)
    {
        PAKAttachmentPoint* point = GetAttachmentPoint(group->assets, BID, attachmentPointIndex);
        if(point)
        {
			u64 pointHash = StringHash(point->name);
            if(pointHash)
            {
                Vec3 P = GetAlignP(dim, point->alignment);
                
                ObjectTransform attachmentTransform = transform;
                attachmentTransform.angle += point->angle;
                attachmentTransform.scale = Hadamart(attachmentTransform.scale, point->scale);
                attachmentTransform.additionalZBias = point->zOffset;
                RenderAttachmentPoint(worldMode, group, P, pointHash, attachmentTransform, mappings, mappingCount, alreadyRendered, lights);
            }
        }
    }
}

internal Rect2 RenderAnimation_(GameModeWorld* worldMode, RenderGroup* group, AssetID animationID, AnimationComponent* component, AnimationParams* params, b32 render = true)
{
    TempMemory temp = BeginTemporaryMemory(worldMode->persistentPool);
    
    b32* usingRendered = 0;
    if(params->equipped)
    {
        usingRendered = PushArray(temp.pool, b32, ArrayCount(params->equipped->mappings));
    }
    b32* equipmentRendered = 0;
    if(params->equipment)
    {
        equipmentRendered = PushArray(temp.pool, b32, ArrayCount(params->equipment->mappings));
    }
    
    Rect2 result = InvertedInfinityRect2();
    Animation* animation = GetAnimation(group->assets, animationID);
    if(animation)
    {
        PAKAnimation* animationInfo = GetAnimationInfo(group->assets, animationID);
        u16 bitmapCount = 0;
        AssetID* bitmaps = GetAllSkinBitmaps(temp.pool, group->assets, GetAssetSubtype(group->assets, AssetType_Image, component->skinHash), &component->skinProperties, &bitmapCount);
        
        u32 pieceCount;
        AnimationPiece* pieces = GetAnimationPiecesAndAdvanceState(temp.pool, animationInfo, animation, component, params, &pieceCount, render);
        
        ObjectTransform transform = params->transform;
        transform.flipOnYAxis = params->flipOnYAxis;
        transform.dontRender = !render;
        transform.modulationPercentage = params->modulationPercentage;
        
        Lights lights = params->lights;
        
        if(render && params->equipped)
        {
            for(u32 pieceIndex = 0; pieceIndex < pieceCount; ++pieceIndex)
            {
                AnimationPiece* piece = pieces + pieceIndex;
                if(piece->placeHolder)
                {
                    ObjectTransform equippedTransform = transform;
                    equippedTransform.angle = piece->angle;
                    equippedTransform.scale = piece->scale;
                    
                    Vec3 offset = GetCameraOffset(group, piece->originOffset);
                    if(transform.flipOnYAxis)
                    {
                        offset.x = -offset.x;
                    }
                    Vec3 P = params->P + offset;
                    
                    RenderAttachmentPoint(worldMode, group, P, piece->nameHash, equippedTransform, params->equipped->mappings, ArrayCount(params->equipped->mappings), usingRendered, lights);
                }
            }
        }
        
        for(u32 pieceIndex = 0; pieceIndex < pieceCount; ++pieceIndex)
        {
            AnimationPiece* piece = pieces + pieceIndex;
            if(!piece->placeHolder)
            {
                for(u32 bitmapIndex = 0; bitmapIndex < bitmapCount; ++bitmapIndex)
                {
                    AssetID BID = bitmaps[bitmapIndex];
                    if(IsValid(BID))
                    {
                        PAKBitmap* bitmapInfo = GetBitmapInfo(group->assets, BID);
                        if(bitmapInfo->nameHash == piece->nameHash)
                        {
                            transform.angle = piece->angle;
                            Vec3 P = params->P;
                            
                            transform.cameraOffset = piece->originOffset;
                            r32 height = bitmapInfo->nativeHeight * params->scale;
                            
                            Vec4 color = Hadamart(piece->color, params->tint);
                            BitmapDim dim = PushBitmapWithPivot(group, transform, BID, P, piece->pivot, height, color, lights);
                            result = Union(result, RectMinDim(dim.P.xy, dim.size));
                            
                            ObjectTransform equipmentTransform = transform;
                            equipmentTransform.cameraOffset = {};
                            if(render)
                            {
                                if(params->equipment)
                                {
                                    RenderObjectMappings(worldMode, group,
                                                         bitmapInfo, BID, dim, equipmentTransform, params->equipment->mappings, ArrayCount(params->equipment->mappings), equipmentRendered, lights);
                                }
                                
                                if(params->equipped)
                                {
                                    RenderObjectMappings(worldMode, group,
                                                         bitmapInfo, BID, dim, equipmentTransform, params->equipped->mappings, ArrayCount(params->equipped->mappings), usingRendered, lights);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    else
    {
        LoadAnimation(group->assets, animationID);
    }
    
    EndTemporaryMemory(temp);
    return result;
}

internal Rect2 RenderAnimation(GameModeWorld* worldMode, RenderGroup* group, AnimationComponent* component, AnimationParams* params, b32 render = true)
{
    Rect2 result = InvertedInfinityRect2();
    RandomSequence seq = {};
    
    b32 flippedByDefault = false;
    AssetID animationID = QueryAnimations(group->assets, component->skeletonHash, &seq, &component->skeletonProperties, &flippedByDefault);
    
    if(render && flippedByDefault)
    {
        params->flipOnYAxis = !params->flipOnYAxis;
    }
    
    if(IsValid(animationID))
    {
        result = RenderAnimation_(worldMode, group, animationID, component, params, render);
    }
    
    return result;
}


internal Rect2 GetAnimationDim(GameModeWorld* worldMode, RenderGroup* group, AnimationComponent* component, AnimationParams* params)
{
    Rect2 result = RenderAnimation(worldMode, group, component, params, false);
    return result;
}

internal Rect2 GetAnimationDim(GameModeWorld* worldMode, RenderGroup* group, AssetID ID, AnimationComponent* component, AnimationParams* params)
{
    Rect2 result = RenderAnimation_(worldMode, group, ID, component, params, false);
    return result;
}

#if 0
inline void PlaySoundForAnimation(GameModeWorld* worldMode, Assets* assets, TaxonomySlot* slot, u64 nameHash, r32 oldSoundTime, r32 soundTime)
{
    SoundState* soundState = worldMode->soundState;
    u32 soundTaxonomy = slot->taxonomy;
    b32 found = false;
    while(soundTaxonomy && !found)
    {
        TaxonomySlot* soundSlot = GetSlotForTaxonomy(worldMode->table, soundTaxonomy);
        for(TaxonomySound* sound = slot->firstSound; sound && !found; sound = sound->next)
        {
            if(nameHash == sound->animationNameHash)
            {
                b32 play;
                if(oldSoundTime <= sound->threesold)
                {
                    play = (soundTime > sound->threesold || soundTime < oldSoundTime);
                }
                else
                {
                    play = (soundTime > sound->threesold && soundTime < oldSoundTime);
                }
                
                if(play)
                {
                    u64 eventHash = sound->eventNameHash;
                    
                    SoundEvent* event = GetSoundEvent(worldMode->table, eventHash);
                    if(event)
                    {
                        u32 labelCount = 0;
                        SoundLabel* labels = 0;
                        
                        PickSoundResult pick = PickSoundFromEvent(assets, event, labelCount, labels, &worldMode->table->eventSequence);
                        
                        r32 distanceFromPlayer = 0;
                        for(u32 pickIndex = 0; pickIndex < pick.soundCount; ++pickIndex)
                        {
                            SoundId toPlay = pick.sounds[pickIndex];
                            r32 delay = pick.delays[pickIndex];
                            r32 decibelOffset = pick.decibelOffsets[pickIndex];
                            r32 pitch = pick.pitches[pickIndex];
                            r32 toleranceDistance = pick.toleranceDistance[pickIndex];
                            r32 distanceFalloffCoeff = pick.distanceFalloffCoeff[pickIndex];
                            
                            if(IsValid(toPlay))
                            {
                                PlaySound(worldMode->soundState, assets, toPlay, distanceFromPlayer, decibelOffset, pitch, delay, toleranceDistance, distanceFalloffCoeff);
                                found = true;
                            }
                        }
                    }
                }
            }
        }
        
        soundTaxonomy = GetParentTaxonomy(worldMode->table, soundTaxonomy);
    }
}
#endif


#if 0
JOB_DEFINITION(RenderPlant)
{
    if(!entityC->plant)
    {
        Plant* newPlant = worldMode->firstFreePlant;
        if(!newPlant)
        {
            newPlant = PushStruct(worldMode->persistentPool, Plant);
        }
        else
        {
            worldMode->firstFreePlant = newPlant->nextFree;
        }
        entityC->plant = newPlant;
        
        Plant* plant = entityC->plant;
        *plant = {};
        plant->sequence = Seed((u32)entityC->identifier);
    }
    
    entityC->plant->leafBitmap = FindBitmapByName(group->assets, ASSET_LEAF, slot->plantDefinition->leafParams.bitmapHash);
    
    entityC->plant->flowerBitmap = FindBitmapByName(group->assets, ASSET_FLOWER, slot->plantDefinition->flowerParams.bitmapHash);
    
    entityC->plant->fruitBitmap = FindBitmapByName(group->assets, ASSET_FRUIT, slot->plantDefinition->fruitParams.bitmapHash);
    entityC->plant->trunkBitmap = FindBitmapByName(group->assets, ASSET_TRUNK, slot->plantDefinition->trunkStringHash);
    
    
    PlantRenderingParams renderingParams = {};
    renderingParams.lights = lights;
    renderingParams.modulationWithFocusColor = entityC->modulationWithFocusColor;
    renderingParams.season = worldMode->season;
    renderingParams.lerpWithFollowingSeason = worldMode->seasonLerp;
    
    UpdateAndRenderPlant(worldMode, group, renderingParams, slot->plantDefinition, entityC->plant, animationP);
    
    for(u32 plantIndex = 0; plantIndex < entityC->plant->plant.plantCount; ++plantIndex)
    {
        Vec3 P = animationP + V3(entityC->plant->plant.offsets[plantIndex], 0);
        
        Rect3 plantBounds = Offset(bounds, P);
        entityC->bounds = Union(entityC->bounds, plantBounds);
    }
}

#endif