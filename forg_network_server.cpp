internal void ResetQueue(ForgNetworkPacketQueue* queue)
{
    Clear(&queue->tempPool);
    queue->firstPacket = 0;
    queue->lastPacket = 0;
}

inline u8* ForgReserveSpace(PlayerComponent* player, GuaranteedDelivery deliveryType, u8 flags, u16 size, EntityID ID)
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
            result += pack(result, "L", ID.archetype_archetypeIndex);
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

#define QueueStandardPacket(player, ...) Queue(player, buff_, buff, GuaranteedDelivery_None, 0, __VA_ARGS__)
#define QueueGuaranteedPacket(player, ...) Queue(player, buff_, buff, GuaranteedDelivery_Guaranteed,0, __VA_ARGS__)

#define QueueFilePacket(player, ...) Queue(player, buff_, buff, GuaranteedDelivery_Guaranteed, ForgNetworkFlag_FileChunk, __VA_ARGS__)

#define QueueOrderedPacket(player, ...) Queue(player, buff_, buff, GuaranteedDelivery_Ordered, ForgNetworkFlag_Ordered, __VA_ARGS__)

inline void Queue(PlayerComponent* player, unsigned char* buff_, unsigned char* buff, GuaranteedDelivery deliveryType, u8 flags, EntityID ID = {})
{
    u16 totalSize = ForgEndPacket_(buff_, buff);
    u8* writeHere = ForgReserveSpace(player, deliveryType, flags, totalSize, ID);
    Assert(writeHere);
    if(writeHere)
    {
        Copy(totalSize, writeHere, buff_);
    }
}

inline void FlushAllPackets(ServerState* server, PlayerComponent* player, ForgNetworkPacketQueue* queue, GuaranteedDelivery deliveryType)
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

inline void FlushAllPackets(ServerState* server, PlayerComponent* player, r32 timeToAdvance)
{
    for(u32 deliveryType = GuaranteedDelivery_None; deliveryType < GuaranteedDelivery_Count; ++deliveryType)
    {
        ForgNetworkPacketQueue* queue = player->queues + deliveryType;
        FlushAllPackets(server, player, queue, (GuaranteedDelivery) deliveryType);
    }
    platformAPI.net.FlushSendQueue(&server->clientInterface, player->connectionSlot, timeToAdvance);
}

#define StartPacket(player, type) unsigned char buff_[2048]; unsigned char* buff = ForgPackHeader( buff_, Type_##type);

#define Pack(formatString, ...) buff += pack(buff, formatString, ##__VA_ARGS__)

internal void QueueLoginResponse(PlayerComponent* player, u16 port, u32 challenge, b32 editingEnabled)
{
    StartPacket(player, login);
    Pack("HLl", port, challenge, editingEnabled);
    QueueOrderedPacket(player);
}

internal void QueueLoginFileTransferBegin(PlayerComponent* player, u32 fileCount)
{
    StartPacket(player, loginFileTransferBegin);
    Pack("L", fileCount);
    QueueOrderedPacket(player);
}

internal void QueueGameAccessConfirm(PlayerComponent* player, u32 worldSeed, EntityID ID, b32 deleteEntities)
{
    StartPacket(player, gameAccess);
    Pack("LLl", worldSeed, ID.archetype_archetypeIndex, deleteEntities);
    QueueOrderedPacket(player);
}

internal u16 PrepareEntityUpdate(ServerState* server, PhysicComponent* physic, unsigned char* buff_)
{
    UniversePos P = physic->P;
    unsigned char* buff = ForgPackHeader(buff_, Type_entityBasics);
    Pack("LLLlllVVHHL", physic->definitionID.subtypeHashIndex, physic->definitionID.index, physic->seed, P.chunkX, P.chunkY, P.chunkZ, P.chunkOffset, physic->speed, physic->action.property, physic->action.value, physic->flags);
    u16 totalSize = ForgEndPacket_( buff_, buff );
    return totalSize;
}

inline void QueueEntityHeader(PlayerComponent* player, EntityID ID)
{
    StartPacket(player, entityHeader);
    Pack("L", ID.archetype_archetypeIndex);
    QueueStandardPacket(player);
}

inline void QueueEntityHeaderReliably(PlayerComponent* player, EntityID ID)
{
    StartPacket(player, entityHeader);
    Pack("L", ID.archetype_archetypeIndex);
    QueueOrderedPacket(player);
}

internal void QueueEquipmentID(PlayerComponent* player, EntityID ID, u16 slotIndex, EntityID equipmentID)
{
    StartPacket(player, EquipmentMapping);
    Pack("HL", slotIndex, equipmentID.archetype_archetypeIndex);
    QueueStandardPacket(player, ID);
}

internal void QueueUsingID(PlayerComponent* player, EntityID ID, u16 slotIndex, EntityID usingID)
{
    StartPacket(player, UsingMapping);
    Pack("HL", slotIndex, usingID.archetype_archetypeIndex);
    QueueStandardPacket(player, ID);
}

internal void QueueContainerID(PlayerComponent* player, EntityID ID, u16 slotIndex, EntityID objectID)
{
    StartPacket(player, ContainerMapping);
    Pack("HL", slotIndex, objectID.archetype_archetypeIndex);
    QueueStandardPacket(player, ID);
}

internal void QueueOpenedByID(PlayerComponent* player, EntityID ID, EntityID openedBy)
{
    StartPacket(player, ContainerOpenedBy);
    Pack("L", openedBy.archetype_archetypeIndex);
    QueueStandardPacket(player, ID);
}


internal void QueueEffectDispatch(PlayerComponent* player, EntityID ID)
{
    StartPacket(player, EffectDispatch);
    QueueStandardPacket(player, ID);
}

inline void QueueFileHeader(PlayerComponent* player, u32 index, u16 type, char* subtype, u32 uncompressedSize, u32 compressedSize)
{
    StartPacket(player, FileHeader);
    Pack("LHsLL", index, type, subtype, uncompressedSize, compressedSize);
    QueueOrderedPacket(player);
}

inline void QueueFileChunks(PlayerComponent* player, u32 index, char* source, u32 offset, u32 sizeToSend, u32 chunkSize)
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
        
        QueueFilePacket(player);
    }
    
    Assert(sentSize == sizeToSend);
}

internal u32 QueueAllPossibleFileData(ServerState* server, PlayerComponent* player, FileToSend** toSendPtr, u32 toSendSize)
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
                QueueFileChunks(player, toSend->playerIndex, (char*) file->content, toSend->sendingOffset, sending, CHUNK_SIZE);
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
                Assert(file->counter > 0);
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
internal void QueueDebugEvent(PlayerComponent* player, DebugEvent* event)
{
    
    b32 result = false;
    StartPacket(player, debugEvent);
    Pack("QssLHCd", event->clock, event->GUID, event->name, event->threadID, event->coreIndex, event->type, event->Value_r32);
    
    QueueOrderedPacket(player);
}
#endif

