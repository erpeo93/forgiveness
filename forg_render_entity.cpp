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

RENDERING_ECS_JOB_CLIENT(RenderCharacterAnimation)
{
    BaseComponent* base = GetComponent(worldMode, ID, BaseComponent);
    if(ShouldBeRendered(worldMode, base))
    {
        r32 height = GetHeight(base);
        r32 deepness = GetWidth(base);
        r32 width = GetWidth(base);
        
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
        params.tint = V4(1, 1, 1, 1);
        params.modulationPercentage = GetModulationPercentage(worldMode, ID); 
        
        GameProperty action = base->action;
        if(FinishedSingleCycleAnimation(animation))
        {
            action = GameProp(action, idle);
            params.fakeAnimation = true;
        }
        
        AddOptionalGamePropertyRaw(&animation->skeletonProperties, action);
        
        
        AnimationEffectsComponent* effects = GetComponent(worldMode, ID, AnimationEffectsComponent);
        if(effects)
        {
            if(effects->timer > 0)
            {
                effects->timer -= elapsedTime;
                params.tint = effects->tint;
            }
        }
        
        if(base->action.value == idle)
        {
            Rect2 animationDefaultDim = GetAnimationDim(worldMode, group, animation, &params);
            r32 defaultHeight = GetDim(animationDefaultDim).y;
            animation->scale = height / defaultHeight;
        }
        
        params.scale = animation->scale;
        RenderAnimation(worldMode, group, animation, &params);
    }
}

internal BitmapId GetImageFromReference(Assets* assets, ImageReference* reference, RandomSequence* seq)
{
    u32 imageType = GetAssetSubtype(assets, AssetType_Image, reference->typeHash);
    BitmapId BID = QueryBitmaps(assets, imageType, seq, &reference->properties);
    
    return BID;
}

RENDERING_ECS_JOB_CLIENT(RenderPlants)
{
    BaseComponent* base = GetComponent(worldMode, ID, BaseComponent);
    if(ShouldBeRendered(worldMode, base))
    {
        r32 height = GetHeight(base);
        r32 deepness = GetWidth(base);
        r32 width = GetWidth(base);
        
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
            ObjectTransform stillTransform = UprightTransform();
            stillTransform.modulationPercentage = modulationPercentage;
            
            BitmapDim bitmapData = PushBitmap(group, UprightTransform(), BID, P, height, V4(1, 1, 1, 1), lights);
            
            Bitmap* bitmap = GetBitmap(group->assets, BID).bitmap;
            PAKBitmap* bitmapInfo = GetBitmapInfo(group->assets, BID);
            if(bitmap)
            {
                ObjectTransform leafTransform = UprightTransform();
                leafTransform.additionalZBias = 0.05f;
                u64 leafHash = StringHash("leaf");
                
                for(u32 attachmentPointIndex = 0;
                    attachmentPointIndex < bitmapInfo->attachmentPointCount; ++attachmentPointIndex)
                {
                    PAKAttachmentPoint* point = bitmap->attachmentPoints + attachmentPointIndex;
                    if(StringHash(point->name) == leafHash)
                    {
                        BitmapId leafID = GetImageFromReference(group->assets, &plant->leaf, &seq);
                        leafTransform.angle = point->angle;
                        leafTransform.scale = point->scale;
                        leafTransform.modulationPercentage = modulationPercentage;
                        
                        Vec3 pointP = GetAlignP(bitmapData, point->alignment);
                        PushBitmap(group, leafTransform, leafID, pointP, 0, V4(1, 1, 1, 1), lights);
                    }
                }
            }
        }
    }
}

RENDERING_ECS_JOB_CLIENT(RenderSpriteEntities)
{
    BaseComponent* base = GetComponent(worldMode, ID, BaseComponent);
    if(ShouldBeRendered(worldMode, base))
    {
        r32 height = GetHeight(base);
        r32 deepness = GetWidth(base);
        r32 width = GetWidth(base);
        
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
            PushBitmap(group, UprightTransform(), BID, P, height, V4(1, 1, 1, 1), lights);
        }
    }
}


internal void RenderLayoutInRectCameraAligned(GameModeWorld* worldMode, RenderGroup* group, Vec3 P, Vec3 rectP, Rect2 cameraRect, ObjectTransform transform, LayoutComponent* layout, u32 seed, Lights lights, LayoutContainer* container);
internal void RenderLayoutInRect(GameModeWorld* worldMode, RenderGroup* group, Rect2 rect, ObjectTransform transform, LayoutComponent* layout, u32 seed, Lights lights, LayoutContainer* container);
internal void DrawObjectMapping(GameModeWorld* worldMode, RenderGroup* group, ObjectMapping* mapping, ObjectTransform transform, Vec3 P, BitmapDim spaceDim, Rect2 rect, Lights lights)
{
    if(IsValidInventoryMapping(&worldMode->gameUI, mapping))
    {
        EntityID objectID = mapping->object.ID;
        BaseComponent* objectBase = GetComponent(worldMode, objectID, BaseComponent);
        transform.modulationPercentage = GetModulationPercentage(worldMode, objectID);
        LayoutComponent* objectLayout = GetComponent(worldMode, objectID, LayoutComponent);
        if(objectBase && objectLayout)
        {
            LayoutContainer objectContainer = {};
            
            if(transform.upright)
            {
                RenderLayoutInRectCameraAligned(worldMode, group, P, spaceDim.P, rect, transform, objectLayout, objectBase->seed, lights, &objectContainer);
            }
            else
            {
                RenderLayoutInRect(worldMode, group, rect, transform, objectLayout, objectBase->seed, lights, &objectContainer);
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

internal Rect2 RenderLayoutRecursive_(GameModeWorld* worldMode, RenderGroup* group, Vec3 P, ObjectTransform transform, LayoutComponent* layout, u64 nameHash, u32 seed, Lights lights, LayoutContainer* container)
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
                        
                        Rect2 subRect = RenderLayoutRecursive_(worldMode, group, newP, finalTransform, layout, StringHash(attachmentPoint->name), seed, lights, container);
                        result = Union(result, subRect);
                    }
                }
            }
        }
    }
    
    return result;
}

internal Rect2 RenderLayout(GameModeWorld* worldMode, RenderGroup* group, Vec3 P, ObjectTransform transform, LayoutComponent* layout, u32 seed, Lights lights, LayoutContainer* container)
{
    Rect2 result = RenderLayoutRecursive_(worldMode, group, P, transform, layout, layout->rootHash, seed, lights, container);
    return result;
}

internal Rect2 GetLayoutDim(RenderGroup* group, Vec3 P, ObjectTransform transform, LayoutComponent* layout, u32 seed, LayoutContainer* container)
{
    transform.dontRender = true;
    LayoutContainer fakeContainer = {};
    fakeContainer.drawMode = container->drawMode;
    Rect2 result = RenderLayoutRecursive_(0, group, P, transform, layout, layout->rootHash, seed, {}, &fakeContainer);
    return result;
}

internal void RenderLayoutInRect(GameModeWorld* worldMode, RenderGroup* group, Rect2 rect, ObjectTransform transform, LayoutComponent* layout, u32 seed, Lights lights, LayoutContainer* container)
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
    
    RenderLayout(worldMode, group, finalP, transform, layout, seed, lights, container);
}

internal void RenderLayoutInRectCameraAligned(GameModeWorld* worldMode, RenderGroup* group, Vec3 P, Vec3 rectP, Rect2 cameraRect, ObjectTransform transform, LayoutComponent* layout, u32 seed, Lights lights, LayoutContainer* container)
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
    
    RenderLayout(worldMode, group, P, transform, layout, seed, lights, container);
}

RENDERING_ECS_JOB_CLIENT(RenderLayoutEntities)
{
    BaseComponent* base = GetComponent(worldMode, ID, BaseComponent);
    if(ShouldBeRendered(worldMode, base))
    {
        r32 height = GetHeight(base);
        r32 deepness = GetWidth(base);
        r32 width = GetWidth(base);
        
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
        
        RenderLayout(worldMode, group, P, transform, layout, base->seed, lights, &layoutContainer);
    }
}
