
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
#include "forg_bolt.cpp"
#include "forg_cutscene.cpp"
#include "forg_ground.cpp"
#include "forg_UIcommon.cpp"
#include "forg_archetypes.cpp"
#include "forg_meta.cpp"
#include "forg_sort.cpp"
#include "forg_game_effect.cpp"

internal Rect3 GetEntityBound(GameModeWorld* worldMode, BaseComponent* base)
{
    Rect3 result = Offset(base->bounds, GetRelativeP(worldMode, base));
    return result;
}
#include "forg_render_entity.cpp"
#include "forg_game_ui.cpp"

RENDERING_ECS_JOB_CLIENT(RenderBound)
{
    BaseComponent* base = GetComponent(worldMode, ID, BaseComponent);
    if(ShouldBeRendered(worldMode, base))
    {
        Rect3 entityBound = GetEntityBound(worldMode, base);
        PushCubeOutline(group, entityBound, V4(1, 0, 0, 1), 0.05f);
    }
}

internal void DeleteEntityClient(GameModeWorld* worldMode, EntityID clientID, EntityID serverID)
{
    FreeArchetype(worldMode, clientID);
    RemoveClientIDMapping(worldMode, serverID);
    if(AreEqual(serverID, worldMode->gameUI.draggingIDServer))
    {
        worldMode->gameUI.draggingIDServer = {};
    }
}


STANDARD_ECS_JOB_CLIENT(PushEntityLight)
{
    AnimationEffectComponent* animation = GetComponent(worldMode, ID, AnimationEffectComponent);
    if(animation->lightIntensity)
    {
        BaseComponent* base = GetComponent(worldMode, ID, BaseComponent);
        Vec3 P = GetRelativeP(worldMode, base);
        AddLightToGridCurrentFrame(worldMode, P, animation->lightColor, animation->lightIntensity);
    }
}
STANDARD_ECS_JOB_CLIENT(UpdateEntity)
{
    BaseComponent* base = GetComponent(worldMode, ID, BaseComponent);
    if(base->timeSinceLastUpdate >= 2.0f)
    {
        DeleteEntityClient(worldMode, ID, base->serverID);
    }
}

internal void PlayGame(GameState* gameState, PlatformInput* input)
{
    SetGameMode(gameState, GameMode_Playing);
    
    Clear(&gameState->persistentPool);
    Clear(&gameState->visualEffectsPool);
    
    GameModeWorld* result = PushStruct(&gameState->modePool, GameModeWorld);
    
    result->gameState = gameState;
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
    
    result->chunkApron = 2;
    result->worldTileView = false;
    result->defaultCameraZ = 34.0f;
    result->cameraWorldOffset = V3(0.0f, 0.0f, result->defaultCameraZ);
    
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
    
    
    result->ambientLightColor = V3(0, 0, 0.2f);
    result->windDirection = V3(1, 0, 0);
    result->windStrength = 1.0f;
    
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
    
    worldMode->windTime += input->timeToAdvance * worldMode->windStrength;
    
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
    
    
    if(IsValidID(myPlayer->clientID))
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
            
            Vec3 unprojectedWorldMouseP = UnprojectAtZ(group, &group->gameCamera, screenMouseP, 0);
            worldMode->groundMouseP = ProjectOnGround(unprojectedWorldMouseP, group->gameCamera.P);
            worldMode->groundMouseP = unprojectedWorldMouseP;
            
            HandleUIInteraction(worldMode, group, myPlayer, input);
            UpdateGameCamera(worldMode, input);
            
            
            BeginDepthPeel(group);
            
            PushGameRenderSettings(group, worldMode->ambientLightColor, worldMode->windTime, worldMode->windDirection, 1.0f);
            
            EXECUTE_JOB(worldMode, PushEntityLight, ArchetypeHas(AnimationEffectComponent), input->timeToAdvance);
            FinalizeLightGrid(worldMode, group);
            
            
            EXECUTE_JOB(worldMode, UpdateEntity, ArchetypeHas(BaseComponent), input->timeToAdvance);
            EXECUTE_JOB(worldMode, UpdateEntityEffects, ArchetypeHas(AnimationEffectComponent), input->timeToAdvance);
            
            EXECUTE_RENDERING_JOB(worldMode, group, RenderGrass, ArchetypeHas(GrassComponent) && ArchetypeHas(BaseComponent) && ArchetypeHas(MagicQuadComponent), input->timeToAdvance);
            
            EXECUTE_RENDERING_JOB(worldMode, group, RenderSpriteEntities, ArchetypeHas(BaseComponent) && ArchetypeHas(StandardImageComponent) && !ArchetypeHas(PlantComponent), input->timeToAdvance);
            
            EXECUTE_RENDERING_JOB(worldMode, group, RenderCharacterAnimation, ArchetypeHas(BaseComponent) && ArchetypeHas(AnimationComponent), input->timeToAdvance);
            
            EXECUTE_RENDERING_JOB(worldMode, group, RenderFrameByFrameEntities, ArchetypeHas(BaseComponent) && ArchetypeHas(FrameByFrameAnimationComponent) && !ArchetypeHas(PlantComponent), input->timeToAdvance);
            
            EXECUTE_RENDERING_JOB(worldMode, group, RenderPlants, ArchetypeHas(BaseComponent) && ArchetypeHas(StandardImageComponent) && ArchetypeHas(PlantComponent), input->timeToAdvance);
            
            EXECUTE_RENDERING_JOB(worldMode, group, RenderLayoutEntities, ArchetypeHas(BaseComponent) && ArchetypeHas(LayoutComponent), input->timeToAdvance);
            
            EXECUTE_RENDERING_JOB(worldMode, group, UpdateAndRenderBolt, ArchetypeHas(BaseComponent) && ArchetypeHas(BoltComponent), input->timeToAdvance);
            
            if(worldMode->editorUI.renderEntityBounds)
            {
                EXECUTE_RENDERING_JOB(worldMode, group, RenderBound, ArchetypeHas(BaseComponent), input->timeToAdvance);
            }
            
            myPlayer->universeP = player->universeP;
            Vec3 deltaP = -SubtractOnSameZChunk(myPlayer->universeP, myPlayer->oldUniverseP);
            
            PreloadAllGroundBitmaps(group->assets);
            UpdateAndRenderGround(worldMode, group, myPlayer->universeP, myPlayer->oldUniverseP, input->timeToAdvance);
            
            
            
            for(u16 weatherIndex = 0; weatherIndex < Weather_count; ++weatherIndex)
            {
                GameProperties weatherProperties = {};
                if(!worldMode->weatherEffects[weatherIndex])
                {
                    AddGameProperty(&weatherProperties, weather, weatherIndex);
                    AssetID effectID = QueryDataFiles(group->assets, ParticleEffect, "weather", 0, &weatherProperties);
                    if(IsValid(effectID))
                    {
                        ParticleEffect* particles = GetData(group->assets, ParticleEffect, effectID);
                        
                        Vec3 startingP = V3(0, 0, 0);
                        Vec3 UpVector = V3(0, 0, 1);
                        
                        worldMode->weatherEffects[weatherIndex] = GetNewParticleEffect(worldMode->particleCache, particles, startingP, UpVector);
                    }
                }
            }
            
            worldMode->particleCache->deltaParticleP = deltaP;
            UpdateAndRenderParticleEffects(worldMode, worldMode->particleCache, input->timeToAdvance, group);
            
            
            
            
            
            RenderUIOverlay(worldMode, group);
            RenderEditorOverlay(worldMode, group, input);
            EndDepthPeel(group);
            
            
            myPlayer->oldUniverseP = myPlayer->universeP;
        }
    }
    
    GameCommand command = ComputeFinalCommand(&worldMode->gameUI, worldMode, myPlayer);
    if(!AreEqual(myPlayer->lastCommand, command))
    {
        myPlayer->lastCommand = command;
        ++myPlayer->currentCommandIndex;
    }
    
    SendCommand(myPlayer->currentCommandIndex, command);
    SendCommandParameters(worldMode->gameUI.commandParameters);
    
    
    return result;
}


internal b32 UpdateAndRenderLauncherScreen(GameState* gameState, RenderGroup* group, PlatformInput* input)
{
#if 0
    platformAPI.DEBUGKillProcessByName("win32_server.exe");
    platformAPI.DEBUGExecuteSystemCommand("server", input->serverEXE, "");
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

