
#include "forg_client.h"
global_variable ClientNetworkInterface* clientNetwork; 

#include "forg_token.cpp"
#include "forg_pool.cpp"
#include "forg_resizable_array.cpp"
#include "forg_physics.cpp"
#include "forg_world.cpp"
//#include "forg_world_client.cpp"
#include "forg_asset.cpp"
#include "forg_world_generation.cpp"
#include "forg_render.cpp"
#include "forg_sound.cpp"
#include "forg_editor.cpp"
#include "forg_camera.cpp"
//#include "forg_light.cpp"
#include "miniz.c"
#include "forg_network_client.cpp"
#include "forg_plant.cpp"
#include "forg_mesh.cpp"
#include "forg_rock.cpp"
#include "forg_object.cpp"
//#include "forg_crafting.cpp"
#include "forg_particles.cpp"
//#include "forg_bolt.cpp"
//#include "forg_bound.cpp"
#include "forg_cutscene.cpp"
#include "forg_ground.cpp"
#include "forg_UIcommon.cpp"
#include "forg_animation.cpp"
#include "forg_archetypes.cpp"
#include "forg_meta.cpp"
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
    
    result->firstFreeRock = 0;
    result->firstFreePlantSegment = 0;
    
    
#if 0    
    result->dayPhases[DayPhase_Day].duration = 6000.0f;
    result->dayPhases[DayPhase_Day].ambientLightColor = V3(0.9f, 0.88f, 1.0f);
    result->dayPhases[DayPhase_Day].next = DayPhase_Sunset;
    
    result->dayPhases[DayPhase_Sunset].duration = 30.0f;
    result->dayPhases[DayPhase_Sunset].ambientLightColor = V3(1.0f, 0.8f, 0.8f);
    result->dayPhases[DayPhase_Sunset].next = DayPhase_Night;
    
    result->dayPhases[DayPhase_Night].duration = 40.0f;
    result->dayPhases[DayPhase_Night].ambientLightColor = V3(0.0f, 0.01f, 0.15f);
    result->dayPhases[DayPhase_Night].next = DayPhase_Day;
#endif
    
    //result->currentPhase = DayPhase_Day;
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
    Vec3 ambientLightColor = {};
#if 0                
    worldMode->currentPhaseTimer += input->timeToAdvance;
    DayPhase* currentPhase = worldMode->dayPhases + worldMode->currentPhase;
    DayPhase* nextPhase = worldMode->dayPhases + currentPhase->next;
    
    r32 maxLerpDuration = 1.0f;
    if(worldMode->currentPhaseTimer >= currentPhase->duration)
    {
        worldMode->currentPhaseTimer = 0;
        worldMode->currentPhase = currentPhase->next;
        ambientLightColor = nextPhase->ambientLightColor;
    }
    else if(worldMode->currentPhaseTimer >= (currentPhase->duration - maxLerpDuration))
    {
        r32 lowTimer = currentPhase->duration - maxLerpDuration;
        
        r32 lerp = Clamp01MapToRange(lowTimer, worldMode->currentPhaseTimer, currentPhase->duration);
        ambientLightColor = Lerp(currentPhase->ambientLightColor, lerp, nextPhase->ambientLightColor);
    }
    else
    {
        ambientLightColor = currentPhase->ambientLightColor;
    }
    switch(worldMode->currentPhase)
    {
        case DayPhase_Sunrise:
        {
            ambientLightColor = V3(0.0f, 0.01f, 0.5f);
        } break;
        
        case DayPhase_Morning:
        {
            ambientLightColor = V3(0.5f, 0.7f, 0.9f);
        } break;
        
        case DayPhase_Day:
        {
            ambientLightColor = V3(0.0f, 0.3f, 0.7f);
        } break;
        
        case DayPhase_Sunset:
        {
            ambientLightColor = V3(0.9f, 0.8f, 0.3f);
        } break;
        
        case DayPhase_Dusk:
        {
            ambientLightColor = V3(0.43f, 0.2f, 0.28f);
        } break;
        
        case DayPhase_Night:
        {
            ambientLightColor = V3(0.02f, 0.02f, 0.1f);
        } break;
    }
#endif
    return ambientLightColor;
}

internal void RenderCharacterAnimations(GameModeWorld* worldMode, RenderGroup* group,r32 timeToAdvance)
{
    for(u16 archetypeIndex = 0; archetypeIndex < Archetype_Count; ++archetypeIndex)
    {
        if(HasComponent(archetypeIndex, BaseComponent) && HasComponent(archetypeIndex, AnimationComponent))
        {
            for(ArchIterator iter = First(worldMode, archetypeIndex); 
                IsValid(iter); 
                iter = Next(iter))
            {
                BaseComponent* base = GetComponent(worldMode, iter.ID, BaseComponent);AnimationComponent* animation = GetComponent(worldMode, iter.ID, AnimationComponent);
                
                
                AnimationParams params = {};
                params.elapsedTime = timeToAdvance;
                params.angle = 0;
                params.P = GetRelativeP(worldMode, base);
                params.scale = V2(1, 1);
                RenderCharacterAnimation(group, animation, &params);
            }
        }
    }
}

internal void AddEntityLights(GameModeWorld* worldMode)
{
    
#if 0    
    for(u32 entityIndex = 0; 
        entityIndex < ArrayCount(worldMode->entities); 
        entityIndex++)
    {
        ClientEntity* entity = worldMode->entities[entityIndex];
        while(entity)
        {
            if(entity->identifier)
            {  
                if(!IsSet(entity, Flag_deleted | Flag_Attached))
                {
                    TaxonomySlot* slot = GetSlotForTaxonomy(worldMode->table, entity->taxonomy);
                    if(slot->hasLight)
                    {
                        AddLightToGridCurrentFrame(worldMode, entity->P, slot->lightColor, entity->lightIntensity);
                    }
                }
            }
            
            entity = entity->next;
        }
    }
#endif
    
}

internal u64 DetectNearestEntities(GameModeWorld* worldMode, RenderGroup* group, Vec2 screenMouseP)
{
    
    u64 result = 0;
    
#if 0    
    for(u32 index = 0; index < ArrayCount(worldMode->nearestCameraZ); ++index)
    {
        worldMode->nearestCameraZ[index] = R32_MAX;
        worldMode->nearestEntities[index] = 0;
    }
    r32 maxOverallDistanceSq = R32_MAX;
    r32 overallMinCameraZ = R32_MAX;
    
    
    for(u32 entityIndex = 0; 
        entityIndex < ArrayCount(worldMode->entities); 
        entityIndex++)
    {
        ClientEntity* entity = worldMode->entities[entityIndex];
        while(entity)
        {
            ClientEntity* next = entity->next;
            if(entity->identifier)
            {  
                if(entity->identifier && !IsSet(entity, Flag_deleted | Flag_Attached))
                {
                    r32 cameraZ = 0.0f;
                    
                    Rect2 screenBounds = InvertedInfinityRect2();
                    if(IsRock(worldMode->table, entity->taxonomy) || IsPlant(worldMode->table, entity->taxonomy) ||
                       AnimatedIn3d(worldMode->table, entity->taxonomy))
                    {
                        screenBounds = ProjectOnScreen(group, entity->bounds, &cameraZ);
                    }
                    else
                    {
                        Rect2 entityBounds = entity->animation.bounds;
                        Vec2 entityRectDim = GetDim(entityBounds);
                        r32 minBoundDim = 0.6f;
                        if(entityRectDim.x < minBoundDim || entityRectDim.x < minBoundDim)
                        {
                            entityRectDim.x = Max(entityRectDim.x, minBoundDim);
                            entityRectDim.y = Max(entityRectDim.y, minBoundDim);
                            
                            Vec2 entityRectCenter = GetCenter(entityBounds);
                            entityBounds = RectCenterDim(entityRectCenter, entityRectDim);
                        }
                        
                        
                        screenBounds = ProjectOnScreenCameraAligned(group, entity->P, entityBounds, &cameraZ);
                    }
                    
                    r32 distanceSq = LengthSq(screenMouseP - GetCenter(screenBounds));
                    if(distanceSq < maxOverallDistanceSq) // && cameraZ < overallMinCameraZ
                    {
                        maxOverallDistanceSq = distanceSq;
                        overallMinCameraZ = cameraZ;
                        result = entity->identifier;
                    }
                    
                    entity->showHUD = false;
                    if(PointInRect(screenBounds, screenMouseP))
                    {
                        entity->showHUD = true;
                        
                        u32 maxEntityCount = ArrayCount(worldMode->nearestEntities);
                        u32 destIndex = maxEntityCount - 1;
                        Assert(maxEntityCount > 1);
                        for(i32 slideIndex = destIndex - 1; slideIndex >= 0; --slideIndex)
                        {
                            if(cameraZ < worldMode->nearestCameraZ[slideIndex])
                            {
                                worldMode->nearestEntities[slideIndex + 1] = worldMode->nearestEntities[slideIndex];
                                worldMode->nearestCameraZ[slideIndex + 1] = worldMode->nearestCameraZ[slideIndex];
                                
                                destIndex = slideIndex;
                            }
                            else
                            {
                                break;
                            }
                        }
                        
                        worldMode->nearestEntities[destIndex] = entity;
                        worldMode->nearestCameraZ[destIndex] = cameraZ;
                    }
                }
            }
            
            entity = entity->next;
        }
    }
#endif
    
    return result;
}

internal void UpdateEntities(GameModeWorld* worldMode, r32 timeToAdvance, ClientPlayer* myPlayer)
{
    
}

internal b32 UpdateAndRenderGame(GameState* gameState, GameModeWorld* worldMode, RenderGroup* group, PlatformInput* input)
{
    b32 result = false;
    
    Vec3 inputAcc = {};
    u64 targetEntityID = 0;
    u32 desiredAction = 0;
    u64 overlappingEntityID = 0;
    
    
    ClientPlayer* myPlayer = &worldMode->player;
    ReceiveNetworkPackets(gameState, worldMode);
    
    
    worldMode->originalTimeToAdvance = input->timeToAdvance;
    if(Pressed(&input->pauseButton))
    {
        worldMode->gamePaused = !worldMode->gamePaused;
        SendPauseToggleMessage();
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
    worldMode->worldMouseP = groundMouseP;
    
    Vec2 newMouseP = V2(input->relativeMouseX, input->relativeMouseY);
    Vec2 deltaMouseScreenP = newMouseP - worldMode->relativeScreenMouseP;
    worldMode->relativeScreenMouseP = newMouseP;
    
    
    if(IsValid(myPlayer->ID))
    {
        BaseComponent* player = GetComponent(worldMode, myPlayer->ID, BaseComponent);
        if(player)
        {
            if(IsDown(&input->moveLeft))
            {
                inputAcc.x = -1.0f;
            }
            
            if(IsDown(&input->moveRight))
            {
                inputAcc.x = 1.0f;
            }
            
            if(IsDown(&input->moveDown))
            {
                inputAcc.y = -1.0f;
            }
            if(IsDown(&input->moveUp))
            {
                inputAcc.y = 1.0f;
            }
            
            if(Pressed(&input->actionUp))
            {
                SendSpawnRequest(player->universeP);
            }
            
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
            //ResetLightGrid(worldMode);
            
            MoveCameraTowards(worldMode, player, V2(0, 0), V2(0, 0), 1.0f);
            UpdateAndSetupGameCamera(worldMode, group, input);
            UpdateEntities(worldMode, input->timeToAdvance, myPlayer);
            
            BeginDepthPeel(group);
            
            AddEntityLights(worldMode);
            //FinalizeLightGrid(worldMode, group);
            
            Vec3 directionalLightColor = V3(1, 1, 1);
            Vec3 directionalLightDirection = V3(0, 0, -1);
            r32 directionalLightIntensity = 1.0f;
            Vec3 ambientLightColor = HandleDaynightCycle(worldMode, input);
            ambientLightColor = V3(0, 0, 0);
            
            PushAmbientLighting(group, ambientLightColor, directionalLightColor, directionalLightDirection, directionalLightIntensity);
            
            RenderCharacterAnimations(worldMode, group, input->timeToAdvance);
            myPlayer->universeP = player->universeP;
            Vec3 deltaP = -Subtract(myPlayer->universeP, myPlayer->oldUniverseP);
            
            
            for(u32 voronoiIndex = 0; voronoiIndex < ArrayCount(worldMode->voronoiPingPong); ++voronoiIndex)
            {
                worldMode->voronoiPingPong[voronoiIndex].deltaP += deltaP;
            }
            
            PreloadAllGroundBitmaps(group->assets);
            //UpdateAndRenderGround(worldMode, group, myPlayer->universeP, myPlayer->oldUniverseP, input->timeToAdvance);
            
            worldMode->particleCache->deltaParticleP = deltaP;
            UpdateAndRenderParticleEffects(worldMode, worldMode->particleCache, input->timeToAdvance, group);
            //worldMode->boltCache->deltaP = deltaP;
            //UpdateAndRenderBolts(worldMode, worldMode->boltCache, input->timeToAdvance, group);
            
            EndDepthPeel(group);
            
            myPlayer->oldUniverseP = myPlayer->universeP;
            
            
            RandomSequence seq = Seed(123);
            GameProperties bitmapProperties = {};
            bitmapProperties.properties[0].property = Property_Test;
            bitmapProperties.properties[0].value = Value1;
            
            BitmapId test = QueryBitmaps(group->assets, 0, &seq, &bitmapProperties);
            
            if(IsValid(test))
            {
                PushBitmap(group, UprightTransform(), test, V3(0, 0, 0), 1.0f);
            }
            
            
            
            
            
            RenderEditor(group, worldMode, deltaMouseScreenP);
#if 0
            Rect3 worldCameraBounds = GetScreenBoundsAtTargetDistance(group);
            Rect2 screenBounds = RectCenterDim(V2(0, 0), V2(worldCameraBounds.max.x - worldCameraBounds.min.x, worldCameraBounds.max.y - worldCameraBounds.min.y));
            PushRectOutline(group, FlatTransform(), screenBounds, V4(1.0f, 0.0f, 0.0f, 1.0f), 0.1f); 
#endif
        }
    }
    SendUpdate(inputAcc);
    
    
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
DebugTable *globalDebugTable;
#endif 

//void GameUpdateAndRender(PlatformMemory* memory, PlatformInput* input, GameRenderCommands* commands)
extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    platformAPI = memory->api;
    
    r32 timeToAdvance = input->timeToAdvance;
#if FORGIVENESS_INTERNAL
    debugGlobalMemory = memory;
    globalDebugTable = memory->debugTable;
    
    {
        DEBUG_DATA_BLOCK(Renderer);
        {
            DEBUG_DATA_BLOCK(Entity);
        }
        {
            DEBUG_DATA_BLOCK(Camera);
        }
    }
    
    {
        DEBUG_DATA_BLOCK(GameProfile);
        DEBUG_UI_ELEMENT(DebugType_FrameSlider, FrameSlider);
        DEBUG_UI_ELEMENT(DebugType_TopClockList, GameUpdateAndRender1);
        DEBUG_UI_ELEMENT(DebugType_FrameBarGraph, GameUpdateAndRender2);
        DEBUG_UI_ELEMENT(DebugType_ThreadIntervalGraph, GameUpdateAndRender3);
        DEBUG_UI_ELEMENT(DebugType_LastFrameInfo, LastFrame);
        DEBUG_UI_ELEMENT(DebugType_MemoryRemaining, MemoryRemaining);
    }
#endif 
    
    TIMED_FUNCTION();
    
    GameState* gameState = memory->gameState;
    if(!gameState)
    {
        LoadMetaData();
        
        gameState = BootstrapPushStruct(GameState, totalPool);
        memory->gameState = gameState;
#if FORGIVENESS_INTERNAL
        gameState->timeCoeff = 100.0f;
#endif
        
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
    TranState* tranState = (TranState*) memory->tranState;
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

