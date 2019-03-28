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

internal void BlendFrames_(Animation* animation, BlendResult* in, u32 lowerFrameIndex, u32 timelineMS, u32 upperFrameIndex)
{
    FrameData* reference = animation->frames + 0;
    
    u32 boneCount = reference->countBones;
    u32 assCount = reference->countAss;
    
    Assert(boneCount < ArrayCount(in->bones));
    Assert(assCount < ArrayCount(in->ass));
    
    in->boneCount = boneCount;
    in->assCount = assCount;
    
    
    Bone* firstRefBone = animation->bones + reference->firstBoneIndex;
    for(u32 boneIndex = 0; boneIndex < boneCount; boneIndex++)
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
        in->bones[boneIndex] = BlendBones_(b1, lerp, b2);
        
        in->boneAlterations[boneIndex] = {};
    }
    
    
    PieceAss* firstRefAss = animation->ass + reference->firstAssIndex;
    for(u32 assIndex = 0; assIndex < assCount; assIndex++)
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
        in->ass[assIndex] = BlendAss_(a1, lerp, a2);
        
        u32 boneIndex = in->ass[assIndex].boneID;
        in->equipmentAssCount[assIndex] = 0;
        
        in->assAlterations[assIndex] = {};
        
        Assert(in->ass[assIndex].spriteIndex < animation->spriteInfoCount);
        
        SpriteInfo* sprite = animation->spriteInfos + in->ass[assIndex].spriteIndex;
        in->sprites[assIndex] = *sprite;
    }
}

internal Vec2 CalculateFinalBoneOffset_(AnimationFixedParams* input, Bone* frameBones, i32 countBones, Bone* bone, AnimationVolatileParams* params)
{
    Assert(bone->parentID < countBones);
    Vec2 baseOffset = V2(0, 0);
    
    Vec2 XAxis;
    Vec2 YAxis;
    
    Vec2 offsetFromParentScale = V2(1, 1);
    if(bone->parentID >= 0)
    {
        Bone* parent = frameBones + bone->parentID;
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

internal void GetAnimationPiecesAndAdvanceState(AnimationFixedParams* input, BlendResult* blended, Animation* animation, AnimationState* state, r32 elapsedTime, AnimationVolatileParams* params)
{
    r32 oldAnimationTime = state->totalTime;
    r32 animationTime = state->totalTime + elapsedTime;
    u32 timeline = (u32) (animationTime * 1000.0f);
    
    AnimationHeader* header = animation->header;
    
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
    
    b32 ended = (timeline >= header->durationMS);
    
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
    BlendFrames_(animation, blended, lowerFrameIndex, animTimeMod, upperFrameIndex);
    
    TaxonomySlot* slot = GetSlotForTaxonomy(input->taxTable, input->entity->taxonomy);
    for(TaxonomyBoneAlterations* boneAlt = slot->firstBoneAlteration; boneAlt; boneAlt = boneAlt->next)
    {
        Assert(boneAlt->boneIndex < ArrayCount(blended->boneAlterations));
        blended->boneAlterations[boneAlt->boneIndex] = boneAlt->alt;
    }
    
    for(TaxonomyAssAlterations* assAlt = slot->firstAssAlteration; assAlt; assAlt = assAlt->next)
    {
        Assert(assAlt->assIndex < ArrayCount(blended->assAlterations));
        blended->assAlterations[assAlt->assIndex] = assAlt->alt;
    }
    
    
    if(ended)
    {
        if(state->nextAction == state->action && header->singleCycle)
        {
            state->action = Action_None;
        }
        else
        {
            state->totalTime = 0;
            state->action = state->nextAction;
        }
    }
    state->totalTime = animationTime;
    
    for(u32 boneIndex = 0; boneIndex < blended->boneCount; boneIndex++)
    {
        Bone* bone = blended->bones + boneIndex;
        BoneAlterations* boneAlt = blended->boneAlterations + boneIndex;
        
        if(bone->parentID != -1)
        {
            Bone* parent = blended->bones + bone->parentID;
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
                                PieceAss* ass = blended->ass + assIndex;
                                Bone* parentBone = blended->bones + ass->boneID;
                                
                                if(eq->index == 0xff || (eq->stringHashID == componentHashID && eq->index == index))
                                {
                                    Assert(blended->equipmentAssCount[assIndex] < ArrayCount(blended->equipment[0]));
                                    u32 equipmentAssIndex = blended->equipmentAssCount[assIndex]++;
                                    
                                    EquipmentAnimationPiece* dest = blended->equipment[assIndex] + equipmentAssIndex;
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
                                        zOffset = piece->parentOffset.z;
                                        angle = piece->parentAngle;
                                        scale = piece->scale;
                                        offset = piece->parentOffset.xy;
                                        
                                        if(decorativePiece)
                                        {
                                            zOffset += piece->parent->parentOffset.z;
                                            angle += piece->parent->parentAngle;
                                            offset += piece->parent->parentOffset.xy;
                                        }
                                    }
                                    else if(decorativePiece)
                                    {
                                        zOffset = piece->parentOffset.z;
                                        angle = piece->parentAngle;
                                        scale = piece->scale;
                                        offset = piece->parentOffset.xy;
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
                                    
                                    destSprite->pivot = piece->pivot;
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


inline b32 PushNewAction(AnimationState* animation, u32 action)
{
    b32 result = false;
    if(animation->action != action)
    {
        animation->stopAtNextBarrier = true;
        animation->nextAction = action;
        result = true;
    }
    
    return result;
}


internal void GetVisualProperties(ComponentsProperties* dest, TaxonomyTable* table, u32 taxonomy, u64 recipeIndex, b32 onGround, b32 drawOpened)
{
    dest->componentCount = 0;
    TaxonomySlot* slot = GetSlotForTaxonomy(table, taxonomy);
    
    ObjectLayout* layout = GetLayout(table, taxonomy, onGround, drawOpened);
    if(layout)
    {
        RandomSequence seq = Seed((u32)recipeIndex);
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

inline BitmapId GetBitmapID(RenderGroup* group, SpriteInfo* sprite, u64 entityHashID, ComponentsProperties* properties)
{
    BitmapId result = {};
    
    TagVector match = {};
    TagVector weight = {};
    
    match.E[Tag_firstHashHalf] = (r32) (entityHashID >> 32);
    match.E[Tag_secondHashHalf] = (r32) (entityHashID & 0xFFFFFFFF);
    weight.E[Tag_firstHashHalf] = 10.0f;
    weight.E[Tag_secondHashHalf] = 10.0f;
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

inline void GetLayoutPieces(AnimationFixedParams* input, BlendResult* output, ObjectLayout* layout, AnimationVolatileParams* params)
{
    output->boneCount = 1;
    output->assCount = 0;
    Bone* bone = output->bones + 0;
    *bone = {};
    bone->mainAxis = V2(1, 0);
    bone->parentID = -1;
    
    BoneAlterations* boneAlt = output->boneAlterations + 0;
    boneAlt->valid = false;
    
    for(LayoutPiece* source = layout->firstPiece; source; source = source->next)
    {
        Assert(output->assCount <= ArrayCount(output->ass));
        u32 pieceIndex = output->assCount++;
        PieceAss* destAss = output->ass + pieceIndex;
        
        output->equipmentAssCount[pieceIndex] = 0;
        
        AssAlterations* destAlt = output->assAlterations + pieceIndex;
        destAlt->valid = false;
        
        SpriteInfo* destSprite = output->sprites + pieceIndex;
        
        *destAss = {};
        *destSprite = {};
        
        destAss->spriteIndex = pieceIndex;
        destAss->boneOffset = source->parentOffset.xy;
        destAss->additionalZOffset = source->parentOffset.z;
        destAss->angle = source->parentAngle;
        
        if(source->parent)
        {
            destAss->boneOffset += source->parent->parentOffset.xy;
            destAss->additionalZOffset += source->parent->parentOffset.z;
            destAss->angle += source->parent->parentAngle;
        }
        
        destAss->scale = source->scale;
        destAss->alpha = source->alpha;
        
        destSprite->pivot = source->pivot;
        destSprite->stringHashID = source->componentHashID;
        destSprite->index = source->index;
        destSprite->flags = source->flags;
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
        PieceAss* ass = blended->ass + assIndex;
        Bone* parentBone = blended->bones + ass->boneID;
        SpriteInfo* sprite = blended->sprites + assIndex;
        
        Vec3 originOffset = AssOriginOffset(parentBone, ass, params->zOffset, params->scale);
        r32 finalAngle = AssFinalAngle(parentBone, ass);
        
        if(!(sprite->flags & Sprite_Composed))
        {	        
            BitmapId BID = GetBitmapID(group, sprite, params->entityHashID, params->properties);
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

inline Rect2 GetLayoutBounds(AnimationFixedParams* input, RenderGroup* group, ObjectLayout* layout, AnimationVolatileParams* params)
{
    BlendResult blended;
    GetLayoutPieces(input, &blended, layout, params);
    Rect2 result = GetPiecesBound(group, &blended, params);
    
    return result;
}


internal void RenderObjectLayout(AnimationFixedParams* input, RenderGroup* group, ObjectLayout* layout, Vec3 P, AnimationVolatileParams* params);

inline b32 DrawModularPiece(AnimationFixedParams* input, RenderGroup* group, Vec3 P, u32 pieceTaxonomy, AnimationVolatileParams* params, b32 onGround, b32 drawOpened, b32 dontRender)
{
    b32 result = false;
    ObjectLayout* layout = GetLayout(input->taxTable, pieceTaxonomy, onGround, drawOpened);
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
            RenderObjectLayout(input, group, layout, P, params);
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
    pieceParams.entityHashID = sprite->stringHashID;
    
    
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
        BitmapId BID = GetBitmapID(group, sprite, params->entityHashID, params->properties);
        if(sprite->flags & Sprite_Composed)
        {
            InvalidCodePath;
#if 0            
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
                Vec2 minDimTest = Hadamart(pieceParams.scale, ass->scale);
                r32 minDim = Min(minDimTest.x, minDimTest.y);
                r32 zoomCoeff = 1.0f / minDim;
                input->output->additionalZoomCoeff = Max(input->output->additionalZoomCoeff, zoomCoeff);
                
                Vec2 finalScale = Hadamart(pieceParams.scale, ass->scale);
                Bitmap* bitmap = GetBitmap(group->assets, BID);
                
                ObjectTransform spaceTransform = UprightTransform();
                spaceTransform.cameraOffset = pieceParams.cameraOffset;
                
                if(bitmap)
                {
                    Vec4 cellColor = V4(pieceParams.color.rgb, 0.5f);
                    r32 zOffset = 0.0f;
                    
                    Vec3 originalGridDim = V3(Hadamart(finalScale, V2(bitmap->widthOverHeight * bitmap->nativeHeight, bitmap->nativeHeight)), 0);
                    
                    u8 gridDimX = input->objectGridDimX;
                    u8 gridDimY = input->objectGridDimY;
                    r32 cellDim =ArrangeObjects(gridDimX, gridDimY, originalGridDim);
                    
                    Vec3 gridDim = cellDim * V3(gridDimX, gridDimY, 0);
                    Vec3 halfGridDim = 0.5f * gridDim;
                    Vec3 lowLeftCorner = pieceParams.cameraOffset - halfGridDim + 0.5f * V3(cellDim, cellDim, 0);
                    
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
                            Rect2 cellRect = ProjectOnGround(group, P, pieceParams.cameraOffset, V3(cellDim, cellDim, 0));
                            
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
                            spaceTransform.additionalZBias += params->additionalZbias;
                            
                            PushBitmap_(group, spaceTransform, bitmap, P, 0, finalScale, cellColor, pieceParams.lightIndexes, V2(0.5f, 0.5f));
                            pieceParams.cameraOffset = objectP;
                            
                            u32 taxonomy = object->taxonomy;
                            u64 recipeIndex = object->recipeIndex;
                            
                            if(taxonomy)
                            {
                                if(IsRecipe(object))
                                {
                                    taxonomy = input->taxTable->recipeTaxonomy;
                                    recipeIndex = 0;
                                }
                                TaxonomySlot* slot = GetSlotForTaxonomy(input->taxTable, taxonomy);
                                AnimationVolatileParams objectParams = pieceParams;
                                objectParams.entityHashID = slot->stringHashID;
                                
                                ObjectLayout* layout = GetLayout(input->taxTable, taxonomy, true, false);
                                Rect2 animationBounds = GetLayoutBounds(input, group, layout, &objectParams);
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
                                    GetVisualProperties(&objectProperties, input->taxTable, taxonomy, recipeIndex, false, false);
                                    
                                    DrawModularPiece(input, group, P, slot->taxonomy, &objectParams, true, false, false);
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
        PieceAss currentAss = blended->ass[assIndex];
        AssAlterations* assAlt = blended->assAlterations + assIndex;
        Bone* parentBone = blended->bones + currentAss.boneID;
        SpriteInfo* sprite = blended->sprites + assIndex;
        
        
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
        
        
        for(u32 equipmentIndex = 0; equipmentIndex < blended->equipmentAssCount[assIndex]; ++equipmentIndex)
        {
            u32 equipmentAssIndex = progressiveEquipmentIndex + equipmentIndex;
            
            EquipmentAnimationPiece* equipment = blended->equipment[assIndex] + equipmentAssIndex;
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
        PieceAss currentAss = blended->ass[hotAssIndex];
        AssAlterations* assAlt = blended->assAlterations + hotAssIndex;
        Bone* parentBone = blended->bones + currentAss.boneID;
        SpriteInfo* sprite = blended->sprites + hotAssIndex;
        
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


inline AnimationId GetAnimationRecursive(Assets* assets, TaxonomyTable* table, u32 taxonomy, AssetTypeId assetID, u64* stringHashID, ObjectState state)
{
    AnimationId result = {};
    
    TagVector match = {};
    TagVector weight = {};
    weight.E[Tag_direction] = 1.0f;
    match.E[Tag_direction] = 0;
    weight.E[Tag_ObjectState] = 10000.0f;
    match.E[Tag_ObjectState] = (r32) state;
    
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
    BlendResult blended;
    GetAnimationPiecesAndAdvanceState(input, &blended, animation, animationState, timeToAdvance, params);
    
    TaxonomySlot* slot = GetSlotForTaxonomy(input->taxTable, input->entity->taxonomy);
    GetEquipmentPieces(&blended, input->taxTable, &slot->equipmentMappings,  input->equipment);
    
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
            Bone* bone = blended.bones + boneIndex;
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


internal void RenderObjectLayout(AnimationFixedParams* input, RenderGroup* group, ObjectLayout* layout, Vec3 P, AnimationVolatileParams* params)
{
    BlendResult blended;
    GetLayoutPieces(input, &blended, layout, params);
    
    if(!input->debug.hideBitmaps)
    {
        AnimationPiecesOperation(input, group, &blended, P, params, CycleAss_Render);
    }
    
    if(input->debug.ortho && input->debug.showPivots)
    {
        AnimationPiecesOperation(input, group, &blended, P, params, CycleAss_RenderPivots);
    }
}




inline void InitializeAnimationInputOutput(AnimationFixedParams* input, AnimationOutput* output, GameModeWorld* worldMode, ClientEntity* entityC, r32 timeToAdvance, ClientEntity* fakeEquipment = 0)
{
    AnimationState* animationState = &entityC->animation;
    
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
                b32 drawOpened = false;
                
                dest->ID = objectEntity->identifier;
                dest->layout = GetLayout(worldMode->table, objectEntity->taxonomy, false, false);
                dest->taxonomy = objectEntity->taxonomy;
                dest->status = I16_MAX;
                GetVisualProperties(&dest->properties, worldMode->table, objectEntity->taxonomy, objectEntity->recipeIndex, false, drawOpened);
                
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
                    u64 recipeIndex;
                    if(objectEntityID == 0xffffffffffffffff)
                    {
                        taxonomy = entityC->prediction.taxonomy;
                        recipeIndex = entityC->prediction.recipeIndex;
                    }
                    else
                    {
                        taxonomy = objectEntity->taxonomy;
                        recipeIndex = objectEntity->recipeIndex;
                    }
                    
                    EquipmentAnimationSlot* dest = input->equipment + slotCount++;
                    b32 drawOpened = false;
                    if(worldMode->UI->mode == UIMode_Equipment)
                    {
                        drawOpened = (objectEntityID == worldMode->UI->lockedInventoryID1 ||
                                      objectEntityID == worldMode->UI->lockedInventoryID2);
                    }
                    dest->ID = objectEntityID;
                    dest->layout = GetLayout(worldMode->table, taxonomy, false, false);
                    dest->taxonomy = taxonomy;
                    dest->status = (i16) objectEntity->status;
                    GetVisualProperties(&dest->properties, worldMode->table, taxonomy, recipeIndex, false, drawOpened);
                    dest->slot.slot = (SlotName) slotIndex;
                    
                    dest->drawModulated = (AreEqual(dest->slot, entityC->animation.output.focusSlots) ||
                                           AreEqual(dest->slot, entityC->animation.nearestCompatibleSlotForDragging));
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
            result.hashID = slot->skeletonHashID;
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
    u64 entityHashID;
    u64 skeletonHashID;
    Vec4 coloration;
    Vec2 originOffset;
};

inline GetAIDResult GetAID(Assets* assets, TaxonomyTable* taxTable, u32 taxonomy, u32 action, b32 drawOpened, u64 forcedNameHashID = 0)
{
    GetAIDResult result = {};
    result.coloration = V4(1, 1, 1, 1);
    
    ObjectState state = drawOpened ? ObjectState_Open : ObjectState_Ground;
    TaxonomySlot* slot = GetSlotForTaxonomy(taxTable, taxonomy);
    
    result.entityHashID = slot->stringHashID;
    
    SkeletonInfo skeletonInfo = GetSkeletonForTaxonomy(taxTable, slot);
    result.skeletonHashID = skeletonInfo.hashID;
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
        result.assetID = GetAssetIDForEntity(assets, taxTable, taxonomy, action);
        Assert(result.assetID);
        
        if(!result.skeletonHashID)
        {
            result.AID = GetAnimationRecursive(assets, taxTable, taxonomy, result.assetID, &result.entityHashID, state);
        }
        else
        {
            TagVector match = {};
            TagVector weight = {};
            result.AID = GetMatchingAnimation(assets, result.assetID, result.skeletonHashID, &match, &weight);
        }
    }
    
    return result;
}

internal AnimationOutput PlayAndDrawAnimation(GameModeWorld* worldMode, RenderGroup* group, Vec4 lightIndexes, ClientEntity* entityC, Vec2 scale, r32 angle, Vec3 offset, r32 timeToAdvance, Vec4 color, b32 drawOpened, b32 onTop, Rect2 bounds, r32 additionalZbias, AnimationDebugParams debugParams = {})
{
    AnimationOutput result = {};
    TaxonomyTable* taxTable = worldMode->table;
    AnimationState* animationState = &entityC->animation;
    animationState->cameInTime += timeToAdvance;
    
    AnimationFixedParams input;
    
    InitializeAnimationInputOutput(&input, &result, worldMode, entityC, timeToAdvance, debugParams.fakeEquipment);
    input.debug = debugParams;
    
    AnimationVolatileParams params;
    params.flipOnYAxis = animationState->flipOnYAxis;
    params.drawEmptySpaces = true;
    params.recipeIndex = 0;
    params.entityHashID = 0;
    params.additionalZbias = additionalZbias;
    params.modulationWithFocusColor = entityC->modulationWithFocusColor;
    params.cameraOffset = offset;
    params.lightIndexes = lightIndexes;
    params.color = color;
    params.angle = angle;
    params.scale = scale;
    params.zOffset = 0;
    params.properties = 0;
    
    if(IsObject(worldMode->table, entityC->taxonomy))
    {
        input.defaultColoration = V4(1, 1, 1, 1);
        ComponentsProperties properties;
        params.properties = &properties;
        b32 onGround = true;
        GetVisualProperties(&properties, taxTable, entityC->taxonomy, entityC->recipeIndex, onGround, drawOpened);
        
        ObjectLayout* layout = GetLayout(taxTable, entityC->taxonomy, onGround, drawOpened);
        if(layout)
        {
            Rect2 animationBounds = GetLayoutBounds(&input, group, layout, &params);
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
            
            RenderObjectLayout(&input, group, layout, entityC->P, &params);
        }
    }
    else
    {
        if(PushNewAction(animationState, entityC->action))
        {
            GetAIDResult prefetchAID = GetAID(group->assets, taxTable, entityC->taxonomy, animationState->nextAction, drawOpened);
            PrefetchAnimation(group->assets, prefetchAID.AID);
        }
        
        GetAIDResult AID = GetAID(group->assets, taxTable, entityC->taxonomy, animationState->action, drawOpened, debugParams.forcedNameHashID);
        
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
                params.entityHashID = AID.entityHashID;
                
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
                    animationState->bounds = animationBounds;
                }
                
                UpdateAndRenderAnimation(&input, group, animation, AID.skeletonHashID, entityC->P, animationState, &params, timeToAdvance);
            }
            else
            {
                LoadAnimation(group->assets, AID.AID);
            }
        }
    }
    
    
    return result;
}



inline Vec3 GetBiomeColorDelta(u8 biome, RandomSequence* seq)
{
    Vec3 result = {};
    switch(biome)
    {
        case Biome_forest:
        {
            //result.r = RandomBil(seq) * 0.01f;
            result.g = RandomBil(seq) * 0.01f;
            //result.b = RandomBil(seq) * 0.05f;
        } break;
    }
    
    return result;
}


inline Vec4 GetBiomeColor(u8 biome)
{
    Vec4 color = {};
    switch(biome)
    {
        case Biome_sea:
        {
            color = V4(0.05f, 0.24f,0.3f, 1.0f);
        } break;
        
        case Biome_beach:
        {
            color = V4(0.9f, 0.9f, 0.6f, 1.0f);
        } break;
        
        case Biome_forest:
        {
            color = V4(0.06f, 0.1f,  0.02f, 1.0f);
        } break;
        
        case Biome_desert:
        {
            color = V4(1.0f, 0.85f,  0.0f, 1.0f);
        } break;
        
        case Biome_mountain:
        {
            color = V4(0.35f, 0.35f,  0.25f, 1.0f);
        } break;
        
        InvalidDefaultCase;
    }
    
    return color;
}



inline Vec4 GetBiomeColor(WorldChunk* chunk, u8 tileX, u8 tileY, RandomSequence* seq)
{
    u8 biome = chunk->biomes[tileY][tileX];
    Vec4 color = GetBiomeColor(biome);
    color.rgb += GetBiomeColorDelta(biome, seq);
    color = SRGBLinearize(color);
    return color;
}


inline r32 InfluenceOffDistance(Vec2 borderP, Vec2 tileCenter, r32 chunkSide)
{
    Vec2 tileToP = borderP - tileCenter;
    r32 length = Length(tileToP);
    r32 result = 1.0f - (length / 0.5f * chunkSide);
    result = Max(result, 0);
    
    return result;
    
}

inline Vec4 InterpolateColor(GameModeWorld* worldMode, Vec4 baseColor, Vec2 toBorderP, Vec2 tileCenter, i32 chunkX, i32 chunkY, u32 otherChunkSubIndex)
{
    Vec4 result = {};
    WorldChunk* chunk = GetChunk(worldMode->chunks, ArrayCount(worldMode->chunks), chunkX, chunkY, 0);
    if(chunk)
    {
        r32 lerp = InfluenceOffDistance(toBorderP, tileCenter, worldMode->chunkSide);
        Vec4 otherSubChunkColor = chunk->subchunkColor[otherChunkSubIndex];
        result = Lerp(baseColor, lerp, otherSubChunkColor);
    }
    
    return result;
}










// NOTE(Leonardo): API
inline Vec4 ComputeTileColor(GameModeWorld* worldMode, WorldChunk* chunk, u8 tileX, u8 tileY, RandomSequence* seq)
{
    u8 biome = chunk->biomes[tileY][tileX];
    u32 X = chunk->worldX;
    u32 Y = chunk->worldY;
    
    u8 chunkDim = worldMode->chunkDim;
    u8 halfChunkDim = chunkDim / 2;
    r32 voxelSide = worldMode->voxelSide;
    Vec2 tileCenter = voxelSide * V2(tileX + 0.5f, tileY + 0.5f);
    
    Vec4 colorSide0 = {};
    Vec4 colorSide1 = {};
    Vec4 colorDiagonal = {};
    
    Vec4 biomeColor = GetBiomeColor(biome);
    if(tileX < halfChunkDim)
    {
        if(tileY < halfChunkDim)
        {
            Vec2 toLeft = V2(0, halfChunkDim * voxelSide);
            colorSide0 = InterpolateColor(worldMode, biomeColor, toLeft, tileCenter, X - 1, Y, 1);
            
            Vec2 toDown = V2(halfChunkDim * voxelSide, 0);
            colorSide1 = InterpolateColor(worldMode, biomeColor, toDown, tileCenter, X, Y - 1, 2);
            
            Vec2 toDownLeft = V2(0, 0);
            colorDiagonal = InterpolateColor(worldMode, biomeColor, toDownLeft, tileCenter, X - 1, Y - 1, 3);
        }
        else
        {
            Vec2 toLeft = V2(0, halfChunkDim * voxelSide);
            colorSide0 = InterpolateColor(worldMode, biomeColor, toLeft, tileCenter, X - 1, Y, 3);
            
            Vec2 toUp = V2(halfChunkDim * voxelSide, chunkDim * voxelSide);
            colorSide1 = InterpolateColor(worldMode, biomeColor, toLeft, tileCenter, X, Y + 1, 0);
            
            Vec2 toUpLeft = V2(0, chunkDim * voxelSide);
            colorDiagonal = InterpolateColor(worldMode, biomeColor, toUpLeft, tileCenter, X - 1, Y + 1, 1);
        }
    }
    else
    {
        if(tileY < halfChunkDim)
        {
            Vec2 toRight = V2(chunkDim * voxelSide, halfChunkDim * voxelSide);
            colorSide0 = InterpolateColor(worldMode, biomeColor, toRight, tileCenter, X + 1, Y, 0);
            
            Vec2 toDown = V2(halfChunkDim * voxelSide, 0);
            colorSide1 = InterpolateColor(worldMode, biomeColor, toDown, tileCenter, X, Y - 1, 3);
            
            Vec2 toDownRight = V2(0, 0);
            colorDiagonal = InterpolateColor(worldMode, biomeColor, toDownRight, tileCenter, X + 1, Y - 1, 2);
        }
        else
        {
            Vec2 toRight = V2(chunkDim * voxelSide, halfChunkDim * voxelSide);
            colorSide0 = InterpolateColor(worldMode, biomeColor, toRight, tileCenter, X + 1, Y, 2);
            
            Vec2 toUp = V2(halfChunkDim * voxelSide, chunkDim * voxelSide);
            colorSide1 = InterpolateColor(worldMode, biomeColor, toRight, tileCenter, X, Y + 1, 1);
            
            Vec2 toUpRight = voxelSide * V2(chunkDim, chunkDim);
            colorDiagonal = InterpolateColor(worldMode, biomeColor, toUpRight, tileCenter, X + 1, Y + 1, 0);
        }
    }
    
    Vec4 averageOther = 0.33f * (colorSide0 + colorSide1 + colorDiagonal);
    Vec4 myColor = GetBiomeColor(chunk, tileX, tileY, seq);
    r32 influence = chunk->influences[tileY][tileX];
    Vec4 result = Lerp(myColor, influence, averageOther);
    return result;
}

inline void RenderWater(RenderGroup* group, Vec2 P, TileInfo tile, TileInfo adiacent, Vec3 lineBetweenStart, Vec3 lineBetweenEnd, b32 isOnLeftSide, r32 waterLevel)
{
    Vec4 waterColor = V4(0, 0.1f, 1, 0.6f);
    if(adiacent.biome == Biome_forest)
    {
        lineBetweenStart.z = waterLevel;
        lineBetweenEnd.z = waterLevel;
    }
    
    if(isOnLeftSide)
    {
        PushTriangle(group, group->whiteTexture, tile.lightIndexes, 
                     V4(P, waterLevel, 0), waterColor, 
                     V4(lineBetweenStart, 0), waterColor,
                     V4(lineBetweenEnd, 0), waterColor, 0);
    }
    else
    {
        PushTriangle(group, group->whiteTexture, tile.lightIndexes, 
                     V4(P, waterLevel, 0), waterColor, 
                     V4(lineBetweenEnd, 0), waterColor,
                     V4(lineBetweenStart, 0), waterColor, 0);
    }
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
                        
                        for(u32 pickIndex = 0; pickIndex < pick.soundCount; ++pickIndex)
                        {
                            SoundId toPlay = pick.sounds[pickIndex];
                            r32 delay = pick.delays[pickIndex];
                            
                            if(IsValid(toPlay))
                            {
                                PlaySound(worldMode->soundState, toPlay, delay);
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

inline TileInfo GetTileInfo(GameModeWorld* worldMode, UniversePos baseP, Vec2 P);
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
    
    
    
    Rect3 bounds = InvertedInfinityRect3();
    AnimationOutput result = {};
    
    ClientEntity* player = GetEntityClient(worldMode, myPlayer->identifier);
    TileInfo tileInfo = GetTileInfo(worldMode, player->universeP, entityC->P.xy);
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
    
    if(IsPlant(worldMode->table, entityC->taxonomy))
    {
        r32 totalHeight = slot->plantBaseParams.maxZeroLevelSegmentNumber * slot->plantBaseParams.maxRootSegmentLength;
        r32 radious = slot->plantBaseParams.maxRootSegmentRadious;
        
        bounds = Offset(slot->physicalBounds, entityC->P);
        Assert(slot->plantParams);
        UpdateAndRenderPlant(worldMode, group, tileInfo, entityC, slot->plantParams, timeToUpdate);
    }
    else if(IsRock(worldMode->table, entityC->taxonomy))
    {
        if(!entityC->rock)
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
            newRock->dim = V3(RandomUni(&rockSeq), RandomUni(&rockSeq), RandomUni(&rockSeq));
            
            MemoryPool tempPool = {};
            TempMemory rockMemory = BeginTemporaryMemory(&tempPool);
            GenerateRock(newRock, &worldMode->tetraModel, 2, &tempPool, &rockSeq, V4(0.02f, 0.02f, 0.02f, 1.0f));
            EndTemporaryMemory(rockMemory);
            
        }
        
        ClientRock* rock = entityC->rock;
        VertexModel onTheFly;
        
        onTheFly.vertexCount = rock->vertexCount;
        onTheFly.vertexes = rock->vertexes;
        
        onTheFly.faceCount = rock->faceCount;
        onTheFly.faces = rock->faces;
        
        PushModel(group, &onTheFly, entityC->P, tileInfo.lightIndexes, rock->dim, V4(1, 1, 1, 1), entityC->modulationWithFocusColor);
        
        bounds = RectCenterDim(entityC->P, rock->dim);
    }
    else
    {
        u32 boundTaxonomy = slot->taxonomy;
        while(boundTaxonomy)
        {
            TaxonomySlot* boundSlot = GetSlotForTaxonomy(worldMode->table, boundTaxonomy);
            if(boundSlot->boundType)
            {
                bounds = Offset(boundSlot->physicalBounds, entityC->P);
                break;
            }
            boundTaxonomy = GetParentTaxonomy(worldMode->table, boundTaxonomy);
        }
        
        Vec2 animationScale = params.scale * V2(0.33f, 0.33f);
        r32 additionalZbias = params.additionalZbias;
        if(params.onTop)
        {
            additionalZbias = 5.0f;
        }
        
        additionalZbias += 0.4f * GetDim(entityC->animation.bounds).y;
        
        oldSoundTime = entityC->animation.normalizedTime;
        
        EquipInfo dragging = entityC->animation.nearestCompatibleSlotForDragging;
        if(IsValid(dragging))
        {
            u64 ID = worldMode->UI->draggingEntity.identifier;
            MarkAllSlotsAsOccupied(entityC->equipment, dragging, ID);
        }
        
        result = PlayAndDrawAnimation(worldMode, group, tileInfo.lightIndexes, entityC, animationScale, params.angle, params.offset, timeToUpdate, bodyColor, params.drawOpened, params.onTop, params.bounds, additionalZbias);
        
        if(IsValid(dragging))
        {
            MarkAllSlotsAsNull(entityC->equipment, dragging);
        }
        
        soundAction = (EntityAction) entityC->animation.action;
        soundTime = entityC->animation.normalizedTime;
    }
    
    
    Rect3 ignored;
    GetPhysicalProperties(worldMode->table, entityC->taxonomy, entityC->identifier, &entityC->boundType, &ignored);
    entityC->bounds = bounds;
    //PushCubeOutline(group, bounds, V4(1, 1, 1, 1), 0.05f);
    
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
    clientEntity.recipeIndex = object->recipeIndex;
    clientEntity.P = P;
    clientEntity.status = object->status;
    
    AnimationEntityParams params = StandardEntityParams();
    params.bounds = RectCenterDim(V2(0, 0), dim);
    params.additionalZbias = additionalZbias;
    RenderEntity(group, worldMode, &clientEntity, 0, params);
    
    Rect2 result = clientEntity.animation.bounds;
    return result;
}


