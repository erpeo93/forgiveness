#include <winsock2.h>
#include <windows.h>
#include <stdio.h>

#include <time.h>

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
#include "forg_taxonomy.cpp"
#include "forg_world_generation.cpp"
#include "forg_consideration.cpp"
#include "forg_network_server.cpp"
#include "forg_editor.cpp"
#include "forg_inventory.cpp"
#include "forg_memory.cpp"
#include "forg_AI.cpp"
#include "forg_action_effect.cpp"
#include "forg_crafting.cpp"
#include "server_world.cpp"
#include "forg_fluid.cpp"
#include "forg_region.cpp"

#pragma comment( lib, "wsock32.lib" )

#if FORGIVENESS_INTERNAL
DebugTable* globalDebugTable;

internal void HandleDebugMessage( PlatformServerMemory* memory, ServerPlayer* player, u32 packetType, unsigned char* packetPtr )
{
    ServerState* server = ( ServerState* ) memory->server;
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
            unpack( packetPtr, "QQLHCQQ", &editEvent->clock, &editEvent->GUID, &editEvent->threadID, &editEvent->coreIndex, &editEvent->type, &editEvent->overNetwork[0], &editEvent->overNetwork[1] );
        } break;
        
    }
}
#endif

inline ForgFile* FindFile(ServerState* server, ForgFile** firstFilePtr, char* fileName)
{
    ForgFile* result = 0;
    
    char nameWithoutPoint[64];
    GetNameWithoutPoint(nameWithoutPoint, sizeof(nameWithoutPoint), fileName);
    
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

inline void LoadAssetsSync(ServerState* server)
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

inline void LoadAssetsAsync(ServerState* server)
{
    server->reloadingAssets = true;
    server->assetBuilder = platformAPI.DEBUGExecuteSystemCommand(".", "../asset_builder.exe", "");
}


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

inline void LoadStaticData()
{
    InitDefaultStateMachine();
    ReadSynthesisRules();
}


internal void FillGenerationData(ServerState* server)
{
    b32 freeTabs = !server->editor;
    ImportAllFiles((u32) 0xffffffff, freeTabs, 0);
    LoadStaticData();
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
                for(u32 entityIndex = 0; entityIndex < block->countEntity; ++entityIndex)
                {
                    u32 ID = block->entityIDs[entityIndex];
                    SimEntity* source = GetEntity(server, ID);
                    TranslateSimEntity(server, source);
                }
                
                block = block->next;
            }
            
            chunk = chunk->next;
        }
    }
}

inline void TranslateServerPlayers(ServerState* server, b32 sendOnlyTaxonomiesFile)
{
    for(u32 playerIndex = 0; playerIndex < MAXIMUM_SERVER_PLAYERS; ++playerIndex)
    {
        ServerPlayer* player = server->players + playerIndex;
        if(player->connectionSlot)
        {
            TranslatePlayer(server->oldTable, server->activeTable, player);
            
            DataFileSentType mode = sendOnlyTaxonomiesFile ? DataFileSent_OnlyTaxonomies : DataFileSent_Everything;
            if(sendOnlyTaxonomiesFile)
            {
                SendSpecificFile(player, "taxonomies.fed");
            }
            else
            {
                SendAllDataFiles(server->editor, player, mode);
            }
            
            SendAllDataFileSentMessage(player, mode);
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


inline void TranslateServer(ServerState* server, b32 onlyTaxonomies)
{
    TranslateServerChunks(server);
    TranslateServerPlayers(server, onlyTaxonomies);
    InvalidateAllHashUpdates(server);
}

inline void ReloadServer(ServerState* server)
{
    SwapTables(server);
    WriteDataFilesAndTaxonomies();
    FillGenerationData(server);
    TranslateServer(server, false);
}

inline void ReloadServerTaxonomies(ServerState* server)
{
    SwapTables(server);
    WriteDataFilesAndTaxonomies();
    CopyAndLoadTabsFromOldTable(server->oldTable);
    LoadStaticData();
    TranslateServer(server, true);
}

inline void PoundToNameAndFedFileRecursively(char* path, char* taxonomyName, b32 add)
{
    char originalFed[512];
    char newFed[512];
    
    FormatString(originalFed, sizeof(originalFed), "%s%s.fed", path, taxonomyName);
    
    if(add)
    {
        FormatString(newFed, sizeof(originalFed), "%s#%s.fed", path, taxonomyName);
    }
    else
    {
        FormatString(newFed, sizeof(originalFed), "%s%s.fed", path, taxonomyName + 1);
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
            FormatString(childPath, sizeof(childPath), "%s%s", path, folderName);
            PoundToNameAndFedFileRecursively(childPath, folderName, add);
        }
    }
    
    char newPath[512];
    u32 written = (u32) FormatString(newPath, sizeof(newPath), "%s", path);
    
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


inline void CreateNewTaxonomy(ServerState* server, char* destPath, char* sourceFile, char* destFile, char* destFileName)
{
	if(platformAPI.CreateFolder(destPath))
    {
		platformAPI.CopyFileOrFolder(sourceFile, destFile);
		ReloadServerTaxonomies(server);
        
        char assetFileName[128];
        FormatString(assetFileName, sizeof(assetFileName), "assets/%s", destFileName);
        platformAPI.CopyFileOrFolder(destFile, assetFileName);
        ImportSpecificFile(0xffffffff, !server->editor, destFile);
        
		for(u32 playerIndex = 0; playerIndex < MAXIMUM_SERVER_PLAYERS; ++playerIndex)
		{
			ServerPlayer* playerToSend = server->players + playerIndex;
			if(playerToSend->connectionSlot)
			{
				SendSpecificFile(playerToSend, destFileName);
                SendAllDataFileSentMessage(playerToSend, DataFileSent_OnlyTaxonomies);
			}
		}
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
                    SendAllDataFiles(server->editor, player, DataFileSent_Everything);
                    SendAllDataFileSentMessage(player, DataFileSent_Everything);
                    player->allDataFileSent = true;
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
                
                packetPtr = unpack(packetPtr, "sLLL", element->name, &element->type, &element->flags, &element->versionNumber);
                if(element->type < EditorElement_List)
                {
                    packetPtr =  unpack(packetPtr, "s", element->value);
                }
                else
                {
                    element->value[0] = 0;
                    
                    if(element->type == EditorElement_Text)
                    {
                        EditorTextBlock* text;
                        FREELIST_ALLOC(text, taxTable->firstFreeEditorText, PushStruct(&taxTable->pool, EditorTextBlock));
                        element->text = text;
                        packetPtr = unpack(packetPtr, "s", text->text);
                        
                    }
                    else if(element->type == EditorElement_List)
                    {
                        packetPtr = unpack(packetPtr, "ss", element->elementName, element->labelName);
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
                        case EditorElement_Text:
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
                
                FreeElementTable(editingSlot->tabs[tab].root, true);
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
                
                
                FreeElementTable(stack->result, true);
                stack->counter = 0;
                
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
                LoadAssetsAsync(server);
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
                char finalDest[512];
                char destFilename[64];
                FormatString(finalSource, sizeof(finalSource), "%s/reference", referenceTaxonomyPath);
                FormatString(destFilename, sizeof(destFilename), "%s.fed", name);
                FormatString(finalDest, sizeof(finalDest), "%s/%s", destPath, destFilename);
                
				CreateNewTaxonomy(server, destPath, finalSource, finalDest, destFilename);
            }
            
        } break;
        
		case Type_CopyTaxonomy:
		{
			if(server->editor)
			{
				u32 brotherTaxonomy;
                packetPtr = unpack(packetPtr, "L", &brotherTaxonomy);
                
                
                TaxonomyTable* taxTable = server->activeTable;
				u32 parentTaxonomy = GetParentTaxonomy(taxTable, brotherTaxonomy);
                
                TaxonomySlot* brotherSlot = GetSlotForTaxonomy(taxTable, brotherTaxonomy);
				char name[64];
                char newName[64];
                FormatString(name, sizeof(name), "%s", brotherSlot->name);
                FormatString(newName, sizeof(newName), "%s_copy", brotherSlot->name);
                
                TaxonomySlot* test = NORUNTIMEGetTaxonomySlotByName(taxTable, newName);
                if(!test)
                {
                    char destPath[512];
                    char sourcePath[512];
                    char ignoredPath[512];
                    
                    BuildTaxonomyDataPath(server->activeTable, parentTaxonomy, newName, destPath, sizeof(destPath), ignoredPath, sizeof(ignoredPath));
                    BuildTaxonomyDataPath(server->activeTable, parentTaxonomy, name, sourcePath, sizeof(sourcePath), ignoredPath, sizeof(ignoredPath));
                    
                    char finalDest[512];
                    char finalSource[512];
                    char destFilename[64];
                    FormatString(finalSource, sizeof(finalSource), "%s/%s.fed", sourcePath, name);
                    
                    FormatString(destFilename, sizeof(destFilename), "%s.fed", newName);
                    FormatString(finalDest, sizeof(finalDest), "%s/%s", destPath, destFilename);
                    
                    CreateNewTaxonomy(server, destPath, finalSource, finalDest, destFilename);
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
                
                ReloadServerTaxonomies(server);
                
                for(u32 playerIndex = 0; playerIndex < MAXIMUM_SERVER_PLAYERS; ++playerIndex)
                {
                    ServerPlayer* playerToSend = server->players + playerIndex;
                    if(playerToSend->connectionSlot)
                    {
                        SendAllDataFileSentMessage(player, DataFileSent_OnlyTaxonomies);
                    }
                }
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
                
                TaxonomySlot* toReviveSlot = GetSlotForTaxonomy(server->activeTable, toDelete);
                BuildTaxonomyDataPath(server->activeTable, toDelete, "", path, sizeof(path), ignored, sizeof(ignored));
                
                PoundToNameAndFedFileRecursively(path, toReviveSlot->name, false);
                ReloadServerTaxonomies(server);
                
                char* toCopy = toReviveSlot->name;
                Assert(toCopy[0] == '#');
                toCopy += 1;
                char toSendName[64];
                FormatString(toSendName, sizeof(toSendName), "%s.fed", toCopy);
                
                ImportSpecificFile(0xffffffff, !server->editor, toSendName);
                for(u32 playerIndex = 0; playerIndex < MAXIMUM_SERVER_PLAYERS; ++playerIndex)
                {
                    ServerPlayer* playerToSend = server->players + playerIndex;
                    if(playerToSend->connectionSlot)
                    {
                        SendSpecificFile(playerToSend, toSendName);
                        SendAllDataFileSentMessage(playerToSend, DataFileSent_OnlyTaxonomies);
                    }
                }
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
    
    
    if(server->reloadingAssets)
    {
        PlatformProcessState assetBuilderState = platformAPI.DEBUGGetProcessState(server->assetBuilder);
        if(!assetBuilderState.isRunning)
        {
            AddAllPakFileHashes(server);
            server->reloadingAssets = false;
            for(u32 toResetIndex = 0; 
                toResetIndex < MAXIMUM_SERVER_PLAYERS; 
                toResetIndex++)
            {
                ServerPlayer* toReset = server->players + toResetIndex;
                if(toReset->connectionSlot)
                {
                    SendAllDataFiles(server->editor, toReset, DataFileSent_OnlyAssets);
                    
                    toReset->allPakFileSent = false;
                    toReset->pakFileIndex = 0;
                    toReset->pakFileOffset = 0;
                }
            }
        }
        
    }
    
    //
    //
    //RECEIVE MESSAGES
    //
    //
    
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
                u32 toSendSize = MegaBytes(1);
                
                char* pakPath = "assets";
                PlatformFileGroup pakGroup = platformAPI.GetAllFilesBegin(PlatformFile_compressedAsset, pakPath);
                
                
                for(u32 fileIndex = 0; fileIndex < pakGroup.fileCount && toSendSize > 0; ++fileIndex)
                {
                    PlatformFileHandle handle = platformAPI.OpenNextFile(&pakGroup, pakPath);
                    
                    if(fileIndex == player->pakFileIndex)
                    {
                        ForgFile* serverFile = FindFile(server, &server->files, handle.name);
                        ForgFile* playerFile = FindFile(server, &player->files, handle.name);
                        
                        Assert(serverFile->hash);
                        if((serverFile->hash != playerFile->hash))
                        {
                            if(server->editor || handle.name[0] != '#')
                            {
                                if(player->pakFileOffset == 0)
                                {
                                    char nameWithoutPoint[64];
                                    GetNameWithoutPoint(nameWithoutPoint, sizeof(nameWithoutPoint), handle.name);
                                    
                                    char uncompressedName[64];
                                    FormatString(uncompressedName, sizeof(uncompressedName), "%s.upak", nameWithoutPoint);
                                    SendPakFileHeader(player, uncompressedName, handle.fileSize, chunkSize);
                                }
                                
                                u32 sizeToRead = Min(toSendSize, handle.fileSize - player->pakFileOffset);
                                TempMemory fileMemory = BeginTemporaryMemory(&server->scratchPool);
                                
                                char* buffer = (char*) PushSize(&server->scratchPool, sizeToRead);
                                
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
                            }
                            else
                            {
                                player->pakFileOffset = handle.fileSize;
                            }
                        }
                        else
                        {
                            SendDontDeleteFile(player, handle.name);
                            player->pakFileOffset = handle.fileSize;
                        }
                        
                        
                        
                        if(player->pakFileOffset >= handle.fileSize)
                        {
                            playerFile->hash = serverFile->hash;
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
            
            server->elapsedTime = 0.1f;
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
        context->pool.minimumBlockSize = MegaBytes(32);
    }
    
#define MAX_COMPONENTS Kilo(64)
    MemoryPool* pool = &server->worldPool;
    InitComponents(pool, server->components, Effect, MAX_COMPONENTS);
    InitComponents(pool, server->components, Plant, MAX_COMPONENTS);
    InitComponents(pool, server->components, Container, MAX_COMPONENTS);
    InitComponents(pool, server->components, Fluid, MAX_COMPONENTS);
    InitComponents(pool, server->components, Creature, MAX_COMPONENTS);
    
    
    server->networkPool.allocationFlags = PlatformMemory_NotRestored;
    server->players = PushArray( &server->networkPool, ServerPlayer, MAXIMUM_SERVER_PLAYERS );
    //server->otherServers = PushArray( &server->pool, Server, MAX_OTHER_SERVERS );
    
    
    
    Assert(universeIndex < ArrayCount(globalPorts));
    Assert(universeIndex < ArrayCount(globalUserPorts));
    
    
    u16 maxConnectionCount = 2;
    NetworkConnection* connections = PushArray(&server->networkPool, NetworkConnection, maxConnectionCount);
    
    u16 clientPort = globalUserPorts[universeIndex];
    platformAPI.net.InitServer(&server->clientInterface, clientPort, connections, maxConnectionCount);
    
    server->receivePacketWork.network = &server->clientInterface;
    server->receivePacketWork.ReceiveData = platformAPI.net.ReceiveData;
    Win32PushWork(&server->slowQueue, ReceiveNetworkPackets, &server->receivePacketWork);
    
    
    // TODO(Leonardo): get this from main server or from file
    server->currentIdentifier = 1;
    server->entityCount = 1;
    
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
        LoadAssetsSync(server);
        WriteDataFilesAndTaxonomies();
    }
    else
    {
        ReadTaxonomiesFromFile();
    }
    AddAllPakFileHashes(server);
    
    server->worldSeed = (u32) time(0);
    FillGenerationData(server);
    BuildWorld(server, GenerateWorld_OnlyChunks);
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
    
    if(server->generateMode)
    {
        BuildWorld(server, server->generateMode);
        server->generateMode = GenerateWorld_None;
    }
    
    if(canAdvance)
    {
        for(DeletedEntity* deleted = server->firstDeletedEntity; deleted;)
        {
            DeletedEntity* next = deleted->next;
            
            if(deleted->entityID)
            {
                FreeEntity* freeEntity;
                FREELIST_ALLOC(freeEntity, server->firstFreeFreeEntity, PushStruct(&server->worldPool, FreeEntity));
                freeEntity->ID = deleted->entityID;
                FREELIST_INSERT(freeEntity, server->firstFreeEntity);
            }
            
            for(u32 componentIndex = 0; componentIndex < Component_Count; ++componentIndex)
            {
                u32 ID = deleted->IDs[componentIndex];
                if(ID)
                {
                    FreeComponent(server->components, (EntityComponentType) componentIndex, ID);
                }
            }
            
			FREELIST_DEALLOC(deleted, server->firstFreeDeletedEntity);
			deleted = next;
        }
        
        server->firstDeletedEntity = 0;
        for(NewEntity* newEntity = server->firstNewEntity; newEntity; )
        {
			NewEntity* next = newEntity->next;
            AddEntitySingleThread(newEntity->region, newEntity->taxonomy, newEntity->P, newEntity->identifier, newEntity->gen, newEntity->params);
            
			FREELIST_DEALLOC(newEntity, server->firstFreeNewEntity);
			newEntity = next;
        }
        
		server->firstNewEntity = 0;
        
        
        
        // NOTE(Leonardo): advance server "status"
        server->seasonTime += timeToAdvance;
        if(server->seasonTime >= SEASON_DURATION)
        {
            server->seasonTime = 0;
            server->season = (WorldSeason) ((server->season == Season_Count - 1) ? 0 : server->season + 1);
        }
        server->seasonLerp = Clamp01MapToRange(0.5f * SEASON_DURATION, server->seasonTime, SEASON_DURATION);
        
        
        
        // NOTE(Leonardo): spawn new stuff
        RegionWorkContext* context = server->threadContext + 0;
        context->immediateSpawn = true;
        
        WorldGeneratorDefinition* generator = server->generator;
        if(generator)
        {
            for(TaxonomyTileTimerSpawn* spawn = generator->firstTimerSpawn; spawn; spawn = spawn->next)
            {
                spawn->timer += timeToAdvance;
                if(spawn->timer >= spawn->destTimer)
                {
                    spawn->timer = 0;
                    u32 counter = 0;
                    
                    TaxonomyTable* taxTable = server->activeTable;
                    RandomSequence* seq = &server->randomSequence;
                    
                    while(counter < 1000)
                    {
                        u32 regionX = RandomChoice(seq, SERVER_REGION_SPAN);
                        u32 regionY = RandomChoice(seq, SERVER_REGION_SPAN);
                        SimRegion* region = GetServerRegion(server, regionX, regionY);
                        region->context = context;
                        
                        Vec3 P = V3(Hadamart(RandomBilV2(seq), V2(server->regionSpan, server->regionSpan)), 0);
                        
                        r32 waterLevel;
                        u32 tileTaxonomy = GetTileTaxonomyFromRegionP(server, region, P, &waterLevel);
                        if(tileTaxonomy == spawn->taxonomy && waterLevel > WATER_LEVEL)
                        {
                            SpawnFromTileAssociation(region, spawn->firstAssociation, spawn->totalWeight, P, seq);
                            break;
                        }
                        ++counter;
                    }
                }
            }
        }
        
        
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
                        SimulateRegionServer(region, &server->testPool);
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
                        SimulateRegionServer(region, &server->testPool);
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
            RegionWorkContext* contextToFree = server->threadContext + contextIndex;
            Assert(!contextToFree->used);
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