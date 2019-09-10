#include <winsock2.h>
#include <windows.h>
#include <stdio.h>

#include <time.h>

#include "forg_server.h"
#include "forg_token.cpp"
#include "forg_pool.cpp"
#include "forg_meta.cpp"
#include "forg_physics.cpp"

#if 0
//#include "physics_server.cpp"
//#include "forg_bound.cpp"
//#include "forg_object.cpp"
#include "forg_crafting.cpp"
#include "forg_world_generation.cpp"
#include "forg_world_server.cpp"
#include "forg_network_server.cpp"
#include "forg_import.cpp"
#include "forg_inventory.cpp"
//#include "forg_memory.cpp"
#include "forg_AI.cpp"
#include "forg_action_effect.cpp"
#include "forg_essence.cpp"
#endif

#include "forg_network_server.cpp"
#include "forg_world.cpp"
#include "forg_world_server.cpp"

#pragma comment(lib, "wsock32.lib")

#include "forg_meta_asset.cpp"
#include "asset_builder.cpp"
#include "miniz.c"

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

internal void DispatchApplicationPacket(ServerState* server, Player* player, u32 playerID,  unsigned char* packetPtr, u16 dataSize)
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
        
        case Type_gameAccess:
        {
            GameAccessRequest clientReq;
            unpack(packetPtr, "L", &clientReq.challenge); 
            if(challenge == clientReq.challenge)
            {
                UniversePos P = {};
                P.chunkX = 1;
                P.chunkY = 1;
                
                u32 ID = AddEntity(server, P, playerID);
                SendGameAccessConfirm(player, server->worldSeed, ID);
                
                P.chunkOffset.x = 1.0f;
                AddEntity(server, P, 0);
            }
            else
            {
                InvalidCodePath;
            }
        } break;
        
        
#if 0        
        case Type_FileHash:
        {
            u16 type;
            u16 subtype;
            u64 hash;
            packetPtr = unpack(packetPtr, "HHQ", &type, &subtype, &hash);
            ForgFile* file = FindFile(server, &player->files, type, subtype);
            file->hash = hash;
        } break;
#endif
        
#if 0        
        case Type_RegenerateWorldChunks:
        {
            if(server->editor)
            {
                u32 worldSeed;
                u32 generateMode;
                packetPtr = unpack(packetPtr, "LL", &worldSeed, &generateMode);
                server->worldSeed = worldSeed;
                
                CompletePastWritesBeforeFutureWrites;
                server->generateMode = (GenerateWorldMode) generateMode;
            }
        } break;
        
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
    
    for( u32 playerIndex = 0; 
        playerIndex < MAXIMUM_SERVER_PLAYERS; 
        playerIndex++ )
    {
        u32 playerID = playerIndex;
        Player* player = server->players + playerIndex;
        if(player->connectionSlot)
        {
            QueueAndFlushAllPackets(server, player, elapsedTime);
            if(player->connectionClosed)
            {
                platformAPI.net.CloseConnection(&server->clientInterface, player->connectionSlot);
                player->connectionSlot = 0;
                //RecyclePlayer(server, player);
            }
            else
            {
                u32 toSendSize = KiloBytes(50);
                while(player->firstLoginFileToSend && (toSendSize > 0))
                {
                    FileToSend* toSend = player->firstLoginFileToSend;
                    toSendSize = SendFileChunksToPlayer(server, player, toSendSize, toSend, &player->firstLoginFileToSend);
                }
                
                if(!player->firstLoginFileToSend)
                {
                    while(player->firstReloadedFileToSend && (toSendSize > 0))
                    {
                        FileToSend* toSend = player->firstReloadedFileToSend;
                        toSendSize = SendFileChunksToPlayer(server, player, toSendSize, toSend, &player->firstReloadedFileToSend);
                    }
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
                            DispatchApplicationPacket(server, player, playerID, test->data + sizeof(ForgNetworkApplicationData), test->dataSize - sizeof(ForgNetworkApplicationData));
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
                        DispatchApplicationPacket(server, player, playerID, packetPtr, received.dataSize - sizeof(ForgNetworkApplicationData));
                    }
                }
            }
        }
        
    }
}


internal Player* FirstFreePlayer(ServerState* server)
{
    Assert(server->playerCount < ArrayCount(server->players));
    Player* result = server->players + server->playerCount++;
    *result = {};
    ResetReceiver(&result->receiver);
    
    return result;
}


PLATFORM_WORK_CALLBACK(WatchForFileChanges)
{
    TimestampHash* hash = (TimestampHash*) param;
    while(true)
    {
        WatchReloadFileChanges(hash, ASSETS_RAW_PATH, ASSETS_PATH);
    }
}

inline void ReadCompressFile(GameFile* file, u32 uncompressedSize, u8* uncompressedContent)
{
    Clear(&file->pool);
	file->uncompressedSize = uncompressedSize;          
    file->compressedSize = compressBound(file->uncompressedSize);
    file->content = PushSize(&file->pool, file->compressedSize);
    u32 cmp_status = compress(file->content, (mz_ulong*) &file->compressedSize, (const unsigned char*) uncompressedContent, file->uncompressedSize);
    Assert(cmp_status == Z_OK);
    PAKFileHeader* header = (PAKFileHeader*) uncompressedContent;
    file->type = GetMetaAssetType(header->type);
    file->subtype = GetMetaAssetSubtype(file->type, header->subtype);
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
            
			ReadCompressFile(file, SafeTruncateUInt64ToU32(info->size), uncompressedContent);
            
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
        
        server->playerCount = 1;
        server->entityCount = 1;
        
        TimestampHash* hash = BootstrapPushStruct(TimestampHash, pool);
        
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
                AddFileCountHash(hash, countHash->type, countHash->subtype, countHash->fileCount, countHash->markupCount);
            }
            
            EndTemporaryMemory(fileMemory);
        }
        platformAPI.GetAllFilesEnd(&timestampFiles);
        
        BuildAssets(hash, ASSETS_RAW_PATH, ASSETS_PATH);
        platformAPI.PushWork(server->slowQueue, WatchForFileChanges, hash);
        
        //Assets* assets = InitAssets(JustEntityDefinitionFiles);
        //BuildWorld(server, GenerateWorld_OnlyChunks);
    }
    
	PlatformFileGroup reloadedFiles = platformAPI.GetAllFilesBegin(PlatformFile_AssetPack, RELOAD_PATH);
	for(PlatformFileInfo* info = reloadedFiles.firstFileInfo; info; info = info->next)
    {
        if(info->size)
        {
            b32 deleteFile = false;
            TempMemory fileMemory = BeginTemporaryMemory(&tempPool);
            
            PlatformFileHandle handle = platformAPI.OpenFile(&reloadedFiles, info);
            PAKFileHeader header;
            platformAPI.ReadFromFile(&handle, 0, sizeof(PAKFileHeader), &header);
            
            u16 type = GetMetaAssetType(header.type);
            u16 subtype = GetMetaAssetSubtype(type, header.subtype);
            
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
                u8* uncompressedContent = (u8*) PushSize(&tempPool, info->size);
                platformAPI.ReadFromFile(&handle, 0, info->size, uncompressedContent);
                
                platformAPI.ReplaceFile(PlatformFile_AssetPack, ASSETS_PATH, nameNoExtension, uncompressedContent, SafeTruncateUInt64ToU32(info->size));
                ReadCompressFile(file, SafeTruncateUInt64ToU32(info->size), uncompressedContent);
                
                for(u32 playerIndex = 0; 
                    playerIndex < MAXIMUM_SERVER_PLAYERS; 
                    playerIndex++ )
                {
                    Player* player = server->players + playerIndex;
                    if(player->connectionSlot)
                    {
                        FileToSend* toSend;
                        FREELIST_ALLOC(toSend, server->firstFreeToSendFile, PushStruct(&server->gamePool, FileToSend));
                        toSend->index = fileIndex;
                        
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
                    }
                }
            }
            
            EndTemporaryMemory(fileMemory);
            platformAPI.CloseFile(&handle);
            
            if(deleteFile)
            {
                platformAPI.ReplaceFile(PlatformFile_AssetPack, RELOAD_PATH, 
                                        nameNoExtension, 0, 0);
            }
        }
    }
    platformAPI.GetAllFilesEnd(&reloadedFiles);
    
    // TODO(Leonardo): change the API of the network library to return a networkConnection*!
    u16 newConnections[16] = {};
    u16 accepted = platformAPI.net.Accept(&server->clientInterface, newConnections, ArrayCount(newConnections));
    
    for(u32 newConnectionIndex = 0; newConnectionIndex < accepted; ++newConnectionIndex)
    {
        u16 connectionSlot = newConnections[newConnectionIndex];
        Player* player = FirstFreePlayer(server);
        player->connectionSlot = connectionSlot;
        
        Assert(!player->firstLoginFileToSend);
        Assert(!player->firstReloadedFileToSend);
        for(u32 fileIndex = 0; fileIndex < server->fileCount; ++fileIndex)
        {
            GameFile* file = server->files + fileIndex;
            
            FileToSend* toSend;
            FREELIST_ALLOC(toSend, server->firstFreeToSendFile, PushStruct(&server->gamePool, FileToSend));
            toSend->index = fileIndex;
            
            ++file->counter;
            FREELIST_INSERT(toSend, player->firstLoginFileToSend);
        }
    }
    
    
    HandlePlayersNetwork(server, elapsedTime);
    HandlePlayersRequest(server);
    MoveEntitiesAndSendUpdates(server, elapsedTime);
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