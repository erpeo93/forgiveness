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

internal void QueueGameAccessConfirm(PlayerComponent* player, u32 worldSeed, EntityID ID, b32 deleteEntities, b32 isGhost)
{
    StartPacket(player, gameAccess);
    Pack("LLll", worldSeed, ID.archetype_archetypeIndex, deleteEntities, isGhost);
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

internal void QueueEssenceUpdate(PlayerComponent* player, u16 essence, u16 quantity)
{
    StartPacket(player, EssenceDelta);
    Pack("HH", essence, quantity);
    QueueOrderedPacket(player);
}

#define PackIfFlag(flags, flag, string, ...) if(flags & SafeTruncateToU16(flag)){Pack(string, ##__VA_ARGS__);}

#define PackR32(V, flags, flag) if(V.dirty || (flags & flag)){flags |= flag; V.dirty = false;Pack("d", V.value);}

#define PackU16(V, flags, flag) if(V.dirty || (flags & flag)){flags |= flag; V.dirty = false;Pack("H", V.value);}

#define PackU32(V, flags, flag) if(V.dirty || (flags & flag)){flags |= flag; V.dirty = false;Pack("L", V.value);}


internal u16 PrepareBasicsUpdate(ServerState* server, EntityID ID, b32 completeUpdate, b32 staticUpdate, unsigned char* buff_)
{
    u16 result = 0;
    
    DefaultComponent* def = GetComponent(server, ID, DefaultComponent);
    MovementComponent* movement = GetComponent(server, ID, MovementComponent);
    
    u16 flags = def->networkFlags;
    def->networkFlags = 0;
    if(completeUpdate)
    {
        flags |= BasicFlags_Definition; 
        flags |= BasicFlags_Position;
        
        if(IsValidID(def->spawnerID))
        {
            flags |= BasicFlags_Spawner;
        }
    }
    
    unsigned char* buff = ForgPackHeader(buff_, Type_entityBasics);
    unsigned char* flagDest = buff;
    
    
    Vec3 speed = movement ? movement->speed : V3(0, 0, 0);
    if(Abs(speed.x) < 0.01f)
    {
        speed.x = 0;
    }
    
    if(Abs(speed.y) < 0.01f)
    {
        speed.y = 0;
    }
    
    Pack("H", 0);
    PackIfFlag(flags, BasicFlags_Definition,"LLL", def->type.subtypeHashIndex, def->type.index, def->seed);
    
    if(flags & BasicFlags_Definition)
    {
        unsigned char* essenceCount = buff;
        Pack("H", 0);
        
        u16 essenceC = 0;
        for(u16 essenceIndex = 0; essenceIndex < ArrayCount(def->essences); ++essenceIndex)
        {
            if(def->essences[essenceIndex])
            {
                ++essenceC;
                Pack("HH", essenceIndex, def->essences[essenceIndex]);
            }
        }
        
        pack(essenceCount, "H", essenceC);
    }
    
    PackIfFlag(flags, BasicFlags_Position, "hhhV", def->P.chunkX, def->P.chunkY, def->P.chunkZ, def->P.chunkOffset);
    PackIfFlag(flags, BasicFlags_Velocity, "V", speed);
    PackU32(def->flags, flags, BasicFlags_Flags);
    PackIfFlag(flags, BasicFlags_Spawner, "L", def->spawnerID);
    
    if(flags)
    {
        result = ForgEndPacket_(buff_, buff);
        pack(flagDest, "H", flags);
    }
    
    return result;
}

internal u16 PrepareActionUpdate(ServerState* server, EntityID ID, b32 completeUpdate, b32 staticUpdate, unsigned char* buff_)
{
    u16 result = 0;
    
    ActionComponent* action = GetComponent(server, ID, ActionComponent);
    if(action)
    {
        u16 flags = 0;
        if(completeUpdate)
        {
            flags = 0xffff;
        }
        
        unsigned char* buff = ForgPackHeader(buff_, Type_Action);
        unsigned char* flagDest = buff;
        Pack("H", 0);
        
        PackU16(action->action, flags, ActionFlags_Action);
        PackR32(action->speed, flags, ActionFlags_Speed);
        if(flags)
        {
            result = ForgEndPacket_(buff_, buff);
            pack(flagDest, "H", flags);
        }
    }
    return result;
}

internal u16 PrepareHealthUpdate(ServerState* server, EntityID ID, b32 completeUpdate, b32 staticUpdate, unsigned char* buff_)
{
    u16 result = 0;
    HealthComponent* health = GetComponent(server, ID, HealthComponent);
    if(health)
    {
        u16 flags = 0;
        if(completeUpdate)
        {
            flags = 0xffff;
        }
        
        unsigned char* buff = ForgPackHeader(buff_, Type_Health);
        unsigned char* flagDest = buff;
        Pack("H", 0);
        
        PackR32(health->physicalHealth, flags, HealthFlag_Physical);
        PackR32(health->maxPhysicalHealth, flags, HealthFlag_MaxPhysical);
        PackR32(health->mentalHealth, flags, HealthFlag_Mental);
        PackR32(health->maxMentalHealth, flags, HealthFlag_MaxMental);
        PackR32(health->onFirePercentage, flags, HealthFlag_OnFirePercentage);
        PackR32(health->poisonPercentage, flags, HealthFlag_PoisonPercentage);
        
        if(flags)
        {
            result = ForgEndPacket_(buff_, buff);
            pack(flagDest, "H", flags);
        }
    }
    
    return result;
}

internal u16 PrepareCombatUpdate(ServerState* server, EntityID ID, b32 completeUpdate, b32 staticUpdate, unsigned char* buff_)
{
    u16 result = 0;
    CombatComponent* combat = GetComponent(server, ID, CombatComponent);
    if(combat)
    {
        u16 flags = 0;
        if(completeUpdate)
        {
            flags = 0xffff;
        }
        
        unsigned char* buff = ForgPackHeader(buff_, Type_Combat);
        unsigned char* flagDest = buff;
        Pack("H", 0);
        
        PackR32(combat->attackDistance, flags, CombatFlag_AttackDistance);
        PackR32(combat->attackContinueCoeff, flags, CombatFlag_AttackContinueCoeff);
        
        if(flags)
        {
            result = ForgEndPacket_(buff_, buff);
            pack(flagDest, "H", flags);
        }
    }
    
    return result;
}

internal u16 PrepareLightUpdate(ServerState* server, EntityID ID, b32 completeUpdate, b32 staticUpdate, unsigned char* buff_)
{
    u16 result = 0;
    LightComponent* light = GetComponent(server, ID, LightComponent);
    if(light)
    {
        u16 flags = 0;
        if(completeUpdate)
        {
            flags = 0xffff;
        }
        
        unsigned char* buff = ForgPackHeader(buff_, Type_Light);
        unsigned char* flagDest = buff;
        Pack("H", 0);
        
        PackR32(light->lightRadious, flags, LightFlag_LightRadious);
        
        if(flags)
        {
            result = ForgEndPacket_(buff_, buff);
            pack(flagDest, "H", flags);
        }
    }
    
    return result;
}

internal u16 PrepareVegetationUpdate(ServerState* server, EntityID ID, b32 completeUpdate, b32 staticUpdate, unsigned char* buff_)
{
    u16 result = 0;
    VegetationComponent* vegetation = GetComponent(server, ID, VegetationComponent);
    if(vegetation)
    {
        u16 flags = 0;
        if(completeUpdate)
        {
            flags = 0xffff;
        }
        
        unsigned char* buff = ForgPackHeader(buff_, Type_Vegetation);
        unsigned char* flagDest = buff;
        Pack("H", 0);
        
        PackR32(vegetation->flowerDensity, flags, VegetationFlag_FlowerDensity);
        PackR32(vegetation->fruitDensity, flags, VegetationFlag_FruitDensity);
        
        if(flags)
        {
            result = ForgEndPacket_(buff_, buff);
            pack(flagDest, "H", flags);
        }
    }
    
    return result;
}

internal unsigned char* PrepareContainerUpdate(unsigned char* buff, ContainerComponent* container, u16* count)
{
    if(IsDirty(container->openedBy))
    {
        ++(*count);
        Pack("CHHL", Mapping_OpenedBy, 0, 0, GetBoundedID(container->openedBy).archetype_archetypeIndex);
        ResetDirty(&container->openedBy);
    }
    
    for(u16 objectIndex = 0; objectIndex < ArrayCount(container->storedObjects); ++objectIndex)
    {
        InventorySlot* slot = container->storedObjects + objectIndex;
        
        if(IsDirty(slot))
        {
            ++(*count);
            Pack("CHLL", Mapping_ContainerStored, objectIndex, slot->flags_type, GetBoundedID(slot).archetype_archetypeIndex);
            ResetDirty(slot);
        }
    }
    
    for(u16 objectIndex = 0; objectIndex < ArrayCount(container->usingObjects); ++objectIndex)
    {
        InventorySlot* slot = container->usingObjects + objectIndex;
        if(IsDirty(slot))
        {
            ++(*count);
            Pack("CHLL", Mapping_ContainerUsing, objectIndex, slot->flags_type, GetBoundedID(slot).archetype_archetypeIndex);
            ResetDirty(slot);
        }
    }
    
    return buff;
}

internal u16 PrepareMappingsUpdate(ServerState* server, EntityID ID, b32 completeUpdate, b32 staticUpdate, unsigned char* buff_)
{
    EquipmentComponent* equipment = GetComponent(server, ID, EquipmentComponent);
    UsingComponent* equipped = GetComponent(server, ID, UsingComponent);
    ContainerComponent* container = GetComponent(server, ID, ContainerComponent);
    
    unsigned char* buff = ForgPackHeader(buff_, Type_Mappings);
    unsigned char* countGoesHere = buff;
    Pack("H", 0);
    
    u16 result = 0;
    u16 count = 0;
    if(equipment)
    {
        for(u16 slotIndex = 0; slotIndex < ArrayCount(equipment->slots); ++slotIndex)
        {
            InventorySlot* slot = equipment->slots + slotIndex;
            if(IsDirty(slot))
            {
                ++count;
                Pack("CHLL", Mapping_Equipment, slotIndex, slot->flags_type, GetBoundedID(slot).archetype_archetypeIndex);
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
        for(u16 slotIndex = 0; slotIndex < ArrayCount(equipped->slots); ++slotIndex)
        {
            InventorySlot* slot = equipped->slots + slotIndex;
            if(IsDirty(slot))
            {
                ++count;
                Pack("CHLL", Mapping_Using, slotIndex, slot->flags_type, GetBoundedID(slot).archetype_archetypeIndex);
                ResetDirty(slot);
            }
        }
    }
    
    if(container)
    {
        buff = PrepareContainerUpdate(buff, container, &count);
    }
    
    if(count > 0)
    {
        pack(countGoesHere, "H", count);
        result = ForgEndPacket_(buff_, buff);
    }
    
    return result;
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

internal void QueueUpdateToNearbyPlayers(ServerState* server, UniversePos P, EntityID ID, unsigned char* buffer, u16 size, b32 orderedAndGuaranteed)
{
    if(size > 0)
    {
        SpatialPartitionQuery playerQuery = QuerySpatialPartitionAtPoint(&server->playerPartition, P);
        for(EntityID playerID = GetCurrent(&playerQuery); IsValid(&playerQuery); playerID = Advance(&playerQuery))
        {
            PlayerComponent* player = GetComponent(server, playerID, PlayerComponent);
            //if(physic->P.chunkZ == playerPhysic->P.chunkZ)
            u8* writeHere;
            if(orderedAndGuaranteed)
            {
                QueueEntityHeaderReliably(player, ID);
                writeHere = ForgReserveSpace(player, GuaranteedDelivery_Ordered, ForgNetworkFlag_Ordered, size, ID);
            }
            else
            {
                QueueEntityHeader(player, ID);
                writeHere = ForgReserveSpace(player, GuaranteedDelivery_None, 0, size, ID);
            }
            if(writeHere)
            {
                Copy(size, writeHere, buffer);
            }
            else
            {
                InvalidCodePath;
            }
        }
    }
}

#define QueueStandardUpdateWork(name) u16 name##size = Prepare##name##Update(server, ID, completeUpdate, staticUpdate, buffer); QueueUpdateToNearbyPlayers(server, def->P, ID, buffer, name##size, false); 

#define QueueOrderedUpdateWork(name) u16 name##size = Prepare##name##Update(server, ID, completeUpdate, staticUpdate, buffer); QueueUpdateToNearbyPlayers(server, def->P, ID, buffer, name##size, true); 

internal void SendEntityUpdate(ServerState* server, EntityID ID, b32 staticUpdate, b32 completeUpdate)
{
    DefaultComponent* def = GetComponent(server, ID, DefaultComponent);
    if(IsSet(GetU32(def->flags), EntityFlag_deleted))
    {
        SpatialPartitionQuery playerQuery = QuerySpatialPartitionAtPoint(&server->playerPartition, def->P);
        for(EntityID playerID = GetCurrent(&playerQuery); IsValid(&playerQuery); playerID = Advance(&playerQuery))
        {
            PlayerComponent* player = GetComponent(server, playerID, PlayerComponent);
            for(u32 messageIndex = 0; messageIndex < 5; ++messageIndex)
            {
                QueueDeletedID(player, ID);
            }
        }
    }
    else
    {
        if(!def->updateSent || staticUpdate)
        {
            def->updateSent = true;
            
            unsigned char buffer[KiloBytes(2)];
            
            QueueStandardUpdateWork(Basics);
            QueueStandardUpdateWork(Action);
            QueueStandardUpdateWork(Health);
            QueueStandardUpdateWork(Combat);
            QueueStandardUpdateWork(Light);
            QueueStandardUpdateWork(Vegetation);
            QueueOrderedUpdateWork(Mappings);
        }
    }
}

internal void SendStaticUpdate(ServerState* server, EntityID ID)
{
    SendEntityUpdate(server, ID, true, true);
}

internal void QueueDayTime(PlayerComponent* player, u16 dayTime)
{
    StartPacket(player, DayTime);
    Pack("H", dayTime);
    QueueOrderedPacket(player);
}

internal void QueueEventTrigger(PlayerComponent* player, EntityID ID, EntityID targetID, u16 eventIndex)
{
    StartPacket(player, EventTrigger);
    Pack("LLH", ID, targetID, eventIndex);
    QueueOrderedPacket(player);
}

internal void QueueEventTrigger(ServerState* server, EntityID ID, EntityID targetID, u16 eventIndex)
{
    DefaultComponent* def = GetComponent(server, targetID, DefaultComponent);
    
    SpatialPartitionQuery playerQuery = QuerySpatialPartitionAtPoint(&server->playerPartition, def->P);
    for(EntityID playerID = GetCurrent(&playerQuery); IsValid(&playerQuery); playerID = Advance(&playerQuery))
    {
        PlayerComponent* player = GetComponent(server, playerID, PlayerComponent);
        QueueEventTrigger(player, ID, targetID, eventIndex);
    }
}
