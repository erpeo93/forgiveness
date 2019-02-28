internal void SendData(void* data, u16 size)
{
    platformAPI.net.SendData(myPlayer->network, 0, ForgNetwork_LastDataGuaranteed, data, size, true);
}

internal void LoginRequest(i32 password)
{
    unsigned char buff_[1024];
    unsigned char* buff = ForgPackHeader(buff_, Type_login);
    
    buff += pack(buff, "l", password);
    
    u16 totalSize = ForgEndPacket(buff_, buff);
    SendData(buff_, totalSize);
}

internal void GameAccessRequest(u32 challenge)
{
    unsigned char buff_[1024];
    unsigned char* buff = ForgPackHeader(buff_, Type_gameAccess);
    
    buff += pack(buff, "L", challenge);
    
    u16 totalSize = ForgEndPacket(buff_, buff);
    SendData(buff_, totalSize);
}

#if FORGIVENESS_INTERNAL
internal void SendEditingEvent( DebugEvent* event )
{
    Assert( event->pointer );
    if( myPlayer )
    {
        unsigned char buff_[2048];
        unsigned char* buff = ForgPackHeader(buff_, Type_debugEvent);
        
        buff += pack(buff, "QQLHCQQ", event->clock, event->pointer, event->threadID, event->coreIndex, event->type, event->overNetwork[0], event->overNetwork[1]);
        
        u16 totalSize = ForgEndPacket(buff_, buff);
        SendData( buff_, totalSize);
    }
}

internal void SendInputRecordingMessage( b32 recording, b32 startAutomatically )
{
    if( myPlayer )
    {
        unsigned char buff_[2048];
        unsigned char* buff = ForgPackHeader( buff_, Type_InputRecording);
        
        buff += pack(buff, "ll", recording, startAutomatically);
        
        u16 totalSize = ForgEndPacket( buff_, buff );
        SendData( buff_, totalSize );
    }
}
#endif

#define CloseAndSendStandardPacket() CloseAndSend(buff_, buff)
inline void CloseAndSend(unsigned char* buff_, unsigned char* buff)
{
    u16 totalSize = ForgEndPacket( buff_, buff );
    SendData( buff_, totalSize );
}

#define StartStandardPacket(type) unsigned char buff_[1024]; unsigned char* buff = ForgPackHeader( buff_, Type_##type);
#define Pack(formatString, ...) buff += pack(buff, formatString, ##__VA_ARGS__)
#define Unpack(formatString, ...) packetPtr = unpack(packetPtr, formatString, ##__VA_ARGS__)

internal void TrackUpdate( ClientPlayer* player, Vec3 acceleration, Vec3 velocity )
{
#if 0    
    NotImplemented;
    SimEntity* entity = &player->P;
    u32 ackIndex = player->firstFreeAck++;
    
    if( player->firstFreeAck == ArrayCount( player->updateToAck ) )
    {
        player->firstFreeAck = 0;
    }
    
    SendedUpdate* toFill = player->updateToAck + ackIndex;
    toFill->sequenceNumber = player->actionSequenceNumber;
    toFill->p = entity->regionPosition;
    toFill->acceleration = acceleration;
    toFill->velocity = velocity;
    toFill->valid = true;
#endif
    
}

internal void SendUpdate( Vec3 acceleration, u64 targetEntityID, u32 desiredAction, u64 overlappingEntityID)
{
    if(LengthSq(acceleration) > 0)
    {
        desiredAction = Action_Move;
    }
    
    unsigned char buff_[1024];
    unsigned char* buff = ForgPackHeader(buff_, Type_ActionRequest);
    
    buff += pack(buff, "LVLQQ", 0, acceleration, desiredAction, targetEntityID, overlappingEntityID);
    
    u16 totalSize = ForgEndPacket(buff_, buff);
    SendData( buff_, totalSize);
}

internal void SendEquipRequest(u64 sourceContainerID, u8 objectIndex)
{
    StartStandardPacket(EquipRequest);
    
    Pack("QC", sourceContainerID, objectIndex);
    
    CloseAndSendStandardPacket();
}

internal void SendDisequipRequest(u32 slotIndex, u64 destContainerID, u8 destObjectIndex)
{
    StartStandardPacket(DisequipRequest);
    
    Pack("LQC", slotIndex, destContainerID, destObjectIndex);
    
    CloseAndSendStandardPacket();
}

internal void SendDropRequest(u64 sourceContainerID, u8 objectIndex)
{
    StartStandardPacket(DropRequest);
    
    Pack("QC", sourceContainerID, objectIndex);
    
    CloseAndSendStandardPacket();
}

internal void SendMoveRequest(u64 sourceContainerID, u8 objectIndex, u64 destContainerID, u8 destObjectIndex)
{
    
    StartStandardPacket(MoveRequest);
    
    Pack("QCQC", sourceContainerID, objectIndex, destContainerID, destObjectIndex);
    
    CloseAndSendStandardPacket();
}

internal void SendSwapRequest(u64 sourceContainerID, u8 sourceObjectIndex)
{
    StartStandardPacket(SwapRequest);
    
    Pack("QC", sourceContainerID, sourceObjectIndex);
    
    CloseAndSendStandardPacket();
}

internal void SendDragEquipmentRequest(u32 slotIndex)
{
    StartStandardPacket(DragEquipmentRequest);
    
    Pack("L", slotIndex);
    
    CloseAndSendStandardPacket();
}

internal void SendEquipDraggingRequest(u32 slotIndex)
{
    StartStandardPacket(EquipDraggingRequest);
    
    Pack("L", slotIndex);
    
    CloseAndSendStandardPacket();
}


internal void SendCraftRequest(u32 taxonomy, u64 recipeIndex)
{
    StartStandardPacket(CraftRequest);
    
    Pack("LQ", taxonomy, recipeIndex);
    
    CloseAndSendStandardPacket();
}

internal void SendCraftFromInventoryRequest(u64 containerID, u32 objectIndex)
{
    StartStandardPacket(CraftFromInventoryRequest);
    
    Pack("QL", containerID, objectIndex);
    
    CloseAndSendStandardPacket();
}


internal void SendActiveSkillRequest(u32 taxonomy)
{
    StartStandardPacket(ActiveSkillRequest);
    
    Pack("L", taxonomy);
    
    CloseAndSendStandardPacket();
}

internal void SendPassiveSkillRequest(u32 taxonomy, b32 deactivate)
{
    StartStandardPacket(PassiveSkillRequest);
    
    Pack("Ll", taxonomy, deactivate);
    
    CloseAndSendStandardPacket();
}

internal void SendUnlockSkillCategoryRequest(u32 taxonomy)
{
    StartStandardPacket(UnlockSkillCategoryRequest);
    
    Pack("L", taxonomy);
    
    CloseAndSendStandardPacket();
}

internal void SendSkillLevelUpRequest(u32 taxonomy)
{
    StartStandardPacket(SkillLevelUpRequest);
    
    Pack("L", taxonomy);
    
    CloseAndSendStandardPacket();
}

internal void SendLearnRequest(u64 containerID, u32 objectIndex)
{
    StartStandardPacket(LearnRequest);
    
    Pack("QL", containerID, objectIndex);
    
    CloseAndSendStandardPacket();
}

internal void SendConsumeRequest(u64 containerID, u32 objectIndex)
{
    StartStandardPacket(ConsumeRequest);
    
    Pack("QL", containerID, objectIndex);
    
    CloseAndSendStandardPacket();
}

internal void SendEditRequest(u32 taxonomy, u32 role)
{
    StartStandardPacket(EditRequest);
    Pack("LL", taxonomy, role);
    CloseAndSendStandardPacket();
}

inline void SendPopMessage(b32 list, b32 pop = true)
{
    StartStandardPacket(PopEditorElement);
    Pack("ll", list, pop);
    CloseAndSendStandardPacket();
}


internal void SendEditorElements(EditorElement* root)
{
    StartStandardPacket(EditorElement);
    Pack("sLL", root->name, root->type, root->flags);
    if(root->type < EditorElement_List)
    {
        Pack("s", root->value);
    }
    else
    {
        
        if(root->type == EditorElement_List)
        {
            Pack("s", root->elementName);
        }
    }
    CloseAndSendStandardPacket();
    
	switch(root->type)
	{
		case EditorElement_Struct:
		{
			SendEditorElements(root->firstValue);
			SendPopMessage(false);
		} break;
        
		case EditorElement_List:
		{
            
            if(root->emptyElement)
            {
                SendEditorElements(root->emptyElement);
                SendPopMessage(true);
			}
            
            if(root->firstInList)
            {
                SendEditorElements(root->firstInList);
                SendPopMessage(false);
            }
            else
            {
                SendPopMessage(false, false);
            }
        } break;
        
        case EditorElement_String:
        case EditorElement_Real:
        case EditorElement_Signed:
        case EditorElement_Unsigned:
        {
            
        } break;
	}
    
	if(root->next)
	{
		SendEditorElements(root->next);
	}
}


inline void SendNewTabMessage()
{
    StartStandardPacket(NewEditorTab);
    CloseAndSendStandardPacket();
}

inline void SendSaveAssetDefinitionFile(char* fileName, EditorElement* root)
{
    SendNewTabMessage();
    SendEditorElements(root);
    
    StartStandardPacket(SaveAssetFadFile);
    Pack("s", fileName);
    CloseAndSendStandardPacket();
}

inline void SendReloadAssetsRequest()
{
    StartStandardPacket(ReloadAssets);
    CloseAndSendStandardPacket();
}

inline void SendPatchServerRequest()
{
    StartStandardPacket(PatchLocalServer);
    CloseAndSendStandardPacket();
}

inline void SendSaveTabRequest(u32 taxonomy)
{
    StartStandardPacket(SaveSlotTabToFile);
    Pack("L", taxonomy);
    CloseAndSendStandardPacket();
}

inline void SendReloadEditingMessage(u32 taxonomy, u32 tabIndex)
{
    StartStandardPacket(ReloadEditingSlot);
    Pack("LL", taxonomy, tabIndex);
    CloseAndSendStandardPacket();
}

inline void SendAddTaxonomyRequest(u32 parentTaxonomy, char* name)
{
    StartStandardPacket(AddTaxonomy);
    Pack("Ls", parentTaxonomy, name);
    CloseAndSendStandardPacket();
}

inline void SendDeleteTaxonomyRequest(u32 taxonomy)
{
    StartStandardPacket(DeleteTaxonomy);
    Pack("L", taxonomy);
    CloseAndSendStandardPacket();
}

inline void SendInstantiateTaxonomyRequest(u32 taxonomy, Vec3 offset)
{
    StartStandardPacket(InstantiateTaxonomy);
    Pack("LV", taxonomy, offset);
    CloseAndSendStandardPacket();
}

inline void SendDeleteRequest(u64 identifier)
{
    StartStandardPacket(DeleteEntity);
    Pack("Q", identifier);
    CloseAndSendStandardPacket();
}

inline void SendImpersonateRequest(u64 identifier)
{
    StartStandardPacket(ImpersonateEntity);
    Pack("Q", identifier);
    CloseAndSendStandardPacket();
}

inline void SendPauseToggleMessage()
{
    StartStandardPacket(PauseToggle);
    CloseAndSendStandardPacket();
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

inline void AddToElementBlock(UIState* UI, BookMode* mode, BookElement element)
{
    element.hot = false;
    BookElementsBlock* block = mode->elements;
    if(!block || block->elementCount == ArrayCount(block->elements))
    {
        BookElementsBlock* newBlock = PushStruct(&UI->bookElementsPool, BookElementsBlock);
        newBlock->next = block;
        mode->elements = newBlock;
        block = newBlock;
    }
    
    Assert(block->elementCount < ArrayCount(block->elements));
    block->elements[block->elementCount++] = element;
}

inline void AddToRecipeBlock(UIState* UI, Recipe recipe)
{
    BookElement element;
    element.type = Book_Recipe;
    element.taxonomy = recipe.taxonomy;
    element.recipeIndex = recipe.recipeIndex;
    
    AddToElementBlock(UI, UI->bookModes + UIBook_Recipes, element);
}

inline void AddToRecipeCategoryBlock(UIState* UI, u32 recipeCategory)
{
    BookElement element;
    element.type = Book_RecipeCategory;
    element.taxonomy = recipeCategory;
    
    AddToElementBlock(UI, UI->bookModes + UIBook_Recipes, element);
}

inline void AddToSkillBlock(UIState* UI, SkillSlot skill)
{
    BookElement element;
    element.type = Book_Skill;
    element.taxonomy = skill.taxonomy;
    element.skillPower = skill.power;
    element.skillLevel = skill.level;
    
    AddToElementBlock(UI, UI->bookModes + UIBook_Skills, element);
}


inline void AddToSkillCategoryBlock(UIState* UI, SkillCategory skillCategory)
{
    BookElement element;
    element.type = Book_SkillCategory;
    element.taxonomy = skillCategory.taxonomy;
    element.unlocked = skillCategory.unlocked;
    
    AddToElementBlock(UI, UI->bookModes + UIBook_Skills, element);
}

internal void ReceiveNetworkPackets(GameModeWorld* worldMode, UIState* UI)
{
    NetworkPacketReceived packet;
    while(true)
    {
        packet = platformAPI.net.GetPacketOnSlot(myPlayer->network, 0);
        unsigned char* packetPtr = packet.packetPtr;
        if(!packetPtr)
        {
            break;
        }
        
        ClientEntity* currentEntity = 0;
        ClientEntity* currentContainer = 0;
        u32 readSize = 0;
        u32 toReadSize = packet.info.dataSize;
        
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
                    
                    
                    u32 salt = 11111;
                    platformAPI.net.CloseConnection(myPlayer->network, 0);
                    platformAPI.net.OpenConnection(myPlayer->network, server, login.port,salt,ForgNetwork_Count, forgNetworkChannelParams, myPlayer->appRecv);
                    GameAccessRequest(login.challenge);
                } break;
                
                case Type_gameAccess:
                {
                    u64 openedContainerID;
                    u8 serverMS5x;
                    
                    Unpack("llQQC", &myPlayer->universeX, &myPlayer->universeY, &myPlayer->identifier, &openedContainerID, &serverMS5x);
                    
                    r32 lastFrameFPS = 1000.0f / (serverMS5x * 5.0f);
                    myPlayer->serverFPS = Lerp(myPlayer->serverFPS, 0.8f, lastFrameFPS);
                    myPlayer->openedContainerID = openedContainerID;
                } break;
                
                case Type_tileUpdate:
                {
                    i32 chunkX;
                    i32 chunkY;
                    
                    u32 tileX;
                    u32 tileY;
                    
                    r32 waterAmount;
                    
                    Unpack("llLLd", &chunkX, &chunkY, &tileX, &tileY, &waterAmount );
                    WorldChunk* chunk = GetChunk(worldMode->chunks, ArrayCount(worldMode->chunks), chunkX, chunkY, &worldMode->chunkPool);
                    
                    chunk->waterAmount[tileY][tileX] = waterAmount;
                } break;
                
                case Type_entityHeader:
                {
                    u64 identifier;
                    Unpack("Q", &identifier);
                    
                    currentEntity = GetEntityClient(worldMode, identifier, true);
                    currentEntity->identifier = identifier;
                    currentEntity->timeFromLastUpdate = 0.0f;
                } break;
                
                case Type_entityBasics:
                {
                    UniversePos P;
                    ClientEntity* e = currentEntity;
                    Assert(e);
                    
                    EntityAction oldAction = e->action;
                    
                    Unpack("llVLLQCddCLddd", &P.chunkX, &P.chunkY, &P.chunkOffset, &e->flags, &e->taxonomy, &e->recipeIndex, &e->action, &e->plantTotalAge, &e->plantStatusPercentage, &e->plantStatus, &e->recipeTaxonomy, &e->lifePoints, &e->maxLifePoints, &e->status);
                    
                    if(e->action != oldAction)
                    {
                        e->actionTime = 0;
                    }
                    
                    for(u32 slotIndex = 0; slotIndex < Slot_Count; ++slotIndex)
                    {
                        e->equipment[slotIndex].ID = 0;
                    }
                    
                    if(e->flags & Flag_deleted)
                    {
                        InvalidCodePath;
                    }
                    
                    
                    i32 lateralChunkSpan = SERVER_REGION_SPAN * SIM_REGION_CHUNK_SPAN;
                    if(e->identifier == myPlayer->identifier && ChunkOutsideWorld(lateralChunkSpan, P.chunkX, P.chunkY))
                    {
                        myPlayer->changedWorld = true;
                        if(P.chunkX < 0)
                        {
                            myPlayer->changedWorldDeltaX += lateralChunkSpan;
                        }
                        else if(P.chunkX >= lateralChunkSpan)
                        {
                            myPlayer->changedWorldDeltaX -= lateralChunkSpan;
                        }
                        
                        if(P.chunkY < 0)
                        {
                            myPlayer->changedWorldDeltaY += lateralChunkSpan;
                        }
                        else if(P.chunkY >= lateralChunkSpan)
                        {
                            myPlayer->changedWorldDeltaY -= lateralChunkSpan;
                        }
                    }
                    
                    
                    r32 maxDistancePrediction = 4.0f;
                    Vec3 deltaP = Subtract(P, e->universeP);
                    r32 deltaLength = Length(deltaP);
                    if(deltaLength >= maxDistancePrediction || (e->flags & Flag_Equipped))
                    {
                        e->universeP = P;
                        e->velocity = {};
                    }
                    else
                    {
                        e->velocity = deltaP * myPlayer->serverFPS;
                        if(e->identifier == myPlayer->identifier)
                        {
                            myPlayer->distanceCoeffFromServerP = deltaLength / maxDistancePrediction;
                        }
                    }
                } break;
                
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
                    Unpack("LQHh", &dest->taxonomy, &dest->recipeIndex, &dest->quantity, &dest->status);
                    ++currentContainer->objects.objectCount;
                } break;
                
                case Type_deletedEntity:
                {
                    u64 deletedID;
                    Unpack("Q", &deletedID);
                    ClientEntity* entityC = GetEntityClient(worldMode, deletedID);
                    if(entityC)
                    {
                        entityC->timeFromLastUpdate = R32_MAX;
                    }
                    
                    if(deletedID == myPlayer->targetIdentifier)
                    {
                        for(u32 actionIndex = 0; actionIndex < Action_Count; ++actionIndex)
                        {
                            myPlayer->targetPossibleActions[actionIndex] = false;
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
                        EssenceSlot* essence = myPlayer->essences + essenceIndex;
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
                        Assert(myPlayer->essenceCount < MAX_DIFFERENT_ESSENCES);
                        Assert(delta > 0);
                        
                        EssenceSlot* newEssence = myPlayer->essences + myPlayer->essenceCount++;
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
                            for(AnimationEffect* effect = slot->firstAnimationEffect; effect; effect = effect->next)
                            {
                                if(effect->triggerEffectTaxonomy == effectTaxonomy)
                                {
                                    AnimationEffect* newEffect;
                                    FREELIST_ALLOC(newEffect, worldMode->firstFreeEffect, PushStruct(&worldMode->entityPool, AnimationEffect, NoClear()));
                                    
                                    *newEffect = *effect;
                                    newEffect->targetID = targetID;
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
                    
                    b32* possibleActions;
                    b32 idMatch = true;
                    if(u.overlapping)
                    {
                        myPlayer->overlappingIdentifier = u.identifier;
                        possibleActions = myPlayer->overlappingPossibleActions;
                    }
                    else
                    {
                        idMatch = (myPlayer->targetIdentifier == u.identifier);
                        possibleActions = myPlayer->targetPossibleActions;
                    }
                    
                    if(idMatch)
                    {
                        for(u32 actionIndex = 0; actionIndex < Action_Count; ++actionIndex)
                        {
                            possibleActions[actionIndex] = false;
                        }
                    }
                    
                    
                    for(u32 counter = 0; counter < u.actionCount; ++counter)
                    {
                        u32 actionIndex;
                        Unpack("L", &actionIndex);
                        if(idMatch)
                        {
                            possibleActions[actionIndex] = true;
                        }
                    }
                    
                } break;
                
                case Type_AvailableRecipes:
                {
                    BookMode* mode = UI->bookModes + UIBook_Recipes;
                    
                    u32 categoryCount;
                    Unpack("L", &categoryCount);
                    for(u32 categoryIndex = 0; categoryIndex < categoryCount; ++categoryIndex)
                    {
                        b32 ignored;
                        u32 taxonomy;
                        Unpack("lL", &ignored, &taxonomy);
                        
                        if(categoryIndex == 0)
                        {
                            mode->rootTaxonomy = taxonomy;
                            mode->filterTaxonomy = taxonomy;
                        }
                        else
                        {
                            AddToRecipeCategoryBlock(UI, taxonomy);
                        }
                    }
                } break;
                
                case Type_NewRecipe:
                {
                    Recipe recipe;
                    Unpack("LQ", &recipe.taxonomy, &recipe.recipeIndex);
                    
                    AddToRecipeBlock(UI, recipe);
                } break;
                
                case Type_SkillCategories:
                {
                    BookMode* mode = UI->bookModes + UIBook_Skills;
                    
                    u32 categoryCount;
                    Unpack("L", &categoryCount);
                    for(u32 categoryIndex = 0; categoryIndex < categoryCount; ++categoryIndex)
                    {
                        SkillCategory skillCategory;
                        Unpack("lL", &skillCategory.unlocked, &skillCategory.taxonomy);
                        
                        if(categoryIndex == 0)
                        {
                            mode->rootTaxonomy = skillCategory.taxonomy;
                            mode->filterTaxonomy = skillCategory.taxonomy;
                        }
                        else
                        {
                            AddToSkillCategoryBlock(UI, skillCategory);
                        }
                    }
                } break;
                
                case Type_UnlockSkillCategoryRequest:
                {
                    u32 taxonomy;
                    Unpack("L", &taxonomy);
                    
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
                } break;
                
                case Type_SkillLevel:
                {
                    SkillSlot skill;
                    b32 isPassive;
                    b32 levelUp;
                    
                    Unpack("lLLld", &levelUp, &skill.taxonomy, &skill.level, &isPassive, &skill.power);
                    
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
                } break;
                
                case Type_StartedAction:
                {
                    u64 target;
                    u8 action;
                    Unpack("CQ", &action, &target);
                    
                    ClientEntity* targetEntity = GetEntityClient(worldMode, target);
                    if(targetEntity)
                    {
                        Vec3 relative = targetEntity->P - currentEntity->P;
                        currentEntity->animation.flipOnYAxis = (relative.x < 0);
                    }
                } break;
                
                case Type_CompletedAction:
                {
                    u64 target;
                    u8 action;
                    Unpack("CQ", &action, &target);
                    
                    if(action == Action_Attack)
                    {
                        ClientEntity* t = GetEntityClient(worldMode, target);
                        if(t)
                        {
                            t->animation.spawnAshParticlesCount = 3;
                        }
                    }
#if 0                    
                    else if(action == Action_Craft && currentEntity->identifier == myPlayer->identifier)
                    {
                        UI->player->prediction.type = Prediction_None;
                        UI->nextMode = UI->previousMode;
                    }
#endif
                    
                } break;
                
                case Type_DataFileHeader:
                {
                    DataFileArrived* dataFile = PushStruct(&worldMode->filePool, DataFileArrived);
                    FREELIST_INSERT(dataFile, worldMode->firstDataFileArrived);
                    Unpack("sLL", dataFile->name, &dataFile->fileSize, &dataFile->chunkSize);
                    
                    dataFile->data = PushSize(&worldMode->filePool, dataFile->fileSize);
                    dataFile->runningFileSize = 0;
                    
                    worldMode->currentFile = dataFile;
                } break;
                
                case Type_PakFileHeader:
                {
                    DataFileArrived* pakFile = PushStruct(&worldMode->filePool, DataFileArrived);
                    FREELIST_INSERT(pakFile, worldMode->firstPakFileArrived);
                    Unpack("sLL", pakFile->name, &pakFile->fileSize, &pakFile->chunkSize);
                    
                    pakFile->data = PushSize(&worldMode->filePool, pakFile->fileSize);
                    pakFile->runningFileSize = 0;
                    
                    worldMode->currentFile = pakFile;
                } break;
                
                case Type_FileChunk:
                {
                    DataFileArrived* file = worldMode->currentFile;
                    u8* source = packetPtr;
                    u8* dest = file->data + file->runningFileSize;
                    
                    u32 sizeToCopy = Min(file->chunkSize, file->fileSize - file->runningFileSize);
                    Copy(sizeToCopy, dest, packetPtr);
                    
                    packetPtr += sizeToCopy;
                    
                    file->runningFileSize += sizeToCopy;
                    
                    Assert(file->runningFileSize <= file->fileSize);
                } break;
                
                case Type_AllDataFileSent:
                {
                    CompletePastWritesBeforeFutureWrites;
                    worldMode->allDataFilesArrived = true;
                    Unpack("l", &worldMode->loadTaxonomies);
                } break;
                
                case Type_AllPakFileSent:
                {
                    CompletePastWritesBeforeFutureWrites;
                    worldMode->allPakFilesArrived = true;
                } break;
                
                case Type_NewEditorTab:
                {
                    b32 editable;
                    Unpack("l", &editable);
                    
                    EditorTabStack* stack = &myPlayer->editorStack;
                    stack->counter = 0;
                    stack->currentTabEditable = editable;
                    stack->previousElementType = EditorElement_Count;
                } break;
                
                case Type_EditorElement:
                {
                    TaxonomyTable* taxTable = worldMode->table;
                    TaxonomySlot* editingSlot = GetSlotForTaxonomy(taxTable, worldMode->UI->editingTaxonomy);
                    
                    EditorElement* element;
                    FREELIST_ALLOC(element, taxTable->firstFreeElement, PushStruct(&taxTable->pool, EditorElement));
                    *element = {};
                    
                    Unpack("sLL", element->name, &element->type, &element->flags);
                    
                    if(element->type < EditorElement_List)
                    {
                        Unpack("s", element->value);
                    }
                    else
                    {
                        if(element->type == EditorElement_List)
                        {
                            Unpack("s", element->elementName);
                        }
                        
                        element->value[0] = 0;
                    }
                    
                    EditorTabStack* stack = &myPlayer->editorStack;
                    EditorElement* current;
                    u32 currentStackIndex = 0;
                    
                    
                    
					if(!stack->counter)
					{
                        stack->stack[stack->counter++] = element;
                        current = element;
					}
					else
					{
                        Assert(stack->counter > 0);
                        currentStackIndex = stack->counter - 1;
                        current = stack->stack[currentStackIndex];
                    }
                    
                    
                    if(StrEqual(element->name, "empty"))
                    {
                        Assert(current->type == EditorElement_List);
                        current->emptyElement = element;
                        Assert(stack->counter < ArrayCount(stack->stack));
                        stack->stack[stack->counter++] = element;
                    }
                    else
                    {
                        switch(stack->previousElementType)
                        {
                            case EditorElement_String:
                            case EditorElement_Real:
                            case EditorElement_Signed:
                            case EditorElement_Unsigned:
                            {
                                current->next = element;
                                stack->stack[currentStackIndex] = element;
                            } break;
                            
                            case EditorElement_Struct:
                            {
                                current->firstValue = element;
                                
                                Assert(stack->counter < ArrayCount(stack->stack));
                                stack->stack[stack->counter++] = element;
                            } break;
                            
                            case EditorElement_List:
                            {
                                current->firstInList = element;
                                
                                Assert(stack->counter < ArrayCount(stack->stack));
                                stack->stack[stack->counter++] = element;
                            } break;
                            
                            case EditorElement_Count:
                            {
                                EditorTab* tab = editingSlot->tabs +editingSlot->tabCount++;
                                tab->root = current;
                                tab->editable = stack->currentTabEditable;
                            } break;
                            
                            InvalidDefaultCase;
                        }
                        
                        stack->previousElementType = element->type;
                    }
                } break;
                
                case Type_PopEditorElement:
                {
                    b32 list;
                    b32 pop;
                    Unpack("ll", &list, &pop);
                    
                    EditorTabStack* stack = &myPlayer->editorStack;
                    
                    if(pop)
                    {
                        Assert(stack->counter > 0);
                        --stack->counter;
                    }
                    
                    if(list)
                    {
                        stack->previousElementType = EditorElement_List;
                    }
                    else
                    {
                        stack->previousElementType = EditorElement_String;
                    }
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
            
            readSize += (u32) (packetPtr - oldPacketPtr);
            lastReceived = header.packetType;
        }
    }
    
}
