inline u8* ForgReserveSpace(PlayerComponent* player, GuaranteedDelivery deliveryType, u8 flags, u16 size, AssetID definitionID, EntityID ID, u32 entitySeed)
{
    ForgNetworkPacketQueue* queue = player->queues + deliveryType; 
    
    u8 applicationFlags = flags;
    u8* result = 0;
    Assert(size <= MTU);
    ForgNetworkPacket* packet = 0;
    
    
    if(queue->lastPacket)
    {
        packet = queue->lastPacket;
        if((packet->size + size) > MTU)
        {
            packet = 0;
        }
    }
    
    b32 writeEntityHeader = false;
    if(!packet)
    {
        packet = PushStruct(&queue->tempPool, ForgNetworkPacket);
        
        if(queue->lastPacket)
        {
            queue->lastPacket->next = packet;
            queue->lastPacket = packet;
        }
        else
        {
            Assert(!queue->firstPacket);
            queue->firstPacket = queue->lastPacket = packet;
        }
        
        ForgNetworkApplicationData data = queue->nextSendApplicationData;
        data.flags = applicationFlags;
        
        result = ForgPackApplicationData(packet->data, data);
        packet->size += sizeof(ForgNetworkApplicationData);
        queue->nextSendApplicationData.index++;
        
        if(IsValid(ID))
        {
            writeEntityHeader = true;
        }
    }
    
    if(packet)
    {
        result = packet->data + packet->size;
        if(writeEntityHeader)
        {
            unsigned char* oldResult = result;
            result = ForgPackHeader(result, Type_entityHeader);
            result += pack(result, "HHLHLL", definitionID.type, definitionID.subtype, definitionID.index, ID.archetype, ID.archetypeIndex);
            packet->size += (u16) (result - oldResult);
        }
        packet->size += size;
    }
    else
    {
        InvalidCodePath;
    }
    
    return result;
}

#define CloseAndStoreStandardPacket(player, ...) CloseAndStore(player, buff_, buff, GuaranteedDelivery_None, 0, __VA_ARGS__)
#define CloseAndStoreGuaranteedPacket(player, ...) CloseAndStore(player, buff_, buff, GuaranteedDelivery_Guaranteed,0, __VA_ARGS__)

#define CloseAndStoreFilePacket(player, ...) CloseAndStore(player, buff_, buff, GuaranteedDelivery_Guaranteed, ForgNetworkFlag_FileChunk, __VA_ARGS__)

#define CloseAndStoreOrderedPacket(player, ...) CloseAndStore(player, buff_, buff, GuaranteedDelivery_Ordered, ForgNetworkFlag_Ordered, __VA_ARGS__)

inline void CloseAndStore(PlayerComponent* player, unsigned char* buff_, unsigned char* buff, GuaranteedDelivery deliveryType, u8 flags, AssetID definitionID = {},EntityID ID = {}, u32 seed = 0)
{
    u16 totalSize = ForgEndPacket_(buff_, buff);
    u8* writeHere = ForgReserveSpace(player, deliveryType, flags, totalSize, definitionID, ID, seed);
    Assert(writeHere);
    if(writeHere)
    {
        Copy(totalSize, writeHere, buff_);
    }
}

inline void QueueAndFlushAllPackets(ServerState* server, PlayerComponent* player, ForgNetworkPacketQueue* queue, GuaranteedDelivery deliveryType)
{
    NetworkSendParams params = {};
    params.guaranteedDelivery = deliveryType;
    
    for(ForgNetworkPacket* toSend = queue->firstPacket; toSend; toSend = toSend->next)
    {
        platformAPI.net.QueuePacket(&server->clientInterface, player->connectionSlot, params, toSend->data, toSend->size);
    }
    
    queue->firstPacket = 0;
    queue->lastPacket = 0;
    Clear(&queue->tempPool);
}

inline void QueueAndFlushAllPackets(ServerState* server, PlayerComponent* player, r32 timeToAdvance)
{
    for(u32 deliveryType = GuaranteedDelivery_None; deliveryType < GuaranteedDelivery_Count; ++deliveryType)
    {
        ForgNetworkPacketQueue* queue = player->queues + deliveryType;
        QueueAndFlushAllPackets(server, player, queue, (GuaranteedDelivery) deliveryType);
    }
    platformAPI.net.FlushSendQueue(&server->clientInterface, player->connectionSlot, timeToAdvance);
}

#define StartPacket(player, type) unsigned char buff_[2048]; unsigned char* buff = ForgPackHeader( buff_, Type_##type);

#define Pack(formatString, ...) buff += pack(buff, formatString, ##__VA_ARGS__)

internal void SendLoginResponse(PlayerComponent* player, u16 port, u32 challenge, b32 editingEnabled)
{
    StartPacket(player, login);
    Pack("HLl", port, challenge, editingEnabled);
    CloseAndStoreOrderedPacket(player);
}

internal void SendGameAccessConfirm(PlayerComponent* player, u64 worldSeed, EntityID ID)
{
    StartPacket(player, gameAccess);
    Pack("QHL", worldSeed, ID.archetype, ID.archetypeIndex);
    CloseAndStoreOrderedPacket(player);
}

internal void SendWorldInfo(PlayerComponent* player, WorldSeason season, r32 seasonLerp)
{
    StartPacket(player, worldInfo);
    Pack("Cd", SafeTruncateToU8(season), seasonLerp);
    CloseAndStoreStandardPacket(player);
}

internal u16 PrepareEntityUpdate(ServerState* server, PhysicComponent* physic, unsigned char* buff_)
{
    UniversePos P = physic->P;
    unsigned char* buff = ForgPackHeader(buff_, Type_entityBasics);
    Pack("llV", P.chunkX, P.chunkY, P.chunkOffset);
    u16 totalSize = ForgEndPacket_( buff_, buff );
    return totalSize;
}


#if 0
inline b32 EntityCanDoAction(SimRegion* region, SimEntity* actor, SimEntity* target, EntityAction action, b32 distanceConstrain, b32* unableBecauseOfDistance);
internal void SendPossibleActions(SimRegion* region, SimEntity* actor, SimEntity* target, b32 overlapping)
{
    ServerState* server = region->server;
    ServerPlayerComponent* player = server->players + actor->playerID;
    
    StartPacket(player, possibleActions);
    unsigned char* actionCountDest = buff;
    buff += sizeof(u32);
    
    Pack("Ql", target->identifier, overlapping);
    
    
    u32 actionCount = 0;
    for(u32 actionIndex = 0; actionIndex < Action_Count; ++actionIndex)
    {
        b32 unableBecauseOfDistance = false;
        if(EntityCanDoAction(region, actor, target, (EntityAction)actionIndex, false, &unableBecauseOfDistance))
        {
            ++actionCount;
            
            PossibleActionType possible = unableBecauseOfDistance ? PossibleAction_TooFar : PossibleAction_CanBeDone;
            Pack("LC", actionIndex, SafeTruncateToU8(possible));
        }
    }
    
    pack(actionCountDest, "L", actionCount);
    
    CloseAndStoreStandardPacket(player);
}


internal void SendDeleteMessage(SimRegion* region, SimEntity* entity)
{
    ServerState* server = region->server;
    PartitionSurfaceEntityBlock* playerSurfaceBlock = QuerySpacePartitionPoint(region, &region->playerPartition, entity->P);
    while(playerSurfaceBlock)
    {
        for( u32 playerIndex = 0; playerIndex < playerSurfaceBlock->entityCount; ++playerIndex )
        {
            CollisionData* collider = playerSurfaceBlock->colliders + playerIndex;
            SimEntity* entityToSend = GetRegionEntity(region, collider->entityIndex);
            if(entityToSend->playerID)
            {
                ServerPlayerComponent* player = server->players + entityToSend->playerID;
                StartPacket(player, deletedEntity);
                
                Pack("Q", entity->identifier);
                CloseAndStoreOrderedPacket(player);
            }
        }
        playerSurfaceBlock = playerSurfaceBlock->next;
    }
}
#endif

inline void SendEntityHeader(PlayerComponent* player, AssetID definitionID, EntityID ID, u32 seed)
{
    StartPacket(player, entityHeader);
    Pack("HHLHLL", definitionID.type, definitionID.subtype, definitionID.index, ID.archetype, ID.archetypeIndex, seed);
    CloseAndStoreStandardPacket(player);
}

inline void SendEntityHeaderReliably(PlayerComponent* player, AssetID definitionID, EntityID ID, u32 seed)
{
    StartPacket(player, entityHeader);
    Pack("HHLHLL", definitionID.type, definitionID.subtype, definitionID.index, ID.archetype, ID.archetypeIndex, seed);
    CloseAndStoreOrderedPacket(player);
}

#if 0
inline void SendPlantUpdate(PlayerComponent* player, u64 entityID, PlantComponent* plant)
{
    StartPacket(player, plantUpdate);
    Pack("ddddd", plant->age, plant->life, plant->leafDensity,
         plant->flowerDensity, plant->fruitDensity);
    CloseAndStoreStandardPacket(player, entityID);
}
#endif

#if 0
inline void SendEquipmentID(PlayerComponent* player, u64 entityID, u8 slotIndex, u64 ID)
{
    StartPacket(player, equipmentSlot);
    Pack("CQ", slotIndex, ID);
    CloseAndStoreStandardPacket(player, entityID);
}

inline void SendStartedAction(PlayerComponent* player, u64 entityID, u8 actionIndex, u64 targetID)
{
    StartPacket(player, StartedAction);
    Pack("CQ", actionIndex, targetID);
    CloseAndStoreStandardPacket(player, entityID);
}

inline void SendCompletedAction(PlayerComponent* player, u64 entityID, u8 actionIndex, u64 targetID)
{
    StartPacket(player, CompletedAction);
    Pack("CQ", actionIndex, targetID);
    CloseAndStoreStandardPacket(player, entityID);
}
#endif

inline void SendFileHeader(PlayerComponent* player, u32 index, u16 type, u16 subtype, u32 uncompressedSize, u32 compressedSize)
{
    StartPacket(player, FileHeader);
    Pack("LHHLL", index, type, subtype, uncompressedSize, compressedSize);
    CloseAndStoreOrderedPacket(player);
}

inline void SendFileChunks(PlayerComponent* player, u32 index, char* source, u32 offset, u32 sizeToSend, u32 chunkSize)
{
    u32 sentSize = 0;
    u8* runningSource = (u8*) source + offset;
    
    while(sentSize < sizeToSend)
    {
        StartPacket(player, FileChunk);
        u16 toSend = SafeTruncateToU16(Min(chunkSize, sizeToSend - sentSize));
        Pack("LLH", offset, index, toSend);
        Copy(toSend, buff, runningSource);
        
        buff += toSend;
        runningSource += toSend;
        offset += toSend;
        sentSize += toSend;
        
        CloseAndStoreFilePacket(player);
    }
    
    Assert(sentSize == sizeToSend);
}

internal u32 SendAllPossibleData(ServerState* server, PlayerComponent* player, FileToSend** toSendPtr, u32 toSendSize)
{
    while(*toSendPtr && (toSendSize > 0))
    {
        FileToSend* toSend = *toSendPtr;
        if(toSend->acked)
        {
            GameFile* file = server->files + toSend->serverFileIndex;
            u32 remainingSizeInFile = SafeTruncateUInt64ToU32(file->compressedSize) - toSend->sendingOffset;
            
            u32 sending = Min(toSendSize, remainingSizeInFile);
            if(sending > 0)
            {
                SendFileChunks(player, toSend->playerIndex, (char*) file->content, toSend->sendingOffset, sending, CHUNK_SIZE);
                toSend->sendingOffset += sending;
            }
            
            u32 roundedSent = sending;
            if(roundedSent % CHUNK_SIZE)
            {
                roundedSent += CHUNK_SIZE - (roundedSent % CHUNK_SIZE);
            }
            Assert(roundedSent % CHUNK_SIZE == 0);
            toSendSize -= roundedSent;
            
            if(toSend->sendingOffset >= file->compressedSize)
            {
                file->counter = file->counter - 1;
                *toSendPtr = toSend->next;
                FREELIST_DEALLOC(toSend, server->firstFreeToSendFile);
            }
            else
            {
                Assert(toSendSize == 0);
            }
        }
        else
        {
            toSendPtr = &toSend->next;
        }
    }
    
    return toSendSize;
}

#if FORGIVENESS_INTERNAL
internal void SendDebugEvent(PlayerComponent* player, DebugEvent* event)
{
    b32 result = false;
    
    StartPacket(debugEvent);
    Pack("QQssLHCQQ", event->clock, event->GUID, event->GUID, event->name, event->threadID, event->coreIndex, event->type, event->overNetwork[0], event->overNetwork[1] );
    
    CloseAndStoreStandardPacket(player, ReliableOrdered);
}

internal void SendMemStats(PlayerComponent* player )
{
    DebugPlatformMemoryStats stats = platformAPI.DEBUGMemoryStats();
    b32 result = false;
    
    StartPacket(player, memoryStats);
    Pack("LQQ", stats.blockCount, stats.totalUsed, stats.totalSize );
    
    CloseAndStoreOrderedPacket(player);
}
#endif

