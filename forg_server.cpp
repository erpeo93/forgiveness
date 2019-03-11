#include <winsock2.h>
#include <windows.h>
#include <stdio.h>

#include <time.h>
#include <mysql.h>
#include "sqlite3.h"

#include "forg_server.h"
#include "forg_token.cpp"

#include "net.h"
#include "forg_pool.cpp"
#include "forg_meta.cpp"
#include "forg_physics.cpp"
#include "physics_server.cpp"
#include "forg_world.cpp"

inline SimRegion* GetServerRegionRaw(ServerState* server, u32 X, u32 Y)
{
    Assert(X < (SERVER_REGION_SPAN + 2));
    Assert(Y < (SERVER_REGION_SPAN + 2));
    SimRegion* result = &server->regions[Y][X];
    return result;
}

inline SimRegion* GetServerRegion(ServerState* server, i32 X, i32 Y)
{
    X += 1;
    Y += 1;
    Assert(X >= 0);
    Assert(Y >= 0);
    Assert(X < (SERVER_REGION_SPAN + 2));
    Assert(Y < (SERVER_REGION_SPAN + 2));
    
    SimRegion* result = GetServerRegionRaw(server, (u32) X, (u32) Y);
    return result;
}

inline SimRegion* GetServerRegionWrap(ServerState* server, i32 X, i32 Y)
{
    Assert(UNIVERSE_DIM == 1);
    Assert(X <= SERVER_REGION_SPAN);
    Assert(Y <= SERVER_REGION_SPAN);
    Assert(X >= -1);
    Assert(Y >= -1);
    
    if(X < 0)
    {
        X = SERVER_REGION_SPAN;
    }
    else if(X == SERVER_REGION_SPAN)
    {
        X = -1;
    }
    
    
    if(Y < 0)
    {
        Y = SERVER_REGION_SPAN;
    }
    else if(Y == SERVER_REGION_SPAN)
    {
        Y = -1;
    }
    
    Assert(server->universeX == 0 && server->universeY == 0);
    SimRegion* destRegion = GetServerRegion(server, X, Y);
    
    return destRegion;
}

inline Vec3 GetRegionP( SimRegion* region, UniversePos* p )
{
    r32 chunkSide = VOXEL_SIZE * region->server->chunkDim;
    Vec3 chunkOffset = chunkSide * V3i( p->chunkX - region->origin.chunkX, 
                                       p->chunkY - region->origin.chunkY, 0 );
    Vec3 result = V3( 0, 0, 0 );
    result = chunkOffset;
    result += p->chunkOffset - region->origin.chunkOffset;
    return result;
}

inline UniversePos GetUniverseP(SimRegion* region, Vec3 p)
{
    UniversePos result = {};
    result.chunkX = (i32) region->origin.chunkX;
    result.chunkY = (i32) region->origin.chunkY;
    result.chunkOffset = p + region->origin.chunkOffset;
    result = NormalizePosition(result, region->server->chunkSide, region->server->oneOverChunkSide);
    return result;
}

#include "forg_rule.cpp"
#include "forg_world_generation.cpp"
#include "forg_taxonomy.cpp"
#include "forg_consideration.cpp"
#include "forg_network_server.cpp"
#include "forg_editor.cpp"
#include "forg_inventory.cpp"
#include "forg_memory.cpp"
#include "forg_AI.cpp"
#include "forg_action_effect.cpp"
#include "forg_crafting.cpp"
#include "server_world.cpp"
#include "forg_network.cpp"
#include "forg_fluid.cpp"
#include "forg_region.cpp"

#pragma comment( lib, "wsock32.lib" )

#if FORGIVENESS_INTERNAL
DebugTable* globalDebugTable;

internal void DebugSpawnEntity( ServerState* server, ServerPlayer* player, DebugSpawnRequest request )
{
    //Assert( request.taxonomy );
    SimRegion* region = GetServerRegion( server, 0, 0 );
    Vec3 P = V3( 0.5f, 10.0f, 0 );
    AddEntity( region, P, request.taxonomy );
}


internal void HandleDebugMessage( PlatformServerMemory* memory, ServerPlayer* player, u32 packetType, unsigned char* packetPtr )
{
    ServerState* server = ( ServerState* ) memory->server;
    switch( packetType )
    {
        case Type_debugSpawnEntity:
        {
            DebugSpawnRequest req;
            unpack( packetPtr, "Lv", &req.taxonomy, &req.offsetFromPlayer );
            DebugSpawnEntity( server, player, req );
        } break;
        
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
            unpack( packetPtr, "QQLHCQQ", &editEvent->clock, &editEvent->GUID, &editEvent->threadID, &editEvent->coreIndex, &editEvent->type, &editEvent->overNetwork[0], &editEvent->overNetwork[1] );
        } break;
        
    }
}
#endif

inline void SwapTables(ServerState* server)
{
    TaxonomyTable* toClear = server->oldTable;
    Clear(&toClear->pool);
    ZeroStruct(*toClear);
    
    TaxonomyTable* temp = toClear;
    server->oldTable = server->activeTable;
    server->activeTable = temp;
    InitTaxonomyReadWrite(server->activeTable);
}

internal void FillGenerationData(ServerState* server)
{
    b32 freeTabs = !server->editor;
    ImportAllFiles("assets", (u32) 0xffffffff, freeTabs);
    ReadBehaviors();
    ReadSynthesisRules();
}

inline void TranslateServerChunks(ServerState* server)
{
    for(u32 chunkIndex = 0; chunkIndex < ArrayCount(server->chunks); ++chunkIndex)
    {
        WorldChunk* chunk = server->chunks[chunkIndex];
        while(chunk)
        {
            EntityBlock* block = chunk->entities;
            while(block)
            {
                u8* entityRaw = block->data;
                for(u32 entityIndex = 0; entityIndex < block->countEntity; ++entityIndex)
                {
                    SimEntity* source = (SimEntity*) entityRaw;
                    TranslateSimEntity(server, source);
                    
                    entityRaw += sizeof(SimEntity);
                    
                }
                
                block = block->next;
            }
            
            chunk = chunk->next;
        }
    }
}

inline void TranslateServerPlayers(ServerState* server)
{
    for(u32 playerIndex = 0; playerIndex < MAXIMUM_SERVER_PLAYERS; ++playerIndex)
    {
        ServerPlayer* player = server->players + playerIndex;
        if(player->connectionSlot)
        {
            TranslatePlayer(server->oldTable, server->activeTable, player);
            SendDataFiles(server->editor, player, true, false);
        }
    }
}

inline void InvalidateAllHashUpdates(ServerState* server)
{
    u32 maxRegionIndex = Squarei(SERVER_REGION_SPAN + 2);
    for(u32 regionIndex = 0; regionIndex < maxRegionIndex; ++regionIndex)
    {
        SimRegion* region = (SimRegion*) server->regions + regionIndex;
        if(region->updateHash)
        {
            for(u32 updateIndex = 0; updateIndex < HASH_UPDATE_COUNT; ++updateIndex)
            {
                HashEntityUpdate* update = region->updateHash + updateIndex;
                update->valid = false;
            }
        }
    }
}


inline void TranslateServer(ServerState* server)
{
    
    TranslateServerChunks(server);
    TranslateServerPlayers(server);
    InvalidateAllHashUpdates(server);
}

inline void ReloadServer(ServerState* server)
{
    SwapTables(server);
    WriteDataFiles();
    FillGenerationData(server);
    TranslateServer(server);
}

inline void PoundToNameAndFedFileRecursively(char* path, char* taxonomyName, b32 add)
{
    char originalFed[512];
    char newFed[512];
    
    FormatString(originalFed, sizeof(originalFed), "%s/%s.fed", path, taxonomyName);
    
    if(add)
    {
        FormatString(newFed, sizeof(originalFed), "%s/#%s.fed", path, taxonomyName);
    }
    else
    {
        FormatString(newFed, sizeof(originalFed), "%s/%s.fed", path, taxonomyName + 1);
    }
    
    
    platformAPI.MoveFileOrFolder(originalFed, newFed);
    
    PlatformSubdirNames subdir;
    subdir.subDirectoryCount = 0;
    
    platformAPI.GetAllSubdirectoriesName(&subdir, path);
    for(u32 subdirIndex = 0; subdirIndex < subdir.subDirectoryCount; ++subdirIndex)
    {
        char* folderName = subdir.subdirs[subdirIndex];
        if(!StrEqual(folderName, ".") && !StrEqual(folderName, "..") && !StrEqual(folderName, "side"))
        {
            char childPath[512];
            FormatString(childPath, sizeof(childPath), "%s/%s", path, folderName);
            PoundToNameAndFedFileRecursively(childPath, folderName, add);
        }
    }
    
    char newPath[512];
    u32 written = FormatString(newPath, sizeof(newPath), "%s", path);
    
    char* end = newPath + written - 2;
    while(end >= newPath)
    {
        if(*end == '/')
        {
            *end = 0;
            break;
        }
        
        --end;
    }
    
    char toAppend[64];
    if(add)
    {
        FormatString(toAppend, sizeof(toAppend), "/#%s", taxonomyName);
    }
    else
    {
        FormatString(toAppend, sizeof(toAppend), "/%s", taxonomyName + 1);
    }
    AppendString(newPath, sizeof(newPath), toAppend, StrLen(toAppend));
    
    platformAPI.MoveFileOrFolder(path, newPath);
}

PLATFORM_WORK_CALLBACK(ReceiveNetworkPackets)
{
    ReceiveNetworkPacketWork* work = (ReceiveNetworkPacketWork*) param;
    
    while(true)
    {
        work->ReceiveData(work->network);
    }
}


extern "C" SERVER_NETWORK_STUFF(NetworkStuff)
{
    platformAPI = memory->api;
    
    ServerState* server = memory->server;
#if FORGIVENESS_INTERNAL
    globalDebugTable = memory->debugTable;
#endif
    //
    //
    //ACCEPT
    //
    //
    
    
    
    //platformAPI.net.ReceiveDataAndAccept(&server->clientInterface);
    
    u16 newConnections[16] = {};
    u16 accepted = platformAPI.net.Accept(&server->clientInterface, newConnections, ArrayCount(newConnections));
    
    for(u32 newConnectionIndex = 0; newConnectionIndex < accepted; ++newConnectionIndex)
    {
        u16 connectionSlot = newConnections[newConnectionIndex];
        ServerPlayer* player = FirstFreePlayer(server);
        player->connectionSlot = connectionSlot;
    }
    
    
#if 0        
    //Assert( server->countOtherServers < MAX_OTHER_SERVERS );
    i32 newSocketServer = (int) accept(server->serversHandle, (sockaddr*) &serverComing, &cSize);
    if(newSocketServer > 0)
    {
        NotImplemented;
        server->otherServers[server->countOtherServers++].handle = newSocketServer;
        SendInfoToServer( newSocketServer, server->port, server->userPort );
        unsigned char* receiveBuffer = PushArray( &server->constantPool, unsigned char, BUFFER_SIZE );
        otherServer->receiver = InitializeReceiveBuffer( receiveBuffer, BUFFER_SIZE, 100000 );
    }
#endif
    
    //
    //
    //RECEIVE MESSAGES
    //
    //
    
    u32 challenge = 1111;
    for( u32 playerIndex = 0; 
        playerIndex < MAXIMUM_SERVER_PLAYERS; 
        playerIndex++ )
    {
        ServerPlayer* player = server->players + playerIndex;
        if(player->connectionSlot)
        {
            if(player->allDataFileSent && !player->allPakFileSent)
            {
#if 1
                u32 chunkSize = KiloBytes(1);
                
                char* pakPath = "assets";
                PlatformFileGroup pakGroup = platformAPI.GetAllFilesBegin(PlatformFile_compressedAsset, pakPath);
                
                u32 toSendSize = server->sendPakBufferSize;
                
                for(u32 fileIndex = 0; fileIndex < pakGroup.fileCount && toSendSize > 0; ++fileIndex)
                {
                    PlatformFileHandle handle = platformAPI.OpenNextFile(&pakGroup, pakPath);
                    if(fileIndex == player->pakFileIndex)
                    {
                        
						if(server->editor || handle.name[0] != '#')
						{
							u32 sizeToRead = Min(toSendSize, handle.fileSize - player->pakFileOffset);
							platformAPI.ReadFromFile(&handle, player->pakFileOffset, sizeToRead, server->sendPakBuffer);
                            
							if(player->pakFileOffset == 0)
							{
								char nameWithoutPoint[64];
								GetNameWithoutPoint(nameWithoutPoint, sizeof(nameWithoutPoint), handle.name);
                                
								char uncompressedName[64];
								FormatString(uncompressedName, sizeof(uncompressedName), "%s.upak", nameWithoutPoint);
                                
                                
								SendPakFileHeader(player, uncompressedName, handle.fileSize, chunkSize);
							}
                            
							SendFileChunks(player, server->sendPakBuffer, sizeToRead, chunkSize);
							player->pakFileOffset += sizeToRead;
                            
                            u32 modSizeToRead = sizeToRead;
                            
                            if(modSizeToRead % chunkSize != 0)
                            {
                                modSizeToRead += chunkSize - (sizeToRead % chunkSize);
                            }
                            Assert(modSizeToRead % chunkSize == 0);
                            toSendSize -= modSizeToRead;
						}
						else
						{
							player->pakFileOffset = handle.fileSize;
						}
                        
                        if(player->pakFileOffset >= handle.fileSize)
                        {
                            player->pakFileOffset = 0;
                            if(++player->pakFileIndex == pakGroup.fileCount)
                            {
                                SendAllPakFileSentMessage(player);
                                player->allPakFileSent = true;
                            }
                        }
                    }
                    platformAPI.CloseHandle(&handle);
                }
                platformAPI.GetAllFilesEnd(&pakGroup);
#else
                SendAllPakFileSentMessage(player);
                player->allPakFileSent = true;
#endif
            }
            
            
            
            FlushAllPackets(server, player);
            if(player->connectionClosed)
            {
                platformAPI.net.RecycleConnection(&server->clientInterface, player->connectionSlot);
                player->connectionSlot = 0;
                RecyclePlayer(server, player);
            }
            else
            {
                while(true)
                {
                    NetworkPacketReceived received = platformAPI.net.GetPacketOnSlot(&server->clientInterface, player->connectionSlot);
                    
                    if(received.disconnected)
                    {
                        player->connectionClosed = true;
                    }
                    
                    unsigned char* packetPtr = received.packetPtr;
                    if(!packetPtr)
                    {
                        break;
                    }
                    
                    if(received.info.brokeChain)
                    {
                        InvalidCodePath;
                        //context = 0;
                    }
                    
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
                                player->connectionClosed = true;
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
                            unpack(packetPtr, "L", &clientReq.challenge ); 
							if(challenge == clientReq.challenge)
                            {
                                SendDataFiles(server->editor, player, true, true);
                                player->allDataFileSent = true;
                                //EntitySQL ep = SQLRetriveHero( server->conn, newPlayer->username, newPlayer->heroSlot );
                                SimRegion* region = GetServerRegion( server, 0, 0 );
                                Vec3 P = V3( 0.5f, 10.0f, 0 );
                                
                                TaxonomySlot* slot = NORUNTIMEGetTaxonomySlotByName( region->taxTable, "centaur" );
                                u64 identifier = AddEntity( region, P, slot->taxonomy, 0, PlayerAddEntityParams(player->playerID));
                                
                                
#if 1                             
                                TaxonomySlot* testSlot = NORUNTIMEGetTaxonomySlotByName(region->taxTable, "strength");
                                AddEntity(region, P + V3( 5.0f, 0.0f, 0.0f ), testSlot->taxonomy, 0, EntityFromObject(identifier, 0, 0));
#endif
                                
                                
                                SendGameAccessConfirm(player, server->universeX, server->universeY, identifier, 0, server->elapsedMS5x);
                                
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
                        
                        
                        
                        
                        
                        
                        
                        
                        case Type_NewEditorTab:
                        {
                            if(server->editor)
                            {
                                EditorTabStack* stack = &server->editorStack;
                                stack->counter = 0;
                                stack->previousElementType = EditorElement_Count;
                            }
                        } break;
                        
                        case Type_EditorElement:
                        {
                            if(server->editor)
                            {
                                TaxonomyTable* taxTable = server->activeTable;
                                EditorElement* element;
                                FREELIST_ALLOC(element, taxTable->firstFreeElement, PushStruct(&taxTable->pool, EditorElement));
                                *element = {};
                                
                                packetPtr = unpack(packetPtr, "sLL", element->name, &element->type, &element->flags);
                                if(element->type < EditorElement_List)
                                {
                                    packetPtr =  unpack(packetPtr, "s", element->value);
                                }
                                else
                                {
                                    element->value[0] = 0;
                                    if(element->type == EditorElement_List)
                                    {
                                        packetPtr = unpack(packetPtr, "s", element->elementName);
                                    }
                                }
                                
                                EditorTabStack* stack = &server->editorStack;
                                EditorElement* current;
                                u32 currentStackIndex = 0;
                                
                                if(!stack->counter)
                                {
                                    stack->stack[stack->counter++] = element;
                                    current = element;
                                    stack->result = element;
                                }
                                else
                                {
                                    Assert(stack->counter > 0);
                                    currentStackIndex = stack->counter - 1;
                                    current = stack->stack[currentStackIndex];
                                }
                                
                                
                                if(StrEqual(element->name, "empty"))
                                {
                                    Assert(current->type == EditorElement_List);
                                    current->emptyElement = element;
                                    Assert(stack->counter < ArrayCount(stack->stack));
                                    stack->stack[stack->counter++] = element;
                                }
                                else
                                {
                                    switch(stack->previousElementType)
                                    {
                                        case EditorElement_String:
                                        case EditorElement_Real:
                                        case EditorElement_Signed:
                                        case EditorElement_Unsigned:
                                        {
                                            current->next = element;
                                            stack->stack[currentStackIndex] = element;
                                        } break;
                                        
                                        case EditorElement_Struct:
                                        {
                                            current->firstValue = element;
                                            
                                            Assert(stack->counter < ArrayCount(stack->stack));
                                            stack->stack[stack->counter++] = element;
                                        } break;
                                        
                                        case EditorElement_List:
                                        {
                                            current->firstInList = element;
                                            
                                            Assert(stack->counter < ArrayCount(stack->stack));
                                            stack->stack[stack->counter++] = element;
                                        } break;
                                        
                                        case EditorElement_Count:
                                        {
                                            //editingSlot->tabs[editingSlot->tabCount++] = current;
                                        } break;
                                        
                                        InvalidDefaultCase;
                                    }
                                    
                                    stack->previousElementType = element->type;
                                    
                                }
                                
                            }
                        } break;
                        
                        case Type_PopEditorElement:
                        {
                            if(server->editor)
                            {
                                b32 list;
                                b32 pop;
                                
                                packetPtr = unpack(packetPtr, "ll", &list, &pop);
                                
                                EditorTabStack* stack = &server->editorStack;
                                
                                if(pop)
                                {
                                    Assert(stack->counter > 0);
                                    --stack->counter;
                                }
                                
                                if(list)
                                {
                                    stack->previousElementType = EditorElement_List;
                                }
                                else
                                {
                                    stack->previousElementType = EditorElement_String;
                                }
                            }
                        } break;
                        
                        case Type_ReloadEditingSlot:
                        {
                            if(server->editor)
                            {
                                EditorTabStack* stack = &server->editorStack;
                                TaxonomyTable* taxTable = server->activeTable;
                                
                                u32 taxonomy;
                                u32 tab;
                                packetPtr = unpack(packetPtr, "LL", &taxonomy, &tab);
                                
                                
                                TaxonomySlot* editingSlot = GetSlotForTaxonomy(taxTable, taxonomy);
                                
                                FreeElement(editingSlot->tabs[tab].root);
                                editingSlot->tabs[tab].root = stack->result;
                                stack->counter = 0;
                                
                                for(u32 tabIndex = 0; tabIndex < editingSlot->tabCount; ++tabIndex)
                                {
                                    EditorTab* tabToReload = editingSlot->tabs + tabIndex;
                                    Import(editingSlot, tabToReload->root);
                                }
                            }
                        } break;
                        
                        case Type_SaveAssetFadFile:
                        {
                            if(server->editor)
                            {
                                EditorTabStack* stack = &server->editorStack;
                                TaxonomyTable* taxTable = server->activeTable;
                                
                                char fileName[64];
                                packetPtr = unpack(packetPtr, "s", fileName);
                                
                                char completePath[256];
                                FormatString(completePath, sizeof(completePath), "definition/%s/%s.fad", fileName, fileName);
                                
                                MemoryPool* tempPool = &taxTable->pool;
                                TempMemory fileMemory = BeginTemporaryMemory(tempPool);
                                
                                u32 bufferSize = MegaBytes(2);
                                char* buffer = PushArray(tempPool, char, bufferSize);
                                
                                
                                stack->result;
                                u32 freeSize = bufferSize;
                                WriteElements(buffer, &freeSize, stack->result);
                                u32 written = bufferSize - freeSize;
                                
                                platformAPI.DEBUGWriteFile(completePath, buffer, written);
                                EndTemporaryMemory(fileMemory);
                                
                                
                                FreeElement(stack->result);
                                stack->counter = 0;
                                
                            }
                        } break;
                        
						case Type_SaveSlotTabToFile:
						{
                            if(server->editor)
                            {
                                TaxonomyTable* taxTable = server->activeTable;
                                
                                u32 taxonomy;
                                packetPtr = unpack(packetPtr, "L", &taxonomy);
                                TaxonomySlot* slot = GetSlotForTaxonomy(taxTable, taxonomy);
                                WriteToFile(taxTable, slot);
                            }
						} break;
                        
                        case Type_ReloadAssets:
                        {
                            if(server->editor)
                            {
                                LoadAssets();
                                for(u32 toResetIndex = 0; 
                                    toResetIndex < MAXIMUM_SERVER_PLAYERS; 
                                    toResetIndex++)
                                {
                                    ServerPlayer* toReset = server->players + toResetIndex;
                                    if(toReset->connectionSlot)
                                    {
                                        toReset->allPakFileSent = false;
                                        SendDataFiles(server->editor, player, false, true);
                                        toReset->pakFileIndex = 0;
                                        toReset->pakFileOffset = 0;
                                    }
                                }
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
                                PatchLocalServer(server);
                            }
                        } break;
                        
						case Type_AddTaxonomy:
						{
                            if(server->editor)
                            {
                                u32 parentTaxonomy;
                                char name[32];
                                
                                packetPtr = unpack(packetPtr, "Ls", &parentTaxonomy, name);
                                
                                char destPath[512];
                                char referenceTaxonomyPath[512];
                                
                                BuildTaxonomyDataPath(server->activeTable, parentTaxonomy, name, destPath, sizeof(destPath), referenceTaxonomyPath, sizeof(referenceTaxonomyPath));
                                
                                char finalSource[512];
                                FormatString(finalSource, sizeof(finalSource), "%s/reference", referenceTaxonomyPath);
                                
                                char finalDest[512];
                                FormatString(finalDest, sizeof(finalDest), "%s/%s.fed", destPath, name);
                                
                                if(platformAPI.CreateFolder(destPath))
                                {
                                    platformAPI.CopyFileOrFolder(finalSource, finalDest);
                                    ReloadServer(server);
                                }
                            }
                            
						} break;
                        
                        
						case Type_DeleteTaxonomy:
						{
                            if(server->editor)
                            {
                                char path[512];
                                char ignored[512];
                                
                                u32 toDelete;
                                packetPtr = unpack(packetPtr, "L", &toDelete);
                                
                                TaxonomySlot* toDeleteSlot = GetSlotForTaxonomy(server->activeTable, toDelete);
                                BuildTaxonomyDataPath(server->activeTable, toDelete, "", path, sizeof(path), ignored, sizeof(ignored));
                                
								PoundToNameAndFedFileRecursively(path, toDeleteSlot->name, true);
                                ReloadServer(server);
                            }
						} break;
                        
						case Type_ReviveTaxonomy:
						{
                            if(server->editor)
                            {
                                char path[512];
                                char ignored[512];
                                
                                u32 toDelete;
                                packetPtr = unpack(packetPtr, "L", &toDelete);
                                
                                TaxonomySlot* toDeleteSlot = GetSlotForTaxonomy(server->activeTable, toDelete);
                                BuildTaxonomyDataPath(server->activeTable, toDelete, "", path, sizeof(path), ignored, sizeof(ignored));
                                
                                PoundToNameAndFedFileRecursively(path, toDeleteSlot->name, false);
                                ReloadServer(server);
                            }
						} break;
                        
						case Type_InstantiateTaxonomy:
                        case Type_DeleteEntity:
						{
                            if(server->editor)
                            {
                                if(player->requestCount < ArrayCount(player->requests))
                                {
                                    PlayerRequest* request = player->requests + player->requestCount++;
                                    Assert(received.info.dataSize < ArrayCount(request->data));
                                    
                                    Copy(received.info.dataSize, request->data, received.packetPtr);
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
                        
                        default:
                        {
                            if(player->requestCount < ArrayCount(player->requests))
                            {
                                PlayerRequest* request = player->requests + player->requestCount++;
                                Assert(received.info.dataSize < ArrayCount(request->data));
                                Copy(received.info.dataSize, request->data, received.packetPtr);
                            }
                        } break;
                        
                        
                        
                        
                        
                        
                        
                        
                        
                        
                        
#if 0
                        default:
                        {
#if FORGIVENESS_INTERNAL
                            if( player == server->debugPlayer )
                            {
                                HandleDebugMessage( memory, player, status.packetType, packetPtr );
                            }
#endif
                        } break;
#endif
                        
                    }
                    
#if FORGIVENESS_INTERNAL
                    //platformAPI.PlatformInputRecordingHandlePlayer( server, &player->request );
#endif
                }
            }
        }
    }
    
    for( i32 serverIndex = 0; serverIndex < MAX_OTHER_SERVERS; serverIndex++ )
    {
        
#if 0        
        Server* otherServer = server->otherServers + serverIndex;
        if( otherServer->handle )
        {
            unsigned char* packetPtr = 0;
            ReceiveResult status = {};
            while( packetPtr = ReceiveData( 0, otherServer->handle, &otherServer->receiver, &status, 0 ) )
            {
#if FORGIVENESS_INTERNAL
                if( !server->editorMode )
#endif
                {
                    switch( status.packetType )
                    {
                        case Type_serverInfo:
                        {
                            ServerInfo serverInfo;
                            unpack( packetPtr, "HH", &serverInfo.port, &serverInfo.userPort );
                            
                            otherServer->port = serverInfo.port;
                            otherServer->userPort = serverInfo.userPort;
                            SendInfoToServer( otherServer->handle, server->port, server->userPort );
                            otherServer->accepted = true;
                        } break;
                        
                        case Type_entityUpdate:
                        {
                            NotImplemented();
                        } break;
                        
                        InvalidDefaultCase;
                    }
                }
            }
        }
#endif
    }
}


inline void InitComponents_(MemoryPool* pool, EntityComponentArray* components, EntityComponentType type, u16 componentSize, u32 maxComponentCount, u32 nextFreeOffset)
{
    EntityComponentArray* array = components + type;
    array->nextID = 1;
    array->componentSize = componentSize;
    array->componentCount = 1;
    array->maxComponentCount = maxComponentCount;
    array->components = PushSize(pool, componentSize * maxComponentCount, NoClear());
    array->firstFree = 0;
    array->nextFreeOffset = nextFreeOffset;
}
#define InitComponents(pool, components, type, maxComponentCount) InitComponents_(pool, components, Component_##type, sizeof(type##Component), maxComponentCount, OffsetOf(type##Component, nextFree))


internal void ServerCommonInit(PlatformServerMemory* memory, u32 universeIndex)
{
    platformAPI = memory->api;
    ServerState* server = memory->server;
    if(!memory->server)
    {
        server = memory->server = BootstrapPushStruct(ServerState, worldPool);
    }
    
    Win32ThreadStartup fastStartups[5] = {};
    Win32MakeQueue(&server->fastQueue, ArrayCount(fastStartups), fastStartups);
    
    Win32ThreadStartup slowStartups[1] = {};
    Win32MakeQueue( &server->slowQueue, ArrayCount(slowStartups), slowStartups);
    
    for( u32 taskIndex = 0; 
        taskIndex < ArrayCount( server->tasks ); 
        taskIndex++ )
    {
        TaskWithMemory* task = server->tasks + taskIndex;
        task->beingUsed = false;
    }
    
    for( u32 contextIndex = 0; contextIndex < ArrayCount( server->threadContext ); contextIndex++ )
    {
        RegionWorkContext* context = server->threadContext + contextIndex;
        *context = {};
        context->pool.minimumBlockSize = MegaBytes(128);
    }
    
    MemoryPool* pool = &server->worldPool;
    InitComponents(pool, server->components, Effect, 0xffff);
    InitComponents(pool, server->components, Plant, 0xffff);
    InitComponents(pool, server->components, Object, 0xffff);
    InitComponents(pool, server->components, Fluid, 0xffff);
    InitComponents(pool, server->components, Creature, 0xffff);
    
    
    server->networkPool.allocationFlags = PlatformMemory_NotRestored;
    server->sendPakBufferSize = MegaBytes(1);
    server->sendPakBuffer = PushArray(&server->networkPool, char, server->sendPakBufferSize);
    server->players = PushArray( &server->networkPool, ServerPlayer, MAXIMUM_SERVER_PLAYERS );
    //server->otherServers = PushArray( &server->pool, Server, MAX_OTHER_SERVERS );
    
    
    
    Assert(universeIndex < ArrayCount(globalPorts));
    Assert(universeIndex < ArrayCount(globalUserPorts));
    
    
    u16 maxConnectionCount = 64;
    NetworkConnection* connections = PushArray(&server->networkPool, NetworkConnection, maxConnectionCount);
    Assert(connections);
    for(u16 connectionIndex = 0; connectionIndex < maxConnectionCount; ++connectionIndex)
    {
        NetworkConnection* connection = connections + connectionIndex;
        u32 recvBufferSize = MegaBytes(3);
        connection->appRecv = ForgAllocateNetworkBuffer(&server->networkPool, recvBufferSize);
    }
    
    u16 clientPort = globalUserPorts[universeIndex];
    platformAPI.net.InitServer(&server->clientInterface, clientPort, ForgNetwork_Count, forgNetworkChannelParams, connections, maxConnectionCount);
    
    server->receivePacketWork.network = &server->clientInterface;
    server->receivePacketWork.ReceiveData = platformAPI.net.ReceiveData;
    Win32PushWork(&server->slowQueue, ReceiveNetworkPackets, &server->receivePacketWork);
    
    
    // TODO(Leonardo): get this from main server or from file
    server->currentIdentifier = 1;
    
    server->activeTable = PushStruct(pool, TaxonomyTable);
    server->oldTable = PushStruct(pool, TaxonomyTable);
}

extern "C" SERVER_INITIALIZE(InitializeServer)
{
    // NOTE(Leonardo): sqlite test!
#if 0
    sqlite3 *db;
    char *zErrMsg = 0;
    int rc;
    
    rc = sqlite3_open("test.db", &db);
    
    if( rc ) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    } else {
        fprintf(stderr, "Opened database successfully\n");
    }
    
    //sqlite3_exec(handle, "PRAGMA locking_mode = EXCLUSIVE",0,0,0);
    
    // 'db' is the pointer you got from sqlite3_open*
    // Any (modifying) SQL commands executed here are not committed until at the you call:
    char *sql = "DROP TABLE IF EXISTS Cars; CREATE TABLE Cars(Id INT, Name TEXT, Price INT);";
    rc = sqlite3_exec(db, sql, 0, 0, 0);
    
    
    sqlite3_stmt *stmtInsert = 0;
    sqlite3_prepare_v2(db, "INSERT INTO Cars VALUES(?1, ?2, ?3);", -1, &stmtInsert, NULL);
    
    sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, NULL);
    for( u32 rowIndex = 0; rowIndex < 5000000; ++rowIndex )
    {
        sqlite3_bind_int(stmtInsert, 1, rowIndex );
        sqlite3_bind_text(stmtInsert, 2, "Audi", -1, 0 );
        sqlite3_bind_int(stmtInsert, 3, rowIndex * 3 - 10 );
        rc = sqlite3_step(stmtInsert); 
        sqlite3_reset(stmtInsert);
    }
    
    sqlite3_exec(db, "END TRANSACTION;", NULL, NULL, NULL);
    sqlite3_exec(db, "CREATE INDEX testingIndex ON Cars(Id);", NULL, NULL, NULL);
    
    
    
    sqlite3_stmt *stmt = 0;
    sqlite3_prepare_v2(db, "UPDATE Cars Set Price = 100 where Id = ?1", -1, &stmt, NULL);
    
    
    time_t start2 = time(0);
    sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, NULL);
    for( u32 updateIndex = 0; updateIndex < 1000000; ++updateIndex )
    {
        sqlite3_bind_int(stmt, 1, updateIndex );
        rc = sqlite3_step(stmt);
        sqlite3_reset(stmt);
    }
    sqlite3_exec(db, "END TRANSACTION;", NULL, NULL, NULL);
    
    time_t end2 = time(0);
    double time = difftime(end2, start2) * 1000.0;
    
    sqlite3_close(db);
#endif
    
    ServerCommonInit(memory, universeIndex);
    
    ServerState* server = (ServerState*) memory->server;
    
    server->editor = editor;
    
    TaxonomyTable* table = server->activeTable;
    
    InitTaxonomyReadWrite(table);
    if(server->editor)
    {
        platformAPI.DeleteFileWildcards("assets", "*");
        
        CheckForDefinitionsToMerge(server);
        LoadAssets();
        WriteDataFiles();
    }
    else
    {
        ReadTaxonomiesFromFile();
    }
    
    FillGenerationData(server);
    BuildWorld(server, universeIndex);
    
}

struct SimulateWorldRegionWork
{
    RegionWorkContext* context;
    SimRegion* region;
    TaskWithMemory* task;
};

PLATFORM_WORK_CALLBACK(SimulateWorldRegionThreaded)
{
    SimulateWorldRegionWork* work = ( SimulateWorldRegionWork* ) param;
    SimRegion* region = work->region;
    region->context = work->context;
    
    SimulateRegionServer(region, &work->task->pool);
    EndTaskWithMemory(work->task);
    
    CompletePastWritesBeforeFutureWrites;
    region->context->used = false;
    region->simulating = false;
    
}

inline RegionWorkContext* GetFreeThreadContext( ServerState* server )
{
    RegionWorkContext* result = 0;
    for( u32 contextIndex = 0; contextIndex < ArrayCount( server->threadContext ); contextIndex++ )
    {
        RegionWorkContext* context = server->threadContext + contextIndex;
        if( !context->used )
        {
            result = context;
            break;
        }
    }
    
    return result;
}

internal b32 SimulateWorldRegion(ServerState* server, SimRegion* region)
{
    b32 result = false;
    RegionWorkContext* context = GetFreeThreadContext(server);
    if(context)
    {
        TaskWithMemory* task = BeginTaskWithMemory(server->tasks, ArrayCount(server->tasks), false);
        if(task)
        {
            result = true;
            region->simulating = true;
            context->used = true;
            
            SimulateWorldRegionWork* work = PushStruct(&task->pool, SimulateWorldRegionWork);
            work->region = region;
            work->task = task;
            work->context = context;
            
            Win32PushWork(&server->fastQueue, SimulateWorldRegionThreaded, work);
        }
    }
    return result;
}

inline void FlushSimulationQueue(ServerState* server, b32 checkNeighboors)
{
    while(server->regionSlotCount)
    {
        for(u32 slotIndex = 0; slotIndex < server->regionSlotCount; slotIndex++)
        {
            RegionSlot* slot = server->regionSlots + slotIndex;
            if(!checkNeighboors || AllNeighBorsFinished(server, slot->region))
            {
                if(SimulateWorldRegion(server, slot->region))
                {
                    *slot = server->regionSlots[--server->regionSlotCount];
                }
            }
        }
    }
    
    Assert(server->regionSlotCount == 0);
    Win32CompleteQueueWork(&server->fastQueue);
}

extern "C" SERVER_SIMULATE_WORLDS(SimulateWorlds)
{
    TIMED_FUNCTION();
    platformAPI = memory->api;
    
    ServerState* server = memory->server;
#if FORGIVENESS_INTERNAL
    globalDebugTable = memory->debugTable;
    {
        DEBUG_DATA_BLOCK( Server_Test );
        DEBUG_DATA_BLOCK( variables );
        DEBUG_B32( server->testing );
        DEBUG_VALUE( server->testing2 );
    }
    
    {
        DEBUG_DATA_BLOCK(ServerProfile);
        DEBUG_UI_ELEMENT(DebugType_TopClockList, GameUpdateAndRender);
    }
    
#endif
    
    r32 timeToAdvance = secondElapsed;
    if(server->gamePaused)
    {
        timeToAdvance = 0;
    }
    
    b32 canAdvance = true;
    
#if FORGIVENESS_INTERNAL
    if( server->fixedTimestep )
    {
        canAdvance = server->canAdvance;
        server->canAdvance = false;
    }
    server->simulationStepDone = canAdvance;
#endif
    
    if(canAdvance)
    {
        for(u32 deletedEntityIndex = 0; deletedEntityIndex < server->deletedEntityCount; ++deletedEntityIndex)
        {
            DeletedEntity* deleted = server->deletedEntities + deletedEntityIndex;
            for(u32 componentIndex = 0; componentIndex < Component_Count; ++componentIndex)
            {
                u32 ID = deleted->IDs[componentIndex];
                if(ID)
                {
                    FreeComponent(server->components, (EntityComponentType) componentIndex, ID);
                }
            }
        }
        server->deletedEntityCount = 0;
        
        for(u32 newEntityIndex = 0; newEntityIndex < server->newEntityCount; ++newEntityIndex)
        {
            NewEntity* newEntity = server->newEntities + newEntityIndex;
            AddEntitySingleThread(newEntity->region, newEntity->taxonomy, newEntity->P, newEntity->identifier, newEntity->params);
        }
        server->newEntityCount = 0;
        
        
        
        
        
        
        // NOTE(Leonardo): we first simulate all the mirror region, so that we dispatch all the update first, and then we update all the other regions
        u32 realServerRegionSpan = SERVER_REGION_SPAN + 2;
        for(u32 Y = 0; Y < realServerRegionSpan; Y++)
        {
            for(u32 X = 0; X < realServerRegionSpan; X++)
            {
                SimRegion* region = GetServerRegionRaw(server, X, Y);
                if(region->border == Border_Mirror)
                {
                    region->timeToUpdate += timeToAdvance;	
                    if(MustBeUpdated(server, region))
                    {
                        // NOTE(Leonardo): single thread simulation toggle
#if 0
                        region->context = server->threadContext + 0;
                        SimulateRegionServer(region, &server->pool);
#else
                        if(!SimulateWorldRegion(server, region))
                        {
                            Assert(server->regionSlotCount < ArrayCount( server->regionSlots));
                            RegionSlot* slot = server->regionSlots + server->regionSlotCount++;
                            slot->region = region;
                        }
#endif
                    }
                }
            }
        }
        FlushSimulationQueue(server, false);
        
        
        for(u32 Y = 0; Y < realServerRegionSpan; Y++)
        {
            for(u32 X = 0; X < realServerRegionSpan; X++)
            {
                SimRegion* region = GetServerRegionRaw(server, X, Y);
                if(region->border != Border_Mirror)
                {
                    region->timeToUpdate += timeToAdvance;	
                    if(MustBeUpdated(server, region))
                    {
                        // NOTE(Leonardo): single thread simulation toggle
#if 0
                        region->context = server->threadContext + 0;
                        SimulateRegionServer(region, &server->pool);
#else
                        if(!AllNeighBorsFinished(server, region) ||
                           !SimulateWorldRegion(server, region))
                        {
                            Assert(server->regionSlotCount < ArrayCount( server->regionSlots));
                            RegionSlot* slot = server->regionSlots + server->regionSlotCount++;
                            slot->region = region;
                        }
#endif
                    }
                }
            }
        }
        
        FlushSimulationQueue(server, true);
        
        
        for(u32 contextIndex = 0; contextIndex < ArrayCount(server->threadContext); contextIndex++)
        {
            RegionWorkContext* context = server->threadContext + contextIndex;
            Assert(!context->used);
        }
    }
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