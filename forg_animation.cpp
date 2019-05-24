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
    Assert(b1->parentID == b2->parentID);
    Bone result = {};
    
    result.id = b1->id;
    result.parentID = b1->parentID;
    
    result.parentAngle = LerpAnglesWithSpin(b1->spin, b1->parentAngle, b2->parentAngle, lerp);
    result.parentOffset = Lerp(b1->parentOffset, lerp, b2->parentOffset);
    result.finalAngle = result.parentAngle;
    
    return result;
}

internal PieceAss BlendAss_(PieceAss* p1, r32 lerp, PieceAss* p2)
{
    TIMED_FUNCTION();
    Assert(p1->boneID == p2->boneID);
    PieceAss result = {};
    
    result.spriteIndex = lerp <= 0.5f ? p1->spriteIndex : p2->spriteIndex;
    result.boneID = p1->boneID;
    result.alpha = Lerp(p1->alpha, lerp, p2->alpha);
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

internal void BlendFrames_(MemoryPool* tempPool, Animation* animation, BlendResult* in, u32 lowerFrameIndex, u32 timelineMS, u32 upperFrameIndex)
{
    FrameData* reference = animation->frames + 0;
    
    in->boneCount = reference->countBones;
    in->bones = PushArray(tempPool, BlendedBone, in->boneCount);
    in->assCount = reference->countAss;
    in->ass = PushArray(tempPool, BlendedAss, in->assCount);
    
    Bone* firstRefBone = animation->bones + reference->firstBoneIndex;
    for(u32 boneIndex = 0; boneIndex < in->boneCount; boneIndex++)
    {
        u32 lowerTimeLine = reference->timelineMS;
        u32 upperTimeLine = reference->timelineMS + animation->header->durationMS;
        
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
        
        BlendedBone* dest = in->bones + boneIndex;
        dest->bone = BlendBones_(b1, lerp, b2);
        dest->alterations = {};
    }
    
    
    PieceAss* firstRefAss = animation->ass + reference->firstAssIndex;
    for(u32 assIndex = 0; assIndex < in->assCount; assIndex++)
    {
        u32 lowerTimeLine = reference->timelineMS;
        u32 upperTimeLine = reference->timelineMS + animation->header->durationMS;
        
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
        
        BlendedAss* dest = in->ass + assIndex;
        dest->ass = BlendAss_(a1, lerp, a2);
        dest->equipmentAssCount = 0;
        dest->alterations = {};
        
        Assert(dest->ass.spriteIndex < animation->spriteInfoCount);
        
        SpriteInfo* sprite = animation->spriteInfos + dest->ass.spriteIndex;
        dest->sprite = *sprite;
    }
}

internal Vec2 CalculateFinalBoneOffset_(AnimationFixedParams* input, BlendedBone* frameBones, i32 countBones, Bone* bone, AnimationVolatileParams* params)
{
    Assert(bone->parentID < countBones);
    Vec2 baseOffset = V2(0, 0);
    
    Vec2 XAxis;
    Vec2 YAxis;
    
    Vec2 offsetFromParentScale = V2(1, 1);
    if(bone->parentID >= 0)
    {
        Bone* parent = &frameBones[bone->parentID].bone;
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
    
    Vec2 parentOffset = Hadamart(Hadamart(bone->parentOffset, offsetFromParentScale), params->scale);
    Vec2 result = baseOffset + (parentOffset.x * XAxis + parentOffset.y * YAxis);
    return result;
}


inline Vec3 AssOriginOffset(Bone* parentBone, PieceAss* ass, r32 zOffset, Vec2 scale)
{
    Vec2 boneXAxis = parentBone->mainAxis;
    Vec2 boneYAxis = Perp(boneXAxis);
    
    Vec2 boneOffset = Hadamart(ass->boneOffset, scale);
    Vec2 offsetFromBone = boneOffset.x * boneXAxis + boneOffset.y * boneYAxis;
    Vec3 originOffset = V3(parentBone->finalOriginOffset + offsetFromBone, zOffset + ass->additionalZOffset);
    
    return originOffset;
}

inline b32 RequiresSync(EntityAction action)
{
    b32 result = (action == Action_Cast);
    return result;
}

internal void GetAnimationPiecesAndAdvanceState(AnimationFixedParams* input, BlendResult* blended, Animation* animation, AnimationState* state, r32 elapsedTime, AnimationVolatileParams* params)
{
    AnimationHeader* header = animation->header;
    
    r32 oldTime = state->totalTime;
    r32 newTime = oldTime + elapsedTime;
    
    if(state->waitingForSync)
    {
        u32 oldTimeline = (u32) (oldTime * 1000.0f) % header->durationMS;
        u32 newTimeline = (u32) (newTime * 1000.0f) & header->durationMS;
        
        u32 syncThreesold = header->syncThreesoldMS;
        if(oldTimeline <= syncThreesold && newTimeline > syncThreesold)
        {
            state->waitingForSyncTimer += elapsedTime;
        }
        else
        {
            state->totalTime = newTime;
        }
    }
    else
    {
        state->totalTime = newTime;
    }
    
    
    u32 timeline = (u32) (state->totalTime * 1000.0f);
    
	u32 animTimeMod;
	if(input->debug.ortho)
	{
        Assert(Normalized(input->debug.modTime));
		animTimeMod = (u32) (input->debug.modTime * header->durationMS);
	}
	else
	{
		animTimeMod = timeline % header->durationMS;
	}
    state->normalizedTime = (r32) animTimeMod / (r32) header->durationMS;
    
    
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
    
    
    
    BlendFrames_(input->tempPool, animation, blended, lowerFrameIndex, animTimeMod, upperFrameIndex);
    
    
    
    TaxonomySlot* slot = GetSlotForTaxonomy(input->taxTable, input->entity->taxonomy);
    for(TaxonomyBoneAlterations* boneAlt = slot->firstBoneAlteration; boneAlt; boneAlt = boneAlt->next)
    {
        if(boneAlt->boneIndex < blended->boneCount)
        {
            BlendedBone* bone = blended->bones + boneAlt->boneIndex;
            bone->alterations = boneAlt->alt;
        }
    }
    
    for(TaxonomyAssAlterations* assAlt = slot->firstAssAlteration; assAlt; assAlt = assAlt->next)
    {
        if(assAlt->assIndex < blended->assCount)
        {
            BlendedAss* ass = blended->ass + assAlt->assIndex;
            ass->alterations = assAlt->alt;
        }
    }
    
    for(u32 boneIndex = 0; boneIndex < blended->boneCount; boneIndex++)
    {
        BlendedBone* blendedBone = blended->bones + boneIndex;
        
        Bone* bone = &blendedBone->bone;
        BoneAlterations* boneAlt = &blendedBone->alterations;
        
        if(bone->parentID != -1)
        {
            BlendedBone* parentBlended = blended->bones + bone->parentID;
            Bone* parent = &parentBlended->bone;
            bone->finalAngle += parent->finalAngle;
        }
        else
        {
            bone->finalAngle += params->angle;
        }
        r32 totalAngleRad = DegToRad(bone->finalAngle);
        bone->finalOriginOffset = CalculateFinalBoneOffset_(input, blended->bones, blended->boneCount, bone, params);
        
        Vec2 scale = V2(1.0f, 1.0f);
        if(boneAlt->valid)
        {
            scale = boneAlt->scale;
        }
        bone->mainAxis = Hadamart(scale, V2(Cos(totalAngleRad), Sin(totalAngleRad))); 
    }
    
    
    
    r32 waitingForSyncThreesold = 0.8f;
    
    if(timeline >= header->durationMS || (state->stopAtNextBarrier && !RequiresSync((EntityAction) state->action)) || state->waitingForSyncTimer >= waitingForSyncThreesold)
    {
        state->stopAtNextBarrier = false;
        state->totalTime = 0;
        state->waitingForSync = false;
        state->waitingForSyncTimer = 0;
        state->action = state->nextAction;
        state->actionSyncronized = Action_None;
        
        if(RequiresSync((EntityAction) state->action))
        {
            state->waitingForSync = true;
        }
    }
    
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

inline r32 AssFinalAngle(Bone* parentBone, PieceAss* ass)
{
    r32 finalAngle = parentBone->finalAngle + ass->angle;
    return finalAngle;
}

inline PieceAss* GetAss(BlendResult* blended, u32 assIndex)
{
    Assert(assIndex < blended->assCount);
    BlendedAss* ass = blended->ass + assIndex;
    PieceAss* result = &ass->ass;
    
    return result;
}

inline Bone* GetBone(BlendResult* blended, u32 boneIndex)
{
    Assert(boneIndex < blended->boneCount);
    BlendedBone* bone = blended->bones + boneIndex;
    Bone* result = &bone->bone;
    
    return result;
}

inline SpriteInfo* GetSprite(BlendResult* blended, u32 spriteIndex)
{
    Assert(spriteIndex < blended->assCount);
    BlendedAss* ass = blended->ass + spriteIndex;
    SpriteInfo* result = &ass->sprite;
    
    return result;
}

internal void GetEquipmentPieces(BlendResult* blended, TaxonomyTable* table, TaxonomyTree* equipmentMappings, EquipmentAnimationSlot* slots)
{
    u64 inserted[Slot_Count] = {};
    for(u32 slotIndex = 0; slotIndex < Slot_Count; ++slotIndex)
    {
        EquipmentAnimationSlot* slot = slots + slotIndex;
        b32 alreadyInserted = false;
        for(u32 slotIndexTest = 0; slotIndexTest < Slot_Count; ++slotIndexTest)
        {
            if(inserted[slotIndexTest] == slot->ID)
            {
                alreadyInserted = true;
                break;
            }
        }
        
        if(!alreadyInserted)
        {
            inserted[slotIndex] = slot->ID;
            
            ObjectState state = slot->isOpen ? ObjectState_Open : ObjectState_Default;
            
            TaxonomyNode* node = FindInTaxonomyTree(table, equipmentMappings->root, slot->taxonomy);
            if(node && node->data.equipmentMapping)
            {
                ObjectLayout* reference = slot->layout;
                
                for(EquipmentLayout* layout = node->data.equipmentMapping->firstEquipmentLayout; layout; layout = layout->next)
                {
                    if(layout->layoutHashID == reference->nameHashID)
                    {
                        for(LayoutPiece* piece = reference->firstPiece; piece; piece = piece->next)
                        {
                            LayoutPieceParams* params = GetParams(piece, state);
                            u64 componentHashID = piece->componentHashID;
                            u8 index = piece->index;
                            b32 decorativePiece = false;
                            
							if(piece->parent)
							{
                                componentHashID = piece->parent->componentHashID;
								index = piece->parent->index;
                                decorativePiece = true;
                            }
                            
                            for(EquipmentAss* eq = layout->firstEquipmentAss; eq; eq = eq->next)
                            {
                                u32 assIndex = eq->assIndex;
                                BlendedAss* blendedAss = blended->ass + assIndex;
                                PieceAss* ass = &blendedAss->ass;
                                Bone* parentBone = GetBone(blended, ass->boneID);
                                
                                if(eq->index == 0xff || (eq->stringHashID == componentHashID && eq->index == index))
                                {
                                    Assert(blendedAss->equipmentAssCount < ArrayCount(blendedAss->associatedEquipment));
                                    
                                    EquipmentAnimationPiece* dest = blendedAss->associatedEquipment + blendedAss->equipmentAssCount++;
                                    
                                    PieceAss* destAss = &dest->ass;
                                    SpriteInfo* destSprite = &dest->sprite;
                                    
                                    *destAss = {};
                                    *destSprite = {};
                                    
                                    
                                    destAss->additionalZOffset = eq->zOffset;
                                    destAss->angle = ass->angle + eq->angle;
                                    destAss->scale = Hadamart(ass->scale, eq->scale);
                                    
                                    r32 zOffset = 0;
                                    r32 angle = 0;
                                    Vec2 scale = V2(1, 1);
                                    Vec2 offset = {};
                                    
                                    if(eq->index == 0xff)
                                    {
                                        zOffset = params->parentOffset.z;
                                        angle = params->parentAngle;
                                        scale = params->scale;
                                        offset = params->parentOffset.xy;
                                        
                                        if(decorativePiece)
                                        {
                                            LayoutPieceParams* parentParams = GetParams(piece->parent, state);
                                            zOffset += parentParams->parentOffset.z;
                                            angle += parentParams->parentAngle;
                                            offset += parentParams->parentOffset.xy;
                                        }
                                    }
                                    else if(decorativePiece)
                                    {
                                        zOffset = params->parentOffset.z;
                                        angle = params->parentAngle;
                                        scale = params->scale;
                                        offset = params->parentOffset.xy;
                                    }
                                    
                                    
                                    destAss->additionalZOffset += zOffset;
                                    destAss->angle += angle;
                                    destAss->scale = Hadamart(destAss->scale, scale);
                                    
                                    
                                    r32 finalAngle = DegToRad(AssFinalAngle(parentBone, ass) + eq->angle + angle);
                                    Vec2 layoutX = V2(Cos(finalAngle), Sin(finalAngle));
                                    Vec2 layoutY = Perp(layoutX);
                                    layoutX *= eq->scale.x;
                                    layoutY *= eq->scale.y;
                                    Vec2 layoutOffset =  offset.x * layoutX + offset.y * layoutY;
                                    
                                    destAss->boneOffset = ass->boneOffset + GetBoneAxisOffsetfromXY(parentBone, eq->assOffset + layoutOffset);
                                    
                                    
                                    
                                    
                                    destAss->alpha = 1.0f;
                                    
                                    destSprite->pivot = params->pivot;
                                    destSprite->stringHashID = piece->componentHashID;
                                    destSprite->index = piece->index;
                                    destSprite->flags = 0;
                                    
                                    dest->status = slot->status;
                                    dest->properties = &slot->properties;
                                    dest->slot = slot->slot;
                                    dest->drawModulated = slot->drawModulated;
                                }
                            }
                        }
                        break;
                    }
                }
            }
        }
    }
}


inline void PushNewAction(AnimationState* animation, u32 action)
{
    if(action != animation->action)
    {
        animation->stopAtNextBarrier = true;
        animation->nextAction = action;
    }
}


internal void GetVisualProperties(ComponentsProperties* dest, TaxonomyTable* table, u32 taxonomy, GenerationData gen)
{
    dest->componentCount = 0;
    TaxonomySlot* slot = GetSlotForTaxonomy(table, taxonomy);
    
    ObjectLayout* layout = GetLayout(table, taxonomy);
    if(layout)
    {
        RandomSequence seq = Seed(gen.ingredientSeed);
        for(LayoutPiece* piece = layout->firstPiece; piece; piece = piece->next)
        {
            Assert(dest->componentCount < ArrayCount(dest->components));
            VisualComponent* visualComponent = dest->components + dest->componentCount++;
            
            visualComponent->stringHashID = piece->componentHashID;
			visualComponent->index = piece->index;
            visualComponent->labelCount = 0;
            
            LayoutPiece* visualPiece = piece;
            if(piece->parent)
            {
                visualPiece = piece->parent;
            }
            
            for(u32 ingredientIndex = 0; ingredientIndex < visualPiece->ingredientCount; ++ingredientIndex)
            {
                u32 ingredientTaxonomy = piece->ingredientTaxonomies[ingredientIndex];
                u32 choosenTaxonomy = GetRandomChild(table, &seq, ingredientTaxonomy);
                TaxonomySlot* ingredientSlot = GetSlotForTaxonomy(table, choosenTaxonomy);
                
                for(VisualLabel* label = ingredientSlot->firstVisualLabel; label; label = label->next)
                {
                    Assert(visualComponent->labelCount < ArrayCount(visualComponent->labels));
                    visualComponent->labels[visualComponent->labelCount++] = *label;
                }
            }
        }
    }
}

inline BitmapId GetBitmapID(RenderGroup* group, SpriteInfo* sprite, u64 skeletonHashID, u64 skinHashID, ComponentsProperties* properties)
{
    BitmapId result = {};
    
    TagVector match = {};
    TagVector weight = {};
    
    match.E[Tag_skinFirstHalf] = (r32) (skinHashID >> 32);
    match.E[Tag_skinSecondHalf] = (r32) (skinHashID & 0xFFFFFFFF);
    weight.E[Tag_skinFirstHalf] = 10.0f;
    weight.E[Tag_skinSecondHalf] = 10.0f;
    
    
    match.E[Tag_skeletonFirstHalf] = (r32) (skeletonHashID >> 32);
    match.E[Tag_skeletonSecondHalf] = (r32) (skeletonHashID & 0xFFFFFFFF);
    weight.E[Tag_skeletonFirstHalf] = 100.0f;
    weight.E[Tag_skeletonSecondHalf] = 100.0f;
    
    
    u32 assetIndex = Asset_count + (sprite->stringHashID & (HASHED_ASSET_SLOTS - 1));
    
    if(properties && properties->componentCount)
    {
        LabelVector labels;
        labels.labelCount = 0;
        
        for(u32 componentIndex = 0; componentIndex < properties->componentCount; ++componentIndex)
        {
            VisualComponent* component = properties->components + componentIndex;
            if(component->stringHashID == sprite->stringHashID && component->index == sprite->index)
            {
                for(u32 labelIndex = 0; labelIndex < component->labelCount; ++labelIndex)
                {
                    if(labels.labelCount < ArrayCount(labels.IDs))
                    {
                        VisualLabel* label = component->labels + labelIndex;
                        u32 labelI = labels.labelCount++;
                        labels.IDs[labelI] = label->ID;
                        labels.values[labelI] = label->value;
                    }
                }
            }
        }
        result = GetMatchingBitmapHashed(group->assets, sprite->stringHashID, &match, &weight, &labels);
        
    }
    else
    {
        result = GetMatchingBitmapHashed(group->assets, sprite->stringHashID, &match, &weight, 0);
    }
    
    return result;
}

inline void GetLayoutPieces(AnimationFixedParams* input, BlendResult* output, ObjectLayout* layout, AnimationVolatileParams* params, ObjectState state)
{
    output->boneCount = 1;
    output->bones = PushArray(input->tempPool, BlendedBone, output->boneCount);
    
    output->assCount = layout->pieceCount;
    output->ass = PushArray(input->tempPool, BlendedAss, output->assCount);
    
    BlendedBone* bone = output->bones + 0;
    bone->bone.mainAxis = V2(1, 0);
    bone->bone.parentID = -1;
    
    u32 pieceIndex = 0;
    for(LayoutPiece* source = layout->firstPiece; source; source = source->next)
    {
        LayoutPieceParams* pieceParams = GetParams(source, state);
        
        BlendedAss* ass = output->ass + pieceIndex++;
        PieceAss* destAss = &ass->ass;
        
        ass->equipmentAssCount = 0;
        
        AssAlterations* destAlt = &ass->alterations;
        destAlt->valid = false;
        
        SpriteInfo* destSprite = &ass->sprite;
        
        destAss->spriteIndex = pieceIndex;
        destAss->boneOffset = pieceParams->parentOffset.xy;
        destAss->additionalZOffset = pieceParams->parentOffset.z;
        destAss->angle = pieceParams->parentAngle;
        
        if(source->parent)
        {
            LayoutPieceParams* parentParams = GetParams(source->parent, state);
            destAss->boneOffset += parentParams->parentOffset.xy;
            destAss->additionalZOffset += parentParams->parentOffset.z;
            destAss->angle += parentParams->parentAngle;
        }
        
        destAss->scale = pieceParams->scale;
        destAss->alpha = pieceParams->alpha;
        
        destSprite->pivot = pieceParams->pivot;
        destSprite->stringHashID = source->componentHashID;
        destSprite->index = source->index;
        destSprite->flags = 0;
    }
}

inline Rect2 GetBitmapRect(Bitmap* bitmap, Vec2 pivot, Vec3 originOffset, r32 angle, b32 flipOnYAxis, Vec2 scale)
{
    r32 angleRad = DegToRad(angle);
    Vec3 XAxis = V3(Cos(angleRad), Sin(angleRad), 0.0f);
    Vec3 YAxis  = V3(Perp(XAxis.xy), 0.0f);
    
    if(flipOnYAxis)
    {
        originOffset.x = -originOffset.x;
    }
    
    BitmapDim dim = GetBitmapDim(bitmap, pivot, originOffset, XAxis, YAxis, bitmap->nativeHeight, scale);
    Rect2 bitmapRect = RectMinMax(dim.P.xy, dim.P.xy + (dim.size.x * dim.XAxis.xy) + (dim.size.y * dim.YAxis.xy));
    return bitmapRect;
}

inline Rect2 GetPiecesBound(RenderGroup* group, BlendResult* blended, AnimationVolatileParams* params)
{
    Rect2 result = InvertedInfinityRect2();
    for(u32 assIndex = 0; assIndex < blended->assCount; ++assIndex)
    {
        PieceAss* ass = GetAss(blended, assIndex);
        Bone* parentBone = GetBone(blended, + ass->boneID);
        SpriteInfo* sprite = GetSprite(blended, assIndex);
        
        Vec3 originOffset = AssOriginOffset(parentBone, ass, params->zOffset, params->scale);
        r32 finalAngle = AssFinalAngle(parentBone, ass);
        
        if(!(sprite->flags & Sprite_Composed))
        {	        
            BitmapId BID = GetBitmapID(group, sprite, params->skeletonHashID, params->skinHashID, params->properties);
            Vec2 pivot = sprite->pivot;
            
            if(IsValid(BID))
            {
                Bitmap* bitmap = GetBitmap(group->assets, BID);
                if(bitmap)
                {
                    Rect2 bitmapRect = GetBitmapRect(bitmap, pivot, originOffset, finalAngle, params->flipOnYAxis, Hadamart(ass->scale, params->scale));
                    result = Union(result, bitmapRect);
                    
                }
                else
                {
                    LoadBitmap(group->assets, BID, false);
                }
            }
        }
        else
        {
            //TODO(leonardo): get the rect of the sub animation and marge it with the parent one
        }
    }
    
    return result;
}

inline Rect2 GetLayoutBounds(AnimationFixedParams* input, RenderGroup* group, ObjectLayout* layout, AnimationVolatileParams* params, ObjectState state)
{
    BlendResult blended;
    GetLayoutPieces(input, &blended, layout, params, state);
    Rect2 result = GetPiecesBound(group, &blended, params);
    
    return result;
}


internal void RenderObjectLayout(AnimationFixedParams* input, RenderGroup* group, ObjectLayout* layout, Vec3 P, AnimationVolatileParams* params, ObjectState state);

inline b32 DrawModularPiece(AnimationFixedParams* input, RenderGroup* group, Vec3 P, u32 pieceTaxonomy, AnimationVolatileParams* params, ObjectState state, b32 dontRender)
{
    b32 result = false;
    ObjectLayout* layout = GetLayout(input->taxTable, pieceTaxonomy);
    if(layout)
    {
        Vec3 offset = params->cameraOffset;
        if(params->flipOnYAxis)
        {
            offset.x = -offset.x;
        }
        Vec3 offsetRealP = offset.x * group->gameCamera.X + offset.y * group->gameCamera.Y + offset.z * group->gameCamera.Z;
        
        Vec3 groundP = P + ProjectOnGround(offsetRealP, group->gameCamera.P);
        r32 distanceSq = LengthSq(groundP - input->mousePOnGround);
        if(distanceSq < input->minFocusSlotDistanceSq)
        {
            result = true;
            input->minFocusSlotDistanceSq = distanceSq;
        }
        
        if(!dontRender)
        {
            AnimationState pieceState = {};
            RenderObjectLayout(input, group, layout, P, params, state);
        }
    }
    
    return result;
}

inline void DispatchAnimationEffect(GameModeWorld* worldMode, AnimationEffect* effect, ClientEntity* entity, Vec3 P,Vec4* colorIn, r32 timeToAdvance)
{
    ParticleCache* particleCache = worldMode->particleCache;
    switch(effect->type)
    {
        case AnimationEffect_ChangeColor:
        {
            if(effect->triggerAction == entity->effectReferenceAction)
            {
                effect->timer += timeToAdvance;
                effect->timer = Min(effect->timer, effect->fadeTime);
            }
            else
            {
                effect->timer -= timeToAdvance;
            }
            
            r32 effectPower = Clamp01(effect->timer / effect->fadeTime);
            Vec4 effectColor = Lerp(V4(1, 1, 1, 1), effectPower, effect->color);
            *colorIn = Hadamart(*colorIn, effectColor);
        } break;
        
        case AnimationEffect_SpawnParticles:
        {
            SpawnFluidParticles(particleCache, effect->particleType, P);
        } break;
        
        case AnimationEffect_SpawnAshesTowardEntity:
        {
            u64 targetID = effect->targetID;
            ClientEntity* target = GetEntityClient(worldMode, targetID);
            
            if(target)
            {
                SpawnAshFromSourceToDest(particleCache, P, target->P, effect->color, 1, effect->dim, effect->timeToArriveAtDest);
            }
            
            effect->timer += timeToAdvance;
            if(effect->timer >= effect->targetTimer)
            {
                effect->type = AnimationEffect_None;
            }
        } break;
    }
}


internal void UpdateAnimationEffects(GameModeWorld* worldMode, ClientEntity* entityC, r32 timeToAdvance)
{
    u32 newAction = entityC->action;
    if(newAction != entityC->effectReferenceAction ||
       (!newAction && !entityC->firstActiveEffect))
    {
        for(AnimationEffect** effectPtr = &entityC->firstActiveEffect; *effectPtr;)
        {
            AnimationEffect* effect = *effectPtr;
            if(!effect->type ||
               !(effect->flags & AnimationEffect_AllActions) && (effect->triggerAction == newAction))
            {
                *effectPtr = effect->next;
                FREELIST_DEALLOC(effect, worldMode->firstFreeEffect);
            }
            else
            {
                effectPtr = &effect->next;
            }
        }
        
        
        b32 found = false;
        u32 currentTaxonomy = entityC->taxonomy;
        while(currentTaxonomy && !found)
        {
            TaxonomySlot* slot = GetSlotForTaxonomy(worldMode->table, currentTaxonomy);
            for(AnimationEffect* effect = slot->firstAnimationEffect; effect; effect = effect->next)
            {
                if(effect->triggerAction == newAction)
                {
                    AnimationEffect* newEffect;
                    FREELIST_ALLOC(newEffect, worldMode->firstFreeEffect, PushStruct(&worldMode->entityPool, AnimationEffect, NoClear()));
                    
                    *newEffect = *effect;
                    FREELIST_INSERT(newEffect, entityC->firstActiveEffect);
                    found = true;
                    break;
                }
            }
            currentTaxonomy = GetParentTaxonomy(worldMode->table, currentTaxonomy);
        }
        
        entityC->effectReferenceAction = newAction;
    }
}

inline r32 ArrangeObjects(u8 gridDimX,u8 gridDimY, Vec3 originalGridDim)
{
    r32 result = Min(originalGridDim.x / gridDimX, originalGridDim.y / gridDimY);
    return result;
}

struct RenderAssResult
{
    b32 onFocus;
    b32 screenInRect;
	r32 distanceFromAssCenter;
};

inline RenderAssResult RenderPieceAss_(AnimationFixedParams* input, RenderGroup* group, Vec3 P, SpriteInfo* sprite, Bone* parentBone, PieceAss* ass, AnimationVolatileParams* params, b32 dontRender, b32 isEquipmentAss, Vec4 proceduralColor = V4(1, 1, 1, 1))
{
    RenderAssResult result = {};
    
    AnimationVolatileParams pieceParams = *params;
    pieceParams.color = Hadamart(pieceParams.color, proceduralColor);
    
    Vec3 originOffset = AssOriginOffset(parentBone, ass, pieceParams.zOffset, pieceParams.scale);
    r32 finalAngle = AssFinalAngle(parentBone, ass);
    
    pieceParams.angle = finalAngle;
    pieceParams.cameraOffset += originOffset;
    pieceParams.skinHashID = sprite->stringHashID;
    
    
    u64 emptySpaceHashID = StringHash("emptySpace");
    
    if(sprite->flags & Sprite_Entity)
    {
        input->output->entityPresent = true;
        Vec3 offset = pieceParams.cameraOffset;
        input->output->entityOffset = offset;
        input->output->entityAngle = pieceParams.angle;
    }
    else
    {
        BitmapId BID = GetBitmapID(group, sprite, params->skeletonHashID, params->skinHashID, params->properties);
        if(sprite->flags & Sprite_Composed)
        {
#if 0            
            InvalidCodePath;
            pieceParams.scale = Hadamart(pieceParams.scale, ass->scale);
            pieceParams.color.a *= ass->alpha;
            
            ComponentsProperties pieceProperties;
            pieceProperties.componentCount = 0;
            pieceParams.properties = &pieceProperties;
            pieceParams.entityHashID = sprite->stringHashID;
            
            if(!input->ortho)
            {
                if(slot->drawOpened)
                {
                    pieceParams.drawEmptySpaces = false;
                }
                GetVisualProperties(&pieceProperties, input->taxTable, slot->taxonomy, slot->recipeIndex, false, slot->drawOpened);
                result.onFocus = DrawModularPiece(input, group, P, slot->taxonomy, &pieceParams, false, slot->drawOpened, dontRender);
            }
            else
            {
                ShortcutSlot* shortcut = GetShortcut(input->taxTable, sprite->stringHashID);
                if(shortcut)
                {
                    DrawModularPiece(input, group, P, shortcut->taxonomy, &pieceParams, false, false, false);
                }
            }
#endif
        }
        else if(sprite->stringHashID == emptySpaceHashID)
        {
            Assert(IsValid(BID));
            if(params->drawEmptySpaces)
            {
                Vec2 finalScale = Hadamart(pieceParams.scale, ass->scale);
                Bitmap* bitmap = GetBitmap(group->assets, BID);
                
                if(bitmap)
                {
                    r32 zOffset = 0.0f;
                    
                    Vec3 originalGridDim = V3(Hadamart(finalScale, V2(bitmap->widthOverHeight * bitmap->nativeHeight, bitmap->nativeHeight)), 0);
                    
                    u8 gridDimX = input->objectGridDimX;
                    u8 gridDimY = input->objectGridDimY;
                    r32 cellDim =ArrangeObjects(gridDimX, gridDimY, originalGridDim);
                    
                    r32 zoomCoeff = 1.0f / cellDim;
                    input->output->additionalZoomCoeff = Max(input->output->additionalZoomCoeff, zoomCoeff);
                    
                    
                    Vec3 gridDim = cellDim * V3(gridDimX, gridDimY, 0);
                    Vec3 halfGridDim = 0.5f * gridDim;
                    Vec3 lowLeftCorner = pieceParams.cameraOffset - halfGridDim + 0.5f * V3(cellDim, cellDim, 0);
                    
                    
                    if(input->debug.ortho)
                    {
                        ObjectTransform debugCell = FlatTransform();
                        debugCell.additionalZBias = params->additionalZbias;
                        Rect2 cellDebugRect = RectCenterDim(P.xy + pieceParams.cameraOffset.xy, originalGridDim.xy);
                        PushRect(group, debugCell, cellDebugRect, V4(0, 0, 0, 0.7f));
                    }
                    
                    for(u8 Y = 0; Y < gridDimY; ++Y)
                    {
                        for(u8 X = 0; X < gridDimX; ++X)
                        {
                            i32 objectIndex = 0; 
                            Object* object = input->objects + objectIndex;
                            
                            Vec3 objectP = lowLeftCorner + cellDim * V3(X, Y, 0);
                            r32 objectScale = 1.0f;
                            Vec4 objectColor = V4(1, 1, 1, 1);
                            if(object->status < 0)
                            {
                                objectColor = V4(0.5f, 0.5f, 0.5f, 0.5f);
                            }
                            else
                            {
                                r32 ratio = Clamp01MapToRange(0, (r32) object->status, (r32) I16_MAX);
                                Vec4 statusColor = Lerp(bodyDead, ratio, V4(1, 1, 1, 1));
                                objectColor = statusColor;
                            }
                            
                            Assert(P.z == 0.0f);
                            Rect2 cellRect = ProjectOnGround(group, P, objectP, V3(cellDim, cellDim, 0));
                            
                            if(input->debug.ortho)
                            {
                                ObjectTransform debugCell = FlatTransform();
                                debugCell.additionalZBias = params->additionalZbias;
                                Rect2 cellDebugRect = RectCenterDim(P.xy + objectP.xy, V2(cellDim, cellDim));
                                PushRect(group, debugCell, cellDebugRect, V4(1, 1, 1, 0.7f));
                            }
                            else
                            {
                                Vec4 cellColor = V4(pieceParams.color.rgb, 0.5f);
                                if(PointInRect(cellRect, input->mousePOnGround.xy))
                                {
                                    if(!input->draggingEntity->taxonomy || input->draggingEntity->objects.objectCount == 0)
                                    {
                                        input->output->focusObjectIndex = objectIndex;
                                        if(!input->draggingEntity)
                                        {
                                            objectScale = 2.5f;
                                        }
                                        zOffset = 0.01f;
                                        
                                        if(input->draggingEntity->objects.objectCount == 0)
                                        {
                                            cellColor.a = 0.8f;
                                        }
                                    }
                                }
                                
                                ObjectTransform spaceTransform = UprightTransform();
                                spaceTransform.additionalZBias += params->additionalZbias;
                                spaceTransform.cameraOffset = objectP;
                                
                                PushRect(group, spaceTransform, P, V2(cellDim, cellDim), cellColor, pieceParams.lightIndexes);
                            }
                            
                            pieceParams.cameraOffset = objectP;
                            
                            u32 taxonomy = object->taxonomy;
                            GenerationData gen = object->gen;
                            
                            if(taxonomy)
                            {
                                if(IsRecipe(object))
                                {
                                    taxonomy = input->taxTable->recipeTaxonomy;
                                    gen = RecipeGenerationData();
                                }
                                TaxonomySlot* slot = GetSlotForTaxonomy(input->taxTable, taxonomy);
                                AnimationVolatileParams objectParams = pieceParams;
                                objectParams.skinHashID = slot->stringHashID;
                                
                                ObjectLayout* layout = GetLayout(input->taxTable, taxonomy);
                                ObjectState objectState = ObjectState_Default;
                                
                                Rect2 animationBounds = GetLayoutBounds(input, group, layout, &objectParams, objectState);
                                if(HasArea(animationBounds))
                                {
                                    r32 cellFillPercentage = 0.9f;
                                    Vec2 boundsDim = GetDim(animationBounds);
                                    r32 scaleX = cellDim / boundsDim.x * cellFillPercentage;
                                    r32 scaleY = cellDim / boundsDim.y * cellFillPercentage;
                                    
                                    r32 cellScale = Min(scaleX, scaleY);
                                    objectParams.cameraOffset -= cellScale * V3(GetCenter(animationBounds), 0);
                                    objectParams.scale *= Min(scaleX, scaleY);
                                    objectParams.scale *= objectScale;
                                    objectParams.color = objectColor;
                                    objectParams.zOffset += zOffset;
                                    objectParams.additionalZbias += 0.01f;
                                    
                                    ComponentsProperties objectProperties;
                                    objectParams.properties = &objectProperties;
                                    GetVisualProperties(&objectProperties, input->taxTable, taxonomy, gen);
                                    
                                    DrawModularPiece(input, group, P, slot->taxonomy, &objectParams, objectState, false);
                                }
                            }
                        }
                    }
                }
                else
                {
                    LoadBitmap(group->assets, BID, false);
                }
            }
        }
        else
        {
            if(IsValid(BID))
            {
                if(!dontRender)
                {
                    ObjectTransform objectTransform;
                    if(input->debug.ortho)
                    {
                        objectTransform = FlatTransform();
                    }
                    else
                    {
                        objectTransform = UprightTransform();
                    }
                    
                    objectTransform.flipOnYAxis = pieceParams.flipOnYAxis;
                    objectTransform.cameraOffset = pieceParams.cameraOffset;
                    objectTransform.angle = pieceParams.angle;
                    objectTransform.modulationPercentage = pieceParams.modulationWithFocusColor;
                    
                    objectTransform.additionalZBias += params->additionalZbias;
                    Vec2 finalScale = Hadamart(pieceParams.scale, ass->scale);
                    
                    Vec4 color = pieceParams.color;
                    color.a *=  ass->alpha;
                    
                    
                    for(AnimationEffect* effect = input->firstActiveEffect; effect; effect = effect->next)
                    {
                        if(effect->stringHashID == sprite->stringHashID)
                        {
                            DispatchAnimationEffect(input->worldMode, effect, input->entity, P, &color, input->timeToAdvance);
                        }
                    }
                    
                    Vec2 pivot = sprite->pivot;
                    BitmapDim dim = PushBitmapWithPivot(group, objectTransform, BID, P, pivot, 0, finalScale, color, pieceParams.lightIndexes);
                    
					if(input->debug.ortho)
					{
                        Vec2 XAxis = dim.XAxis.xy * dim.size.x;
                        Vec2 YAxis = dim.YAxis.xy * dim.size.y;
                        Vec2 startP = dim.P.xy;
						if(PointInUnalignedRect(startP, XAxis, YAxis, input->relativeScreenMouseP))
						{
							Vec2 centerP = startP + 0.5f * XAxis + 0.5f * YAxis;
							result.distanceFromAssCenter = Length(centerP - input->relativeScreenMouseP);
							result.screenInRect = true;
						}
					}
                    
                    Vec3 particleP = P + pieceParams.cameraOffset.x * group->gameCamera.X + pieceParams.cameraOffset.y * group->gameCamera.Y + pieceParams.cameraOffset.z * group->gameCamera.Z;
                    Vec3 velocity = V3(0, 0, 0.14f);
                    r32 lifeTime = 2.5f;
                    
                    AnimationState* animationState = &input->entity->animation;
                    if(animationState->spawnAshParticlesCount > 0)
                    {
                        SpawnAsh(input->worldMode->particleCache, particleP, velocity, lifeTime, animationState->ashColor, animationState->spawnAshParticlesCount, animationState->ashParticleViewPercentage, animationState->ashDim);
                    }
                    
                }
                
                
                Vec3 offsetRealP = pieceParams.cameraOffset.x * group->gameCamera.X + pieceParams.cameraOffset.y * group->gameCamera.Y + pieceParams.cameraOffset.z * group->gameCamera.Z;
                if(pieceParams.flipOnYAxis)
                {
                    offsetRealP.x = -offsetRealP.x;
                }
                
                Vec3 groundP = P + ProjectOnGround(offsetRealP, group->gameCamera.P);
                r32 distanceSq = LengthSq(groundP - input->mousePOnGround);
                
                if(isEquipmentAss)
                {
                    if(distanceSq < input->minFocusSlotDistanceSq)
                    {
                        result.onFocus = true;
                        input->minFocusSlotDistanceSq = distanceSq;
                    }
                }
                else
                {
                    if(distanceSq < input->minHotAssDistanceSq)
                    {
                        result.onFocus = true;
                        input->minHotAssDistanceSq = distanceSq;
                    }
                }
            }
        }
        
    }
    
    params->zOffset += 0.01f;
    return result;
}

#define ALPHA_CAME_IN_SECONDS 2.0f
#define MIN_STATUS_ALPHA -20.0f
#define MAX_STATUS_ALPHA 20.0f

inline r32 GetAssAlphaFade(u64 identifier, u32 assIndex, u32 lifePointsSeedResetCounter, r32 fadeDuration, r32 alphaThreesold, r32 lifePointRatio, r32 cameInTime, r32 status)
{
    RandomSequence seqLife = Seed((u32) identifier * assIndex + lifePointsSeedResetCounter);
    RandomSequence seqCameIn = Seed((u32) identifier * assIndex);
    RandomSequence seqStatus = Seed((u32) identifier * assIndex);
    
    r32 lifePointsAlpha = 1.0f;
    
    r32 threesoldUp = RandomRangeFloat(&seqLife, fadeDuration, alphaThreesold);
    if(lifePointRatio < alphaThreesold)
    {
        r32 threesoldDown = threesoldUp - fadeDuration;
        lifePointsAlpha = Clamp01MapToRange(threesoldDown, lifePointRatio, threesoldUp);
    }
    
    r32 maxAlphaTime = ALPHA_CAME_IN_SECONDS;
    r32 cameInAlphaThreeSold = RandomRangeFloat(&seqCameIn, 0.8f, 1.0f) * maxAlphaTime;
    r32 cameInAlpha = Clamp01MapToRange(0, cameInTime, cameInAlphaThreeSold);
    
    
    r32 statusAlpha = 1.0f;
    if(status < 0)
    {
        r32 statusAlphaThreesold = RandomRangeFloat(&seqStatus, MIN_STATUS_ALPHA, 0);
        r32 lerp = Clamp01MapToRange(statusAlphaThreesold, status, 0);
        statusAlpha = Lerp(0.4f, lerp, 1.0f);
    }
    else
    {
        r32 statusAlphaThreesold = RandomRangeFloat(&seqStatus, 1, MAX_STATUS_ALPHA);
        statusAlpha = Clamp01MapToRange(0, status, statusAlphaThreesold);
    }
    
    r32 result = lifePointsAlpha * cameInAlpha * statusAlpha;
    
    return result;
}

enum CycleAssOperation
{
    CycleAss_Render,
    CycleAss_RenderPivots,
};


inline void ApplyAssAlterations(PieceAss* ass, AssAlterations* assAlt, Bone* parentBone, Vec4* proceduralColor)
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

inline void AnimationPiecesOperation(AnimationFixedParams* input, RenderGroup* group, BlendResult* blended, Vec3 P, AnimationVolatileParams* params, CycleAssOperation operation)
{
    PieceAss* currentEquipmentRig = 0;
    r32 bestAssDistance = R32_MAX;
    u32 progressiveEquipmentIndex = 0;
    
    for(u32 assIndex = 0;assIndex < blended->assCount; ++assIndex)
    {
        BlendedAss* blendedAss = blended->ass + assIndex;
        PieceAss currentAss = *(GetAss(blended, assIndex));
        
        AssAlterations* assAlt = &blendedAss->alterations;
        Bone* parentBone = GetBone(blended, currentAss.boneID);
        SpriteInfo* sprite = GetSprite(blended, assIndex);
        
        
        Vec4 proceduralColor = input->defaultColoration;
        ApplyAssAlterations(&currentAss, assAlt, parentBone, &proceduralColor);
        switch(operation)
        {
            case CycleAss_Render:
            {
                r32 alpha = GetAssAlphaFade(input->entity->identifier, assIndex, 
                                            input->lifePointsSeedResetCounter, input->lifePointFadeDuration, input->lifePointThreesold, input->lifePointRatio, input->cameInTime, input->entity->status);
                proceduralColor.a *= alpha;
                
				RenderAssResult render = RenderPieceAss_(input, group, P, sprite, parentBone, &currentAss, params, false, true,  proceduralColor);
                
                if(render.onFocus)
                {
                    input->output->nearestAss = (i16) assIndex;
                }
                
                if(render.screenInRect && render.distanceFromAssCenter < bestAssDistance)
                {
                    bestAssDistance = render.distanceFromAssCenter;
                    input->output->hotAssIndex = (i16) assIndex;
                }
            } break;
            
            case CycleAss_RenderPivots:
            {
                Vec3 pivotP = AssOriginOffset(parentBone, &currentAss, params->zOffset, params->scale);
                
                ObjectTransform pivotTranform = FlatTransform();
                pivotTranform.additionalZBias = 30.0f;
                PushRect(group, pivotTranform, RectCenterDim(pivotP.xy + params->cameraOffset.xy, V2(4, 4)), V4(1, 1, 1, 1));
            } break;
        }
        
        
        for(u32 equipmentIndex = 0; equipmentIndex < blendedAss->equipmentAssCount; ++equipmentIndex)
        {
            EquipmentAnimationPiece* equipment = blendedAss->associatedEquipment + equipmentIndex;
            PieceAss* equipmentAss = &equipment->ass;
            SpriteInfo* spriteInfo = &equipment->sprite;
            i16 status = equipment->status;
            ComponentsProperties* properties = equipment->properties;
            
            switch(operation)
            {
                case CycleAss_Render:
                {
                    r32 alpha = GetAssAlphaFade(input->entity->identifier, (u32) spriteInfo->stringHashID,
                                                input->lifePointsSeedResetCounter, input->lifePointFadeDuration, input->lifePointThreesold, input->lifePointRatio, input->cameInTime, input->entity->status);
                    
                    equipmentAss->alpha *= alpha;
                    
                    AnimationVolatileParams pieceParams = *params;
                    
                    r32 ratio = Clamp01MapToRange(0, (r32) status, (r32) I16_MAX);
                    Vec4 statusColor = Lerp(bodyDead, ratio, V4(1, 1, 1, 1));
                    pieceParams.color = statusColor;
                    
                    pieceParams.properties = properties;
                    
                    if(equipment->drawModulated)
                    {
                        pieceParams.modulationWithFocusColor = input->defaultModulatonWithFocusColor;
                    }
                    
                    if(RenderPieceAss_(input, group, P, spriteInfo, parentBone, equipmentAss, &pieceParams, false, true).onFocus)
                    {
                        input->output->focusSlots = equipment->slot;
                    }
                    params->zOffset = pieceParams.zOffset;
                } break;
            }
        }
    }
    
    
    
    
    i16 hotAssIndex = input->output->hotAssIndex;
    if(hotAssIndex >= 0)
    {
        BlendedAss* blendedAss = blended->ass + hotAssIndex;
        PieceAss currentAss = *(GetAss(blended, hotAssIndex));
        AssAlterations* assAlt = &blendedAss->alterations;
        Bone* parentBone = GetBone(blended, currentAss.boneID);
        SpriteInfo* sprite = GetSprite(blended, hotAssIndex);
        
        Vec4 proceduralColor = input->defaultColoration;
        ApplyAssAlterations(&currentAss, assAlt, parentBone, &proceduralColor);
        
        RenderPieceAss_(input, group, P, sprite, parentBone, &currentAss, params, false, false, Hadamart(proceduralColor, V4(0.1f, 0.1f, 0.1f, 1)));
        
        if(input->debug.showPivots)
        {
            Vec3 pivotP = AssOriginOffset(parentBone, &currentAss, params->zOffset, params->scale);
            
            ObjectTransform pivotTranform = FlatTransform();
            pivotTranform.additionalZBias = 30.1f;
            PushRect(group, pivotTranform, RectCenterDim(pivotP.xy + params->cameraOffset.xy, V2(4, 4)), V4(0, 1, 0, 1));
        }
    }
}


inline AnimationId GetAnimationRecursive(Assets* assets, TaxonomyTable* table, u32 taxonomy, AssetTypeId assetID, u64* stringHashID)
{
    AnimationId result = {};
    
    TagVector match = {};
    TagVector weight = {};
    weight.E[Tag_direction] = 1.0f;
    match.E[Tag_direction] = 0;
    
    
#if 0    
    weight.E[Tag_ObjectState] = 10000.0f;
    match.E[Tag_ObjectState] = (r32) state;
#endif
    
    u32 currentTaxonomy = taxonomy;
    do
    {
        TaxonomySlot* slot = GetSlotForTaxonomy(table, currentTaxonomy);
        u64 testHash = slot->stringHashID;
        
        AnimationId ID = GetMatchingAnimation(assets, assetID, testHash, &match, &weight);
        if(IsValid(ID))
        {
            *stringHashID = testHash;
            result = ID;
            break;
        }
        currentTaxonomy = GetParentTaxonomy(slot);
        
    }while(currentTaxonomy);
    
    return result;
}

internal void UpdateAndRenderAnimation(AnimationFixedParams* input, RenderGroup* group, Animation* animation, u64 skeletonHashID, Vec3 P, AnimationState* animationState, AnimationVolatileParams* params, r32 timeToAdvance)
{
    BlendResult blended = {};
    
    GetAnimationPiecesAndAdvanceState(input, &blended, animation, animationState, timeToAdvance, params);
    
    TaxonomySlot* slot = GetSlotForTaxonomy(input->taxTable, input->entity->taxonomy);
    GetEquipmentPieces(&blended, input->taxTable, &slot->equipmentMappings, input->equipment);
    
    FrameData* referenceFrame = animation->frames + 0;
    Assert(referenceFrame->countBones == blended.boneCount);
    Assert(referenceFrame->countAss == blended.assCount);
    
    if(!input->debug.hideBitmaps)
    {
        AnimationPiecesOperation(input, group, &blended, P, params, CycleAss_Render);
    }
    
    
    
    if(input->debug.ortho && input->debug.showBones)
    {
        for(u32 boneIndex = 0; boneIndex < blended.boneCount; ++boneIndex)
        {
            Bone* bone = GetBone(&blended, boneIndex);
            Vec2 startP = P.xy + params->cameraOffset.xy + bone->finalOriginOffset;
            
            r32 thickness = 2.5f;
            
            Vec2 XAxis = 0.6f * Hadamart(params->scale, bone->mainAxis);
            Vec2 YAxis = Perp(XAxis);
            YAxis = 2.0f * thickness * Normalize(YAxis);
            
            Vec2 toP = startP + XAxis;
            
            Vec4 boneColor = V4(1, 0, 0, 1);
            if(PointInUnalignedRect(startP - 0.5f * YAxis, XAxis, YAxis, input->relativeScreenMouseP))
            {
                input->output->hotBoneIndex = (i16) boneIndex;
                boneColor = V4(0, 0, 0, 1);
            }
            
            PushLine(group, boneColor, V3(startP, params->additionalZbias + 1.0f), V3(toP, params->additionalZbias + 1.0f), thickness);
        }
    }
    
    if(input->debug.ortho && input->debug.showPivots)
    {
        AnimationPiecesOperation(input, group, &blended, P, params, CycleAss_RenderPivots);
    }
}


internal void RenderObjectLayout(AnimationFixedParams* input, RenderGroup* group, ObjectLayout* layout, Vec3 P, AnimationVolatileParams* params, ObjectState state)
{
    BlendResult blended;
    GetLayoutPieces(input, &blended, layout, params, state);
    
    if(!input->debug.hideBitmaps)
    {
        AnimationPiecesOperation(input, group, &blended, P, params, CycleAss_Render);
    }
    
    if(input->debug.ortho && input->debug.showPivots)
    {
        AnimationPiecesOperation(input, group, &blended, P, params, CycleAss_RenderPivots);
    }
}




inline void InitializeAnimationInputOutput(AnimationFixedParams* input, MemoryPool* tempPool, AnimationOutput* output, GameModeWorld* worldMode, ClientEntity* entityC, r32 timeToAdvance, ClientEntity* fakeEquipment = 0)
{
    AnimationState* animationState = &entityC->animation;
    
    input->tempPool = tempPool;
    input->timeToAdvance = timeToAdvance;
    input->worldMode = worldMode;
    input->taxTable = worldMode->table;
    input->defaultModulatonWithFocusColor = worldMode->modulationWithFocusColor;
    r32 slotMaxDistance = 0.6f;
    input->minFocusSlotDistanceSq = Square(slotMaxDistance);
    r32 hotAssMaxDistance = 0.3f;
    input->minHotAssDistanceSq = Square(hotAssMaxDistance);
    
    input->mousePOnGround = worldMode->worldMouseP;
    input->relativeScreenMouseP = worldMode->UI->relativeScreenMouse;
    
    
    r32 lifePointFadeDuration = 0.08f;
    r32 lifePointThreesold = 0.2f;
    r32 lifePointRatio = entityC->lifePoints / entityC->maxLifePoints;
    
    if(lifePointRatio > lifePointThreesold)
    {
        ++entityC->animation.lifePointsSeedResetCounter;
    }
    
    input->cameInTime = entityC->animation.cameInTime;
    if(timeToAdvance == 0 && input->cameInTime == 0)
    {
        input->cameInTime = R32_MAX;
    }
    input->lifePointsSeedResetCounter = entityC->animation.lifePointsSeedResetCounter;
    input->lifePointRatio = lifePointRatio;
    input->lifePointFadeDuration = lifePointFadeDuration;
    input->lifePointThreesold = lifePointThreesold;
    
    input->entity = entityC;
    input->firstActiveEffect = entityC->firstActiveEffect;
    
    
    u32 slotCount = 0;
    for(u32 slotIndex = Slot_None; slotIndex < Slot_Count; ++slotIndex)
    {
        input->equipment[slotIndex] = {};
        if(fakeEquipment)
        {
            ClientEntity* objectEntity = fakeEquipment + slotIndex;
            if(objectEntity->taxonomy)
            {
                EquipmentAnimationSlot* dest = input->equipment + slotCount++;
                
                dest->ID = objectEntity->identifier;
                dest->layout = GetLayout(worldMode->table, objectEntity->taxonomy);
                dest->taxonomy = objectEntity->taxonomy;
                dest->status = I16_MAX;
                GetVisualProperties(&dest->properties, worldMode->table, objectEntity->taxonomy, objectEntity->gen);
                
                dest->slot.slot = (SlotName) slotIndex;
            }
        }
        else
        {
            u64 objectEntityID = entityC->equipment[slotIndex].ID;
            if(objectEntityID)
            {
                ClientEntity* objectEntity = 0;
                objectEntity = GetEntityClient(worldMode, objectEntityID);
                
                if(objectEntity)
                {
                    u32 taxonomy;
                    GenerationData gen;
                    if(objectEntityID == 0xffffffffffffffff)
                    {
                        taxonomy = entityC->prediction.taxonomy;
                        gen = entityC->prediction.gen;
                    }
                    else
                    {
                        taxonomy = objectEntity->taxonomy;
                        gen = objectEntity->gen;
                    }
                    
                    EquipmentAnimationSlot* dest = input->equipment + slotCount++;
                    
                    dest->ID = objectEntityID;
                    dest->layout = GetLayout(worldMode->table, taxonomy);
                    dest->taxonomy = taxonomy;
                    dest->status = (i16) objectEntity->status;
                    GetVisualProperties(&dest->properties, worldMode->table, taxonomy, gen);
                    dest->slot.slot = (SlotName) slotIndex;
                    
                    dest->drawModulated = (AreEqual(dest->slot, entityC->animation.output.focusSlots) ||
                                           AreEqual(dest->slot, entityC->animation.nearestCompatibleSlotForDragging));
                    
                    dest->isOpen = false;
                    if(worldMode->UI->mode == UIMode_Equipment)
                    {
                        dest->isOpen = (objectEntityID == worldMode->UI->lockedInventoryID1 ||
                                        objectEntityID == worldMode->UI->lockedInventoryID2);
                    }
                }
            }
        }
    }
    
    input->objectCount = entityC->objects.objectCount;
    input->objects = entityC->objects.objects;
    
    TaxonomySlot* slot = GetSlotForTaxonomy(input->taxTable, entityC->taxonomy);
    input->objectGridDimX = slot->gridDimX;
    input->objectGridDimY = slot->gridDimY;
    
    input->draggingEntity = &worldMode->UI->draggingEntity;
    
    
    input->output = output;
    output->focusSlots = {};
    output->focusObjectIndex = -1;
    output->additionalZoomCoeff = 1.0f;
    output->hotBoneIndex = -1;
    output->hotAssIndex = -1;
    
}

inline SkeletonInfo GetSkeletonForTaxonomy(TaxonomyTable* table, TaxonomySlot* slot)
{
    SkeletonInfo result = {};
    while(slot->taxonomy)
    {
        if(slot->skeletonHashID)
        {
            result.skeletonHashID = slot->skeletonHashID;
            result.skinHashID = slot->skinHashID;
            result.coloration = slot->defaultColoration;
            result.originOffset = slot->originOffset;
            break;
        }
        
        slot = GetParentSlot(table, slot);
    }
    
    return result;
}

struct GetAIDResult
{
    AssetTypeId assetID;
    AnimationId AID;
    u64 skinHashID;
    u64 skeletonHashID;
    Vec4 coloration;
    Vec2 originOffset;
};

inline GetAIDResult GetAID(Assets* assets, TaxonomyTable* taxTable, u32 taxonomy, u32 action,u64 forcedNameHashID = 0)
{
    GetAIDResult result = {};
    result.coloration = V4(1, 1, 1, 1);
    
    TaxonomySlot* slot = GetSlotForTaxonomy(taxTable, taxonomy);
    
    
    SkeletonInfo skeletonInfo = GetSkeletonForTaxonomy(taxTable, slot);
    result.skeletonHashID = skeletonInfo.skeletonHashID;
    result.skinHashID = skeletonInfo.skinHashID;
    result.coloration = skeletonInfo.coloration;
    result.originOffset = skeletonInfo.originOffset;
    
    if(forcedNameHashID)
    {
        FindAnimationResult find = FindAnimationByName(assets, result.skeletonHashID, forcedNameHashID);
        result.assetID = find.assetType;
        result.AID = find.ID;	
    }
    else
    {
        
        AssetTypeId fallbackID = Asset_rig;
        
        result.assetID = GetAssetIDForEntity(assets, taxTable, taxonomy, action);
        Assert(result.assetID);
        
        if(!result.skeletonHashID)
        {
            result.AID = GetAnimationRecursive(assets, taxTable, taxonomy, result.assetID, &result.skinHashID);
        }
        else
        {
            TagVector match = {};
            TagVector weight = {};
            result.AID = GetMatchingAnimation(assets, result.assetID, result.skeletonHashID, &match, &weight);
            
            if(!IsValid(result.AID))
            {
                result.AID = GetMatchingAnimation(assets, fallbackID, result.skeletonHashID, &match, &weight);
            }
            
        }
    }
    
    return result;
}

internal AnimationOutput PlayAndDrawEntity(GameModeWorld* worldMode, RenderGroup* group, Vec4 lightIndexes, ClientEntity* entityC, Vec2 scale, r32 angle, Vec3 offset, r32 timeToAdvance, Vec4 color, b32 drawOpened, b32 onTop, Rect2 bounds, r32 additionalZbias, AnimationDebugParams debugParams = {})
{
    AnimationOutput result = {};
    TaxonomyTable* taxTable = worldMode->table;
    AnimationState* animationState = &entityC->animation;
    animationState->cameInTime += timeToAdvance;
    
    AnimationFixedParams input;
    
    MemoryPool tempPool = {};
    
    InitializeAnimationInputOutput(&input, &tempPool, &result, worldMode, entityC, timeToAdvance, debugParams.fakeEquipment);
    input.debug = debugParams;
    
    AnimationVolatileParams params;
    params.flipOnYAxis = animationState->flipOnYAxis;
    params.drawEmptySpaces = true;
    params.recipeIndex = 0;
    params.skinHashID = 0;
    params.additionalZbias = additionalZbias;
    params.modulationWithFocusColor = entityC->modulationWithFocusColor;
    params.cameraOffset = offset;
    params.lightIndexes = lightIndexes;
    params.color = color;
    params.angle = angle;
    params.scale = scale;
    params.zOffset = 0;
    params.properties = 0;
    
    TaxonomySlot* entitySlot = GetSlotForTaxonomy(worldMode->table, entityC->taxonomy);
    
    if(entitySlot->animationIn3d)
    {
        ModelId MID = GetFirstModel(group->assets, Asset_RockModels);
        
        PakModel* modelInfo = GetModelInfo(group->assets, MID);
        Vec3 standardDim = modelInfo->dim;
        Vec3 desiredDim = GetDim(entityC->bounds);
        
        r32 scaleX = desiredDim.x / standardDim.x;
        r32 scaleY = desiredDim.y / standardDim.y;
        r32 scaleZ = desiredDim.z / standardDim.z;
        
        PushModel(group, MID, Identity(), entityC->P + V3(0, 0, 0), lightIndexes, V3(scaleX, scaleY, scaleZ), V4(1, 1, 1, 1), entityC->modulationWithFocusColor);
    }
    else if(IsObject(worldMode->table, entityC->taxonomy))
    {
        ObjectState state = drawOpened ? ObjectState_GroundOpen : ObjectState_Ground;
        input.defaultColoration = V4(1, 1, 1, 1);
        ComponentsProperties properties;
        params.properties = &properties;
        
        GetVisualProperties(&properties, taxTable, entityC->taxonomy, entityC->gen);
        
        ObjectLayout* layout = GetLayout(taxTable, entityC->taxonomy);
        if(layout)
        {
            Rect2 animationBounds = GetLayoutBounds(&input, group, layout, &params, state);
            if(HasArea(bounds))
            {
                Vec2 boundsDim = GetDim(bounds);
                r32 boundsFillPercentage = 0.9f;
                Vec2 animationDim = GetDim(animationBounds);
                r32 boundScaleX = boundsDim.x / animationDim.x * boundsFillPercentage;
                r32 boundScaleY = boundsDim.y / animationDim.y * boundsFillPercentage;
                
                r32 boundScale = Min(boundScaleX, boundScaleY);
                Vec2 animationCenter = GetCenter(animationBounds);
                params.cameraOffset = -boundScale * V3(animationCenter, 0);
                params.scale *= Min(boundScaleX, boundScaleY);
                
                animationBounds = RectCenterDim(animationCenter, Hadamart(animationDim, V2(boundScaleX, boundScaleY)));
            }
            
            if(!debugParams.ortho)
            {
                animationState->bounds = animationBounds;
            }
            
            RenderObjectLayout(&input, group, layout, entityC->P, &params, state);
        }
    }
    else
    {
        PushNewAction(animationState, entityC->action);
        GetAIDResult prefetchAID = GetAID(group->assets, taxTable, entityC->taxonomy, entityC->action);
        PrefetchAnimation(group->assets, prefetchAID.AID);
        
        GetAIDResult AID = GetAID(group->assets, taxTable, entityC->taxonomy, animationState->action, debugParams.forcedNameHashID);
        
        input.defaultColoration = AID.coloration;
        input.combatAnimation = (AID.assetID == Asset_attacking);
        
        params.cameraOffset.xy += AID.originOffset;
        
        if(IsValid(AID.AID))
        {
            Animation* animation = GetAnimation(group->assets, AID.AID);
            if(animation)
            {
                result.playedAnimationNameHash = animation->header->nameHash;
                
                r32 quicknessCoeff = 1.0f;
                timeToAdvance *= quicknessCoeff;
                params.skeletonHashID = AID.skeletonHashID;
                params.skinHashID = AID.skinHashID;
                
                BlendResult blended;
                AnimationState dummyState = {};
                GetAnimationPiecesAndAdvanceState(&input, &blended, animation, &dummyState, 0, &params);
                Rect2 animationBounds = GetPiecesBound(group, &blended, &params);
                if(HasArea(bounds))
                {
                    Vec2 boundsDim = GetDim(bounds);
                    r32 boundsFillPercentage = 0.9f;
                    Vec2 animationDim = GetDim(animationBounds);
                    r32 boundScaleX = boundsDim.x / animationDim.x * boundsFillPercentage;
                    r32 boundScaleY = boundsDim.y / animationDim.y * boundsFillPercentage;
                    
                    r32 boundScale = Min(boundScaleX, boundScaleY);
                    Vec2 animationCenter = GetCenter(animationBounds);
                    params.cameraOffset = -boundScale * V3(animationCenter, 0);
                    params.scale *= Min(boundScaleX, boundScaleY);
                    
                    animationBounds = RectCenterDim(animationCenter, Hadamart(animationDim, V2(boundScaleX, boundScaleY)));
                }
                
                if(!debugParams.ortho)
                {
                    animationState->bounds = Offset(animationBounds, AID.originOffset);
					if(entityC->action == Action_None)
					{
						animationState->cameraEntityOffset = GetCenter(animationState->bounds);
					}
                }
                
                UpdateAndRenderAnimation(&input, group, animation, AID.skeletonHashID, entityC->P, animationState, &params, timeToAdvance);
            }
            else
            {
                LoadAnimation(group->assets, AID.AID);
            }
        }
    }
    
    Clear(&tempPool);
    
    return result;
}

inline r32 GetChunkyness(WorldTile* t0, WorldTile* t1)
{
    r32 result = (t0->taxonomy == t1->taxonomy) ? t0->chunkynessSame : t0->chunkynessOther;
    return result;
}

inline Vec3 GetTileColorDelta(WorldTile* tile, RandomSequence* seq)
{
    Vec4 delta = tile->colorDelta;
    r32 noiseBilateral = (tile->layoutNoise - 0.5f) * 2.0f;
    
    Vec3 noisy;
    noisy.r = delta.r * noiseBilateral;
    noisy.g = delta.g * noiseBilateral;
    noisy.b = delta.b * noiseBilateral;
    
    
    Vec3 random;
    random.r = delta.r * RandomBil(seq);
    random.g = delta.g * RandomBil(seq);
    random.b = delta.b * RandomBil(seq);
    
    
    Vec3 result = Lerp(noisy, tile->colorRandomness, random);
    
    return result;
}

inline Vec4 GetTileColor(WorldTile* tile, b32 uniformColor, RandomSequence* seq)
{
    Vec4 color = tile->baseColor;
    if(!uniformColor)
    {
        color.rgb += GetTileColorDelta(tile, seq);
    }
    
    color = Clamp01(color);
    color = SRGBLinearize(color);
    return color;
}

inline Vec4 ComputeWeightedChunkColor(WorldChunk* chunk)
{
    Vec4 result = {};
    
    for(u8 Y = 0; Y < CHUNK_DIM; ++Y)
    {
        for(u8 X = 0; X < CHUNK_DIM; ++X)
        {
            WorldTile* tile = GetTile(chunk, X, Y);
            result += GetTileColor(tile, false, 0);
        }
    }
    
    result *= (1.0f / Square(CHUNK_DIM));
    return result;
}




inline Vec4 GetWaterColor(WorldTile* tile)
{
    Vec4 waterColor = {};
    if(tile->waterLevel < WATER_LEVEL)
    {
        r32 maxColorDisplacement = 0.4f * WATER_LEVEL;
        r32 maxAlphaDisplacement = 0.3f * WATER_LEVEL;
        
        Vec3 minColorDeep = V3(0.0f, 0.03f, 0.05f);
        Vec3 maxColorDeep = V3(0.0f, 0.08f, 0.4f);
        
        r32 maxAlphaDeep = 1.0f;
        r32 minAlphaDeep = 0.7f;
        
        Vec3 minColorSwallow = V3(0.0f, 0.1f, 0.78f);
        Vec3 maxColorSwallow = V3(0.65f, 0.75f, 1.0f);
        
        r32 maxAlphaSwallow = 1.0f;
        r32 minAlphaSwallow = 0.0f;
        
        r32 sineWaterLevel = Clamp01MapToRange(0.85f * WATER_LEVEL, tile->waterLevel, WATER_LEVEL);
        r32 normalizedWaterLevel = Clamp01MapToRange(0, tile->waterLevel, 0.95f * WATER_LEVEL);
        normalizedWaterLevel = Pow(normalizedWaterLevel, 15.0f);
        
        Vec3 minColor = Lerp(minColorDeep, normalizedWaterLevel, minColorSwallow);
        Vec3 maxColor = Lerp(maxColorDeep, normalizedWaterLevel, maxColorSwallow);
        
        r32 minAlpha = Lerp(minAlphaDeep, normalizedWaterLevel, minAlphaSwallow);
        r32 maxAlpha = Lerp(maxAlphaDeep, normalizedWaterLevel, maxAlphaSwallow);
        
        r32 blueNoise = tile->blueNoise;
        r32 alphaNoise = tile->alphaNoise;
        
        r32 sine = Sin(DegToRad(tile->waterSine));
        r32 blueSine = sine;
        r32 alphaSine = sine;
        
        
        r32 blueNoiseSine = Lerp(blueNoise, sineWaterLevel, blueSine);
        r32 alphaNoiseSine = Lerp(alphaNoise, sineWaterLevel, alphaSine);
        
        
        r32 blueDisplacement = blueNoiseSine * maxColorDisplacement;
        r32 alphaDisplacement = alphaNoiseSine * maxAlphaDisplacement;
        
        
        r32 blueLerp = Clamp01MapToRange(0, tile->waterLevel + blueDisplacement, WATER_LEVEL);
        
        r32 alphaLevel = tile->waterLevel + alphaDisplacement;
        
        r32 alphaLerp = Clamp01MapToRange(0, alphaLevel, WATER_LEVEL);
        alphaLerp = Pow(alphaLerp, 2.2f);
        
        Vec3 color = Lerp(minColor, blueLerp, maxColor);
        r32 alpha = Lerp(maxAlpha, alphaLerp, minAlpha);
        
        waterColor = V4(color, alpha);
    }
    
    return waterColor;
}


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

internal AnimationOutput RenderEntity(RenderGroup* group, GameModeWorld* worldMode, ClientEntity* entityC, r32 timeToUpdate, AnimationEntityParams params = StandardEntityParams())
{
    r32 ratio;
    if(entityC->maxLifePoints > 0)
    {
        ratio = entityC->lifePoints / entityC->maxLifePoints;
    }
    else
    {
        if(entityC->status < 0)
        {
            ashAlive = V4(0.5f, 0.5f, 0.5f, 1.0f);
            bodyAlive = ashAlive;
            ratio = 1.0f;
        }
        else
        {
            ratio = Clamp01MapToRange(0, (r32) entityC->status, (r32) I16_MAX);
        }
    }
    
    Vec4 bodyColor = Lerp(bodyDead, ratio, bodyAlive);
    if(params.transparent)
    {
        bodyColor.a *= 0.2f;
    }
    
    Vec4 ashColor = Lerp(ashDead, ratio, ashAlive);
    
    
    entityC->animation.ashColor = ashColor;
    entityC->animation.ashDim = 0.06f;
    entityC->animation.ashParticleViewPercentage = 1.0f;
    
    
    
    AnimationOutput result = {};
    
    ClientEntity* player = GetEntityClient(worldMode, worldMode->player.identifier);
    
    Vec4 lightIndexes = V4(-1, -1, -1, -1);
    TaxonomySlot* slot = GetSlotForTaxonomy(worldMode->table, entityC->taxonomy);
    
    for(AnimationEffect* effect = entityC->firstActiveEffect; effect; effect = effect->next)
    {
        if(!effect->stringHashID)
        {
            Vec4 ignored;
            DispatchAnimationEffect(worldMode, effect, entityC, entityC->P, &ignored, timeToUpdate);
        }
    }
    
    
    entityC->animation.ashIdleTimer += timeToUpdate;
    if(!entityC->animation.spawnAshParticlesCount && entityC->animation.ashIdleTimer >= 1.3f)
    {
        entityC->animation.ashIdleTimer = 0;
        entityC->animation.spawnAshParticlesCount = 1;
        entityC->animation.ashParticleViewPercentage = 0.13f;
        entityC->animation.ashColor.a = 1.0f;
        entityC->animation.ashDim = 0.05f;
    }
    
    EntityAction soundAction = entityC->action;
    r32 oldSoundTime = entityC->actionTime - timeToUpdate;
    r32 soundTime = entityC->actionTime;
    
    Rect3 bounds = InvertedInfinityRect3();
    GetPhysicalProperties(worldMode->table, entityC->taxonomy, entityC->identifier, &entityC->boundType, &bounds, entityC->generationIntensity);
    entityC->bounds = Offset(bounds, entityC->P);
    
    if(IsPlant(worldMode->table, entityC->taxonomy))
    {
        Assert(slot->plant);
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
            plant->sequence = Seed((u32)entityC->identifier);
        }
        
        entityC->plant->leafBitmap = FindBitmapByName(group->assets, Asset_leaf, slot->plant->leafStringHash);
        entityC->plant->trunkBitmap = FindBitmapByName(group->assets, Asset_trunk, slot->plant->trunkStringHash);
        
        
        PlantRenderingParams renderingParams = {};
        renderingParams.lightIndexes = lightIndexes;
        renderingParams.modulationWithFocusColor = entityC->modulationWithFocusColor;
        
        UpdateAndRenderPlant(worldMode, group, renderingParams, slot->plant, entityC->plant, entityC->P);
        
        for(u32 plantIndex = 0; plantIndex < entityC->plant->plant.plantCount; ++plantIndex)
        {
            Vec3 P = entityC->P + V3(entityC->plant->plant.offsets[plantIndex], 0);
            
            Rect3 plantBounds = Offset(bounds, P);
            entityC->bounds = Union(entityC->bounds, plantBounds);
        }
    }
    else if(IsRock(worldMode->table, entityC->taxonomy))
    {
        TaxonomySlot* rockSlot = GetSlotForTaxonomy(worldMode->table, entityC->taxonomy);
        RockDefinition* rockDefinition = rockSlot->rock;
        
        if(!entityC->rock)
        {
            ModelId ID = GetFirstModel(group->assets, Asset_RockModels);
            VertexModel* tetraModel = GetModel(group->assets, ID);
            if(tetraModel)
            {
                ClientRock* newRock = worldMode->firstFreeRock;
                if(!newRock)
                {
                    newRock = PushStruct(&worldMode->entityPool, ClientRock);
                }
                else
                {
                    worldMode->firstFreeRock = newRock->nextFree;
                }
                entityC->rock = newRock;
                
                
                RandomSequence rockSeq = Seed((u32)entityC->identifier);
                newRock->dim = GetRockDim(rockDefinition, &rockSeq); 
                
                MemoryPool tempPool = {};
                TempMemory rockMemory = BeginTemporaryMemory(&tempPool);
                GenerateRock(newRock, tetraModel, &tempPool, &rockSeq, rockDefinition);
                EndTemporaryMemory(rockMemory);
            }
            else
            {
                LoadModel(group->assets, ID);
            }
        }
        
        ClientRock* rock = entityC->rock;
        if(rock)
        {
            VertexModel onTheFly;
            
            onTheFly.vertexCount = rock->vertexCount;
            onTheFly.vertexes = rock->vertexes;
            
            onTheFly.faceCount = rock->faceCount;
            onTheFly.faces = rock->faces;
            
            RandomSequence rockRenderSeq = Seed((u32)entityC->identifier);
            u32 rockCount = Max(1, rockDefinition->renderingRocksCount + RandomChoice(&rockRenderSeq, rockDefinition->renderingRocksDelta));
            for(u32 rockIndex = 0; rockIndex < rockCount; ++rockIndex)
            {
                Vec3 finalP =entityC->P +Hadamart(RandomBilV3(&rockRenderSeq), rockDefinition->renderingRocksRandomOffset);
                m4x4 rotation = ZRotation(RandomUni(&rockRenderSeq) * TAU32);
                Vec3 finalScale = rock->dim + rockDefinition->scaleRandomness *Hadamart(RandomBilV3(&rockRenderSeq), rock->dim);
                
                PushModel(group, &onTheFly, rotation, finalP, lightIndexes, finalScale, V4(1, 1, 1, 1), entityC->modulationWithFocusColor);
                
                Rect3 rockBounds = Offset(bounds, finalP);
                entityC->bounds = Union(entityC->bounds, rockBounds);
            }
        }
    }
    else
    {
        oldSoundTime = entityC->animation.normalizedTime;
        Vec2 animationScale = params.scale * V2(0.33f, 0.33f);
        r32 additionalZbias = params.additionalZbias;
        if(params.onTop)
        {
            additionalZbias = 5.0f;
        }
        
        additionalZbias += 0.4f * GetDim(entityC->animation.bounds).y;
        
        EquipInfo dragging = entityC->animation.nearestCompatibleSlotForDragging;
        if(IsValid(dragging))
        {
            u64 ID = worldMode->UI->draggingEntity.identifier;
            MarkAllSlotsAsOccupied(entityC->equipment, dragging, ID);
        }
        
        result = PlayAndDrawEntity(worldMode, group, lightIndexes, entityC, animationScale, params.angle, params.offset, timeToUpdate, bodyColor, params.drawOpened, params.onTop, params.bounds, additionalZbias);
        
        if(IsValid(dragging))
        {
            MarkAllSlotsAsNull(entityC->equipment, dragging);
        }
        
        soundAction = (EntityAction) entityC->animation.action;
        soundTime = entityC->animation.normalizedTime;
    }
    
    //PushCubeOutline(group, entityC->bounds, V4(1, 1, 1, 1), 0.05f);
    
    PlaySoundForAnimation(worldMode, group->assets, slot, result.playedAnimationNameHash, oldSoundTime, soundTime);
    
    return result;
}


internal AnimationOutput RenderEntity(RenderGroup* group, GameModeWorld* worldMode, ClientEntity* clientEntity, Vec3 P, Vec2 dim, r32 additionalZbias)
{
    Vec3 oldP = clientEntity->P;
    clientEntity->P = P;
    AnimationEntityParams params = StandardEntityParams();
    params.bounds = RectCenterDim(V2(0, 0), dim);
    params.additionalZbias = additionalZbias;
    
    AnimationOutput result = RenderEntity(group, worldMode, clientEntity, 0, params);
    
    
    clientEntity->P = oldP;
    
    return result;
}

internal Rect2 RenderObject(RenderGroup* group, GameModeWorld* worldMode, Object* object, Vec3 P, Vec2 dim, r32 additionalZbias)
{
    ClientEntity clientEntity = {};
    clientEntity.animation.cameInTime = R32_MAX;
    
    clientEntity.taxonomy = object->taxonomy;
    clientEntity.gen = object->gen;
    clientEntity.P = P;
    clientEntity.status = object->status;
    
    AnimationEntityParams params = StandardEntityParams();
    params.bounds = RectCenterDim(V2(0, 0), dim);
    params.additionalZbias = additionalZbias;
    RenderEntity(group, worldMode, &clientEntity, 0, params);
    
    Rect2 result = clientEntity.animation.bounds;
    return result;
}


