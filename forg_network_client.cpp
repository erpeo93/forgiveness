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

internal void SendCommand(u16 index, GameCommand command)
{
    StartPacket(Command);
    Pack("HHLHL", index, command.action, command.targetID, command.skillIndex, command.usingID);
    CloseAndSendStandardPacket();
}

internal void SendInventoryCommand(GameCommand command)
{
    StartPacket(InventoryCommand);
    Pack("HLLHLH", command.action, command.targetID, command.containerID, command.targetObjectIndex, command.targetContainerID, command.optionIndex);
    CloseAndSendOrderedPacket();
}

internal void SendCommandParameters(CommandParameters parameters)
{
    StartPacket(CommandParameters);
    Pack("VV", parameters.acceleration, parameters.targetOffset);
    CloseAndSendStandardPacket();
}

internal void SendSelectRecipeEssence(u16 slot, u16 essence)
{
    StartPacket(selectRecipeEssence);
    Pack("HH", slot, essence);
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
    Pack("LHlllV", definitionID.subtypeHashIndex, definitionID.index, P.chunkX, P.chunkY, P.chunkZ, P.chunkOffset);
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

internal void SendRecipeEssenceSlot(u16 slot, u16 essence)
{
    StartPacket(selectRecipeEssence);
    Pack("HH", slot, essence);
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

internal void RemoveClientIDMapping(GameModeWorld* worldMode, EntityID serverID)
{
    EntityID result = {};
    u32 hashIndex = ServerClientIDMappingHashIndex(worldMode, serverID);
    for(ServerClientIDMapping** mappingPtr = &worldMode->mappings[hashIndex]; *mappingPtr;)
    {
        ServerClientIDMapping* mapping = *mappingPtr;
        if(AreEqual(mapping->serverID, serverID))
        {
            *mappingPtr = mapping->next;
            FREELIST_DEALLOC(mapping, worldMode->firstFreeMapping);
            break;
        }
        else
        {
            mappingPtr = &mapping->next;
        }
    }
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

internal void StoreInventorySlot(GameModeWorld* worldMode, InventorySlot* slots, u32 slotCount, u16 index, u32 flags_type, EntityID ID, u64 stringHash)
{
    Assert(index < slotCount);
    
    InventorySlot* slot = slots + index;
    slot->flags_type = flags_type;
    slot->ID = ID;
    slot->slotHash = stringHash;
    slot->pieceHash = 0;
    slot->zoomCoeff = 1.0;
    slot->zoomSpeed = 1.0f;
    slot->maxZoomCoeff = 1.0f;
    
    if(IsValidID(ID))
    {
        BaseComponent* objectBase = GetComponent(worldMode, ID, BaseComponent);
        EntityDefinition* definition = GetEntityTypeDefinition(worldMode->gameState->assets, objectBase->definitionID);
        
        slot->pieceHash = StringHash(definition->client.name.name);
        slot->zoomSpeed = definition->client.slotZoomSpeed;
        slot->maxZoomCoeff = definition->client.maxSlotZoom;
    }
}

internal void RemoveInventorySlot(InventorySlot* slots, u32 slotCount, EntityID ID)
{
    for(u32 index = 0; index < slotCount; ++index)
    {
        InventorySlot* slot = slots + index;
        if(AreEqual(slot->ID, ID))
        {
            slot->ID = {};
        }
    }
}

STANDARD_ECS_JOB_CLIENT(DeleteEntities)
{
    FreeArchetype(worldMode, ID);
}


#define UnpackFlags(flag, string, ...) if(receivedFlags & SafeTruncateToU16(flag)){Unpack(string, ##__VA_ARGS__);}

internal void MusicTrigger(GameModeWorld* worldMode, char* musicType, u32 priority);
internal void DispatchGameEffect(GameModeWorld* worldMode, EntityID ID);
#if FORGIVENESS_INTERNAL
internal void CollateDebugEvent(DebugState* debugState, DebugCollationState* collation, DebugEvent* event);
#endif
internal void InitEntity(GameModeWorld* worldMode, EntityID ID, 
                         CommonEntityInitParams* common, 
                         ServerEntityInitParams* s, 
                         ClientEntityInitParams* c);
internal void MarkForDeletion(GameModeWorld* worldMode, EntityID clientID);
internal void DispatchApplicationPacket(GameState* gameState, GameModeWorld* worldMode, unsigned char* packetPtr, u16 dataSize)
{
    ClientPlayer* player = &worldMode->player;
    EntityID currentServerID = {};
    EntityID currentClientID = {};
    
    u16 readSize = 0;
    u16 toReadSize = dataSize;
    
    while(readSize < toReadSize)
    {
        unsigned char* oldPacketPtr = packetPtr;
        ForgNetworkHeader header;
        packetPtr = ForgUnpackHeader(packetPtr, &header);
        
        switch(header.packetType)
        {
            case Type_login:
            {
				LoginResponse login;
                Unpack("HLl", &login.port, &login.challenge, &login.editingEnabled);
                worldMode->editingEnabled = login.editingEnabled;
#if 0
#if 0
                char* server = "forgiveness.hopto.org";
#else
                char* server = "127.0.0.1";
#endif
                platformAPI.net.CloseConnection(clientNetwork->network, 0);
                platformAPI.net.OpenConnection(clientNetwork->network, server, login.port);
                ResetReceiver(&clientNetwork->receiver);
                clientNetwork->nextSendStandardApplicationData = {};
                clientNetwork->nextSendOrderedApplicationData = {};
#endif
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
                        
                        AssetSubtypeArray* assetSubtypeArray = GetAssetSubtypeForFile(assets, &file->header);
                        
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
                b32 isGhost = false;
                Unpack("LLll", &worldMode->worldSeed, 
                       &player->serverID.archetype_archetypeIndex, &deleteEntities, &isGhost);
                player->clientID = {};
                
                if(deleteEntities)
                {
                    for(u32 mappingIndex = 0; mappingIndex < ArrayCount(worldMode->mappings); ++mappingIndex)
                    {
                        FREELIST_FREE(worldMode->mappings[mappingIndex], ServerClientIDMapping, worldMode->firstFreeMapping);
                    }
                    EXECUTE_JOB(worldMode, DeleteEntities, true, 0);
                }
                
                if(!isGhost)
                {
                    SetState(worldMode, PlayingGame_None);
                }
            } break;
            
            case Type_GameOver:
            {
                SetState(worldMode, PlayingGame_Over);
            } break;
            
            case Type_GameWon:
            {
                SetState(worldMode, PlayingGame_Won);
            } break;
            
            case Type_CompletedCommand:
            {
                GameCommand command = {};
                Unpack("HLH", &command.action, &command.targetID, &command.skillIndex);
                
                GameUIContext* UI = &worldMode->gameUI;
                if(AreEqual(command, UI->lockedCommand))
                {
                    UI->lockedInteractionType = LockedInteraction_Completed;
                    UI->keyboardInteractionDisabled = true;
                }
            } break;
            
            case Type_entityHeader:
            {
                Unpack("L", &currentServerID.archetype_archetypeIndex);
                Assert(GetArchetype(currentServerID) < Archetype_Count);
                currentClientID = GetClientIDMapping(worldMode, currentServerID);
            } break;
            
            
            case Type_entityBasics:
            {
				AssetID definitionID;
                definitionID.type = AssetType_EntityDefinition;
                u16 essences[Count_essence] = {};
                u32 seed = 0;
				UniversePos P = {};
				Vec3 speed = {};
                u16 action = 0;
                u16 status = 0;
                u32 flags = 0;
                EntityID spawnerID = {};
				
				u16 receivedFlags;
				Unpack("H", &receivedFlags);
                
				if(receivedFlags & (u16) EntityBasics_Definition)
				{
					Unpack("LLL", &definitionID.subtypeHashIndex, &definitionID.index, &seed);
                    
                    u16 essenceCount;
                    Unpack("H", &essenceCount);
                    
                    for(u16 essenceIndex = 0; essenceIndex < essenceCount; ++essenceIndex)
                    {
                        u16 essence;
                        u16 quantity;
                        
                        Unpack("HH", &essence, &quantity);
                        essences[essence] = quantity;
                    }
				}
                
				if(receivedFlags & (u16) EntityBasics_Position)
				{
					Unpack("hhhV", &P.chunkX, &P.chunkY, &P.chunkZ, &P.chunkOffset);
				}
                
				if(receivedFlags & (u16) EntityBasics_Velocity)
				{
					Unpack("V", &speed);
                    Assert(LengthSq(speed) < Square(1000.0f));
				}
                
				if(receivedFlags & (u16) EntityBasics_Action)
				{
					Unpack("H", &action);
				}
                
				if(receivedFlags & (u16) EntityBasics_Status)
				{
					Unpack("H", &status);
				}
                
				if(receivedFlags & (u16) EntityBasics_Flags)
				{
					Unpack("L", &flags);
				}
                
				if(receivedFlags & (u16) EntityBasics_Spawner)
				{
                    EntityID spawnerIDServer;
					Unpack("L", &spawnerIDServer);
                    spawnerID = GetClientIDMapping(worldMode, spawnerIDServer);
				}
                
                b32 justCreated = false;
				if(receivedFlags & EntityBasics_Definition)
				{
					if(!IsValidID(currentClientID) && !IsSet(flags, EntityFlag_deleted))
					{
                        justCreated = true;
						Assets* assets = gameState->assets;
						EntityDefinition* definition = GetData(assets, EntityDefinition, definitionID);
						if(definition)
						{
							currentClientID = {};
							AcquireArchetype(worldMode, GetArchetype(currentServerID), (&currentClientID));
                            ClientEntityInitParams params = definition->client;
							params.ID = currentServerID;
							params.seed = seed;
                            
							definition->common.definitionID = EntityReference(definitionID);
                            definition->common.essences = essences;
							InitEntity(worldMode, currentClientID, &definition->common, 0, &params);
							AddClientIDMapping(worldMode, currentServerID, currentClientID);
						}
					}
                    
					if(AreEqual(currentServerID, player->serverID))
					{
						player->clientID = currentClientID;
					}
				}
                
                
                BaseComponent* base = GetComponent(worldMode, currentClientID, BaseComponent);
                if(base)
                {
					if(receivedFlags & EntityBasics_Velocity)
					{
						base->velocity = speed;
					}
                    
					if(receivedFlags & EntityBasics_Position)
                    {
                        b32 coldSetPosition = false;
						if(justCreated || (base->flags & EntityFlag_notInWorld) || (base->flags & EntityFlag_ghost))
						{
                            coldSetPosition = true;
                        }
						else
						{
							Vec3 deltaClientServer = SubtractOnSameZChunk(P, base->universeP);
                            r32 distance = Length(deltaClientServer);
                            r32 minDistance = 0.3f;
                            r32 maxDistance = 2.0f;
                            if(distance > maxDistance)
                            {
                                coldSetPosition = true;
                            }
                            else
                            {
                                r32 lerp = Clamp01MapToRange(minDistance, distance, maxDistance);
                                Vec3 maxSpeed = Normalize(deltaClientServer);
                                base->velocity = Lerp(base->velocity, lerp, maxSpeed);
                            }
						}
                        
                        if(coldSetPosition)
                        {
                            if(IsValidID(spawnerID))
                            {
                                BaseComponent* spawner = GetComponent(worldMode, spawnerID, BaseComponent);
                                
                                Vec3 projectileOffset = {};
                                AnimationComponent* animation = GetComponent(worldMode, spawnerID, AnimationComponent);
                                if(animation)
                                {
                                    projectileOffset = animation->spawnProjectileOffset;
                                }
                                UniversePos spawnedP = Offset(spawner->universeP, projectileOffset);
                                base->universeP = spawnedP;
                            }
                            else
                            {
                                base->universeP = P;
                            }
                            
                            if(!(base->flags & EntityFlag_notInWorld))
                            {
                                base->totalLifeTime = 0;
                            }
                            
                            if(AreEqual(currentClientID, player->clientID))
                            {
                                player->universeP = P;
                                worldMode->resetDayTime = true;
                            }
                        }
                    }
                    
					if(receivedFlags & EntityBasics_Action)
					{
						base->properties[Network_Action] = GameProp(action, action);
					}
                    
					if(receivedFlags & EntityBasics_Status)
					{
						base->properties[Network_Status] = GameProp(status, status);
					}
                    
					if(receivedFlags & EntityBasics_Flags)
					{
						base->flags = flags;
						if(IsSet(base->flags, EntityFlag_deleted))
						{
							MarkForDeletion(worldMode, currentClientID);
						}
					}
                    
                    base->timeSinceLastUpdate = 0;
                    //base->deletedTime = 0;
                    
#if 0                    
                    if(???)
                    {
                        SoundEventTrigger(worldMode, ID, SoundTrigger_LoosingHealth);
                    }
#endif
                    
                }
            } break;
            
            case Type_Health:
            {
                u16 receivedFlags;
				Unpack("H", &receivedFlags);
                
                u32 physicalHealth = 0;
                u32 maxPhysicalHealth = 0;
                u32 mentalHealth = 0;
                u32 maxMentalHealth = 0;
                
                UnpackFlags(HealthFlag_Physical, "L", &physicalHealth);
                UnpackFlags(HealthFlag_MaxPhysical, "L", &maxPhysicalHealth);
                UnpackFlags(HealthFlag_Mental, "L", &mentalHealth);
                UnpackFlags(HealthFlag_MaxMental, "L", &maxMentalHealth);
                
                AliveComponent* alive = GetComponent(worldMode, currentClientID, AliveComponent);
                if(alive)
                {
                    if(receivedFlags & HealthFlag_Physical)
                    {
                        alive->physicalHealth = physicalHealth;
                    }
                    
                    if(receivedFlags & HealthFlag_MaxPhysical)
                    {
                        alive->maxPhysicalHealth = maxPhysicalHealth;
                    }
                    
                    if(receivedFlags & HealthFlag_Mental)
                    {
                        alive->mentalHealth = mentalHealth;
                    }
                    
                    if(receivedFlags & HealthFlag_MaxMental)
                    {
                        alive->maxMentalHealth = maxMentalHealth;
                    }
                }
                
            } break;
            
            case Type_Misc:
            {
                u16 receivedFlags;
				Unpack("H", &receivedFlags);
                
                r32 attackDistance = 0;
                r32 attackContinueCoeff = 0;
                r32 lightRadious = 0;
                
                UnpackFlags(MiscFlag_AttackDistance, "d", &attackDistance);
                UnpackFlags(MiscFlag_AttackContinueCoeff, "d", &attackContinueCoeff);
                UnpackFlags(MiscFlag_LightRadious, "d", &lightRadious);
                
                MiscComponent* misc = GetComponent(worldMode, currentClientID, MiscComponent);
                if(misc)
                {
                    if(receivedFlags & MiscFlag_AttackDistance)
                    {
                        misc->attackDistance = attackDistance;
                    }
                    
                    if(receivedFlags & MiscFlag_AttackContinueCoeff)
                    {
                        misc->attackContinueCoeff = attackContinueCoeff;
                    }
                    
                    if(receivedFlags & MiscFlag_LightRadious)
                    {
                        misc->lightRadious = lightRadious;
                    }
                }
                
            } break;
            
            case Type_Essence:
            {
                u16 essence;
                u16 quantity;
                
                Unpack("HH", &essence, &quantity);
                
                BaseComponent* base = GetComponent(worldMode, worldMode->player.clientID, BaseComponent);
                if(base)
                {
                    base->essences[essence] = quantity;
                }
                else
                {
                    InvalidCodePath;
                }
            } break;
            
            case Type_deletedEntity:
            {
                EntityID ID;
                Unpack("L", &ID);
                EntityID clientID = GetClientIDMapping(worldMode, ID);
                BaseComponent* base = GetComponent(worldMode, clientID, BaseComponent);
                if(base)
                {
                    MarkForDeletion(worldMode, clientID);
                }
                
                MusicTrigger(worldMode, "forest", 1);
            } break;
            
			case Type_Mappings:
			{
                u16 count;
                Unpack("H", &count);
				for(u16 mappingIndex = 0; mappingIndex < count; ++mappingIndex)
				{
					u8 mappingType;
					u16 index;
                    u32 flags_type;
					EntityID ID;
					Unpack("CHLL", &mappingType, &index, &flags_type, &ID);
					switch(mappingType)
					{
						case Mapping_Equipment:
						{
                            EquipmentComponent* equipment = GetComponent(worldMode, currentClientID, EquipmentComponent);
                            
                            if(equipment)
                            {
                                StoreInventorySlot(worldMode, equipment->slots, ArrayCount(equipment->slots), index, flags_type, ID, StringHash(MetaTable_equipmentSlot[index]));
                                
                            }
                            else
                            {
                                InvalidCodePath;
                            }
						} break;
                        
						case Mapping_Using:
						{               UsingComponent* equipped = GetComponent(worldMode, currentClientID, UsingComponent);
                            if(equipped)
                            {
                                StoreInventorySlot(worldMode, equipped->slots, ArrayCount(equipped->slots), index, flags_type, ID, StringHash(MetaTable_usingSlot[index]));
                            }
                            else
                            {
                                InvalidCodePath;
                            }
                            
						} break;
                        
						case Mapping_ContainerStored:
						{
                            ContainerComponent* container = GetComponent(worldMode, currentClientID, ContainerComponent);
                            
                            if(container)
                            {
                                StoreInventorySlot(worldMode, container->storedObjects, ArrayCount(container->storedObjects), index, flags_type, ID, 0);
                            }
                            else
                            {
                                InvalidCodePath;
                            }
						} break;
                        
						case Mapping_ContainerUsing:
						{
                            ContainerComponent* container = GetComponent(worldMode, currentClientID, ContainerComponent);
                            
                            if(container)
                            {
                                StoreInventorySlot(worldMode, container->usingObjects, ArrayCount(container->usingObjects), index, flags_type, ID, 0);
                            }
                            else
                            {
                                InvalidCodePath;
                            }
						} break;
                        
						case Mapping_Dragging:
						{
                            BaseComponent* base = GetComponent(worldMode, currentClientID, BaseComponent);
                            if(base)
                            {
                                base->draggingID = ID;
                            }
                            else
                            {
                                InvalidCodePath;
                            }
						} break;
                        
						case Mapping_OpenedBy:
						{
                            ContainerComponent* container = GetComponent(worldMode, currentClientID, ContainerComponent);
                            
                            if(container)
                            {
                                container->openedBy = ID;
                                GameUIContext* UI = &worldMode->gameUI;
                                b32 wasLooting = IsValidID(UI->lootingIDServer);
                                
                                if(AreEqual(currentServerID, UI->lootingIDServer))
                                {
                                    if(!IsValidID(ID))
                                    {
                                        UI->lootingIDServer = {};
                                    }
                                }
                                else if(AreEqual(ID, worldMode->player.serverID))
                                {
                                    UI->lootingIDServer = currentServerID;
                                }
                                
                                
                                if(!IsValidID(UI->lootingIDServer))
                                {
                                    UI->lootingMode = false;
                                }
                                
                                if(IsValidID(UI->lootingIDServer) && !wasLooting)
                                {
                                    UI->lootingMode = true;
                                    UI->inventoryMode = false;
                                }
                            }
                            else
                            {
                                InvalidCodePath;
                            }
						} break;
					}
				}
                
			} break;
            
            case Type_DayTime:
            {
                worldMode->previousDayTime = worldMode->dayTime;
                Unpack("H", &worldMode->dayTime);
                worldMode->dayTimeTime = 0;
                
                if(worldMode->resetDayTime)
                {
                    worldMode->previousDayTime = worldMode->dayTime;
                    worldMode->resetDayTime = false;
                }
            } break;
            
#if 0            
            case Type_Weather:
            {
                
            } break;
#endif
            
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