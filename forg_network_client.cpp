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
internal void SendEditingEvent( DebugEvent* event )
{
    Assert( event->pointer );
    if( clientNetwork )
    {
        StartPacket(debugEvent);
        
        Pack("QQLHCQQ", event->clock, event->pointer, event->threadID, event->coreIndex, event->type, event->overNetwork[0], event->overNetwork[1]);
        
        CloseAndSendOrderedPacket();
    }
}

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


internal void SendCraftRequest(u32 taxonomy, GenerationData gen)
{
    StartPacket(CraftRequest);
    
    Pack("LQ", taxonomy, gen.generic);
    
    CloseAndSendOrderedPacket();
}

internal void SendCraftFromInventoryRequest(u64 containerID, u32 objectIndex)
{
    StartPacket(CraftFromInventoryRequest);
    
    Pack("QL", containerID, objectIndex);
    
    CloseAndSendOrderedPacket();
}

inline void SendCustomTargetPRequest(Vec3 P)
{
    StartPacket(CustomTargetPRequest);
    Pack("V", P);
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

internal void SendReleaseDraggingRequest()
{
    StartPacket(ReleaseDraggingRequest);
    CloseAndSendOrderedPacket();
}

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


inline void SendPatchServerRequest()
{
    StartPacket(PatchLocalServer);
    CloseAndSendOrderedPacket();
}

inline void SendPatchCheckRequest()
{
    StartPacket(PatchCheck);
    CloseAndSendOrderedPacket();
}


inline void SendMovePlayerRequest(Vec3 offset)
{
    StartPacket(MovePlayerInOtherRegion);
    Pack("V", offset);
    CloseAndSendOrderedPacket();
}

inline void SendPauseToggleMessage()
{
    StartPacket(PauseToggle);
    CloseAndSendOrderedPacket();
}

inline void SendFileHash(u16 type, u16 subtype, u64 hash)
{
    StartPacket(FileHash);
    Pack("HHQ", type, subtype, hash);
    CloseAndSendOrderedPacket();
}

internal void SendFileHeaderAck(u32 index)
{
    StartPacket(FileHeader);
    Pack("L", index);
    CloseAndSendGuaranteedPacket();
}

#if FORGIVENESS_INTERNAL
inline SavedNameSlot* GetNameSlot( DebugTable* debugTable, u64 pointer )
{
    u32 index = ( u32 ) ( pointer & ( u64 ) ( ArrayCount( debugTable->nameSlots - 1) ) );
    
    SavedNameSlot* result = 0;
    for( SavedNameSlot* test =  debugTable->nameSlots[index]; test; test = test->next )
    {
        if( test->pointer == pointer )
        {
            result = test;
            break;
        }
    }
    
    if( !result )
    {
        result = PushStruct( &debugTable->tempPool, SavedNameSlot );
        result->next = debugTable->nameSlots[index];
        debugTable->nameSlots[index] = result;
        result->pointer = pointer;
    }
    
    return result;
}
#endif

inline ClientEntity* GetEntityClient(GameModeWorld* worldMode, u64 ID, b32 canAllocate);
inline void SignalAnimationSyncCompleted(AnimationState* animation, u32 action, AnimationSyncState state);
inline void AddAnimationEffects(GameModeWorld* worldMode, ClientEntity* entity, EntityAction action, u64 targetID, u32 animationEffectFlags);
inline void AddSkillAnimationEffects(GameModeWorld* worldMode, ClientEntity* entity, u32 skillTaxonomy, u64 targetID, u32 animationEffectFlags);
internal void DispatchApplicationPacket(GameState* gameState, GameModeWorld* worldMode, unsigned char* packetPtr, u16 dataSize)
{
    ClientPlayer* player = &worldMode->player;
    
    ClientEntity* currentEntity = 0;
    ClientEntity* currentContainer = 0;
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
                
                Assets* assets = gameState->assets;
                
                MemoryPool tempPool = {};
                
                
                for(u16 type = 0; type < AssetType_Count; ++type)
                {
                    AssetArray* assetTypeArray = assets->assets + type;
                    for(u16 subtype = 0; subtype < assetTypeArray->subtypeCount; ++subtype)
                    {
                        u64 dataHash = 0;
                        for(u32 fileIndex = 0; fileIndex < assets->fileCount; ++fileIndex)
                        {
                            AssetFile* file = GetAssetFile(assets, fileIndex);
                            PAKFileHeader* fileHeader = GetFileInfo(assets, fileIndex);
                            PlatformFileHandle* fileHandle = GetHandleFor(assets, fileIndex);
                            
                            u16 fileType = GetMetaAssetType(fileHeader->type);
                            u16 fileSubtype = GetMetaAssetSubtype(type, fileHeader->subtype);
                            
                            if(fileType == type && fileSubtype == subtype)
                            {
                                TempMemory fileMemory = BeginTemporaryMemory(&tempPool);
                                
                                u8* content = (u8*) PushSize(&tempPool, file->size);
                                platformAPI.ReadFromFile(fileHandle, 0, file->size, content);
                                
                                dataHash = DataHash((char*) content, file->size);
                                EndTemporaryMemory(fileMemory);
                            }
                        }
                        SendFileHash(type, subtype, dataHash);
                    }
                }
                
                clientNetwork->serverChallenge = login.challenge;
                GameAccessRequest(clientNetwork->serverChallenge);
            } break;
            
            case Type_gameAccess:
            {
                Unpack("QL", &worldMode->worldSeed, &worldMode->player.identifier);
            } break;
            
            case Type_worldInfo:
            {
                Unpack("Cd", &worldMode->season, &worldMode->seasonLerp);
            } break;
            
            case Type_entityHeader:
            {
                u32 identifier;
                Unpack("L", &identifier);
                
                currentEntity = GetEntityClient(worldMode, identifier, true);
                currentEntity->identifier = identifier;
                currentEntity->timeFromLastUpdate = 0.0f;
            } break;
            
            case Type_entityBasics:
            {
                UniversePos P;
                ClientEntity* e = currentEntity;
                Assert(e);
                
                Unpack("llV", &P.chunkX, &P.chunkY, &P.chunkOffset);
                
                r32 maxDistancePrediction = 2.5f;
                Vec3 deltaP = Subtract(P, e->universeP);
                r32 deltaLength = Length(deltaP);
                if(true)
                {
                    e->universeP = P;
                    e->velocity = {};
                }
                else
                {
                    e->velocity = deltaP * clientNetwork->serverFPS;
                    if(e->identifier == worldMode->player.identifier)
                    {
                        worldMode->player.distanceCoeffFromServerP = deltaLength / maxDistancePrediction;
                    }
                }
                
                if(e->identifier == worldMode->player.identifier)
                {
                    worldMode->player.universeP = P;
                }
            } break;
            
            
            case Type_FileHeader:
            {
                ReceivingAssetFile* newFile = BootstrapPushStruct(ReceivingAssetFile, memory);
                Unpack("LHHLL", &newFile->index, &newFile->type, &newFile->subtype, &newFile->uncompressedSize, &newFile->compressedSize);
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
                            AssetFile* destFile = 0;
                            u32 destFileIndex = 0;
                            
                            for(u32 assetFileIndex = 0; assetFileIndex < assets->fileCount; ++assetFileIndex)
                            {
                                AssetFile* file = GetAssetFile(assets, assetFileIndex);
                                u16 type = GetMetaAssetType(file->header.type);
                                u16 subtype = GetMetaAssetSubtype(type, file->header.subtype);
                                
                                if(receiving->type == type && receiving->subtype == subtype)
                                {
                                    platformAPI.CloseFile(&file->handle);
                                    destFile = file;
                                    destFileIndex = assetFileIndex;
                                    
                                    break;
                                }
                            }
                            
                            
                            
                            if(!destFile)
                            {
                                Assert(assets->fileCount < assets->maxFileCount);
                                destFileIndex = assets->fileCount++;
                                destFile = GetAssetFile(assets, destFileIndex);
                            }
                            //platformAPI.deletefile(preexisting);
                            char* type = GetAssetTypeName(receiving->type);
                            char* subtype = GetAssetSubtypeName(receiving->type, receiving->subtype);
                            
                            u8* compressed = receiving->content;
                            u32 compressedSize = receiving->compressedSize;
                            
                            u32 uncompressedSize = receiving->uncompressedSize;
                            u8* uncompressed = PushSize(&receiving->memory, uncompressedSize);
                            u32 cmp_status = uncompress(uncompressed, (mz_ulong*) &uncompressedSize, compressed, compressedSize);
                            
                            Assert(cmp_status == Z_OK);
                            Assert(uncompressedSize == receiving->uncompressedSize);
                            
                            char newName[128];
                            FormatString(newName, sizeof(newName), "%s_%s", type, subtype);
                            platformAPI.ReplaceFile(PlatformFile_AssetPack, ASSETS_PATH, newName, uncompressed, uncompressedSize);
                            
                            char path[64];
                            PlatformFileGroup fake = {};
                            fake.path = path;
                            FormatString(fake.path, sizeof(path), "%s", ASSETS_PATH);
                            
                            char name[64];
                            PlatformFileInfo fakeInfo = {};
                            fakeInfo.name = name;
                            FormatString(fakeInfo.name, sizeof(name), "%s.upak", newName);
                            
                            destFile->handle = platformAPI.OpenFile(&fake, &fakeInfo);
                            ReloadAssetFile(assets, destFile, destFileIndex, &receiving->memory);
                            
                            DLLIST_REMOVE(receiving);
                            Clear(&receiving->memory);
                        }
                    }
                }
                packetPtr += sizeToCopy;
            } break;
            
            case Type_plantUpdate:
            {
                InvalidCodePath;
                
#if 0                
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
#endif
                
            } break;
            
            
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
                DebugEvent event = {};
                
                Unpack("QQ", &event.clock, &event.pointer );
                SavedNameSlot* slot = GetNameSlot( globalDebugTable, ( u64 ) event.pointer );
                event.GUID = slot->GUID;
                event.name = slot->name;
                Unpack("ssLHCQQ", event.GUID, event.name, &event.threadID, &event.coreIndex, &event.type, event.overNetwork, event.overNetwork + 1 );
                Assert( event.pointer );
                
                if( event.pointer == globalDebugTable->pointerToIgnore )
                {
                    // NOTE(Leonardo): we ignore the event cause the data it contains will be wrong... we have just finished editing that
                    //event, so the server can't have the updated information.
                    globalDebugTable->pointerToIgnore = 0;
                    event.overNetwork[0] = globalDebugTable->overNetworkEdit[0];
                    event.overNetwork[1] = globalDebugTable->overNetworkEdit[1];
                }
                
                u32 arrayIndex = globalDebugTable->currentServerEventArrayIndex;
                u32 eventIndex = globalDebugTable->serverEventCount[arrayIndex]++;
                globalDebugTable->serverEvents[arrayIndex][eventIndex] = event;
            } break;
            
            
            case Type_memoryStats:
            {
                DebugPlatformMemoryStats* serverStats = &globalDebugTable->serverStats;
                Unpack("LQQ", &serverStats->blockCount, &serverStats->totalUsed, &serverStats->totalSize );
                globalDebugTable->serverFinished = true;
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
