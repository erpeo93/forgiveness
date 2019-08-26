
#include "forg_client.h"
#include "client_generated.h"
global_variable ClientNetworkInterface* clientNetwork; 

#include "forg_token.cpp"
#include "forg_meta.cpp"
#include "forg_pool.cpp"
#include "forg_taxonomy.cpp"
#include "forg_physics.cpp"
#include "forg_world.cpp"
#include "forg_world_client.cpp"
#include "forg_world_generation.cpp"
#include "forg_editor.cpp"
#include "forg_inventory.cpp"
#include "forg_asset.cpp"
#include "forg_render.cpp"
#include "forg_camera.cpp"
#include "forg_light.cpp"
#include "forg_network_client.cpp"
#include "forg_plant.cpp"
#include "forg_mesh.cpp"
#include "forg_rock.cpp"
#include "forg_object.cpp"
#include "forg_crafting.cpp"
#include "forg_sound.cpp"
#include "forg_particles.cpp"
#include "forg_bolt.cpp"
#include "forg_bound.cpp"
#include "forg_animation.cpp"
#include "forg_cutscene.cpp"
#include "forg_ground.cpp"
#include "forg_UIcommon.cpp"

inline void MarkAllPakFilesAsToDelete(GameModeWorld* worldMode, char* path)
{
    PlatformFileGroup fileGroup = platformAPI.GetAllFilesBegin(PlatformFile_uncompressedAsset, path);
    
    for(u32 fileIndex = 0; fileIndex < fileGroup.fileCount; ++fileIndex)
    {
        PlatformFileHandle handle = platformAPI.OpenNextFile(&fileGroup, path);
        
        ToDeleteFile* toDelete;
        FREELIST_ALLOC(toDelete, worldMode->firstFreeFileToDelete, PushStruct(&worldMode->deletedFilesPool, ToDeleteFile));
        
        toDelete->toDelete = true;
        RemoveExtension(toDelete->filename, sizeof(toDelete->filename), handle.name);
        
        FREELIST_INSERT(toDelete, worldMode->firstFileToDelete);
        
        platformAPI.CloseHandle(&handle);
    }
    
    platformAPI.GetAllFilesEnd(&fileGroup);
}

inline void DeleteAllFilesNotArrived(GameModeWorld* worldMode, char* path)
{
    for(ToDeleteFile* file = worldMode->firstFileToDelete; file; file = file->next)
    {
        if(file->toDelete)
        {
            char toDeleteWildcard[128];
            FormatString(toDeleteWildcard, sizeof(toDeleteWildcard), "%s.*", file->filename);
            platformAPI.DeleteFileWildcards(path, toDeleteWildcard);
        }
    }
    
    worldMode->firstFileToDelete = 0;
    Clear(&worldMode->deletedFilesPool);
}

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
    
    MarkAllPakFilesAsToDelete(result, "assets");
    
    clientNetwork->nextSendUnreliableApplicationData = {};
    clientNetwork->nextSendReliableApplicationData = {};
    ResetReceiver(&clientNetwork->receiver);
    platformAPI.net.OpenConnection(clientNetwork->network, loginServer, LOGIN_PORT);
    
    
    LoginRequest(4444);
    
    
    u32 entityCount = ArrayCount(result->entities);
    Assert(!(entityCount & (entityCount - 1)));
    
    result->temporaryPool = &gameState->framePool;
    result->persistentPool = &gameState->persistentPool;
    
    result->table = PushStruct(&gameState->modePool, TaxonomyTable);
    result->oldTable = PushStruct(&gameState->modePool, TaxonomyTable);
    
    result->chunkDim = CHUNK_DIM;
    Assert(result->chunkDim % 2 == 0);
    u8 halfChunkDim = result->chunkDim / 2;
    result->voxelSide = VOXEL_SIZE;
    result->chunkSide = result->voxelSide * result->chunkDim;
    result->oneOverVoxelSide = 1.0f / result->voxelSide;
    result->oneOverChunkSide = 1.0f / result->chunkSide;
    
    
    result->particleCache = PushStruct(&gameState->modePool, ParticleCache, AlignClear(16));
    InitParticleCache(result->particleCache, gameState->assets, &gameState->visualEffectsPool);
    
    result->boltCache = PushStruct(&gameState->modePool, BoltCache);
    InitBoltCache(result->boltCache, &gameState->visualEffectsPool, 11111);
    
    
    gameState->world = result;
    result->defaultCameraZ = 34.0f;
    result->cameraWorldOffset = V3(0.0f, 0.0f, result->defaultCameraZ);
    result->cameraFocusID = 0;
    result->modulationWithFocusColor = 0.15f;
    
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
    
    result->currentPhase = DayPhase_Day;
    result->soundState = &gameState->soundState;
    
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
#endif
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
    
    return ambientLightColor;
}

internal void RenderEntities(GameModeWorld* worldMode, RenderGroup* group, ClientPlayer* myPlayer, r32 timeToAdvance)
{
    for(u32 entityIndex = 0; 
        entityIndex < ArrayCount(worldMode->entities); 
        entityIndex++)
    {
        ClientEntity* entity = worldMode->entities[entityIndex];
        while(entity)
        {
            ClientEntity* next = entity->next;
            if(entity->identifier && !IsSet(entity, Flag_deleted | Flag_Attached))
            {
                AnimationEntityParams params = StandardEntityParams();
                if(entity->identifier == myPlayer->openedContainerID)
                {
                    params = ContainerEntityParams();
                }
                
                
#if RESTRUCTURING                
                if(UI->mode == UIMode_Loot && entity->identifier == myPlayer->identifier)
                {
                    ClientEntity* container = GetEntityClient(worldMode, myPlayer->openedContainerID);
                    if(container && container->P.y > 0)
                    {
                        params.transparent = true;
                    }
                }
#endif
                
                entity->animation.output = RenderEntity(group, worldMode, entity, timeToAdvance, params);
                
                if(IsCreature(worldMode->table, entity->taxonomy))
                {
                    entity->lifePointsTriggerTime += timeToAdvance;
                    entity->staminaTriggerTime += timeToAdvance;
                    
                    r32 HUD_FADE_TIME = 2.0f;
                    r32 HUD_TRIGGER_TIME = 2.0f;
                    if(entity->showHUD)
                    {
                        if(entity->lifePointsTriggerTime >= HUD_FADE_TIME)
                        {
                            entity->lifePointsTriggerTime = 0.5f * HUD_TRIGGER_TIME;
                        }
                        
                        if(entity->staminaTriggerTime >= HUD_FADE_TIME)
                        {
                            entity->staminaTriggerTime = 0.5f * HUD_TRIGGER_TIME;
                        }
                        
                        entity->lifePointsTriggerTime = Min(entity->lifePointsTriggerTime, HUD_TRIGGER_TIME);
                        entity->staminaTriggerTime = Min(entity->lifePointsTriggerTime, HUD_TRIGGER_TIME);
                    }
                    
                    r32 lifePointAlpha;
                    r32 staminaAlpha;
                    
                    if(entity->lifePointsTriggerTime <= HUD_TRIGGER_TIME)
                    {
                        lifePointAlpha = Clamp01MapToRange(0.0f, entity->lifePointsTriggerTime, HUD_TRIGGER_TIME);
                    }
                    else
                    {
                        lifePointAlpha = 1.0f - Clamp01MapToRange(HUD_TRIGGER_TIME, entity->lifePointsTriggerTime, HUD_FADE_TIME);
                    }
                    
                    if(entity->staminaTriggerTime <= HUD_TRIGGER_TIME)
                    {
                        staminaAlpha = Clamp01MapToRange(0.0f, entity->staminaTriggerTime, HUD_TRIGGER_TIME);
                    }
                    else
                    {
                        staminaAlpha = 1.0f - Clamp01MapToRange(HUD_TRIGGER_TIME, entity->staminaTriggerTime, HUD_FADE_TIME);
                    }
                    
                    
                    r32 yOffset = 0.18f;
                    r32 maxBarWidth = 1.0f;
                    r32 barHeight = 0.05f;
                    
                    
                    ObjectTransform lifePointTransform = UprightTransform();
                    lifePointTransform.additionalZBias = 3.0f;
                    
                    ObjectTransform backTransform = UprightTransform();
                    backTransform.additionalZBias = 2.9f;
                    
                    Vec4 lifeColor = V4(0.5f, 0, 0, lifePointAlpha);
                    Vec4 staminaColor = V4(0, 0.5f, 0, staminaAlpha);
                    Vec4 backLifeColor = V4(0.2f, 0.2f, 0.2f, lifePointAlpha);
                    Vec4 backStaminaColor = V4(0.2f, 0.2f, 0.2f, staminaAlpha);
                    
                    
                    if(entity->maxLifePoints)
                    {
                        r32 lifePointRatio = entity->lifePoints / entity->maxLifePoints;
                        Rect2 lifeRect = RectMinDim(entity->P.xy - V2(0.5f * maxBarWidth, yOffset), V2(lifePointRatio * maxBarWidth, barHeight));
                        
                        Rect2 backRect = RectMinDim(entity->P.xy - V2(0.5f * maxBarWidth, yOffset), V2(maxBarWidth, barHeight));
                        
                        PushRect(group, backTransform, backRect, backLifeColor);
                        PushRect(group, lifePointTransform, lifeRect, lifeColor);
                    }
                    
                    if(entity->maxStamina)
                    {
                        r32 staminaRatio = entity->maxStamina ? entity->stamina / entity->maxStamina : 0.0f;
                        
                        Rect2 staminaRect = RectMinDim(entity->P.xy - V2(0.5f * maxBarWidth, yOffset + 2.0f * barHeight), V2(staminaRatio * maxBarWidth, barHeight));
                        Rect2 backRect = RectMinDim(entity->P.xy - V2(0.5f * maxBarWidth, yOffset + 2.0f * barHeight), V2(maxBarWidth, barHeight));
                        
                        PushRect(group, backTransform, backRect, backStaminaColor);
                        PushRect(group, lifePointTransform, staminaRect, staminaColor);
                    }
                }
                
                if(entity->animation.output.entityPresent && entity->draggingID)
                {
                    // NOTE(Leonardo): render target entity here at specified angle and offset
                    ClientEntity* targetEntity = GetEntityClient(worldMode, entity->draggingID);
                    if(targetEntity)
                    {
                        targetEntity->animation.flipOnYAxis = entity->animation.flipOnYAxis;
                        AnimationEntityParams targetParams = StandardEntityParams();
                        targetParams.angle = entity->animation.output.entityAngle;
                        targetParams.offset = entity->animation.output.entityOffset;
                        
                        targetEntity->P = entity->P;
                        RenderEntity(group, worldMode, targetEntity, 0, targetParams);
                    }
                }
            }
            
            entity = next;
        }
    }
}

internal void AddEntityLights(GameModeWorld* worldMode)
{
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
}

internal u64 DetectNearestEntities(GameModeWorld* worldMode, RenderGroup* group, Vec2 screenMouseP)
{
    u64 result = 0;
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
    
    return result;
}

internal void UpdateEntities(GameModeWorld* worldMode, r32 timeToAdvance, ClientEntity* player, ClientPlayer* myPlayer)
{
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
                entity->actionTime += timeToAdvance;
                
                
                entity->P = Subtract(entity->universeP, myPlayer->universeP);
                entity->timeFromLastUpdate += timeToAdvance;
                
                if(entity->beingDeleted)
                {
                    entity->animation.goOutTime += timeToAdvance;
                }
                
                if(entity->timeFromLastUpdate >= 3.0f || entity->animation.goOutTime >= ALPHA_GO_OUT_SECONDS)
                {
                    DeleteEntityClient(worldMode, entity);
                }
                
                Vec3 offset = {};
                if(entity->identifier == myPlayer->identifier)
                {
                    r32 maxDistanceForAccPrediction = 0.3f;
                    if(myPlayer->distanceCoeffFromServerP < maxDistanceForAccPrediction && timeToAdvance > 0)
                    {
                        r32 accelerationCoeff = 1.0f;
                        Vec3 acc = ComputeAcceleration(myPlayer->acceleration, myPlayer->velocity, DefaultMoveSpec(accelerationCoeff));
                        myPlayer->velocity += acc * timeToAdvance;
                        
                        Vec3 velocity;
                        if(myPlayer->distanceCoeffFromServerP < 0.15f && player->action <= Action_Idle)
                        {
                            velocity = myPlayer->velocity;
                        }
                        else
                        {
                            r32 lerp = Clamp01MapToRange(0.0f, myPlayer->distanceCoeffFromServerP, maxDistanceForAccPrediction);
                            velocity = Lerp(myPlayer->velocity, lerp, entity->velocity);
                        }
                        
                        if(Abs(myPlayer->acceleration.x) > 0.1f)
                        {
                            entity->animation.flipOnYAxis = (myPlayer->acceleration.x < 0);
                        }
                        
                        if(myPlayer->distanceCoeffFromServerP < 0.15f)
                        {
                            offset = MoveEntityClient(worldMode, entity, timeToAdvance, acc, velocity, &myPlayer->velocity);
                            
                            if(LengthSq(myPlayer->acceleration) > Square(0.1f))
                            {
                                entity->action = Action_Move;
                            }
                        }
                        else
                        {
                            offset = timeToAdvance * velocity;
                            if(LengthSq(velocity) > Square(0.1f))
                            {
                                entity->action = Action_Move;
                            }
                            
                        }
                    }
                    else
                    {
                        offset = timeToAdvance * entity->velocity;
                        if(entity->action == Action_Move && Abs(entity->velocity.x) > 0.1f)
                        {
                            entity->animation.flipOnYAxis = (offset.x < 0);
                        }
                    }
                }
                else
                {
                    offset = timeToAdvance * entity->velocity;
                    if(entity->action == Action_Move && Abs(entity->velocity.x) > 0.1f)
                    {
                        entity->animation.flipOnYAxis = (offset.x < 0);
                    }
                }
                
                entity->universeP = Offset(entity->universeP, offset.xy);
                
                entity->modulationWithFocusColor = 0;
                HandleClientPrediction(entity, timeToAdvance);
                SetEquipmentReferenceAction(worldMode, entity);
                UpdateAnimationEffects(worldMode, entity, timeToAdvance);
            }
            
            entity = entity->next;
        }
    }
}

internal b32 UpdateAndRenderGame(GameState* gameState, GameModeWorld* worldMode, RenderGroup* group, PlatformInput* input)
{
    Vec3 inputAcc = {};
    u64 targetEntityID = 0;
    u32 desiredAction = Action_Idle;
    u64 overlappingEntityID = 0;
    
    b32 result = false;
    
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
    worldMode->relativeScreenMouseP = V2(input->relativeMouseX, input->relativeMouseY);
    
    
    b32 reloadTaxonomyAutocompletes = false;
    b32 reloadAssetAutocompletes = false;
    if(myPlayer->identifier)
    {
        ClientEntity* player = GetEntityClient(worldMode, myPlayer->identifier);
        if(player)
        {
            char* filePath = "assets";
            if(worldMode->dataFileSent)
            {
                ++worldMode->patchSectionArrived;
                
                
#if RESTRUCTURING                
                WriteAllFiles(&worldMode->filePool, filePath, worldMode->firstDataFileArrived, false);
                switch(worldMode->dataFileSent)
                {
                    case DataFileSent_OnlyTaxonomies:
                    {
                        SwapTables(worldMode);
                        ReadTaxonomiesFromFile();
                        CopyAndLoadTabsFromOldTable(worldMode->oldTable);
                        
                        for(DataFileArrived* file = worldMode->firstDataFileArrived; file; file = file->next)
                        {
                            ImportAllFiles(worldMode->editorRoles, file->name);
                        }
                    } break;
                    
                    case DataFileSent_OnlyAssets:
                    {
                        CopyAndLoadTabsFromOldTable(worldMode->oldTable);
                        ImportAllAssetFiles(worldMode, filePath, &worldMode->filePool);
                    } break;
                    
                    case DataFileSent_Everything:
                    {
                        SwapTables(worldMode);
                        ReadTaxonomiesFromFile();
                        
                        ImportAllFiles(worldMode->editorRoles, 0);
                        ImportAllAssetFiles(worldMode, filePath, &worldMode->filePool);
                        
                        platformAPI.DEBUGWriteFile("editorErrors", worldMode->table->errors, sizeof(worldMode->table->errors[0]) * worldMode->table->errorCount);
                    } break;
                }
#endif
                
                worldMode->firstDataFileArrived = 0;
                TranslateParticleEffects(worldMode->particleCache, worldMode->oldTable, worldMode->table);
                
                for(u32 entityIndex = 0; entityIndex < ArrayCount(worldMode->entities); ++entityIndex)
                {
                    ClientEntity* entity = worldMode->entities[entityIndex];
                    while(entity)
                    {
                        TranslateClientEntity(worldMode->oldTable, worldMode->table, entity);
                        entity = entity->next;
                    }
                }
                
                TranslateClientPlayer(worldMode->oldTable, worldMode->table, myPlayer);
                TaxonomySlot* rootSlot = &worldMode->table->root;
                
                reloadTaxonomyAutocompletes = true;
                
                
                
                
                worldMode->dataFileSent = DataFileSent_Nothing;
            }
            
            if(worldMode->allPakFilesArrived)
            {
                ++worldMode->patchSectionArrived;
                CloseAllHandles(gameState->assets);
                
                
#if RESTRUCTURING                
                for(DataFileArrived* file = worldMode->firstPakFileArrived; file; file = file->next)
                {
                    MarkFileAsArrived(worldMode, file->name);
                }
                
                
                WriteAllFiles(&worldMode->filePool, filePath, worldMode->firstPakFileArrived, true);
                DeleteAllFilesNotArrived(worldMode, "assets");
                
                worldMode->firstPakFileArrived = 0;
                
                i32 destIndex = (gameState->assetsIndex) ? 0 : 1;
                MemoryPool* toFree = gameState->assetsPool + destIndex;
                Clear(toFree);
                gameState->pingPongAssets[destIndex] = InitAssets(gameState, toFree, gameState->textureQueue, MegaBytes(256));
                
                gameState->assets = gameState->pingPongAssets[destIndex];
                gameState->assetsIndex = destIndex;
                
                Clear(&worldMode->filePool);
                worldMode->currentFile = 0;
                
                
#endif
                
                // TODO(Leonardo): this should not be here!
#if 0
                if(!gameState->music)
                {
                    RandomSequence seq = Seed(worldMode->worldSeed);
                    gameState->music = PlaySound(&gameState->soundState, gameState->assets, GetRandomSound(gameState->assets, Asset_music, &seq), 0.0f);
                }
#endif
                
                
                reloadAssetAutocompletes = true;
                MarkAllPakFilesAsToDelete(worldMode, "assets");
                worldMode->allPakFilesArrived = false;
            }
            
            
            
            
            group->assets = gameState->assets;
            player->identifier = myPlayer->identifier;
            player->P = V3(0, 0, 0);
            
            b32 canRender = (worldMode->patchSectionArrived >= 2);
            if(canRender)
            {
                Clear(group, V4(0.0f, 0.0f, 0.0f, 1.0f));
                ResetLightGrid(worldMode);
                
                MoveCameraTowards(worldMode, player, V2(0, 0), V2(0, 0), 1.0f);
                
#if RESTRUCTURING                
                if(!TooFarForAction(myPlayer, output.desiredAction, output.targetEntityID))
                {
                    b32 resetAcceleration = false;
                    if(NearEnoughForAction(myPlayer, output.desiredAction, output.targetEntityID, &resetAcceleration))
                    {
                        player->action = (EntityAction) output.desiredAction;
                        if(resetAcceleration)
                        {
                            output.inputAcc = {};
                        }
                    }
                }
                ClientEntity* target = GetEntityClient(worldMode, output.targetEntityID);
                if(target)
                {
                    player->animation.flipOnYAxis = (target->P.x < 0);
                }
                myPlayer->acceleration = output.inputAcc;
                u64 overallNearestID = DetectNearestEntities(worldMode, group, screenMouseP);
                if(!output.overlappingEntityID && overallNearestID != player->identifier)
                {
                    output.overlappingEntityID = overallNearestID;
                }
                
#endif
                
                UpdateAndSetupGameCamera(worldMode, group, input);
                
                UpdateEntities(worldMode, input->timeToAdvance, player, myPlayer);
                
                BeginDepthPeel(group);
                
                AddEntityLights(worldMode);
                FinalizeLightGrid(worldMode, group);
                
                Vec3 directionalLightColor = V3(1, 1, 1);
                Vec3 directionalLightDirection = V3(0, 0, -1);
                r32 directionalLightIntensity = 1.0f;
                Vec3 ambientLightColor = HandleDaynightCycle(worldMode, input);
                ambientLightColor = V3(0, 0, 0);
                
                PushAmbientLighting(group, ambientLightColor, directionalLightColor, directionalLightDirection, directionalLightIntensity);
                RenderEntities(worldMode, group, myPlayer, input->timeToAdvance);
                
                myPlayer->universeP = player->universeP;
                Vec3 deltaP = -Subtract(myPlayer->universeP, myPlayer->oldUniverseP);
                
                
                for(u32 voronoiIndex = 0; voronoiIndex < ArrayCount(worldMode->voronoiPingPong); ++voronoiIndex)
                {
                    worldMode->voronoiPingPong[voronoiIndex].deltaP += deltaP;
                }
                
                u32 chunkApron = 2;
                UpdateAndRenderGround(gameState, worldMode, group, myPlayer, chunkApron, input->timeToAdvance);
                
                worldMode->particleCache->deltaParticleP = deltaP;
                UpdateAndRenderParticleEffects(worldMode, worldMode->particleCache, input->timeToAdvance, group);
                
                worldMode->boltCache->deltaP = deltaP;
                UpdateAndRenderBolts(worldMode, worldMode->boltCache, input->timeToAdvance, group);
                
                EndDepthPeel(group);
                
                myPlayer->oldUniverseP = myPlayer->universeP;
                
#if 0
                Rect3 worldCameraBounds = GetScreenBoundsAtTargetDistance(group);
                Rect2 screenBounds = RectCenterDim(V2(0, 0), V2(worldCameraBounds.max.x - worldCameraBounds.min.x, worldCameraBounds.max.y - worldCameraBounds.min.y));
                PushRectOutline(group, FlatTransform(), screenBounds, V4(1.0f, 0.0f, 0.0f, 1.0f), 0.1f); 
#endif
            }
        }
        
    }
    
    SendUpdate(inputAcc, targetEntityID, desiredAction, overlappingEntityID);
    
    
    return result;
}


internal b32 UpdateAndRenderLauncherScreen(GameState* gameState, RenderGroup* group, PlatformInput* input)
{
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
    
    return 0;
}

inline void FreeGameMode(GameModeWorld* worldMode)
{
    Clear(&worldMode->oldTable->pool_);
    Clear(&worldMode->table->pool_);
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
        
        MemoryPool* pool = &gameState->assetsPool;
        gameState->assets = InitAssets(gameState, pool, gameState->textureQueue, MegaBytes(256));
        
        
        InitializeSoundState(&gameState->soundState, &gameState->audioPool);
        
        gameState->networkInterface.network = input->network;
        clientNetwork = &gameState->networkInterface;
        
        gameState->receiveNetworkPackets.network = &clientNetwork->network;
        gameState->receiveNetworkPackets.ReceiveData = platformAPI.net.ReceiveData;
        
        clientNetwork->nextSendUnreliableApplicationData = {};
        clientNetwork->nextSendReliableApplicationData = {};
        
        platformAPI.PushWork(gameState->slowQueue, ReceiveNetworkPackets, &gameState->receiveNetworkPackets);
    }
    
    
    
    TempMemory renderMemory = BeginTemporaryMemory(&gameState->framePool);
    
    RestoreLockedAssets(gameState->assets);
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
                    FreeGameMode(gameState->world);
                    
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

