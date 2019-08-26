#include <winsock2.h>
#include <windows.h>
#include <stdio.h>

#include <time.h>

#include "forg_server.h"
#include "forg_token.cpp"
#include "forg_pool.cpp"
#include "forg_meta.cpp"
#include "forg_taxonomy.cpp"
#include "forg_entity.cpp"
#include "forg_physics.cpp"
#include "physics_server.cpp"
#include "forg_bound.cpp"
#include "forg_object.cpp"
#include "forg_crafting.cpp"
#include "forg_world.cpp"
#include "forg_world_generation.cpp"
#include "forg_world_server.cpp"
#include "forg_network_server.cpp"
#include "forg_editor.cpp"
#include "forg_import.cpp"
#include "forg_inventory.cpp"
//#include "forg_memory.cpp"
#include "forg_AI.cpp"
#include "forg_action_effect.cpp"
#include "forg_essence.cpp"
#include "forg_region.cpp"
#include "miniz.c"

#pragma comment(lib, "wsock32.lib")

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

inline ForgFile* FindFile(ServerState* server, ForgFile** firstFilePtr, char* fileName)
{
    ForgFile* result = 0;
    
    char nameWithoutPoint[64];
    RemoveExtension(nameWithoutPoint, sizeof(nameWithoutPoint), fileName);
    
    u32 withoutPointLength = StrLen(nameWithoutPoint);
    
    for(ForgFile* test = *firstFilePtr; test; test = test->next)
    {
        if(StrEqual(withoutPointLength, test->filename, nameWithoutPoint))
        {
            result = test;
            break;
        }
    }
    
    if(!result)
    {
        FREELIST_ALLOC(result, server->firstFreeFile, PushStruct(&server->filePool, ForgFile));
        StrCpy(fileName, StrLen(fileName), result->filename, sizeof(result->filename));
        result->hash = 0;
        
        FREELIST_INSERT(result, *firstFilePtr);
    }
    return result;
}

inline void AddAllPakFileHashes(ServerState* server)
{
    char* pakPath = "assets";
    PlatformFileGroup pakGroup = platformAPI.GetAllFilesBegin(PlatformFile_compressedAsset, pakPath);
    for(u32 fileIndex = 0; fileIndex < pakGroup.fileCount; ++fileIndex)
    {
        TempMemory fileMemory = BeginTemporaryMemory(&server->scratchPool);
        
        PlatformFileHandle handle = platformAPI.OpenNextFile(&pakGroup, pakPath);
        
        char* buffer = (char*) PushSize(&server->scratchPool, handle.fileSize);
        platformAPI.ReadFromFile(&handle, 0, handle.fileSize, buffer);
        
        
        uLong uncompressedSize = *((uLong*) buffer);
        u32 fileSize = uncompressedSize;
        
        u8* uncompressed = PushArray(&server->scratchPool, u8, uncompressedSize);
        
        u8* compressedSource = (u8*) buffer + 4;
        uLong compressedLen = handle.fileSize - 4;
        
        int cmp_status = uncompress(uncompressed, &uncompressedSize, compressedSource, compressedLen);
        Assert(cmp_status == Z_OK);
        Assert(uncompressedSize == fileSize); 
        
        
        
        
        ForgFile* serverFile = FindFile(server, &server->files, handle.name);
        
        u64 hash64 = *(u64*) uncompressed;
        serverFile->hash = hash64;
        
        platformAPI.CloseHandle(&handle);
        
        EndTemporaryMemory(fileMemory);
    }
    platformAPI.GetAllFilesEnd(&pakGroup);
}

inline void BuildAssetsSync(ServerState* server)
{
    PlatformProcessHandle assetBuilder = platformAPI.DEBUGExecuteSystemCommand(".", "../asset_builder.exe", "");
    while(true)
    {
        PlatformProcessState assetBuilderState = platformAPI.DEBUGGetProcessState(assetBuilder);
        if(!assetBuilderState.isRunning)
        {
            break;
        }
    }
}

inline void BuildAssetsAsync(ServerState* server)
{
    server->reloadingAssets = true;
    server->assetBuilder = platformAPI.DEBUGExecuteSystemCommand(".", "../asset_builder.exe", "");
}

inline void ReloadServer(ServerState* server)
{
    WriteDataFiles(server->activeTable);
    ImportAllFiles();
}

PLATFORM_WORK_CALLBACK(ReceiveNetworkPackets)
{
    ReceiveNetworkPacketWork* work = (ReceiveNetworkPacketWork*) param;
    
    while(true)
    {
        work->ReceiveData(work->network);
    }
}

internal void DispatchApplicationPacket(ServerState* server, ServerPlayer* player, unsigned char* packetPtr, u16 dataSize)
{
    u32 challenge = 1111;
    
    unsigned char* original = packetPtr;
    
    ForgNetworkHeader header;
    packetPtr = ForgUnpackHeader(packetPtr, &header);
    switch(header.packetType)
    {
        // TODO( Leonardo ): type_register, type_activate and type_selectHero
        case Type_login:
        {
            // TODO( Leonardo ): get these from the client!
            char* username = "leo";
            char* password = "1234";
            
            if(true)//SQLCheckPassword( server->conn, username, password ) )
            {
                // TODO( Leonardo ): send message to correct server so that it can add the serverPlayer! in this case WE ARE ALWAYS the correct server.
                //ResetSecureSeeds(newC, &server->playerSeedSequence); 
                // TODO( Leonardo ): change login port whit whatever port 
                //the player is in
                b32 editingEnabled = server->editor;
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
            unpack(packetPtr, "Ll", &clientReq.challenge, &clientReq.sendDataFiles); 
            if(challenge == clientReq.challenge)
            {
                if(clientReq.sendDataFiles)
                {
                    InvalidCodePath;
                    
#if RESTRUCTURING                    
                    SendAllDataFiles(server->editor, player, DataFileSent_Everything);
                    SendAllDataFileSentMessage(player, DataFileSent_Everything);
                    player->allDataFileSent = true;
#endif
                    
                }
                
                PlayerPermanent* permanent = &server->editorPlayerPermanent;
                SimRegion* region = GetServerRegion( server, permanent->regionX, permanent->regionY);
                Vec3 P = permanent->P;
                
                TaxonomySlot* slot = NORUNTIMEGetTaxonomySlotByName( region->taxTable, "centaur" );
                u64 identifier = AddEntity(region, P, slot->taxonomy, NullGenerationData(), PlayerAddEntityParams(player->playerID));
                
                
                TaxonomySlot* testSlot = NORUNTIMEGetTaxonomySlotByName(region->taxTable, "strength");
                AddEntity(region, P + V3( 10.0f, 0.0f, 0.0f ), testSlot->taxonomy, NullGenerationData(), EntityFromObject(identifier, 0, 0));
                
                
                SendGameAccessConfirm(player, server->worldSeed, identifier, 0, server->elapsedMS5x);
                
#if FORGIVENESS_INTERNAL
                if(!server->debugPlayer)
                {
                    server->debugPlayer = player;
                }
#endif
            }
            else
            {
                InvalidCodePath;
            }
        } break;
        
        
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
        
        case Type_InstantiateTaxonomy:
        case Type_InstantiateRecipe:
        case Type_DeleteEntity:
        {
            if(server->editor)
            {
                if(player->requestCount < ArrayCount(player->requests))
                {
                    PlayerRequest* request = player->requests + player->requestCount++;
                    Assert(dataSize < ArrayCount(request->data));
                    Copy(dataSize, request->data, original);
                }
            }
        } break;
        
        case Type_PauseToggle:
        {
            if(server->editor)
            {
                server->gamePaused = !server->gamePaused;
            }
        } break;
        
        case Type_FileHash:
        {
            char filename[64];
            u64 hash;
            packetPtr = unpack(packetPtr, "sQ", filename, &hash);
            ForgFile* file = FindFile(server, &player->files, filename);
            file->hash = hash;
        } break;
        
        default:
        {
            if(!server->gamePaused && player->requestCount < ArrayCount(player->requests))
            {
                PlayerRequest* request = player->requests + player->requestCount++;
                Assert(dataSize < ArrayCount(request->data));
                Copy(dataSize, request->data, original);
            }
        } break;
    }
}

internal void DispatchPlayerMessages()
{
    server->elapsedTime = 0.1f;
    for( u32 playerIndex = 0; 
        playerIndex < MAXIMUM_SERVER_PLAYERS; 
        playerIndex++ )
    {
        ServerPlayer* player = server->players + playerIndex;
        if(player->connectionSlot)
        {
            b32 allPacketSent = QueueAndFlushAllPackets(server, player, server->elapsedTime);
            if(player->connectionClosed)
            {
                if(allPacketSent)
                {
                    platformAPI.net.CloseConnection(&server->clientInterface, player->connectionSlot);
                    player->connectionSlot = 0;
                    RecyclePlayer(server, player);
                }
            }
            else
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
}


internal ServerPlayer* FirstFreePlayer(ServerState* server)
{
    ServerPlayer* result = server->firstFree;
    if(!result)
    {
        Assert(server->currentPlayerIndex < (MAXIMUM_SERVER_PLAYERS - 1));
        u32 index = ++server->currentPlayerIndex;
        
        result = server->players + index;
        result->playerID = index;
        
        result->standardPacketQueue = {};
        result->reliablePacketQueue = {};
    }
    
    result->standardPacketQueue.nextSendApplicationData = {};
    result->reliablePacketQueue.nextSendApplicationData = {};
    
    Assert(!result->standardPacketQueue.firstPacket);
    Assert(!result->standardPacketQueue.lastPacket);
    
    Assert(!result->reliablePacketQueue.firstPacket);
    Assert(!result->reliablePacketQueue.lastPacket);
    
    result->connectionClosed = false;
    result->overlappingEntityID = 0;
    result->requestCount = 0;
    result->ignoredActionCount = 0;
    result->draggingEntity = {};
    result->unlockedCategoryCount = 0;
    result->recipeCount = 0;
    result->allDataFileSent = false;
    result->allPakFileSent = false;
    result->pakFileIndex = 0;
    result->pakFileOffset = 0;
    
    
    ResetReceiver(&result->receiver);
    
    server->firstFree = result->next;
    result->next = 0;
    return result;
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
        server = memory->server = BootstrapPushStruct(ServerState, worldPool);
        // NOTE(Leonardo): sqlite test!
        
        Win32ThreadStartup fastStartups[5] = {};
        Win32MakeQueue(&server->fastQueue, ArrayCount(fastStartups), fastStartups);
        
        Win32ThreadStartup slowStartups[1] = {};
        Win32MakeQueue(&server->slowQueue, ArrayCount(slowStartups), slowStartups);
        
        for( u32 taskIndex = 0; 
            taskIndex < ArrayCount( server->tasks ); 
            taskIndex++ )
        {
            TaskWithMemory* task = server->tasks + taskIndex;
            task->beingUsed = false;
        }
        
        
        server->networkPool.allocationFlags = PlatformMemory_NotRestored;
        server->players = PushArray(&server->networkPool, ServerPlayer, MAXIMUM_SERVER_PLAYERS);
        
        u16 maxConnectionCount = 2;
        NetworkConnection* connections = PushArray(&server->networkPool, NetworkConnection, maxConnectionCount);
        
        u16 clientPort = LOGIN_PORT;
        platformAPI.net.InitServer(&server->clientInterface, clientPort, connections, maxConnectionCount);
        
        server->receivePacketWork.network = &server->clientInterface;
        server->receivePacketWork.ReceiveData = platformAPI.net.ReceiveData;
        Win32PushWork(&server->slowQueue, ReceiveNetworkPackets, &server->receivePacketWork);
        
        server->worldSeed = (u32) time(0);
        
        
        platformAPI.DeleteFileWildcards("assets", "*");
        
        BuildAssetsSync(server);
        WriteDataFiles(server->activeTable);
        AddAllPakFileHashes(server);
        ImportAllFiles();
        
        BuildWorld(server, GenerateWorld_OnlyChunks);
    }
    
    
    // TODO(Leonardo): change the API of the network library to return a networkConnection*!
    u16 newConnections[16] = {};
    u16 accepted = platformAPI.net.Accept(&server->clientInterface, newConnections, ArrayCount(newConnections));
    
    for(u32 newConnectionIndex = 0; newConnectionIndex < accepted; ++newConnectionIndex)
    {
        u16 connectionSlot = newConnections[newConnectionIndex];
        ServerPlayer* player = FirstFreePlayer(server);
        player->connectionSlot = connectionSlot;
    }
    
    DispatchPlayerMessages();
    UpdateSeasonTimer();
    MoveEntities();
    SendEntityUpdates();
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