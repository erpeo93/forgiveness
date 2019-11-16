PLATFORM_WORK_CALLBACK(ReceiveNetworkPackets)
{
    ReceiveNetworkPacketWork* work = (ReceiveNetworkPacketWork*) param;
    
    while(true)
    {
        work->ReceiveData(work->network);
    }
}

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
        
        if(IsValidID(ID))
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
    Pack("LLLlllVVHHL", physic->definitionID.subtypeHashIndex, physic->definitionID.index, physic->seed, P.chunkX, P.chunkY, P.chunkZ, P.chunkOffset, physic->speed, physic->action, physic->status, physic->flags);
    u16 totalSize = ForgEndPacket_( buff_, buff );
    return totalSize;
}

internal void QueueCompletedCommand(PlayerComponent* player, GameCommand* command)
{
    StartPacket(player, CompletedCommand);
    Pack("HLH", command->action, command->targetID.archetype_archetypeIndex, command->skillIndex);
    QueueOrderedPacket(player);
}

internal void QueueCompletedCommand(ServerState* server, EntityID ID, GameCommand* command)
{
    PlayerComponent* player = GetComponent(server, ID, PlayerComponent);
    if(player)
    {
        QueueCompletedCommand(player, command);
        
    }
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

internal void QueueDeletedID(PlayerComponent* player, EntityID ID)
{
    StartPacket(player, deletedEntity);
    Pack("L", ID.archetype_archetypeIndex);
    QueueStandardPacket(player);
}


internal void QueueEquipmentID(PlayerComponent* player, EntityID ID, u16 slotIndex, u16 slotType, EntityID equipmentID)
{
    StartPacket(player, EquipmentMapping);
    Pack("HHL", slotIndex, slotType, equipmentID.archetype_archetypeIndex);
    QueueStandardPacket(player, ID);
}

internal void QueueUsingID(PlayerComponent* player, EntityID ID, u16 slotIndex, u16 slotType, EntityID usingID)
{
    StartPacket(player, UsingMapping);
    Pack("HHL", slotIndex, slotType, usingID.archetype_archetypeIndex);
    QueueStandardPacket(player, ID);
}

internal void QueueContainerStoredID(PlayerComponent* player, EntityID ID, u16 slotIndex, u16 slotType, EntityID objectID)
{
    StartPacket(player, ContainerStoredMapping);
    Pack("HHL", slotIndex, slotType, objectID.archetype_archetypeIndex);
    QueueStandardPacket(player, ID);
}

internal void QueueContainerUsingID(PlayerComponent* player, EntityID ID, u16 slotIndex, u16 slotType, EntityID objectID)
{
    StartPacket(player, ContainerUsingMapping);
    Pack("HHL", slotIndex, slotType, objectID.archetype_archetypeIndex);
    QueueStandardPacket(player, ID);
}

internal void QueueOpenedByID(PlayerComponent* player, EntityID ID, EntityID openedBy)
{
    StartPacket(player, ContainerOpenedBy);
    Pack("L", openedBy.archetype_archetypeIndex);
    QueueStandardPacket(player, ID);
}


internal void QueueDraggingID(PlayerComponent* player, EntityID ID, EntityID dragging)
{
    StartPacket(player, DraggingMapping);
    Pack("L", dragging.archetype_archetypeIndex);
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

STANDARD_ECS_JOB_SERVER(SendEntityUpdate)
{
    PhysicComponent* physic = GetComponent(server, ID, PhysicComponent);
    unsigned char buff_[KiloBytes(2)];
    u16 totalSize = PrepareEntityUpdate(server, physic, buff_);
    
    EquipmentComponent* equipment = GetComponent(server, ID, EquipmentComponent);
    UsingComponent* equipped = GetComponent(server, ID, UsingComponent);
    ContainerComponent* container = GetComponent(server, ID, ContainerComponent);
    
    r32 maxDistance = 3.0f * CHUNK_DIM;
    r32 maxDistanceSq = Square(maxDistance);
    
    SpatialPartitionQuery playerQuery = QuerySpatialPartitionAtPoint(&server->playerPartition, physic->P);
    for(EntityID playerID = GetCurrent(&playerQuery); IsValid(&playerQuery); playerID = Advance(&playerQuery))
    {
        PlayerComponent* player = GetComponent(server, playerID, PlayerComponent);
        PhysicComponent* playerPhysic = GetComponent(server, playerID, PhysicComponent);
        
        //if(physic->P.chunkZ == playerPhysic->P.chunkZ)
        {
            Vec3 distance = SubtractOnSameZChunk(physic->P, playerPhysic->P);
            if(LengthSq(distance) < maxDistanceSq)
            {
                QueueEntityHeader(player, ID);
                
                u8* writeHere = ForgReserveSpace(player, GuaranteedDelivery_None, 0, totalSize, ID);
                Assert(writeHere);
                if(writeHere)
                {
                    Copy(totalSize, writeHere, buff_);
                }
                
                if(equipment)
                {
                    for(u16 slotIndex = 0; slotIndex < Count_equipmentSlot; ++slotIndex)
                    {
                        InventorySlot* slot = equipment->slots + slotIndex;
                        QueueEquipmentID(player, ID, slotIndex, slot->type, slot->ID);
                    }
                }
                
                if(equipped)
                {
                    QueueDraggingID(player, ID, equipped->draggingID);
                    for(u16 slotIndex = 0; slotIndex < Count_usingSlot; ++slotIndex)
                    {
                        InventorySlot* slot = equipped->slots + slotIndex;
                        QueueUsingID(player, ID, slotIndex, slot->type, slot->ID);
                        if(IsValidID(slot->ID))
                        {
                            Assert(!HasComponent(slot->ID, ContainerComponent));
                        }
                    }
                }
                
                if(container)
                {
                    QueueOpenedByID(player, ID, container->openedBy);
                    for(u16 objectIndex = 0; objectIndex < ArrayCount(container->storedObjects); ++objectIndex)
                    {
                        InventorySlot* slot = container->storedObjects + objectIndex;
                        QueueContainerStoredID(player, ID, objectIndex, slot->type, slot->ID);
                    }
                    
                    for(u16 objectIndex = 0; objectIndex < ArrayCount(container->usingObjects); ++objectIndex)
                    {
                        InventorySlot* slot = container->usingObjects + objectIndex;
                        QueueContainerUsingID(player, ID, objectIndex, slot->type, slot->ID);
                    }
                }
            }
        }
    }
}
