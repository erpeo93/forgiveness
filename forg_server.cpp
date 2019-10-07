#include <winsock2.h>
#include <windows.h>
#include <stdio.h>

#include <time.h>

#include "forg_server.h"
#include "forg_token.cpp"
#include "forg_pool.cpp"
#include "forg_physics.cpp"
#include "forg_resizable_array.cpp"
//#include "forg_bound.cpp"
#include "forg_network_server.cpp"
#include "forg_world.cpp"
#include "forg_archetypes.cpp"

#define ONLY_DATA_FILES
#include "forg_asset.cpp"
#include "forg_meta.cpp"
#include "asset_builder.cpp"
#include "miniz.c"

#include "forg_world_generation.cpp"
#include "forg_world_server.cpp"
#if FORGIVENESS_INTERNAL
DebugTable* globalDebugTable;
internal void HandleDebugMessage(PlatformServerMemory* memory, ServerPlayer* player, u32 packetType, unsigned char* packetPtr)
{
    ServerState* server = (ServerState*) memory->server;
    switch( packetType )
    {
        case Type_InputRecording:
        {
            b32 recording;
            b32 startAutomatically;
            unpack( packetPtr, "ll", &recording, &startAutomatically );
            platformAPI.PlatformInputRecordingCommand( server, recording, startAutomatically );
        } break;
        
        case Type_debugEvent:
        {
            DebugEvent* editEvent = &globalDebugTable->editEvent;
            // NOTE(Leonardo): leonardo: we avoid receiving the "string" version of the guid, and we override it with the original one.
            unpack( packetPtr, "QQLHCQQ", &editEvent->clock, &editEvent->GUID, &editEvent->threadID, &editEvent->coreIndex, &editEvent->type, &editEvent->overNetwork[0], &editEvent->overNetwork[1]);
        } break;
        
    }
}
#endif


PLATFORM_WORK_CALLBACK(ReceiveNetworkPackets)
{
    ReceiveNetworkPacketWork* work = (ReceiveNetworkPacketWork*) param;
    
    while(true)
    {
        work->ReceiveData(work->network);
    }
}

#define SpawnPlayer(server, player, P) SpawnPlayer_(server, player, P, false)
#define RespawnPlayer(server, player, P) SpawnPlayer_(server, player, P, true)
internal void SpawnPlayer_(ServerState* server, PlayerComponent* player, UniversePos P, b32 deleteEntities)
{
    EntityRef type = {};
    EntityID ID = AddEntity(server, P, &server->entropy, type, player);
    
    ResetQueue(player->queues + GuaranteedDelivery_None);
    SendGameAccessConfirm(player, server->worldSeed, ID, deleteEntities);
}

internal void DispatchApplicationPacket(ServerState* server, PlayerComponent* player,  unsigned char* packetPtr, u16 dataSize)
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
                b32 editingEnabled = false;
                SendLoginResponse(player, LOGIN_PORT, challenge, editingEnabled);
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
            u16 subtype;
            u64 hash;
            packetPtr = unpack(packetPtr, "HHQ", &type, &subtype, &hash);
            
            for(u32 fileIndex = 0; fileIndex < server->fileCount; ++fileIndex)
            {
                GameFile* file = server->files + fileIndex;
                if(file->type == type && file->subtype == subtype)
                {
                    Assert(file->dataHash);
                    if(file->dataHash != hash)
                    {
                        FileToSend* toSend;
                        FREELIST_ALLOC(toSend, server->firstFreeToSendFile, PushStruct(&server->gamePool, FileToSend));
                        toSend->acked = false;
                        toSend->playerIndex = player->runningFileIndex++;
                        toSend->serverFileIndex = fileIndex;
                        toSend->sendingOffset = 0;
                        
                        ++file->counter;
                        SendFileHeader(player, toSend->playerIndex, file->type, file->subtype, file->uncompressedSize, file->compressedSize);
                        FREELIST_INSERT(toSend, player->firstLoginFileToSend);
                    }
                    break;
                }
            }
        } break;
        case Type_gameAccess:
        {
            GameAccessRequest clientReq;
            unpack(packetPtr, "L", &clientReq.challenge); 
            if(challenge == clientReq.challenge)
            {
                UniversePos P = {};
                P.chunkX = 1;
                P.chunkY = 1;
                SpawnPlayer(server, player, P);
            }
            else
            {
                InvalidCodePath;
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
            unpack(packetPtr, "HHllV", &ID.subtype, &ID.index, &P.chunkX, &P.chunkY, &P.chunkOffset);
            
            u32 seed = GetNextUInt32(&server->entropy);
            AddEntity_(server, P, ID, seed, 0);
        } break;
        
        
        case Type_RecreateWorld:
        {
            UniversePos P = {};
            b32 createEntities;
            unpack(packetPtr, "lllV", &createEntities, &P.chunkX, &P.chunkY, &P.chunkOffset);
            EXECUTE_JOB(server, DeleteAllEntities, (1 == 1), 0);
            BuildWorld(server, createEntities);
            RespawnPlayer(server, player, P);
        } break;
        
#if 0        
        
        case Type_ReloadAssets:
        {
            if(server->editor)
            {
                BuildAssetsAsync(server);
            }
        } break;
        
        case Type_PatchCheck:
        {
            if(server->editor)
            {
                ReloadServer(server);
                if(!server->activeTable->errorCount)
                {
                    printf("Yay! you can patch the local server!\n");
                }
                else
                {
                    InvalidCodePath;
                }
                
                SendPatchDoneMessageToAllPlayers(server);
            } break;
        } break;
        
        case Type_PatchLocalServer:
        {
            if(server->editor)
            {
                char* destinationFolder = "../server/assets";
                char* destinationPath = "../server";
                platformAPI.DeleteFolderRecursive(destinationFolder);
                platformAPI.CopyAllFiles("assets", destinationPath);
                
                SendPatchDoneMessageToAllPlayers(server);
            }
        } break;
        
        case Type_PauseToggle:
        {
            if(server->editor)
            {
                server->gamePaused = !server->gamePaused;
            }
        } break;
#endif
        
        default:
        {
            if(player->requestCount < ArrayCount(player->requests))
            {
                PlayerRequest* request = player->requests + player->requestCount++;
                Assert(dataSize < ArrayCount(request->data));
                Copy(dataSize, request->data, original);
            }
        } break;
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
            QueueAndFlushAllPackets(server, player, elapsedTime);
            if(player->connectionClosed)
            {
                platformAPI.net.CloseConnection(&server->clientInterface, player->connectionSlot);
                player->connectionSlot = 0;
                //RecyclePlayer(server, player);
                
                for(FileToSend* login = player->firstLoginFileToSend; login; login = login->next)
                {
                    GameFile* file = server->files + login->serverFileIndex;
                    --file->counter;
                    FREELIST_DEALLOC(login, server->firstFreeToSendFile);
                }
                player->firstLoginFileToSend = 0;
                
                for(FileToSend* reload = player->firstReloadedFileToSend; reload; reload = reload->next)
                {
                    GameFile* file = server->files + reload->serverFileIndex;
                    --file->counter;
                    FREELIST_DEALLOC(reload, server->firstFreeToSendFile);
                }
                player->firstReloadedFileToSend = 0;
                
            }
            else
            {
                u32 toSendSize = KiloBytes(250);
                
                FileToSend** toSendPtr = &player->firstLoginFileToSend;
                toSendSize = SendAllPossibleData(server, player, toSendPtr, toSendSize);
                
                if(!player->firstLoginFileToSend)
                {
                    FileToSend** reloadPtr = &player->firstReloadedFileToSend;
                    SendAllPossibleData(server, player, reloadPtr, toSendSize);
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
                            DispatchApplicationPacket(server, player, test->data + sizeof(ForgNetworkApplicationData), test->dataSize - sizeof(ForgNetworkApplicationData));
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
                        DispatchApplicationPacket(server, player, packetPtr, received.dataSize - sizeof(ForgNetworkApplicationData));
                    }
                }
            }
        }
    }
}

PLATFORM_WORK_CALLBACK(WatchForFileChanges)
{
    TimestampHash* hash = (TimestampHash*) param;
    while(true)
    {
        WatchReloadFileChanges(hash, ASSETS_RAW_PATH, RELOAD_PATH, RELOAD_SEND_PATH);
    }
}

inline void ReadCompressFile(ServerState* server, GameFile* file, u32 uncompressedSize, u8* uncompressedContent)
{
    Clear(&file->pool);
	file->uncompressedSize = uncompressedSize;          
    file->compressedSize = compressBound(file->uncompressedSize);
    file->content = PushSize(&file->pool, file->compressedSize);
    u32 cmp_status = compress(file->content, (mz_ulong*) &file->compressedSize, (const unsigned char*) uncompressedContent, file->uncompressedSize);
    Assert(cmp_status == Z_OK);
    PAKFileHeader* header = (PAKFileHeader*) uncompressedContent;
    file->type = GetMetaAssetType(header->type);
    file->subtype = GetAssetSubtype(server->assets, file->type, header->subtype);
    file->dataHash = DataHash((char*) uncompressedContent, uncompressedSize);
}

internal void ProcessReloadedFile(ServerState* server, MemoryPool* pool, PlatformFileGroup* group, PlatformFileInfo* info, b32 sendToPlayers)
{
    b32 deleteFile = false;
    TempMemory fileMemory = BeginTemporaryMemory(pool);
    
    PlatformFileHandle handle = platformAPI.OpenFile(group, info);
    PAKFileHeader header;
    platformAPI.ReadFromFile(&handle, 0, sizeof(PAKFileHeader), &header);
    
    u16 type = GetMetaAssetType(header.type);
    u16 subtype = GetAssetSubtype(server->assets, type, header.subtype);
    
    u32 fileIndex = 0;
    GameFile* file = 0;
    
    for(u32 testIndex = 0; testIndex < server->fileCount; ++testIndex)
    {
        GameFile* test = server->files + testIndex;
        if(test->type == type && test->subtype == subtype)
        {
            file = test;
            fileIndex = testIndex;
            break;
        }
    }
    
    char nameNoExtension[128];
    TrimToFirstCharacter(nameNoExtension, sizeof(nameNoExtension), info->name, '.');
    if(file && (file->counter == 0))
    {
        deleteFile = true;
        u8* uncompressedContent = (u8*) PushSize(pool, info->size);
        platformAPI.ReadFromFile(&handle, 0, info->size, uncompressedContent);
        
        
        u32 destFileIndex = 0;
        AssetFile* destFile = CloseAssetFileFor(server->assets, type, subtype, &destFileIndex);
        Assert(destFile);
        ReopenReloadAssetFile(server->assets, destFile, destFileIndex, type, subtype, uncompressedContent, SafeTruncateUInt64ToU32(info->size), pool);
        
        ReadCompressFile(server, file, SafeTruncateUInt64ToU32(info->size), uncompressedContent);
        
        if(sendToPlayers)
        {
            for(u16 archetypeIndex = 0; archetypeIndex < Archetype_Count; ++archetypeIndex)
            {
                if(HasComponent(archetypeIndex, PlayerComponent))
                {
                    for(ArchIterator iter = First(server, archetypeIndex); 
                        IsValid(iter); 
                        iter = Next(iter))
                    {
                        PlayerComponent* player = GetComponent(server, iter.ID, PlayerComponent);
                        if(player && player->connectionSlot)
                        {
                            FileToSend* toSend;
                            FREELIST_ALLOC(toSend, server->firstFreeToSendFile, PushStruct(&server->gamePool, FileToSend));
                            
                            toSend->acked = false;
                            toSend->playerIndex = player->runningFileIndex++;
                            toSend->serverFileIndex = fileIndex;
                            toSend->sendingOffset = 0;
                            
                            ++file->counter;
                            if(!player->firstReloadedFileToSend)
                            {
                                player->firstReloadedFileToSend = toSend;
                            }
                            else
                            {
                                for(FileToSend* firstToSend = player->firstReloadedFileToSend;; firstToSend = firstToSend->next)
                                {
                                    if(!firstToSend->next)
                                    {
                                        firstToSend->next = toSend;
                                        break;
                                    }
                                }
                            }
                            
                            SendFileHeader(player, toSend->playerIndex, file->type, file->subtype, file->uncompressedSize, file->compressedSize);
                        }
                    }
                }
            }
        }
    }
    
    EndTemporaryMemory(fileMemory);
    
    BeginTicketMutex(&server->fileHash.fileMutex);
    platformAPI.CloseFile(&handle);
    if(deleteFile)
    {
        char* path = sendToPlayers ? RELOAD_SEND_PATH : RELOAD_PATH;
        platformAPI.ReplaceFile(PlatformFile_AssetPack, path, 
                                nameNoExtension, 0, 0, 0);
    }
    EndTicketMutex(&server->fileHash.fileMutex);
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
        
        b32 editorMode = true;
        BuildAssets(hash, ASSETS_RAW_PATH, ASSETS_PATH);
        
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
        BuildWorld(server, true);
    }
    
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
    
    
    SpawnEntities(server, elapsedTime);
    HandlePlayersNetwork(server, elapsedTime);
    EXECUTE_JOB(server, HandlePlayerRequests, ArchetypeHas(PlayerComponent), elapsedTime);
    EXECUTE_JOB(server, MoveAndSendUpdate, ArchetypeHas(PhysicComponent), elapsedTime);
}




#if FORGIVENESS_INTERNAL
extern "C" SERVER_FRAME_END(ServerFrameEnd)
{
    TIMED_FUNCTION();
    ServerState* server = memory->server;
    globalDebugTable->currentEventArrayIndex = !globalDebugTable->currentEventArrayIndex;
    
    u64 arrayIndex_eventIndex = AtomicExchangeU64(&globalDebugTable->eventArrayIndex_EventIndex, 
                                                  ((u64) globalDebugTable->currentEventArrayIndex << 32));
    
    u32 eventArrayIndex = arrayIndex_eventIndex >> 32;
    Assert(eventArrayIndex <= 1);
    u32 eventCount = arrayIndex_eventIndex & 0xffffffff;
    
    if(!server->recompiled && server->debugPlayer)
    {
        for(u32 eventIndex = 0; eventIndex < eventCount; ++eventIndex)
        {
            DebugEvent* event = globalDebugTable->eventArray[eventArrayIndex] + eventIndex;
            SendDebugEvent(server->debugPlayer, event);
        }
        
        SendMemStats(server->debugPlayer);
    }
    return globalDebugTable;
}
#else
extern "C" SERVER_FRAME_END( ServerFrameEnd )
{
    return 0;
}
#endif