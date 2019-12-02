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

internal void QueueGameOverMessage(PlayerComponent* player)
{
    StartPacket(player, GameOver);
    QueueOrderedPacket(player);
}

internal void QueueGameWonMessage(PlayerComponent* player)
{
    StartPacket(player, GameWon);
    QueueOrderedPacket(player);
}

internal u16 PrepareBasicsUpdate(ServerState* server, DefaultComponent* def, PhysicComponent* physic, ActionComponent* act, unsigned char* buff_)
{
    u16 result = 0;
    if(def->basicPropertiesChanged != 0)
    {
        unsigned char* buff = ForgPackHeader(buff_, Type_entityBasics);
        Vec3 speed = physic ? physic->speed : V3(0, 0, 0);
        
        if(Abs(speed.x) < 0.01f)
        {
            speed.x = 0;
        }
        
        if(Abs(speed.y) < 0.01f)
        {
            speed.y = 0;
        }
        
        u16 action = act ? GetU16(act->action) : 0;
        u16 sendingFlags = def->basicPropertiesChanged;
        Pack("H", sendingFlags);
        def->basicPropertiesChanged = 0;
        
        if(sendingFlags & (u16) EntityBasics_Definition)
        {
            Pack("LLL", def->definitionID.subtypeHashIndex, def->definitionID.index, def->seed);
        }
        
        if(sendingFlags & (u16) EntityBasics_Position)
        {
            Pack("hhhV", def->P.chunkX, def->P.chunkY, def->P.chunkZ, def->P.chunkOffset);
        }
        
        if(sendingFlags & (u16) EntityBasics_Velocity)
        {
            Pack("V", speed);
            Assert(LengthSq(speed) < Square(1000.0f));
        }
        
        if(sendingFlags & (u16) EntityBasics_Action)
        {
            Pack("H", action);
        }
        
        if(sendingFlags & (u16) EntityBasics_Status)
        {
            Pack("H", def->status);
        }
        
        if(sendingFlags & (u16) EntityBasics_Flags)
        {
            Pack("L", def->flags);
        }
        
        result = ForgEndPacket_(buff_, buff);
    }
    
    return result;
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





internal u16 PrepareMappingsUpdate(ServerState* server, EquipmentComponent* equipment, UsingComponent* equipped, ContainerComponent* container, unsigned char* buff_)
{
    unsigned char* buff = ForgPackHeader(buff_, Type_Mappings);
    unsigned char* countGoesHere = buff;
    Pack("H", 0);
    
    u16 result = 0;
    u16 count = 0;
    if(equipment)
    {
        for(u16 slotIndex = 0; slotIndex < Count_equipmentSlot; ++slotIndex)
        {
            InventorySlot* slot = equipment->slots + slotIndex;
            if(IsDirty(slot))
            {
                ++count;
                Pack("CHHL", Mapping_Equipment, slotIndex, slot->type, GetBoundedID(slot).archetype_archetypeIndex);
                ResetDirty(slot);
            }
        }
    }
    
    if(equipped)
    {
        if(IsDirty(equipped->draggingID))
        {
            ++count;
            Pack("CHHL", Mapping_Dragging, 0, 0, GetBoundedID(equipped->draggingID).archetype_archetypeIndex);
            ResetDirty(&equipped->draggingID);
        }
        for(u16 slotIndex = 0; slotIndex < Count_usingSlot; ++slotIndex)
        {
            InventorySlot* slot = equipped->slots + slotIndex;
            
            if(IsDirty(slot))
            {
                ++count;
                Pack("CHHL", Mapping_Using, slotIndex, slot->type, GetBoundedID(slot).archetype_archetypeIndex);
                ResetDirty(slot);
            }
            
            if(IsValidID(GetBoundedID(slot)))
            {
                Assert(!HasComponent(GetBoundedID(slot), ContainerComponent));
            }
        }
    }
    
    if(container)
    {
        if(IsDirty(container->openedBy))
        {
            ++count;
            Pack("CHHL", Mapping_OpenedBy, 0, 0, GetBoundedID(container->openedBy).archetype_archetypeIndex);
            ResetDirty(&container->openedBy);
        }
        
        for(u16 objectIndex = 0; objectIndex < ArrayCount(container->storedObjects); ++objectIndex)
        {
            InventorySlot* slot = container->storedObjects + objectIndex;
            
            if(IsDirty(slot))
            {
                ++count;
                Pack("CHHL", Mapping_ContainerStored, objectIndex, slot->type, GetBoundedID(slot).archetype_archetypeIndex);
                ResetDirty(slot);
            }
        }
        
        for(u16 objectIndex = 0; objectIndex < ArrayCount(container->usingObjects); ++objectIndex)
        {
            InventorySlot* slot = container->usingObjects + objectIndex;
            if(IsDirty(slot))
            {
                ++count;
                Pack("CHHL", Mapping_ContainerUsing, objectIndex, slot->type, GetBoundedID(slot).archetype_archetypeIndex);
                ResetDirty(slot);
            }
        }
    }
    
    if(count > 0)
    {
        pack(countGoesHere, "H", count);
        result = ForgEndPacket_(buff_, buff);
    }
    
    return result;
}


internal void QueueSeason(PlayerComponent* player, u16 season)
{
    StartPacket(player, Season);
    Pack("H", season);
    QueueOrderedPacket(player);
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
                --file->counter;
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

internal void SendEntityUpdate(ServerState* server, EntityID ID, b32 staticUpdate, b32 completeUpdate)
{
    DefaultComponent* def = GetComponent(server, ID, DefaultComponent);
    if(!def->updateSent || staticUpdate)
    {
        def->updateSent = true;
        if(completeUpdate)
        {
            def->basicPropertiesChanged |= EntityBasics_Definition; 
            def->basicPropertiesChanged |= EntityBasics_Position;
            
            if(!staticUpdate)
            {
                def->basicPropertiesChanged |= EntityBasics_Action; 
                def->basicPropertiesChanged |= EntityBasics_Status; 
            }
        }
        PhysicComponent* physic = GetComponent(server, ID, PhysicComponent);
        ActionComponent* act = GetComponent(server, ID, ActionComponent);
        
        unsigned char basicsBuffer_[KiloBytes(2)];
        u16 basicsSize = PrepareBasicsUpdate(server, def, physic, act, basicsBuffer_);
        
        EquipmentComponent* equipment = GetComponent(server, ID, EquipmentComponent);
        UsingComponent* equipped = GetComponent(server, ID, UsingComponent);
        ContainerComponent* container = GetComponent(server, ID, ContainerComponent);
        
        unsigned char mappingsBuffer_[KiloBytes(2)];
        u16 mappingsSize = PrepareMappingsUpdate(server, equipment, equipped, container, mappingsBuffer_);
        
        if(basicsSize > 0 || mappingsSize > 0)
        {
            SpatialPartitionQuery playerQuery = QuerySpatialPartitionAtPoint(&server->playerPartition, def->P);
            for(EntityID playerID = GetCurrent(&playerQuery); IsValid(&playerQuery); playerID = Advance(&playerQuery))
            {
                PlayerComponent* player = GetComponent(server, playerID, PlayerComponent);
                //if(physic->P.chunkZ == playerPhysic->P.chunkZ)
                if(basicsSize > 0)
                {
                    QueueEntityHeader(player, ID);
                    u8* writeHere = ForgReserveSpace(player, GuaranteedDelivery_None, 0, basicsSize, ID);
                    if(writeHere)
                    {
                        Copy(basicsSize, writeHere, basicsBuffer_);
                    }
                    else
                    {
                        InvalidCodePath;
                    }
                }
                
                if(mappingsSize > 0)
                {
                    QueueEntityHeaderReliably(player, ID);
                    u8* writeHere = ForgReserveSpace(player, GuaranteedDelivery_Ordered, ForgNetworkFlag_Ordered, mappingsSize, ID);
                    if(writeHere)
                    {
                        Copy(mappingsSize, writeHere, mappingsBuffer_);
                    }
                    else
                    {
                        InvalidCodePath;
                    }
                }
            }
        }
    }
}

internal void SendStaticUpdate(ServerState* server, EntityID ID)
{
    SendEntityUpdate(server, ID, true, true);
}