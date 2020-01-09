#include "forg_client.h"
global_variable ClientNetworkInterface* clientNetwork; 

#include "forg_token.cpp"
#include "forg_pool.cpp"
#include "forg_resizable_array.cpp"
#include "forg_world.cpp"
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
#include "forg_particles.cpp"
//#include "forg_cutscene.cpp"
#include "forg_ground.cpp"
#include "forg_archetypes.cpp"
#include "forg_meta.cpp"
#include "forg_sort.cpp"

internal Rect3 GetEntityBound(GameModeWorld* worldMode, EntityID ID)
{
    BaseComponent* base = GetComponent(worldMode, ID, BaseComponent);
    Rect3 result = Offset(base->bounds, GetRelativeP(worldMode, ID));
    return result;
}

#include "forg_entity_layout.cpp"
#include "forg_game_effect.cpp"
#include "forg_crafting.cpp"
#include "forg_render_entity.cpp"
#include "forg_game_ui.cpp"

RENDERING_ECS_JOB_CLIENT(RenderBound)
{
    BaseComponent* base = GetComponent(worldMode, ID, BaseComponent);
    if(ShouldBeRendered(worldMode, base))
    {
        Rect3 entityBound = GetEntityBound(worldMode, ID);
        PushDebugCubeOutline(group, entityBound, V3(1, 0, 0), 0.02f);
        
        Vec3 entityOrigin = GetRelativeP(worldMode, ID);
        PushRect(group, FlatTransform(V4(0, 1, 0, 1)), entityOrigin, V2(0.05f, 0.05f));
    }
}

#if 0
RENDERING_ECS_JOB_CLIENT(RenderEntityHUD)
{
    BaseComponent* base = GetComponent(worldMode, ID, BaseComponent);
    AliveComponent* alive = GetComponent(worldMode, ID, AliveComponent);
    
    if(ShouldBeRendered(worldMode, base))
    {;
        Vec2 screenP = base->projectedOnScreen.min + 0.5f * V2(GetDim(base->projectedOnScreen).x, 0);
        
        r32 originYOffset = 8;
        r32 barSeparation = 10.0f;
        Vec2 barDim = V2(90.0f, 8.0f);
        
        Vec4 baseC = V4(0, 0, 0, 1);
        Vec4 hC = V4(1, 0, 0, 1);
        Vec4 mC = V4(0, 0, 1, 1);
        
        Vec2 hMin = screenP - V2(0.5f * barDim.x, originYOffset + 0.5f * barDim.y);
        r32 hRatio = (r32) alive->physicalHealth / (r32)alive->maxPhysicalHealth;
        r32 hWidth = hRatio * barDim.x;
        
        PushRect(group, FlatTransform(baseC), RectMinDim(hMin, barDim));
        PushRect(group, FlatTransform(hC), RectMinDim(hMin, V2(hWidth, barDim.y)));
        
        
        Vec2 mMin = hMin - V2(0, barSeparation + 0.5f * barDim.y);
        r32 mRatio = (r32) alive->mentalHealth / (r32)alive->maxMentalHealth;
        r32 mWidth = mRatio * barDim.x;
        
        PushRect(group, FlatTransform(baseC), RectMinDim(mMin, barDim));
        PushRect(group, FlatTransform(mC), RectMinDim(mMin, V2(mWidth, barDim.y)));
        
        
    }
}
#endif

internal void DeleteEntityClient(GameModeWorld* worldMode, EntityID clientID, EntityID serverID)
{
    if(HasComponent(clientID, AnimationEffectComponent))
    {
        AnimationEffectComponent* animation = GetComponent(worldMode, clientID, AnimationEffectComponent);
        for(u32 effectIndex = 0; effectIndex < animation->effectCount; ++effectIndex)
        {
            AnimationEffect* effect = animation->effects + effectIndex;
            FreeAnimationEffect(effect);
        }
        animation->effectCount = 0;
    }
    
    FreeArchetype(worldMode, clientID);
    RemoveClientIDMapping(worldMode, serverID);
}

internal void MarkForDeletion(GameModeWorld* worldMode, EntityID clientID)
{
    BaseComponent* base = GetComponent(worldMode, clientID, BaseComponent);
    if(base && base->deletedTime == 0)
    {
        ActionComponent* action = GetComponent(worldMode, clientID, ActionComponent);
        if(action)
        {
            action->action = idle;
        }
        base->velocity = {};
        base->deletedTime = base->totalLifeTime;
        
        if(AreEqual(base->serverID, worldMode->gameUI.draggingIDServer))
        {
            worldMode->gameUI.draggingIDServer = {};
        }
    }
}

STANDARD_ECS_JOB_CLIENT(PushEntityLight)
{
    LightComponent* light = GetComponent(worldMode, ID, LightComponent);
    if(light->lightRadious)
    {
        BaseComponent* base = GetComponent(worldMode, ID, BaseComponent);
        Vec3 P = GetRelativeP(worldMode, ID);
        AddLight(worldMode, P, light->lightColor, light->lightRadious);
    }
}
STANDARD_ECS_JOB_CLIENT(UpdateEntity)
{
    BaseComponent* base = GetComponent(worldMode, ID, BaseComponent);
    base->timeSinceLastUpdate += elapsedTime;
	base->totalLifeTime += elapsedTime * base->lifeTimeSpeed;
    
    if(base->occludePlayerVisual)
    {
        BaseComponent* player = GetComponent(worldMode, worldMode->player.clientID, BaseComponent);
        
        Vec3 deltaP = SubtractOnSameZChunk(player->universeP, base->universeP);
        
        if(deltaP.y > 0 && RectOverlaps(Scale(base->projectedOnScreen, base->occludeBoundsScale), player->projectedOnScreen))
        {
            base->flags = AddFlags(base->flags, EntityFlag_occluding);
        }
    }
    
    if(base->flags & EntityFlag_occluding)
    {
        r32 occludeTime = 1.0f;
        AnimationEffectComponent* animation = GetComponent(worldMode, ID, AnimationEffectComponent);
        if(animation)
        {
            occludeTime = animation->occludeDissolveTime;
        }
        base->occludingTime += elapsedTime;
        base->occludingTime = Min(base->occludingTime, occludeTime);
    }
    else
    {
        base->occludingTime -= elapsedTime;
        base->occludingTime = Max(base->occludingTime, 0);
    }
    
    
    if(base->timeSinceLastUpdate >= 2.0f * STATIC_UPDATE_TIME && !(base->flags & EntityFlag_notInWorld))
    {
		MarkForDeletion(worldMode, ID);
    }
    
	if(base->deletedTime)
	{
		if(base->totalLifeTime >= (base->deletedTime + base->fadeOutTime))
		{
			DeleteEntityClient(worldMode, ID, base->serverID);
		}
	}
    
    Assert(PositionInsideWorld(&base->universeP));
    UniversePos oldP = base->universeP;
    base->universeP.chunkOffset += base->velocity * elapsedTime;
    base->universeP = NormalizePosition(base->universeP);
    
    if(!PositionInsideWorld(&base->universeP))
    {
        base->universeP = oldP;
    }
    
    Assert(PositionInsideWorld(&base->universeP));
    
    
    //ResetProjectedOnScreenStuff();
}

internal void PlayGame(GameState* gameState, PlatformInput* input)
{
    Clear(&gameState->persistentPool);
    Clear(&gameState->visualEffectsPool);
    
    GameModeWorld* result = PushStruct(&gameState->modePool, GameModeWorld);
    
    result->gameState = gameState;
#if FORGIVENESS_MULTIPLAYER
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
    
    BoltDefinition* bolt = &result->boltDefinition;
    bolt->animationTick = 0.01f;
    bolt->fadeinTime = 0.05f;
    bolt->fadeoutTime = 0.05f;
    bolt->ttl = 0.2f;
    bolt->ttlV = 0;
    bolt->color = V4(1, 1, 1, 1);
    bolt->thickness = 0.05f;
    bolt->thicknessV = 0;
    bolt->magnitudoStructure = 1.0f;
    bolt->magnitudoAnimation = 1.0f;
    bolt->subdivisions = 3;
    bolt->subdivisionsV = 0;
    bolt->lightColor = V3(1, 1, 1);
    bolt->lightIntensity = 4.0f;
    bolt->lightStartTime = R32_MAX;
    bolt->lightEndTime = 0;
    
    
    gameState->world = result;
    
    result->worldTileView = false;
    result->defaultCameraZ = 34.0f;
    result->cameraWorldOffset = V3(0.0f, 0.0f, result->defaultCameraZ);
    
    result->gameUI.input = input;
    
    result->editorUI.input = input;
    result->editorUI.pool = &gameState->assetsPool;
    result->editorUI.assets = gameState->assets;
    result->editorUI.soundState = &gameState->soundState;
    
    result->editorUI.firstFreeCommand = 0;
    DLLIST_INIT(&result->editorUI.undoRedoSentinel);
    result->editorUI.currentCommand = &result->editorUI.undoRedoSentinel;
    
    for(u16 archetypeIndex = 0; archetypeIndex < Archetype_Count; ++archetypeIndex)
    {
        InitArchetype(result, archetypeIndex, 16);
    }
    
    result->defaultZoomCoeff = 1.2f;
    result->defaultZoomSpeed = 2.0f;
    result->equipmentZoomCoeff = 2.0f;
    result->equipmentZoomSpeed = 2.0f;
    
    result->ambientLightColor = V3(1, 1, 1);
    result->windDirection = V3(1, 0, 0);
    result->windStrength = 1.0f;
}

internal Vec3 GetAmbientColor(u16 dayTime)
{
    Vec3 result = {};
    switch(dayTime)
    {
        case DayTime_Day:
        {
            result = V3(1, 1, 1);
        } break;
        
        case DayTime_Night:
        {
            result = V3(0, 0, 0.2f);
        } break;
        
        InvalidDefaultCase;
    }
    
    return result;
}

internal void UpdateAmbientParameters(GameModeWorld* worldMode, r32 elapsedTime)
{
    worldMode->dayTimeTime += elapsedTime;
    
    Vec3 oldColor = GetAmbientColor(worldMode->previousDayTime);
    Vec3 color = GetAmbientColor(worldMode->dayTime);
    
    r32 fullColorTime = 2.0f;
    r32 lerp = Clamp01MapToRange(0.0f, worldMode->dayTimeTime, fullColorTime);
    
    worldMode->ambientLightColor = Lerp(oldColor, lerp, color);
}

internal void UpdateAndRenderGame(GameState* gameState, GameModeWorld* worldMode, RenderGroup* group, PlatformInput* input)
{
    worldMode->originalTimeToAdvance = input->timeToAdvance;
    if(worldMode->gamePaused)
    {
        input->timeToAdvance = 0;
    }
    ClientPlayer* myPlayer = &worldMode->player;
    ReceiveNetworkPackets(gameState, worldMode);
    
    worldMode->windTime += input->timeToAdvance * worldMode->windStrength;
    worldMode->stateTime += input->timeToAdvance;
    
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
    
    Vec2 screenMouseP = V2(input->mouseX, input->mouseY) - 0.5f * group->screenDim;
    worldMode->deltaMouseP = screenMouseP - worldMode->relativeMouseP;
    worldMode->relativeMouseP = screenMouseP;
    
    BEGIN_BLOCK("update entities");
    EXECUTE_JOB(worldMode, UpdateEntity, ArchetypeHas(BaseComponent), input->timeToAdvance);
    EXECUTE_JOB(worldMode, UpdateEntityEffects, ArchetypeHas(AnimationEffectComponent), input->timeToAdvance);
    EXECUTE_JOB(worldMode, UpdateEntitySoundEffects, ArchetypeHas(SoundEffectComponent), input->timeToAdvance);
    END_BLOCK();
    
    b32 renderWorld = false;
    BaseComponent* player_ = GetComponent(worldMode, myPlayer->clientID, BaseComponent);
    if(player_)
    {
        myPlayer->universeP = player_->universeP;
        BEGIN_BLOCK("ground generation");
        if(CountGroundBitmaps(group->assets) > 0)
        {
            PreloadAllGroundBitmaps(group->assets);
            UpdateGround(worldMode, group, myPlayer->universeP);
        }
        
        if(GroundIsComplete(worldMode, myPlayer->universeP))
        {
            renderWorld = true;
        }
    }
    
    Vec3 deltaP = -SubtractOnSameZChunk(myPlayer->universeP, myPlayer->oldUniverseP);
    worldMode->editorUI.playerP = myPlayer->universeP;
    group->assets = gameState->assets;
    Clear(group, V4(0.0f, 0.0f, 0.0f, 1.0f));
    
    END_BLOCK();
    
    UpdateGameCamera(worldMode, input);
    ResetGameCamera(worldMode, group);
    UpdateAmbientParameters(worldMode, input->timeToAdvance);
    
    BEGIN_BLOCK("setup rendering");
    ResetLightGrid(worldMode);
    Vec3 unprojectedWorldMouseP = UnprojectAtZ(group, &group->gameCamera, screenMouseP, 0);
    worldMode->groundMouseP = ProjectOnGround(unprojectedWorldMouseP, group->gameCamera.P);
    
    if(!worldMode->gamePaused)
    {
        HandleGameUIInteraction(worldMode, group, myPlayer, input);
    }
    PushGameRenderSettings(group, worldMode->ambientLightColor, worldMode->windTime, worldMode->windDirection, 1.0f);
    EXECUTE_JOB(worldMode, PushEntityLight, ArchetypeHas(LightComponent), input->timeToAdvance);
    FinalizeLightGrid(worldMode, group);
    END_BLOCK();
    
#if FORGIVENESS_INTERNAL
    if(Pressed(&input->confirmButton))
    {
        if(input->altDown)
        {
            if(!worldMode->gamePaused)
            {
                worldMode->gamePaused = true;
                SendOrderedMessage(PauseToggle);
            }
            
            if(++worldMode->renderMode >= RenderMode_Count)
            {
                worldMode->renderMode = 0;
            }
        }
        
        if(worldMode->testEffect)
        {
            FreeParticleEffect(worldMode->testEffect);
            worldMode->testEffect = 0;
        }
    }
#endif
    
    switch(worldMode->renderMode)
    {
        case RenderMode_World:
        {
            if(Pressed(&input->pauseButton))
            {
                worldMode->gamePaused = !worldMode->gamePaused;
                SendOrderedMessage(PauseToggle);
            }
            
            BEGIN_BLOCK("rendering");
            if(renderWorld)
            {
                RenderGroundAndPlaySounds(worldMode, group, myPlayer->universeP, input->timeToAdvance);
                
#if 0
                EXECUTE_RENDERING_JOB(worldMode, group, RenderShadow, ArchetypeHas(ShadowComponent), input->timeToAdvance);
                
                EXECUTE_RENDERING_JOB(worldMode, group, RenderGrass, ArchetypeHas(GrassComponent) && ArchetypeHas(MagicQuadComponent), input->timeToAdvance);
                
                EXECUTE_RENDERING_JOB(worldMode, group, RenderSpriteEntities, ArchetypeHas(StandardImageComponent), input->timeToAdvance);
                
                EXECUTE_RENDERING_JOB(worldMode, group, RenderSegmentEntities, ArchetypeHas(SegmentImageComponent), input->timeToAdvance);
                
                EXECUTE_RENDERING_JOB(worldMode, group, RenderCharacterAnimation, ArchetypeHas(AnimationComponent), input->timeToAdvance);
                
                EXECUTE_RENDERING_JOB(worldMode, group, RenderFrameByFrameEntities, ArchetypeHas(FrameByFrameAnimationComponent) && !ArchetypeHas(PlantComponent), input->timeToAdvance);
                
                EXECUTE_RENDERING_JOB(worldMode, group, RenderPlant, ArchetypeHas(PlantComponent), input->timeToAdvance);
                
                EXECUTE_RENDERING_JOB(worldMode, group, RenderRock, ArchetypeHas(RockComponent), input->timeToAdvance);
                
                EXECUTE_RENDERING_JOB(worldMode, group, RenderLayoutEntities, ArchetypeHas(LayoutComponent), input->timeToAdvance);
                
                EXECUTE_RENDERING_JOB(worldMode, group, RenderMultipartEntity, ArchetypeHas(MultipartAnimationComponent), input->timeToAdvance);
#else
                EXECUTE_RENDERING_JOB(worldMode, group, RenderEntity, true, input->timeToAdvance);
#endif
                if(worldMode->editorUI.renderEntityBounds)
                {
                    EXECUTE_RENDERING_JOB(worldMode, group, RenderBound, true, input->timeToAdvance);
                }
                END_BLOCK();
                
                
                BEGIN_BLOCK("particles");
                
                worldMode->particleCache->deltaParticleP = deltaP;
                UpdateAndRenderParticleEffects(worldMode, worldMode->particleCache, input->timeToAdvance, group);
                END_BLOCK();
            }
        } break;
        
        case RenderMode_Entity:
        case RenderMode_Particle:
        case RenderMode_ParticleEntity:
        {
            EditorUIContext* context = &worldMode->editorUI;
            
            if(worldMode->renderMode == RenderMode_Entity ||
               worldMode->renderMode == RenderMode_ParticleEntity)
            {
                UniversePos oldP = worldMode->player.universeP;
                worldMode->player.universeP = {};
                EntityType test = GetEntityType(group->assets, context->name);
                u32 seed = context->seed;
                u16* essences = context->essences;
                u16 action = context->action;
                r32 health = context->health;
                
                PushGameRenderSettings(group, V3(1, 1, 1), 0, {}, 0);
                PushRect(group, FlatTransform(V4(0.9f, 0.9f, 0.9f, 1)), V3(0, 0, 0), V2(200, 200));
                
                if(IsValid(test))
                {
                    EntityDefinition* definition = GetEntityTypeDefinition(group->assets, test);
                    if(definition)
                    {
                        EntityID ID = {};
                        u8 archetype = SafeTruncateToU8(ConvertEnumerator(EntityArchetype, definition->archetype));
                        AcquireArchetype(worldMode, archetype, (&ID));
                        ClientEntityInitParams params = definition->client;
                        params.ID = ID;
                        params.seed = seed;
                        
                        definition->common.type = test;
                        definition->common.essences = essences;
                        InitEntity(worldMode, ID, &definition->common, 0, &params);
                        
                        BaseComponent* base = GetComponent(worldMode, ID, BaseComponent);
                        if(base)
                        {
                            base->universeP = Offset(base->universeP, context->entityOffset);
                        }
                        
                        ActionComponent* actionComp = GetComponent(worldMode, ID, ActionComponent);
                        if(actionComp)
                        {
                            actionComp->action = action;
                        }
                        
                        HealthComponent* healthComp = GetComponent(worldMode, ID, HealthComponent);
                        if(healthComp)
                        {
                            healthComp->physicalHealth = health;
                            healthComp->maxPhysicalHealth = health;
                            
                            healthComp->mentalHealth = health;
                            healthComp->maxMentalHealth = health;
                        }
                        
                        RenderEntity(worldMode, group, ID, input->timeToAdvance);
                        
                        
                        if(true)
                        {
                            RenderBound(worldMode, group, ID, input->timeToAdvance);
                        }
                        FreeArchetype(worldMode, ID);
                    }
                }
                
                worldMode->player.universeP = oldP;
            }
            
            if(worldMode->renderMode == RenderMode_Particle ||
               worldMode->renderMode == RenderMode_ParticleEntity)
            {
                PushGameRenderSettings(group, V3(1, 1, 1), 0, {}, 0);
                PushRect(group, FlatTransform(V4(0.9f, 0.9f, 0.9f, 1)), V3(0, 0, 0), V2(200, 200));
                PushRect(group, FlatTransform(V4(0, 1, 0, 1)), V3(0, 0, 0), V2(0.05f, 0.05f));
                
                
                if(!worldMode->testEffect)
                {
                    AssetID effectID = QueryDataFiles(group->assets, ParticleEffect, context->effectName.name, 0, &context->properties);
                    if(IsValid(effectID))
                    {
                        ParticleEffect* particles = GetData(group->assets, ParticleEffect, effectID);
                        
                        worldMode->testEffect = GetNewParticleEffect(worldMode->particleCache, particles);
                    }
                }
                
                if(worldMode->testEffect)
                {
                    SetEffectParameters(worldMode->testEffect, context->particleOffset, context->particleSpeed, context->particleScale);
                    UpdateAndRenderEffect(worldMode, worldMode->particleCache, worldMode->testEffect, worldMode->originalTimeToAdvance, {}, group);
                }
            }
        } break;
    }
    
    BEGIN_BLOCK("editor and UI overlay");
    SetOrthographicTransformScreenDim(group);
    //FixedOrderedRendering(group);
    
    if(!worldMode->gamePaused)
    {
        RenderUIOverlay(worldMode, group, input->timeToAdvance);
    }
    //EXECUTE_RENDERING_JOB(worldMode, group, RenderEntityHUD, ArchetypeHas(AliveComponent), input->timeToAdvance);
    if(worldMode->editingEnabled)
    {
        RenderEditorOverlay(worldMode, group, input);
    }
    END_BLOCK();
    
    myPlayer->oldUniverseP = myPlayer->universeP;
    GameCommand command = ComputeFinalCommand(&worldMode->gameUI, worldMode, myPlayer);
    if(!AreEqual(myPlayer->lastCommand, command))
    {
        myPlayer->lastCommand = command;
        ++myPlayer->currentCommandIndex;
    }
    
    SendCommand(myPlayer->currentCommandIndex, command);
    SendCommandParameters(worldMode->gameUI.commandParameters);
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
        SetMetaAssets(gameState->assets);
        
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
    UpdateAndRenderGame(gameState, gameState->world, &group, input);
    
    FlushAllQueuedPackets(timeToAdvance);
    
    EndRenderGroup(&group);
    EndTemporaryMemory(renderMemory);
    
    CheckPool(&gameState->framePool);
}

#if 0
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
#endif

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

