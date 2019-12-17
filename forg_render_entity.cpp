internal b32 ShouldBeRendered(GameModeWorld* worldMode, BaseComponent* base)

{
    b32 result = false;
    if(base)
    {
        result = ((base->universeP.chunkZ == worldMode->player.universeP.chunkZ) &&
                  !(base->flags & EntityFlag_notInWorld) &&
                  !(base->flags & EntityFlag_occluding));
        
        r32 maxLengthSq = Square(3 * (r32) CHUNK_DIM);
        if(result && LengthSq(SubtractOnSameZChunk(base->universeP, worldMode->player.universeP)) > maxLengthSq)
        {
            result = false;
        }
    }
    return result;
}

internal EntityAnimationParams GetEntityAnimationParams(GameModeWorld* worldMode, EntityID ID)
{
    AnimationEffectComponent* animation = GetComponent(worldMode, ID, AnimationEffectComponent);
    EntityAnimationParams result = animation ? animation->params : DefaultAnimationParams();
    
    return result;
}

internal void RenderShadow(GameModeWorld* worldMode, RenderGroup* group, Vec3 P, ShadowComponent* shadowComponent, r32 deepness, r32 width)
{
    Lights lights = GetLights(worldMode, P);
    RandomSequence ignored = Seed(1);
    BitmapId shadowID = QueryBitmaps(group->assets, GetAssetSubtype(group->assets, AssetType_Image, "shadow"), &ignored, 0);
    if(IsValid(shadowID))
    {
        Bitmap* shadow = GetBitmap(group->assets, shadowID).bitmap;
        if(shadow)
        {
            r32 height = shadowComponent->height;
            r32 nativeDeepness = height;
            r32 nativeWidth = height * shadow->widthOverHeight;
            
            r32 yScale = deepness / nativeDeepness;
            r32 xScale = width / nativeWidth;
            
            ObjectTransform transform = FlatTransform();
            transform.scale = Hadamart(shadowComponent->scale, V2(xScale, yScale));
            transform.tint = shadowComponent->color;
            PushBitmap(group, transform, shadowID, P + shadowComponent->offset, 0, lights);
        }
        else
        {
            LoadBitmap(group->assets, shadowID);
        }
    }
}

internal GameProperty SearchForProperty(GameProperty* properties, u32 propertyCount, u16 propertyType)
{
    GameProperty result = {};
    for(u32 propertyIndex = 0; propertyIndex < propertyCount; ++propertyIndex)
    {
        GameProperty* test = properties + propertyIndex;
        if(test->property == propertyType)
        {
            result = *test;
            break;
        }
    }
    
    return result;
}

RENDERING_ECS_JOB_CLIENT(RenderCharacterAnimation)
{
    EntityAnimationParams animationParams = GetEntityAnimationParams(worldMode, ID);
    elapsedTime *= animationParams.speed;
    
    BaseComponent* base = GetComponent(worldMode, ID, BaseComponent);
    if(ShouldBeRendered(worldMode, base))
    {
        EntityDefinition* definition = GetData(group->assets, EntityDefinition, EntityRefToAssetID(base->definitionID));
        
        r32 height = GetHeight(base->bounds);
        
        Vec3 P = GetRelativeP(worldMode, base);
        AnimationComponent* animation = GetComponent(worldMode, ID, AnimationComponent);
        
        if(Abs(base->velocity.x) > 0.1f)
        {
            animation->flipOnYAxis = (base->velocity.x < 0.0f);
        }
        Clear(&animation->skeletonProperties);
        
        AnimationParams params = {};
        params.elapsedTime = elapsedTime * animation->speed;
        params.angle = 0;
        params.P = P;
        params.lights = GetLights(worldMode, P);
        params.scale = 1;
        params.transform = BillboardTransform();
        params.flipOnYAxis = animation->flipOnYAxis;
        params.equipment = GetComponent(worldMode, ID, EquipmentMappingComponent);
        params.equipped = GetComponent(worldMode, ID, UsingMappingComponent);
        params.tint = animationParams.tint;
        params.dissolveCoeff = animationParams.dissolveCoeff;
        params.modulationPercentage = animationParams.modulationPercentage; 
        params.replacementCount = definition->client.replacementCount;
        params.replacements = definition->client.animationReplacements;
        params.ID = ID;
        
        GameProperty action =  SearchForProperty(base->properties, ArrayCount(base->properties), Property_action);
        if(FinishedSingleCycleAnimation(animation))
        {
            params.fakeAnimation = true;
        }
        
        AddOptionalGamePropertyRaw(&animation->skeletonProperties, action);
        
        if(!animation->defaultScaleComputed || action.value == idle)
        {
            Rect2 animationDefaultDim = GetAnimationDim(worldMode, group, animation, &params);
            if(HasArea(animationDefaultDim))
            {
                r32 defaultHeight = GetDim(animationDefaultDim).y;
                animation->scale = height / defaultHeight;
            }
            
            if(action.value == idle)
            {
                animation->defaultScaleComputed = true;
            }
            
        }
        
        params.scale = animation->scale;
        RenderAnimationAndTriggerSounds(worldMode, group, animation, &params);
    }
}

internal BitmapId GetImageFromSubtype(Assets* assets, u64 typeHash, GameProperties* properties, RandomSequence* seq)
{
    u32 imageType = GetAssetSubtype(assets, AssetType_Image, typeHash);
    BitmapId result = QueryBitmaps(assets, imageType, seq, properties);
    return result;
}

internal BitmapId GetImageFromReference(Assets* assets, ImageReference* reference, RandomSequence* seq)
{
    BitmapId result = GetImageFromSubtype(assets, reference->typeHash, &reference->properties, seq);
    return result;
}

internal BitmapId GetCorrenspondingFrameByFrameImage(Assets* assets, u64 typeHash, r32 time)
{
    u32 imageType = GetAssetSubtype(assets, AssetType_Image, typeHash);
    u32 frameCount = GetAssetCount(assets, AssetType_Image, imageType);
    
    u16 frameIndex = SafeTruncateToU16(TruncateReal32ToU32(Mod(time, (r32)frameCount)));
    GameProperties properties = {};
    AddGameProperty(&properties, frameIndex, frameIndex);
    BitmapId result = QueryBitmaps(assets, imageType, 0, &properties);
    return result;
}

RENDERING_ECS_JOB_CLIENT(RenderShadow)
{
    BaseComponent* base = GetComponent(worldMode, ID, BaseComponent);
    ShadowComponent* shadow = GetComponent(worldMode, ID, ShadowComponent);
    Vec3 P = GetRelativeP(worldMode, base);
    r32 deepness = GetWidth(base->bounds);
    r32 width = GetWidth(base->bounds);
    
    RenderShadow(worldMode, group, P, shadow, deepness, width);
}

internal r32 GetDissolveCoeff(r32 density, u32 index)
{
    RandomSequence seq = Seed(index);
    r32 startDissolving = RandomUni(&seq);
    r32 endDissolving = startDissolving - 0.05f;
    endDissolving = Max(endDissolving, 0);
    
    r32 result = 1.0f - Clamp01MapToRange(endDissolving, density, startDissolving);
    
    return result;
}

RENDERING_ECS_JOB_CLIENT(RenderPlants)
{
    u64 branchHash = StringHash("branch");
    u64 leafHash = StringHash("leaf");
    u64 flowerHash = StringHash("flower");
    u64 fruitHash = StringHash("fruit");
    
    EntityAnimationParams params = GetEntityAnimationParams(worldMode, ID);
    elapsedTime *= params.speed;
    
    BaseComponent* base = GetComponent(worldMode, ID, BaseComponent);
    if(ShouldBeRendered(worldMode, base))
    {
        r32 height = GetHeight(base->bounds);
        Vec3 P = GetRelativeP(worldMode, base);
        Lights lights = GetLights(worldMode, P);
        StandardImageComponent* image = GetComponent(worldMode, ID, StandardImageComponent);
        
        PlantComponent* plant = GetComponent(worldMode, ID, PlantComponent);
        
        RandomSequence seq = Seed(base->seed);
        BitmapId trunkID = GetImageFromReference(group->assets, &plant->trunk, &seq);
        if(IsValid(trunkID))
        {  
            ObjectTransform transform = BillboardTransform();
            transform.dissolvePercentages = params.dissolveCoeff * V4(1, 1, 1, 1);
            transform.modulationPercentage = params.modulationPercentage;
            transform.tint = params.tint;
            
            BitmapDim bitmapData = PushBitmap(group, transform, trunkID, P, height, lights);
            
            Bitmap* bitmap = GetBitmap(group->assets, trunkID).bitmap;
            PAKBitmap* bitmapInfo = GetBitmapInfo(group->assets, trunkID);
            if(bitmap)
            {
                BitmapId leafID = GetImageFromReference(group->assets, &plant->leaf, &seq);
                BitmapId flowerID = GetImageFromReference(group->assets, &plant->flower, &seq);
                BitmapId fruitID = GetImageFromReference(group->assets, &plant->fruit, &seq);
                
                u32 leafRunningIndex = 0;
                u32 flowerRunningIndex = 0;
                u32 fruitRunningIndex = 0;
                
                for(u32 branchPointIndex = 0;
                    branchPointIndex < bitmapInfo->attachmentPointCount; ++branchPointIndex)
                {
                    PAKAttachmentPoint* branch = bitmap->attachmentPoints + branchPointIndex;
                    if(StringHash(branch->name) == branchHash)
                    {
                        BitmapId branchID = GetImageFromReference(group->assets, &plant->branch, &seq);
                        if(IsValid(branchID))
                        {
                            Bitmap* branchBitmap = GetBitmap(group->assets, branchID).bitmap;
                            if(branchBitmap)
                            {
                                PAKBitmap* branchInfo = GetBitmapInfo(group->assets, branchID);
                                Vec2 branchPivot = V2(branchInfo->align[0], branchInfo->align[1]);
                                Vec2 branchInvUV = GetInvUV(branchBitmap->width, branchBitmap->height);
								Vec3 branchP = GetAlignP(bitmapData, branch->alignment);
                                
								r32 angleRad = DegToRad(branch->angle);
								Vec3 XAxis = V3(Cos(angleRad), Sin(angleRad), 0.0f);
								Vec3 YAxis  = V3(Perp(XAxis.xy), 0.0f);
                                
								Vec3 bLateral =branch->scale.x * (XAxis.x * magicLateralVector + XAxis.y * magicUpVector);
								Vec3 bUp = branch->scale.y * (YAxis.x * magicLateralVector + YAxis.y * magicUpVector);
                                
								Vec3 pivotOffset = branchPivot.x * bLateral + branchPivot.y * bUp; 
								branchP -= pivotOffset;
                                branchP += (0.5f * bLateral + 0.5f * bUp);
                                
                                u32 C = 0xffffffff;
                                Vec4 windInfluences = V4(0, 0, 0, 0);
                                u8 windFrequency = 1;
                                u8 seed = (u8) branchPointIndex;
                                Vec4 dissolvePercentages = params.dissolveCoeff * V4(1, 1, 1, 1);
                                r32 alphaThreesold = 0;
                                b32 flat = false;
                                
                                PushMagicQuad(group, branchP, flat, 0.5f * bLateral, 0.5f * bUp, branchInvUV, C, branchBitmap->textureHandle, lights, params.modulationPercentage, 0, 0, windInfluences, windFrequency, dissolvePercentages, alphaThreesold, seed);
                                
                                Vec3 branchMin = branchP - 0.5f * bLateral - 0.5f * bUp;
                                
                                for(u32 leafPointIndex = 0;
                                    leafPointIndex < branchInfo->attachmentPointCount; ++leafPointIndex)
                                {
                                    PAKAttachmentPoint* leaf = branchBitmap->attachmentPoints + leafPointIndex;
                                    
                                    BitmapId LFF = {};
                                    
                                    r32 dissolveCoeff = 0;
                                    if(StringHash(leaf->name) == leafHash)
                                    {
                                        LFF = leafID;
                                        dissolveCoeff = GetDissolveCoeff(plant->leafDensity, leafRunningIndex++);
                                    }
                                    else if(StringHash(leaf->name) == flowerHash)
                                    {
                                        LFF = flowerID;
                                        dissolveCoeff = GetDissolveCoeff(plant->flowerDensity, flowerRunningIndex++);
                                    }
                                    else if(StringHash(leaf->name) == fruitHash)
                                    {
                                        LFF = fruitID;
                                        dissolveCoeff = GetDissolveCoeff(plant->fruitDensity, fruitRunningIndex++);
                                    }
                                    
                                    if(IsValid(LFF))
                                    {
                                        Bitmap* LFFBitmap = GetBitmap(group->assets, LFF).bitmap;
                                        if(LFFBitmap)
                                        {
                                            PAKBitmap* leafInfo = GetBitmapInfo(group->assets, LFF);
                                            Vec2 leafPivot = V2(leafInfo->align[0], leafInfo->align[1]);
                                            Vec2 leafInvUV = GetInvUV(LFFBitmap->width, LFFBitmap->height);
                                            
                                            Vec3 leafP = branchMin + leaf->alignment.x * bLateral + leaf->alignment.y * bUp;
                                            
                                            angleRad = DegToRad(leaf->angle);
                                            XAxis = V3(Cos(angleRad), Sin(angleRad), 0.0f);
                                            YAxis  = V3(Perp(XAxis.xy), 0.0f);
                                            
                                            Vec3 lateral =leaf->scale.x * (XAxis.x * magicLateralVector + XAxis.y * magicUpVector);
                                            Vec3 up =leaf->scale.y * (YAxis.x * magicLateralVector + YAxis.y * magicUpVector);
                                            
                                            
                                            pivotOffset = leafPivot.x * lateral + leafPivot.y * up; 
                                            leafP -= pivotOffset;
                                            leafP += (0.5f * lateral + 0.5f * up);
                                            
                                            C = 0xffffffff;
                                            
                                            windInfluences = V4(0, 0, plant->windInfluence, plant->windInfluence);
                                            windFrequency = 1;
                                            seed = (u8) leafPointIndex;
                                            dissolvePercentages = dissolveCoeff * V4(1, 1, 1, 1);
                                            alphaThreesold = 0;
                                            flat = false;
                                            
                                            PushMagicQuad(group, leafP, flat, 0.5f * lateral, 0.5f * up, leafInvUV, C, LFFBitmap->textureHandle, lights, params.modulationPercentage, 0, 0, windInfluences, windFrequency, dissolvePercentages, alphaThreesold, seed);
                                        }
                                        else
                                        {
                                            LoadBitmap(group->assets, LFF);
                                        }
                                    }
                                }
                            }
                        }
                        else
                        {
                            LoadBitmap(group->assets, branchID);
                        }
                    }
                }
            }
        }
    }
}

internal void RenderStaticAnimation(GameModeWorld* worldMode, RenderGroup* group, EntityAnimationParams params, BaseComponent* base, ImageReference* image, r32 elapsedTime)
{
    r32 height = GetHeight(base->bounds);
    Vec3 P = GetRelativeP(worldMode, base);
    Lights lights = GetLights(worldMode, P);
    
    RandomSequence seq = Seed(base->seed);
    BitmapId BID = GetImageFromReference(group->assets, image, &seq);
    if(IsValid(BID))
    {
        ObjectTransform transform = BillboardTransform();
        transform.modulationPercentage = params.modulationPercentage; 
        transform.dissolvePercentages = params.dissolveCoeff * V4(1, 1, 1, 1); 
        transform.tint = params.tint;
        PushBitmap(group, transform, BID, P, height, lights);
    }
}

internal void RenderSegmentImage(GameModeWorld* worldMode, RenderGroup* group, EntityAnimationParams params, BaseComponent* base, ImageReference* image, r32 elapsedTime)
{
    r32 height = GetHeight(base->bounds);
    r32 width = GetWidth(base->bounds);
    
    Vec3 P = GetRelativeP(worldMode, base);
    Lights lights = GetLights(worldMode, P);
    
    RandomSequence seq = Seed(base->seed);
    BitmapId BID = GetImageFromReference(group->assets, image, &seq);
    if(IsValid(BID))
    {
        r32 modulationPercentage = params.modulationPercentage; 
        Vec4 dissolvePercentages = params.dissolveCoeff * V4(1, 1, 1, 1); 
        
        Vec3 speed = Normalize(base->velocity);
        
        Vec3 fromP = P + speed * width;
        Vec3 toP = P - speed * width;
        
        PushTextureSegment(group, BID, params.tint, fromP, toP, height, lights);
    }
}


RENDERING_ECS_JOB_CLIENT(RenderSpriteEntities)
{
    EntityAnimationParams params = GetEntityAnimationParams(worldMode, ID);
	elapsedTime *= params.speed;
    BaseComponent* base = GetComponent(worldMode, ID, BaseComponent);
    
    if(ShouldBeRendered(worldMode, base))
    {
        StandardImageComponent* image = GetComponent(worldMode, ID, StandardImageComponent);
        RenderStaticAnimation(worldMode, group, params, base, &image->entity, elapsedTime);
    }
}

RENDERING_ECS_JOB_CLIENT(RenderSegmentEntities)
{
    EntityAnimationParams params = GetEntityAnimationParams(worldMode, ID);
	elapsedTime *= params.speed;
    BaseComponent* base = GetComponent(worldMode, ID, BaseComponent);
    
    if(ShouldBeRendered(worldMode, base))
    {
        SegmentImageComponent* image = GetComponent(worldMode, ID, SegmentImageComponent);
        RenderSegmentImage(worldMode, group, params, base, &image->entity, elapsedTime);
    }
}

RENDERING_ECS_JOB_CLIENT(RenderGrass)
{
    BaseComponent* base = GetComponent(worldMode, ID, BaseComponent);
    MagicQuadComponent* quad = GetComponent(worldMode, ID, MagicQuadComponent);
    GrassComponent* grass = GetComponent(worldMode, ID, GrassComponent);
    
    if(ShouldBeRendered(worldMode, base))
    {
        if(IsValid(quad->bitmapID))
        {
            Bitmap* bitmap = GetBitmap(group->assets, quad->bitmapID).bitmap;
            if(bitmap)
            {
                RandomSequence seq = Seed(base->seed);
				Vec3 P = GetRelativeP(worldMode, base->universeP);
                Lights lights = GetLights(worldMode, P);
                P -= quad->offset;
                
                BaseComponent* playerBase = GetComponent(worldMode, worldMode->player.clientID, BaseComponent);
                
                u8 windFrequency = (u8) grass->windFrequencyStandard;
                if(RectOverlaps(Offset(grass->bounds, P), playerBase->bounds) 
                   && LengthSq(playerBase->velocity) > 0.1f
                   )
                {
                    windFrequency = (u8) grass->windFrequencyOverlap;
                }
                Vec4 windInfluences = V4(0, 0, grass->windInfluence, grass->windInfluence);
                windInfluences = V4(0, 0, 0, 0);
                Vec4 dissolvePercentages = V4(0, 0, 0, 0);
                r32 alphaThreesold = quad->alphaThreesold;
                b32 flat = false;
				for(u32 grassIndex = 0; grassIndex < grass->count; ++grassIndex)
				{
					Vec3 grassP = P + Hadamart(RandomBilV3(&seq), grass->maxOffset);
					PushMagicQuad(group, grassP, flat, quad->lateral, quad->up, quad->invUV, quad->color, bitmap->textureHandle, lights, 0, 0, 0, windInfluences, windFrequency, dissolvePercentages, alphaThreesold, (u8) grassIndex);
				}
            }
            else
            {
                LoadBitmap(group->assets, quad->bitmapID);
            }
        }
    }
}

internal void RenderFrameByFrameAnimation(GameModeWorld* worldMode, RenderGroup* group, EntityAnimationParams params, BaseComponent* base, FrameByFrameAnimationComponent* animation, r32 elapsedTime)
{
    elapsedTime *= params.speed;
    r32 height = GetHeight(base->bounds);
    
    Vec3 P = GetRelativeP(worldMode, base);
    Lights lights = GetLights(worldMode, P);
    
    r32 stepTime = elapsedTime * animation->speed;
    animation->runningTime += stepTime;
    
    for(u32 advanceIndex = 0; advanceIndex < 5; ++advanceIndex)
    {
        BitmapId nextID = GetCorrenspondingFrameByFrameImage(group->assets, animation->typeHash, animation->runningTime + advanceIndex * stepTime);
        LoadBitmap(group->assets, nextID);
    }
    
    BitmapId BID = GetCorrenspondingFrameByFrameImage(group->assets, animation->typeHash, animation->runningTime);
    if(IsValid(BID))
    {
        ObjectTransform transform = BillboardTransform();
        transform.modulationPercentage = params.modulationPercentage; 
        transform.dissolvePercentages = params.dissolveCoeff * V4(1, 1, 1, 1);
        transform.tint = params.tint;
        
        if(animation->overridesPivot)
        {
            PushBitmapWithPivot(group, transform, BID, P, animation->pivot, height, lights);
        }
        else
        {
            PushBitmap(group, transform, BID, P, height, lights);
        }
    }
}

RENDERING_ECS_JOB_CLIENT(RenderFrameByFrameEntities)
{
    BaseComponent* base = GetComponent(worldMode, ID, BaseComponent);
    EntityAnimationParams params = GetEntityAnimationParams(worldMode, ID);
    if(ShouldBeRendered(worldMode, base))
    {
        FrameByFrameAnimationComponent* animation = GetComponent(worldMode, ID, FrameByFrameAnimationComponent);
		RenderFrameByFrameAnimation(worldMode, group, params, base, animation, elapsedTime);
    }
}

RENDERING_ECS_JOB_CLIENT(RenderMultipartEntity)
{
    BaseComponent* base = GetComponent(worldMode, ID, BaseComponent);
    EntityAnimationParams params = GetEntityAnimationParams(worldMode, ID);
    if(ShouldBeRendered(worldMode, base))
    {
        MultipartAnimationComponent* multipart = GetComponent(worldMode, ID, MultipartAnimationComponent);
		for(u32 staticIndex = 0; staticIndex < multipart->staticCount; ++staticIndex)
		{
            ImageReference* staticImage = multipart->staticParts + staticIndex;
            RenderStaticAnimation(worldMode, group, params, base, staticImage, elapsedTime);
		}
        
        for(u32 frameByFrameIndex = 0;
            frameByFrameIndex < multipart->frameByFrameCount; 
            ++frameByFrameIndex)
		{
            FrameByFrameAnimationComponent* frameByFrame = multipart->frameByFrameParts + frameByFrameIndex;
            RenderFrameByFrameAnimation(worldMode, group, params, base, frameByFrame, elapsedTime);
		}
    }
}

internal void RenderLayoutInRectCameraAligned(GameModeWorld* worldMode, RenderGroup* group, Vec3 P, Vec3 rectP, Rect2 cameraRect, ObjectTransform transform, LayoutComponent* layout, u32 seed, Lights lights, LayoutContainer* container, r32 elapsedTime);
internal void RenderLayoutInRect(GameModeWorld* worldMode, RenderGroup* group, Rect2 rect, ObjectTransform transform, LayoutComponent* layout, u32 seed, Lights lights, LayoutContainer* container, r32 elapsedTime, r32 zOffset = 0.0f);
internal void OverdrawEssence(GameModeWorld* worldMode, RenderGroup* group, ObjectTransform transform, u16 essence, Rect2 rect, Vec4 color);
internal void DrawObjectMapping(GameModeWorld* worldMode, RenderGroup* group, ObjectMapping* mapping, ObjectTransform transform, Vec3 P, BitmapDim spaceDim, Rect2 rect, Lights lights, r32 elapsedTime)
{
    if(IsValidInventoryMapping(&worldMode->gameUI, mapping))
    {
        EntityID objectID = mapping->object.ID;
        BaseComponent* objectBase = GetComponent(worldMode, objectID, BaseComponent);
        EntityAnimationParams params = GetEntityAnimationParams(worldMode, objectID);
        transform.modulationPercentage = params.modulationPercentage;
        transform.dissolvePercentages = params.dissolveCoeff * V4(1, 1, 1, 1);
        LayoutComponent* objectLayout = GetComponent(worldMode, objectID, LayoutComponent);
        
        if(objectBase && objectLayout)
        {
            LayoutContainer objectContainer = {};
            objectContainer.container = GetComponent(worldMode, objectID, ContainerMappingComponent);
            objectContainer.recipeEssences = GetComponent(worldMode, objectID, RecipeEssenceComponent);
            
            if(transform.upright)
            {
                RenderLayoutInRectCameraAligned(worldMode, group, P, spaceDim.P, rect, transform, objectLayout, objectBase->seed, lights, &objectContainer, elapsedTime);
            }
            else
            {
                RenderLayoutInRect(worldMode, group, rect, transform, objectLayout, objectBase->seed, lights, &objectContainer, elapsedTime);
            }
        }
    }
}

internal void DrawRecipe(GameModeWorld* worldMode, RenderGroup* group, u32 recipeSeed, ObjectTransform transform, Vec3 P, BitmapDim spaceDim, Rect2 rect, Lights lights, RecipeEssenceComponent* essences)
{
    transform.modulationPercentage = 0;
    u16 recipeEssences[Count_essence] = {};
    if(essences)
    {
        Vec2 startingSlotP = rect.min;
        Rect2 essenceRect = RectMinDim(startingSlotP, 0.5f * GetDim(rect));
        
        for(u32 slotIndex = 0; slotIndex < ArrayCount(essences->essences); ++slotIndex)
        {
            essences->projectedOnScreen[slotIndex] = essenceRect;
            
            u16 essence = essences->essences[slotIndex];
            
            BaseComponent* playerBase = GetComponent(worldMode, worldMode->player.clientID, BaseComponent);
            
            Vec4 color = V4(1, 1, 1, 1);
            if(essence < Count_essence && playerBase->essences[essence] > 0)
            {
            }
            else
            {
                color = V4(1, 1, 1, 0.1f);
            }
            
            OverdrawEssence(worldMode, group, transform, essence, essenceRect, color);
            ++recipeEssences[essence];
        }
    }
    
    Vec2 recipeDim = 0.5f * GetDim(rect);
    Vec2 recipeMin = V2(rect.min.x, rect.min.y + recipeDim.y);
    Rect2 recipeRect = RectMinDim(recipeMin, recipeDim);
    
    LayoutComponent layout = {};
    EntityRef type = GetCraftingType(group->assets, recipeSeed);
    EntityDefinition* definition = GetEntityTypeDefinition(group->assets, type);
    
    
    CommonEntityInitParams common = definition->common;
    common.definitionID = type;
    common.essences = recipeEssences;
    ClientEntityInitParams entityParams = definition->client;
    entityParams.seed = recipeSeed;
    
    InitLayoutComponent(worldMode, &layout, {}, &common, 0, &entityParams);
    LayoutContainer recipeContainer = {};
    
    if(transform.upright)
    {
        RenderLayoutInRectCameraAligned(worldMode, group, P, spaceDim.P, recipeRect, transform, &layout, recipeSeed, lights, &recipeContainer, 0);
    }
    else
    {
        RenderLayoutInRect(worldMode, group, recipeRect, transform, &layout, recipeSeed, lights, &recipeContainer, 0, 0);
    }
}



internal ObjectMapping* FindCompatibleMapping(ObjectMapping* mappings, u32 mappingCount, b32* alreadyDrawn, u32 alreadyDrawnCount, u16 inventorySlotType)
{
    Assert(mappingCount == alreadyDrawnCount);
    
    ObjectMapping* result = 0;
    for(u32 mappingIndex = 0; mappingIndex < mappingCount; ++mappingIndex)
    {
        ObjectMapping* mapping = mappings + mappingIndex;
        InventorySlot* slot = &mapping->object;
        
        u16 slotType = SafeTruncateToU16(slot->flags_type & 0xffff);
        if(slotType == inventorySlotType)
        {
            if(!alreadyDrawn[mappingIndex])
            {
                alreadyDrawn[mappingIndex] = true;
                result = mapping;
                break;
            }
        }
    }
    
    return result;
}

internal Rect2 RenderLayoutRecursive_(GameModeWorld* worldMode, RenderGroup* group, Vec3 P, ObjectTransform transform, LayoutComponent* layout, u64 nameHash, u32 seed, Lights lights, LayoutContainer* container, r32 elapsedTime)
{
    u64 emptySpaceHash = StringHash("emptySpace");
    u64 usingSpaceHash = StringHash("usingSpace");
    u64 recipeSpaceHash = StringHash("recipeSpace");
    
    LayoutPiece* pieces = 0;
    u32 pieceCount = 0;
    
    switch(container->drawMode)
    {
        case LayoutContainerDraw_Standard:
        {
            pieces = layout->pieces;
            pieceCount = layout->pieceCount;
        } break;
        
        case LayoutContainerDraw_Open:
        {
            pieces = layout->openPieces;
            pieceCount = layout->openPieceCount;
        } break;
        
        case LayoutContainerDraw_Using:
        {
            pieces = layout->usingPieces;
            pieceCount = layout->usingPieceCount;
        } break;
        
        case LayoutContainerDraw_Equipped:
        {
            pieces = layout->equippedPieces;
            pieceCount = layout->equippedPieceCount;
        } break;
    }
    
    if(pieceCount == 0)
    {
        pieces = layout->pieces;
        pieceCount = layout->pieceCount;
    }
    
    Rect2 result = InvertedInfinityRect2();
    RandomSequence seq = Seed(seed);
    for(u32 pieceIndex = 0; pieceIndex < pieceCount; ++pieceIndex)
    {
        LayoutPiece* piece = pieces + pieceIndex;
        BitmapId BID = GetImageFromReference(group->assets, &piece->image, &seq);
        if(piece->nameHash == nameHash)
        {
            if(IsValid(BID))
            {
                BitmapDim dim = GetBitmapDim(group, transform, BID, P, piece->height);
                if(transform.upright)
                {
                    result = ProjectOnScreen(group, dim);
                }
                else
                {
                    result = RectMinDim(dim.P.xy, dim.size);
                }
                
                PushBitmap(group, transform, BID, P, piece->height, lights);
                if(container->container)
                {
                    ContainerMappingComponent* c = container->container;
                    if(piece->nameHash == emptySpaceHash)
                    {
                        ObjectMapping* mapping = FindCompatibleMapping(c->storedMappings, ArrayCount(c->storedMappings),container->storedObjectsDrawn, ArrayCount(container->storedObjectsDrawn),piece->inventorySlotType);
                        if(mapping)
                        {
                            r32 zoomSpeed = 0.0f;
                            if(mapping->hot)
                            {
                                mapping->hot = false;
                                zoomSpeed = (mapping->maxZoomCoeff - mapping->zoomCoeff);
                            }
                            else
                            {
                                zoomSpeed = -(mapping->zoomCoeff - 1.0f);
                            }
                            mapping->zoomCoeff += elapsedTime * zoomSpeed;
                            mapping->zoomCoeff = Clamp(1.0f, mapping->zoomCoeff, mapping->maxZoomCoeff);
                            Rect2 objectRect = Scale(result, mapping->zoomCoeff);
                            
                            mapping->projOnScreen = result;
                            ObjectTransform inventoryTransform = transform;
                            
                            DrawObjectMapping(worldMode, group, mapping, inventoryTransform, P, dim, objectRect, lights, elapsedTime);
                        }
                        
                        result = InvertedInfinityRect2();
                    }
                    
                    else if(piece->nameHash == usingSpaceHash)
                    {
                        ObjectMapping* mapping = FindCompatibleMapping(c->usingMappings, ArrayCount(c->usingMappings), container->usingObjectsDrawn, ArrayCount(container->usingObjectsDrawn),piece->inventorySlotType);
                        if(mapping)
                        {
                            r32 zoomSpeed = 0.0f;
                            if(mapping->hot)
                            {
                                mapping->hot = false;
                                zoomSpeed = (mapping->maxZoomCoeff - mapping->zoomCoeff);
                            }
                            else
                            {
                                zoomSpeed = -(mapping->zoomCoeff - 1.0f);
                            }
                            mapping->zoomCoeff += elapsedTime * zoomSpeed * mapping->zoomSpeed;
                            mapping->zoomCoeff = Clamp(1.0f, mapping->zoomCoeff, mapping->maxZoomCoeff);
                            Rect2 objectRect = Scale(result, mapping->zoomCoeff);
                            
                            mapping->projOnScreen = result;
                            ObjectTransform inventoryTransform = transform;
                            
                            DrawObjectMapping(worldMode, group, mapping, inventoryTransform, P, dim, objectRect, lights, elapsedTime);
                        }
                        result = InvertedInfinityRect2();
                    }
                    else if(piece->nameHash == recipeSpaceHash)
                    {
                        ObjectTransform recipeTransform = transform;
                        Rect2 recipeRect = result;
                        DrawRecipe(worldMode, group, seed, recipeTransform, P, dim, recipeRect, lights, container->recipeEssences);
                        result = InvertedInfinityRect2();
                    }
                }
                
                PAKBitmap* bitmap = GetBitmapInfo(group->assets, BID);
                for(u32 attachmentIndex = 0; attachmentIndex < bitmap->attachmentPointCount; ++attachmentIndex)
                {
                    PAKAttachmentPoint* attachmentPoint = GetAttachmentPoint(group->assets, BID, attachmentIndex);
                    
                    if(attachmentPoint)
                    {
                        Vec3 newP = GetAlignP(dim, attachmentPoint->alignment);
                        ObjectTransform finalTransform = transform;
                        finalTransform.angle += attachmentPoint->angle;
                        finalTransform.scale = Hadamart(finalTransform.scale, attachmentPoint->scale);
                        
                        Rect2 subRect = RenderLayoutRecursive_(worldMode, group, newP, finalTransform, layout, StringHash(attachmentPoint->name), seed, lights, container, elapsedTime);
                        result = Union(result, subRect);
                    }
                }
            }
        }
    }
    
    return result;
}

internal Rect2 RenderLayout(GameModeWorld* worldMode, RenderGroup* group, Vec3 P, ObjectTransform transform, LayoutComponent* layout, u32 seed, Lights lights, LayoutContainer* container, r32 elapsedTime)
{
    Rect2 result = RenderLayoutRecursive_(worldMode, group, P, transform, layout, layout->rootHash, seed, lights, container, elapsedTime);
    return result;
}


internal Rect2 RenderLayoutSpecificPiece(GameModeWorld* worldMode, RenderGroup* group, Vec3 P, ObjectTransform transform, LayoutComponent* layout, u32 seed, Lights lights, LayoutContainer* container, r32 elapsedTime, u64 pieceHash)
{
    Rect2 result = RenderLayoutRecursive_(worldMode, group, P, transform, layout, pieceHash, seed, lights, container, elapsedTime);
    
    return result;
}

internal Rect2 GetLayoutDim(RenderGroup* group, Vec3 P, ObjectTransform transform, LayoutComponent* layout, u32 seed, LayoutContainer* container)
{
    transform.dontRender = true;
    LayoutContainer fakeContainer = {};
    fakeContainer.drawMode = container->drawMode;
    Rect2 result = RenderLayoutRecursive_(0, group, P, transform, layout, layout->rootHash, seed, {}, &fakeContainer, 0);
    return result;
}

internal void RenderLayoutInRect(GameModeWorld* worldMode, RenderGroup* group, Rect2 rect, ObjectTransform transform, LayoutComponent* layout, u32 seed, Lights lights, LayoutContainer* container, r32 elapsedTime, r32 zOffset)
{
    Vec3 fakeP = V3(GetCenter(rect), zOffset);
    Vec2 desiredDim = GetDim(rect);
    
    Rect2 layoutDim = GetLayoutDim(group, fakeP, transform, layout, seed, container);
    Vec2 dim = GetDim(layoutDim);
    r32 scale = Min(desiredDim.x / dim.x, desiredDim.y / dim.y);
    transform.scale *= scale;
    
    Rect2 finalLayoutDim = GetLayoutDim(group, fakeP, transform, layout, seed, container);
    Vec2 drawnP = GetCenter(finalLayoutDim);
    
    Vec3 offset = V3(drawnP - GetCenter(rect), 0);
    Vec3 finalP = fakeP - offset;
    
    RenderLayout(worldMode, group, finalP, transform, layout, seed, lights, container, elapsedTime);
}

internal void RenderLayoutInRectCameraAligned(GameModeWorld* worldMode, RenderGroup* group, Vec3 P, Vec3 rectP, Rect2 cameraRect, ObjectTransform transform, LayoutComponent* layout, u32 seed, Lights lights, LayoutContainer* container, r32 elapsedTime)
{
    Vec2 desiredDim = GetDim(cameraRect);
    Rect2 layoutDim = GetLayoutDim(group, P, transform, layout, seed, container);
    Vec2 dim = GetDim(layoutDim);
    r32 scale = Min(desiredDim.x / dim.x, desiredDim.y / dim.y);
    transform.scale *= scale;
    
    Rect2 finalLayoutDim = GetLayoutDim(group, P, transform, layout, seed, container);
    
    Vec3 drawnCenter = UnprojectAtZ(group, &group->gameCamera, GetCenter(finalLayoutDim), rectP.z);
    Vec3 desiredCenter = UnprojectAtZ(group, &group->gameCamera, GetCenter(cameraRect), rectP.z);
    
    Vec3 offset = drawnCenter - desiredCenter;
    transform.cameraOffset.x -= Dot(offset, group->gameCamera.X);
    transform.cameraOffset.y -= Dot(offset, group->gameCamera.Y);
    
    RenderLayout(worldMode, group, P, transform, layout, seed, lights, container, elapsedTime);
}

RENDERING_ECS_JOB_CLIENT(RenderLayoutEntities)
{
    EntityAnimationParams params = GetEntityAnimationParams(worldMode, ID);
    elapsedTime *= params.speed;
    BaseComponent* base = GetComponent(worldMode, ID, BaseComponent);
    if(ShouldBeRendered(worldMode, base))
    {
        r32 height = GetHeight(base->bounds);
        r32 deepness = GetWidth(base->bounds);
        r32 width = GetWidth(base->bounds);
        
        Vec3 P = GetRelativeP(worldMode, base);
        Lights lights = GetLights(worldMode, P);
        LayoutComponent* layout = GetComponent(worldMode, ID, LayoutComponent);
        
        ObjectTransform transform = BillboardTransform();
        transform.angle = layout->rootAngle;
        transform.scale = layout->rootScale;
        transform.tint = params.tint;
        transform.modulationPercentage = params.modulationPercentage; 
        transform.dissolvePercentages = params.dissolveCoeff * V4(1, 1, 1, 1); 
        
        ContainerMappingComponent* container = GetComponent(worldMode, ID, ContainerMappingComponent);
        LayoutContainer layoutContainer = {};
        if(container && AreEqual(container->openedBy, worldMode->player.serverID))
        {
            layoutContainer.container = container; 
            layoutContainer.drawMode = LayoutContainerDraw_Open;
        }
        
        RenderLayout(worldMode, group, P, transform, layout, base->seed, lights, &layoutContainer, elapsedTime);
    }
}

internal void AddAnimationEffectIfNotPresent(GameModeWorld* worldMode, Vec3 P, AnimationEffectComponent* effects, AnimationEffectDefinition* effect, u16 ID)
{
    b32 add = true;
    for(u32 effectIndex = 0; effectIndex < effects->effectCount; ++effectIndex)
    {
        AnimationEffect* test = effects->effects + effectIndex;
        if(test->ID == ID)
        {
            switch(test->type)
            {
                case AnimationEffect_Particles:
                {
                    SetPosition(test->particles, P);
                } break;
            }
            
            add = false;
            break;
        }
    }
    
    if(add && effects->effectCount < ArrayCount(effects->effects))
    {
        AnimationEffect* dest = effects->effects + effects->effectCount++;
        dest->ID = ID;
        
        dest->type = SafeTruncateToU16(ConvertEnumerator(AnimationEffectType, effect->type));
        dest->time = effect->time;
        switch(dest->type)
        {
            case AnimationEffect_Tint:
            {
                dest->tint = effect->tint;
            } break;
            
            case AnimationEffect_Light:
            {
                dest->lightIntensity = effect->lightIntensity;
                dest->lightColor = effect->lightColor;
            } break;
            
            case AnimationEffect_Particles:
            {
                Assets* assets = worldMode->gameState->assets;
                GameProperties properties = {};
                
                for(u32 propertyIndex = 0; propertyIndex < effect->particlePropertyCount; ++propertyIndex)
                {
                    GameProperty* property = effect->particleProperties + propertyIndex;
                    AddOptionalGamePropertyRaw(&properties, *property);
                }
                
                AssetID effectID = QueryDataFiles(assets, ParticleEffect, "default", 0, &properties);
                if(IsValid(effectID))
                {
                    ParticleEffect* particles = GetData(assets, ParticleEffect, effectID);
                    
                    Vec3 startingP = P;
                    Vec3 UpVector = effect->particleEndOffset;
                    
                    dest->particles = GetNewParticleEffect(worldMode->particleCache, particles, startingP, UpVector);
                }
            } break;
            
            case AnimationEffect_SlowDown:
            {
                dest->slowDownCoeff = effect->slowDownCoeff;
            } break;
        }
    }
}

internal void FreeAnimationEffect(AnimationEffect* effect)
{
    switch(effect->type)
    {
        case AnimationEffect_Particles:
        {
            FreeParticleEffect(effect->particles);
            effect->particles = 0;
        } break;
    }
}

STANDARD_ECS_JOB_CLIENT(UpdateEntityEffects)
{
    BaseComponent* base = GetComponent(worldMode, ID, BaseComponent);
    AnimationEffectComponent* effects = GetComponent(worldMode, ID, AnimationEffectComponent);
    InteractionComponent* interaction = GetComponent(worldMode, ID, InteractionComponent);
    
    Vec3 P = GetRelativeP(worldMode, base);
    
    effects->params = DefaultAnimationParams();
    effects->lightIntensity = 0;
    
    for(u32 effectIndex = 0; effectIndex < effects->effectCount; ++effectIndex)
    {
        AnimationEffect* effect = effects->effects + effectIndex;
        if(effect->time >= 0)
        {
            effect->time -= elapsedTime;
            if(effect->time <= 0)
            {
                FreeAnimationEffect(effect);
                *effect = effects->effects[--effects->effectCount];
            }
        }
    }
    
    EntityDefinition* definition = GetData(worldMode->gameState->assets, EntityDefinition, EntityRefToAssetID(base->definitionID));
    if(definition)
    {
        for(u32 effectIndex = 0; effectIndex < definition->client.animationEffectsCount; ++effectIndex)
        {
            AnimationEffectDefinition* effect = definition->client.animationEffects + effectIndex;
            
            b32 matches = false;
            for(u32 testIndex = 0; testIndex < effect->propertyCount; ++testIndex)
            {
                matches = false;
                GameProperty* testProperty = effect->properties + testIndex;
                for(u32 propertyIndex = 0; propertyIndex < ArrayCount(base->properties); ++propertyIndex)
                {
                    GameProperty* entityProperty = base->properties + propertyIndex;
                    if(AreEqual(*entityProperty, *testProperty))
                    {
                        matches = true;
                        break;
                    }
                }
                
                if(!matches)
                {
                    break;
                }
            }
            
            if(matches)
            {
                AddAnimationEffectIfNotPresent(worldMode, P, effects, effect, SafeTruncateToU16(effectIndex));
            }
        }
    }
    
    
    Vec4 averageTint = {};
    u32 tintCount = 0;
    
    r32 averageLightIntensity = 0;
    Vec3 averageLightColor = {};
    u32 lightCount = 0;
    
    u32 slowDownCount = 0;
    r32 averageSlowDown = 0;
    
    for(u32 effectIndex = 0; effectIndex < effects->effectCount; ++effectIndex)
    {
        AnimationEffect* effect = effects->effects + effectIndex;
        switch(effect->type)
        {
            case AnimationEffect_Tint:
            {
                ++tintCount;
                averageTint += effect->tint;
            } break;
            
            case AnimationEffect_Light:
            {
                ++lightCount;
                averageLightIntensity += effect->lightIntensity;
                averageLightColor += effect->lightColor;
            } break;
            
            case AnimationEffect_Particles:
            {
                
            } break;
            
            case AnimationEffect_SlowDown:
            {
                ++slowDownCount;
                averageSlowDown += effect->slowDownCoeff;
            } break;
            InvalidDefaultCase;
        }
    }
    
    if(interaction && interaction->isOnFocus)
    {
        effects->params.modulationPercentage = 1.0f;
    }
    
    if(tintCount > 0)
    {
        effects->params.tint = averageTint *= 1.0f / tintCount;
    }
    
    Vec3 fadeInOutColor = V3(1, 1, 1);
    if(base)
    {
        if(base->deletedTime)
        {
            fadeInOutColor = (1.0f - Clamp01MapToRange(base->deletedTime, base->totalLifeTime, base->deletedTime + base->fadeOutTime)) * V3(1, 1, 1);
        }
        else
        {
            fadeInOutColor = V3(1, 1, 1) * Clamp01MapToRange(0, base->totalLifeTime, base->fadeInTime);
        }
    }
    effects->params.tint = Hadamart(effects->params.tint, V4(fadeInOutColor, 1));
    
    
    if(slowDownCount > 0)
    {
        effects->params.speed = averageSlowDown / slowDownCount;
    }
    
    
    
    if(base->deletedTime)
    {
        effects->params.dissolveCoeff = Clamp01MapToRange(base->deletedTime, base->totalLifeTime, base->deletedTime + base->fadeOutTime);
    }
    else
    {
        effects->params.dissolveCoeff = 1.0f - Clamp01MapToRange(0, base->totalLifeTime, base->fadeInTime);
    }
    
    
    if(lightCount > 0)
    {
        effects->lightIntensity = averageLightIntensity *= 1.0f / lightCount;
        effects->lightColor = averageLightColor *= 1.0f / lightCount;
    }
    
}

inline b32 ValidVector(Vec3 original, Vec3 pick)
{
    b32 result = true;
    if(pick.x == 0.0f && pick.y == 0.0f && pick.z == 0.0f)
    {
        result = false;
    }
    else
    {
        r32 dot = Dot(Normalize(original), Normalize(pick));
        result = (dot != 1.0f); 
    }
    
    return result;
}

RENDERING_ECS_JOB_CLIENT(UpdateAndRenderBolt)
{
    BoltComponent* bolt = GetComponent(worldMode, ID, BoltComponent);
    BaseComponent* base = GetComponent(worldMode, ID, BaseComponent);
    BoltDefinition* definition = &worldMode->boltDefinition;
    if(bolt->ttl > definition->ttl)
    {
        bolt->ttl = definition->ttl;
    }
    
    bolt->ttl -= elapsedTime;
    bolt->timeSinceLastAnimationTick += elapsedTime;
    
    if(bolt->ttl > 0)
    {
        
        Vec3 endP = GetRelativeP(worldMode, base);
        Vec3 startP = endP + bolt->highOffset;
        
        TempMemory subdivisionsMemory = BeginTemporaryMemory(worldMode->temporaryPool);
        
        u32 subdivisions = definition->subdivisions;
        subdivisions = Min(subdivisions, MAX_BOLT_SUBDIVISIONS);
        Vec3* subdivisionPoints = PushArray(subdivisionsMemory.pool, Vec3, subdivisions);
        
        Vec3 deltaP = endP - startP;
        Vec3 deltaPerSegment = deltaP / (r32) subdivisions;
        
        b32 computeAnimationOffsets = (bolt->timeSinceLastAnimationTick >= definition->animationTick);
        if(computeAnimationOffsets)
        {
            bolt->timeSinceLastAnimationTick = 0.0f;
        }
        
        Lights lights = GetLights(worldMode, startP + 0.5f * deltaP);
        AddLight(worldMode, startP + 0.5f * deltaP, definition->lightColor, definition->lightIntensity);
        
        RandomSequence seq = Seed(bolt->seed);
        for(u32 subIndex = 0; subIndex < subdivisions - 1; ++subIndex)
        {
            Vec3 parallel = (r32) (subIndex + 1) * deltaPerSegment;
            
            Vec3 random = {};
            while(!ValidVector(parallel, random))
            {
                random = RandomBilV3(&seq);
            }
            Vec3 perpStructure = Cross(parallel, definition->magnitudoStructure * random);
            
            if(computeAnimationOffsets)
            {
                Vec3 randomAnimation = {};
                while(!ValidVector(parallel, randomAnimation))
                {
                    randomAnimation = RandomBilV3(&bolt->animationSeq);
                }
                bolt->subdivisionAnimationOffsets[subIndex] = Cross(parallel, definition->magnitudoAnimation * randomAnimation);
            }
            
            subdivisionPoints[subIndex] = startP + parallel + perpStructure + bolt->subdivisionAnimationOffsets[subIndex];
        }
        subdivisionPoints[subdivisions - 1] = endP;
        
        
        
        
        Vec3 subStartP = startP;
        for(u32 subdivisionIndex = 0; subdivisionIndex < subdivisions; ++subdivisionIndex)
        {
            Vec3 subEndP = subdivisionPoints[subdivisionIndex];
            
            Vec4 color = definition->color;
            
            r32 segmentAlpha = 1.0f;
            if(bolt->ttl > (definition->ttl - definition->fadeinTime))
            {
                r32 fadeinAvailableTime = definition->fadeinTime;
                r32 segmentFadeinTime = fadeinAvailableTime / subdivisions;
                
                r32 segmentMinAlphaTTL = definition->ttl - subdivisionIndex * segmentFadeinTime;
                r32 segmentMaxAlphaTTL = definition->ttl - (subdivisionIndex + 1) * segmentFadeinTime;
                
                segmentAlpha = Clamp01MapToRange(segmentMinAlphaTTL, bolt->ttl, segmentMaxAlphaTTL);
            }
            else if(bolt->ttl < definition->fadeoutTime)
            {
                r32 fadeoutAvailableTime = definition->fadeoutTime;
                r32 segmentFadeoutTime = fadeoutAvailableTime / subdivisions;
                
                r32 segmentMaxAlphaTTL = definition->fadeoutTime - subdivisionIndex * segmentFadeoutTime;
                r32 segmentMinAlphaTTL = definition->fadeoutTime - (subdivisionIndex + 1) * segmentFadeoutTime;
                
                segmentAlpha = 1.0f - Clamp01MapToRange(segmentMaxAlphaTTL, bolt->ttl, segmentMinAlphaTTL);
            }
            
            color.a = segmentAlpha;
            
            PushDebugLine(group, color.rgb, subStartP, subEndP, definition->thickness, lights);
            subStartP = subEndP;
        }
        
        if(bolt->ttl <= definition->lightStartTime && bolt->ttl >= definition->lightEndTime)
        {
            AddLight(worldMode, endP, definition->lightColor, definition->lightIntensity);
        }
        
        EndTemporaryMemory(subdivisionsMemory);
    }
}