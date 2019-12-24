internal b32 FinishedSingleCycleAnimation(AnimationComponent* animation)
{
    b32 result = false;
    
    PAKAnimation* info = animation->currentAnimation;
    if(info && info->singleCycle)
    {
        if(RoundReal32ToU32(animation->totalTime * 1000.0f) > info->durationMS)
        {
            result = true;
        }
    }
    
    return result;
}

inline r32 LerpAnglesWithSpin(i32 spin, r32 angle1, r32 angle2, r32 lerp)
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

internal void CalculateFinalBoneAngleAndOffset(Bone* frameBones, i32 countBones, Bone* bone, AnimationParams* params)
{
    bone->computedFinal = true;
    
    Vec2 baseOffset = V2(0, 0);
    Vec2 XAxis;
    Vec2 YAxis;
    
    Assert(bone->parentIndex < countBones);
    if(bone->parentIndex != -1)
    {
        Bone* parent = frameBones + bone->parentIndex;
        if(parent != bone)
        {
            if(!parent->computedFinal)
            {
                CalculateFinalBoneAngleAndOffset(frameBones, countBones, parent, params);
            }
        }
        
        bone->finalZBias += parent->finalZBias;
        bone->finalAngle += parent->finalAngle;
        baseOffset = parent->finalOriginOffset;
        XAxis = parent->mainAxis;
    }
    else
    {
        bone->finalZBias = 0;
        bone->finalAngle += params->angle;
        r32 rad = DegToRad(params->angle);
        XAxis = V2(Cos(rad), Sin(rad));
    }
    YAxis = Perp(XAxis);
    
    r32 totalAngleRad = DegToRad(bone->finalAngle);
    bone->mainAxis = V2(Cos(totalAngleRad), Sin(totalAngleRad)); 
    Vec2 parentOffset = bone->parentOffset * params->scale;
    bone->finalOriginOffset = baseOffset + (parentOffset.x * XAxis + parentOffset.y * YAxis);
}

internal u32 AdvanceAnimationState(PAKAnimation* info, AnimationComponent* animation, r32 elapsedTime, b32 fakeAnimation)
{
    TIMED_FUNCTION();
    if(info != animation->currentAnimation)
    {
        if(!fakeAnimation)
        {
            animation->totalTime = 0;
            animation->time = 0;
            animation->oldTime = 0;
            animation->currentAnimation = info;
        }
    }
    
    animation->totalTime += elapsedTime;
    if(animation->backward)
    {
        elapsedTime = -elapsedTime;
    }
    
    animation->oldTime = animation->time;
    animation->time = animation->oldTime + elapsedTime;
    
    
    i32 oldTimeline = RoundReal32ToI32(animation->oldTime * 1000.0f);
    i32 newTimelineMod = oldTimeline + RoundReal32ToI32(elapsedTime * 1000.0f);
    u32 result = 0;
    
    if(info->pingPongLooping)
    {
        i32 upTarget = (i32) info->durationMS;
        i32 lowTarget = (i32) info->loopingBaselineMS;
        
        b32 changedDirection = false;
        if(!animation->backward)
        {
            Assert(animation->oldTime <= animation->time);
            if(newTimelineMod >= upTarget)
            {
                changedDirection = true;
                result = (u32) upTarget;
            }
        }
        else
        {
            Assert(animation->oldTime >= animation->time);
            if(newTimelineMod <= lowTarget)
            {
                changedDirection = true;
                result = (u32) lowTarget;
            }
        }
        
        if(changedDirection)
        {
            animation->time = animation->oldTime;
            animation->backward = !animation->backward;
        }
        else
        {
            if(newTimelineMod < 0 || newTimelineMod >= upTarget)
            {
                newTimelineMod = 0;
            }
            result = (u32) newTimelineMod;
        }
    }
    else
    {
        animation->backward = false;
        newTimelineMod = Max(newTimelineMod, 0);
        result = (u32) newTimelineMod % info->durationMS;
        if(RoundReal32ToU32(animation->totalTime * 1000.0f) >= info->durationMS)
        {
            u32 old = result;
            result = Max(result, info->loopingBaselineMS);
            Assert(result >= old);
            u32 delta = result - old;
            animation->time += (r32) delta / 1000.0f;
        }
        
    }
    
    return result;
}

internal AnimationPiece* GetAnimationPieces(MemoryPool* tempPool, PAKSkeleton* skeleton, PAKAnimation* animationInfo, Animation* animation,
                                            AnimationComponent* component, AnimationParams* params, u32 animTimeMod, u32* animationPieceCount, b32 render)
{
    TIMED_FUNCTION();
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
    
    if(params->flipOnYAxis)
    {
        Assert(skeleton->flippedBone1 < blended.boneCount);
        Assert(skeleton->flippedBone2 < blended.boneCount);
        
        Bone* bone1 = blended.bones + skeleton->flippedBone1;
        bone1->finalZBias += skeleton->flippedBone1ZOffset;
        
        Bone* bone2 = blended.bones + skeleton->flippedBone2;
        bone2->finalZBias += skeleton->flippedBone2ZOffset;
        
        Bone temp = *bone1;
        *bone1 = *bone2;
        *bone2 = temp;
    }
    
    for(u32 boneIndex = 0; boneIndex < blended.boneCount; boneIndex++)
    {
        Bone* bone = blended.bones + boneIndex;
        CalculateFinalBoneAngleAndOffset(blended.bones, blended.boneCount, bone, params);
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
        dest->originOffset = V3(parentBone->finalOriginOffset + offsetFromBone, zOffset + ass->additionalZOffset + parentBone->finalZBias);
        dest->height = sprite->height;
        
        dest->angle = parentBone->finalAngle + ass->angle;
        dest->scale = ass->scale;
        dest->color = ass->color;
        dest->mainAxis = boneXAxis;
        
        zOffset += 0.01f;
    }
    
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


internal b32 IsValidUsingSlot(GameUIContext* UI, InventorySlot* slot, u16 slotIndex)
{
    b32 result = IsValidID(slot->ID);
    if(result)
    {
        if(AreEqual(slot->ID, UI->draggingIDServer))
        {
            result = false;
            if(UI->testingDraggingOnEquipment)
            {
                UsingEquipOption* test = UI->draggingTestUsingOption;
                if(test)
                {
                    for(u32 i = 0; i < ArrayCount(test->slots); ++i)
                    {
                        if(slotIndex == test->slots[i])
                        {
                            result = true;
                            break;
                        }
                    }
                }
            }
        }
    }
    return result;
}

internal b32 IsValidEquipmentSlot(GameUIContext* UI, InventorySlot* slot, u16 slotIndex)
{
    b32 result = IsValidID(slot->ID);
    if(result)
    {
        if(AreEqual(slot->ID, UI->draggingIDServer))
        {
            result = false;
            if(UI->testingDraggingOnEquipment)
            {
                UsingEquipOption* test = UI->draggingTestEquipOption;
                if(test)
                {
                    for(u32 i = 0; i < ArrayCount(test->slots); ++i)
                    {
                        if(slotIndex == test->slots[i])
                        {
                            result = true;
                            break;
                        }
                    }
                }
            }
        }
    }
    return result;
}

internal b32 IsValidInventorySlot(GameUIContext* UI, InventorySlot* slot)
{
    b32 result = IsValidID(slot->ID);
    if(result && !UI->testingDraggingOnEquipment && AreEqual(slot->ID, UI->draggingIDServer))
    {
        result = false;
    }
    return result;
}

internal b32 IsValidInventorySlot(GameUIContext* UI, InventorySlot* slot, u16 slotIndex, b32 equipmentSlot)
{
    b32 result = equipmentSlot ? IsValidEquipmentSlot(UI, slot, slotIndex) : IsValidUsingSlot(UI, slot, slotIndex);
    return result;
}

internal EntityAnimationParams GetEntityAnimationParams(GameModeWorld* worldMode, EntityID ID);
internal Rect2 RenderLayout(GameModeWorld* worldMode, RenderGroup* group, Vec3 P, ObjectTransform transform, LayoutComponent* layout, u32 seed, Lights lights, struct LayoutContainer* container, r32 elapsedTime);
internal Rect2 RenderLayoutSpecificPiece(GameModeWorld* worldMode, RenderGroup* group, Vec3 P, ObjectTransform transform, LayoutComponent* layout, u32 seed, Lights lights, LayoutContainer* container, r32 elapsedTime, u64 pieceHash);

internal b32 RenderAttachmentPoint(GameModeWorld* worldMode, RenderGroup* group, Vec3 P, u64 hash, ObjectTransform transform, InventorySlot* slots, u32 slotCount, b32* alreadyRendered, Lights lights, b32 equipmentSlots, r32 elapsedTime, u64 multipartHash = 0)
{
    b32 result = false;
    
    b32 multipart = (multipartHash > 0);
    Assert(hash);
    for(u16 slotIndex = 0; slotIndex < slotCount; ++slotIndex)
    {
        if(!alreadyRendered || !alreadyRendered[slotIndex] || multipart)
        {
            InventorySlot* slot = slots + slotIndex;
            if(hash == slot->slotHash || hash == slot->pieceHash)
            {
                result = true;
                
                Vec3 pieceP = P + GetCameraOffset(group, transform.cameraOffset);
                r32 ignored;
                slot->distanceFromMouseSq = LengthSq(ProjectOnScreen(group, pieceP, &ignored) - worldMode->relativeMouseP);
                
                if(IsValidInventorySlot(&worldMode->gameUI, slot, slotIndex, equipmentSlots))
                {
                    EntityID equipmentID = slot->ID;
                    BaseComponent* equipmentBase = GetComponent(worldMode, equipmentID, BaseComponent);
                    InteractionComponent* equipmentInteraction = GetComponent(worldMode, equipmentID, InteractionComponent);
                    LayoutComponent* equipmentLayout = GetComponent(worldMode, equipmentID, LayoutComponent);
                    
                    if(equipmentBase && equipmentLayout)
                    {
                        ObjectTransform finalTransform = transform;
                        finalTransform.angle += equipmentLayout->rootAngle;
                        finalTransform.scale = Hadamart(finalTransform.scale, equipmentLayout->rootScale);
                        
                        EntityAnimationParams params = GetEntityAnimationParams(worldMode, equipmentID);
                        finalTransform.modulationPercentage = params.modulationPercentage;
                        
#if 0                        
                        if(equipmentInteraction && equipmentInteraction->isOnFocus)
                        {
                        }
#endif
                        
                        finalTransform.tint = params.tint;
                        LayoutContainer container = {};
                        container.drawMode = LayoutContainerDraw_Equipped;
                        //container.container = GetComponent(worldMode, equipmentID, ContainerMappingComponent);
                        
                        if(alreadyRendered && !alreadyRendered[slotIndex])
                        {
                            equipmentBase->projectedOnScreen = InvertedInfinityRect2();
                        }
                        
                        if(multipart)
                        {
                            equipmentBase->projectedOnScreen = Union(equipmentBase->projectedOnScreen, RenderLayoutSpecificPiece(worldMode, group, P, finalTransform, equipmentLayout, equipmentBase->seed, lights, &container, elapsedTime, multipartHash));
                        }
                        else
                        {
                            equipmentBase->projectedOnScreen = RenderLayout(worldMode, group, P, finalTransform, equipmentLayout, equipmentBase->seed, lights, &container, elapsedTime);
                        }
                    }
                }
                
                if(alreadyRendered)
                {
                    alreadyRendered[slotIndex] = true;
                }
                
                break;
            }
        }
    }
    
    return result;
}

internal void RenderObjectMappings(GameModeWorld* worldMode, RenderGroup* group,
                                   PAKBitmap* bitmapInfo, BitmapId BID, BitmapDim dim, ObjectTransform transform, InventorySlot* slots, u32 slotCount, b32* alreadyRendered, Lights lights, b32 equipmentSlots, r32 elapsedTime)
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
                attachmentTransform.cameraOffset.z += point->zOffset;
                
                u64 baseHash = pointHash;
                u64 multipartHash = 0;
                
                u32 pound = FindFirstInString(point->name, '#');
                if(pound != 0xffffffff)
                {
                    baseHash = StringHash(point->name, pound);
                    multipartHash = StringHash(point->name + pound + 1);
                    Assert(multipartHash);
                }
                
                RenderAttachmentPoint(worldMode, group, P, baseHash, attachmentTransform, slots, slotCount, alreadyRendered, lights, equipmentSlots, elapsedTime, multipartHash);
            }
            else
            {
                break;
            }
        }
        else
        {
            break;
        }
    }
}

internal void TriggerAnimationSoundEvents(GameModeWorld* worldMode, Assets* assets, PAKAnimation* animation, AssetID animationID, EntityID ID, u32 oldTime, u32 newTime, b32 backward)
{
    for(u32 triggerIndex = 0; triggerIndex < animation->triggerCount; ++triggerIndex)
    {
        PAKAnimationSoundTrigger* trigger = GetAnimationSoundTrigger(assets, animationID, triggerIndex);
        
        if(IsValid(trigger->property))
        {
            if(oldTime > newTime)
            {
                if(backward)
                {
                    if(trigger->timeline <= oldTime && trigger->timeline > newTime)
                    {
                        SoundEventTrigger(worldMode, ID, trigger->property);
                    }
                }
                else
                {
                    if(trigger->timeline < oldTime || trigger->timeline >= newTime)
                    {
                        SoundEventTrigger(worldMode, ID, trigger->property);
                    }
                }
            }
            else
            {
                if(trigger->timeline >= oldTime && trigger->timeline < newTime)
                {
                    SoundEventTrigger(worldMode, ID, trigger->property);
                }
            }
        }
        else
        {
            break;
        }
    }
}

internal EntityID GetClientIDMapping(GameModeWorld* worldMode, EntityID serverID);
internal Rect2 RenderAnimationAndTriggerSounds_(GameModeWorld* worldMode, RenderGroup* group, PAKSkeleton* skeletonInfo, AssetID animationID, AnimationComponent* component, AnimationParams* params, b32 render = true)
{
    TIMED_FUNCTION();
    
    TempMemory temp = BeginTemporaryMemory(worldMode->persistentPool);
    
    b32* usingRendered = 0;
    if(params->equipped)
    {
        usingRendered = PushArray(temp.pool, b32, ArrayCount(params->equipped->slots));
    }
    b32* equipmentRendered = 0;
    if(params->equipment)
    {
        equipmentRendered = PushArray(temp.pool, b32, ArrayCount(params->equipment->slots));
    }
    
    Rect2 result = InvertedInfinityRect2();
    Animation* animation = GetAnimation(group->assets, animationID);
    if(animation)
    {
        PAKAnimation* animationInfo = GetAnimationInfo(group->assets, animationID);
        u16 bitmapCount = 0;
        AssetID* bitmaps = GetAllSkinBitmaps(temp.pool, group->assets, GetAssetSubtype(group->assets, AssetType_Image, component->skinHash), &component->skinProperties, &bitmapCount);
        PAKBitmap** bitmapInfos = PushArray(temp.pool, PAKBitmap*, bitmapCount, NoClear());
        for(u16 bitmapIndex = 0; bitmapIndex < bitmapCount; ++bitmapIndex)
        {
            bitmapInfos[bitmapIndex] = GetBitmapInfo(group->assets, bitmaps[bitmapIndex]);
        }
        
        u32 animTimeMod = 0;
        if(render)
        {
            u32 oldTimeMod = RoundReal32ToU32(component->oldTime * 1000.0f) % animationInfo->durationMS;
            animTimeMod = AdvanceAnimationState(animationInfo, component, params->elapsedTime, params->fakeAnimation);
            if(IsValidID(params->ID))
            {
                TriggerAnimationSoundEvents(worldMode, group->assets, animationInfo, animationID, params->ID, oldTimeMod, animTimeMod, component->backward);
            }
        }
        
        u32 pieceCount;
        AnimationPiece* pieces = GetAnimationPieces(temp.pool, skeletonInfo, animationInfo, animation, component, params, animTimeMod, &pieceCount, render);
        
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
                    
                    if(!RenderAttachmentPoint(worldMode, group, P, piece->nameHash, equippedTransform, params->equipped->slots, ArrayCount(params->equipped->slots), usingRendered, lights, false, params->elapsedTime))
                    {
                        for(u32 equipmentIndex = 0; equipmentIndex < ArrayCount(params->equipment->slots); ++equipmentIndex)
                        {
                            EntityID serverID = params->equipment->slots[equipmentIndex].ID;
                            ContainerComponent* subContainer = GetComponent(worldMode, GetClientIDMapping(worldMode, serverID), ContainerComponent);
                            if(subContainer)
                            {
                                if(RenderAttachmentPoint(worldMode, group, P, piece->nameHash, equippedTransform, subContainer->usingObjects, ArrayCount(subContainer->usingObjects), 0, lights, false, params->elapsedTime))
                                {
                                    break;
                                }
                            }
                        }
                        
                    }
                }
            }
        }
        
        BEGIN_BLOCK("animation pieces");
        for(u32 pieceIndex = 0; pieceIndex < pieceCount; ++pieceIndex)
        {
            AnimationPiece* piece = pieces + pieceIndex;
            if(!piece->placeHolder)
            {
                for(u32 bitmapIndex = 0; bitmapIndex < bitmapCount; ++bitmapIndex)
                {
                    PAKBitmap* bitmapInfo = bitmapInfos[bitmapIndex];
                    if(bitmapInfo->nameHash == piece->nameHash)
                    {
                        AssetID BID = bitmaps[bitmapIndex];
                        Assert(IsValid(BID));
                        
                        Vec3 P = params->P;
                        
                        transform.angle = piece->angle;
                        transform.cameraOffset = piece->originOffset;
                        transform.dissolvePercentages = params->dissolveCoeff * V4(1, 1, 1, 1);
                        transform.tint = Hadamart(piece->color, params->tint);
                        r32 height = piece->height * params->scale;
                        
                        AssetID renderID = BID;
                        b32 replacementFound = false;
                        
                        for(u32 replacementIndex = 0; replacementIndex < params->replacementCount; ++replacementIndex)
                        {
                            AnimationReplacement* replacement = params->replacements + replacementIndex;
                            if(piece->nameHash == replacement->name.hash)
                            {
                                replacementFound = true;
                                for(ArrayCounter repIndex = 0; repIndex < replacement->pieceCount; ++repIndex)
                                {
                                    PieceReplacement* rep = replacement->pieces + repIndex;
                                    
                                    u32 subtype = GetAssetSubtype(group->assets, AssetType_Image, rep->imageType.subtypeHash);
                                    BitmapId subst = QueryBitmaps(group->assets, subtype, 0, 0);
                                    if(IsValid(subst))
                                    {
                                        ObjectTransform repTransform = transform;
                                        
                                        renderID = subst;
                                        PAKBitmap* substitutionImage = GetBitmapInfo(group->assets, renderID);
                                        
                                        r32 nativeHeight = rep->inheritHeight ? piece->height : rep->height;
                                        height = nativeHeight * rep->scale * params->scale;
                                        
                                        repTransform.cameraOffset.xy += rep->offset.x * piece->mainAxis;
                                        repTransform.cameraOffset.xy += rep->offset.y * Perp(piece->mainAxis);
                                        repTransform.cameraOffset.z += rep->offset.z;
                                        
                                        Vec2 pivot = rep->inheritPivot ? piece->pivot : rep->pivot;
                                        BitmapDim dim = PushBitmapWithPivot(group, repTransform, renderID, P, pivot, height, lights);
                                        //result = Union(result, RectMinDim(dim.P.xy, dim.size));
                                    }
                                }
                                
                                break;
                            }
                        }
                        
                        ObjectTransform singleTransform = transform;
                        if(replacementFound)
                        {
                            singleTransform.dontRender = true;
                        }
                        
                        BitmapDim dim = PushBitmapWithPivot(group, singleTransform, BID, P, piece->pivot, height, lights);
                        result = Union(result, RectMinDim(dim.P.xy, dim.size));
                        
                        ObjectTransform equipmentTransform = transform;
                        equipmentTransform.cameraOffset = {};
                        if(render)
                        {
                            if(params->equipment)
                            {
                                RenderObjectMappings(worldMode, group,
                                                     bitmapInfo, BID, dim, equipmentTransform, params->equipment->slots, ArrayCount(params->equipment->slots), equipmentRendered, lights, true, params->elapsedTime);
                            }
                            
                            if(params->equipped)
                            {
                                RenderObjectMappings(worldMode, group,
                                                     bitmapInfo, BID, dim, equipmentTransform, params->equipped->slots, ArrayCount(params->equipped->slots), usingRendered, lights, false, params->elapsedTime);
                            }
                        }
                        
                        break;
                    }
                }
            }
        }
        
        END_BLOCK();
    }
    else
    {
        LoadAnimation(group->assets, animationID);
    }
    
    EndTemporaryMemory(temp);
    return result;
}

internal Rect2 RenderAnimationAndTriggerSounds(GameModeWorld* worldMode, RenderGroup* group, AnimationComponent* component, AnimationParams* params, b32 render = true)
{
    Rect2 result = InvertedInfinityRect2();
    RandomSequence seq = {};
    
    b32 flippedByDefault = false;
    
    PAKSkeleton* skeletonInfo;
    AssetID animationID = QueryAnimations(group->assets, component->skeletonHash, &seq, &component->skeletonProperties, &flippedByDefault, &skeletonInfo);
    
    if(render && flippedByDefault)
    {
        params->flipOnYAxis = !params->flipOnYAxis;
    }
    
    if(IsValid(animationID))
    {
        result = RenderAnimationAndTriggerSounds_(worldMode, group, skeletonInfo, animationID, component, params, render);
    }
    
    return result;
}

internal Rect2 GetAnimationDim(GameModeWorld* worldMode, RenderGroup* group, AnimationComponent* component, AnimationParams* params)
{
    Rect2 result = RenderAnimationAndTriggerSounds(worldMode, group, component, params, false);
    return result;
}

internal Rect2 GetAnimationDim(GameModeWorld* worldMode, RenderGroup* group, AssetID ID, AnimationComponent* component, AnimationParams* params)
{
    Rect2 result = RenderAnimationAndTriggerSounds_(worldMode, group, 0, ID, component, params, false);
    return result;
}