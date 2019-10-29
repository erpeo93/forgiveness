
#include "forg_client.h"
global_variable ClientNetworkInterface* clientNetwork; 

#include "forg_token.cpp"
#include "forg_pool.cpp"
#include "forg_resizable_array.cpp"
#include "forg_world.cpp"
#include "forg_physics.cpp"
#include "forg_asset.cpp"
#include "forg_world_generation.cpp"
#include "forg_render.cpp"
#include "forg_sound.cpp"
#include "forg_light.cpp"
#include "forg_animation.cpp"
#include "forg_camera.cpp"
#include "miniz.c"
#include "forg_network_client.cpp"
#include "forg_editor.cpp"
#include "forg_plant.cpp"
#include "forg_mesh.cpp"
#include "forg_rock.cpp"
#include "forg_particles.cpp"
//#include "forg_bolt.cpp"
#include "forg_cutscene.cpp"
#include "forg_ground.cpp"
#include "forg_UIcommon.cpp"
#include "forg_archetypes.cpp"
#include "forg_meta.cpp"
#include "forg_sort.cpp"

internal void PlayGame(GameState* gameState, PlatformInput* input)
{
    SetGameMode(gameState, GameMode_Playing);
    
    Clear(&gameState->persistentPool);
    Clear(&gameState->visualEffectsPool);
    
    GameModeWorld* result = PushStruct(&gameState->modePool, GameModeWorld);
    
    result->gameState = gameState;
    result->editorRoles = gameState->editorRoles;
#if 0
    char* loginServer = "forgiveness.hopto.org";
#else
    char* loginServer = "127.0.0.1";
#endif
    
    clientNetwork->nextSendStandardApplicationData = {};
    clientNetwork->nextSendOrderedApplicationData = {};
    ResetReceiver(&clientNetwork->receiver);
    platformAPI.net.OpenConnection(clientNetwork->network, loginServer, LOGIN_PORT);
    
    
    LoginRequest(4444);
    
    result->temporaryPool = &gameState->framePool;
    result->persistentPool = &gameState->persistentPool;
    
    
    result->particleCache = PushStruct(&gameState->modePool, ParticleCache, AlignClear(16));
    InitParticleCache(result->particleCache, gameState->assets, &gameState->visualEffectsPool);
    
    
#if 0    
    result->boltCache = PushStruct(&gameState->modePool, BoltCache);
    InitBoltCache(result->boltCache, &gameState->visualEffectsPool, 11111);
#endif
    
    
    gameState->world = result;
    
    result->chunkApron = 2;
    result->worldTileView = false;
    result->defaultCameraZ = 34.0f;
    result->cameraWorldOffset = V3(0.0f, 0.0f, result->defaultCameraZ);
    
    result->firstFreePlantSegment = 0;
    
    result->soundState = &gameState->soundState;
    
    result->editorUI.input = input;
    result->editorUI.pool = &gameState->assetsPool;
    result->editorUI.assets = gameState->assets;
    result->editorUI.soundState = result->soundState;
    
    result->editorUI.firstFreeCommand = 0;
    DLLIST_INIT(&result->editorUI.undoRedoSentinel);
    result->editorUI.currentCommand = &result->editorUI.undoRedoSentinel;
    
    for(u16 archetypeIndex = 0; archetypeIndex < Archetype_Count; ++archetypeIndex)
    {
        InitArchetype(result, archetypeIndex, 16);
    }
}

internal Vec3 HandleDaynightCycle(GameModeWorld* worldMode, PlatformInput* input)
{
    Vec3 ambientLightColor = V3(0.1f, 0.1f, 0.1f);
    return ambientLightColor;
}

internal r32 GetHeight(BaseComponent* base)
{
    r32 height = GetDim(base->bounds).z;
    return height;
}

internal r32 GetWidth(BaseComponent* base)
{
    r32 height = GetDim(base->bounds).x;
    return height;
}

internal r32 GetDeepness(BaseComponent* base)
{
    r32 height = GetDim(base->bounds).y;
    return height;
}

internal b32 ShouldBeRendered(GameModeWorld* worldMode, BaseComponent* base)

{
    b32 result = ((base->universeP.chunkZ == worldMode->player.universeP.chunkZ) &&
                  !(base->flags & EntityFlag_notInWorld));
    return result;
}

internal void DispatchGameEffect(GameModeWorld* worldMode, EntityID ID)
{
    // TODO(Leonardo): for now let's hardcode this, but in the future we would like to customize these on a entity basis
#if 0    
    EntityDefinitionDef* = GetDefinition();
    AddAllAnimationEffectsFor(def, effect->effectType.value);
#else
    AnimationEffectsComponent* effects = GetComponent(worldMode, ID, AnimationEffectsComponent);
    if(effects)
    {
        effects->timer = 0.3f;
        effects->tint = V4(0, 0, 0, 1);
    }
#endif
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
        
        AddOptionalGamePropertyRaw(&animation->skeletonProperties, base->action);
        
        
        AnimationParams params = {};
        params.elapsedTime = elapsedTime;
        params.angle = 0;
        params.P = P;
        params.lights = GetLights(worldMode, P);
        params.scale = 1;
        params.transform = UprightTransform();
        params.flipOnYAxis = animation->flipOnYAxis;
        params.equipment = GetComponent(worldMode, ID, EquipmentMappingComponent);
        params.equipped = GetComponent(worldMode, ID, UsingMappingComponent);
        params.tint = V4(1, 1, 1, 1);
        params.modulationPercentage = GetModulationPercentageAndResetFocus(base); 
        
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
        
        r32 modulationPercentage = GetModulationPercentageAndResetFocus(base); 
        
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
            transform.modulationPercentage = GetModulationPercentageAndResetFocus(base); 
            PushBitmap(group, UprightTransform(), BID, P, height, V4(1, 1, 1, 1), lights);
        }
    }
}


internal void RenderLayoutInRectCameraAligned(GameModeWorld* worldMode, RenderGroup* group, Vec3 P, Vec3 rectP, Rect2 cameraRect, ObjectTransform transform, LayoutComponent* layout, u32 seed, Lights lights, LayoutContainer* container);
internal void RenderLayoutInRect(GameModeWorld* worldMode, RenderGroup* group, Rect2 rect, ObjectTransform transform, LayoutComponent* layout, u32 seed, Lights lights, LayoutContainer* container);
internal Rect2 RenderLayoutRecursive_(GameModeWorld* worldMode, RenderGroup* group, Vec3 P, ObjectTransform transform, LayoutComponent* layout, u64 nameHash, u32 seed, Lights lights, LayoutContainer* container)
{
    u64 emptySpaceHash = StringHash("emptySpace");
    
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
                BitmapDim dim = PushBitmap(group, transform, BID, P, piece->height, V4(1, 1, 1, 1), lights);
                if(transform.upright)
                {
                    result = ProjectOnScreen(group, dim);
                }
                else
                {
                    result = RectMinDim(dim.P.xy, dim.size);
                }
                
                if(piece->nameHash == emptySpaceHash && container->container)
                {
                    ObjectMapping* mappings = (container->drawMode == LayoutContainerDraw_Using) ? container->container->usingMappings : container->container->storedMappings;
                    u32 mappingCount = (container->drawMode == LayoutContainerDraw_Using) ? ArrayCount(container->container->usingMappings) : ArrayCount(container->container->storedMappings);
                    
                    if(container->currentObjectIndex < mappingCount)
                    {
                        ObjectMapping* mapping = mappings + container->currentObjectIndex++;
                        EntityID objectID = mapping->ID;
                        
                        if(IsValid(objectID))
                        {
                            BaseComponent* objectBase = GetComponent(worldMode, objectID, BaseComponent);
                            transform.modulationPercentage = GetModulationPercentageAndResetFocus(objectBase);
                            LayoutComponent* objectLayout = GetComponent(worldMode, objectID, LayoutComponent);
                            if(objectBase && objectLayout)
                            {
                                Rect2 objectRect = result;
                                LayoutContainer objectContainer = {};
                                
                                mapping->projectedOnScreen = Offset(result, 0.5f * group->screenDim);
                                if(transform.upright)
                                {
                                    RenderLayoutInRectCameraAligned(worldMode, group, P, dim.P, objectRect, transform, objectLayout, objectBase->seed, lights, &objectContainer);
                                }
                                else
                                {
                                    RenderLayoutInRect(worldMode, group, objectRect, transform, objectLayout, objectBase->seed, lights, &objectContainer);
                                }
                            }
                        }
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

internal Rect2 GetLayoutDim(RenderGroup* group, Vec3 P, ObjectTransform transform, LayoutComponent* layout, u32 seed)
{
    transform.dontRender = true;
    LayoutContainer fakeContainer = {};
    Rect2 result = RenderLayoutRecursive_(0, group, P, transform, layout, layout->rootHash, seed, {}, &fakeContainer);
    return result;
}

internal void RenderLayoutInRect(GameModeWorld* worldMode, RenderGroup* group, Rect2 rect, ObjectTransform transform, LayoutComponent* layout, u32 seed, Lights lights, LayoutContainer* container)
{
    Vec3 fakeP = V3(GetCenter(rect), 0);
    Vec2 desiredDim = GetDim(rect);
    
    Rect2 layoutDim = GetLayoutDim(group, fakeP, transform, layout, seed);
    Vec2 dim = GetDim(layoutDim);
    r32 scale = Min(desiredDim.x / dim.x, desiredDim.y / dim.y);
    transform.scale *= scale;
    
    Rect2 finalLayoutDim = GetLayoutDim(group, fakeP, transform, layout, seed);
    Vec2 drawnP = GetCenter(finalLayoutDim);
    
    Vec3 offset = V3(drawnP - GetCenter(rect), 0);
    Vec3 finalP = fakeP - offset;
    
    RenderLayout(worldMode, group, finalP, transform, layout, seed, lights, container);
}

internal void RenderLayoutInRectCameraAligned(GameModeWorld* worldMode, RenderGroup* group, Vec3 P, Vec3 rectP, Rect2 cameraRect, ObjectTransform transform, LayoutComponent* layout, u32 seed, Lights lights, LayoutContainer* container)
{
    Vec2 desiredDim = GetDim(cameraRect);
    Rect2 layoutDim = GetLayoutDim(group, P, transform, layout, seed);
    Vec2 dim = GetDim(layoutDim);
    r32 scale = Min(desiredDim.x / dim.x, desiredDim.y / dim.y);
    transform.scale *= scale;
    
    Rect2 finalLayoutDim = GetLayoutDim(group, P, transform, layout, seed);
    
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
        transform.modulationPercentage = GetModulationPercentageAndResetFocus(base); 
        
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

internal Rect3 GetEntityBound(GameModeWorld* worldMode, BaseComponent* base)
{
    Rect3 result = Offset(base->bounds, GetRelativeP(worldMode, base));
    return result;
}

RENDERING_ECS_JOB_CLIENT(RenderBound)
{
    BaseComponent* base = GetComponent(worldMode, ID, BaseComponent);
    if(ShouldBeRendered(worldMode, base))
    {
        Rect3 entityBound = GetEntityBound(worldMode, base);
        PushCubeOutline(group, entityBound, V4(1, 0, 0, 1), 0.05f);
    }
}

internal Vec2 HandleKeyboardInteraction(GameModeWorld* worldMode, ClientPlayer* player, PlatformInput* input)
{
    GameCommand* command = &player->currentCommand;
    
    command->action = idle;
    command->acceleration = {};
    command->targetID = {};
    
    Vec2 cameraOffset = {};
    
    r32 cameraOffsetMagnitudo = 0.6f;
    if(IsDown(&input->moveLeft))
    {
        command->acceleration.x = -1.0f;
        cameraOffset.x = -cameraOffsetMagnitudo;
    }
    if(IsDown(&input->moveRight))
    {
        command->acceleration.x = 1.0f;
        cameraOffset.x = cameraOffsetMagnitudo;
    }
    if(IsDown(&input->moveDown))
    {
        command->acceleration.y = -1.0f;
        cameraOffset.y = -cameraOffsetMagnitudo;
    }
    if(IsDown(&input->moveUp))
    {
        command->acceleration.y = 1.0f;
        cameraOffset.y = cameraOffsetMagnitudo;
    }
    
    if(LengthSq(command->acceleration) > 0)
    {
        command->action = move;
    }
    
    if(Pressed(&input->mouseCenter))
    {
        if(IsValid(worldMode->lootingID))
        {
            if(worldMode->lootingMode)
            {
                worldMode->lootingMode = false;
                worldMode->inventoryMode = true;
            }
            else
            {
                worldMode->lootingMode = true;
                worldMode->inventoryMode = false;
            }
        }
        else
        {
            worldMode->inventoryMode = !worldMode->inventoryMode;
        }
    }
    
    return cameraOffset;
}

internal void ResetInteractions(GameModeWorld* worldMode)
{
    worldMode->hotCount = 0;
}

internal void AddPossibleInteraction(GameModeWorld* worldMode, InteractionType type, PossibleActionList* list, EntityID ID)
{
    if(worldMode->hotCount < ArrayCount(worldMode->hotInteractions))
    {
        EntityHotInteraction* dest = worldMode->hotInteractions + worldMode->hotCount++;
        dest->type = type;
        dest->actionCount = 0;
        dest->actions[0] = 0;
        
        for(u32 actionIndex = 0; actionIndex < list->actionCount; ++actionIndex)
        {
            if(dest->actionCount < ArrayCount(dest->actions))
            {
                dest->actions[dest->actionCount++] = list->actions[actionIndex];
            }
        }
        dest->ID = ID;
    }
}

internal void AddContainerObjectsInteractions(GameModeWorld* worldMode, ObjectMapping* mappings, u32 mappingCount, InteractionType interactionType)
{
	for(u32 mappingIndex = 0; mappingIndex < mappingCount; ++mappingIndex)
	{
        ObjectMapping* mapping = mappings + mappingIndex;
        
        if(IsValid(mapping->ID))
        {
            InteractionComponent* interaction = GetComponent(worldMode, mapping->ID, InteractionComponent);
            
            if(interaction && PointInRect(mapping->projectedOnScreen, worldMode->absoluteMouseP))
            {
                ResetInteractions(worldMode);
                AddPossibleInteraction(worldMode, interactionType, interaction->actions + interactionType, mapping->ID);
            }
        }
    }
}

internal void HandleEquipmentInteraction(GameModeWorld* worldMode, EntityID ID)
{
    EquipmentMappingComponent* equipment = GetComponent(worldMode, ID, EquipmentMappingComponent);
    if(equipment)
    {
        AddContainerObjectsInteractions(worldMode, equipment->mappings, ArrayCount(equipment->mappings), Interaction_Equipment);
    }
}

internal void HandleContainerInteraction(GameModeWorld* worldMode)
{
    if(IsValid(worldMode->openIDRight))
    {
        ContainerMappingComponent* container = GetComponent(worldMode, worldMode->openIDRight, ContainerMappingComponent);
        if(container)
        {
			AddContainerObjectsInteractions(worldMode, container->storedMappings, ArrayCount(container->storedMappings), Interaction_Container);
        }
    }
    
    if(IsValid(worldMode->openIDLeft))
    {
        ContainerMappingComponent* container = GetComponent(worldMode, worldMode->openIDLeft, ContainerMappingComponent);
        if(container)
        {
            AddContainerObjectsInteractions(worldMode, container->storedMappings, ArrayCount(container->storedMappings), Interaction_Container);
        }
    }
}

internal void HandleUsingSlotInteraction(GameModeWorld* worldMode)
{
    UsingMappingComponent* equipped = GetComponent(worldMode, worldMode->player.clientID, UsingMappingComponent);
    if(equipped)
    {
        AddContainerObjectsInteractions(worldMode, equipped->mappings, ArrayCount(equipped->mappings), Interaction_Equipped);
    }
    
    EquipmentMappingComponent* equipment = GetComponent(worldMode, worldMode->player.clientID, EquipmentMappingComponent);
    if(equipment)
    {
        for(u32 equipIndex = 0; equipIndex < ArrayCount(equipment->mappings); ++equipIndex)
        {
            EntityID ID = equipment->mappings[equipIndex].ID;
            ContainerMappingComponent* container = GetComponent(worldMode, ID, ContainerMappingComponent);
            if(container)
            {
                AddContainerObjectsInteractions(worldMode, container->usingMappings, ArrayCount(container->usingMappings), Interaction_Equipped);
            }
        }
    }
}

internal void OverdrawLayout(GameModeWorld* worldMode, RenderGroup* group, EntityID ID, Rect2 rect, LayoutContainerDrawMode drawMode)
{
    BaseComponent* base = GetComponent(worldMode, ID, BaseComponent);
    LayoutComponent* layout = GetComponent(worldMode, ID, LayoutComponent);
    LayoutContainer container = {};
    container.container = GetComponent(worldMode, ID, ContainerMappingComponent);
    container.drawMode = drawMode;
    
    ObjectTransform transform = FlatTransform(1.0f);
    transform.angle = layout->rootAngle;
    transform.scale = layout->rootScale;
    transform.modulationPercentage = GetModulationPercentageAndResetFocus(base); 
    RenderLayoutInRect(worldMode, group, rect, transform, layout, base->seed, {}, &container);
}

internal void RenderUIOverlay(GameModeWorld* worldMode, RenderGroup* group)
{
    SetOrthographicTransformScreenDim(group);
    
    Vec2 mouseP = worldMode->absoluteMouseP - 0.5f * group->screenDim;
    if(worldMode->lootingMode)
    {
    }
    else if(worldMode->inventoryMode)
    {
        Vec2 rightP = V2(400, 0);
        Vec2 leftP = V2(-400, 0);
        Vec2 defaultDim = V2(400, 400);
        if(IsValid(worldMode->openIDRight))
        {
            ContainerMappingComponent* container = GetComponent(worldMode, worldMode->openIDRight, ContainerMappingComponent);
            Vec2 dim = container ? container->desiredOpenedDim : defaultDim;
            Rect2 rect = RectCenterDim(rightP, dim);
            OverdrawLayout(worldMode, group, worldMode->openIDRight, rect, LayoutContainerDraw_Open);
        }
        
        if(IsValid(worldMode->openIDLeft))
        {
            ContainerMappingComponent* container = GetComponent(worldMode, worldMode->openIDLeft, ContainerMappingComponent);
            Vec2 dim = container ? container->desiredOpenedDim : defaultDim;
            Rect2 rect = RectCenterDim(leftP, dim);
            OverdrawLayout(worldMode, group, worldMode->openIDLeft, rect, 
                           LayoutContainerDraw_Open);
        }
    }
    
    Vec2 minP = -0.5f * group->screenDim;
    Vec2 dim = V2(100, 100);
    r32 margin = 20.0f;
    
    UsingMappingComponent* usingMappings = GetComponent(worldMode, worldMode->player.clientID, UsingMappingComponent);
    if(usingMappings)
    {
        for(u32 slotIndex = 0; slotIndex < ArrayCount(usingMappings->mappings); ++slotIndex)
        {
            ObjectMapping* mapping = usingMappings->mappings + slotIndex;
            Rect2 rect = RectMinDim(minP, dim);
            PushRectOutline(group, FlatTransform(2.0f), rect, V4(0, 0, 0, 1), 2.0f);
            if(IsValid(mapping->ID))
            {
                OverdrawLayout(worldMode, group, mapping->ID, rect, LayoutContainerDraw_Standard);
            }
            
            minP.x += dim.x + margin;
        }
    }
    
    minP = -0.5f * group->screenDim + V2(0, 2.0f * dim.y);
    EquipmentMappingComponent* equipment = GetComponent(worldMode, worldMode->player.clientID, EquipmentMappingComponent);
    if(equipment)
    {
        for(u32 slotIndex = 0; slotIndex < ArrayCount(equipment->mappings); ++slotIndex)
        {
            ObjectMapping* mapping = equipment->mappings + slotIndex;
            Rect2 rect = RectMinDim(minP, dim);
            PushRectOutline(group, FlatTransform(2.0f), rect, V4(0, 0, 0, 1), 2.0f);
            
            if(IsValid(mapping->ID))
            {
                OverdrawLayout(worldMode, group, mapping->ID, rect, LayoutContainerDraw_Standard);
            }
            
            minP.x += dim.x + margin;
        }
    }
    
    FontId fontID = QueryFonts(group->assets, "game", 0, 0);
    if(IsValid(fontID))
    {
        Vec3 tooltipP = V3(mouseP, 0);
        Vec4 color = worldMode->multipleActions ? V4(1, 0, 0, 1) : V4(1, 1, 1, 1);
        PushText(group, fontID, worldMode->tooltip, tooltipP, 0.7f, color);
    }
}

INTERACTION_ECS_JOB_CLIENT(HandleEntityInteraction)
{
    if(!AreEqual(ID, worldMode->player.clientID))
    {
        BaseComponent* component = GetComponent(worldMode, ID, BaseComponent);
		InteractionComponent* interaction = GetComponent(worldMode, ID, InteractionComponent);
        
        r32 cameraZ;
        Rect3 bound = GetEntityBound(worldMode, component);
        Rect2 screenBounds = ProjectOnScreen(group, bound, &cameraZ);
        if(ShouldBeRendered(worldMode, component) && PointInRect(screenBounds, worldMode->absoluteMouseP))
        {
            AddPossibleInteraction(worldMode, Interaction_Ground, interaction->actions + Interaction_Ground, ID);
        }
    }
}

internal void UpdateEntities(GameModeWorld* worldMode, r32 timeToAdvance, ClientPlayer* myPlayer)
{
    
}

internal b32 UpdateAndRenderGame(GameState* gameState, GameModeWorld* worldMode, RenderGroup* group, PlatformInput* input)
{
    b32 result = false;
    
    ClientPlayer* myPlayer = &worldMode->player;
    ReceiveNetworkPackets(gameState, worldMode);
    
    
    worldMode->originalTimeToAdvance = input->timeToAdvance;
    if(Pressed(&input->pauseButton))
    {
        worldMode->gamePaused = !worldMode->gamePaused;
        SendOrderedMessage(PauseToggle);
    }
    if(worldMode->gamePaused)
    {
        input->timeToAdvance = 0;
    }
    
    worldMode->totalRunningTime += input->timeToAdvance;
    
#if FORGIVENESS_INTERNAL
    if(Pressed(&input->debugButton1))
    {
        SendInputRecordingMessage(true, true);
    }
    
    if(Pressed(&input->debugButton2))
    {
        SendInputRecordingMessage(false, false);
    }
#endif
    
    Vec2 screenMouseP = V2(input->mouseX, input->mouseY);
    Vec3 unprojectedWorldMouseP = UnprojectAtZ(group, &group->gameCamera, screenMouseP, 0);
    Vec3 groundMouseP = ProjectOnGround(unprojectedWorldMouseP, group->gameCamera.P);
    
    worldMode->deltaMouseP = screenMouseP - worldMode->absoluteMouseP;
    worldMode->absoluteMouseP = screenMouseP;
    
    
    if(IsValid(myPlayer->clientID))
    {
        BaseComponent* player = GetComponent(worldMode, myPlayer->clientID, BaseComponent);
        if(player)
        {
            worldMode->editorUI.playerP = player->universeP;
            group->assets = gameState->assets;
#if 0            
            if(!gameState->music)
            {
                RandomSequence seq = Seed(1111);
                AssetLabels soundLabels = {};
                SoundId testSound = QueryAssets(group->assets, AssetType_Sound, AssetSound_exploration, &seq, &soundLabels);
                
                gameState->music = PlaySound(&gameState->soundState, group->assets, testSound, 0);
                
            }
#endif
            Clear(group, V4(0.1f, 0.1f, 0.1f, 1.0f));
            ResetLightGrid(worldMode);
            
            SetupGameCamera(worldMode, group, input);
            UpdateEntities(worldMode, input->timeToAdvance, myPlayer);
            
            worldMode->hotCount = 0;
            worldMode->tooltip[0] = 0;
            
            Vec2 cameraOffset = HandleKeyboardInteraction(worldMode, myPlayer, input);
            EXECUTE_INTERACTION_JOB(worldMode, group, input, HandleEntityInteraction, ArchetypeHas(BaseComponent) && ArchetypeHas(InteractionComponent), input->timeToAdvance);
            MoveCameraTowards(worldMode, player, 0.5f, cameraOffset, V2(0, 0), 1.0f);
			if(worldMode->lootingMode)
			{
                Assert(IsValid(worldMode->lootingID));
                ContainerMappingComponent* container = GetComponent(worldMode, worldMode->lootingID, ContainerMappingComponent);
                BaseComponent* lootingBase = GetComponent(worldMode, worldMode->lootingID, BaseComponent);
                
                if(container && lootingBase)
                {
                    AddContainerObjectsInteractions(worldMode, container->storedMappings, ArrayCount(container->storedMappings), Interaction_Container);
                    MoveCameraTowards(worldMode, lootingBase, 5.0f, V2(0, 0), V2(0, 0), container->zoomCoeff);
                }
			}
            else if(worldMode->inventoryMode)
            {
                HandleEquipmentInteraction(worldMode, myPlayer->clientID);
                HandleContainerInteraction(worldMode);
                MoveCameraTowards(worldMode, player, 2.0f, V2(0, 0), V2(0, 0), 2.0f);
            }
            HandleUsingSlotInteraction(worldMode);
            
            
            if(worldMode->hotCount > 0)
            {
                b32 interactionChanged = true;
                for(u32 hotIndex = 0; hotIndex < worldMode->hotCount; ++hotIndex)
                {
                    EntityHotInteraction hotInteraction = worldMode->hotInteractions[hotIndex];
                    
                    if(AreEqual(hotInteraction, worldMode->lastFrameHotInteraction))
                    {
                        worldMode->currentHotIndex = (i32) hotIndex;
                        interactionChanged = false;
                        break;
                    }
                }
                if(interactionChanged)
                {
                    worldMode->currentActionIndex = 0;
                }
                
                
                if(Pressed(&input->switchButton))
                {
                    ++worldMode->currentHotIndex;
                }
                
                worldMode->currentHotIndex = Wrap(0, worldMode->currentHotIndex, (i32) worldMode->hotCount);
                
                EntityHotInteraction hotInteraction = worldMode->hotInteractions[worldMode->currentHotIndex];
                
                worldMode->currentActionIndex += input->mouseWheelOffset;
                worldMode->currentActionIndex = Wrap(0, worldMode->currentActionIndex, (i32) hotInteraction.actionCount);
                
                
                worldMode->lastFrameHotInteraction = hotInteraction;
                
                EntityID hotID = hotInteraction.ID;
                u16 hotAction = hotInteraction.actions[worldMode->currentActionIndex];
                
                worldMode->multipleActions = (hotInteraction.actionCount > 1);
                FormatString(worldMode->tooltip, sizeof(worldMode->tooltip), "%s", MetaTable_action[hotAction]);
                
                BaseComponent* base = GetComponent(worldMode, hotID, BaseComponent);
                if(base)
                {
                    base->isOnFocus = true;
                }
                switch(hotInteraction.type)
                {
                    case Interaction_Ground:
                    case Interaction_Container:
                    case Interaction_Equipped:
                    {
                        if(Pressed(&input->mouseLeft))
                        {
                            GameCommand* command = &myPlayer->currentCommand;
                            command->targetID = base->serverID;
                            command->action = hotAction;
                        }
                    } break;
                    
                    case Interaction_Equipment:
                    {
                        if(Pressed(&input->mouseLeft))
                        {
                            if(AreEqual(hotID, worldMode->openIDLeft))
                            {
                                worldMode->openIDLeft = {};
                            }
                            else if(AreEqual(hotID, worldMode->openIDRight))
                            {
                                worldMode->openIDRight = {};
                            }
                            else
                            {
                                if(!IsValid(worldMode->openIDLeft))
                                {
                                    worldMode->openIDLeft = hotID;
                                }
                                else if(!IsValid(worldMode->openIDRight))
                                {
                                    worldMode->openIDRight = hotID;
                                }
                            }
                        }
                        
                        if(Pressed(&input->mouseRight))
                        {
                            GameCommand command = {};
                            command.action = hotAction;
                            command.targetID = base->serverID;
                            SendInventoryCommand(command);
                            
                            if(AreEqual(hotID, worldMode->openIDLeft))
                            {
                                worldMode->openIDLeft = {};
                            }
                            else if(AreEqual(hotID, worldMode->openIDRight))
                            {
                                worldMode->openIDRight = {};
                            }
                        }
                    } break;
                    
                    InvalidDefaultCase;
                }
            }
            
            UpdateGameCamera(worldMode, input);
            BeginDepthPeel(group);
            
            Vec3 ambientLightColor = HandleDaynightCycle(worldMode, input);
            ambientLightColor = V3(1, 1, 1);
            PushAmbientLighting(group, ambientLightColor);
            //AddLightToGridCurrentFrame(worldMode, V3(0, 0, 0), V3(0, 0, 1), 4);
            //AddLightToGridCurrentFrame(worldMode, V3(1, 0, 0), V3(1, 0, 0), 8);
            FinalizeLightGrid(worldMode, group);
            
            
            EXECUTE_RENDERING_JOB(worldMode, group, RenderCharacterAnimation, ArchetypeHas(BaseComponent) && ArchetypeHas(AnimationComponent), input->timeToAdvance);
            
            EXECUTE_RENDERING_JOB(worldMode, group, RenderSpriteEntities, ArchetypeHas(BaseComponent) && ArchetypeHas(StandardImageComponent) && !ArchetypeHas(PlantComponent), input->timeToAdvance);
            
            EXECUTE_RENDERING_JOB(worldMode, group, RenderPlants, ArchetypeHas(BaseComponent) && ArchetypeHas(StandardImageComponent) && ArchetypeHas(PlantComponent), input->timeToAdvance);
            
            EXECUTE_RENDERING_JOB(worldMode, group, RenderLayoutEntities, ArchetypeHas(BaseComponent) && ArchetypeHas(LayoutComponent), input->timeToAdvance);
            
            
            if(worldMode->editorUI.renderEntityBounds)
            {
                EXECUTE_RENDERING_JOB(worldMode, group, RenderBound, ArchetypeHas(BaseComponent), input->timeToAdvance);
            }
            
            myPlayer->universeP = player->universeP;
            Vec3 deltaP = -SubtractOnSameZChunk(myPlayer->universeP, myPlayer->oldUniverseP);
            
            
            for(u32 voronoiIndex = 0; voronoiIndex < ArrayCount(worldMode->voronoiPingPong); ++voronoiIndex)
            {
                worldMode->voronoiPingPong[voronoiIndex].deltaP += deltaP;
            }
            
            PreloadAllGroundBitmaps(group->assets);
            UpdateAndRenderGround(worldMode, group, myPlayer->universeP, myPlayer->oldUniverseP, input->timeToAdvance);
            
            worldMode->particleCache->deltaParticleP = deltaP;
            UpdateAndRenderParticleEffects(worldMode, worldMode->particleCache, input->timeToAdvance, group);
            //worldMode->boltCache->deltaP = deltaP;
            //UpdateAndRenderBolts(worldMode, worldMode->boltCache, input->timeToAdvance, group);
            
            RenderUIOverlay(worldMode, group);
            RenderEditor(worldMode, group, input);
            EndDepthPeel(group);
            
            myPlayer->oldUniverseP = myPlayer->universeP;
            
#if 0
            Rect3 worldCameraBounds = GetScreenBoundsAtTargetDistance(group);
            Rect2 screenBounds = RectCenterDim(V2(0, 0), V2(worldCameraBounds.max.x - worldCameraBounds.min.x, worldCameraBounds.max.y - worldCameraBounds.min.y));
            PushRectOutline(group, FlatTransform(), screenBounds, V4(1.0f, 0.0f, 0.0f, 1.0f), 0.1f); 
#endif
        }
    }
    
    if(!AreEqual(myPlayer->lastCommand, myPlayer->currentCommand))
    {
        myPlayer->lastCommand = myPlayer->currentCommand;
        ++myPlayer->currentCommandIndex;
    }
    SendCommand(myPlayer->currentCommandIndex, myPlayer->currentCommand);
    
    
    return result;
}


internal b32 UpdateAndRenderLauncherScreen(GameState* gameState, RenderGroup* group, PlatformInput* input)
{
    
#if 0    
    GameRenderCommands* commands = group->commands;
    Vec2 relativeScreenMouse = V2(input->relativeMouseX, input->relativeMouseY);
    
    Clear( group, V4( 0.25f, 0.25f, 0.25f, 1.0f));
    
    r32 width = (r32) commands->settings.width;
    r32 height = (r32) commands->settings.height;
    
    SetCameraTransform(group, Camera_Orthographic, 0.0f, V3(2.0f / width, 0.0f, 0.0f), V3(0.0f, 2.0f / width, 0.0f), V3( 0, 0, 1));
    
    b32 serverRunning = platformAPI.DEBUGExistsProcessWithName("win32_server.exe");
    
    char* serverText = serverRunning ? "end server" : "start server";
    char* editorText = serverRunning ? "end editor" : "start editor";
    char* joinText = "join local server";
    r32 fontScale = 0.52f;
    Vec2 serverP = V2(300, 100);
    Vec2 editorP = V2(-300, 100);
    Vec2 joinP = V2(0, 100);
    
    TagVector matchVector = {};
    TagVector weightVector = {};
    weightVector.E[Tag_fontType] = 1.0f;
    matchVector.E[Tag_fontType] = ( r32 ) Font_default;
    FontId fontId = GetMatchingFont(group->assets, Asset_font, &matchVector, &weightVector);
    
    Rect2 serverRect = InvertedInfinityRect2();
    Rect2 editorRect = InvertedInfinityRect2();
    Rect2 joinRect = InvertedInfinityRect2();
    
    ObjectTransform rectTransform = FlatTransform();
    if(IsValid(fontId))
    {
        Font* font = PushFont(group, fontId);
        if(font)
        {
            PakFont* fontInfo = GetFontInfo(group->assets, fontId);
            if(fontInfo)
            {
                b32 drawShadow = true;
                serverRect = UIOrthoTextOp(group, font, fontInfo, serverText, fontScale, V3(serverP, 0), TextOp_getSize, V4(1, 1, 1, 1), drawShadow);
                editorRect = UIOrthoTextOp(group, font, fontInfo, editorText, fontScale, V3(editorP, 0), TextOp_getSize, V4(1, 1, 1, 1), drawShadow);
                
                joinRect = UIOrthoTextOp(group, font, fontInfo, joinText, fontScale, V3(joinP, 0), TextOp_getSize, V4(1, 1, 1, 1), drawShadow);
                
                
                UIOrthoTextOp(group, font, fontInfo, serverText, fontScale, V3(serverP, 1.0f), TextOp_draw, V4(1, 1, 1, 1), drawShadow);
                UIOrthoTextOp(group, font, fontInfo, editorText, fontScale, V3(editorP, 1.0f), TextOp_draw, V4(1, 1, 1, 1), drawShadow);
                UIOrthoTextOp(group, font, fontInfo, joinText, fontScale, V3(joinP, 1.0f), TextOp_draw, V4(1, 1, 1, 1), drawShadow);
                
                
                Vec2 startingP = V2(-300, 0);
                for(u32 roleIndex = 0; roleIndex < ArrayCount(MetaFlags_EditorRole); ++roleIndex)
                {
                    MetaFlag* flag = MetaFlags_EditorRole + roleIndex;
                    Rect2 roleRect = UIOrthoTextOp(group, font, fontInfo, flag->name, fontScale, V3(startingP, 0), TextOp_getSize, V4(1, 1, 1, 1), drawShadow);
                    
                    Vec4 color = V4(1, 0, 0, 0.4f);
                    
                    if(gameState->editorRoles & flag->value)
                    {
                        color.a = 1.0f;
                    }
                    
                    if(PointInRect(roleRect, relativeScreenMouse))
                    {
                        color.b = 1.0f;
                        if(Pressed(&input->mouseLeft))
                        {
                            if(gameState->editorRoles & flag->value)
                            {
                                gameState->editorRoles &= ~flag->value;
                            }
                            else
                            {
                                gameState->editorRoles |= flag->value;
                            }
                        }
                    }
                    
                    PushRect(group, rectTransform, roleRect, color);
                    UIOrthoTextOp(group, font, fontInfo, flag->name, fontScale, V3(startingP, 1.0f), TextOp_draw, V4(1, 1, 1, 1), drawShadow);
                    
                    startingP.y -= 40;
                }
            }
        }
    }
    else
    {
        editorRect = RectMinDim(V2(-300, 100), V2(200, 50));
        serverRect = RectMinDim(V2(0, 100), V2(200, 50));
        joinRect = RectMinDim(V2(300, 100), V2(200, 50));
    }
    
    Vec4 serverColor = V4(1, 0, 0, 0.5f);
    Vec4 editorColor = V4(0, 1, 0, 0.5f);
    Vec4 joinColor = V4(0, 0, 1, 0.5f);
    
    if(PointInRect(serverRect, relativeScreenMouse))
    {
        serverColor.a = 1.0f;
        if(Pressed(&input->mouseLeft))
        {
            if(serverRunning)
            {
                platformAPI.DEBUGKillProcessByName("win32_server.exe");
            }
            else
            {
                platformAPI.DEBUGExecuteSystemCommand("server", input->serverEXE, "");
            }
        }
    }
    else if(PointInRect(editorRect, relativeScreenMouse) && gameState->editorRoles)
    {
        editorColor.a = 1.0f;
        if(Pressed(&input->mouseLeft))
        {
            if(serverRunning)
            {
                platformAPI.DEBUGKillProcessByName("win32_server.exe");
            }
            else
            {
                platformAPI.DEBUGExecuteSystemCommand("editor", input->serverEXE," editor");
            }
        }
    }
    else if(PointInRect(joinRect, relativeScreenMouse))
    {
        if(serverRunning)
        {
            joinColor.a = 1.0f;
            if(Pressed(&input->mouseLeft))
            {
                PlayGame(gameState, input);
            }
        }
    }
    
    PushRect(group, rectTransform, serverRect, serverColor);
    PushRect(group, rectTransform, editorRect, editorColor);
    PushRect(group, rectTransform, joinRect, joinColor);
#endif
    
    return 0;
}

#if FORGIVENESS_INTERNAL
PlatformClientMemory* debugGlobalMemory;
DebugTable* globalDebugTable;
#endif 

//void GameUpdateAndRender(PlatformMemory* memory, PlatformInput* input, GameRenderCommands* commands)
extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    platformAPI = memory->api;
    r32 timeToAdvance = input->timeToAdvance;
#if FORGIVENESS_INTERNAL
    debugGlobalMemory = memory;
    globalDebugTable = memory->debugTable;
#endif 
    
    TIMED_FUNCTION();
    
    GameState* gameState = memory->gameState;
    if(!gameState)
    {
        LoadMetaData();
        
        gameState = BootstrapPushStruct(GameState, totalPool);
        memory->gameState = gameState;
        
        for(u32 taskIndex = 0; 
            taskIndex < ArrayCount(gameState->tasks); 
            taskIndex++)
        {
            TaskWithMemory* task = gameState->tasks + taskIndex;
            task->beingUsed = false;
        }
        
        gameState->renderQueue = memory->highPriorityQueue;
        gameState->slowQueue = memory->lowPriorityQueue;
        gameState->textureQueue = &memory->textureQueue;
        
        gameState->assets = InitAssets(gameState->slowQueue, gameState->tasks, ArrayCount(gameState->tasks), &gameState->assetsPool, gameState->textureQueue, MegaBytes(256));
        
        InitializeSoundState(&gameState->soundState, &gameState->audioPool);
        
        
        gameState->networkInterface.network = input->network;
        clientNetwork = &gameState->networkInterface;
        
        gameState->receiveNetworkPackets.network = &clientNetwork->network;
        gameState->receiveNetworkPackets.ReceiveData = platformAPI.net.ReceiveData;
        
        clientNetwork->nextSendStandardApplicationData = {};
        clientNetwork->nextSendOrderedApplicationData = {};
        
        platformAPI.PushWork(gameState->slowQueue, ReceiveNetworkPackets, &gameState->receiveNetworkPackets);
        
        PlayGame(gameState, input);
    }
    
    
    
    TempMemory renderMemory = BeginTemporaryMemory(&gameState->framePool);
    
    UnlockLockedAssets(gameState->assets);
    RenderGroup group = BeginRenderGroup(gameState->assets, commands);
    
    b32 rerun = false;
    input->allowedToQuit = true;
    
    do
    {
        switch(gameState->mode)
        {
            case GameMode_Launcher:
            {
                rerun = UpdateAndRenderLauncherScreen(gameState, &group, input);
            } break;
            
            case GameMode_TitleScreen:
            {
                rerun = UpdateAndRenderTitleScreen(gameState, gameState->titleScreen, &group, input);
            } break;
            
            case GameMode_Cutscene:
            {
                rerun = UpdateAndRenderCutscene(gameState, gameState->cutscene, &group, input);
            } break;
            
            case GameMode_Playing:
            {
                rerun = UpdateAndRenderGame(gameState, gameState->world, &group, input);
                if(input->allowedToQuit && input->altDown && Pressed(&input->exitButton))
                {
                    if(gameState->music)
                    {
                        ChangeVolume(&gameState->soundState, gameState->music, 1.0f, V2(0.0f, 0.0f));
                        gameState->music = 0;
                    }
                    platformAPI.net.CloseConnection(input->network, 0);
                    SetGameMode(gameState, GameMode_Launcher);
                }
            } break;
            
            InvalidDefaultCase;
        }
        
    }
    while(rerun);
    
    FlushAllQueuedPackets(timeToAdvance);
    
    EndRenderGroup(&group);
    EndTemporaryMemory(renderMemory);
    
    CheckPool(&gameState->framePool);
}

internal void SetGameMode(GameState* gameState, GameMode mode)
{
    b32 completeTask = false;
    for(u32 taskIndex = 0; taskIndex < ArrayCount(gameState->tasks); ++taskIndex)
    {
        if(gameState->tasks[taskIndex].dependOnGameMode)
        {
            completeTask = true;
        }
    }
    
    if(completeTask)
    {
        platformAPI.CompleteQueueWork(gameState->slowQueue);
    }
    
    Clear(&gameState->modePool);
    gameState->mode = mode;
}

//void GameGetSoundOutput(PlatformMemory* memory, PlatformInput* input, PlatformSoundBuffer* soundBuffer)
extern "C" GAME_GET_SOUND_OUTPUT(GameGetSoundOutput)
{
    GameState* gameState = (GameState*) memory->gameState;
    //OutputSineWave(soundBuffer, gameState);
    
    PlayMixedAudio(&gameState->soundState, input->timeToAdvance, soundBuffer, gameState->assets, &gameState->framePool);
}

#if FORGIVENESS_INTERNAL
#include "forg_debug.cpp"
#else
extern "C" GAME_FRAME_END(GameDEBUGFrameEnd)
{
}
#endif

