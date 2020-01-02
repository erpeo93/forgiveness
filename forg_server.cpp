#include <winsock2.h>
#include <windows.h>
#include <stdio.h>

#include <time.h>

#include "forg_server.h"
#include "forg_token.cpp"
#include "forg_pool.cpp"
#include "forg_world.cpp"
#include "forg_physics.cpp"
#include "forg_resizable_array.cpp"
#include "forg_network_server.cpp"
#define ONLY_DATA_FILES
#include "forg_asset.cpp"
#include "forg_archetypes.cpp"
#include "forg_meta.cpp"
#include "miniz.c"
#include "asset_builder.cpp"
#include "forg_world_generation.cpp"
#include "forg_game_effect.cpp"
#include "forg_crafting.cpp"
#include "forg_entity_layout.cpp"
#include "forg_world_server.cpp"
#include "forg_brain.cpp"

internal void DispatchApplicationPacket(ServerState* server, u32 playerIndex, PlayerComponent* player,  unsigned char* packetPtr, u16 dataSize)
{
    u32 challenge = 1111;
    
    unsigned char* original = packetPtr;
    
    ForgNetworkHeader header;
    packetPtr = ForgUnpackHeader(packetPtr, &header);
    switch(header.packetType)
    {
        case Type_login:
        {
            char* username = "leo";
            char* password = "1234";
            if(true)
            {
#if FORGIVENESS_INTERNAL
                b32 editingEnabled = true;
#else
                b32 editingEnabled = false;
#endif
                QueueLoginResponse(player, LOGIN_PORT, challenge, editingEnabled);
            }
            else
            {
                InvalidCodePath;
                //SendLoginResponse(player, 0, 0);
            }
        } break;
        
        case Type_FileHash:
        {
            u16 type;
            u64 subtypeHash;
            u64 dataHash;
            packetPtr = unpack(packetPtr, "HQQ", &type, &subtypeHash, &dataHash);
            
            if(!type && !subtypeHash && !dataHash)
            {
                u32 fileCount = 0;
                for(u32 fileIndex = 0; fileIndex < server->fileCount; ++fileIndex)
                {
                    GameFile* file = server->files + fileIndex;
                    if(file->valid)
                    {
                        Assert(file->dataHash);
                        
                        b32 toSend = true;
                        for(FileHash* hash = player->firstFileHash; hash; hash = hash->next)
                        {
                            if(file->type == hash->type && StringHash(file->subtype) == hash->subtypeHash)
                            {
                                Assert(hash->dataHash);
                                if(file->dataHash == hash->dataHash)
                                {
                                    toSend = false;
                                }
                                break;
                            }
                        }
                        
                        if(toSend)
                        {
                            ++fileCount;
                        }
                    }
                }
                QueueLoginFileTransferBegin(player, fileCount);
                
                
                for(u32 fileIndex = 0; fileIndex < server->fileCount; ++fileIndex)
                {
                    GameFile* file = server->files + fileIndex;
                    if(file->valid)
                    {
                        Assert(file->dataHash);
                        
                        b32 send = true;
                        for(FileHash* hash = player->firstFileHash; hash; hash = hash->next)
                        {
                            if(file->type == hash->type && StringHash(file->subtype) == hash->subtypeHash)
                            {
                                Assert(hash->dataHash);
                                if(file->dataHash == hash->dataHash)
                                {
                                    send = false;
                                }
                                break;
                            }
                        }
                        
                        if(send)
                        {
                            FileToSend* toSend;
                            FREELIST_ALLOC(toSend, server->firstFreeToSendFile, PushStruct(&server->gamePool, FileToSend));
                            toSend->acked = false;
                            toSend->playerIndex = player->runningFileIndex++;
                            toSend->serverFileIndex = fileIndex;
                            toSend->sendingOffset = 0;
                            
                            ++file->counter;
                            FREELIST_INSERT(toSend, player->firstLoginFileToSend);
                            QueueFileHeader(player, toSend->playerIndex, file->type, file->subtype, file->uncompressedSize, file->compressedSize);
                        }
                    }
                }
            }
            else
            {
                FileHash* hash;
                FREELIST_ALLOC(hash, server->firstFreeFileHash, PushStruct(&server->gamePool, FileHash));
                hash->type = type;
                hash->subtypeHash = subtypeHash;
                hash->dataHash = dataHash;
                
                FREELIST_INSERT(hash, player->firstFileHash);
            }
        } break;
        
        case Type_gameAccess:
        {
            GameAccessRequest clientReq;
            unpack(packetPtr, "L", &clientReq.challenge); 
            
            if(challenge == clientReq.challenge)
            {
				if(IsValidID(player->ID))
				{
					DeleteEntity(server, player->ID, DeleteEntity_Ghost);
					player->ID = {};
				}
                
                UniversePos P = FindPlayerStartingP(server, 0);
                AddEntityParams params = DefaultAddEntityParams();
                params.playerIndex = playerIndex;
                SpawnPlayer(server, P, params);
            }
            else
            {
                InvalidCodePath;
            }
        } break;
        
        case Type_Command:
        {
            u16 index;
            u16 action;
            EntityID targetID;
            EntityID usingID;
            u16 skillIndex;
            unpack(packetPtr, "HHLHL", &index, &action, &targetID, &skillIndex, &usingID);
            
            if(CommandIndexValid(index, player->expectingCommandIndex))
            {
                player->expectingCommandIndex = index + 1;
                GameCommand* command = &player->requestCommand;
                command->action = action;
                command->targetID = targetID;
                command->skillIndex = skillIndex;
                command->usingID = usingID;
            }
        } break;
        
        case Type_InventoryCommand:
        {
            GameCommand* command = &player->inventoryCommand;
            Assert(command->action == none);
            unpack(packetPtr, "HLLHLH", &command->action, &command->targetID, 
                   &command->containerID, 
                   &command->targetObjectIndex, &command->targetContainerID, &command->optionIndex);
            
        } break;
        
        
        case Type_CommandParameters:
        {
            CommandParameters* parameters = &player->commandParameters;
            unpack(packetPtr, "VV", &parameters->acceleration, &parameters->targetOffset);
        } break;
        
        case Type_selectRecipeEssence:
        {
            u16 index;
            u16 essence;
            unpack(packetPtr, "HH", &index, &essence);
            
            ActionComponent* action = GetComponent(server, player->ID, ActionComponent);
            if(action)
            {
                action->selectedCrafingEssences[index] = essence;
            }
            
        } break;
        
        case Type_FileHeader:
        {
            u32 index;
            unpack(packetPtr, "L", &index); 
            for(FileToSend* test = player->firstLoginFileToSend; test; test = test->next)
            {
                if(test->playerIndex == index)
                {
                    test->acked = true;
                    break;
                }
            }
            
            for(FileToSend* test = player->firstReloadedFileToSend; test; test = test->next)
            {
                if(test->playerIndex == index)
                {
                    test->acked = true;
                    break;
                }
            }
            
        } break;
        
        case Type_SpawnEntity:
        {
            UniversePos P = {};
            AssetID ID;
            ID.type = AssetType_EntityDefinition;
            unpack(packetPtr, "LHlllV", &ID.subtypeHashIndex, &ID.index, &P.chunkX, &P.chunkY, &P.chunkZ, &P.chunkOffset);
            
            u32 seed = GetNextUInt32(&server->entropy);
            
            AddEntityParams params = DefaultAddEntityParams();
            AddEntity_(server, P, ID, seed, params);
        } break;
        
        case Type_RecreateWorld:
        {
            UniversePos P = {};
            b32 createEntities;
            unpack(packetPtr, "llllV", &createEntities, &P.chunkX, &P.chunkY, &P.chunkZ, &P.chunkOffset);
            EXECUTE_JOB(server, DeleteAllEntities, (1 == 1), 0);
            BuildWorld(server, createEntities);
            QueueGameAccessConfirm(player, server->worldSeed, {}, true, false);
            AddEntityParams params = DefaultAddEntityParams();
            params.playerIndex = playerIndex;
            SpawnPlayer(server, P, params);
        } break;
        
        case Type_PauseToggle:
        {
            if(true)
            {
                server->gamePaused = !server->gamePaused;
            }
        } break;
        
#if FORGIVENESS_INTERNAL
        case Type_CaptureFrame:
        {
            server->captureFrame = true;
        } break;
#endif
        
        InvalidDefaultCase;
    }
}

internal void HandlePlayersNetwork(ServerState* server, r32 elapsedTime)
{
    MemoryPool scratchPool = {};
    for(CompIterator iter = FirstComponent(server, PlayerComponent); 
        IsValid(iter); iter = Next(iter))
    {
        PlayerComponent* player = GetComponentRaw(server, iter, PlayerComponent);
        if(player->connectionSlot)
        {
            FlushAllPackets(server, player, elapsedTime);
            if(player->connectionClosed)
            {
                platformAPI.net.CloseConnection(&server->clientInterface, player->connectionSlot);
                player->connectionSlot = 0;
                //RecyclePlayer(server, player);
                
                for(FileToSend* login = player->firstLoginFileToSend; login; login = login->next)
                {
                    GameFile* file = server->files + login->serverFileIndex;
                    Assert(file->counter > 0);
                    --file->counter;
                    FREELIST_DEALLOC(login, server->firstFreeToSendFile);
                }
                player->firstLoginFileToSend = 0;
                
                for(FileToSend* reload = player->firstReloadedFileToSend; reload; reload = reload->next)
                {
                    GameFile* file = server->files + reload->serverFileIndex;
                    Assert(file->counter > 0);
                    --file->counter;
                    FREELIST_DEALLOC(reload, server->firstFreeToSendFile);
                }
                player->firstReloadedFileToSend = 0;
                
                FREELIST_FREE(player->firstFileHash, FileHash, server->firstFreeFileHash);
                
            }
            else
            {
                u32 toSendSize = KiloBytes(250);
                
                FileToSend** toSendPtr = &player->firstLoginFileToSend;
                toSendSize = QueueAllPossibleFileData(server, player, toSendPtr, toSendSize);
                
                if(!player->firstLoginFileToSend)
                {
                    FileToSend** reloadPtr = &player->firstReloadedFileToSend;
                    QueueAllPossibleFileData(server, player, reloadPtr, toSendSize);
                }
            }
        }
        
        if(player->connectionSlot)
        {
            while(true)
            {
                NetworkPacketReceived received = platformAPI.net.GetPacketOnSlot(&server->clientInterface, player->connectionSlot);
                
                if(received.disconnected)
                {
                    player->connectionClosed = true;
                    break;
                }
                
                if(!received.dataSize)
                {
                    break;
                }
                
                
                unsigned char* packetPtr = received.data;
                
                ForgNetworkApplicationData applicationData;
                packetPtr = ForgUnpackApplicationData(packetPtr, &applicationData);
                
                ForgNetworkReceiver* receiver = &player->receiver;
                if(applicationData.flags & ForgNetworkFlag_Ordered)
                {
                    u32 delta = ApplicationDelta(applicationData, receiver->orderedBiggestReceived);
                    if(delta > 0 && delta < WINDOW_SIZE)
                    {
                        u32 index = (receiver->circularStartingIndex + (delta - 1)) % WINDOW_SIZE;
                        receiver->orderedWindow[index] = received;
                    }
                    
                    u32 dispatched = 0;
                    
                    while(true)
                    {
                        u32 index = (receiver->circularStartingIndex + dispatched) % WINDOW_SIZE;
                        NetworkPacketReceived* test = receiver->orderedWindow + index;
                        if(test->dataSize)
                        {
                            DispatchApplicationPacket(server, iter.index, player, test->data + sizeof(ForgNetworkApplicationData), test->dataSize - sizeof(ForgNetworkApplicationData));
                            test->dataSize = 0;
                            ++dispatched;
                        }
                        else
                        {
                            break;
                        }
                    }
                    
                    receiver->circularStartingIndex += dispatched;
                    receiver->orderedBiggestReceived.index += dispatched;
                }
                else
                {
                    if(ApplicationIndexGreater(applicationData, receiver->unorderedBiggestReceived))
                    {
                        receiver->unorderedBiggestReceived = applicationData;
                        DispatchApplicationPacket(server, iter.index, player, packetPtr, received.dataSize - sizeof(ForgNetworkApplicationData));
                    }
                }
            }
        }
    }
}

extern "C" SERVER_SIMULATE_WORLDS(SimulateWorlds)
{
    r32 elapsedTime = memory->elapsedTime;
    platformAPI = memory->api;
    ServerState* server = memory->server;
#if FORGIVENESS_INTERNAL
    globalDebugTable = memory->debugTable;
#endif
    MemoryPool tempPool = {};
    if(!memory->server)
    {
        LoadMetaData();
        
        server = memory->server = BootstrapPushStruct(ServerState, gamePool);
        TimestampHash* hash = &server->fileHash;
        
        PlatformFileGroup timestampFiles = platformAPI.GetAllFilesBegin(PlatformFile_timestamp, TIMESTAMP_PATH);
        for(PlatformFileInfo* info = timestampFiles.firstFileInfo; info; info = info->next)
        {
            TempMemory fileMemory = BeginTemporaryMemory(&tempPool);
            u8* fileContent = ReadEntireFile(&tempPool, &timestampFiles, info);
            if(sizeof(SavedFileInfoHash) == info->size)
            {
                SavedFileInfoHash* infoHash = (SavedFileInfoHash*) fileContent;
                AddFileDateHash(hash, infoHash->pathAndName, infoHash->timestamp);
            }
            else if(sizeof(SavedTypeSubtypeCountHash) == info->size)
            {
                SavedTypeSubtypeCountHash* countHash = (SavedTypeSubtypeCountHash*) fileContent;
                AddFileCountHash(hash, countHash->typeSubtype, countHash->fileCount, countHash->markupCount);
            }
            
            EndTemporaryMemory(fileMemory);
        }
        platformAPI.GetAllFilesEnd(&timestampFiles);
        
        BuildAssets(hash, ASSETS_RAW_PATH, ASSETS_PATH);
        
#if FORGIVENESS_INTERNAL
        PlatformFileGroup pakFiles = platformAPI.GetAllFilesBegin(PlatformFile_AssetPack, ASSETS_PATH);
        
        server->fileCount = pakFiles.fileCount;
        server->files = PushArray(&server->gamePool, GameFile, server->fileCount);
        
        u32 fileIndex = 0;
        for(PlatformFileInfo* info = pakFiles.firstFileInfo; info; info = info->next)
        {
            PlatformFileHandle handle = platformAPI.OpenFile(&pakFiles, info);
            GameFile* file = server->files + fileIndex++;
            TempMemory fileMemory = BeginTemporaryMemory(&tempPool);
            u8* uncompressedContent = (u8*) PushSize(&tempPool, info->size);
            platformAPI.ReadFromFile(&handle, 0, info->size, uncompressedContent);
            ReadCompressFile(server, file, SafeTruncateUInt64ToU32(info->size), uncompressedContent);
            platformAPI.CloseFile(&handle);        
            EndTemporaryMemory(fileMemory);
        }
        platformAPI.GetAllFilesEnd(&pakFiles);
#endif
        
        
        server->fastQueue = memory->fastQueue;
        server->slowQueue = memory->slowQueue;
        
        for(u32 taskIndex = 0; 
            taskIndex < ArrayCount(server->tasks); 
            taskIndex++)
        {
            TaskWithMemory* task = server->tasks + taskIndex;
            task->beingUsed = false;
        }
        
        server->networkPool.allocationFlags = PlatformMemory_NotRestored;
        
        u16 maxConnectionCount = 2;
        NetworkConnection* connections = PushArray(&server->networkPool, NetworkConnection, maxConnectionCount);
        
        u16 clientPort = LOGIN_PORT;
        platformAPI.net.InitServer(&server->clientInterface, clientPort, connections, maxConnectionCount);
        
        server->receivePacketWork.network = &server->clientInterface;
        server->receivePacketWork.ReceiveData = platformAPI.net.ReceiveData;
        platformAPI.PushWork(server->slowQueue, ReceiveNetworkPackets, &server->receivePacketWork);
        
        server->worldSeed = (u32) time(0);
        
        u32 maxEntityCount = 0xffff;
        MemoryPool* compPool = &server->gamePool;
        for(u16 archetypeIndex = 0; archetypeIndex < Archetype_Count; ++archetypeIndex)
        {
            InitArchetype(server, archetypeIndex, 16);
        }
        InitComponentArray(server, PlayerComponent, 16);
        
        platformAPI.PushWork(server->slowQueue, WatchForFileChanges, hash);
        server->assets = InitAssets(server->slowQueue, server->tasks, ArrayCount(server->tasks), &server->gamePool, 0, MegaBytes(16));
        
        
        SetMetaAssets(server->assets);
        
        InitSpatialPartition(&server->gamePool, &server->staticPartition);
        BuildWorld(server, true);
    }
    
    
    
    
    
    BEGIN_BLOCK("file reload");
    PlatformFileGroup reloadedFiles = platformAPI.GetAllFilesBegin(PlatformFile_AssetPack, RELOAD_PATH);
    for(PlatformFileInfo* info = reloadedFiles.firstFileInfo; info; info = info->next)
    {
        if(info->size)
        {
            ProcessReloadedFile(server, &tempPool, &reloadedFiles, info, false);
        }
    }
    platformAPI.GetAllFilesEnd(&reloadedFiles);
    
    
    PlatformFileGroup reloadedSendFiles = platformAPI.GetAllFilesBegin(PlatformFile_AssetPack, RELOAD_SEND_PATH);
    for(PlatformFileInfo* info = reloadedSendFiles.firstFileInfo; info; info = info->next)
    {
        if(info->size)
        {
            ProcessReloadedFile(server, &tempPool, &reloadedSendFiles, info, true);
        }
    }
    platformAPI.GetAllFilesEnd(&reloadedSendFiles);
    END_BLOCK();
    
    
    
    
    BEGIN_BLOCK("accept connections");
    // TODO(Leonardo): change the API of the network library to return a networkConnection*!
    u16 newConnections[16] = {};
    u16 accepted = platformAPI.net.Accept(&server->clientInterface, newConnections, ArrayCount(newConnections));
    
    for(u32 newConnectionIndex = 0; newConnectionIndex < accepted; ++newConnectionIndex)
    {
        u16 connectionSlot = newConnections[newConnectionIndex];
        u32 ignored;
        PlayerComponent* player = AcquireComponent(server, PlayerComponent, &ignored);
        *player = {};
        ResetReceiver(&player->receiver);
        player->connectionSlot = connectionSlot;
    }
    END_BLOCK();
    
    
    
    
    
    
    
    
    server->frameByFramePool = &tempPool;
    
    
    BEGIN_BLOCK("handle players stuff");
    HandlePlayersNetwork(server, elapsedTime);
	END_BLOCK();
    
    if(server->gamePaused)
	{
		elapsedTime = 0;
	}
    
    BEGIN_BLOCK("spatial partitions");
    InitSpatialPartition(server->frameByFramePool, &server->playerPartition);
    EXECUTE_JOB(server, FillPlayerSpacePartition, ArchetypeHas(PlayerComponent), elapsedTime);
    InitSpatialPartition(server->frameByFramePool, &server->standardPartition);
    EXECUTE_JOB(server, FillCollisionSpatialPartition, true, elapsedTime);
    END_BLOCK();
    
    BEGIN_BLOCK("entity effects");
    EXECUTE_JOB_TIMED(server, DispatchEntityDefaultEffects, true, server->defaultEffectTimer, elapsedTime, 0.5f);
    END_BLOCK();
    
    UpdateWorldBasics(server, elapsedTime);
    BEGIN_BLOCK("update entities");
    EXECUTE_JOB(server, MoveEntity, ArchetypeHas(MovementComponent), elapsedTime);
    EXECUTE_JOB(server, UpdateEntity, true, elapsedTime);
    EXECUTE_JOB_TIMED(server, UpdatePlant, ArchetypeHas(VegetationComponent), server->vegetationUpdateTimer, elapsedTime, 1.0f);
    
    server->updateBrains = false;
    r32 targetBrainTimer = 0.1f;
    server->brainTimer += elapsedTime;
    if(server->brainTimer >= targetBrainTimer)
    {
        server->updateBrains = true;
        server->brainTimer = 0;
    }
    
    EXECUTE_JOB(server, UpdateBrain, ArchetypeHas(BrainComponent), targetBrainTimer);
    EXECUTE_JOB(server, ExecuteCommand, ArchetypeHas(BrainComponent) && ArchetypeHas(ActionComponent), elapsedTime);
	EXECUTE_JOB(server, UpdateTempEntity, ArchetypeHas(TempEntityComponent), elapsedTime);
    EXECUTE_JOB(server, HandleOpenedContainers, ArchetypeHas(ContainerComponent), elapsedTime);
    END_BLOCK();
    
    
    BEGIN_BLOCK("send updates");
    b32 staticUpdate = false;
    server->staticUpdateTimer += elapsedTime;
    if(server->staticUpdateTimer >= 0.9f * STATIC_UPDATE_TIME)
    {
        server->staticUpdateTimer = 0;
        staticUpdate = true;
    }
    
    for(CompIterator iter = FirstComponent(server, PlayerComponent); 
        IsValid(iter); iter = Next(iter))
    {
        PlayerComponent* player = GetComponentRaw(server, iter, PlayerComponent);
        if(player->connectionSlot && IsValidID(player->ID))
        {
            DefaultComponent* playerDefault = GetComponent(server, player->ID, DefaultComponent);
            
            Rect3 updateBounds = AddRadius(playerDefault->bounds, UPDATE_DISTANCE * V3(1, 1, 1));
            SpatialPartitionQuery updateQuery = QuerySpatialPartition(&server->standardPartition, playerDefault->P, updateBounds);
            
            b32 sendStaticUpdate = staticUpdate;
            if(player->justEnteredWorld)
            {
                sendStaticUpdate = true;
                player->justEnteredWorld = false;
            }
            
            b32 completeUpdate = (sendStaticUpdate);
            for(EntityID ID = GetCurrent(&updateQuery); IsValid(&updateQuery); ID = Advance(&updateQuery))
            {
                SendEntityUpdate(server, ID, false, completeUpdate);
                if(HasComponent(ID, UsingComponent))
                {
                    UsingComponent* equipped = GetComponent(server, ID, UsingComponent);
                    for(u32 slotIndex = 0; slotIndex < ArrayCount(equipped->slots); ++slotIndex)
                    {
                        EntityID slotID = GetBoundedID(equipped->slots + slotIndex);
                        if(IsValidID(slotID))
                        {
                            SendEntityUpdate(server, slotID, false, completeUpdate);
                            
                            if(HasComponent(slotID, ContainerComponent))
                            {
                                ContainerComponent* container = GetComponent(server, slotID, ContainerComponent);
                                for(u32 storeIndex = 0; storeIndex < ArrayCount(container->storedObjects); ++storeIndex)
                                {
                                    EntityID subID = GetBoundedID(container->storedObjects + storeIndex);
                                    if(IsValidID(subID))
                                    {
                                        SendEntityUpdate(server, subID, false, false);
                                    }
                                }
                                
                                for(u32 usingIndex = 0; usingIndex < ArrayCount(container->usingObjects); ++usingIndex)
                                {
                                    EntityID subID = GetBoundedID(container->usingObjects + usingIndex);
                                    if(IsValidID(subID))
                                    {
                                        SendEntityUpdate(server, subID, false, false);
                                    }
                                }
                            }
                        }
                    }
                }
                
                if(HasComponent(ID, EquipmentComponent))
                {
                    EquipmentComponent* equipment = GetComponent(server, ID, EquipmentComponent);
                    for(u32 slotIndex = 0; slotIndex < ArrayCount(equipment->slots); ++slotIndex)
                    {
                        EntityID slotID = GetBoundedID(equipment->slots + slotIndex);
                        if(IsValidID(slotID))
                        {
                            SendEntityUpdate(server, slotID, false, completeUpdate);
                            if(HasComponent(slotID, ContainerComponent))
                            {
                                ContainerComponent* container = GetComponent(server, slotID, ContainerComponent);
                                for(u32 storeIndex = 0; storeIndex < ArrayCount(container->storedObjects); ++storeIndex)
                                {
                                    EntityID subID = GetBoundedID(container->storedObjects + storeIndex);
                                    if(IsValidID(subID))
                                    {
                                        SendEntityUpdate(server, subID, false, false);
                                    }
                                }
                                
                                for(u32 usingIndex = 0; usingIndex < ArrayCount(container->usingObjects); ++usingIndex)
                                {
                                    EntityID subID = GetBoundedID(container->usingObjects + usingIndex);
                                    if(IsValidID(subID))
                                    {
                                        SendEntityUpdate(server, subID, false, false);
                                    }
                                }
                            }
                        }
                    }
                }
            }
            
            if(sendStaticUpdate)
            {
                SpatialPartitionQuery staticUpdateQuery = QuerySpatialPartition(&server->staticPartition, playerDefault->P, updateBounds);
                for(EntityID ID = GetCurrent(&staticUpdateQuery); IsValid(&staticUpdateQuery); ID = Advance(&staticUpdateQuery))
                {
                    SendStaticUpdate(server, ID);
                }
            }
        }
    }
    Clear(&tempPool);
    END_BLOCK();
    
    BEGIN_BLOCK("spawn and delete entities");
    SpawnAndDeleteEntities(server, elapsedTime);
    END_BLOCK();
    
}




#if FORGIVENESS_INTERNAL
extern "C" SERVER_FRAME_END(ServerFrameEnd)
{
    ServerState* server = memory->server;
    FlipTableResult flip = FlipDebugTable(globalDebugTable);
    
    for(CompIterator iter = FirstComponent(server, PlayerComponent); 
        IsValid(iter); iter = Next(iter))
    {
        PlayerComponent* player = GetComponentRaw(server, iter, PlayerComponent);
        if(player->connectionSlot)
        {
            if(server->captureFrame)
            {
                for(u32 eventIndex = 0; eventIndex < flip.eventCount; ++eventIndex)
                {
                    DebugEvent* event = flip.eventArray + eventIndex;
                    QueueDebugEvent(player, event);
                }
            }
            else
            {
                DebugEvent* frameMarkerEvent = flip.eventArray + flip.eventCount - 1;
                Assert(frameMarkerEvent->type == DebugType_frameMarker);
                QueueDebugEvent(player, frameMarkerEvent);
            }
        }
    }
    server->captureFrame = false;
}
#else
extern "C" SERVER_FRAME_END(ServerFrameEnd)
{
}
#endif