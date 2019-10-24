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
internal void HandleDebugMessage(PlatformServerMemory* memory, PlayerComponent* player, u32 packetType, unsigned char* packetPtr)
{
    InvalidCodePath;
#if 0    
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
    }
#endif
    
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
    EntityRef type = EntityReference(server, "default", "human");
    EntityID ID = AddEntity(server, P, &server->entropy, type, player);
    
    ResetQueue(player->queues + GuaranteedDelivery_None);
    QueueGameAccessConfirm(player, server->worldSeed, ID, deleteEntities);
}

internal void StoreFileHash(ServerState* server, PlayerComponent* player, u16 type, u64 subtypeHash, u64 dataHash)
{
    FileHash* hash;
    FREELIST_ALLOC(hash, server->firstFreeFileHash, PushStruct(&server->gamePool, FileHash));
    hash->type = type;
    hash->subtypeHash = subtypeHash;
    hash->dataHash = dataHash;
    
    FREELIST_INSERT(hash, player->firstFileHash);
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
                    if(file->type != 0xffff)
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
                    if(file->type != 0xffff)
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
                            QueueFileHeader(player, toSend->playerIndex, file->type, file->subtype, file->uncompressedSize, file->compressedSize);
                            FREELIST_INSERT(toSend, player->firstLoginFileToSend);
                        }
                    }
                }
            }
            else
            {
                StoreFileHash(server, player, type, subtypeHash, dataHash);
            }
        } break;
        
        case Type_gameAccess:
        {
            GameAccessRequest clientReq;
            unpack(packetPtr, "L", &clientReq.challenge); 
            if(challenge == clientReq.challenge)
            {
                UniversePos P = {};
                P.chunkZ = 0;
                P.chunkX = 1;
                P.chunkY = 1;
                SpawnPlayer(server, player, P);
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
            Vec3 acc;
            EntityID targetID;
            unpack(packetPtr, "HHVL", &index, &action, &acc, &targetID);
            
            if(CommandIndexValid(index, player->expectingCommandIndex))
            {
                player->expectingCommandIndex = index + 1;
                GameCommand* command = &player->requestCommand;
                command->action = action;
                command->acceleration = acc;
                command->targetID = targetID;
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
            AddEntity_(server, P, ID, seed, 0);
        } break;
        
        case Type_RecreateWorld:
        {
            UniversePos P = {};
            b32 createEntities;
            unpack(packetPtr, "llllV", &createEntities, &P.chunkX, &P.chunkY, &P.chunkZ, &P.chunkOffset);
            EXECUTE_JOB(server, DeleteAllEntities, (1 == 1), 0);
            BuildWorld(server, createEntities);
            RespawnPlayer(server, player, P);
        } break;
        
#if 0        
        case Type_PauseToggle:
        {
            if(server->editor)
            {
                server->gamePaused = !server->gamePaused;
            }
        } break;
#endif
        
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
    
    Assert(StrLen(header->subtype) <= ArrayCount(file->subtype));
    FormatString(file->subtype, sizeof(file->subtype), "%s", header->subtype);
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
    u64 subtypeHash = StringHash(header.subtype);
    
    u32 fileIndex = 0;
    GameFile* file = 0;
    
    for(u32 testIndex = 0; testIndex < server->fileCount; ++testIndex)
    {
        GameFile* test = server->files + testIndex;
        if(test->type == type && StringHash(test->subtype) == subtypeHash)
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
        AssetFile* destFile = CloseAssetFileFor(server->assets, type, subtypeHash, &destFileIndex);
        Assert(destFile);
        ReopenReloadAssetFile(server->assets, destFile, destFileIndex, type, header.subtype, uncompressedContent, SafeTruncateUInt64ToU32(info->size), pool);
        
        ReadCompressFile(server, file, SafeTruncateUInt64ToU32(info->size), uncompressedContent);
        
        if(sendToPlayers)
        {
            for(u16 archetypeIndex = 0; archetypeIndex < Archetype_Count; ++archetypeIndex)
            {
                if(HasComponent_(archetypeIndex, PlayerComponent))
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
                            
                            QueueFileHeader(player, toSend->playerIndex, file->type, file->subtype, file->uncompressedSize, file->compressedSize);
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
    TIMED_FUNCTION();
    
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
        BuildWorld(server, false);
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
    
    
    
    
    
    
    
    
    
    server->frameByFramePool = &tempPool;
    
    SpawnEntities(server, elapsedTime);
    HandlePlayersNetwork(server, elapsedTime);
    InitSpatialPartition(server->frameByFramePool, &server->playerPartition);
    EXECUTE_JOB(server, FillPlayerSpacePartition, ArchetypeHas(PlayerComponent) && ArchetypeHas(PhysicComponent), elapsedTime);
    
    InitSpatialPartition(server->frameByFramePool, &server->collisionPartition);
    EXECUTE_JOB(server, FillCollisionSpatialPartition, ArchetypeHas(PhysicComponent), elapsedTime);
    
    EXECUTE_JOB(server, HandlePlayerCommands, ArchetypeHas(PlayerComponent), elapsedTime);
    EXECUTE_JOB(server, DispatchEquipmentEffects, ArchetypeHas(PhysicComponent) && (ArchetypeHas(EquipmentComponent) || ArchetypeHas(UsingComponent)), elapsedTime);
    EXECUTE_JOB(server, UpdateEntity, ArchetypeHas(PhysicComponent), elapsedTime);
    EXECUTE_JOB(server, SendEntityUpdate, ArchetypeHas(PhysicComponent), elapsedTime);
    
    Clear(&tempPool);
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
extern "C" SERVER_FRAME_END( ServerFrameEnd )
{
    return 0;
}
#endif