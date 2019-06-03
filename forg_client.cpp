#include "forg_client.h"
global_variable ClientNetworkInterface* clientNetwork; 
#include "client_generated.h"
#include "forg_sort.cpp"
#include "forg_token.cpp"
#include "forg_meta.cpp"
#include "forg_pool.cpp"
#include "forg_world.cpp"
#include "forg_taxonomy.cpp"
#include "forg_world_generation.cpp"
#include "forg_editor.cpp"
#include "forg_inventory.cpp"
#include "forg_physics.cpp"
#include "forg_rule.cpp"

inline void AddFlags(ClientEntity* entity, u32 flags)
{
    entity->flags |= flags;
}

inline b32 IsSet(ClientEntity* entity, i32 flag)
{
    b32 result = (entity->flags & flag);
    return result;
}

inline void ClearFlags(ClientEntity* entity, i32 flags)
{
    entity->flags &= ~flags;
}

inline ClientEntity* GetEntityClient(GameModeWorld* worldMode, u64 identifier, b32 allocate = false)
{
    ClientEntity* result = 0;
    if(identifier)
    {
        u32 index = identifier & (ArrayCount(worldMode->entities) - 1);
        
        BeginTicketMutex(&worldMode->entityMutex);
        
        ClientEntity* entity = worldMode->entities[index];
        ClientEntity* firstFree = 0;
        
        while(entity)
        {
            if(IsSet(entity, Flag_deleted))
            {
                firstFree = entity;
            }
            else
			{
				Assert(entity->identifier);
				if(entity->identifier == identifier)
				{
					result = entity;
					break;
				}
			}
            
            entity = entity->next;
        }
        
        if(!result && allocate)
        {
            if(firstFree)
            {
                result = firstFree;
            }
            else
            {
                result = PushStruct(&worldMode->entityPool, ClientEntity);
                result->next = worldMode->entities[index];
                worldMode->entities[index] = result;
            }
        }
        
        EndTicketMutex(&worldMode->entityMutex);
    }
    
    return result;
}

internal void FreeStem(GameModeWorld* worldMode, PlantStem* stem)
{
    for(PlantSegment* segment = stem->root; segment;)
    {
        PlantSegment* nextToFree = segment->next;
        
        for(PlantStem* clone = segment->clones; clone;)
        {
            PlantStem* next = clone->next;
            FreeStem(worldMode, clone);
            FREELIST_DEALLOC(clone, worldMode->firstFreePlantStem);
            
            clone = next;
        }
        
        for(PlantStem* child = segment->childs; child;)
        {
            PlantStem* next = child->next;
            FreeStem(worldMode, child);
            FREELIST_DEALLOC(child, worldMode->firstFreePlantStem);
            
            child = next;
        }
        
        FREELIST_DEALLOC(segment, worldMode->firstFreePlantSegment);
        segment = nextToFree;
    }
}

internal void DeleteEntityClient(GameModeWorld* worldMode, ClientEntity* entity)
{
    AddFlags(entity, Flag_deleted);
    
    
    u32 taxonomy = entity->taxonomy;
    Assert(taxonomy);
    if(IsRock(worldMode->table, taxonomy))
    {
        ClientRock* toFree = entity->rock;
        if(toFree)
        {
            FREELIST_DEALLOC(toFree, worldMode->firstFreeRock);
            entity->rock = 0;
        }
    }
    else if(IsPlant(worldMode->table, taxonomy))
    {
        ClientPlant* toFree = entity->plant;
        if(toFree)
        {
            Assert(toFree->canRender);
            for(PlantStem* stem = toFree->plant.firstTrunk; stem; stem = stem->next)
            {
                FreeStem(worldMode, stem);
            }
            FREELIST_FREE(toFree->plant.firstTrunk, PlantStem, worldMode->firstFreePlantStem);
            *toFree = {};
            
            FREELIST_DEALLOC(toFree, worldMode->firstFreePlant);
            entity->plant = 0;
        }
        
    }
    
    entity->effectCount = 0;
    entity->objects.maxObjectCount = 0;
    entity->objects.objectCount = 0;
    
    
    for(u32 slotIndex = 0; slotIndex < Slot_Count; ++slotIndex)
    {
        entity->equipment[slotIndex].ID = 0;
    }
    
    entity->animation = {};
    entity->timeFromLastUpdate = 0;
    
    entity->effectReferenceAction = 0;
    
    
    for(AnimationEffect** effectPtr = &entity->firstActiveEffect; *effectPtr;)
    {
        AnimationEffect* effect = *effectPtr;
        *effectPtr = effect->next;
        
        BeginTicketMutex(&worldMode->animationEffectMutex);
        FREELIST_DEALLOC(effect, worldMode->firstFreeEffect);
        EndTicketMutex(&worldMode->animationEffectMutex);
    }
    
    entity->prediction.type = Prediction_None;
    
}

inline b32 ActionRequiresZooming(EntityAction action, r32* zoomLevel)
{
    b32 result = false;
    if(action == Action_Open)
    {
        result = true;
        *zoomLevel = Max(*zoomLevel, 1.8f);
    }
    return result;
}

inline void MoveTowards_(GameModeWorld* worldMode, u64 id, Vec2 cameraWorldOffset, Vec2 cameraEntityOffset, r32 zoomCoeff = 1.0f)
{
    worldMode->cameraFocusID = id;
    worldMode->destCameraEntityOffset = cameraEntityOffset;
    worldMode->destCameraWorldOffset = V3(cameraWorldOffset, worldMode->defaultCameraZ / zoomCoeff);
}

inline void MoveTowards(GameModeWorld* worldMode, ClientEntity* entityC, Vec2 cameraWorldOffset = V2(0, 0), Vec2 cameraEntityOffset = V2(0, 0), r32 zoomCoeff = 1.0f)
{
    cameraWorldOffset += entityC->P.xy;
    cameraEntityOffset += entityC->animation.cameraEntityOffset;
    MoveTowards_(worldMode, entityC->identifier, cameraWorldOffset, cameraEntityOffset, zoomCoeff);
}

inline void UpdateCamera(GameModeWorld* worldMode, r32 timeToUpdate)
{
    if(worldMode->cameraFocusID)
    {
        ClientEntity* entity = GetEntityClient(worldMode, worldMode->cameraFocusID);
        if(entity)
        {
            worldMode->cameraWorldOffset += 6.0f * timeToUpdate * (worldMode->destCameraWorldOffset - worldMode->cameraWorldOffset);
            worldMode->cameraEntityOffset += 6.0f * timeToUpdate * (worldMode->destCameraEntityOffset - worldMode->cameraEntityOffset);
        } 
    }
}


struct GetUniversePosQuery
{
    WorldChunk* chunk;
    u32 tileX;
    u32 tileY;
    
    Vec2 chunkOffset;
};

GetUniversePosQuery TranslateRelativePos(GameModeWorld* worldMode, UniversePos baseP, Vec2 relativeP)
{
    GetUniversePosQuery result = {};
    
    baseP.chunkOffset.xy += relativeP;
    
    i32 chunkOffsetX = Floor(baseP.chunkOffset.x * worldMode->oneOverChunkSide);
    baseP.chunkX += chunkOffsetX;
    baseP.chunkOffset.x -= chunkOffsetX * worldMode->chunkSide;
    
    i32 chunkOffsetY = Floor(baseP.chunkOffset.y * worldMode->oneOverChunkSide);
    baseP.chunkY += chunkOffsetY;
    baseP.chunkOffset.y -= chunkOffsetY * worldMode->chunkSide;
    
    WorldChunk* chunk = GetChunk(worldMode->chunks, ArrayCount(worldMode->chunks), baseP.chunkX, baseP.chunkY, 0);
	if(chunk)
    {
        result.tileX = TruncateReal32ToI32(baseP.chunkOffset.x * worldMode->oneOverVoxelSide);
        result.tileY = TruncateReal32ToI32(baseP.chunkOffset.y * worldMode->oneOverVoxelSide);
        
        result.chunkOffset = baseP.chunkOffset.xy;
        if(result.tileX < CHUNK_DIM && result.tileY < CHUNK_DIM)
        {
            result.chunk = chunk;
        }
    }
    
    
    return result;
}


inline void ResetLightGrid(GameModeWorld* worldMode)
{
    UniversePos playerP = worldMode->player.universeP;
    
    for(u32 chunkIndex = 0; chunkIndex < ArrayCount(worldMode->chunks); ++chunkIndex)
    {
        WorldChunk* chunk = worldMode->chunks[chunkIndex]; 
        if(chunk)
        {
            for(u32 tileY = 0; tileY < CHUNK_DIM; ++tileY)
            {
                for(u32 tileX = 0; tileX < CHUNK_DIM; ++tileX)
                {
                    chunk->tiles[tileY][tileX].lightCount = 0;
                    chunk->tiles[tileY][tileX].lightIndexes = V4(-1, -1, -1, -1);
                }
            }
        }
    }
}

inline void AddLightToGrid(GameModeWorld* worldMode, Vec3 P, u32 index)
{
    u32 voxelApron = 8;
    for(r32 offsetY = -VOXEL_SIZE * voxelApron; offsetY <= VOXEL_SIZE * voxelApron; offsetY += VOXEL_SIZE)
    {
        for(r32 offsetX = -VOXEL_SIZE * voxelApron; offsetX <= VOXEL_SIZE * voxelApron; offsetX += VOXEL_SIZE)
        {
            GetUniversePosQuery query = TranslateRelativePos(worldMode, worldMode->player.universeP, P.xy + V2(offsetX, offsetY));
            
            if(query.chunk)
            {
                u32 tileX = query.tileX;
                u32 tileY = query.tileY;
                
                Assert(tileX < CHUNK_DIM);
                Assert(tileY < CHUNK_DIM);
                
                WorldTile* tile = &query.chunk->tiles[tileY][tileX];
                if(tile->lightCount < 4)
                {
                    r32 lightIndexReal = (r32) index;
                    Vec4* lightIndexes = (Vec4*) &tile->lightIndexes;
                    
                    
                    if((lightIndexes->x != lightIndexReal) &&
                       (lightIndexes->y != lightIndexReal) &&
                       (lightIndexes->z != lightIndexReal) &&
                       (lightIndexes->w != lightIndexReal))
                    {
                        u8 lightIndex = tile->lightCount++;
                        lightIndexes->E[lightIndex] = (r32) index;
                    }
                    
                }
                else
                {
                    InvalidCodePath;
                }
            }
        }
    }
}

inline Vec4 GetLightIndexes(GameModeWorld* worldMode, Vec3 P)
{
    Vec4 result = V4(-1, -1, -1, -1);
    GetUniversePosQuery query = TranslateRelativePos(worldMode, worldMode->player.universeP, P.xy);
    if(query.chunk)
    {
        result = query.chunk->tiles[query.tileY][query.tileX].lightIndexes;
    }
    
    return result;
}

inline WorldTile* GetTile(GameModeWorld* worldMode, UniversePos baseP, Vec2 P)
{
    WorldTile* result = &worldMode->nullTile;
    GetUniversePosQuery query = TranslateRelativePos(worldMode, baseP, P);
    if(query.chunk)
    {
        result = GetTile(query.chunk, query.tileX, query.tileY);
    }
    return result;
}

#include "forg_network_client.cpp"
#include "forg_asset.cpp"
#include "forg_render.cpp"
#include "forg_plant.cpp"
#include "forg_model.cpp"
#include "forg_crafting.cpp"
#include "forg_particles.cpp"
#include "forg_audio.cpp"
#include "forg_animation.cpp"
#include "forg_UI.cpp"
#include "forg_cutscene.cpp"
#include "forg_ground.cpp"

PLATFORM_WORK_CALLBACK(ReceiveNetworkPackets)
{
    ReceiveNetworkPacketWork* work = (ReceiveNetworkPacketWork*) param;
    
    while(true)
    {
        if(*work->network)
        {
            work->ReceiveData(*work->network);
        }
    }
}

internal void PlayGame(GameState* gameState, PlatformInput* input)
{
    SetGameMode(gameState, GameMode_Playing);
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
    
    result->table = PushStruct(&gameState->modePool, TaxonomyTable);
    result->oldTable = PushStruct(&gameState->modePool, TaxonomyTable);
    
    result->chunkDim = CHUNK_DIM;
    Assert(result->chunkDim % 2 == 0);
    u8 halfChunkDim = result->chunkDim / 2;
    result->voxelSide = VOXEL_SIZE;
    result->chunkSide = result->voxelSide * result->chunkDim;
    result->oneOverVoxelSide = 1.0f / result->voxelSide;
    result->oneOverChunkSide = 1.0f / result->chunkSide;
    
    
    result->UI = PushStruct(&gameState->modePool, UIState);
    
    result->particleCache = PushStruct(&gameState->modePool, ParticleCache, AlignClear(16));
    InitParticleCache(result->particleCache, gameState->assets);
    
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
    result->windSpeed = 1.0f;
    
    result->soundState = &gameState->soundState;
    
}

inline void InsertIntoEntityList(ClientEntity** nearestEntities, u32 maxEntityCount, r32* nearestCameraZ, r32 cameraZ, ClientEntity* entityC)
{
    u32 destIndex = maxEntityCount - 1;
    Assert(maxEntityCount > 1);
    for(i32 slideIndex = destIndex - 1; slideIndex >= 0; --slideIndex)
    {
        if(cameraZ < nearestCameraZ[slideIndex])
        {
            nearestEntities[slideIndex + 1] = nearestEntities[slideIndex];
            nearestCameraZ[slideIndex + 1] = nearestCameraZ[slideIndex];
            
            destIndex = slideIndex;
        }
        else
        {
            break;
        }
    }
    
    nearestEntities[destIndex] = entityC;
    nearestCameraZ[destIndex] = cameraZ;
}

internal void HandleClientPrediction(ClientEntity* entity, r32 timeToUpdate)
{
    ClientPrediction* prediction = &entity->prediction;
    prediction->timeLeft -= timeToUpdate;
    if(prediction->timeLeft <= 0)
    {
        prediction->type = Prediction_None;
    }
    switch(prediction->type)
    {
        case Prediction_None:
        {
            
        } break;
        
        case Prediction_EquipmentRemoved:
        {
            entity->equipment[prediction->slot.slot].ID = 0;
        } break;
        
        case Prediction_EquipmentAdded:
        {
            u64 ID = prediction->identifier;
            entity->equipment[prediction->slot.slot].ID = ID;
        } break;
        
        case Prediction_ActionBegan:
        {
            EntityAction currentAction = entity->action;
            if((currentAction == prediction->action) || (currentAction == Action_None))
            {
                entity->action = prediction->action;
            }
            else
            {
                prediction->type = Prediction_None;
            }
        } break;
    }
}

internal Vec3 MoveEntityClient(GameModeWorld* worldMode, ClientEntity* entity, r32 timeToAdvance, Vec3 acceleration, Vec3 velocity, Vec3* velocityToUpdate)
{
    Vec3 result = {};
    
    Vec3 deltaP = 0.5f * acceleration * Square(timeToAdvance) + velocity * timeToAdvance;
    r32 tRemaining = 1.0f;
    for(u32 iteration = 0; (iteration < 2) && tRemaining > 0; iteration++)
    {
        r32 tStop = tRemaining;
        Vec3 wallNormalMin = {};
        
        for(u32 entityIndex = 0; entityIndex < ArrayCount(worldMode->entities); ++entityIndex)
        {
            ClientEntity* entityToCheck = worldMode->entities[entityIndex];
            while(entityToCheck)
            {
                if(ShouldCollide(entity->identifier, entity->boundType, entityToCheck->identifier, entityToCheck->boundType))
                {
                    CheckCollisionCurrent ignored = {};
                    HandleVolumeCollision(entity->P, entity->bounds, deltaP, entityToCheck->P, entityToCheck->bounds, &tStop, &wallNormalMin, ignored);
                }
                
                entityToCheck = entityToCheck->next;
            }
        }
        
        Vec3 wallNormal = wallNormalMin;
        result += tStop * deltaP;
        
        *velocityToUpdate = *velocityToUpdate - Dot(entity->velocity, wallNormal) * wallNormal;
        deltaP = deltaP - Dot(deltaP, wallNormal) * wallNormal;
        tRemaining -= tStop * tRemaining;
    }
    
    return result;
}

inline b32 TooFarForAction(ClientPlayer* myPlayer, u32 desiredAction, u64 targetID)
{
    b32 result = false;
    if(desiredAction >= Action_Attack && targetID)
    {
        if(targetID == myPlayer->targetIdentifier && myPlayer->targetPossibleActions[desiredAction] == PossibleAction_TooFar)
        {
            result = true;
        }
    }
    
    return result;
}

inline b32 NearEnoughForAction(ClientPlayer* myPlayer, u32 desiredAction, u64 targetID, b32* resetAcceleration)
{
    b32 result = false;
    if(desiredAction >= Action_Attack)
    {
        if(targetID)
        {
            if(targetID == myPlayer->targetIdentifier && myPlayer->targetPossibleActions[desiredAction] == PossibleAction_CanBeDone)
            {
                *resetAcceleration = true;
                result = true;
            }
        }
    }
    else
    {
        if(desiredAction > Action_Move)
        {
            if(desiredAction == Action_Rolling)
            {
                *resetAcceleration = false;
            }
            else
            {
                *resetAcceleration = true;
            }
            result = true;
        }
    }
    
    return result;
}

internal b32 UpdateAndRenderGame(GameState* gameState, GameModeWorld* worldMode, RenderGroup* group, PlatformInput* input)
{
    
    Vec3 inputAcc = {};
    u64 targetEntityID = 0;
    u32 desiredAction = 0;
    u64 overlappingEntityID = 0;
    
    
    b32 result = false;
#if 0    
    if(Pressed(&input->backButton))
    {
        // TODO(Leonardo): if(situation allows to quit)
        // TODO(Leonardo): send quit request to server
        input->quitRequested = true;
    }
#endif
    
    ClientPlayer* myPlayer = &worldMode->player;
    UIState* UI = worldMode->UI;
    ReceiveNetworkPackets(worldMode);
    
#if FORGIVENESS_INTERNAL
    input->timeToAdvance = input->timeToAdvance * (gameState->timeCoeff / 100.0f);
#endif
    
    if(Pressed(&input->pauseButton))
    {
        worldMode->gamePaused = !worldMode->gamePaused;
        SendPauseToggleMessage();
    }
    
    worldMode->originalTimeToAdvance = input->timeToAdvance;
    if(worldMode->gamePaused)
    {
        input->timeToAdvance = 0;
    }
    
    worldMode->windTime += worldMode->windSpeed * input->timeToAdvance;
    
    ParticleCache* particleCache = worldMode->particleCache;
    
    Assert(input);
    Vec2 mouseP = V2(input->mouseX, input->mouseY);
    Vec2 dMouseP = mouseP - worldMode->lastMouseP;
    worldMode->lastMouseP = mouseP;
    
    
#if FORGIVENESS_INTERNAL
    if(input->altDown && IsDown(&input->mouseLeft))
    {
        r32 rotationSpeed = 0.001f * PI32;
        worldMode->debugCameraOrbit += rotationSpeed * dMouseP.x;
        worldMode->debugCameraPitch += rotationSpeed * dMouseP.y;
    }
    else if(input->altDown && IsDown(&input->mouseRight))
    {
        r32 zoomSpeed = (worldMode->debugCameraDolly) * 0.01f;
        worldMode->debugCameraDolly -= zoomSpeed * dMouseP.y;
    }
#endif
    
    worldMode->cameraPitch = 0.32f * PI32;
    worldMode->cameraDolly = 0.0f;
    worldMode->cameraOrbit = 0.0f;
    
    
    Vec2 finalXYOffset = worldMode->cameraWorldOffset.xy + UI->cameraOffset.xy;
    
    m4x4 cameraO = ZRotation(worldMode->cameraOrbit) * XRotation(worldMode->cameraPitch);
    Vec3 cameraOffsetFinal = cameraO * (V3(worldMode->cameraEntityOffset, worldMode->cameraWorldOffset.z + worldMode->cameraDolly + UI->cameraOffset.z) + V3(finalXYOffset, 0));
    
    SetCameraTransform(group, 0, 3.5f, GetColumn(cameraO, 0), GetColumn(cameraO, 1), GetColumn(cameraO, 2), cameraOffsetFinal, worldMode->cameraEntityOffset);
    
    if(input->altDown && Pressed(&input->mouseRight))
    {
        worldMode->useDebugCamera = !worldMode->useDebugCamera;
    }
    
    if(worldMode->useDebugCamera)
    {
        cameraO = ZRotation(worldMode->debugCameraOrbit) * XRotation(worldMode->debugCameraPitch);
        cameraOffsetFinal = cameraO * (V3(worldMode->cameraEntityOffset, worldMode->cameraWorldOffset.z + worldMode->debugCameraDolly));
        
        SetCameraTransform(group, Camera_Debug, 3.5f, GetColumn(cameraO, 0), GetColumn(cameraO, 1), GetColumn(cameraO, 2), cameraOffsetFinal + V3(worldMode->cameraWorldOffset.xy, 0), worldMode->cameraEntityOffset);
    }
    
    UpdateCamera(worldMode, input->timeToAdvance);
    
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
    
    r32 animationTimeElapsed = input->timeToAdvance;
#if FORGIVENESS_INTERNAL
    if(worldMode->fixedTimestep)
    {
        animationTimeElapsed = 0;
        if(worldMode->canAdvance)
        {
            worldMode->canAdvance = false;
            animationTimeElapsed = SERVER_MIN_MSEC_PER_FRAME * 1000.0f;
        }
    }
#endif
    
    
    
    Vec2 screenMouseP = V2(input->mouseX, input->mouseY);
    Vec3 unprojectedWorldMouseP = UnprojectAtZ(group, &group->gameCamera, screenMouseP, 0);
    
    Vec3 groundMouseP = ProjectOnGround(unprojectedWorldMouseP, group->gameCamera.P);
    worldMode->worldMouseP = groundMouseP;
    
    
    b32 reloadTaxonomyAutocompletes = false;
    b32 reloadAssetAutocompletes = false;
    if(myPlayer->identifier)
    {
        ClientEntity* player = GetEntityClient(worldMode, myPlayer->identifier);
        if(player)
        {
            char* filePath = "assets";
            if(worldMode->allDataFilesArrived)
            {
                if(worldMode->loadTaxonomies)
                {
                    platformAPI.DeleteFileWildcards(filePath, "*.fed");
                }
                
                WriteAllFiles(&worldMode->filePool, filePath, worldMode->firstDataFileArrived, false);
                worldMode->firstDataFileArrived = 0;
                
                if(worldMode->loadTaxonomies)
                {
                    TaxonomyTable* old = worldMode->oldTable;
                    
                    Clear(&old->pool);
                    ZeroStruct(*old);
                    
                    worldMode->oldTable = worldMode->table;
                    worldMode->table = old;
                    
                    ++worldMode->patchSectionArrived;
                    
                    
                    InitTaxonomyReadWrite(worldMode->table);
                    ReadTaxonomiesFromFile();
                    
                    ImportAllFiles(filePath, worldMode->editorRoles, false);
                    ImportAllAssetFiles(worldMode, filePath, &worldMode->filePool);
                    
                    platformAPI.DEBUGWriteFile("editorErrors", worldMode->table->errors, sizeof(worldMode->table->errors[0]) * worldMode->table->errorCount);
                    
                    
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
                    TranslateUI(worldMode->oldTable, worldMode->table, worldMode->UI);
                    
                    UI->editorTaxonomyTree = 0;
                    
                    TaxonomySlot* rootSlot = &worldMode->table->root;
                    UI->editorTaxonomyTree = BuildEditorTaxonomyTree(worldMode->editorRoles, worldMode->table, rootSlot);
                    
                    reloadTaxonomyAutocompletes = true;
                }
                worldMode->allDataFilesArrived = false;
            }
            
            if(worldMode->allPakFilesArrived)
            {
                worldMode->allPakFilesArrived = false;
                worldMode->UI->reloadingAssets = false;
                
                ++worldMode->patchSectionArrived;
                CloseAllHandles(gameState->assets);
                
                
                for(DataFileArrived* file = worldMode->firstPakFileArrived; file; file = file->next)
                {
                    MarkFileAsArrived(worldMode, file->name);
                }
                
                
                WriteAllFiles(&worldMode->filePool, filePath, worldMode->firstPakFileArrived, true);
                DeleteAllFilesNotArrived(worldMode, "assets");
                
                worldMode->firstPakFileArrived = 0;
                
                
                u32 destIndex = 0;
                if(gameState->assetsIndex == 0)
                {
                    destIndex = 1;
                }
                
                MemoryPool* toFree = gameState->assetsPool + destIndex;
                Clear(toFree);
                gameState->pingPongAssets[destIndex] = InitAssets(gameState, toFree, gameState->textureQueue, MegaBytes(256));
                
                gameState->assets = gameState->pingPongAssets[destIndex];
                gameState->assetsIndex = destIndex;
                
                
                
                Clear(&worldMode->filePool);
                worldMode->currentFile = 0;
                worldMode->UI->gameFont = {};
                worldMode->UI->editorFont = {};
                
                reloadAssetAutocompletes = true;
                MarkAllPakFilesAsToDelete(worldMode, "assets");
                
                
#if 1
                if(!gameState->music)
                {
                    RandomSequence seq = Seed(worldMode->worldSeed);
                    gameState->music = PlaySound(&gameState->soundState, gameState->assets, GetRandomSound(gameState->assets, Asset_music, &seq), 0.0f);
                }
#endif
            }
            
            group->assets = gameState->assets;
            
            player->identifier = myPlayer->identifier;
            player->targetID = myPlayer->targetIdentifier;
            player->P = V3(0, 0, 0);
            
            UI->output = {};
            b32 canRender = (worldMode->patchSectionArrived >= 2);
            if(canRender)
            {
                
                ResetUI(UI, worldMode, group, player, input, worldMode->cameraWorldOffset.z / worldMode->defaultCameraZ, reloadTaxonomyAutocompletes, reloadAssetAutocompletes);
                
                ClientEntity* nearestEntities[8] = {};
                r32 nearestCameraZ[ArrayCount(nearestEntities)];
                for(u32 index = 0; index < ArrayCount(nearestCameraZ); ++index)
                {
                    nearestCameraZ[index] = R32_MAX;
                }
                //
                //
                //UPDATE AND RENDER
                //
                //
                Clear(group, V4(0.0f, 0.0f, 0.0f, 1.0f));
                BeginDepthPeel(group);
                
                ResetLightGrid(worldMode);
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
                        ambientLightColor = V3(1, 0.73f, 1);
                    } break;
                    
                    case DayPhase_Morning:
                    {
                        ambientLightColor = V3(0.83f, 0.91f, 0.99f);
                    } break;
                    
                    case DayPhase_Day:
                    {
                        ambientLightColor = V3(1, 1, 1);
                    } break;
                    
                    case DayPhase_Sunset:
                    {
                        ambientLightColor = V3(0.96f, 0.54f, 0.74f);
                    } break;
                    
                    case DayPhase_Dusk:
                    {
                        ambientLightColor = V3(0.53f, 0.25f, 0.32f);
                    } break;
                    
                    case DayPhase_Night:
                    {
                        ambientLightColor = V3(0.02f, 0.02f, 0.1f);
                    } break;
                }
                
                
                
                
                
                r32 maxOverallDistanceSq = R32_MAX;
                u64 overallNearestID = 0;
                r32 overallMinCameraZ = R32_MAX;
                
                // NOTE(Leonardo): pre-render stuff
                // NOTE(Leonardo): add lights to the light grid
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
                            entity->actionTime += input->timeToAdvance;
                            TaxonomySlot* slot = GetSlotForTaxonomy(worldMode->table, entity->taxonomy);
                            if(!slot)
                            {
                                entity->timeFromLastUpdate = R32_MAX;
                            }
                            entity->slot = slot;
                            
                            Vec3 offset = {};
                            if(entity->identifier == player->identifier)
                            {
                                r32 maxDistanceForAccPrediction = 0.3f;
                                if(myPlayer->distanceCoeffFromServerP < maxDistanceForAccPrediction && input->timeToAdvance > 0)
                                {
                                    r32 accelerationCoeff = 1.0f;
                                    Vec3 acc = ComputeAcceleration(myPlayer->acceleration, myPlayer->velocity, DefaultMoveSpec(accelerationCoeff));
                                    myPlayer->velocity += acc * input->timeToAdvance;
                                    
                                    Vec3 velocity;
                                    if(myPlayer->distanceCoeffFromServerP < 0.15f && player->action == Action_None)
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
                                        offset = MoveEntityClient(worldMode, entity, input->timeToAdvance, acc, velocity, &myPlayer->velocity);
                                        
                                        if(LengthSq(myPlayer->acceleration) > Square(0.1f))
                                        {
                                            entity->action = Action_Move;
                                        }
                                    }
                                    else
                                    {
                                        offset = input->timeToAdvance * velocity;
                                        if(LengthSq(velocity) > Square(0.1f))
                                        {
                                            entity->action = Action_Move;
                                        }
                                        
                                    }
                                }
                                else
                                {
                                    offset = input->timeToAdvance * entity->velocity;
                                    if(entity->action == Action_Move && Abs(entity->velocity.x) > 0.1f)
                                    {
                                        entity->animation.flipOnYAxis = (offset.x < 0);
                                    }
                                }
                            }
                            else
                            {
                                offset = input->timeToAdvance * entity->velocity;
                                if(entity->action == Action_Move && Abs(entity->velocity.x) > 0.1f)
                                {
                                    entity->animation.flipOnYAxis = (offset.x < 0);
                                }
                            }
                            
                            entity->universeP = Offset(entity->universeP, offset.xy);
                            if(myPlayer->changedWorld)
                            {
                                entity->universeP.chunkX += myPlayer->changedWorldDeltaX;
                                entity->universeP.chunkY += myPlayer->changedWorldDeltaY;
                            }
                            
                            
                            entity->modulationWithFocusColor = 0;
                            
                            HandleClientPrediction(entity, input->timeToAdvance);
                            UpdateAnimationEffects(worldMode, entity, input->timeToAdvance);
                            
                            if(entity->identifier && !IsSet(entity, Flag_deleted))
                            {
                                if(slot->lightIntensity)
                                {
                                    u32 lightIndex = PushPointLight(group, entity->P, slot->lightColor, slot->lightIntensity);
                                    AddLightToGrid(worldMode, entity->P, lightIndex);
                                }
                            }
                            
                            if(entity->identifier && !IsSet(entity, Flag_deleted | Flag_Equipped))
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
                                    overallNearestID = entity->identifier;
                                }
                                
                                if(PointInRect(screenBounds, screenMouseP))
                                {
                                    InsertIntoEntityList(nearestEntities, ArrayCount(nearestEntities), nearestCameraZ, cameraZ, entity);
                                    
                                }
                            }
                        }
                        
                        entity = entity->next;
                    }
                    
                }
                
                DEBUG_VALUE(worldMode->modulationWithFocusColor);
                
                
                UIHandle(UI, input, screenMouseP, nearestEntities, ArrayCount(nearestEntities));
                UIOutput output = UI->output;
                
                if(TooFarForAction(myPlayer, output.desiredAction, output.targetEntityID))
				{
                    ClientEntity* target = GetEntityClient(worldMode, output.targetEntityID);
                    Assert(target);
                    output.inputAcc = target->P - player->P;
					//ApplyCollisionAvoidance();
				}
                else
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
                
                myPlayer->acceleration = output.inputAcc;
                
                
                SetCameraTransform(group, 0, 3.5f, GetColumn(cameraO, 0), GetColumn(cameraO, 1), GetColumn(cameraO, 2), cameraOffsetFinal + V3(worldMode->cameraWorldOffset.xy, 0), worldMode->cameraEntityOffset);
                
                MoveTowards(worldMode, player, UI->additionalCameraOffset, V2(0, 0), UI->zoomLevel);
                PushAmbientColor(group, ambientLightColor);
                
                for(u32 entityIndex = 0; 
                    entityIndex < ArrayCount(worldMode->entities); 
                    entityIndex++)
                {
                    ClientEntity* entity = worldMode->entities[entityIndex];
                    while(entity)
                    {
                        ClientEntity* next = entity->next;
                        if(entity->identifier && !IsSet(entity, Flag_deleted | Flag_Equipped))
                        {
                            entity->P = Subtract(entity->universeP, player->universeP);
                            entity->timeFromLastUpdate += input->timeToAdvance;
                            if(entity->timeFromLastUpdate >= 3.0f)
                            {
                                DeleteEntityClient(worldMode, entity);
                            }
                            else
                            {
                                AnimationEntityParams params = StandardEntityParams();
                                if(UI->mode != UIMode_None && entity->identifier == myPlayer->openedContainerID)
                                {
                                    params = ContainerEntityParams();
                                }
                                
                                if(UI->mode == UIMode_Loot && entity->identifier == myPlayer->identifier)
                                {
                                    ClientEntity* container = GetEntityClient(worldMode, myPlayer->openedContainerID);
                                    if(container && container->P.y > 0)
                                    {
                                        params.transparent = true;
                                    }
                                }
                                
                                entity->animation.output = RenderEntity(group, worldMode, entity, animationTimeElapsed, params);
                                
                                entity->animation.spawnAshParticlesCount = 0;
                                
                                if(entity->animation.output.entityPresent && entity->targetID)
                                {
                                    // NOTE(Leonardo): render target entity here at specified angle and offset
                                    ClientEntity* targetEntity = GetEntityClient(worldMode, entity->targetID);
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
                        }
                        
                        entity = next;
                    }
                }
                
                
                b32 forceVoronoiRegeneration = myPlayer->changedWorld || output.forceVoronoiRegeneration;
                if(myPlayer->changedWorld)
                {
                    myPlayer->oldUniverseP.chunkX += myPlayer->changedWorldDeltaX;
                    myPlayer->oldUniverseP.chunkY += myPlayer->changedWorldDeltaY;
                    
                    myPlayer->changedWorld = false;
                    myPlayer->changedWorldDeltaX = 0;
                    myPlayer->changedWorldDeltaY = 0;
                }
                
                myPlayer->universeP = player->universeP;
                Vec3 deltaP = -Subtract(myPlayer->universeP, myPlayer->oldUniverseP);
                
                Vec3 particleDelta = deltaP;
                if(LengthSq(particleDelta) > Square(100.0f))
                {
                    particleDelta = {};
                }
                particleCache->deltaParticleP = particleDelta;
                UI->deltaMouseP += deltaP;
                
                for(u32 voronoiIndex = 0; voronoiIndex < ArrayCount(worldMode->voronoiPingPong); ++voronoiIndex)
                {
                    worldMode->voronoiPingPong[voronoiIndex].deltaP += deltaP;
                }
                
                UniversePos voronoiP = player->universeP;
                
                b32 changedChunk = (voronoiP.chunkX != myPlayer->oldVoronoiP.chunkX || voronoiP.chunkY != myPlayer->oldVoronoiP.chunkY);
                
                myPlayer->oldVoronoiP = voronoiP;
                myPlayer->oldUniverseP = myPlayer->universeP;
                
                i32 lateralChunkSpan = SERVER_REGION_SPAN * SIM_REGION_CHUNK_SPAN;
                i32 chunkApron = UI->chunkApron;
                i32 originChunkX = voronoiP.chunkX;
                i32 originChunkY = voronoiP.chunkY;
                
                u32 seed = worldMode->worldSeed;
                
                
                WorldGenerator* generator = 0;
                
                RandomSequence generatorSeq = Seed(seed);
                u32 generatorTaxonomy = GetRandomChild(worldMode->table, &generatorSeq, worldMode->table->generatorTaxonomy);
                
                if(generatorTaxonomy != worldMode->table->generatorTaxonomy)
                {
                    generator = GetSlotForTaxonomy(worldMode->table, generatorTaxonomy)->generator;
                }
                
                for(i32 Y = originChunkY - chunkApron; Y <= originChunkY + chunkApron; Y++)
                {
                    for(i32 X = originChunkX - chunkApron; X <= originChunkX + chunkApron; X++)
                    {
                        i32 chunkX = Wrap(0, X, lateralChunkSpan);
                        i32 chunkY = Wrap(0, Y, lateralChunkSpan);
                        
                        if(ChunkValid(lateralChunkSpan, X, Y))
                        {	
                            WorldChunk* chunk = GetChunk(worldMode->chunks, ArrayCount(worldMode->chunks), X, Y, &worldMode->chunkPool);
                            
                            if(!chunk->initialized)
                            {
                                forceVoronoiRegeneration = true;
                                BuildChunk(worldMode->table, generator, chunk, X, Y, seed);
                            }
                            
                            
                            r32 waterSpeed = 0.12f;
                            r32 waterSineSpeed = 70.0f;
                            for(u32 tileY = 0; tileY < CHUNK_DIM; ++tileY)
                            {
                                for(u32 tileX = 0; tileX < CHUNK_DIM; ++tileX)
                                {
                                    WorldTile* tile = GetTile(chunk, tileX, tileY);
                                    if(tile->movingNegative)
                                    {
                                        tile->waterPhase -= waterSpeed * input->timeToAdvance;
                                        if(tile->waterPhase < 0)
                                        {
                                            tile->waterPhase = 0;
                                            tile->movingNegative = false;
                                        }
                                    }
                                    else
                                    {
                                        tile->waterPhase += waterSpeed * input->timeToAdvance;
                                        if(tile->waterPhase > 1.0f)
                                        {
                                            tile->waterPhase = 1.0f;
                                            tile->movingNegative = true;
                                        }
                                    }
                                    
                                    RandomSequence seq = tile->waterSeq;
                                    NoiseParams waterParams = NoisePar(4.0f, 2, 0.0f, 1.0f);
                                    r32 blueNoise = Evaluate(tile->waterPhase, 0, waterParams, GetNextUInt32(&seq));
                                    r32 alphaNoise = Evaluate(tile->waterPhase, 0, waterParams, GetNextUInt32(&seq));
                                    tile->blueNoise = UnilateralToBilateral(blueNoise);
                                    tile->alphaNoise = UnilateralToBilateral(alphaNoise);
                                    
                                    
                                    tile->waterSine += waterSineSpeed * input->timeToAdvance;
                                }
                            }
                        }
                    }
                }
                
                
                
                if(changedChunk || !worldMode->activeDiagram || forceVoronoiRegeneration)
                {
                    if(!worldMode->generatingVoronoi)
                    {
                        GenerateVoronoi(gameState,worldMode, voronoiP, originChunkX, originChunkY, chunkApron, lateralChunkSpan);
                    }
                }
                
                
                if(UI->groundViewMode == GroundView_Tile || UI->groundViewMode == GroundView_Chunk)
                {
                    for(u32 chunkIndex = 0; chunkIndex < ArrayCount(worldMode->chunks); ++chunkIndex)
                    {
                        WorldChunk* chunk = worldMode->chunks[chunkIndex];
                        while(chunk)
                        {
                            if(chunk->initialized && !ChunkOutsideWorld(lateralChunkSpan, chunk->worldX, chunk->worldY))
                            {
                                r32 chunkSide = worldMode->chunkSide;
                                
                                Vec3 chunkLowLeftCornerOffset = V3(V2i(chunk->worldX - originChunkX, chunk->worldY - originChunkY), 0.0f) * chunkSide - player->universeP.chunkOffset;
                                
                                if(UI->groundViewMode == GroundView_Chunk)
                                {
                                    ObjectTransform chunkTransform = FlatTransform();
                                    Rect2 rect = RectMinDim(chunkLowLeftCornerOffset.xy, V2(chunkSide, chunkSide));
                                    
                                    Vec4 chunkColor = ComputeWeightedChunkColor(chunk);
                                    if(ChunkOutsideWorld(lateralChunkSpan, chunk->worldX, chunk->worldY))
                                    {
                                        chunkColor = V4(1, 0, 0, 1);
                                    }
                                    
                                    
                                    PushRect(group, chunkTransform, rect, chunkColor);
                                    
                                    if(UI->showGroundOutline)
                                    {
                                        ObjectTransform chunkOutlineTransform = FlatTransform(0.01f);
                                        PushRectOutline(group, chunkTransform, rect, V4(1, 1, 1, 1), 0.1f);
                                    }
                                }
                                else
                                {
                                    for(u8 Y = 0; Y < CHUNK_DIM; ++Y)
                                    {
                                        for(u8 X = 0; X < CHUNK_DIM; ++X)
                                        {
                                            Vec3 tileMin = chunkLowLeftCornerOffset + V3(X * worldMode->voxelSide, Y * worldMode->voxelSide, 0);
                                            ObjectTransform tileTransform = FlatTransform();
                                            Rect2 rect = RectMinDim(tileMin.xy, V2(worldMode->voxelSide, worldMode->voxelSide));
                                            
                                            WorldTile* tile = GetTile(chunk, X, Y);
                                            
                                            RandomSequence seq = Seed(0);
                                            Vec4 tileColor = GetTileColor(tile, UI->uniformGroundColors, &seq);
                                            PushRect(group, tileTransform, rect, tileColor);
                                            
                                            if(UI->showGroundOutline)
                                            {
                                                PushRectOutline(group, tileTransform, rect, V4(1, 1, 1, 1), 0.1f);
                                            }
                                            
                                            
                                            r32 waterLevel = tile->waterLevel;
                                            if(waterLevel < WATER_LEVEL)
                                            {
                                                Vec4 waterColor = V4(0, 0, waterLevel, 1 - waterLevel);
                                                PushRect(group, tileTransform, rect, waterColor);
                                            }
                                        }
                                    }
                                }
                            }
                            
                            chunk = chunk->next;
                        }
                    }
                    
                }
                else
                {
                    r32 rippleThreesold = 0.78f;
                    r32 waterRandomPercentage = 0.002f;
                    r32 ripplesLifetime = 3.0f;
                    
                    
                    
                    ForgVoronoiDiagram* voronoi = worldMode->activeDiagram;
                    if(voronoi)
                    {
                        TempMemory voronoiMemory = BeginTemporaryMemory(worldMode->temporaryPool);
                        BEGIN_BLOCK("voronoi sites");
                        
                        jcv_site* sites = jcv_diagram_get_sites(&voronoi->diagram);
                        for(int i = 0; i < voronoi->diagram.numsites; ++i)
                        {
                            jcv_site* site = sites + i;
                            Vec2 siteP = V2(site->p.x, site->p.y);
							if(!site->tile)
							{
								site->tile = GetTile(worldMode, voronoi->originP, siteP);
							}
                            
                            
                            WorldTile* tile = site->tile;
                            if(tile->waterLevel < rippleThreesold * WATER_LEVEL && RandomUni(&worldMode->waterRipplesSequence) < waterRandomPercentage)
                            {
                                Vec3 ripplesP = V3(siteP + voronoi->deltaP.xy, tile->height);
                                //SpawnWaterRipples(particleCache, ripplesP, V3(0, 0, 0), ripplesLifetime);
                            }
                        }
                        
                        END_BLOCK();
                        
                        
                        BEGIN_BLOCK("edge rendering");
                        jcv_edge* edge = jcv_diagram_get_edges(&voronoi->diagram);
                        
                        jcv_edge* toRender = edge;
                        u32 counter = 0;
                        u32 waterCounter = 0;
                        while(edge)
                        {
                            Vec2 offsetFrom = V2(edge->pos[0].x, edge->pos[0].y);
                            Vec2 offsetTo = V2(edge->pos[1].x, edge->pos[1].y);
                            
                            if(Length(offsetTo - offsetFrom) > 100.0f)
                            {
                                int a = 5;
                            }
							if(!edge->tile[0])
							{
                                edge->tile[0] = GetTile(worldMode, voronoi->originP, offsetFrom);
							}
                            
                            if(edge->sites[0]->tile->waterLevel < WATER_LEVEL)
                            {
                                ++waterCounter;
                            }
                            
							if(!edge->tile[1])
							{
                                edge->tile[1] = GetTile(worldMode, voronoi->originP, offsetTo);
							}
                            
                            if(edge->sites[1]->tile->waterLevel < WATER_LEVEL)
                            {
                                ++waterCounter;
                            }
                            
                            if(UI->showGroundOutline)
                            {
                                Vec4 offsetFromCamera = V4(offsetFrom + voronoi->deltaP.xy, edge->tile[0]->height, 0);
                                Vec4 offsetToCamera = V4(offsetTo + voronoi->deltaP.xy, edge->tile[1]->height, 0);
                                
                                PushLine(group, V4(1, 1, 1, 1), offsetFromCamera.xyz, offsetToCamera.xyz, 0.02f);
                            }
                            
                            
                            edge = edge->next;
                            if(++counter == 512 || !edge)
                            {
                                
#if 1         
                                RenderVoronoiWork* work = PushStruct(worldMode->temporaryPool, RenderVoronoiWork);
                                work->group = group;
                                work->voronoi = voronoi;
                                work->edges = toRender;
                                work->edgeCount = counter;
                                // NOTE(Leonardo): 2 standard and 2 water
                                work->triangleVertexes = ReserveTriangles(group, counter * 2 + waterCounter);
                                // NOTE(Leonardo): 2 standard
                                work->quadVertexes = ReserveQuads(group, counter * 2);
                                
#if 1                                
                                platformAPI.PushWork(gameState->renderQueue, RenderVoronoiEdges, work);
#else
                                RenderVoronoiEdges(work);
#endif
#endif
                                
                                toRender = edge;
                                counter = 0;
                                waterCounter = 0;
                            }
                        }
                        
                        platformAPI.CompleteQueueWork(gameState->renderQueue);
                        EndTemporaryMemory(voronoiMemory);
                        END_BLOCK();
                    }
                }
                
                BEGIN_BLOCK("particles");
                SetParticleCacheBitmaps(particleCache, group->assets);
                UpdateAndRenderParticleSystems(worldMode, particleCache, input->timeToAdvance, group);
                END_BLOCK();
                
                if(UI->mode == UIMode_Equipment || UI->mode == UIMode_Loot)
                {
                    UIOverdrawInventoryView(UI);
                }
                
                EndDepthPeel(group);
                
                
                
                if(UI->mode == UIMode_Loot)
                {
                    ClientEntity* lootingEntity = GetEntityClient(worldMode, myPlayer->openedContainerID);
                    Assert(lootingEntity);
                    r32 additionalZoomCoeff = Max(1.0f, lootingEntity->animation.output.additionalZoomCoeff);
                    MoveTowards(worldMode, lootingEntity, V2(0, 0), V2(0, 0), 3.0f * additionalZoomCoeff);
                }
                r32 focusZoom = UI->zoomLevel;
                if(ActionRequiresZooming(player->action, &focusZoom))
                {
                    ClientEntity* entityC = GetEntityClient(worldMode, output.targetEntityID);
                    Assert(entityC);
                    MoveTowards(worldMode, entityC, V2(0, 0), V2(0, 0), focusZoom);
                }
                
                if(!output.overlappingEntityID)
                {
                    if(overallNearestID != player->identifier)
                    {
                        output.overlappingEntityID = overallNearestID;
                    }
                }
                
                
                if(output.targetEntityID)
                {
                    ClientEntity* target = GetEntityClient(worldMode, output.targetEntityID);
                    if(target)
                    {
                        player->animation.flipOnYAxis = (target->P.x < 0);
                    }
                }
                
                inputAcc = output.inputAcc;
                targetEntityID = output.targetEntityID;
                desiredAction = output.desiredAction;
                overlappingEntityID = output.overlappingEntityID;
                
                
                //Rect3 worldCameraBounds = GetScreenBoundsAtTargetDistance(group);
                //Rect2 screenBounds = RectCenterDim(V2(0, 0), V2(worldCameraBounds.max.x - worldCameraBounds.min.x, worldCameraBounds.max.y - worldCameraBounds.min.y));
                //PushRectOutline(group, FlatTransform(), screenBounds, V4(1.0f, 0.0f, 0.0f, 1.0f), 0.1f); 
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
        
        gameState->assetsIndex = 0;
        MemoryPool* pool = gameState->assetsPool + 0;
        gameState->pingPongAssets[0] = InitAssets(gameState, pool, gameState->textureQueue, MegaBytes(256));
        gameState->assets = gameState->pingPongAssets[0];
        
        
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
                    ChangeVolume(&gameState->soundState, gameState->music, 1.0f, V2(0.0f, 0.0f));
                    gameState->music = 0;
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

