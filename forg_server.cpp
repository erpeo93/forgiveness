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
#include "miniz.c"
#endif

#include "forg_network_server.cpp"
#include "forg_world.cpp"
#include "forg_world_server.cpp"

#pragma comment(lib, "wsock32.lib")

#include "forg_meta_asset.cpp"
#include "asset_builder.cpp"

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

internal void HandlePlayersNetwork(ServerState* server)
{
    MemoryPool scratchPool = {};
    
    server->elapsedTime = 0.1f;
    for( u32 playerIndex = 0; 
        playerIndex < MAXIMUM_SERVER_PLAYERS; 
        playerIndex++ )
    {
        u32 playerID = playerIndex;
        Player* player = server->players + playerIndex;
        if(player->connectionSlot)
        {
            b32 allPacketSent = QueueAndFlushAllPackets(server, player, server->elapsedTime);
            if(player->connectionClosed)
            {
                if(allPacketSent)
                {
                    platformAPI.net.CloseConnection(&server->clientInterface, player->connectionSlot);
                    player->connectionSlot = 0;
                    //RecyclePlayer(server, player);
                }
            }
            else
            {
                if(!player->allPakFileSent)
                {
                    u32 chunkSize = KiloBytes(1);
                    u32 toSendSize = MegaBytes(1);
                    
                    PlatformFileGroup pakFiles = platformAPI.GetAllFilesBegin(PlatformFile_uncompressedAsset, ASSETS_PATH);
                    
                    u32 fileIndex = 0;
                    for(PlatformFileInfo* info = pakFiles.firstFileInfo; info; info = info->next)
                    {
                        if(fileIndex++ == player->pakFileIndex)
                        {
                            PlatformFileHandle handle = platformAPI.OpenFile(&pakFiles, info);
                            if(player->pakFileOffset == 0)
                            {
                                PAKFileHeader header;
                                platformAPI.ReadFromFile(&handle, 0, sizeof(PAKFileHeader), &header);
                                u16 type = GetMetaAssetType(header.type);
                                u16 subtype = GetMetaAssetSubtype(type, header.subtype);
                                SendFileHeader(player, type, subtype, SafeTruncateUInt64ToU32(info->size), chunkSize);
                            }
                            
                            u32 sizeToRead = Min(toSendSize, SafeTruncateUInt64ToU32(info->size) - player->pakFileOffset);
                            
                            if(sizeToRead <= chunkSize)
                            {
                                break;
                            }
                            
                            TempMemory fileMemory = BeginTemporaryMemory(&scratchPool);
                            
                            char* buffer = (char*) PushSize(&scratchPool, sizeToRead);
                            platformAPI.ReadFromFile(&handle, player->pakFileOffset, sizeToRead, buffer);
                            SendFileChunks(player, buffer, sizeToRead, chunkSize);
                            
                            EndTemporaryMemory(fileMemory);
                            
                            
                            
                            
                            player->pakFileOffset += sizeToRead;
                            u32 modSizeToRead = sizeToRead;
                            if(modSizeToRead % chunkSize)
                            {
                                modSizeToRead += chunkSize - (sizeToRead % chunkSize);
                            }
                            Assert(modSizeToRead % chunkSize == 0);
                            toSendSize -= modSizeToRead;
                            
                            if(player->pakFileOffset >= info->size)
                            {
                                player->pakFileOffset = 0;
                                if(++player->pakFileIndex == pakFiles.fileCount)
                                {
                                    player->allPakFileSent = true;
                                }
                            }
                            platformAPI.CloseFile(&handle);
                        }
                    }
                    platformAPI.GetAllFilesEnd(&pakFiles);
                }
                
                
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

extern "C" SERVER_SIMULATE_WORLDS(SimulateWorlds)
{
    platformAPI = memory->api;
    ServerState* server = memory->server;
#if FORGIVENESS_INTERNAL
    globalDebugTable = memory->debugTable;
#endif
    if(!memory->server)
    {
        LoadMetaData();
        
        
        server = memory->server = BootstrapPushStruct(ServerState, worldPool);
        
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
        
        MemoryPool tempPool = {};
        
        PlatformFileGroup timestampFiles = platformAPI.GetAllFilesBegin(PlatformFile_timestamp, TIMESTAMP_PATH);
        for(PlatformFileInfo* info = timestampFiles.firstFileInfo; info; info = info->next)
        {
            TempMemory fileMemory = BeginTemporaryMemory(&tempPool);
            
            if(sizeof(SavedFileInfoHash) == info->size)
            {
                u8* fileContent = ReadEntireFile(&tempPool, &timestampFiles, info);
                SavedFileInfoHash* infoHash = (SavedFileInfoHash*) fileContent;
                AddFileDateHash(hash, infoHash->pathAndName, infoHash->timestamp);
            }
            
            EndTemporaryMemory(fileMemory);
        }
        platformAPI.GetAllFilesEnd(&timestampFiles);
        
        BuildAssets(hash, ASSETS_RAW_PATH, ASSETS_PATH);
        platformAPI.PushWork(server->slowQueue, WatchForFileChanges, hash);
        
        //Assets* assets = InitAssets(JustEntityDefinitionFiles);
        //BuildWorld(server, GenerateWorld_OnlyChunks);
    }
    
    // TODO(Leonardo): change the API of the network library to return a networkConnection*!
    u16 newConnections[16] = {};
    u16 accepted = platformAPI.net.Accept(&server->clientInterface, newConnections, ArrayCount(newConnections));
    
    for(u32 newConnectionIndex = 0; newConnectionIndex < accepted; ++newConnectionIndex)
    {
        u16 connectionSlot = newConnections[newConnectionIndex];
        Player* player = FirstFreePlayer(server);
        player->connectionSlot = connectionSlot;
    }
    
    
    HandlePlayersNetwork(server);
    HandlePlayersRequest(server);
    MoveEntitiesAndSendUpdates(server);
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