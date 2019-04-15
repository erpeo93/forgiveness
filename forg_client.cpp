#include "forg_client.h"
global_variable ClientPlayer* myPlayer; 
#include "client_generated.h"
#include "forg_sort.cpp"
#include "forg_token.cpp"
#include "forg_meta.cpp"
#include "forg_pool.cpp"
#include "forg_world.cpp"
#include "forg_taxonomy.cpp"
#include "forg_editor.cpp"
#include "forg_inventory.cpp"
#include "forg_physics.cpp"
#include "forg_rule.cpp"
#include "forg_world_generation.cpp"

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
    }
    
    return result;
}

internal void FreePlant(GameModeWorld* worldMode, PlantSegment* root)
{
    for(u32 childIndex = 0; childIndex < PlantChild_Count; ++childIndex)
    {
        if(root->childs[childIndex])
        {
            FreePlant(worldMode, root->childs[childIndex]);
            root->childs[childIndex] = 0;
        }
    }
    
    root->nextFree = worldMode->firstFreePlantSegment;
    worldMode->firstFreePlantSegment = root;
}

inline void DeleteEntityClient(GameModeWorld* worldMode, ClientEntity* entity)
{
    u32 taxonomy = entity->taxonomy;
    Assert(taxonomy);
    if(IsRock(worldMode->table, taxonomy))
    {
        ClientRock* toFree = entity->rock;
        if(toFree)
        {
            toFree->nextFree = worldMode->firstFreeRock;
            worldMode->firstFreeRock = toFree;
            entity->rock = 0;
        }
    }
    else if(IsPlant(worldMode->table, taxonomy))
    {
        ClientPlant* toFree = entity->plant;
        if(toFree)
        {
            Assert(toFree->futureRoot);
            FreePlant(worldMode, toFree->root);
            FreePlant(worldMode, toFree->futureRoot);
            
            toFree->nextFree = worldMode->firstFreePlant;
            worldMode->firstFreePlant = toFree;
            entity->plant = 0;
        }
        
    }
    
    AddFlags(entity, Flag_deleted);
    
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
        FREELIST_DEALLOC(effect, worldMode->firstFreeEffect);
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
    cameraEntityOffset += GetCenter(entityC->animation.bounds);
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
                    chunk->lightCount[tileY][tileX] = 0;
                    chunk->lightIndexes[tileY][tileX] = V4(-1, -1, -1, -1);
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
                
                if(query.chunk->lightCount[tileY][tileX] < 4)
                {
                    r32 lightIndexReal = (r32) index;
                    Vec4* lightIndexes = (Vec4*) query.chunk->lightIndexes + (tileY * CHUNK_DIM + tileX);
                    
                    
                    if((lightIndexes->x != lightIndexReal) &&
                       (lightIndexes->y != lightIndexReal) &&
                       (lightIndexes->z != lightIndexReal) &&
                       (lightIndexes->w != lightIndexReal))
                    {
                        u8 lightIndex = query.chunk->lightCount[tileY][tileX];
                        query.chunk->lightCount[tileY][tileX] = lightIndex + 1;
                        
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
        result = query.chunk->lightIndexes[query.tileY][query.tileX];
    }
    
    return result;
}

inline TileInfo GetTileInfo(GameModeWorld* worldMode, UniversePos baseP, Vec2 P)
{
    TileInfo result = {};
    GetUniversePosQuery query = TranslateRelativePos(worldMode, baseP, P);
    
    if(query.chunk)
    {
        u8 tileX = SafeTruncateToU8(query.tileX);
        u8 tileY = SafeTruncateToU8(query.tileY);
        
        TileGenerationData* tileData = &query.chunk->tileData[tileY][tileX];
        result.color = GetTileColor(worldMode->table, query.chunk, query.tileX, query.tileY, query.chunkOffset);
        result.height = tileData->height;
        result.lightIndexes = query.chunk->lightIndexes[tileY][tileX];
        
        TaxonomySlot* slot = GetSlotForTaxonomy(worldMode->table, tileData->biomeTaxonomy);
        result.chunkyness = slot->chunkyness;
    }
    
    return result;
}

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
    
    result->editorRoles = gameState->editorRoles;
    myPlayer = &result->player;
    
    u32 sendBufferSize = KiloBytes(4);
    u32 recvBufferSize = MegaBytes(16);
    
    myPlayer->network = input->network;
#if 0
    char* loginServer = "forgiveness.hopto.org";
#else
    char* loginServer = "127.0.0.1";
#endif
    
    platformAPI.net.OpenConnection(myPlayer->network, loginServer, LOGIN_PORT);
    myPlayer->nextSendUnreliableApplicationData = {};
    myPlayer->nextSendReliableApplicationData = {};
    ResetReceiver(&myPlayer->receiver);
    
    LoginRequest(4444);
    
    
    gameState->receivePacketWork.network = &myPlayer->network;
    gameState->receivePacketWork.ReceiveData = platformAPI.net.ReceiveData;
    
    platformAPI.PushWork(gameState->slowQueue, ReceiveNetworkPackets, &gameState->receivePacketWork);
    
    
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
    
    
    ColoredVertex vertexes[] = 
    {
        {V3(0.5773502692f, 0.5773502692f, 0.5773502692f), V4(1, 0, 0, 1)},
        {V3(-0.5773502692f, 0.5773502692f, -0.5773502692f), V4(0, 1, 1, 1)},
        {V3(0.5773502692f, -0.5773502692f, -0.5773502692f), V4(1, 1, 0, 1)},
        {V3(-0.5773502692f, -0.5773502692f, 0.5773502692f), V4(1, 1, 1, 1)},
    };
    
    ModelFace faces[] =
    {
        {0, 2, 1},
        {1, 2, 3},
        {0, 3, 2},
        {0, 1, 3}
    };
    
    result->tetraModel = CreateModel(&gameState->modePool, vertexes, ArrayCount(vertexes), faces, ArrayCount(faces));
    
    result->firstFreeRock = 0;
    result->firstFreePlantSegment = 0;
    
    
    result->dayPhases[DayPhase_Day].duration = 60.0f;
    result->dayPhases[DayPhase_Day].ambientLightColor = V3(0.9f, 0.88f, 1.0f);
    result->dayPhases[DayPhase_Day].next = DayPhase_Sunset;
    
    result->dayPhases[DayPhase_Sunset].duration = 30.0f;
    result->dayPhases[DayPhase_Sunset].ambientLightColor = V3(1.0f, 0.8f, 0.8f);
    result->dayPhases[DayPhase_Sunset].next = DayPhase_Night;
    
    result->dayPhases[DayPhase_Night].duration = 40.0f;
    result->dayPhases[DayPhase_Night].ambientLightColor = V3(0.0f, 0.01f, 0.15f);
    result->dayPhases[DayPhase_Night].next = DayPhase_Day;
    
    result->currentPhase = DayPhase_Sunset;
    
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


global_variable r32 globalSine = 0;
global_variable r32 global_cameraZStep = 6.0f;
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
    
    myPlayer = &worldMode->player;
    UIState* UI = worldMode->UI;
    ReceiveNetworkPackets(worldMode);
    
#if FORGIVENESS_INTERNAL
    myPlayer->networkTimeElapsed += input->timeToAdvance;
    r32 receivingBytesPerSec = myPlayer->network->totalBytesReceived / myPlayer->networkTimeElapsed;
    DEBUG_VALUE(receivingBytesPerSec);
    DEBUG_VALUE(gameState->timeCoeff);
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
    
    
    globalSine += input->timeToAdvance * 2.0f;
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
    
    m4x4 cameraO = ZRotation(worldMode->cameraOrbit) * XRotation(worldMode->cameraPitch);
    Vec3 cameraOffsetFinal = cameraO * (V3(worldMode->cameraEntityOffset, worldMode->cameraWorldOffset.z + worldMode->cameraDolly));
    
    SetCameraTransform(group, 0, 3.5f, GetColumn(cameraO, 0), GetColumn(cameraO, 1), GetColumn(cameraO, 2), cameraOffsetFinal + V3(worldMode->cameraWorldOffset.xy, 0), worldMode->cameraEntityOffset);
    
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
    
#if 0    
    if(Pressed(&input->actionDown))
    {
        b32 fixedTimestep = worldMode->fixedTimestep;
        if(input->altDown)
        {
            fixedTimestep = !fixedTimestep;
        }
        //DebugFixedTimestep(fixedTimestep, true);
        gameState->editorMode = !gameState->editorMode;
    }
#endif
    
    if(Pressed(&input->debugButton1))
    {
        SendInputRecordingMessage(true, true);
    }
    
    if(Pressed(&input->debugButton2))
    {
        SendInputRecordingMessage(false, false);
    }
#endif
    r32 cameraZStep = global_cameraZStep;
#if 0
    if(IsDown(&input->actionLeft))
    {
        worldMode->cameraOffset.z += cameraZStep * input->timeToAdvance;
    }
    else if(IsDown(&input->actionRight))
    {
        worldMode->cameraOffset.z -= cameraZStep * input->timeToAdvance;
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
    
    if(myPlayer->identifier)
    {
        ClientEntity* player = GetEntityClient(worldMode, myPlayer->identifier);
        if(player)
        {
            char* filePath = "assets";
            if(worldMode->allDataFilesArrived)
            {
                
#if 0                
                if(worldMode->loadTaxonomies)
                {
                    platformAPI.DeleteFileWildcards(filePath, "*.fed");
                }
#endif
                
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
                    ReadPlantChart();
                    InitializeWorldGenerator(worldMode->table, &worldMode->generator, 0, 0);
                    
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
                }
                worldMode->allDataFilesArrived = false;
            }
            
            if(worldMode->allPakFilesArrived)
            {
                worldMode->allPakFilesArrived = false;
                worldMode->UI->reloadingAssets = false;
                
                ++worldMode->patchSectionArrived;
                CloseAllHandles(gameState->assets);
                
                WriteAllFiles(&worldMode->filePool, filePath, worldMode->firstPakFileArrived, true);
                
                worldMode->firstPakFileArrived = 0;
                
                
                Clear(&gameState->assetPool);
                gameState->assets = InitAssets(gameState, gameState->textureQueue, MegaBytes(32));
                group->assets = gameState->assets;
                
                Clear(&worldMode->filePool);
                worldMode->currentFile = 0;
                
                worldMode->UI->font = 0;
                
                
#if 1
                gameState->music = PlaySound(&gameState->soundState, gameState->assets, GetFirstSound(gameState->assets, Asset_music), 0.0f);
                ChangeVolume(&gameState->soundState, gameState->music, 1000.0f, V2(1.0f, 1.0f));
#endif
                
            }
            
            b32 canRender = (worldMode->patchSectionArrived >= 2);
            
            if(canRender)
            {
                player->identifier = myPlayer->identifier;
                player->targetID = myPlayer->targetIdentifier;
                player->P = V3(0, 0, 0);
                
                ResetUI(UI, worldMode, group, player, input, worldMode->cameraWorldOffset.z / worldMode->defaultCameraZ);
                
                
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
                
                
                
                r32 maxOverallDistanceSq = 4.0f;
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
                                if(myPlayer->distanceCoeffFromServerP < 0.3f && input->timeToAdvance > 0)
                                {
                                    Vec3 acc = ComputeAcceleration(myPlayer->acceleration, myPlayer->velocity, DefaultMoveSpec());
                                    myPlayer->velocity += acc * input->timeToAdvance;
                                    
                                    Vec3 velocity;
                                    if(myPlayer->distanceCoeffFromServerP < 0.15f && player->action == Action_None)
                                    {
                                        velocity = myPlayer->velocity;
                                    }
                                    else
                                    {
                                        velocity = Lerp(myPlayer->velocity, myPlayer->distanceCoeffFromServerP, entity->velocity);
                                    }
                                    
                                    if(Abs(myPlayer->acceleration.x) > 0.1f)
                                    {
                                        entity->animation.flipOnYAxis = (myPlayer->acceleration.x < 0);
                                    }
                                    
                                    if(myPlayer->distanceCoeffFromServerP < 0.2f)
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
                                if(IsRock(worldMode->table, entity->taxonomy) || IsPlant(worldMode->table, entity->taxonomy))
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
                                if(distanceSq < maxOverallDistanceSq)
                                {
                                    if(cameraZ < overallMinCameraZ)
                                    {
                                        overallMinCameraZ = cameraZ;
                                        overallNearestID = entity->identifier;
                                    }
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
                
                UIOutput output = {};
                output = UIHandle(UI, input, screenMouseP, nearestEntities, ArrayCount(nearestEntities));
                
                SetCameraTransform(group, 0, 3.5f, GetColumn(cameraO, 0), GetColumn(cameraO, 1), GetColumn(cameraO, 2), cameraOffsetFinal + V3(worldMode->cameraWorldOffset.xy, 0), worldMode->cameraEntityOffset);
                
                myPlayer->acceleration = output.inputAcc;
                
                MoveTowards(worldMode, player, UI->additionalCameraOffset, V2(0, 0), UI->zoomLevel);
                PushAmbientColor(group, ambientLightColor);
                
                for(u32 entityIndex = 0; 
                    entityIndex < ArrayCount(worldMode->entities); 
                    entityIndex++)
                {
                    ClientEntity* entity = worldMode->entities[entityIndex];
                    while(entity)
                    {
                        if(entity->identifier && !IsSet(entity, Flag_deleted | Flag_Equipped))
                        {
                            entity->P = Subtract(entity->universeP, player->universeP);
                            entity->timeFromLastUpdate += input->timeToAdvance;
                            if(entity->timeFromLastUpdate >= 8.0f)
                            {
                                AddFlags(entity, Flag_deleted);
                                DeleteEntityClient(worldMode, entity);
                            }
                            else
                            {
#if 0                    
                                if(DEBUG_UI_ENABLED)
                                {
                                    SimEntity* hotEntity = GetEntityClient(worldMode, entity->globalID, entity->worldID);
                                    DebugID entityDebugID =  DEBUG_POINTER_ID(hotEntity);
                                    if(entity->body->solid)
                                    {
                                        CollVolume* volume = &entity->body->volume;
                                        Rect2 groundRect = RectCenterDim(finalP.xy + volume->offset.xy, volume->dim.xy);
                                        
                                        if(PointInRect(groundRect, worldMouseP))
                                        {
                                            DEBUG_HIT(entityDebugID);
                                        }
                                        
                                        Vec4 outlineColor = V4(1.0f, 0.0f, 1.0f, 1.0f);
                                        if(DEBUG_HIGHLIGHTED(entityDebugID, &outlineColor))
                                        {
                                            PushRectOutline(group, DefaultFlatTransform(), finalP, volume->dim.xy, outlineColor, 0.05f);
                                        }
                                        
                                    }
                                    
                                    if(DEBUG_REQUESTED(entityDebugID))
                                    {
                                        DEBUG_DATA_BLOCK("simulation/Entity");
                                        
                                        
                                        //DebugAskForAllAttributes(hotEntity);
                                    }
                                    
                                    if(DEBUG_EDITING(entityDebugID))
                                    {
                                        //SetHover(hotEntity);
                                        //DebugAskForAIInfo(hotEntity);
                                        
                                        if(Pressed(&input->debugCancButton))
                                        {
                                            //DebugDeleteEntity(hotEntity);
                                            SetHover(0);
                                            DEBUG_RESET_EDITING();
                                        }
                                        else
                                        {
                                            //DebugAskForAllAttributes(hotEntity);
                                        }
                                    }
                                    
                                    Vec2 screenP = entity->regionPosition.xy + V3(0.0f, 2.0f, 0.0f).xy;
                                    if(entity->action)
                                    {
                                        DEBUG_TEXT(MetaTable_EntityAction[entity->action], screenP, V4(1.0f, 1.0f, 1.0f, 1.0f));
                                    }
                                }
#endif
                                
                                //UIHighlighted(group, DefaultUprightTransform(), UI, entity, finalP, destEntity, targetEnemy);
                                
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
                        
                        entity = entity->next;
                    }
                }
                
                b32 forceVoronoiRegeneration = myPlayer->changedWorld || output.forceVoronoiRegeneration;
                forceVoronoiRegeneration = true;
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
                worldMode->voronoiDelta += deltaP;
                
                UniversePos voronoiP = player->universeP;
                
                b32 changedChunk = (voronoiP.chunkX != myPlayer->oldVoronoiP.chunkX || voronoiP.chunkY != myPlayer->oldVoronoiP.chunkY);
                
                myPlayer->oldVoronoiP = voronoiP;
                myPlayer->oldUniverseP = myPlayer->universeP;
                
                
                if(changedChunk || !worldMode->voronoiGenerated || forceVoronoiRegeneration)
                {
                    worldMode->voronoiDelta = {};
                    worldMode->voronoiP = voronoiP;
                    if(worldMode->voronoiGenerated)
                    {
                        jcv_diagram_free(&worldMode->voronoi);
                        memset(&worldMode->voronoi, 0, sizeof(jcv_diagram));
                    }
                    
                    BEGIN_BLOCK("voronoi setup");
                    r32 sorrounderChunksInfluence = 0.3f;
                    
                    i32 lateralChunkSpan = SERVER_REGION_SPAN * SIM_REGION_CHUNK_SPAN;
                    r32 voxelSide = worldMode->voxelSide;
                    r32 chunkSide = worldMode->chunkSide;
                    u8 chunkDim = worldMode->chunkDim;
                    
                    i32 chunkApron = UI->chunkApron;
                    i32 originChunkX = voronoiP.chunkX;
                    i32 originChunkY = voronoiP.chunkY;
                    
                    jcv_rect rect;
                    r32 dim = (chunkApron + 4.0f) * chunkSide;
                    rect.min.x = -dim;
                    rect.min.y = -dim;
                    rect.max.x = dim;
                    rect.max.y = dim;
                    
                    u32 maxPointsPerTile = 16;
                    u32 maxPointCount = Squarei(worldMode->chunkDim) * Squarei(2 * chunkApron + 1) * maxPointsPerTile;
                    jcv_point* points = PushArray(worldMode->temporaryPool, jcv_point, maxPointCount, NoClear());
                    
                    
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
                                    BuildChunk(&worldMode->generator, chunk, X, Y);
                                    
                                    for(u32 tileY = 0; tileY < CHUNK_DIM; ++tileY)
                                    {
                                        for(u32 tileX = 0; tileX < CHUNK_DIM; ++tileX)
                                        {
                                            chunk->lightCount[tileY][tileX] = 0;
                                            chunk->lightIndexes[tileY][tileX] = V4(-1, -1, -1, -1);
                                        }
                                    }
                                }
                            }
                        }
                    }
                    
                    
                    u32 pointIndex = 0;
                    for(i32 Y = originChunkY - chunkApron; Y <= originChunkY + chunkApron; Y++)
                    {
                        for(i32 X = originChunkX - chunkApron; X <= originChunkX + chunkApron; X++)
                        {
                            Vec3 chunkLowLeftCornerOffset = V3(V2i(X - originChunkX, Y - originChunkY), 0.0f) * chunkSide - voronoiP.chunkOffset;
                            Rect2 chunkRect = RectMinDim(chunkLowLeftCornerOffset.xy, V2(chunkSide, chunkSide));
                            
                            i32 chunkX = Wrap(0, X, lateralChunkSpan);
                            i32 chunkY = Wrap(0, Y, lateralChunkSpan);
                            
                            if(ChunkValid(lateralChunkSpan, X, Y))
                            {	
                                RandomSequence seq = Seed((chunkX + 10) * (chunkY + 10));
                                WorldChunk* chunk = GetChunk(worldMode->chunks, ArrayCount(worldMode->chunks), X, Y, &worldMode->chunkPool);
                                
                                if(!chunk->initialized)
                                {
                                    InvalidCodePath;
                                }
                                
                                Assert(X == chunk->worldX);
                                Assert(Y == chunk->worldY);
                                
                                for(u8 tileY = 0; tileY < worldMode->chunkDim; tileY++)
                                {
                                    for(u8 tileX = 0; tileX < worldMode->chunkDim; tileX++)
                                    {
                                        Vec2 tileCenter = worldMode->voxelSide * V2(tileX + 0.5f, tileY + 0.5f);
                                        Vec2 destP2D = chunkLowLeftCornerOffset.xy + tileCenter;
                                        
                                        
                                        TileGenerationData* tileData = &chunk->tileData[tileY][tileX];
                                        
                                        TaxonomySlot* tileSlot = GetSlotForTaxonomy(worldMode->table, tileData->biomeTaxonomy);
                                        r32 pointMaxOffset = Min(0.5f * voxelSide, tileSlot->groundPointMaxOffset);
                                        u32 pointsPerTile = Min(maxPointsPerTile, tileSlot->groundPointPerTile);
                                        
                                        for(u32 pointI = 0; pointI < pointsPerTile; ++pointI)
                                        {
                                            points[pointIndex].x = destP2D.x + RandomBil(&seq) * pointMaxOffset;
                                            points[pointIndex].y = destP2D.y + RandomBil(&seq) * pointMaxOffset;
                                            
                                            ++pointIndex;
                                        }
                                    }
                                }                         
                            }
                        }
                    }
                    END_BLOCK();
                    
                    
                    // NOTE(Leonardo): voronoi ground
                    BEGIN_BLOCK("build diagram");
                    jcv_diagram_generate(pointIndex, points, &rect, &worldMode->voronoi);
                    worldMode->voronoiGenerated = true;
                    END_BLOCK();
                }
                
                
#if 0
                for(u32 chunkIndex = 0; chunkIndex < ArrayCount(worldMode->chunks); ++chunkIndex)
                {
                    WorldChunk* chunk = worldMode->chunks[chunkIndex];
                    while(chunk)
                    {
                        if(chunk->initialized)
                        {
                            Vec4 color = V4(1, 1, 1, 1);
                            i32 lateralChunkSpan = SERVER_REGION_SPAN * SIM_REGION_CHUNK_SPAN;
                            if(ChunkOutsideWorld(lateralChunkSpan, chunk->worldX, chunk->worldY))
                            {
                                color = V4(1, 0, 0, 1);
                            }
                            
                            r32 chunkSide = worldMode->chunkSide;
                            i32 originChunkX = voronoiP.chunkX;
                            i32 originChunkY = voronoiP.chunkY;
                            
                            Vec3 chunkLowLeftCornerOffset = V3(V2i(chunk->worldX - originChunkX, chunk->worldY - originChunkY), 0.0f) * chunkSide - player->universeP.chunkOffset;
                            
                            ObjectTransform chunkTransform = FlatTransform();
                            Rect2 rect = RectMinDim(chunkLowLeftCornerOffset.xy, V2(chunkSide, chunkSide));
                            
                            PushRect(group, chunkTransform, rect, color);
                            PushRectOutline(group, chunkTransform, rect, V4(0, 1, 0, 1), 0.1f);
                        }
                        
                        chunk = chunk->next;
                    }
                }
#endif
                
                BEGIN_BLOCK("voronoi rendering");
                jcv_site* sites = jcv_diagram_get_sites(&worldMode->voronoi);
                
                for(int i = 0; i < worldMode->voronoi.numsites; ++i)
                {
                    jcv_site* site = sites + i;
                    Vec2 offsetP = V2(site->p.x, site->p.y);
                    TileInfo QP = GetTileInfo(worldMode, worldMode->voronoiP, offsetP);
                    site->tile = QP;
                }
                
                const jcv_edge* edge = jcv_diagram_get_edges(&worldMode->voronoi);
                while(edge)
                {
                    jcv_site* site0 = edge->sites[0];
                    Vec2 site0P = V2(site0->p.x, site0->p.y);
                    TileInfo QSite0 = site0->tile;
                    
                    jcv_site* site1 = edge->sites[1];
                    Vec2 site1P = V2(site1->p.x, site1->p.y);
                    TileInfo QSite1 = site1->tile;
                    
                    Vec2 offsetFrom = V2(edge->pos[0].x, edge->pos[0].y);
                    Vec2 offsetTo = V2(edge->pos[1].x, edge->pos[1].y);
                    
#if 0
                    PushLine(group, V4(1, 1, 1, 1), V3(offsetFrom, 0.5f), V3(offsetTo, 0.5f), 0.1f);
                    
#else
                    if(offsetFrom.y > offsetTo.y)
                    {
                        Vec2 temp = offsetFrom;
                        offsetFrom = offsetTo;
                        offsetTo = temp;
                    }
                    
                    TileInfo QFrom = GetTileInfo(worldMode, worldMode->voronoiP, offsetFrom);
                    
                    Vec4 CFrom0 = Lerp(QFrom.color, QSite0.chunkyness, QSite0.color);
                    Vec4 CFrom1 = Lerp(QFrom.color, QSite1.chunkyness, QSite1.color);
                    
                    TileInfo QTo = GetTileInfo(worldMode, worldMode->voronoiP, offsetTo);
                    Vec4 CTo0 = Lerp(QTo.color, QSite0.chunkyness, QSite0.color);
                    Vec4 CTo1 = Lerp(QTo.color, QSite1.chunkyness, QSite1.color);
                    
                    
                    r32 outer0 = Outer(offsetFrom, offsetTo, site0P);
                    r32 probe0 = Outer(offsetFrom, offsetTo, offsetFrom - V2(1, 0));
                    b32 zeroIsOnLeftSide = false;
                    if((probe0 < 0 && outer0 < 0) || 
                       (probe0 > 0 && outer0 > 0))
                    {
                        zeroIsOnLeftSide = true;
                    }
                    
                    Vec4 site0PCamera = V4(site0P + worldMode->voronoiDelta.xy, QSite0.height, 0);
                    Vec4 site1PCamera = V4(site1P + worldMode->voronoiDelta.xy, QSite1.height, 0);
                    Vec4 offsetFromCamera = V4(offsetFrom + worldMode->voronoiDelta.xy, QFrom.height, 0);
                    Vec4 offsetToCamera = V4(offsetTo + worldMode->voronoiDelta.xy, QTo.height, 0);
                    
                    
                    TexturedQuadsCommand* entry = GetCurrentTriangles(group, 2, 6, 6);
                    if(entry)
                    {
                        if(zeroIsOnLeftSide)
                        {
                            PushTriangle(group, group->whiteTexture, QSite0.lightIndexes, 
                                         site0PCamera, QSite0.color,
                                         offsetFromCamera, CFrom0,
                                         offsetToCamera, CTo0, 0);
                            PushTriangle(group, group->whiteTexture, QSite1.lightIndexes,
                                         site1PCamera, QSite1.color,
                                         offsetToCamera, CTo1,
                                         offsetFromCamera, CFrom1, 0);
                        }
                        else
                        {
                            PushTriangle(group, group->whiteTexture, QSite0.lightIndexes, 
                                         site1PCamera, QSite1.color,
                                         offsetFromCamera, CFrom1,
                                         offsetToCamera, CTo1, 0);
                            PushTriangle(group, group->whiteTexture, QSite1.lightIndexes,
                                         site0PCamera, QSite0.color,
                                         offsetToCamera, CTo0,
                                         offsetFromCamera, CFrom0, 0);
                        }
                    }
#endif
                    
                    edge = edge->next;
                }
                END_BLOCK();
                
                
                BEGIN_BLOCK("particles");
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
                targetEntityID = targetEntityID;
                desiredAction = output.desiredAction;
                overlappingEntityID = output.overlappingEntityID;
                
                
                Rect3 worldCameraBounds = GetScreenBoundsAtTargetDistance(group);
                Rect2 screenBounds = RectCenterDim(V2(0, 0), V2(worldCameraBounds.max.x - worldCameraBounds.min.x, worldCameraBounds.max.y - worldCameraBounds.min.y));
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
                serverRect = UIOrthoTextOp(group, font, fontInfo, serverText, fontScale, V3(serverP, 0), TextOp_getSize, V4(1, 1, 1, 1));
                editorRect = UIOrthoTextOp(group, font, fontInfo, editorText, fontScale, V3(editorP, 0), TextOp_getSize, V4(1, 1, 1, 1));
                
                joinRect = UIOrthoTextOp(group, font, fontInfo, joinText, fontScale, V3(joinP, 0), TextOp_getSize, V4(1, 1, 1, 1));
                
                
                UIOrthoTextOp(group, font, fontInfo, serverText, fontScale, V3(serverP, 1.0f), TextOp_draw, V4(1, 1, 1, 1));
                UIOrthoTextOp(group, font, fontInfo, editorText, fontScale, V3(editorP, 1.0f), TextOp_draw, V4(1, 1, 1, 1));
                UIOrthoTextOp(group, font, fontInfo, joinText, fontScale, V3(joinP, 1.0f), TextOp_draw, V4(1, 1, 1, 1));
                
                
                Vec2 startingP = V2(-300, 0);
                for(u32 roleIndex = 0; roleIndex < ArrayCount(MetaFlags_EditorRole); ++roleIndex)
                {
                    MetaFlag* flag = MetaFlags_EditorRole + roleIndex;
                    Rect2 roleRect = UIOrthoTextOp(group, font, fontInfo, flag->name, fontScale, V3(startingP, 0), TextOp_getSize, V4(1, 1, 1, 1));
                    
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
                    UIOrthoTextOp(group, font, fontInfo, flag->name, fontScale, V3(startingP, 1.0f), TextOp_draw, V4(1, 1, 1, 1));
                    
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
                platformAPI.DEBUGExecuteSystemCommand("../server", "../build/win32_server.exe", "");
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
                platformAPI.DEBUGExecuteSystemCommand("../editor", "../build/win32_server.exe"," editor");
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
        
        gameState->assets = InitAssets(gameState, gameState->textureQueue, MegaBytes(32));
        
        InitializeSoundState(&gameState->soundState, &gameState->audioPool);
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
                if(input->allowedToQuit && input->ctrlDown && Pressed(&input->escButton))
                {
                    InvalidCodePath;
                    //gameState->mode = GameMode_Launcher;
                }
                
            } break;
            
            InvalidDefaultCase;
        }
        
    }
    while(rerun);
    
    if(myPlayer)
    {
        FlushAllQueuedPackets(input->timeToAdvance);
    }
    
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

