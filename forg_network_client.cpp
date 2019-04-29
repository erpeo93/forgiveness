internal void SendUnreliableData(void* data, u16 size)
{
    NetworkSendParams params = {};
    platformAPI.net.QueuePacket(myPlayer->network, 0, params, data, size);
}

internal void SendReliableData(void* data, u16 size)
{
    NetworkSendParams params = {};
    params.guaranteedDelivery = GuaranteedDelivery_HalfASecond;
    platformAPI.net.QueuePacket(myPlayer->network, 0, params, data, size);
}

inline void FlushAllQueuedPackets(r32 timeToAdvance)
{
    platformAPI.net.FlushSendQueue(myPlayer->network, 0, timeToAdvance);
}


inline void CloseAndSend(unsigned char* buff_, unsigned char* buff, b32 reliable)
{
    ForgNetworkApplicationData data;
    
    if(reliable)
    {
        data = myPlayer->nextSendReliableApplicationData;
        myPlayer->nextSendReliableApplicationData.index++;
    }
    else
    {
        data = myPlayer->nextSendUnreliableApplicationData;
        myPlayer->nextSendUnreliableApplicationData.index++;
    }
    
    data.flags = reliable ? ForgNetworkFlag_Ordered : 0;
    
    unsigned char* indexDest = buff_;
    ForgPackApplicationData(indexDest, data);
    
    u16 totalSize = ForgEndPacket_( buff_, buff);
    if(reliable)
    {
        SendReliableData(buff_, totalSize);
    }
    else
    {
        SendUnreliableData(buff_, totalSize);
    }
}

#define StartPacket(type) unsigned char buff_[1024]; unsigned char* buff = buff_ + sizeof(ForgNetworkApplicationData);buff = ForgPackHeader( buff, Type_##type);

#define Pack(formatString, ...) buff += pack(buff, formatString, ##__VA_ARGS__)
#define Unpack(formatString, ...) packetPtr = unpack(packetPtr, formatString, ##__VA_ARGS__)

#define CloseAndSendStandardPacket() CloseAndSend(buff_, buff, false)
#define CloseAndSendReliablePacket() CloseAndSend(buff_, buff, true)



internal void LoginRequest(i32 password)
{
    StartPacket(login);
    Pack("l", password);
    CloseAndSendReliablePacket();
}

internal void GameAccessRequest(u32 challenge)
{
    StartPacket(gameAccess);
    
    Pack("L", challenge);
    
    CloseAndSendReliablePacket();
}

#if FORGIVENESS_INTERNAL
internal void SendEditingEvent( DebugEvent* event )
{
    Assert( event->pointer );
    if( myPlayer )
    {
        StartPacket(debugEvent);
        
        Pack("QQLHCQQ", event->clock, event->pointer, event->threadID, event->coreIndex, event->type, event->overNetwork[0], event->overNetwork[1]);
        
        CloseAndSendReliablePacket();
    }
}

internal void SendInputRecordingMessage( b32 recording, b32 startAutomatically )
{
    if( myPlayer )
    {
        StartPacket(InputRecording);
        
        Pack("ll", recording, startAutomatically);
        
        CloseAndSendReliablePacket();
    }
}
#endif

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
    
    StartPacket(ActionRequest);
    
    Pack("LVLQQ", 0, acceleration, desiredAction, targetEntityID, overlappingEntityID);
    
    CloseAndSendStandardPacket();
}

internal void SendEquipRequest(u64 sourceContainerID, u8 objectIndex)
{
    StartPacket(EquipRequest);
    
    Pack("QC", sourceContainerID, objectIndex);
    
    CloseAndSendReliablePacket();
}

internal void SendDisequipRequest(u32 slotIndex, u64 destContainerID, u8 destObjectIndex)
{
    StartPacket(DisequipRequest);
    
    Pack("LQC", slotIndex, destContainerID, destObjectIndex);
    
    CloseAndSendReliablePacket();
}

internal void SendDropRequest(u64 sourceContainerID, u8 objectIndex)
{
    StartPacket(DropRequest);
    
    Pack("QC", sourceContainerID, objectIndex);
    
    CloseAndSendReliablePacket();
}

internal void SendMoveRequest(u64 sourceContainerID, u8 objectIndex, u64 destContainerID, u8 destObjectIndex)
{
    
    StartPacket(MoveRequest);
    
    Pack("QCQC", sourceContainerID, objectIndex, destContainerID, destObjectIndex);
    
    CloseAndSendReliablePacket();
}

internal void SendSwapRequest(u64 sourceContainerID, u8 sourceObjectIndex)
{
    StartPacket(SwapRequest);
    
    Pack("QC", sourceContainerID, sourceObjectIndex);
    
    CloseAndSendReliablePacket();
}

internal void SendDragEquipmentRequest(u32 slotIndex)
{
    StartPacket(DragEquipmentRequest);
    
    Pack("L", slotIndex);
    
    CloseAndSendReliablePacket();
}

internal void SendEquipDraggingRequest(u32 slotIndex)
{
    StartPacket(EquipDraggingRequest);
    
    Pack("L", slotIndex);
    
    CloseAndSendReliablePacket();
}


internal void SendCraftRequest(u32 taxonomy, GenerationData gen)
{
    StartPacket(CraftRequest);
    
    Pack("LQ", taxonomy, gen.generic);
    
    CloseAndSendReliablePacket();
}

internal void SendCraftFromInventoryRequest(u64 containerID, u32 objectIndex)
{
    StartPacket(CraftFromInventoryRequest);
    
    Pack("QL", containerID, objectIndex);
    
    CloseAndSendReliablePacket();
}


internal void SendActiveSkillRequest(u32 taxonomy)
{
    StartPacket(ActiveSkillRequest);
    
    Pack("L", taxonomy);
    
    CloseAndSendReliablePacket();
}

internal void SendPassiveSkillRequest(u32 taxonomy, b32 deactivate)
{
    StartPacket(PassiveSkillRequest);
    
    Pack("Ll", taxonomy, deactivate);
    
    CloseAndSendReliablePacket();
}

internal void SendUnlockSkillCategoryRequest(u32 taxonomy)
{
    StartPacket(UnlockSkillCategoryRequest);
    
    Pack("L", taxonomy);
    
    CloseAndSendReliablePacket();
}

internal void SendSkillLevelUpRequest(u32 taxonomy)
{
    StartPacket(SkillLevelUpRequest);
    
    Pack("L", taxonomy);
    
    CloseAndSendReliablePacket();
}

internal void SendLearnRequest(u64 containerID, u32 objectIndex)
{
    StartPacket(LearnRequest);
    
    Pack("QL", containerID, objectIndex);
    
    CloseAndSendReliablePacket();
}

internal void SendConsumeRequest(u64 containerID, u32 objectIndex)
{
    StartPacket(ConsumeRequest);
    
    Pack("QL", containerID, objectIndex);
    
    CloseAndSendReliablePacket();
}

inline void SendPopMessage(b32 list, b32 pop = true)
{
    StartPacket(PopEditorElement);
    Pack("ll", list, pop);
    CloseAndSendReliablePacket();
}


internal void SendEditorElements(EditorElement* root)
{
    StartPacket(EditorElement);
    Pack("sLLL", root->name, root->type, root->flags, root->versionNumber);
    if(root->type < EditorElement_List)
    {
        Pack("s", root->value);
    }
    else
    {
        
        if(root->type == EditorElement_List)
        {
            Pack("ss", root->elementName, root->labelName);
        }
    }
    CloseAndSendReliablePacket();
    
	switch(root->type)
	{
		case EditorElement_Struct:
		{
            if(root->firstValue)
            {
                SendEditorElements(root->firstValue);
                SendPopMessage(false);
            }
            else
            {
                SendPopMessage(false, false);
            }
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
    StartPacket(NewEditorTab);
    CloseAndSendReliablePacket();
}

inline void SendSaveAssetDefinitionFile(char* fileName, EditorElement* root)
{
    SendNewTabMessage();
    SendEditorElements(root);
    
    StartPacket(SaveAssetFadFile);
    Pack("s", fileName);
    CloseAndSendReliablePacket();
}

inline void SendReloadAssetsRequest()
{
    StartPacket(ReloadAssets);
    CloseAndSendReliablePacket();
}

inline void SendPatchServerRequest()
{
    StartPacket(PatchLocalServer);
    CloseAndSendReliablePacket();
}

inline void SendPatchCheckRequest()
{
    StartPacket(PatchCheck);
    CloseAndSendReliablePacket();
}

inline void SendSaveTabRequest(u32 taxonomy)
{
    StartPacket(SaveSlotTabToFile);
    Pack("L", taxonomy);
    CloseAndSendReliablePacket();
}

inline void SendReloadEditingMessage(u32 taxonomy, u32 tabIndex)
{
    StartPacket(ReloadEditingSlot);
    Pack("LL", taxonomy, tabIndex);
    CloseAndSendReliablePacket();
}

inline void SendAddTaxonomyRequest(u32 parentTaxonomy, char* name)
{
    StartPacket(AddTaxonomy);
    Pack("Ls", parentTaxonomy, name);
    CloseAndSendReliablePacket();
}

inline void SendDeleteTaxonomyRequest(u32 taxonomy)
{
    StartPacket(DeleteTaxonomy);
    Pack("L", taxonomy);
    CloseAndSendReliablePacket();
}

inline void SendReviveTaxonomyRequest(u32 taxonomy)
{
    StartPacket(ReviveTaxonomy);
    Pack("L", taxonomy);
    CloseAndSendReliablePacket();
}

inline void SendInstantiateRecipeRequest(u32 taxonomy, u64 recipeIndex, Vec3 offset)
{
    StartPacket(InstantiateRecipe);
    Pack("LQV", taxonomy, recipeIndex, offset);
    CloseAndSendReliablePacket();
}


inline void SendInstantiateTaxonomyRequest(u32 taxonomy, Vec3 offset)
{
    StartPacket(InstantiateTaxonomy);
    Pack("LV", taxonomy, offset);
    CloseAndSendReliablePacket();
}

inline void SendMovePlayerRequest(Vec3 offset)
{
    StartPacket(MovePlayerInOtherRegion);
    Pack("V", offset);
    CloseAndSendReliablePacket();
}

inline void SendDeleteRequest(u64 identifier)
{
    StartPacket(DeleteEntity);
    Pack("Q", identifier);
    CloseAndSendReliablePacket();
}

inline void SendImpersonateRequest(u64 identifier)
{
    StartPacket(ImpersonateEntity);
    Pack("Q", identifier);
    CloseAndSendReliablePacket();
}

inline void SendPauseToggleMessage()
{
    StartPacket(PauseToggle);
    CloseAndSendReliablePacket();
}

inline void SendRegenerateWorldChunksRequest(u32 worldSeed)
{
    StartPacket(RegenerateWorldChunks);
    Pack("L", worldSeed);
    CloseAndSendReliablePacket();
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
    element.gen = recipe.gen;
    
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

internal void DispatchApplicationPacket(GameModeWorld* worldMode, unsigned char* packetPtr, u16 dataSize)
{
    UIState* UI = worldMode->UI;
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
                
                platformAPI.net.CloseConnection(myPlayer->network, 0);
                platformAPI.net.OpenConnection(myPlayer->network, server, login.port);
                ResetReceiver(&myPlayer->receiver);
                myPlayer->nextSendUnreliableApplicationData = {};
                myPlayer->nextSendReliableApplicationData = {};
                GameAccessRequest(login.challenge);
            } break;
            
            case Type_gameAccess:
            {
                u64 openedContainerID;
                u8 serverMS5x;
                
                Unpack("LQQC", &worldMode->worldSeed, &myPlayer->identifier, &openedContainerID, &serverMS5x);
                
                r32 lastFrameFPS = 1000.0f / (serverMS5x * 5.0f);
                myPlayer->serverFPS = Lerp(myPlayer->serverFPS, 0.8f, lastFrameFPS);
                myPlayer->openedContainerID = openedContainerID;
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
                u32 oldTaxonomy = e->taxonomy;
                
                Unpack("llVLLQCLddd", &P.chunkX, &P.chunkY, &P.chunkOffset, &e->flags, &e->taxonomy, &e->gen, &e->action, &e->recipeTaxonomy, &e->lifePoints, &e->maxLifePoints, &e->status);
                
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
                Unpack("LQ", &recipe.taxonomy, &recipe.gen);
                
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
            
            case Type_PatchLocalServer:
            {
                worldMode->UI->patchingLocalServer = false;
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
        
        readSize += (u16) (packetPtr - oldPacketPtr);
        lastReceived = header.packetType;
    }
}

internal void ReceiveNetworkPackets(GameModeWorld* worldMode)
{
    NetworkPacketReceived packet;
    while(true)
    {
        packet = platformAPI.net.GetPacketOnSlot(myPlayer->network, 0);
        if(!packet.dataSize)
        {
            break;
        }
        
        unsigned char* packetPtr = packet.data;
        
        
        ForgNetworkApplicationData applicationData;
        packetPtr = ForgUnpackApplicationData(packetPtr, &applicationData);
        
        
        ForgNetworkReceiver* receiver = &myPlayer->receiver;
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
                            
                            DispatchApplicationPacket(worldMode, test->data + sizeof(ForgNetworkApplicationData), test->dataSize - sizeof(ForgNetworkApplicationData));
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
        else
        {
            if(ApplicationIndexGreater(applicationData, receiver->unorderedBiggestReceived))
            {
                receiver->unorderedBiggestReceived = applicationData;
                DispatchApplicationPacket(worldMode, packetPtr, packet.dataSize - sizeof(ForgNetworkApplicationData));
            }
        }
    }
}
