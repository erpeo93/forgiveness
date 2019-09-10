inline u8* ForgReserveSpace(Player* player, b32 reliable, u16 size, u64 identifier)
{
    ForgNetworkPacketQueue* queue = reliable ? &player->reliablePacketQueue : &player->standardPacketQueue;
    
    u8 applicationFlags = reliable ? ForgNetworkFlag_Ordered : 0;
    
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
        data.flags = reliable ? ForgNetworkFlag_Ordered : 0;
        
        result = ForgPackApplicationData(packet->data, data);
        packet->size += sizeof(ForgNetworkApplicationData);
        queue->nextSendApplicationData.index++;
        
        if(identifier)
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
            result += pack(result, "Q", identifier);
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

#define CloseAndStoreStandardPacket(player, ...) CloseAndStore(player, buff_, buff, false, __VA_ARGS__)

#define CloseAndStoreReliablePacket(player, ...) CloseAndStore(player, buff_, buff, true, __VA_ARGS__)

inline void CloseAndStore(Player* player, unsigned char* buff_, unsigned char* buff, b32 reliableAndOrdered, u64 identifier = 0)
{
    u16 totalSize = ForgEndPacket_(buff_, buff);
    u8* writeHere = ForgReserveSpace(player, reliableAndOrdered, totalSize, identifier);
    Assert(writeHere);
    if(writeHere)
    {
        Copy(totalSize, writeHere, buff_);
    }
}

inline void QueueAndFlushAllPackets(ServerState* server, Player* player, ForgNetworkPacketQueue* queue, b32 reliable)
{
    NetworkSendParams params = {};
    if(reliable)
    {
        params.guaranteedDelivery = GuaranteedDelivery_Standard;
    }
    for(ForgNetworkPacket* toSend = queue->firstPacket; toSend; toSend = toSend->next)
    {
        platformAPI.net.QueuePacket(&server->clientInterface, player->connectionSlot, params, toSend->data, toSend->size);
    }
    
    queue->firstPacket = 0;
    queue->lastPacket = 0;
    Clear(&queue->tempPool);
}

inline b32 QueueAndFlushAllPackets(ServerState* server, Player* player, r32 timeToAdvance)
{
    QueueAndFlushAllPackets(server, player, &player->standardPacketQueue, false);
    QueueAndFlushAllPackets(server, player, &player->reliablePacketQueue, true);
    b32 result = platformAPI.net.FlushSendQueue(&server->clientInterface, player->connectionSlot, timeToAdvance);
    return result;
}

#define StartPacket(player, type) unsigned char buff_[2048]; unsigned char* buff = ForgPackHeader( buff_, Type_##type);

#define Pack(formatString, ...) buff += pack(buff, formatString, ##__VA_ARGS__)

internal void SendLoginResponse(Player* player, u16 port, u32 challenge, b32 editingEnabled)
{
    StartPacket(player, login);
    Pack("HLl", port, challenge, editingEnabled);
    CloseAndStoreReliablePacket(player);
}

internal void SendGameAccessConfirm(Player* player, u64 worldSeed, u32 identifier)
{
    StartPacket(player, gameAccess);
    Pack("QL", worldSeed, identifier);
    CloseAndStoreReliablePacket(player);
}

internal void SendWorldInfo(Player* player, WorldSeason season, r32 seasonLerp)
{
    StartPacket(player, worldInfo);
    Pack("Cd", SafeTruncateToU8(season), seasonLerp);
    CloseAndStoreStandardPacket(player);
}

internal u16 PrepareEntityUpdate(ServerState* server, Entity* entity, unsigned char* buff_)
{
    UniversePos P = entity->P;
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
    ServerPlayer* player = server->players + actor->playerID;
    
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
                ServerPlayer* player = server->players + entityToSend->playerID;
                StartPacket(player, deletedEntity);
                
                Pack("Q", entity->identifier);
                CloseAndStoreReliablePacket(player);
            }
        }
        playerSurfaceBlock = playerSurfaceBlock->next;
    }
}
#endif

inline void SendEntityHeader(Player* player, u32 ID)
{
    StartPacket(player, entityHeader);
    Pack("L", ID);
    CloseAndStoreStandardPacket(player);
}

inline void SendEntityHeaderReliably(Player* player, u32 ID)
{
    StartPacket(player, entityHeader);
    Pack("L", ID);
    CloseAndStoreReliablePacket(player);
}

#if 0
inline void SendPlantUpdate(Player* player, u64 entityID, PlantComponent* plant)
{
    StartPacket(player, plantUpdate);
    Pack("ddddd", plant->age, plant->life, plant->leafDensity,
         plant->flowerDensity, plant->fruitDensity);
    CloseAndStoreStandardPacket(player, entityID);
}
#endif

inline void SendEquipmentID(Player* player, u64 entityID, u8 slotIndex, u64 ID)
{
    StartPacket(player, equipmentSlot);
    Pack("CQ", slotIndex, ID);
    CloseAndStoreStandardPacket(player, entityID);
}

inline void SendStartedAction(Player* player, u64 entityID, u8 actionIndex, u64 targetID)
{
    StartPacket(player, StartedAction);
    Pack("CQ", actionIndex, targetID);
    CloseAndStoreStandardPacket(player, entityID);
}

inline void SendCompletedAction(Player* player, u64 entityID, u8 actionIndex, u64 targetID)
{
    StartPacket(player, CompletedAction);
    Pack("CQ", actionIndex, targetID);
    CloseAndStoreStandardPacket(player, entityID);
}


#if 0
inline void SendEffectTriggered(Player* player, EffectTriggeredToSend* toSend)
{
    StartPacket(player, effectTriggered);
    Pack("QQL", toSend->actor, toSend->target, toSend->ID);
    CloseAndStoreReliablePacket(player);
}
#endif

inline void SendFileHeader(Player* player, u16 type, u16 subtype, u32 uncompressedSize, u32 compressedSize, u32 chunkSize)
{
    StartPacket(player, FileHeader);
    Pack("HHLLL", type, subtype, uncompressedSize, compressedSize, chunkSize);
    CloseAndStoreReliablePacket(player);
}

inline void SendFileChunks(Player* player, char* source, u32 sizeToSend, u32 chunkSize)
{
    u32 sentSize = 0;
    u8* runningSource = (u8*) source;
    
    while(sentSize < sizeToSend)
    {
        StartPacket(player, FileChunk);
        u32 toSent = Min(chunkSize, sizeToSend - sentSize);
        Copy(toSent, buff, runningSource);
        buff += toSent;
        runningSource += toSent;
        
        CloseAndStoreReliablePacket(player);
        sentSize += toSent;
    }
}

#define CHUNK_SIZE KiloBytes(1)
internal u32 SendFileChunksToPlayer(ServerState* server, Player* player, u32 sizeToSend, FileToSend* toSend, FileToSend** writeNext)
{
    u32 result = sizeToSend;
    
    Assert(toSend->index < server->fileCount);
    GameFile* file = server->files + toSend->index;
    if(player->sendingFileOffset == 0)
    {
        SendFileHeader(player, file->type, file->subtype, file->uncompressedSize, file->compressedSize, CHUNK_SIZE);
    }
    
    u32 remainingSizeInFile = SafeTruncateUInt64ToU32(file->compressedSize) - player->sendingFileOffset;
    u32 sending = Min(sizeToSend, remainingSizeInFile);
    
    if(sending > 0)
    {
        u8* buffer = file->content + player->sendingFileOffset;
        SendFileChunks(player, (char*) buffer, sending, CHUNK_SIZE);
        player->sendingFileOffset += sending;
    }
    
    u32 roundedSent = sending;
    if(roundedSent % CHUNK_SIZE)
    {
        roundedSent += CHUNK_SIZE - (roundedSent % CHUNK_SIZE);
    }
    Assert(roundedSent % CHUNK_SIZE == 0);
    result -= roundedSent;
    
    if(player->sendingFileOffset >= file->compressedSize)
    {
		--file->counter;
        player->sendingFileOffset = 0;
        *writeNext = toSend->next;
        FREELIST_DEALLOC(toSend, server->firstFreeToSendFile);
    }
    
    return result;
}

#if FORGIVENESS_INTERNAL
internal void SendDebugEvent(Player* player, DebugEvent* event)
{
    b32 result = false;
    
    StartPacket(debugEvent);
    Pack("QQssLHCQQ", event->clock, event->GUID, event->GUID, event->name, event->threadID, event->coreIndex, event->type, event->overNetwork[0], event->overNetwork[1] );
    
    CloseAndStoreStandardPacket(player, ReliableOrdered);
}

internal void SendMemStats( Player* player )
{
    DebugPlatformMemoryStats stats = platformAPI.DEBUGMemoryStats();
    b32 result = false;
    
    StartPacket(player, memoryStats);
    Pack("LQQ", stats.blockCount, stats.totalUsed, stats.totalSize );
    
    CloseAndStoreReliablePacket(player);
}
#endif

