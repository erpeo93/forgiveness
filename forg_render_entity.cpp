internal b32 ShouldBeRendered(GameModeWorld* worldMode, BaseComponent* base)

{
    b32 result = false;
    if(base)
    {
        result = ((base->universeP.chunkZ == worldMode->player.universeP.chunkZ) &&
                  !(base->flags & EntityFlag_notInWorld));
        
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

internal void RenderShadow(GameModeWorld* worldMode, RenderGroup* group, Vec3 P, ShadowComponent* shadowComponent)
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
            ObjectTransform transform = FlatTransform();
            transform.scale = shadowComponent->scale;
            transform.tint = shadowComponent->color;
            PushBitmap(group, transform, shadowID, P + shadowComponent->offset, height, lights);
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
        EntityDefinition* definition = GetData(group->assets, EntityDefinition, EntityTypeToAssetID(base->type));
        
        r32 height = GetHeight(base->bounds);
        
        Vec3 P = GetRelativeP(worldMode, ID);
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
        params.skeletonScale = 1;
        params.bitmapScale = 1;
        params.transform = BillboardTransform();
        params.flipOnYAxis = animation->flipOnYAxis;
        params.equipment = GetComponent(worldMode, ID, EquipmentComponent);
        params.equipped = GetComponent(worldMode, ID, UsingComponent);
        params.tint = animationParams.tint;
        params.dissolveCoeff = animationParams.dissolveCoeff;
        params.modulationPercentage = 0; 
        params.replacementCount = definition->client.replacementCount;
        params.replacements = definition->client.animationReplacements;
        params.ID = ID;
        
        GameProperty action = {};
        ActionComponent* actionComp = GetComponent(worldMode, ID, ActionComponent);
        if(actionComp)
        {
            action = GameProp(action, actionComp->action);
        }
        
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
        
        params.skeletonScale = animation->scale;
        params.bitmapScale = animation->scale;
        RenderAnimationAndTriggerSounds(worldMode, group, animation, &params);
        
        
        if(animationParams.modulationPercentage > 0)
        {
            params.bitmapScale *= animation->scaleCoeffWhenOnFocus;
            params.P += animation->cameraZOffsetWhenOnFocus * group->gameCamera.Z;
            params.modulationPercentage = animationParams.modulationPercentage;
            params.flipOnYAxis = animation->flipOnYAxis;
            params.elapsedTime = 0;
            RenderAnimationAndTriggerSounds(worldMode, group, animation, &params);
        }
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

internal BitmapId GetImageFromReferenceVariantIndex(Assets* assets, ImageReference* reference, RandomSequence* seq, u16 index)
{
    GameProperties properties = reference->properties;
    AddGameProperty(&properties, variant, index);
    
    BitmapId result = GetImageFromSubtype(assets, reference->typeHash, &properties, seq);
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


internal void RenderStaticAnimation(GameModeWorld* worldMode, RenderGroup* group, EntityAnimationParams params, BaseComponent* base, ImageReference* image, r32 elapsedTime, Vec3 cameraOffset = {})
{
    r32 height = GetHeight(base->bounds);
    Vec3 P = GetRelativeP(worldMode, base, params);
    Lights lights = GetLights(worldMode, P);
    
    RandomSequence seq = Seed(base->seed);
    BitmapId BID = GetImageFromReference(group->assets, image, &seq);
    if(IsValid(BID))
    {
        ObjectTransform transform = image->flat ? FlatTransform() : BillboardTransform();
        transform.modulationPercentage = params.modulationPercentage; 
        transform.dissolvePercentages = params.dissolveCoeff * V4(1, 1, 1, 1); 
        transform.tint = params.tint;
        transform.cameraOffset = cameraOffset;
        transform.cameraOffset.z += image->zOffset;
        if(image->emittors)
        {
            transform.lightInfluence = 1.0f;
        }
        PushBitmap(group, transform, BID, P, height, lights);
    }
}

internal void RenderSegmentImage(GameModeWorld* worldMode, RenderGroup* group, EntityAnimationParams params, BaseComponent* base, ImageReference* image, r32 elapsedTime)
{
    r32 height = GetHeight(base->bounds);
    r32 width = GetWidth(base->bounds);
    
    Vec3 P = GetRelativeP(worldMode, base, params);
    Lights lights = GetLights(worldMode, P);
    
    RandomSequence seq = Seed(base->seed);
    BitmapId BID = GetImageFromReference(group->assets, image, &seq);
    if(IsValid(BID))
    {
        r32 modulationPercentage = params.modulationPercentage; 
        Vec4 dissolvePercentages = params.dissolveCoeff * V4(1, 1, 1, 1); 
        
        Vec3 speed = Normalize(base->velocity);
        
        Vec3 fromP = P - speed * width;
        Vec3 toP = P + speed * width;
        
        PushTextureSegment(group, BID, params.tint, fromP, toP, height, lights, dissolvePercentages);
    }
}

RENDERING_ECS_JOB_CLIENT(RenderShadow)
{
    BaseComponent* base = GetComponent(worldMode, ID, BaseComponent);
    if(ShouldBeRendered(worldMode, base))
    {
        ShadowComponent* shadow = GetComponent(worldMode, ID, ShadowComponent);
        Vec3 P = GetRelativeP(worldMode, ID);
        P.z = 0.0f;
        RenderShadow(worldMode, group, P, shadow);
    }
}

internal r32 GetDissolveCoeff(r32 density, r32 dissolveDuration, u32 index)
{
    RandomSequence seq = Seed(index);
    r32 startDissolving = Max(RandomUni(&seq), dissolveDuration);
    r32 endDissolving = startDissolving - dissolveDuration;
    endDissolving = Max(endDissolving, 0);
    
    r32 result = 0.0f;
    if(startDissolving != endDissolving)
    {
        result = 1.0f - Clamp01MapToRange(endDissolving, density, startDissolving);
    }
    else
    {
        result = (density > startDissolving) ? 0.0f : 1.0f;
    }
    
    return result;
}

RENDERING_ECS_JOB_CLIENT(RenderRock)
{
    u64 mineralHash = StringHash("mineral");
    EntityAnimationParams params = GetEntityAnimationParams(worldMode, ID);
    elapsedTime *= params.speed;
    
    BaseComponent* base = GetComponent(worldMode, ID, BaseComponent);
    
    Vec3 cameraZ = group->gameCamera.Z;
    r32 zOffset = 0.001f;
    
    if(ShouldBeRendered(worldMode, base))
    {
        r32 height = GetHeight(base->bounds);
        Vec3 P = GetRelativeP(worldMode, ID);
        Lights lights = GetLights(worldMode, P);
        
        RockComponent* rock = GetComponent(worldMode, ID, RockComponent);
        
        RandomSequence seq = Seed(base->seed);
        BitmapId rockID = GetImageFromReference(group->assets, &rock->rock, &seq);
        if(IsValid(rockID))
        {  
            ObjectTransform transform = BillboardTransform();
            transform.dissolvePercentages = params.dissolveCoeff * V4(1, 1, 1, 1);
            transform.modulationPercentage = params.modulationPercentage;
            transform.tint = Hadamart(params.tint, rock->color);
            
            BitmapDim bitmapData = PushBitmap(group, transform, rockID, P, height, lights);
            Bitmap* bitmap = GetBitmap(group->assets, rockID).bitmap;
            PAKBitmap* bitmapInfo = GetBitmapInfo(group->assets, rockID);
            
            u16 possibleEssences[Count_essence];
            u16 essenceCount = 0;
            
            for(u16 essenceIndex = 0; essenceIndex < ArrayCount(base->essences); ++essenceIndex)
            {
                if(base->essences[essenceIndex] > 0)
                {
                    possibleEssences[essenceCount++] = essenceIndex;
                }
            }
            
            if(bitmap && essenceCount)
            {
                u16 mineralRunningIndex = 0;
                for(u32 mineralPointIndex = 0;
                    mineralPointIndex < bitmapInfo->attachmentPointCount; ++mineralPointIndex)
                {
                    PAKAttachmentPoint* mineral = bitmap->attachmentPoints + mineralPointIndex;
                    
                    u64 mHash = StringHash(mineral->name);
                    
                    u32 mineralPound = FindFirstInString(mineral->name, '#');
                    if(mineralPound != 0xffffffff)
                    {
                        mHash = StringHash(mineral->name, mineralPound);
                    }
                    
                    if(mHash == mineralHash)
                    {
                        u16 essence = possibleEssences[RandomChoice(&seq, essenceCount)];
                        AddGameProperty(&rock->mineral.properties, essence, essence);
                        BitmapId mineralID = GetImageFromReference(group->assets, &rock->mineral, &seq);
                        
                        if(IsValid(mineralID))
                        {
                            ColoredBitmap b = GetBitmap(group->assets, mineralID);
                            Bitmap* mineralBitmap = b.bitmap;
                            if(mineralBitmap)
                            {
                                PAKBitmap* mineralInfo = GetBitmapInfo(group->assets, mineralID);
                                Vec2 mineralPivot = V2(mineralInfo->align[0], mineralInfo->align[1]);
                                Vec2 mineralInvUV = GetInvUV(mineralBitmap->width, mineralBitmap->height);
                                Vec3 mineralP = GetAlignP(bitmapData, mineral->alignment);
                                
                                r32 angleRad = DegToRad(mineral->angle);
                                Vec3 XAxis = V3(Cos(angleRad), Sin(angleRad), 0.0f);
                                Vec3 YAxis  = V3(Perp(XAxis.xy), 0.0f);
                                
                                Vec2 mineralScale = mineral->scale;
                                if(mineralInfo->flippedByDefault)
                                {
                                    mineralScale.x = -mineralScale.x;
                                }
                                
                                Vec3 mLateral =mineralScale.x * (XAxis.x * magicLateralVector + XAxis.y * magicUpVector);
                                Vec3 mUp = mineralScale.y * (YAxis.x * magicLateralVector + YAxis.y * magicUpVector);
                                
                                Vec3 pivotOffset = mineralPivot.x * mLateral + mineralPivot.y * mUp; 
                                mineralP -= pivotOffset;
                                mineralP += (0.5f * mLateral + 0.5f * mUp);
                                
                                mineralP += zOffset * cameraZ;
                                zOffset += 0.001f;
                                
                                Vec4 windInfluences = V4(0, 0, 0, 0);
                                u8 windFrequency = 1;
                                u8 seed = (u8) mineralPointIndex;
                                r32 dissolveCoeff = GetDissolveCoeff(rock->mineralDensity, 0.05f, mineralRunningIndex++);
                                dissolveCoeff = Max(dissolveCoeff, params.dissolveCoeff);
                                Vec4 dissolvePercentages = dissolveCoeff * V4(1, 1, 1, 1);
                                r32 alphaThreesold = 0;
                                b32 flat = false;
                                
                                u32 mineralC = StoreColor(b.coloration);
                                
                                PushMagicQuad(group, mineralP, flat, 0.5f * mLateral, 0.5f * mUp, mineralInvUV, mineralC, mineralBitmap->textureHandle, lights, params.modulationPercentage, 0, 0, windInfluences, windFrequency, dissolvePercentages, alphaThreesold, seed);
                            }
                            else
                            {
                                LoadBitmap(group->assets, mineralID);
                            }
                        }
                    }
                }
            }
        }
    }
}

internal void RenderLeafFlowerFruit(RenderGroup* group, PlantComponent* plant,  VegetationComponent* vegetation,
                                    EntityAnimationParams params,
                                    PAKBitmap* info, Bitmap* bitmap, BitmapId* leafIDs, u32 leafVariantCount, u32 leafC, u32* leafRunningIndex,  BitmapId* flowerIDs, u32 flowerVariantCount, u32 flowerC, u32* flowerRunningIndex, BitmapId* fruitIDs, u32 fruitVariantCount, u32 fruitC, u32* fruitRunningIndex,
                                    Vec3 branchMin, Vec3 bLateral, Vec3 bUp, r32 trunkHeight, r32* zOffset, Lights lights, RandomSequence* seq)
{
    u64 leafHash = StringHash("leaf");
    u64 flowerHash = StringHash("flower");
    u64 fruitHash = StringHash("fruit");
    
    r32 leafDensity = 1.0f;
    r32 flowerDensity = vegetation->flowerDensity;
    r32 fruitDensity = vegetation->fruitDensity;
    
    r32 dissolveDuration = plant->dissolveDuration;
    
    for(u32 leafPointIndex = 0;
        leafPointIndex < info->attachmentPointCount; ++leafPointIndex)
    {
        PAKAttachmentPoint* leaf = bitmap->attachmentPoints + leafPointIndex;
        
        BitmapId LFF = {};
        r32 dissolveCoeff = 0;
        
        u64 baseHash = StringHash(leaf->name);
        u16 variantIndex = 0;
        u32 C = 0;
        Vec4 windInfluences = {};
        r32 randomAngle = 0;
        
        u32 pound = FindFirstInString(leaf->name, '#');
        if(pound != 0xffffffff)
        {
            baseHash = StringHash(leaf->name, pound);
            variantIndex = SafeTruncateToU16(StringToUInt32(leaf->name + pound + 1));
        }
        
        if(baseHash == leafHash)
        {
            if(variantIndex < leafVariantCount)
            {
                LFF = leafIDs[variantIndex];
            }
            C = leafC;
            dissolveCoeff = GetDissolveCoeff(leafDensity, dissolveDuration, (*leafRunningIndex)++);
            windInfluences = V4(0, 0, plant->leafWindInfluence, plant->leafWindInfluence);
            randomAngle = plant->leafRandomAngle;
        }
        else if(baseHash == flowerHash)
        {
            if(variantIndex < flowerVariantCount)
            {
                LFF = flowerIDs[variantIndex];
            }
            C = flowerC;
            dissolveCoeff = GetDissolveCoeff(flowerDensity, dissolveDuration, (*flowerRunningIndex)++);
            windInfluences = V4(0, 0, plant->flowerWindInfluence, plant->flowerWindInfluence);
            randomAngle = plant->flowerRandomAngle;
        }
        else if(baseHash == fruitHash)
        {
            if(variantIndex < fruitVariantCount)
            {
                LFF = fruitIDs[variantIndex];
            }
            C = fruitC;
            dissolveCoeff = GetDissolveCoeff(fruitDensity, dissolveDuration, (*fruitRunningIndex)++);
            windInfluences = V4(0, 0, plant->fruitWindInfluence, plant->fruitWindInfluence);
            randomAngle = plant->fruitRandomAngle;
        }
        
        dissolveCoeff = Max(dissolveCoeff, params.dissolveCoeff);
        if(IsValid(LFF))
        {
            Bitmap* LFFBitmap = GetBitmap(group->assets, LFF).bitmap;
            if(LFFBitmap)
            {
                PAKBitmap* leafInfo = GetBitmapInfo(group->assets, LFF);
                Vec2 leafPivot = V2(leafInfo->align[0], leafInfo->align[1]);
                Vec2 leafInvUV = GetInvUV(LFFBitmap->width, LFFBitmap->height);
                
                Vec3 leafP = branchMin + leaf->alignment.x * bLateral + leaf->alignment.y * bUp;
                
                r32 leafAngle = leaf->angle + RandomUni(seq) * randomAngle;
                r32 angleRad = DegToRad(leafAngle);
                Vec3 XAxis = V3(Cos(angleRad), Sin(angleRad), 0.0f);
                Vec3 YAxis  = V3(Perp(XAxis.xy), 0.0f);
                
                Vec2 leafScale = leaf->scale;
                if(plant->scaleLeafAccordingToTrunk)
                {
                    leafScale *= trunkHeight;
                }
                
                Vec3 lateral =leafScale.x * (XAxis.x * magicLateralVector + XAxis.y * magicUpVector);
                Vec3 up =leafScale.y * (YAxis.x * magicLateralVector + YAxis.y * magicUpVector);
                
                
                Vec3 pivotOffset = leafPivot.x * lateral + leafPivot.y * up; 
                leafP -= pivotOffset;
                leafP += (0.5f * lateral + 0.5f * up);
                
                leafP += *zOffset * group->gameCamera.Z;
                *zOffset += 0.001f;
                
                u8 windFrequency = 1;
                u8 seed = (u8) leafPointIndex;
                Vec4 dissolvePercentages = dissolveCoeff * V4(1, 1, 1, 1);
                r32 alphaThreesold = 0;
                b32 flat = false;
                
                PushMagicQuad(group, leafP, flat, 0.5f * lateral, 0.5f * up, leafInvUV, C, LFFBitmap->textureHandle, lights, params.modulationPercentage, 0, 0, windInfluences, windFrequency, dissolvePercentages, alphaThreesold, seed);
            }
            else
            {
                LoadBitmap(group->assets, LFF);
            }
        }
    }
}

RENDERING_ECS_JOB_CLIENT(RenderPlant)
{
    u64 branchHash = StringHash("branch");
    EntityAnimationParams params = GetEntityAnimationParams(worldMode, ID);
    elapsedTime *= params.speed;
    
    BaseComponent* base = GetComponent(worldMode, ID, BaseComponent);
    VegetationComponent* vegetation = GetComponent(worldMode, ID, VegetationComponent);
    PlantComponent* plant = GetComponent(worldMode, ID, PlantComponent);
    
    r32 branchDensity = vegetation->branchDensity;
    r32 dissolveDuration = plant->dissolveDuration;
    
    r32 zOffset = 0.001f;
    
    if(ShouldBeRendered(worldMode, base))
    {
        r32 height = GetHeight(base->bounds);
        Vec3 P = GetRelativeP(worldMode, ID);
        Lights lights = GetLights(worldMode, P);
        
        u32 branchC = StoreColor(plant->branchColor);
        u32 leafC = StoreColor(plant->leafColor);
        u32 flowerC = StoreColor(plant->flowerColor);
        u32 fruitC = StoreColor(plant->fruitColor);
        
        RandomSequence seq = Seed(base->seed);
        BitmapId trunkID = GetImageFromReference(group->assets, &plant->trunk, &seq);
        if(IsValid(trunkID))
        {  
            ObjectTransform transform = BillboardTransform();
            transform.dissolvePercentages = params.dissolveCoeff * V4(1, 1, 1, 1);
            transform.modulationPercentage = params.modulationPercentage;
            transform.tint = Hadamart(params.tint, plant->branchColor);
            
            BitmapDim bitmapData = PushBitmap(group, transform, trunkID, P, height, lights);
            
            Bitmap* bitmap = GetBitmap(group->assets, trunkID).bitmap;
            PAKBitmap* bitmapInfo = GetBitmapInfo(group->assets, trunkID);
            if(bitmap)
            {
                BitmapId leafIDs[8] = {};
                BitmapId flowerIDs[8] = {};
                BitmapId fruitIDs[8] = {};
                
                if(plant->hasLeafVariant)
                {
                    for(u16 variantIndex = 0; variantIndex < ArrayCount(leafIDs); ++variantIndex)
                    {
                        leafIDs[variantIndex] = GetImageFromReferenceVariantIndex(group->assets, &plant->leaf, &seq, variantIndex);
                    }
                }
                else
                {
                    leafIDs[0] = GetImageFromReference(group->assets, &plant->leaf, &seq);
                }
                
                
                
                if(plant->hasFlowerVariant)
                {
                    for(u16 variantIndex = 0; variantIndex < ArrayCount(leafIDs); ++variantIndex)
                    {
                        flowerIDs[variantIndex] = GetImageFromReferenceVariantIndex(group->assets, &plant->flower, &seq, variantIndex);
                    }
                }
                else
                {
                    flowerIDs[0] = GetImageFromReference(group->assets, &plant->flower, &seq);
                }
                
                
                if(plant->hasFruitVariant)
                {
                    for(u16 variantIndex = 0; variantIndex < ArrayCount(leafIDs); ++variantIndex)
                    {
                        fruitIDs[variantIndex] = GetImageFromReferenceVariantIndex(group->assets, &plant->fruit, &seq, variantIndex);
                    }
                }
                else
                {
                    fruitIDs[0] = GetImageFromReference(group->assets, &plant->fruit, &seq);
                }
                
                u32 leafRunningIndex = 0;
                u32 flowerRunningIndex = 0;
                u32 fruitRunningIndex = 0;
                
                Vec3 trunkMin = GetAlignP(bitmapData, V2(0, 0));
                Vec3 tLateral = bitmapData.XAxis * bitmapData.size.x;
                Vec3 tUp = bitmapData.YAxis * bitmapData.size.y;
                
                RenderLeafFlowerFruit(group, plant, vegetation, params, bitmapInfo, bitmap, leafIDs, ArrayCount(leafIDs), leafC, &leafRunningIndex, flowerIDs, ArrayCount(flowerIDs), flowerC, &flowerRunningIndex, fruitIDs, ArrayCount(fruitIDs), fruitC, &fruitRunningIndex, trunkMin, tLateral, tUp, height, &zOffset, lights, &seq);
                
                for(u32 branchPointIndex = 0;
                    branchPointIndex < bitmapInfo->attachmentPointCount; ++branchPointIndex)
                {
                    PAKAttachmentPoint* branch = bitmap->attachmentPoints + branchPointIndex;
                    
                    u64 bHash = StringHash(branch->name);
                    u16 branchIndex = 0;
                    
                    u32 branchPound = FindFirstInString(branch->name, '#');
                    if(branchPound != 0xffffffff)
                    {
                        bHash = StringHash(branch->name, branchPound);
                        branchIndex = SafeTruncateToU16(StringToUInt32(branch->name + branchPound + 1));
                    }
                    
                    if(bHash == branchHash)
                    {
                        BitmapId branchID;
                        if(plant->hasBranchVariant)
                        {
                            branchID = GetImageFromReferenceVariantIndex(group->assets, &plant->branch, &seq, branchIndex);
                        }
                        else
                        {
                            branchID = GetImageFromReference(group->assets, &plant->branch, &seq);
                        }
                        
                        
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
                                
                                Vec2 branchScale = branch->scale;
                                if(plant->scaleBranchesAccordingToTrunk)
                                {
                                    branchScale *= height;
                                }
                                
                                if(branchInfo->flippedByDefault)
                                {
                                    branchScale.x = -branchScale.x;
                                }
                                
								Vec3 bLateral =branchScale.x * (XAxis.x * magicLateralVector + XAxis.y * magicUpVector);
								Vec3 bUp = branchScale.y * (YAxis.x * magicLateralVector + YAxis.y * magicUpVector);
                                
								Vec3 pivotOffset = branchPivot.x * bLateral + branchPivot.y * bUp; 
								branchP -= pivotOffset;
                                branchP += (0.5f * bLateral + 0.5f * bUp);
                                
                                branchP += zOffset * group->gameCamera.Z;
                                zOffset += 0.001f;
                                
                                r32 branchDissolveCoeff = GetDissolveCoeff(branchDensity, dissolveDuration, branchPointIndex);
                                
                                Vec4 windInfluences = V4(0, 0, 0, 0);
                                u8 windFrequency = 1;
                                u8 seed = (u8) branchPointIndex;
                                Vec4 dissolvePercentages = Max(branchDissolveCoeff, params.dissolveCoeff) * V4(1, 1, 1, 1);
                                r32 alphaThreesold = 0;
                                b32 flat = false;
                                
                                PushMagicQuad(group, branchP, flat, 0.5f * bLateral, 0.5f * bUp, branchInvUV, branchC, branchBitmap->textureHandle, lights, params.modulationPercentage, 0, 0, windInfluences, windFrequency, dissolvePercentages, alphaThreesold, seed);
                                
                                Vec3 branchMin = branchP - 0.5f * bLateral - 0.5f * bUp;
                                RenderLeafFlowerFruit(group, plant, vegetation, params, branchInfo, branchBitmap, leafIDs, ArrayCount(leafIDs), leafC, &leafRunningIndex, flowerIDs, ArrayCount(flowerIDs), flowerC, &flowerRunningIndex, fruitIDs, ArrayCount(fruitIDs), fruitC, &fruitRunningIndex, branchMin, bLateral, bUp, height, &zOffset, lights, &seq);
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
				Vec3 P = GetRelativeP(worldMode, ID);
                Lights lights = GetLights(worldMode, P);
                P -= quad->offset;
                
                
                u8 windFrequency = (u8) grass->windFrequencyStandard;
                
                
#if 0                
                BaseComponent* playerBase = GetComponent(worldMode, worldMode->player.clientID, BaseComponent);
                if(playerBase)
                {
                    if(RectOverlaps(Offset(grass->bounds, P), playerBase->bounds) 
                       && LengthSq(playerBase->velocity) > 0.1f
                       )
                    {
                        windFrequency = (u8) grass->windFrequencyOverlap;
                    }
                }
#endif
                
                Vec4 windInfluences = V4(0, 0, grass->windInfluence, grass->windInfluence);
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
    
    Vec3 P = GetRelativeP(worldMode, base, params);
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
        
        if(animation->emittors)
        {
            transform.lightInfluence = 1.0f;
        }
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

internal void RenderLayoutInRectCameraAligned(GameModeWorld* worldMode, RenderGroup* group, Vec3 minP, Rect2 screenRect, ObjectTransform transform, LayoutComponent* layout, u32 seed, Lights lights, LayoutContainer* container, r32 elapsedTime);
internal void RenderLayoutInRect(GameModeWorld* worldMode, RenderGroup* group, Rect2 rect, ObjectTransform transform, LayoutComponent* layout, u32 seed, Lights lights, LayoutContainer* container, r32 elapsedTime, r32 zOffset = 0.0f);
internal void OverdrawEssence(GameModeWorld* worldMode, RenderGroup* group, ObjectTransform transform, u16 essence, Rect2 rect, Vec4 color);


internal void RenderObjectInRect(GameModeWorld* worldMode, RenderGroup* group, EntityID objectID, ObjectTransform transform, Vec3 spaceP, Rect2 rect, Lights lights, r32 elapsedTime)
{
    BaseComponent* objectBase = GetComponent(worldMode, objectID, BaseComponent);
    EntityAnimationParams params = GetEntityAnimationParams(worldMode, objectID);
    transform.modulationPercentage = params.modulationPercentage;
    transform.dissolvePercentages = params.dissolveCoeff * V4(1, 1, 1, 1);
    LayoutComponent* objectLayout = GetComponent(worldMode, objectID, LayoutComponent);
    
    if(objectBase && objectLayout)
    {
        LayoutContainer objectContainer = {};
        objectContainer.drawMode = LayoutContainerDraw_Container;
        objectContainer.container = GetComponent(worldMode, objectID, ContainerComponent);
        objectContainer.infused = GetComponent(worldMode, objectID, InfusedEffectsComponent);
        objectContainer.definition = GetEntityTypeDefinition(group->assets, objectBase->type);
        //objectContainer.recipeEssences = GetComponent(worldMode, objectID, RecipeEssenceComponent);
        
        if(transform.upright)
        {
            RenderLayoutInRectCameraAligned(worldMode, group, spaceP, rect, transform, objectLayout, objectBase->seed, lights, &objectContainer, elapsedTime);
        }
        else
        {
            RenderLayoutInRect(worldMode, group, rect, transform, objectLayout, objectBase->seed, lights, &objectContainer, elapsedTime);
        }
    }
}

internal void DrawObjectSlot(GameModeWorld* worldMode, RenderGroup* group, InventorySlot* slot, ObjectTransform transform, BitmapDim spaceDim, Rect2 rect, Lights lights, r32 elapsedTime)
{
    if(IsValidInventorySlot(&worldMode->gameUI, slot))
    {
        EntityID objectID = GetBoundedID(slot);
        RenderObjectInRect(worldMode, group, objectID, transform, spaceDim.P, rect, lights, elapsedTime);
    }
}

internal void DrawRecipeContent(GameModeWorld* worldMode, RenderGroup* group, u32 recipeSeed, ObjectTransform transform, Vec3 P, BitmapDim spaceDim, Rect2 rect, Lights lights, RecipeEssenceComponent* essences)
{
    if(worldMode)
    {
        EntityType type = GetCraftingType(group->assets, recipeSeed);
        Vec2 dim = GetDim(rect);
        Vec2 half = 0.5f * dim;
        
        transform.modulationPercentage = 0;
        
        Vec2 recipeMin = V2(rect.min.x, rect.min.y + half.y);
        Rect2 recipeRect = RectMinDim(recipeMin, V2(dim.x, half.y));
        
        u16 recipeEssences[Count_essence] = {};
        if(transform.upright)
        {
            recipeMin = rect.min;
            recipeRect = RectMinDim(recipeMin, dim);
        }
        else
        {
            if(essences)
            {
                Vec2 startingSlotP = rect.min;
                Rect2 essenceTotalRect = RectMinDim(startingSlotP, V2(dim.x, 0.5f * half.y));
                
                u16 essenceCount = GetCraftingEssenceCount(group->assets, type, recipeSeed);
                r32 essenceWidth = dim.x / essenceCount;
                
                Rect2 essenceRect = RectMinDim(startingSlotP, V2(essenceWidth, 0.5f * half.y));
                for(u32 slotIndex = 0; slotIndex < essenceCount; ++slotIndex)
                {
                    essences->projectedOnScreen[slotIndex] = essenceRect;
                    u16 essence = essences->essences[slotIndex];
                    BaseComponent* playerBase = GetComponent(worldMode, worldMode->player.clientID, BaseComponent);
                    
                    Vec4 color = V4(1, 1, 1, 1);
                    if(playerBase)
                    {
                        
                    }
                    if(essence < Count_essence && playerBase && playerBase->essences[essence] > 0)
                    {
                    }
                    else
                    {
                        color = V4(1, 1, 1, 0.1f);
                    }
                    
                    OverdrawEssence(worldMode, group, transform, essence, essenceRect, color);
                    ++recipeEssences[essence];
                    essenceRect = Offset(essenceRect, V2(essenceWidth, 0));
                }
            }
            
            u32 craftSeed = recipeSeed;
            
            Vec2 startingCompP = rect.min + 0.25f * V2(0, dim.y);
            Rect2 componentTotalRect = RectMinDim(startingCompP, V2(dim.x, 0.5f * half.y));
            
            EntityType components[8];
            b32 deleteAfterCrafting[8];
            u16 componentCount = GetCraftingComponents(group->assets, type, craftSeed, components, deleteAfterCrafting, ArrayCount(components));
            if(componentCount > 0)
            {
                r32 componentWidth = dim.x / componentCount;
                Rect2 componentRect = RectMinDim(startingCompP, V2(componentWidth, 0.5f * half.y));
                
                for(u16 componentIndex = 0; componentIndex < componentCount; ++componentIndex)
                {
                    EntityID ID;
                    if(EntityHasType(worldMode, worldMode->player.clientID, components[componentIndex], &ID))
                    {
                        transform.tint = V4(1, 1, 1, 1);
                        RenderObjectInRect(worldMode, group, ID, transform, spaceDim.P, componentRect, lights, 0);
                    }
                    else
                    {
                        transform.tint.a *= 0.4f;
                        
                        u32 componentSeed = 0;
                        LayoutComponent layout = {};
                        EntityDefinition* definition = GetEntityTypeDefinition(group->assets, components[componentIndex]);
                        
                        CommonEntityInitParams common = definition->common;
                        common.type = type;
                        common.essences = 0;
                        ClientEntityInitParams entityParams = definition->client;
                        entityParams.seed = componentSeed;
                        
                        InitLayoutComponent(worldMode, group->assets, &layout, {}, &common, 0, &entityParams);
                        LayoutContainer componentContainer = {};
                        
                        if(transform.upright)
                        {
                            RenderLayoutInRectCameraAligned(worldMode, group, spaceDim.P, componentRect, transform, &layout, componentSeed, lights, &componentContainer, 0);
                        }
                        else
                        {
                            RenderLayoutInRect(worldMode, group, componentRect, transform, &layout, componentSeed, lights, &componentContainer, 0, 0);
                        }
                    }
                    
                    componentRect = Offset(componentRect, V2(componentWidth, 0));
                }
            }
        }
        
        LayoutComponent layout = {};
        EntityDefinition* definition = GetEntityTypeDefinition(group->assets, type);
        
        CommonEntityInitParams common = definition->common;
        common.type = type;
        common.essences = recipeEssences;
        ClientEntityInitParams entityParams = definition->client;
        entityParams.seed = recipeSeed;
        
        InitLayoutComponent(worldMode, group->assets, &layout, {}, &common, 0, &entityParams);
        LayoutContainer recipeContainer = {};
        
        transform.cameraOffset.z += 0.001f;
        if(transform.upright)
        {
            RenderLayoutInRectCameraAligned(worldMode, group, spaceDim.P, recipeRect, transform, &layout, recipeSeed, lights, &recipeContainer, 0);
        }
        else
        {
            RenderLayoutInRect(worldMode, group, recipeRect, transform, &layout, recipeSeed, lights, &recipeContainer, 0, 0);
        }
    }
}

internal void DrawInfusedEffects(GameModeWorld* worldMode, RenderGroup* group, ObjectTransform transform, Vec3 P, BitmapDim spaceDim, Rect2 rect, Lights lights, InfusedEffectsComponent* effects, EntityDefinition* definition)
{
    if(worldMode)
    {
        transform.modulationPercentage = 0;
        u32 effectCount = 0;
        for(u32 effectIndex = 0; effectIndex < ArrayCount(effects->effects); ++effectIndex)
        {
            InfusedEffect* effect = effects->effects + effectIndex;
            if(effect->essenceCount > 0)
            {
                ++effectCount;
            }
        }
        
        if(effectCount > 0)
        {
            Vec2 effectDim = V2(GetDim(rect).x, GetDim(rect).y / effectCount);
            Vec2 startingEffectP = rect.min;
            
            for(u32 effectIndex = 0; effectIndex < ArrayCount(effects->effects); ++effectIndex)
            {
                
                InfusedEffect* effect = effects->effects + effectIndex;
                InfuseEffect* reference = definition->common.infuseEffects + effect->effectIndex;
                
                if(effect->essenceCount > 0)
                {
                    u32 maxEssenceCount = 5;
                    Assert(effect->essenceCount == reference->requiredEssenceCount);
                    Assert(effect->essenceCount <= maxEssenceCount);
                    
                    Rect2 effectRect = RectMinDim(startingEffectP, effectDim);
                    startingEffectP.y += effectDim.y;
                    
                    Vec2 essenceStartingP = effectRect.min;
                    Vec2 essenceDim = V2((0.5f * effectDim.x) / maxEssenceCount, effectDim.y);
                    
                    Rect2 totalEssenceRect = RectMinDim(essenceStartingP, V2(essenceDim.x * maxEssenceCount, essenceDim.y));
                    effect->projectedOnScreenEssence = totalEssenceRect;
                    
                    
                    
                    for(u32 essenceIndex = 0; essenceIndex < effect->essenceCount; ++essenceIndex)
                    {
                        Rect2 essenceRect = RectMinDim(essenceStartingP, essenceDim);
                        essenceStartingP.x += essenceDim.x;
                        
                        u16 essence = reference->requiredEssences[essenceIndex].essence.value;
                        OverdrawEssence(worldMode, group, transform, essence, essenceRect, V4(1, 1, 1, 1));
                    }
                    
                    
                    
                    ObjectTransform effectTransform = transform;
                    if(effect->level == 0)
                    {
                        effectTransform.tint.a *= 0.5f;
                    }
                    
                    Vec2 iconMin = rect.min + V2(0.75f * effectDim.x, 0);
                    Vec2 iconDim = V2(0.25f * effectDim.x, effectDim.y);
                    
                    Rect2 iconRect = RectMinDim(iconMin, iconDim);
                    
                    effect->projectedOnScreenEffect = iconRect;
                    ImageReference icon = {};
                    InitImageReference_(group->assets, &icon, &definition->common.infuseEffects[effect->effectIndex].iconProperties);
                    if(icon.emittors)
                    {
                        effectTransform.lightInfluence = 1.0f;
                    }
                    
                    BitmapId BID = GetImageFromReference(group->assets, &icon, 0);
                    PushBitmapInRect(group, effectTransform, BID, P, iconRect, lights);
                }
            }
        }
    }
}



internal InventorySlot* FindCompatibleSlot(InventorySlot* slots, u32 slotCount, b32* alreadyDrawn, u32 alreadyDrawnCount, u16 inventorySlotType)
{
    Assert(slotCount == alreadyDrawnCount);
    
    InventorySlot* result = 0;
    for(u32 slotIndex = 0; slotIndex < slotCount; ++slotIndex)
    {
        InventorySlot* slot = slots + slotIndex;
        
        u16 slotType = SafeTruncateToU16(slot->flags_type & 0xffff);
        if(slotType == inventorySlotType)
        {
            if(!alreadyDrawn[slotIndex])
            {
                alreadyDrawn[slotIndex] = true;
                result = slot;
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
    u64 infusedSpaceHash = StringHash("infusedSpace");
    
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
        
        case LayoutContainerDraw_Container:
        {
            pieces = layout->containerPieces;
            pieceCount = layout->containerPieceCount;
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
                Vec3 pieceP = P + piece->offset;
                ObjectTransform pieceTransform = transform;
                pieceTransform.cameraOffset.z += piece->image.zOffset;
                
                if(piece->image.emittors)
                {
                    pieceTransform.lightInfluence = 1.0f;
                }
                
                if(piece->image.flat)
                {
                    pieceTransform.upright = false;
                }
                
                PAKBitmap* bitmap = GetBitmapInfo(group->assets, BID);
                if(bitmap->flippedByDefault)
                {
                    pieceTransform.scale.x = -pieceTransform.scale.x;
                }
                pieceTransform.tint = Hadamart(pieceTransform.tint, piece->color);
                BitmapDim dim = GetBitmapDim(group, pieceTransform, BID, pieceP, piece->height);
                if(transform.upright)
                {
                    result = ProjectOnScreen(group, dim);
                }
                else
                {
                    result = RectMinDim(dim.P.xy, dim.size);
                }
                
                PushBitmap(group, pieceTransform, BID, pieceP, piece->height, lights);
                if(true)
                {
                    if(piece->nameHash == emptySpaceHash)
                    {
                        ContainerComponent* c = container->container;
                        if(c)
                        {
                            InventorySlot* slot = FindCompatibleSlot(c->storedObjects, ArrayCount(c->storedObjects),container->storedObjectsDrawn, ArrayCount(container->storedObjectsDrawn),piece->inventorySlotType);
                            if(slot)
                            {
                                r32 zoomSpeed = 0.0f;
                                if(slot->hot)
                                {
                                    slot->hot = false;
                                    zoomSpeed = (slot->maxZoomCoeff - slot->zoomCoeff);
                                }
                                else
                                {
                                    zoomSpeed = -(slot->zoomCoeff - 1.0f);
                                }
                                slot->zoomCoeff += elapsedTime * zoomSpeed;
                                slot->zoomCoeff = Clamp(1.0f, slot->zoomCoeff, slot->maxZoomCoeff);
                                Rect2 objectRect = Scale(result, slot->zoomCoeff);
                                
                                slot->projOnScreen = Scale(result, slot->zoomCoeff);
                                ObjectTransform inventoryTransform = transform;
                                inventoryTransform.cameraOffset.z += 0.001f;
                                DrawObjectSlot(worldMode, group, slot, inventoryTransform, dim, objectRect, lights, elapsedTime);
                            }
                        }
                        result = InvertedInfinityRect2();
                        break;
                    }
                    
                    else if(piece->nameHash == usingSpaceHash)
                    {
                        ContainerComponent* c = container->container;
                        if(c)
                        {
                            InventorySlot* slot = FindCompatibleSlot(c->usingObjects, ArrayCount(c->usingObjects), container->usingObjectsDrawn, ArrayCount(container->usingObjectsDrawn),piece->inventorySlotType);
                            if(slot)
                            {
                                r32 zoomSpeed = 0.0f;
                                if(slot->hot)
                                {
                                    slot->hot = false;
                                    zoomSpeed = (slot->maxZoomCoeff - slot->zoomCoeff);
                                }
                                else
                                {
                                    zoomSpeed = -(slot->zoomCoeff - 1.0f);
                                }
                                slot->zoomCoeff += elapsedTime * zoomSpeed * slot->zoomSpeed;
                                slot->zoomCoeff = Clamp(1.0f, slot->zoomCoeff, slot->maxZoomCoeff);
                                Rect2 objectRect = Scale(result, slot->zoomCoeff);
                                
                                slot->projOnScreen = result;
                                ObjectTransform inventoryTransform = transform;
                                inventoryTransform.cameraOffset.z += 0.001f;
                                DrawObjectSlot(worldMode, group, slot, inventoryTransform, dim, objectRect, lights, elapsedTime);
                            }
                        }
                        result = InvertedInfinityRect2();
                        break;
                    }
                    else if(piece->nameHash == recipeSpaceHash)
                    {
                        ObjectTransform recipeTransform = transform;
                        Rect2 recipeRect = result;
                        DrawRecipeContent(worldMode, group, seed, recipeTransform, pieceP, dim, recipeRect, lights, 0);
                        result = InvertedInfinityRect2();
                        break;
                    }
                    else if(piece->nameHash == infusedSpaceHash)
                    {
                        if(container->infused && container->definition)
                        {
                            ObjectTransform infusedTransform = transform;
                            Rect2 infusedRect = result;
                            DrawInfusedEffects(worldMode, group, infusedTransform, pieceP, dim, infusedRect, lights, container->infused, container->definition);
                        }
                        result = InvertedInfinityRect2();
                        break;
                    }
                }
                
                for(u32 attachmentIndex = 0; attachmentIndex < bitmap->attachmentPointCount; ++attachmentIndex)
                {
                    PAKAttachmentPoint* attachmentPoint = GetAttachmentPoint(group->assets, BID, attachmentIndex);
                    
                    if(attachmentPoint)
                    {
                        Vec3 newP = GetAlignP(dim, attachmentPoint->alignment);
                        
                        ObjectTransform finalTransform = transform;
                        finalTransform.angle += attachmentPoint->angle;
                        finalTransform.scale = Hadamart(finalTransform.scale, attachmentPoint->scale);
                        finalTransform.cameraOffset.z += attachmentPoint->zOffset;
                        
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

internal void RenderLayoutInRectCameraAligned(GameModeWorld* worldMode, RenderGroup* group, Vec3 minP, Rect2 screenRect, ObjectTransform transform, LayoutComponent* layout, u32 seed, Lights lights, LayoutContainer* container, r32 elapsedTime)
{
    Vec2 desiredDim = GetDim(screenRect);
    Rect2 layoutDim = GetLayoutDim(group, minP, transform, layout, seed, container);
    Vec2 dim = GetDim(layoutDim);
    r32 scale = Min(desiredDim.x / dim.x, desiredDim.y / dim.y);
    transform.scale *= scale;
    
    Rect2 finalLayoutDim = GetLayoutDim(group, minP, transform, layout, seed, container);
    
    Vec3 drawnCenter = UnprojectAtZ(group, &group->gameCamera, GetCenter(finalLayoutDim), minP.z);
    Vec3 desiredCenter = UnprojectAtZ(group, &group->gameCamera, GetCenter(screenRect), minP.z);
    
    Vec3 offset = desiredCenter - drawnCenter;
    minP += offset.x * group->gameCamera.X;
    minP += offset.y * group->gameCamera.Y;
    
    RenderLayout(worldMode, group, minP, transform, layout, seed, lights, container, elapsedTime);
}

internal void RenderLayoutEntity(GameModeWorld* worldMode, RenderGroup* group, EntityID ID, r32 elapsedTime, LayoutContainerDrawMode mode)
{
    EntityAnimationParams params = GetEntityAnimationParams(worldMode, ID);
    elapsedTime *= params.speed;
    BaseComponent* base = GetComponent(worldMode, ID, BaseComponent);
    if(ShouldBeRendered(worldMode, base))
    {
        r32 height = GetHeight(base->bounds);
        r32 deepness = GetWidth(base->bounds);
        r32 width = GetWidth(base->bounds);
        
        Vec3 P = GetRelativeP(worldMode, ID);
        Lights lights = GetLights(worldMode, P);
        LayoutComponent* layout = GetComponent(worldMode, ID, LayoutComponent);
        
        ObjectTransform transform = BillboardTransform();
        transform.angle = layout->rootAngle;
        transform.scale = layout->rootScale * params.scaleAccumulated * params.scaleComputed;
        transform.tint = params.tint;
        transform.modulationPercentage = params.modulationPercentage; 
        transform.dissolvePercentages = params.dissolveCoeff * V4(1, 1, 1, 1); 
        
        ContainerComponent* container = GetComponent(worldMode, ID, ContainerComponent);
        LayoutContainer layoutContainer = {};
        layoutContainer.drawMode = mode;
        layoutContainer.container = container; 
        
        if(container && AreEqual(container->openedBy, worldMode->player.serverID) && worldMode->gameUI.lootingMode)
        {
            layoutContainer.drawMode = LayoutContainerDraw_Open;
        }
        
        RenderLayout(worldMode, group, P, transform, layout, base->seed, lights, &layoutContainer, elapsedTime);
    }
}

RENDERING_ECS_JOB_CLIENT(RenderLayoutEntities)
{
    RenderLayoutEntity(worldMode, group, ID, elapsedTime, LayoutContainerDraw_Standard);
}

RENDERING_ECS_JOB_CLIENT(RenderStatue)
{
    StatueComponent* statue = GetComponent(worldMode, ID, StatueComponent);
    LayoutComponent* layout = GetComponent(worldMode, ID, LayoutComponent);
    
    for(u32 effectIndex = 0; effectIndex < statue->effectCount; ++effectIndex)
    {
        SculptureEffect * effect = statue->effects + effectIndex;
        effect->runningTime += elapsedTime * effect->speed;
        
        for(u32 pieceIndex = 0; pieceIndex < layout->pieceCount; ++pieceIndex)
        {
            LayoutPiece* piece = layout->pieces + pieceIndex;
            if(piece->nameHash == effect->pieceHash)
            {
                r32 lerp = Clamp01MapToRange(-1.0f, Sin(effect->runningTime), 1.0f);
                piece->offset = Lerp(effect->activeMinCameraOffset, lerp, effect->activeMaxCameraOffset);
            }
        }
    }
    
    
    RenderLayoutEntity(worldMode, group, ID, elapsedTime, LayoutContainerDraw_Standard);
}

internal void AddAnimationEffectIfNotPresent(GameModeWorld* worldMode, AnimationEffectComponent* effects, AnimationEffectDefinition* effect, u16 ID)
{
    b32 add = true;
    for(u32 effectIndex = 0; effectIndex < effects->effectCount; ++effectIndex)
    {
        AnimationEffect* test = effects->effects + effectIndex;
        if(test->ID == ID)
        {
            add = false;
            
            if(effect->refreshWhenAlreadyPresent)
            {
                test->time = 0;
            }
            break;
        }
    }
    
    if(add && effects->effectCount < ArrayCount(effects->effects))
    {
        AnimationEffect* dest = effects->effects + effects->effectCount++;
        dest->ID = ID;
        
        dest->type = SafeTruncateToU16(ConvertEnumerator(AnimationEffectType, effect->type));
        dest->time = 0;
        dest->targetTime = effect->targetTime;
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
                    
                    dest->particles = GetNewParticleEffect(worldMode->particleCache, particles);
                }
            } break;
            
            case AnimationEffect_SineOffset:
            {
                dest->maxOffset = effect->sineMaxOffset;
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

internal void AnimationEventTrigger(GameModeWorld* worldMode, EntityID ID, GameProperty trigger)
{
    b32 isPlayer = (AreEqual(ID, worldMode->player.clientID));
    if(HasComponent(ID, AnimationEffectComponent))
    {
        BaseComponent* base = GetComponent(worldMode, ID, BaseComponent);
        AnimationEffectComponent* effects = GetComponent(worldMode, ID, AnimationEffectComponent);
        
        EntityDefinition* definition = GetEntityTypeDefinition(worldMode->gameState->assets, base->type);
        if(definition)
        {
            for(u32 effectIndex = 0; effectIndex < definition->client.animationEffectsCount; ++effectIndex)
            {
                AnimationEffectDefinition* effect = definition->client.animationEffects + effectIndex;
                if(AreEqual(effect->triggerType, trigger) && (isPlayer == effect->playerEffect))
                {
                    AddAnimationEffectIfNotPresent(worldMode, effects, effect, SafeTruncateToU16(effectIndex));
                }
            }
        }
    }
}

STANDARD_ECS_JOB_CLIENT(UpdateEntityEffects)
{
    BaseComponent* base = GetComponent(worldMode, ID, BaseComponent);
    AnimationEffectComponent* effects = GetComponent(worldMode, ID, AnimationEffectComponent);
    InteractionComponent* interaction = GetComponent(worldMode, ID, InteractionComponent);
    
    Vec3 P = GetRelativeP(worldMode, ID);
    
    r32 oldScale = effects->params.scaleAccumulated;
    Vec3 oldOffset = effects->params.offsetAccumulated;
    
    effects->params = DefaultAnimationParams();
    
    effects->params.scaleAccumulated = oldScale;
    effects->params.offsetAccumulated = oldOffset;
    
    
    Vec4 averageTint = {};
    u32 tintCount = 0;
    
    r32 averageLightIntensity = 0;
    Vec3 averageLightColor = {};
    u32 lightCount = 0;
    
    for(u32 effectIndex = 0; effectIndex < effects->effectCount; ++effectIndex)
    {
        AnimationEffect* effect = effects->effects + effectIndex;
        effect->time += elapsedTime;
        
        if(effect->targetTime >= 0 && effect->time >= effect->targetTime)
        {
            FreeAnimationEffect(effect);
            *effect = effects->effects[--effects->effectCount];
        }
        else
        {
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
                    SetEffectParameters(effect->particles, P, {}, 1.0f);
                } break;
                
                case AnimationEffect_SineOffset:
                {
                    effects->params.offsetComputed = Clamp01MapToRange(-1, Sin(effect->time), 1) * effect->maxOffset;
                } break;
                
                InvalidDefaultCase;
            }
        }
    }
    
    b32 zoom = false;
    if(interaction && interaction->isOnFocus)
    {
        effects->params.modulationPercentage = effects->outlineWidth;
        
        r32 maxDistanceSqZoom = Square(2.0f);
        r32 distanceSq =LengthSq(SubtractOnSameZChunk(base->universeP, worldMode->player.universeP));
        if(distanceSq <= maxDistanceSqZoom)
        {
            effects->params.scaleAccumulated += effects->speedOnFocus * elapsedTime;
            effects->params.scaleAccumulated = Min(effects->params.scaleAccumulated, effects->scaleMaxOnFocus);
            
            effects->params.offsetAccumulated += effects->speedOnFocus * V3(elapsedTime, elapsedTime, elapsedTime);
            effects->params.offsetAccumulated.x = Min(effects->params.offsetAccumulated.x, effects->offsetMaxOnFocus.x);
            effects->params.offsetAccumulated.y = Min(effects->params.offsetAccumulated.y, effects->offsetMaxOnFocus.y);
            effects->params.offsetAccumulated.z = Min(effects->params.offsetAccumulated.z, effects->offsetMaxOnFocus.z);
            zoom = true;
        }
    }
    
    if(!zoom)
    {
        effects->params.scaleAccumulated -= effects->speedOnNoFocus * elapsedTime;
        effects->params.scaleAccumulated = Max(effects->params.scaleAccumulated, 1.0f);
        
        effects->params.offsetAccumulated -= effects->speedOnNoFocus * V3(elapsedTime, elapsedTime, elapsedTime);
        effects->params.offsetAccumulated.x = Max(effects->params.offsetAccumulated.x, 0);
        effects->params.offsetAccumulated.y = Max(effects->params.offsetAccumulated.y, 0);
        effects->params.offsetAccumulated.z = Max(effects->params.offsetAccumulated.z, 0);
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
    
    ActionComponent* action = GetComponent(worldMode, ID, ActionComponent);
    if(action)
    {
        effects->params.speed *= action->speed;
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
    
    
    HealthComponent* health = GetComponent(worldMode, ID, HealthComponent);
    if(health)
    {
        r32 mRatio = (r32) health->mentalHealth / (r32)health->maxMentalHealth;
        r32 hRatio = (r32) health->physicalHealth / (r32)health->maxPhysicalHealth;
        
        Vec4 lifeColor = V4(hRatio, hRatio, hRatio, 1.0f);
        effects->params.tint = Hadamart(effects->params.tint, lifeColor);
        
        effects->params.dissolveCoeff = Max(effects->params.dissolveCoeff, Lerp(0.0f, 1.0f - mRatio, MAX_DISSOLVE_ENERGY)); 
        
        
        r32 fireRatio = Clamp01(health->onFirePercentage);
        Vec4 fireColor = Lerp(V4(1, 1, 1, 1), fireRatio, V4(1, 0, 0, 1));
        
        r32 poisonRatio = Clamp01(health->poisonPercentage);
        Vec4 poisonColor = Lerp(V4(1, 1, 1, 1), poisonRatio, V4(0, 1, 0, 1));
        
        
        effects->params.tint = Hadamart(effects->params.tint, fireColor);
        effects->params.tint = Hadamart(effects->params.tint, poisonColor);
    }
    
    r32 occludingDissolve = Clamp01MapToRange(0, base->occludingTime, effects->occludeDissolveTime);
    occludingDissolve = Lerp(0.0f, occludingDissolve, effects->occludeDissolvePercentage);
    
    effects->params.dissolveCoeff = Max(effects->params.dissolveCoeff, occludingDissolve);
}

RENDERING_ECS_JOB_CLIENT(RenderEntity)
{
    if(HasComponent(ID, ShadowComponent))
    {
        RenderShadow(worldMode, group, ID, elapsedTime);
    }
    
    if(HasComponent(ID, GrassComponent) && HasComponent(ID, MagicQuadComponent))
    {
        RenderGrass(worldMode, group, ID, elapsedTime);
    }
    else if(HasComponent(ID, StandardImageComponent))
    {
        RenderSpriteEntities(worldMode, group, ID, elapsedTime);
    }
    else if(HasComponent(ID, SegmentImageComponent))
    {
        RenderSegmentEntities(worldMode, group, ID, elapsedTime);
    }
    else if(HasComponent(ID, AnimationComponent))
    {
        RenderCharacterAnimation(worldMode, group, ID, elapsedTime);
    }
    else if(HasComponent(ID, FrameByFrameAnimationComponent))
    {
        RenderFrameByFrameEntities(worldMode, group, ID, elapsedTime);
    }
    else if(HasComponent(ID, PlantComponent))
    {
        RenderPlant(worldMode, group, ID, elapsedTime);
    }
    else if(HasComponent(ID, RockComponent))
    {
        RenderRock(worldMode, group, ID, elapsedTime);
    }
    else if(HasComponent(ID, StatueComponent))
    {
        RenderStatue(worldMode, group, ID, elapsedTime);
    }
    else if(HasComponent(ID, LayoutComponent))
    {
        RenderLayoutEntities(worldMode, group, ID, elapsedTime);
    }
    else if(HasComponent(ID, MultipartAnimationComponent))
    {
        RenderMultipartEntity(worldMode, group, ID, elapsedTime);
    }
}

#if 0
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
#endif
