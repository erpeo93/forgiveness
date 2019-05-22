inline u8* ForgReserveSpace(ServerPlayer* player, b32 reliable, u16 size, u64 identifier)
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

inline void CloseAndStore(ServerPlayer* player, unsigned char* buff_, unsigned char* buff, b32 reliableAndOrdered, u64 identifier = 0)
{
    u16 totalSize = ForgEndPacket_(buff_, buff);
    u8* writeHere = ForgReserveSpace(player, reliableAndOrdered, totalSize, identifier);
    Assert(writeHere);
    if(writeHere)
    {
        Copy(totalSize, writeHere, buff_);
    }
}

inline void QueueAndFlushAllPackets(ServerState* server, ServerPlayer* player, ForgNetworkPacketQueue* queue, b32 reliable)
{
    NetworkSendParams params = {};
    if(reliable)
    {
        params.guaranteedDelivery = GuaranteedDelivery_ResendEverySecond;
        params.criticalLevel = NetworkCritical_Important;
    }
    for(ForgNetworkPacket* toSend = queue->firstPacket; toSend; toSend = toSend->next)
    {
        platformAPI.net.QueuePacket(&server->clientInterface, player->connectionSlot, params, toSend->data, toSend->size);
    }
    
    queue->firstPacket = 0;
    queue->lastPacket = 0;
    Clear(&queue->tempPool);
}

inline b32 QueueAndFlushAllPackets(ServerState* server, ServerPlayer* player, r32 timeToAdvance)
{
    QueueAndFlushAllPackets(server, player, &player->standardPacketQueue, false);
    QueueAndFlushAllPackets(server, player, &player->reliablePacketQueue, true);
    b32 result = platformAPI.net.FlushSendQueue(&server->clientInterface, player->connectionSlot, timeToAdvance);
    
    return result;
}

#define StartPacket(player, type) unsigned char buff_[2048]; unsigned char* buff = ForgPackHeader( buff_, Type_##type);

#define Pack(formatString, ...) buff += pack(buff, formatString, ##__VA_ARGS__)

#if 0
internal Server * GetServer( ServerState * server, i32 port )
{
    InvalidCodePath;
    Server * result = 0;
    return result;
}

internal void AddServerInfoIfNecessary( ServerState* server, u32 otherIP, u16 otherPort )
{
    InvalidCodePath;
}
#endif

internal ServerPlayer* FirstFreePlayer(ServerState* server)
{
    ServerPlayer* result = server->firstFree;
    if(!result)
    {
        Assert(server->currentPlayerIndex < (MAXIMUM_SERVER_PLAYERS - 1));
        u32 index = ++server->currentPlayerIndex;
        
        result = server->players + index;
        result->playerID = index;
        
        result->standardPacketQueue = {};
        result->reliablePacketQueue = {};
    }
    
    result->standardPacketQueue.nextSendApplicationData = {};
    result->reliablePacketQueue.nextSendApplicationData = {};
    
    Assert(!result->standardPacketQueue.firstPacket);
    Assert(!result->standardPacketQueue.lastPacket);
    
    Assert(!result->reliablePacketQueue.firstPacket);
    Assert(!result->reliablePacketQueue.lastPacket);
    
    result->connectionClosed = false;
    result->overlappingEntityID = 0;
    result->requestCount = 0;
    result->ignoredActionCount = 0;
    result->draggingEntity = {};
    result->unlockedCategoryCount = 0;
    result->recipeCount = 0;
    result->allDataFileSent = false;
    result->allPakFileSent = false;
    result->pakFileIndex = 0;
    result->pakFileOffset = 0;
    
    
    ResetReceiver(&result->receiver);
    
    server->firstFree = result->next;
    result->next = 0;
    return result;
}

inline void RecyclePlayer(ServerState* server, ServerPlayer* player)
{
    player->nextFree = server->firstFree;
    server->firstFree = player;
}

internal void SendLoginResponse(ServerPlayer* player, u16 port, u32 challenge, b32 editingEnabled)
{
    StartPacket(player, login);
    Pack("HLl", port, challenge, editingEnabled);
    CloseAndStoreReliablePacket(player);
}

internal void SendGameAccessConfirm(ServerPlayer* player, u64 worldSeed, u64 identifier, u64 openedContainerID, u8 additionalMS5x)
{
    StartPacket(player, gameAccess);
    Pack("LQQC", worldSeed, identifier, openedContainerID, additionalMS5x);
    CloseAndStoreStandardPacket(player);
}


internal void SendUnlockedSkillCatConfirm( ServerPlayer* player, u32 taxonomy)
{
    StartPacket(player, UnlockSkillCategoryRequest);
    Pack("L", taxonomy);
    CloseAndStoreReliablePacket(player);
}


inline void SendNewRecipeMessage(ServerPlayer* player, Recipe* recipe)
{
    StartPacket(player, NewRecipe);
    Pack("LQ", recipe->taxonomy, recipe->gen.generic);
    
    CloseAndStoreReliablePacket(player);
}


internal unsigned char* SubCategoryOperation(TaxonomyTable* table, ServerPlayer* player, TaxonomySlot* slot, u32* total, unsigned char* buff)
{
    if(buff)
    {
        b32 unlocked = false;
        for(u32 catIndex = 0; catIndex < player->unlockedCategoryCount; ++catIndex)
        {
            if(player->unlockedSkillCategories[catIndex] == slot->taxonomy)
            {
                unlocked = true;
                break;
            }
        }
        
        Pack("lL", unlocked, slot->taxonomy);
    }
    
    Assert(slot->subTaxonomiesCount);
    u32 count = 1;
    
    for(u32 childIndex = 0; childIndex < slot->subTaxonomiesCount; ++childIndex)
    {
        u32 childTaxonomy = GetNthChildTaxonomy(table, slot, childIndex);
        TaxonomySlot* childSlot = GetSlotForTaxonomy(table, childTaxonomy);
        if(childSlot->subTaxonomiesCount)
        {
            u32 subTotal;
            buff = SubCategoryOperation(table, player, childSlot, &subTotal, buff);
            
            count += subTotal;
        }
    }
    
    *total = count;
    return buff;
}


internal void SendAvailableRecipes(ServerPlayer* player, TaxonomyTable* table)
{
    StartPacket(player, AvailableRecipes);
    
    TaxonomySlot* recipeRoot = NORUNTIMEGetTaxonomySlotByName(table, "equipment");
    
    u32 categoryCount;
    SubCategoryOperation(table, player, recipeRoot, &categoryCount, 0);
    Pack("L", categoryCount);
    
    buff = SubCategoryOperation(table, player, recipeRoot, &categoryCount, buff);
    CloseAndStoreReliablePacket(player);
    
    for(u32 recipeIndex = 0; recipeIndex < player->recipeCount; ++recipeIndex)
    {
        Recipe* recipe = player->recipes + recipeIndex;
        SendNewRecipeMessage(player, recipe);
    }
}




internal void SendSkillLevel(ServerPlayer* player, u32 taxonomy, u32 level, b32 isPassiveSkill, r32 power, b32 levelUp)
{
    StartPacket(player, SkillLevel);
    Pack("lLLld", levelUp, taxonomy, level, isPassiveSkill, power);
    CloseAndStoreReliablePacket(player);
}

inline void SendSkillLevelUp(ServerPlayer* player, SkillSlot* skill, b32 passive)
{
    SendSkillLevel(player, skill->taxonomy, skill->level, passive, skill->power, true);
}

internal void SendAllSkills(SimRegion* region, SimEntity* entity, ServerPlayer* player, TaxonomyTable* table, TaxonomySlot* slot)
{
    CreatureComponent* creature = Creature(region, entity);
    for(u32 subTaxonomyIndex = 0; subTaxonomyIndex < slot->subTaxonomiesCount; ++subTaxonomyIndex)
    {
        TaxonomySlot* childSlot = GetNthChildSlot(table, slot, subTaxonomyIndex);
        u32 taxonomy = childSlot->taxonomy;
        
        if(!childSlot->subTaxonomiesCount)
        {
            b32 isPassive = childSlot->isPassiveSkill;
            u32 skillLevel = 0;
            r32 skillPower = 0;
            for(u32 skillIndex = 0; skillIndex < creature->skillCount; ++skillIndex)
            {
                SkillSlot* skillSlot = creature->skills + skillIndex;
                if(skillSlot->taxonomy == taxonomy)
                {
                    skillLevel = skillSlot->level;
                    skillPower = skillSlot->power;
                }
            }
            SendSkillLevel(player, taxonomy, skillLevel, isPassive, skillPower, false);
        }
        else
        {
            SendAllSkills(region, entity, player, table, childSlot);
        }
        
    }
}

internal void SendSkills(SimRegion* region, SimEntity* entity, ServerPlayer* player, TaxonomyTable* table)
{
    StartPacket(player, SkillCategories);
    
    TaxonomySlot* skillsRoot = NORUNTIMEGetTaxonomySlotByName(table, "skills");
    
    u32 categoryCount;
    SubCategoryOperation(table, player, skillsRoot, &categoryCount, 0);
    Pack("L", categoryCount);
    
    buff = SubCategoryOperation(table, player, skillsRoot, &categoryCount, buff);
    CloseAndStoreReliablePacket(player);
    
    
    SendAllSkills(region, entity, player, table, skillsRoot);
}

internal void SendServerInfoToPlayer( ServerState* server, SimEntity* entity, i32 newWorldID, i32 deltaX, i32 deltaY )
{
    //NotImplemented;
}

internal void SendInfoToServer(NetworkConnection* connection, i16 myPort, i16 myUserPort )
{
    //NotImplemented;
    
#if 0    
    unsigned char buff_[1024];
    unsigned char* buff = PackHeader( buff_, Type_serverInfo, 0 );
    
    buff +=pack( buff, "hh", myPort, myUserPort );
    
    u32 totalSize = ForgEndPacket( buff_, buff );
    Win32SendData( socketHandle, buff_, totalSize );
#endif
    
}

internal u16 PrepareEntityUpdate(SimRegion* region, SimEntity* entity, unsigned char* buff_)
{
    UniversePos P = GetUniverseP(region, entity->P);
    
    r32 lifePoints = 0;
    r32 maxLifePoints = 0;
    u8 action = 0;
    
    r32 plantTotalAge = 0;
    r32 plantStatusPercentage = 0;
    u8 plantStatus = 0;
    
    if(entity->IDs[Component_Creature])
    {
        CreatureComponent* creature = Creature(region, entity);
        lifePoints = creature->lifePoints;
        maxLifePoints = creature->maxLifePoints;
        action = SafeTruncateToU8(entity->action);
    }
    
    unsigned char* buff = ForgPackHeader(buff_, Type_entityBasics);
    Pack("llVLLQCLddd", P.chunkX, P.chunkY, P.chunkOffset, entity->flags, entity->taxonomy, entity->gen.generic, SafeTruncateToU8(action), entity->recipeTaxonomy, lifePoints, maxLifePoints, entity->status);
    u16 totalSize = ForgEndPacket_( buff_, buff );
    return totalSize;
}

inline HashEntityUpdate* GetHashUpdate(SimRegion* region, u64 identifier)
{
    u32 mask = HASH_UPDATE_COUNT - 1;
    
    HashEntityUpdate* found = 0;
    HashEntityUpdate* firstFree = 0;
    
    u32 index = identifier & mask;
    u32 examined = 0;
    while( !found )
    {
        Assert( examined++ < HASH_UPDATE_COUNT );
        HashEntityUpdate* test = region->updateHash + index;
        if( !test->identifier || test->identifier == identifier )
        {
            found = test;
            break;
        }
        else
        {
            if( !test->valid )
            {
                firstFree = test;
            }
        }
        
        if( ++index >= HASH_UPDATE_COUNT )
        {
            index = 0;
        }
    }
    
    if( !found )
    {
        found = firstFree;
    }
    
    Assert( found );
    return found;
}

internal void StoreUpdate(SimRegion* destRegion, SimRegion* sourceRegion, u64 identifier)
{
    Assert(destRegion->border == Border_Mirror);
    Assert(destRegion->updateHash);
    
    HashEntityUpdate* upd = GetHashUpdate(destRegion, identifier);
    //Assert(!upd->valid);
    
    upd->valid = true;
    
    SimEntity* sourceEntity = GetRegionEntityByID(sourceRegion, identifier);
    upd->identifier = identifier;
    upd->entity = *sourceEntity;
}

internal void SendUpdateTo(SimRegion* region, i32 destX, i32 destY, u64 identifier)
{
    ServerState* server = region->server;
    
    if(UNIVERSE_DIM == 1)
    {
        SimRegion* destRegion = GetServerRegionWrap(server, destX, destY);
        StoreUpdate(destRegion, region, identifier);
    }
    else
    {
        InvalidCodePath;
        
#if 0        
        destX = Wrap(0, destX, SERVER_REGION_SPAN, &universeX);
        destY = Wrap(0, destY, SERVER_REGION_SPAN, &universeY);
        
        universeX = Wrap(0, universeX, UNIVERSE_DIM);
        universeY = Wrap(0, universeY, UNIVERSE_DIM);
#endif
        
        
#if 0
        if(PositionOutsideWorld)
        {
            SendComponentsAsWell();
        }
#endif
        
        //SendUpdateTo( universeX, universeY );
        // NOTE(Leonardo): other server!
        //NotImplemented;
    }
}

internal void SendUpdateToAdiacentRegions(SimRegion* region, u64 identifier)
{
    
    switch(region->border)
    {
        case Border_None:
        {
            
        } break;
        
        case Border_RightDown:
        {
            SendUpdateTo(region, region->regionX + 1, region->regionY, identifier);
            SendUpdateTo(region, region->regionX + 1, region->regionY - 1, identifier);
            SendUpdateTo(region, region->regionX, region->regionY - 1, identifier);
        } break;
        
        case Border_DownLeft:
        {
            SendUpdateTo(region, region->regionX, region->regionY - 1, identifier);
            SendUpdateTo(region, region->regionX - 1, region->regionY, identifier);
            SendUpdateTo(region, region->regionX - 1, region->regionY - 1, identifier);
        } break;
        
        case Border_LeftUp:
        {
            SendUpdateTo(region, region->regionX - 1, region->regionY, identifier);
            SendUpdateTo(region, region->regionX, region->regionY + 1, identifier);
            SendUpdateTo(region, region->regionX - 1, region->regionY + 1, identifier);
        } break;
        
        case Border_UpRight:
        {
            SendUpdateTo(region, region->regionX, region->regionY + 1, identifier);
            SendUpdateTo(region, region->regionX + 1, region->regionY, identifier);
            SendUpdateTo(region, region->regionX + 1, region->regionY + 1, identifier);
        } break;
        
        case Border_Right:
        {
            SendUpdateTo(region, region->regionX + 1, region->regionY, identifier);
        } break;
        
        case Border_Down:
        {
            SendUpdateTo(region, region->regionX, region->regionY - 1, identifier);
        } break;
        
        case Border_Left:
        {
            SendUpdateTo(region, region->regionX - 1, region->regionY, identifier);
        } break;
        
        case Border_Up:
        {
            SendUpdateTo(region, region->regionX, region->regionY + 1, identifier);
        } break;
        
        InvalidDefaultCase;
    }
}

internal void SendTileUpdate( ServerPlayer* player, i32 chunkX, i32 chunkY, u32 tileX, u32 tileY, r32 waterAmount )
{
    //NotImplemented;
    
#if 0    
    b32 result = false;
    ConstantPlayerVariables* c = player->c;
    i32 socketHandle = player->socketHandle;
    Assert( socketHandle );
    
    unsigned char buff_[1024];
    unsigned char* buff = PackHeader( buff_, Type_tileUpdate, c->recvValue );
    buff +=pack( buff, "llLLd", chunkX, chunkY, tileX, tileY, waterAmount );
    
    u32 totalSize = ForgEndPacket( buff_, buff );
    if( SendData( socketHandle, buff_, totalSize ) )
    {
        c->recvValue = GetNextUInt32( &c->recv ) + GetNextUInt32( &c->recvOffset );
    }
#endif
    
}

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
            if(region->border != Border_Mirror || !IsSet(entityToSend, Flag_insideRegion))
            {
                if(entityToSend->playerID)
                {
                    ServerPlayer* player = server->players + entityToSend->playerID;
                    StartPacket(player, deletedEntity);
                    
                    Pack("Q", entity->identifier);
                    CloseAndStoreReliablePacket(player);
                }
            }
        }
        playerSurfaceBlock = playerSurfaceBlock->next;
    }
    
    if(region->border > Border_Mirror)
    {
        Assert(!region->updateHash);
    }
}

inline void SendEntityHeader(ServerPlayer* player, u64 ID)
{
    StartPacket(player, entityHeader);
    Pack("Q", ID);
    CloseAndStoreStandardPacket(player);
}

inline void SendEntityHeaderReliably(ServerPlayer* player, u64 ID)
{
    StartPacket(player, entityHeader);
    Pack("Q", ID);
    CloseAndStoreReliablePacket(player);
}

inline void SendPlantUpdate(ServerPlayer* player, u64 entityID, PlantComponent* plant)
{
    StartPacket(player, plantUpdate);
    Pack("dddd", plant->age, plant->life, plant->leafDensity, plant->leafDimension);
    CloseAndStoreStandardPacket(player, entityID);
}

inline void SendEquipmentID(ServerPlayer* player, u64 entityID, u8 slotIndex, u64 ID)
{
    StartPacket(player, equipmentSlot);
    Pack("CQ", slotIndex, ID);
    CloseAndStoreStandardPacket(player, entityID);
}

inline void SendStartedAction(ServerPlayer* player, u64 entityID, u8 actionIndex, u64 targetID)
{
    StartPacket(player, StartedAction);
    Pack("CQ", actionIndex, targetID);
    CloseAndStoreStandardPacket(player, entityID);
}

inline void SendCompletedAction(ServerPlayer* player, u64 entityID, u8 actionIndex, u64 targetID)
{
    StartPacket(player, CompletedAction);
    Pack("CQ", actionIndex, targetID);
    CloseAndStoreReliablePacket(player, entityID);
}

inline void SendSyncAction(ServerPlayer* player, u8 actionIndex)
{
    StartPacket(player, SyncAction);
    Pack("C", actionIndex);
    CloseAndStoreReliablePacket(player, 0);
}

inline void SendObjectEntityHeader(ServerPlayer* player, u64 containerID)
{
    StartPacket(player, containerHeader);
    Pack("Q", containerID);
    CloseAndStoreReliablePacket(player);
}

inline void SendContainerInfo_(ServerPlayer* player, u8 maxObjectCount)
{
    StartPacket(player, containerInfo);
    Pack("C", maxObjectCount);
    CloseAndStoreReliablePacket(player);
}

inline void SendContainerInfo(ServerPlayer* player, u64 identifier, u8 maxObjectCount)
{
    SendObjectEntityHeader(player, identifier);
    SendContainerInfo_(player, maxObjectCount);
}

inline void SendObjectRemoveUpdate(ServerPlayer* player, u8 objectIndex)
{
    StartPacket(player, objectRemoved);
    Pack("C", objectIndex);
    CloseAndStoreReliablePacket(player);
}

inline void SendObjectAddUpdate(ServerPlayer* player, u8 objectIndex, Object* object)
{
    StartPacket(player, objectAdded);
    Pack("CLQHh", objectIndex, object->taxonomy, object->gen.generic, object->quantity, object->status);
    CloseAndStoreReliablePacket(player);
}

inline Object* GetObject(SimRegion* region, SimEntity* container, u32 objectIndex);
inline void SendCompleteContainerInfoIdentifier(SimRegion* region, ServerPlayer* player, u64 ID, SimEntity* container)
{
    ObjectComponent* object = Object(region, container);
    SendContainerInfo(player, ID, object->objects.maxObjectCount);
    for(u8 objectIndex = 0; objectIndex < object->objects.maxObjectCount; ++objectIndex)
    {
        Object* toSend = GetObject(region, container, objectIndex);
        if(toSend->taxonomy)
        {
            SendObjectAddUpdate(player, objectIndex, toSend);
        }
    }
}


inline void SendCompleteContainerInfo(SimRegion* region, ServerPlayer* player, SimEntity* container)
{
    SendCompleteContainerInfoIdentifier(region, player, container->identifier, container);
}

inline void SendEssenceDelta(ServerPlayer* player, u32 essenceTaxonomy, i16 delta)
{
    StartPacket(player, essenceDelta);
    Pack("Lh", essenceTaxonomy, delta);
    CloseAndStoreReliablePacket(player);
}

inline void SendEffectTriggered(ServerPlayer* player, EffectTriggeredToSend* toSend)
{
    StartPacket(player, effectTriggered);
    Pack("QQL", toSend->actor, toSend->target, toSend->ID);
    CloseAndStoreReliablePacket(player);
}

internal void SendEntityUpdate(SimRegion* region, SimEntity* entity)
{
    ServerState* server = region->server;
    unsigned char buff_[KiloBytes(2)];
    Assert(sizeof(EntityUpdate) < ArrayCount(buff_));
    
    u16 totalSize = PrepareEntityUpdate(region, entity, buff_);
    
    PartitionSurfaceEntityBlock* playerSurfaceBlock = QuerySpacePartitionPoint(region, &region->playerPartition, entity->P);
    while(playerSurfaceBlock)
    {
        for( u32 playerIndex = 0; playerIndex < playerSurfaceBlock->entityCount; ++playerIndex )
        {
            CollisionData* collider = playerSurfaceBlock->colliders + playerIndex;
            SimEntity* entityToSend = GetRegionEntity(region, collider->entityIndex);
            
            Assert(entityToSend->playerID);
            ServerPlayer* player = server->players + entityToSend->playerID;
            
            if(collider->insideRegion)
            {
                SendEntityHeader(player, entity->identifier);
                
                u8* writeHere = ForgReserveSpace(player, false, totalSize, entity->identifier);
                Assert(writeHere);
                if(writeHere)
                {
                    Copy(totalSize, writeHere, buff_);
                }
                
                if(entity->IDs[Component_Creature])
                {   
                    CreatureComponent* creature = Creature(region, entity);
                    for(u8 slotIndex = 0; slotIndex < Slot_Count; ++slotIndex)
                    {
                        u64 equipmentID = creature->equipment[slotIndex].ID;
                        if(equipmentID)
                        {
                            SendEquipmentID(player, entity->identifier, slotIndex, equipmentID);
                        }
                    }
                }
                
                if(entity->IDs[Component_Plant])
                {
                    PlantComponent* plant = Plant(region, entity);
                    SendPlantUpdate(player, entity->identifier, plant);
                }
            }
            
            
            if(entity->IDs[Component_Creature])
            {   
                CreatureComponent* creature = Creature(region, entity);
                if(creature->startedAction)
                {
                    SendStartedAction(player, entity->identifier, creature->startedAction, creature->startedActionTarget);
                }
                
                if(creature->completedAction)
                {
                    SendCompletedAction(player, entity->identifier, creature->completedAction, creature->completedActionTarget);
                }
            }
            
            
            if(collider->insideRegion)
            {
                if(entityToSend->targetID == entity->identifier)
                {
                    SendPossibleActions(region, entityToSend, entity, false);
                }
                
                if(player->overlappingEntityID == entity->identifier)
                {
                    SendPossibleActions(region, entityToSend, entity, true);
                }
            }
        }
        
        playerSurfaceBlock = playerSurfaceBlock->next;
    }
    
    if(IsSet(entity, Flag_insideRegion) && region->border > Border_Mirror)
    {
        Assert(!region->updateHash);
        //SendUpdateToAdiacentRegions(region, entity->identifier);
    }
}

inline void SendDataFileHeader(ServerPlayer* player, char* name, u32 fileSize, u32 chunkSize)
{
    StartPacket(player, DataFileHeader);
    Pack("sLL", name, fileSize, chunkSize);
    CloseAndStoreReliablePacket(player);
}

inline void SendPakFileHeader(ServerPlayer* player, char* name, u32 fileSize, u32 chunkSize)
{
    StartPacket(player, PakFileHeader);
    Pack("sLL", name, fileSize, chunkSize);
    CloseAndStoreReliablePacket(player);
}

inline void SendFileChunks(ServerPlayer* player, char* source, u32 sizeToSend, u32 chunkSize)
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

inline void SendDontDeleteFile(ServerPlayer* player, char* filename)
{
    StartPacket(player, DontDeleteFile);
    Pack("s", filename);
    CloseAndStoreReliablePacket(player);
}

internal void SendDataFile(ServerPlayer* player, char* name, char* source, u32 fileSize)
{
    u32 chunkSize = KiloBytes(1);
    {
        SendDataFileHeader(player, name, fileSize, chunkSize);
    }
    
    SendFileChunks(player, source, fileSize, chunkSize);
}

internal void SendAllDataFileSentMessage(ServerPlayer* player, b32 loadTaxonomies)
{
    StartPacket(player, AllDataFileSent);
    Pack("l", loadTaxonomies);
    CloseAndStoreReliablePacket(player);
}

internal void SendAllPakFileSentMessage(ServerPlayer* player)
{
    StartPacket(player, AllPakFileSent);
    CloseAndStoreReliablePacket(player);
}

internal void SendPatchDoneMessage(ServerPlayer* player)
{
    StartPacket(player, PatchLocalServer);
    CloseAndStoreReliablePacket(player);
}

inline void SendPatchDoneMessageToAllPlayers(ServerState* server)
{
    u32 playerCount = 0;
    for(u32 playerIndex = 0; playerIndex < MAXIMUM_SERVER_PLAYERS; ++playerIndex)
    {
        ServerPlayer* player = server->players + playerIndex;
        if(player->connectionSlot)
        {
            ++playerCount;
            SendPatchDoneMessage(player);
        }
    }
    Assert(playerCount == 1);
}

#if FORGIVENESS_INTERNAL
internal void SendDebugEvent(ServerPlayer* player, DebugEvent* event)
{
    
#if 0    
    b32 result = false;
    
    StartPacket(debugEvent);
    Pack("QQssLHCQQ", event->clock, event->GUID, event->GUID, event->name, event->threadID, event->coreIndex, event->type, event->overNetwork[0], event->overNetwork[1] );
    
    CloseAndStoreStandardPacket(player, ReliableOrdered);
#endif
    
}

internal void SendMemStats( ServerPlayer* player )
{
    DebugPlatformMemoryStats stats = platformAPI.DEBUGMemoryStats();
    b32 result = false;
    
    StartPacket(player, memoryStats);
    Pack("LQQ", stats.blockCount, stats.totalUsed, stats.totalSize );
    
    CloseAndStoreReliablePacket(player);
}
#endif

