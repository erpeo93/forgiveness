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

internal void SendNetworkData(void* data, u16 size, GuaranteedDelivery deliveryType)
{
    NetworkSendParams params = {};
    params.guaranteedDelivery = deliveryType;
    platformAPI.net.QueuePacket(clientNetwork->network, 0, params, data, size);
}

inline void FlushAllQueuedPackets(r32 timeToAdvance)
{
    platformAPI.net.FlushSendQueue(clientNetwork->network, 0, timeToAdvance);
}


inline void CloseAndSend(unsigned char* buff_, unsigned char* buff, GuaranteedDelivery deliveryType)
{
    ForgNetworkApplicationData data;
    
    if(deliveryType == GuaranteedDelivery_Ordered)
    {
        data = clientNetwork->nextSendOrderedApplicationData;
        data.flags = ForgNetworkFlag_Ordered;
        clientNetwork->nextSendOrderedApplicationData.index++;
    }
    else
    {
        data = clientNetwork->nextSendStandardApplicationData;
        data.flags = 0;
        clientNetwork->nextSendStandardApplicationData.index++;
    }
    
    
    unsigned char* indexDest = buff_;
    ForgPackApplicationData(indexDest, data);
    
    u16 totalSize = ForgEndPacket_( buff_, buff);
    SendNetworkData(buff_, totalSize, deliveryType);
}

#define StartPacket(type) unsigned char buff_[1024]; unsigned char* buff = buff_ + sizeof(ForgNetworkApplicationData);buff = ForgPackHeader( buff, Type_##type);

#define Pack(formatString, ...) buff += pack(buff, formatString, ##__VA_ARGS__)
#define Unpack(formatString, ...) packetPtr = unpack(packetPtr, formatString, ##__VA_ARGS__)

#define CloseAndSendStandardPacket() CloseAndSend(buff_, buff, GuaranteedDelivery_None)
#define CloseAndSendGuaranteedPacket() CloseAndSend(buff_, buff, GuaranteedDelivery_Guaranteed)
#define CloseAndSendOrderedPacket() CloseAndSend(buff_, buff, GuaranteedDelivery_Ordered)



internal void LoginRequest(i32 password)
{
    StartPacket(login);
    Pack("l", password);
    CloseAndSendOrderedPacket();
}

internal void GameAccessRequest(u32 challenge)
{
    StartPacket(gameAccess);
    Pack("L", challenge);
    CloseAndSendOrderedPacket();
}

#if FORGIVENESS_INTERNAL
internal void SendInputRecordingMessage( b32 recording, b32 startAutomatically )
{
    if( clientNetwork )
    {
        StartPacket(InputRecording);
        Pack("ll", recording, startAutomatically);
        CloseAndSendOrderedPacket();
    }
}
#endif

internal void SendUpdate(Vec3 acceleration)
{
    StartPacket(ActionRequest);
    Pack("V", acceleration);
    CloseAndSendStandardPacket();
}

internal void SendEquipRequest(u64 sourceContainerID, u8 objectIndex)
{
    StartPacket(EquipRequest);
    
    Pack("QC", sourceContainerID, objectIndex);
    
    CloseAndSendOrderedPacket();
}

internal void SendDisequipRequest(u32 slotIndex, u64 destContainerID, u8 destObjectIndex)
{
    StartPacket(DisequipRequest);
    
    Pack("LQC", slotIndex, destContainerID, destObjectIndex);
    
    CloseAndSendOrderedPacket();
}

internal void SendDropRequest(u64 sourceContainerID, u8 objectIndex)
{
    StartPacket(DropRequest);
    
    Pack("QC", sourceContainerID, objectIndex);
    
    CloseAndSendOrderedPacket();
}

internal void SendMoveRequest(u64 sourceContainerID, u8 objectIndex, u64 destContainerID, u8 destObjectIndex)
{
    
    StartPacket(MoveRequest);
    
    Pack("QCQC", sourceContainerID, objectIndex, destContainerID, destObjectIndex);
    
    CloseAndSendOrderedPacket();
}

internal void SendSwapRequest(u64 sourceContainerID, u8 sourceObjectIndex)
{
    StartPacket(SwapRequest);
    
    Pack("QC", sourceContainerID, sourceObjectIndex);
    
    CloseAndSendOrderedPacket();
}

internal void SendDragEquipmentRequest(u32 slotIndex)
{
    StartPacket(DragEquipmentRequest);
    
    Pack("L", slotIndex);
    
    CloseAndSendOrderedPacket();
}

internal void SendEquipDraggingRequest(u32 slotIndex)
{
    StartPacket(EquipDraggingRequest);
    
    Pack("L", slotIndex);
    
    CloseAndSendOrderedPacket();
}


#if 0
internal void SendCraftRequest(u32 taxonomy, GenerationData gen)
{
    StartPacket(CraftRequest);
    
    Pack("LQ", taxonomy, gen.generic);
    
    CloseAndSendOrderedPacket();
}
#endif

internal void SendCraftFromInventoryRequest(u64 containerID, u32 objectIndex)
{
    StartPacket(CraftFromInventoryRequest);
    
    Pack("QL", containerID, objectIndex);
    
    CloseAndSendOrderedPacket();
}

internal void SendActiveSkillRequest(u32 taxonomy)
{
    StartPacket(ActiveSkillRequest);
    Pack("L", taxonomy);
    CloseAndSendOrderedPacket();
}

internal void SendPassiveSkillRequest(u32 taxonomy)
{
    StartPacket(PassiveSkillRequest);
    Pack("L", taxonomy);
    CloseAndSendOrderedPacket();
}

#if 0
internal void SendReleaseDraggingRequest()
{
    StartPacket(ReleaseDraggingRequest);
    CloseAndSendOrderedPacket();
}
#endif

internal void SendUnlockSkillCategoryRequest(u32 taxonomy)
{
    StartPacket(UnlockSkillCategoryRequest);
    
    Pack("L", taxonomy);
    
    CloseAndSendOrderedPacket();
}

internal void SendSkillLevelUpRequest(u32 taxonomy)
{
    StartPacket(SkillLevelUpRequest);
    Pack("L", taxonomy);
    CloseAndSendOrderedPacket();
}

internal void SendLearnRequest(u64 containerID, u32 objectIndex)
{
    StartPacket(LearnRequest);
    Pack("QL", containerID, objectIndex);
    CloseAndSendOrderedPacket();
}

internal void SendConsumeRequest(u64 containerID, u32 objectIndex)
{
    StartPacket(ConsumeRequest);
    Pack("QL", containerID, objectIndex);
    CloseAndSendOrderedPacket();
}

inline void SendMovePlayerRequest(Vec3 offset)
{
    StartPacket(MovePlayerInOtherRegion);
    Pack("V", offset);
    CloseAndSendOrderedPacket();
}

#define SendOrderedMessage(message) {StartPacket(message); CloseAndSendOrderedPacket();}

inline void SendFileHash(u16 type, u64 subtypeHash, u64 hash)
{
    StartPacket(FileHash);
    Pack("HQQ", type, subtypeHash, hash);
    CloseAndSendOrderedPacket();
}

internal void SendFileHeaderAck(u32 index)
{
    StartPacket(FileHeader);
    Pack("L", index);
    CloseAndSendGuaranteedPacket();
}

internal void SendSpawnRequest(UniversePos P, AssetID definitionID)
{
    StartPacket(SpawnEntity);
    Pack("LHllV", definitionID.subtypeHashIndex, definitionID.index, P.chunkX, P.chunkY, P.chunkOffset);
    CloseAndSendGuaranteedPacket();
}

internal void SendMoveChunkZRequest()
{
    StartPacket(MoveChunkZ);
    CloseAndSendGuaranteedPacket();
}

internal void SendRecreateWorldRequrest(b32 createEntities, UniversePos P)
{
    StartPacket(RecreateWorld);
    Pack("llllV", createEntities, P.chunkX, P.chunkY, P.chunkZ, P.chunkOffset);
    CloseAndSendGuaranteedPacket();
}

internal u32 ServerClientIDMappingHashIndex(GameModeWorld* worldMode, EntityID ID)
{
    u32 result = 0;
    return result;
}

internal EntityID GetClientIDMapping(GameModeWorld* worldMode, EntityID serverID)
{
    EntityID result = {};
    u32 hashIndex = ServerClientIDMappingHashIndex(worldMode, serverID);
    for(ServerClientIDMapping* mapping = worldMode->mappings[hashIndex]; mapping; mapping = mapping->next)
    {
        if(AreEqual(mapping->serverID, serverID))
        {
            result = mapping->clientID;
            break;
        }
    }
    
    return result;
}

internal void AddClientIDMapping(GameModeWorld* worldMode, EntityID serverID, EntityID clientID)
{
    ServerClientIDMapping* mapping;
    FREELIST_ALLOC(mapping, worldMode->firstFreeMapping, PushStruct(worldMode->persistentPool, ServerClientIDMapping));
    mapping->serverID = serverID;
    mapping->clientID = clientID;
    
    u32 hashIndex = ServerClientIDMappingHashIndex(worldMode, serverID);
    FREELIST_INSERT(mapping, worldMode->mappings[hashIndex]);
}

STANDARD_ECS_JOB_CLIENT(DeleteEntities)
{
    FreeArchetype(worldMode, &ID);
}

internal void CollateDebugEvent(DebugState* debugState, DebugCollationState* collation, DebugEvent* event);
internal void DispatchApplicationPacket(GameState* gameState, GameModeWorld* worldMode, unsigned char* packetPtr, u16 dataSize)
{
    ClientPlayer* player = &worldMode->player;
    EntityID currentID = {};
    
    
    u16 readSize = 0;
    u16 toReadSize = dataSize;
    
    u32 lastReceived = 0;
    while(readSize < toReadSize)
    {
        unsigned char* oldPacketPtr = packetPtr;
        ForgNetworkHeader header;
        packetPtr = ForgUnpackHeader(packetPtr, &header);
        
        switch(header.packetType)
        {
            case Type_login:
            {
#if 0
                char* server = "forgiveness.hopto.org";
#else
                char* server = "127.0.0.1";
#endif
                LoginResponse login;
                
                Unpack("HLl", &login.port, &login.challenge, &login.editingEnabled);
                worldMode->editingEnabled = login.editingEnabled;
                
                platformAPI.net.CloseConnection(clientNetwork->network, 0);
                platformAPI.net.OpenConnection(clientNetwork->network, server, login.port);
                ResetReceiver(&clientNetwork->receiver);
                clientNetwork->nextSendStandardApplicationData = {};
                clientNetwork->nextSendOrderedApplicationData = {};
                clientNetwork->serverChallenge = login.challenge;
                
                Assets* assets = gameState->assets;
                MemoryPool tempPool = {};
                
                for(u32 fileIndex = 0; fileIndex < assets->fileCount; ++fileIndex)
                {
                    AssetFile* file = GetAssetFile(assets, fileIndex);
                    if(file->valid)
                    {
                        PAKFileHeader* fileHeader = GetFileInfo(assets, fileIndex);
                        PlatformFileHandle* fileHandle = GetHandleFor(assets, fileIndex);
                        
                        u16 fileType = GetMetaAssetType(fileHeader->type);
                        u64 fileSubtypeHash = StringHash(fileHeader->subtype);
                        
                        TempMemory fileMemory = BeginTemporaryMemory(&tempPool);
                        
                        u8* content = (u8*) PushSize(&tempPool, file->size);
                        platformAPI.ReadFromFile(fileHandle, 0, file->size, content);
                        
                        u64 dataHash = DataHash((char*) content, file->size);
                        EndTemporaryMemory(fileMemory);
                        
                        SendFileHash(fileType, fileSubtypeHash, dataHash);
                    }
                }
                
                SendFileHash(0, 0, 0);
            } break;
            
            case Type_loginFileTransferBegin:
            {
                u32 fileToReceive;
                Unpack("L", &fileToReceive);
                
                worldMode->loginFileToReceiveCount = fileToReceive;
                worldMode->loginReceivedFileCount = 0;
                
                if(!fileToReceive)
                {
                    GameAccessRequest(clientNetwork->serverChallenge);
                }
            } break;
            
            case Type_gameAccess:
            {
                b32 deleteEntities = false;
                Unpack("LHLl", &worldMode->worldSeed, 
                       &player->serverID.archetype, &player->serverID.archetypeIndex, &deleteEntities);
                
                if(deleteEntities)
                {
                    for(u32 mappingIndex = 0; mappingIndex < ArrayCount(worldMode->mappings); ++mappingIndex)
                    {
                        FREELIST_FREE(worldMode->mappings[mappingIndex], ServerClientIDMapping, worldMode->firstFreeMapping);
                    }
                    EXECUTE_JOB(worldMode, DeleteEntities, true, 0);
                }
            } break;
            
            case Type_worldInfo:
            {
                Unpack("Cd", &worldMode->season, &worldMode->seasonLerp);
            } break;
            
            case Type_entityHeader:
            {
                AssetID definitionID;
                EntityID serverID;
                u32 seed;
                
                Unpack("HLLHLL", &definitionID.type, &definitionID.subtypeHashIndex, &definitionID.index, &serverID.archetype, &serverID.archetypeIndex, &seed);
                Assert(serverID.archetype < Archetype_Count);
                
                EntityID clientID = GetClientIDMapping(worldMode, serverID);
                if(!IsValid(clientID))
                {
                    Assets* assets = gameState->assets;
                    EntityDefinition* definition = GetData(assets, EntityDefinition, definitionID);
                    
                    if(definition)
                    {
                        clientID = {};
                        AcquireArchetype(worldMode, serverID.archetype, (&clientID));
                        
                        ClientEntityInitParams params = definition->client;
                        params.ID = serverID;
                        params.seed = seed;
                        
                        InitFunc[serverID.archetype](worldMode, clientID, &definition->common, &params);
                        AddClientIDMapping(worldMode, serverID, clientID);
                    }
                }
                currentID = clientID;
                if(AreEqual(serverID, player->serverID))
                {
                    player->clientID = clientID;
                }
            } break;
            
            case Type_entityBasics:
            {
                UniversePos P;
                Vec3 speed;
                GameProperty action;
                Unpack("lllVVHH", &P.chunkX, &P.chunkY, &P.chunkZ, &P.chunkOffset, &speed, &action.property, &action.value);
                
                BaseComponent* base = GetComponent(worldMode, currentID, BaseComponent);
                if(base)
                {
                    r32 maxDistancePrediction = 2.5f;
                    //Vec3 deltaP = Subtract(P, base->universeP);
                    //r32 deltaLength = Length(deltaP);
                    base->universeP = P;
                    base->velocity = speed;
                    base->action = action;
                    
                    if(AreEqual(currentID, player->clientID))
                    {
                        player->universeP = P;
                    }
                }
            } break;
            
            case Type_FileHeader:
            {
                ReceivingAssetFile* newFile = BootstrapPushStruct(ReceivingAssetFile, memory);
                Unpack("LHsLL", &newFile->index, &newFile->type, newFile->subtype, &newFile->uncompressedSize, &newFile->compressedSize);
                newFile->receivedSize = 0;
                newFile->content = PushSize(&newFile->memory, newFile->compressedSize);
                
                
                newFile->chunkCount = (newFile->compressedSize / CHUNK_SIZE) + 1;
                newFile->receivedChunks = PushArray(&newFile->memory, b32, newFile->chunkCount);
                
                if(!player->receiveFileSentinel.next)
                {
                    DLLIST_INIT(&player->receiveFileSentinel);
                }
                
                DLLIST_INSERT(&player->receiveFileSentinel, newFile);
                
				SendFileHeaderAck(newFile->index);
            } break;
            
            case Type_FileChunk:
            {
                u32 offset;
                u32 fileIndex;
                u16 sizeToCopy;
                Unpack("LLH", &offset, &fileIndex, &sizeToCopy);
                
                ReceivingAssetFile* receiving = 0;
                for(ReceivingAssetFile* test = player->receiveFileSentinel.next; test != &player->receiveFileSentinel; test = test->next)
                {
                    if(test->index == fileIndex)
                    {
                        receiving = test;
                        break;
                    }
                }
                
                if(receiving)
                {
                    Assert((offset % CHUNK_SIZE) == 0);
                    u32 chunkIndex = offset / CHUNK_SIZE;
                    Assert(chunkIndex < receiving->chunkCount);
                    
                    if(!receiving->receivedChunks[chunkIndex])
                    {
                        receiving->receivedChunks[chunkIndex] = true;
                        u8* dest = receiving->content + offset;
                        Copy(sizeToCopy, dest, packetPtr);
                        receiving->receivedSize += sizeToCopy;
                        if(receiving->receivedSize >= receiving->compressedSize)
                        {
                            Assert(receiving->receivedSize == receiving->compressedSize);
                            Assets* assets = gameState->assets;
                            u32 destFileIndex = 0;
                            AssetFile* destFile = CloseAssetFileFor(assets, receiving->type, StringHash(receiving->subtype), &destFileIndex);
                            if(!destFile)
                            {
                                Assert(assets->fileCount < assets->maxFileCount);
                                destFileIndex = assets->fileCount++;
                                destFile = GetAssetFile(assets, destFileIndex);
                            }
                            //platformAPI.deletefile(preexisting);
                            u8* compressed = receiving->content;
                            u32 compressedSize = receiving->compressedSize;
                            
                            u32 uncompressedSize = receiving->uncompressedSize;
                            u8* uncompressed = PushSize(&receiving->memory, uncompressedSize);
                            u32 cmp_status = uncompress(uncompressed, (mz_ulong*) &uncompressedSize, compressed, compressedSize);
                            
                            Assert(cmp_status == Z_OK);
                            Assert(uncompressedSize == receiving->uncompressedSize);
                            
                            ReopenReloadAssetFile(assets, destFile, destFileIndex, receiving->type, receiving->subtype, uncompressed, uncompressedSize, &receiving->memory);
                            
                            DLLIST_REMOVE(receiving);
                            Clear(&receiving->memory);
                            
                            if(worldMode->loginFileToReceiveCount)
                            {
                                if(++worldMode->loginReceivedFileCount == worldMode->loginFileToReceiveCount)
                                {
                                    GameAccessRequest(clientNetwork->serverChallenge);
                                }
                            }
                        }
                    }
                }
                packetPtr += sizeToCopy;
            } break;
#if 0                
            case Type_plantUpdate:
            {
                InvalidCodePath;
                
                r32 age;
                r32 life;
                
                r32 leafDensity;
                r32 flowerDensity;
                r32 fruitDensity;
                
                
                
                Unpack("ddddd", &age, &life, &leafDensity, &flowerDensity, &fruitDensity);
                Plant* plant = currentEntity->plant;
                if(plant)
                {
                    plant->serverAge = age;
                    plant->life = life;
                    plant->leafDensity = leafDensity;
                    plant->flowerDensity = flowerDensity;
                    plant->fruitDensity = fruitDensity;
                }
                
            } break;
#endif
            
            
#if 0            
            case Type_equipmentSlot:
            {
                u8 slotIndex;
                u64 identifier;
                Unpack("CQ", &slotIndex, &identifier);
                
                currentEntity->equipment[slotIndex].ID = identifier;
            } break;
            
            
            case Type_containerHeader:
            {
                u64 identifier;
                Unpack("Q", &identifier);
                currentContainer = GetEntityClient(worldMode, identifier, true);
                currentContainer->identifier = identifier;
            } break;
            
            case Type_containerInfo:
            {
                u8 maxObjectCount;
                Unpack("C", &maxObjectCount);
                currentContainer->objects.maxObjectCount = maxObjectCount;
                currentContainer->objects.objectCount = 0;
            } break;
            
            case Type_objectRemoved:
            {
                u8 objectIndex;
                Unpack("C", &objectIndex);
                
                currentContainer->objects.objects[objectIndex].taxonomy = 0;
                Assert(currentContainer->objects.objectCount > 0);
                --currentContainer->objects.objectCount;
            } break;
            
            case Type_objectAdded:
            {
                u8 objectIndex;
                
                Unpack("C", &objectIndex);
                Object* dest = currentContainer->objects.objects + objectIndex;
                Unpack("LQHh", &dest->taxonomy, &dest->gen, &dest->quantity, &dest->status);
                ++currentContainer->objects.objectCount;
            } break;
            
            case Type_deletedEntity:
            {
                u64 deletedID;
                Unpack("Q", &deletedID);
                ClientEntity* entityC = GetEntityClient(worldMode, deletedID);
                if(entityC)
                {
                    entityC->beingDeleted = true;
                    entityC->animation.goOutTime = 0.0f;
                }
                
                if(deletedID == worldMode->player.targetIdentifier)
                {
                    for(u32 actionIndex = 0; actionIndex < Action_Count; ++actionIndex)
                    {
                        worldMode->player.targetPossibleActions[actionIndex] = PossibleAction_CantBeDone;
                    }
                }
            } break;
            
            case Type_essenceDelta:
            {
                u32 essenceTaxonomy;
                i16 delta;
                
                Unpack("Lh", &essenceTaxonomy, &delta);
                
                b32 found = false;
                for(u32 essenceIndex = 0; essenceIndex < MAX_DIFFERENT_ESSENCES; ++essenceIndex)
                {
                    EssenceSlot* essence = worldMode->player.essences + essenceIndex;
                    if(essence->taxonomy == essenceTaxonomy)
                    {
                        u32 diff = delta < 0 ? (u32) -delta : (u32) delta;
                        if(delta < 0)
                        {
                            Assert(essence->quantity >= diff);
                            essence->quantity -= diff;
                        }
                        else
                        {
                            essence->quantity += diff;
                        }
                        
                        found = true;
                        break;
                    }
                }
                
                
                if(!found)
                {
                    Assert(worldMode->player.essenceCount < MAX_DIFFERENT_ESSENCES);
                    Assert(delta > 0);
                    
                    EssenceSlot* newEssence = worldMode->player.essences + worldMode->player.essenceCount++;
                    newEssence->taxonomy = essenceTaxonomy;
                    newEssence->quantity = delta;
                }
                
            } break;
            
            case Type_effectTriggered:
            {
                u64 actorID;
                u64 targetID;
                u32 effectTaxonomy;
                
                Unpack("QQL", &actorID, &targetID, &effectTaxonomy);
                
                ClientEntity* actor = GetEntityClient(worldMode, actorID);
                if(actor)
                {
                    b32 found = false;
                    u32 currentTaxonomy = actor->taxonomy;
                    while(currentTaxonomy && !found)
                    {
                        TaxonomySlot* slot = GetSlotForTaxonomy(worldMode->table, currentTaxonomy);
                        AnimationEffects* effects = &slot->animationEffects;
                        for(AnimationEffect* effect = effects->firstAnimationEffect; effect; effect = effect->next)
                        {
                            if(effect->triggerEffectTaxonomy == effectTaxonomy)
                            {
                                ClientAnimationEffect* newEffect;
                                FREELIST_ALLOC(newEffect, worldMode->firstFreeEffect, PushStruct(worldMode->persistentPool, ClientAnimationEffect, NoClear()));
                                
                                newEffect->effect = *effect;
                                newEffect->effect.targetID = targetID;
                                FREELIST_INSERT(newEffect, actor->firstActiveEffect);
                                found = true;
                                break;
                            }
                        }
                        currentTaxonomy = GetParentTaxonomy(worldMode->table, currentTaxonomy);
                    }
                }
                
            } break;
            
            case Type_possibleActions:
            {
                EntityPossibleActions u;
                Unpack("LQl", &u.actionCount, &u.identifier, &u.overlapping);
                
                PossibleActionType* possibleActions;
                b32 idMatch = true;
                if(u.overlapping)
                {
                    worldMode->player.overlappingIdentifier = u.identifier;
                    possibleActions = worldMode->player.overlappingPossibleActions;
                }
                else
                {
                    idMatch = (worldMode->player.targetIdentifier == u.identifier);
                    possibleActions = worldMode->player.targetPossibleActions;
                }
                
                if(idMatch)
                {
                    for(u32 actionIndex = 0; actionIndex < Action_Count; ++actionIndex)
                    {
                        possibleActions[actionIndex] = PossibleAction_CantBeDone;
                    }
                }
                
                
                for(u32 counter = 0; counter < u.actionCount; ++counter)
                {
                    u32 actionIndex;
                    u8 possible;
                    Unpack("LC", &actionIndex, &possible);
                    if(idMatch)
                    {
                        possibleActions[actionIndex] = (PossibleActionType) possible;
                    }
                }
            } break;
            
            case Type_AvailableRecipes:
            {
                
                //BookMode* mode = UI->bookModes + UIBook_Recipes;
                
                u32 categoryCount;
                Unpack("L", &categoryCount);
                for(u32 categoryIndex = 0; categoryIndex < categoryCount; ++categoryIndex)
                {
                    b32 ignored;
                    u32 taxonomy;
                    Unpack("lL", &ignored, &taxonomy);
                    
                    if(categoryIndex == 0)
                    {
                        //mode->rootTaxonomy = taxonomy;
                        //mode->filterTaxonomy = taxonomy;
                    }
                    else
                    {
                        //AddToRecipeCategoryBlock(UI, taxonomy);
                    }
                }
            } break;
            
            case Type_NewRecipe:
            {
                Recipe recipe;
                Unpack("LQ", &recipe.taxonomy, &recipe.gen);
                
                //AddToRecipeBlock(UI, recipe);
            } break;
            
            case Type_SkillCategories:
            {
                //BookMode* mode = UI->bookModes + UIBook_Skills;
                
                u32 categoryCount;
                Unpack("L", &categoryCount);
                for(u32 categoryIndex = 0; categoryIndex < categoryCount; ++categoryIndex)
                {
                    SkillCategory skillCategory;
                    Unpack("lL", &skillCategory.unlocked, &skillCategory.taxonomy);
                    
                    if(categoryIndex == 0)
                    {
                        //mode->rootTaxonomy = skillCategory.taxonomy;
                        //mode->filterTaxonomy = skillCategory.taxonomy;
                    }
                    else
                    {
                        //AddToSkillCategoryBlock(UI, skillCategory);
                    }
                }
            } break;
            
            case Type_UnlockSkillCategoryRequest:
            {
                u32 taxonomy;
                Unpack("L", &taxonomy);
                
                
#if RESTRUCTURING                
                BookElementsBlock* block = UI->bookModes[UIBook_Skills].elements;
                
                b32 found = false;
                while(block && !found)
                {
                    for(u32 elementIndex = 0; elementIndex < block->elementCount; ++elementIndex)
                    {
                        BookElement* element = block->elements + elementIndex;
                        if(element->taxonomy == taxonomy)
                        {
                            element->unlocked = true;
                            found = true;
                            break;
                        }
                    }
                    
                    block = block->next;
                }
#endif
                
            } break;
            
            case Type_SkillLevel:
            {
                SkillSlot skill;
                b32 isPassive;
                b32 levelUp;
                
                Unpack("lLLl", &levelUp, &skill.taxonomy, &skill.level, &isPassive);
                
                
#if RESTRUCTURING                
                if(levelUp)
                {
                    BookElementsBlock* block = UI->bookModes[UIBook_Skills].elements;
                    
                    b32 found = false;
                    while(block && !found)
                    {
                        for(u32 elementIndex = 0; elementIndex < block->elementCount; ++elementIndex)
                        {
                            BookElement* element = block->elements + elementIndex;
                            if(element->taxonomy == skill.taxonomy)
                            {
                                element->skillLevel = skill.level;
                                found = true;
                                break;
                            }
                        }
                        block = block->next;
                    }
                    
                    //Assert(found);
                }
                else
                {
                    AddToSkillBlock(UI, skill);
                }
#endif
                
            } break;
            
            case Type_StartedAction:
            {
                u64 target;
                u8 action;
                Unpack("CQ", &action, &target);
                
                if(action == Action_Cast)
                {
                    u32 skillTaxonomy;
                    Unpack("L", &skillTaxonomy);
                    AddSkillAnimationEffects(worldMode, currentEntity, skillTaxonomy, target, AnimationEffect_ActionStart);
                }
                
                SignalAnimationSyncCompleted(&currentEntity->animation, action, AnimationSync_Preparing);
                
                ClientEntity* targetEntity = GetEntityClient(worldMode, target);
                if(targetEntity)
                {
                    Vec3 relative = targetEntity->P - currentEntity->P;
                    currentEntity->animation.flipOnYAxis = (relative.x < 0);
                    currentEntity->actionID = target;
                }
                
                AddAnimationEffects(worldMode, currentEntity, (EntityAction) action, target, AnimationEffect_ActionStart);
            } break;
            
            case Type_CompletedAction:
            {
                u64 target;
                u8 action;
                Unpack("CQ", &action, &target);
                
                if(action == Action_Cast)
                {
                    u32 skillTaxonomy;
                    Unpack("L", &skillTaxonomy);
                    AddSkillAnimationEffects(worldMode, currentEntity, skillTaxonomy, target, AnimationEffect_ActionCompleted);
                }
                
                SignalAnimationSyncCompleted(&currentEntity->animation, action, AnimationSync_WaitingForCompletion);
                currentEntity->actionID = 0;
                
                AddAnimationEffects(worldMode, currentEntity, (EntityAction) action, target, AnimationEffect_ActionCompleted);
            } break;
            
            case Type_StartedDragging:
            {
                u64 target;
                Unpack("Q", &target);
                
                ClientEntity* player = GetEntityClient(worldMode, worldMode->player.identifier);
                player->draggingID = target;
            } break;
            
            case Type_EndedDragging:
            {
                ClientEntity* player = GetEntityClient(worldMode, worldMode->player.identifier);
                player->draggingID = 0;
            } break;
            
            case Type_PatchLocalServer:
            {
                worldMode->patchingLocalServer = false;
            } break;
#endif
            
#if FORGIVENESS_INTERNAL
            case Type_InputRecording:
            {
                b32 started;
                Unpack("l", &started );
                if( started )
                {
                    //clientSideMovement = false;
                    //ResetParticleSystem();
                }
                else
                {
                    //clientSideMovement = true;
                }
            } break;
            
            
            case Type_debugEvent:
            {
                char GUID[256];
                char name[256];
                DebugEvent event = {};
                event.GUID = GUID;
                event.name = name;
                
                Unpack("QssLHCd", &event.clock, event.GUID, event.name, &event.threadID, &event.coreIndex, &event.type, &event.Value_r32);
                
                DebugState* debugState = debugGlobalMemory->debugState;
                if(debugState)
                {
                    DebugCollationState* collation = &debugState->serverState;
                    CollateDebugEvent(debugState, collation, &event);
                }
            } break;
#endif
            InvalidDefaultCase;
        }
        
        readSize += (u16) (packetPtr - oldPacketPtr);
        lastReceived = header.packetType;
    }
}

internal void ReceiveNetworkPackets(GameState* gameState, GameModeWorld* worldMode)
{
    NetworkPacketReceived packet;
    while(true)
    {
        packet = platformAPI.net.GetPacketOnSlot(clientNetwork->network, 0);
        if(!packet.dataSize)
        {
            break;
        }
        
        unsigned char* packetPtr = packet.data;
        
        
        ForgNetworkApplicationData applicationData;
        packetPtr = ForgUnpackApplicationData(packetPtr, &applicationData);
        
        ForgNetworkReceiver* receiver = &clientNetwork->receiver;
        if(applicationData.flags & ForgNetworkFlag_Ordered)
        {
            u32 delta = ApplicationDelta(applicationData, receiver->orderedBiggestReceived);
            if(delta > 0)
            {
                if(delta < WINDOW_SIZE)
                {
                    u32 placeHereIndex = (receiver->circularStartingIndex + (delta - 1)) % WINDOW_SIZE;
                    receiver->orderedWindow[placeHereIndex] = packet;
                    
                    u32 dispatched = 0;
                    
                    while(true)
                    {
                        u32 index = (receiver->circularStartingIndex + dispatched) % WINDOW_SIZE;
                        NetworkPacketReceived* test = receiver->orderedWindow + index;
                        if(test->dataSize)
                        {
                            ++receiver->orderedBiggestReceived.index;
                            
                            DispatchApplicationPacket(gameState, worldMode, test->data + sizeof(ForgNetworkApplicationData), test->dataSize - sizeof(ForgNetworkApplicationData));
                            test->dataSize = 0;
                            ++dispatched;
                        }
                        else
                        {
                            break;
                        }
                        
                        receiver->circularStartingIndex += dispatched;
                    }
                }
                else
                {
                    InvalidCodePath;
                }
            }
        }
        else if(applicationData.flags & ForgNetworkFlag_FileChunk)
        {
            DispatchApplicationPacket(gameState, worldMode, packetPtr, packet.dataSize - sizeof(ForgNetworkApplicationData));
        }
        else
        {
            if(ApplicationIndexGreater(applicationData, receiver->unorderedBiggestReceived))
            {
                receiver->unorderedBiggestReceived = applicationData;
                DispatchApplicationPacket(gameState, worldMode, packetPtr, packet.dataSize - sizeof(ForgNetworkApplicationData));
            }
        }
    }
}


#if 0
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
            entity->equipment[prediction->slot].ID = 0;
        } break;
        
        case Prediction_EquipmentAdded:
        {
            u64 ID = prediction->identifier;
            entity->equipment[prediction->slot].ID = ID;
        } break;
        
        case Prediction_ActionBegan:
        {
            EntityAction currentAction = entity->action;
            if((currentAction == prediction->action) || (currentAction <= Action_Idle))
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
#endif
