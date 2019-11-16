internal b32 ShouldBeRendered(GameModeWorld* worldMode, BaseComponent* base)

{
    b32 result = false;
    if(base)
    {
        result = ((base->universeP.chunkZ == worldMode->player.universeP.chunkZ) &&
                  !(base->flags & EntityFlag_notInWorld) &&
                  !(base->flags & EntityFlag_occluding));
    }
    return result;
}

internal Vec4 GetTint(GameModeWorld* worldMode, EntityID ID)
{
    AnimationEffectComponent* animationEffects = GetComponent(worldMode, ID, AnimationEffectComponent);
    Vec4 result = animationEffects ? animationEffects->tint : V4(1, 1, 1, 1);
    
    return result;
}

internal void SlowDown(GameModeWorld* worldMode, EntityID ID, r32* timeToAdvance)
{
    AnimationEffectComponent* animationEffects = GetComponent(worldMode, ID, AnimationEffectComponent);
    if(animationEffects)
    {
        *timeToAdvance *= (1.0f - animationEffects->slowDownCoeff);
    }
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
            r32 nativeDeepness = shadow->nativeHeight;
            r32 nativeWidth = shadow->nativeHeight * shadow->widthOverHeight;
            
            r32 yScale = deepness / nativeDeepness;
            r32 xScale = width / nativeWidth;
            
            ObjectTransform transform = FlatTransform(0.01f);
            transform.scale = Hadamart(shadowComponent->scale, V2(xScale, yScale));
            PushBitmap(group, transform, shadowID, P + shadowComponent->offset, 0, shadowComponent->color, lights);
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
    SlowDown(worldMode, ID, &elapsedTime);
    BaseComponent* base = GetComponent(worldMode, ID, BaseComponent);
    if(ShouldBeRendered(worldMode, base))
    {
        r32 height = GetHeight(base->bounds);
        r32 deepness = GetWidth(base->bounds);
        r32 width = GetWidth(base->bounds);
        
        Vec3 P = GetRelativeP(worldMode, base);
        AnimationComponent* animation = GetComponent(worldMode, ID, AnimationComponent);
        RenderShadow(worldMode, group, P, &animation->shadow, deepness, width);
        
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
        params.transform = UprightTransform();
        params.flipOnYAxis = animation->flipOnYAxis;
        params.equipment = GetComponent(worldMode, ID, EquipmentMappingComponent);
        params.equipped = GetComponent(worldMode, ID, UsingMappingComponent);
        params.tint = GetTint(worldMode, ID);
        params.modulationPercentage = GetModulationPercentage(worldMode, ID); 
        
        GameProperty action = SearchForProperty(base->properties, base->propertyCount, Property_action);
        if(FinishedSingleCycleAnimation(animation))
        {
            params.fakeAnimation = true;
        }
        
        AddOptionalGamePropertyRaw(&animation->skeletonProperties, action);
        
        if(action.value == idle)
        {
            Rect2 animationDefaultDim = GetAnimationDim(worldMode, group, animation, &params);
            r32 defaultHeight = GetDim(animationDefaultDim).y;
            animation->scale = height / defaultHeight;
        }
        
        params.scale = animation->scale;
        RenderAnimation(worldMode, group, animation, &params);
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

RENDERING_ECS_JOB_CLIENT(RenderPlants)
{
    SlowDown(worldMode, ID, &elapsedTime);
    BaseComponent* base = GetComponent(worldMode, ID, BaseComponent);
    if(ShouldBeRendered(worldMode, base))
    {
        r32 height = GetHeight(base->bounds);
        r32 deepness = GetWidth(base->bounds);
        r32 width = GetWidth(base->bounds);
        
        Vec3 P = GetRelativeP(worldMode, base);
        Lights lights = GetLights(worldMode, P);
        StandardImageComponent* image = GetComponent(worldMode, ID, StandardImageComponent);
        RenderShadow(worldMode, group, P, &image->shadow, deepness, width);
        
        r32 modulationPercentage = GetModulationPercentage(worldMode, ID); 
        
        PlantComponent* plant = GetComponent(worldMode, ID, PlantComponent);
        
        RandomSequence seq = Seed(base->seed);
        BitmapId BID = GetImageFromReference(group->assets, &image->entity, &seq);
        if(IsValid(BID))
        {  
            Vec4 color = GetTint(worldMode, ID);
            BitmapDim bitmapData = PushBitmap(group, UprightTransform(), BID, P, height, color, lights);
            
            Bitmap* bitmap = GetBitmap(group->assets, BID).bitmap;
            PAKBitmap* bitmapInfo = GetBitmapInfo(group->assets, BID);
            if(bitmap)
            {
                u64 leafHash = StringHash("leaf");
                BitmapId leafID = GetImageFromReference(group->assets, &plant->leaf, &seq);
				if(IsValid(leafID))
				{
					Bitmap* leafBitmap = GetBitmap(group->assets, leafID).bitmap;
					if(leafBitmap)
					{
                        PAKBitmap* leafInfo = GetBitmapInfo(group->assets, leafID);
                        Vec2 leafPivot = V2(leafInfo->align[0], leafInfo->align[1]);
						for(u32 attachmentPointIndex = 0;
                            attachmentPointIndex < bitmapInfo->attachmentPointCount; ++attachmentPointIndex)
						{
							PAKAttachmentPoint* point = bitmap->attachmentPoints + attachmentPointIndex;
							if(StringHash(point->name) == leafHash)
							{
								Vec3 pointP = GetAlignP(bitmapData, point->alignment);
                                
								r32 angleRad = DegToRad(point->angle);
								Vec3 XAxis = V3(Cos(angleRad), Sin(angleRad), 0.0f);
								Vec3 YAxis  = V3(Perp(XAxis.xy), 0.0f);
                                
								Vec4 lateral = 0.5f * point->scale.x * (XAxis.x * magicLateralVector + XAxis.y * magicUpVector);
								Vec4 up = 0.5f * point->scale.y * (YAxis.x * magicLateralVector + YAxis.y * magicUpVector);
                                
								Vec3 offset = (leafPivot.x - 0.5f) * lateral.xyz + (leafPivot.y - 0.5f) * up.xyz;
								pointP -= offset;
                                u32 C = 0xffffffff;
                                
                                Vec4 windInfluences = V4(0, 0, plant->windInfluence, plant->windInfluence);
								PushMagicQuad(group, V4(pointP, 0), lateral, up, C, leafBitmap->textureHandle, lights, modulationPercentage, 0, 0, windInfluences);
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
    SlowDown(worldMode, ID, &elapsedTime);
    BaseComponent* base = GetComponent(worldMode, ID, BaseComponent);
    
    if(ShouldBeRendered(worldMode, base))
    {
        r32 height = GetHeight(base->bounds);
        r32 deepness = GetWidth(base->bounds);
        r32 width = GetWidth(base->bounds);
        
        Vec3 P = GetRelativeP(worldMode, base);
        Lights lights = GetLights(worldMode, P);
        StandardImageComponent* image = GetComponent(worldMode, ID, StandardImageComponent);
        RenderShadow(worldMode, group, P, &image->shadow, deepness, width);
        
        RandomSequence seq = Seed(base->seed);
        BitmapId BID = GetImageFromReference(group->assets, &image->entity, &seq);
        if(IsValid(BID))
        {
            ObjectTransform transform = UprightTransform();
            transform.modulationPercentage = GetModulationPercentage(worldMode, ID); 
            PushBitmap(group, UprightTransform(), BID, P, height, GetTint(worldMode, ID), lights);
        }
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
                Vec3 P = GetRelativeP(worldMode, base->universeP);
                Lights lights = GetLights(worldMode, P);
                
                P -= quad->offset;
                
                Vec4 windInfluences = V4(0, 0, grass->windInfluence, grass->windInfluence);
                PushMagicQuad(group, V4(P, 0), quad->lateral, quad->up, quad->color, bitmap->textureHandle, lights, 0, 0, 0, windInfluences);
            }
            else
            {
                LoadBitmap(group->assets, quad->bitmapID);
            }
        }
    }
}

RENDERING_ECS_JOB_CLIENT(RenderFrameByFrameEntities)
{
    SlowDown(worldMode, ID, &elapsedTime);
    BaseComponent* base = GetComponent(worldMode, ID, BaseComponent);
    
    if(ShouldBeRendered(worldMode, base))
    {
        r32 height = GetHeight(base->bounds);
        r32 deepness = GetWidth(base->bounds);
        r32 width = GetWidth(base->bounds);
        
        Vec3 P = GetRelativeP(worldMode, base);
        Lights lights = GetLights(worldMode, P);
        FrameByFrameAnimationComponent* animation = GetComponent(worldMode, ID, FrameByFrameAnimationComponent);
        animation->runningTime += elapsedTime * animation->speed;
        
        
        RenderShadow(worldMode, group, P, &animation->shadow, deepness, width);
        
        BitmapId BID = GetCorrenspondingFrameByFrameImage(group->assets, animation->typeHash, animation->runningTime);
        if(IsValid(BID))
        {
            ObjectTransform transform = UprightTransform();
            transform.modulationPercentage = GetModulationPercentage(worldMode, ID); 
            PushBitmap(group, UprightTransform(), BID, P, height, GetTint(worldMode, ID), lights);
        }
    }
}


internal void RenderLayoutInRectCameraAligned(GameModeWorld* worldMode, RenderGroup* group, Vec3 P, Vec3 rectP, Rect2 cameraRect, ObjectTransform transform, Vec4 color, LayoutComponent* layout, u32 seed, Lights lights, LayoutContainer* container);
internal void RenderLayoutInRect(GameModeWorld* worldMode, RenderGroup* group, Rect2 rect, ObjectTransform transform, Vec4 color, LayoutComponent* layout, u32 seed, Lights lights, LayoutContainer* container);
internal void DrawObjectMapping(GameModeWorld* worldMode, RenderGroup* group, ObjectMapping* mapping, ObjectTransform transform, Vec3 P, BitmapDim spaceDim, Rect2 rect, Lights lights)
{
    if(IsValidInventoryMapping(&worldMode->gameUI, mapping))
    {
        EntityID objectID = mapping->object.ID;
        BaseComponent* objectBase = GetComponent(worldMode, objectID, BaseComponent);
        transform.modulationPercentage = GetModulationPercentage(worldMode, objectID);
        LayoutComponent* objectLayout = GetComponent(worldMode, objectID, LayoutComponent);
        
        Vec4 color = GetTint(worldMode, objectID);
        if(objectBase && objectLayout)
        {
            LayoutContainer objectContainer = {};
            
            if(transform.upright)
            {
                RenderLayoutInRectCameraAligned(worldMode, group, P, spaceDim.P, rect, transform, color, objectLayout, objectBase->seed, lights, &objectContainer);
            }
            else
            {
                RenderLayoutInRect(worldMode, group, rect, transform, color, objectLayout, objectBase->seed, lights, &objectContainer);
            }
        }
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
        if(slot->type == inventorySlotType)
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

internal Rect2 RenderLayoutRecursive_(GameModeWorld* worldMode, RenderGroup* group, Vec3 P, ObjectTransform transform, Vec4 color, LayoutComponent* layout, u64 nameHash, u32 seed, Lights lights, LayoutContainer* container)
{
    u64 emptySpaceHash = StringHash("emptySpace");
    u64 usingSpaceHash = StringHash("usingSpace");
    
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
                transform.additionalZBias += 0.01f * pieceIndex;
                BitmapDim dim = GetBitmapDim(group, transform, BID, P, piece->height);
                if(transform.upright)
                {
                    result = ProjectOnScreen(group, dim);
                }
                else
                {
                    result = RectMinDim(dim.P.xy, dim.size);
                }
                
                r32 alpha = 0.7f;         
                if(container->container)
                {
                    ContainerMappingComponent* c = container->container;
                    if(piece->nameHash == emptySpaceHash)
                    {
                        ObjectMapping* mapping = FindCompatibleMapping(c->storedMappings, ArrayCount(c->storedMappings),container->storedObjectsDrawn, ArrayCount(container->storedObjectsDrawn),piece->inventorySlotType);
                        if(mapping)
                        {
                            if(mapping->hot)
                            {
                                alpha = 1.0f;
                            }
                            
                            mapping->hot = false;
                            mapping->projOnScreen = result;
                            DrawObjectMapping(worldMode, group, mapping, transform, P, dim, result, lights);
                        }
                        
                        result = InvertedInfinityRect2();
                    }
                    
                    else if(piece->nameHash == usingSpaceHash)
                    {
                        ObjectMapping* mapping = FindCompatibleMapping(c->usingMappings, ArrayCount(c->usingMappings), container->usingObjectsDrawn, ArrayCount(container->usingObjectsDrawn),piece->inventorySlotType);
                        if(mapping)
                        {
                            if(mapping->hot)
                            {
                                alpha = 1.0f;
                            }
                            
                            mapping->hot = false;
                            mapping->projOnScreen = result;
                            DrawObjectMapping(worldMode, group, mapping, transform, P, dim, result, lights);
                        }
                        result = InvertedInfinityRect2();
                    }
                }
                PushBitmap(group, transform, BID, P, piece->height, V4(1, 1, 1, alpha), lights);
                
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
                        
                        Rect2 subRect = RenderLayoutRecursive_(worldMode, group, newP, finalTransform, color, layout, StringHash(attachmentPoint->name), seed, lights, container);
                        result = Union(result, subRect);
                    }
                }
            }
        }
    }
    
    return result;
}

internal Rect2 RenderLayout(GameModeWorld* worldMode, RenderGroup* group, Vec3 P, ObjectTransform transform, Vec4 color, LayoutComponent* layout, u32 seed, Lights lights, LayoutContainer* container)
{
    Rect2 result = RenderLayoutRecursive_(worldMode, group, P, transform, color, layout, layout->rootHash, seed, lights, container);
    return result;
}

internal Rect2 GetLayoutDim(RenderGroup* group, Vec3 P, ObjectTransform transform, LayoutComponent* layout, u32 seed, LayoutContainer* container)
{
    transform.dontRender = true;
    LayoutContainer fakeContainer = {};
    fakeContainer.drawMode = container->drawMode;
    Rect2 result = RenderLayoutRecursive_(0, group, P, transform, V4(1, 1, 1, 1), layout, layout->rootHash, seed, {}, &fakeContainer);
    return result;
}

internal void RenderLayoutInRect(GameModeWorld* worldMode, RenderGroup* group, Rect2 rect, ObjectTransform transform, Vec4 color, LayoutComponent* layout, u32 seed, Lights lights, LayoutContainer* container)
{
    Vec3 fakeP = V3(GetCenter(rect), 0);
    Vec2 desiredDim = GetDim(rect);
    
    Rect2 layoutDim = GetLayoutDim(group, fakeP, transform, layout, seed, container);
    Vec2 dim = GetDim(layoutDim);
    r32 scale = Min(desiredDim.x / dim.x, desiredDim.y / dim.y);
    transform.scale *= scale;
    
    Rect2 finalLayoutDim = GetLayoutDim(group, fakeP, transform, layout, seed, container);
    Vec2 drawnP = GetCenter(finalLayoutDim);
    
    Vec3 offset = V3(drawnP - GetCenter(rect), 0);
    Vec3 finalP = fakeP - offset;
    
    RenderLayout(worldMode, group, finalP, transform, color, layout, seed, lights, container);
}

internal void RenderLayoutInRectCameraAligned(GameModeWorld* worldMode, RenderGroup* group, Vec3 P, Vec3 rectP, Rect2 cameraRect, ObjectTransform transform, Vec4 color, LayoutComponent* layout, u32 seed, Lights lights, LayoutContainer* container)
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
    
    RenderLayout(worldMode, group, P, transform, color, layout, seed, lights, container);
}

RENDERING_ECS_JOB_CLIENT(RenderLayoutEntities)
{
    SlowDown(worldMode, ID, &elapsedTime);
    BaseComponent* base = GetComponent(worldMode, ID, BaseComponent);
    if(ShouldBeRendered(worldMode, base))
    {
        r32 height = GetHeight(base->bounds);
        r32 deepness = GetWidth(base->bounds);
        r32 width = GetWidth(base->bounds);
        
        Vec3 P = GetRelativeP(worldMode, base);
        Lights lights = GetLights(worldMode, P);
        LayoutComponent* layout = GetComponent(worldMode, ID, LayoutComponent);
        RenderShadow(worldMode, group, P, &layout->shadow, deepness, width);
        
        ObjectTransform transform = UprightTransform();
        transform.angle = layout->rootAngle;
        transform.scale = layout->rootScale;
        transform.modulationPercentage = GetModulationPercentage(worldMode, ID); 
        
        ContainerMappingComponent* container = GetComponent(worldMode, ID, ContainerMappingComponent);
        LayoutContainer layoutContainer = {};
        if(container && AreEqual(container->openedBy, worldMode->player.serverID))
        {
            layoutContainer.container = container; 
            layoutContainer.drawMode = LayoutContainerDraw_Open;
        }
        
        Vec4 color = GetTint(worldMode, ID);
        RenderLayout(worldMode, group, P, transform, color, layout, base->seed, lights, &layoutContainer);
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
    
    Vec3 P = GetRelativeP(worldMode, base);
    
    effects->tint = V4(1, 1, 1, 1);
    effects->lightIntensity = 0;
    effects->slowDownCoeff = 0.0f;
    
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
                for(u32 propertyIndex = 0; propertyIndex < base->propertyCount; ++propertyIndex)
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
    
    if(tintCount > 0)
    {
        effects->tint = averageTint *= 1.0f / tintCount;
    }
    
    if(lightCount > 0)
    {
        effects->lightIntensity = averageLightIntensity *= 1.0f / lightCount;
        effects->lightColor = averageLightColor *= 1.0f / lightCount;
    }
    
    if(slowDownCount > 0)
    {
        effects->slowDownCoeff = averageSlowDown / slowDownCount;
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
        AddLightToGridNextFrame(worldMode, startP + 0.5f * deltaP, definition->lightColor, definition->lightIntensity);
        
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
            
            PushLineSegment(group, group->whiteTexture, color, subStartP, subEndP, definition->thickness, lights);
            subStartP = subEndP;
        }
        
        if(bolt->ttl <= definition->lightStartTime && bolt->ttl >= definition->lightEndTime)
        {
            AddLightToGridNextFrame(worldMode, endP, definition->lightColor, definition->lightIntensity);
        }
        
        EndTemporaryMemory(subdivisionsMemory);
    }
}