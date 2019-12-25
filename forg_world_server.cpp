internal void AddEntity_(ServerState* server, UniversePos P, AssetID definitionID, u32 seed, AddEntityParams params)
{
    NewEntity* newEntity;
    FREELIST_ALLOC(newEntity, server->firstFreeNewEntity, PushStruct(&server->gamePool, NewEntity));
    newEntity->P = P;
    newEntity->definitionID = definitionID;
    newEntity->seed = seed;
    newEntity->params = params;
    FREELIST_INSERT(newEntity, server->firstNewEntity);
}

internal void AddEntity(ServerState* server, UniversePos P, RandomSequence* seq, GameProperties* definitionProperties, AddEntityParams params = DefaultAddEntityParams())
{
    AssetID definitionID = QueryDataFiles(server->assets, EntityDefinition, "default", seq, definitionProperties);
    
    u32 seed = GetNextUInt32(seq);
    AddEntity_(server, P, definitionID, seed, params);
}

internal void AddEntity(ServerState* server, UniversePos P, RandomSequence* seq, EntityRef type, AddEntityParams params = DefaultAddEntityParams())
{
    AssetID definitionID;
    definitionID.type = AssetType_EntityDefinition;
    definitionID.subtypeHashIndex = type.subtypeHashIndex;
    definitionID.index = type.index;
    
    u32 seed = GetNextUInt32(seq);
    AddEntity_(server, P, definitionID, seed, params);
}

internal void AddEntity(ServerState* server, UniversePos P, u32 seed, EntityRef type, AddEntityParams params = DefaultAddEntityParams())
{
    AssetID definitionID;
    definitionID.type = AssetType_EntityDefinition;
    definitionID.subtypeHashIndex = type.subtypeHashIndex;
    definitionID.index = type.index;
    
    AddEntity_(server, P, definitionID, seed, params);
}

internal void SpawnPlayer(ServerState* server, UniversePos P, AddEntityParams params)
{
    EntityRef type = EntityReference(server->assets, "default", "human");
    AddEntity(server, P, &server->entropy, type, params);
    
    type = EntityReference(server->assets, "default", "passive_rune");
    AddEntityParams runeParams = DefaultAddEntityParams();
    runeParams.essences[fire] = 1;
    UniversePos runeP = Offset(P, V3(RandomBilV2(&server->entropy) * 0.5f * VOXEL_SIZE, 0));
    
    //AddEntity(server, runeP, &server->entropy, type, runeParams);
    
}

internal void SpawnZombie(ServerState* server, UniversePos P, AddEntityParams params)
{
    EntityRef type = EntityReference(server->assets, "default", "wolf");
    AddEntity(server, P, &server->entropy, type, params);
}

internal void SpawnPlayerGhost(ServerState* server, UniversePos P, AddEntityParams params)
{
    EntityRef type = EntityReference(server->assets, "default", "placeholder");
    AddEntity(server, P, &server->entropy, type, params);
}

internal void MakeIntangible(ServerState* server, EntityID ID)
{
    DefaultComponent* def = GetComponent(server, ID, DefaultComponent);
    AddEntityFlags(def, EntityFlag_notInWorld);
}

internal void MakeTangible(ServerState* server, EntityID ID)
{
    DefaultComponent* def = GetComponent(server, ID, DefaultComponent);
    ClearEntityFlags(def, EntityFlag_notInWorld);
    
    r32 maxDropDistance = 0.3f;
    def->P.chunkOffset.xy += maxDropDistance * RandomBilV2(&server->entropy);
    def->P = NormalizePosition(def->P);
    AddChangedFlags(def, &def->basicPropertiesChanged, EntityBasics_Position);
}

internal void DeleteEntity(ServerState* server, EntityID ID, DeleteEntityReasonType type)
{
    DefaultComponent* def = GetComponent(server, ID, DefaultComponent);
    Assert(!EntityHasFlags(def, EntityFlag_locked));
    AddEntityFlags(def, EntityFlag_deleted);
    DeletedEntity* deleted;
    FREELIST_ALLOC(deleted, server->firstFreeDeletedEntity, PushStruct(&server->gamePool, DeletedEntity));
    deleted->ID = ID;
    deleted->type = type;
    FREELIST_INSERT(deleted, server->firstDeletedEntity);
}

internal b32 Find(InventorySlot* slots, u32 slotCount, EntityID ID)
{
    b32 result = false;
    for(u32 slotIndex = 0; slotIndex < slotCount; ++slotIndex)
    {
        InventorySlot* slot = slots + slotIndex;
        if(AreEqual(slot, ID))
        {
            result = true;
            break;
        }
    }
    
    return result;
}

internal b32 UsingEquipOptionApplicable(UsingEquipOption* option, InventorySlot* slots, u32 slotCount, EntityID targetID)
{
    b32 result = false;
    
    if(option->slots[0] != 0xffff)
    {
        result = true;
        
        for(u32 slotIndex = 0; slotIndex < ArrayCount(option->slots); ++slotIndex)
        {
            u16 slot = option->slots[slotIndex];
            
            if(slot == 0xffff)
            {
                break;
            }
            
            Assert(slot < slotCount);
            EntityID currentID = GetBoundedID(slots + slot);
            if(!AreEqual(currentID, targetID) && IsValidID(currentID))
            {
                result = false;
                break;
            }
        }
    }
    
    return result;
}

internal void ApplyOption(UsingEquipOption* option, InventorySlot* slots, u32 slotCount, EntityID targetID)
{
    for(u16 slotIndex = 0; slotIndex < ArrayCount(option->slots); ++slotIndex)
    {
        u16 slot = option->slots[slotIndex];
        if(slot == 0xffff)
        {
            break;
        }
        
        Assert(slot < slotCount);
        SetBoundedID(slots + slot, targetID);
    }
}

internal UsingEquipOption* ValidOption(UsingEquipOption* options, u32 optionCount, InventorySlot* slots, u32 slotCount, EntityID targetID)
{
    UsingEquipOption* result = 0;
    for(u32 optionIndex = 0; optionIndex < optionCount; ++optionIndex)
    {
        UsingEquipOption* option = options + optionIndex;
        if(UsingEquipOptionApplicable(option, slots, slotCount, targetID))
        {
            result = option;
            break;
        }
    }
    
    return result;
}

internal UsingEquipOption* CanUse(ServerState* server, EntityID ID, EntityID targetID)
{
    UsingEquipOption* result = 0;
    
    UsingComponent* usingComponent = GetComponent(server, ID, UsingComponent);
    InteractionComponent* interaction = GetComponent(server, targetID, InteractionComponent);
    if(usingComponent && interaction)
    {
        result = ValidOption(interaction->usingConfigurations, interaction->usingConfigurationCount,
                             usingComponent->slots, ArrayCount(usingComponent->slots), targetID);
    }
    
    return result;
}

internal UsingEquipOption* CanEquip(ServerState* server, EntityID ID, EntityID targetID)
{
    UsingEquipOption* result = 0;
    EquipmentComponent* equipComponent = GetComponent(server, ID, EquipmentComponent);
    InteractionComponent* interaction = GetComponent(server, targetID, InteractionComponent);
    
    if(equipComponent && interaction)
    {
        result = ValidOption(interaction->equipConfigurations, interaction->equipConfigurationCount,
                             equipComponent->slots, ArrayCount(equipComponent->slots), targetID);
    }
    
    return result;
}

internal void Use_(ServerState* server, UsingEquipOption* option, EntityID ID, EntityID targetID)
{
    UsingComponent* equipped = GetComponent(server, ID, UsingComponent);
    ApplyOption(option, equipped->slots, ArrayCount(equipped->slots), targetID);
    MakeIntangible(server, targetID);
}


internal void Equip_(ServerState* server, UsingEquipOption* option, EntityID ID, EntityID targetID)
{
    EquipmentComponent* equipment = GetComponent(server, ID, EquipmentComponent);
    ApplyOption(option, equipment->slots, ArrayCount(equipment->slots), targetID);
    MakeIntangible(server, targetID);
}

internal b32 Use(ServerState* server, EntityID ID, EntityID targetID)
{
    b32 result = false;
    
    UsingEquipOption* option = CanUse(server, ID, targetID);
    if(option)
    {
        Use_(server, option, ID, targetID);
        result = true;
    }
    
    return result;
}

internal b32 Equip(ServerState* server, EntityID ID, EntityID targetID)
{
    b32 result = false;
    UsingEquipOption* option = CanEquip(server, ID, targetID);
    if(option)
    {
        Equip_(server, option, ID, targetID);
        result = true;
    }
    
    return result;
}

internal b32 UseOptionIndex(ServerState* server, EntityID ID, EntityID targetID, u16 optionIndex)
{
    b32 result = false;
    
    UsingComponent* usingComponent = GetComponent(server, ID, UsingComponent);
    InteractionComponent* interaction = GetComponent(server, targetID, InteractionComponent);
    
    if(usingComponent && interaction)
    {
        if(optionIndex < interaction->usingConfigurationCount)
        {
            UsingEquipOption* option = interaction->usingConfigurations + optionIndex;
            if(UsingEquipOptionApplicable(option, usingComponent->slots, ArrayCount(usingComponent->slots), targetID))
            {
                Use_(server, option, ID, targetID);
                result = true;
            }
        }
    }
    
    return result;
}

internal b32 EquipOptionIndex(ServerState* server, EntityID ID, EntityID targetID, u16 optionIndex)
{
    b32 result = false;
    
    EquipmentComponent* equipComponent = GetComponent(server, ID, EquipmentComponent);
    InteractionComponent* interaction = GetComponent(server, targetID, InteractionComponent);
    
    if(equipComponent && interaction)
    {
        if(optionIndex < interaction->equipConfigurationCount)
        {
            UsingEquipOption* option = interaction->equipConfigurations + optionIndex;
            if(UsingEquipOptionApplicable(option, equipComponent->slots, ArrayCount(equipComponent->slots), targetID))
            {
                Equip_(server, option, ID, targetID);
                result = true;
            }
        }
    }
    
    return result;
}

internal void EmptySlot(ServerState* server, InventorySlot* slot)
{
    MakeTangible(server, GetBoundedID(slot));
    SetBoundedID(slot, {});
}

internal b32 RemoveFromContainer(ServerState* server, ContainerComponent* container, EntityID ID)
{
    b32 result = false;
    if(container)
    {
        for(u32 objectIndex = 0; objectIndex < ArrayCount(container->storedObjects); ++objectIndex)
        {
            InventorySlot* slot = container->storedObjects + objectIndex;
            if(slot->flags_type == InventorySlot_Invalid)
            {
                break;
            }
            
            if(AreEqual(slot, ID) && !(slot->flags_type & InventorySlot_Locked))
            {
                EmptySlot(server, slot);
                result = true;
                break;
            }
        }
        
        if(!result)
        {
            for(u32 objectIndex = 0; objectIndex < ArrayCount(container->usingObjects); ++objectIndex)
            {
                InventorySlot* slot = container->usingObjects + objectIndex;
                
                if(slot->flags_type == InventorySlot_Invalid)
                {
                    break;
                }
                
                if(AreEqual(slot, ID) && !(slot->flags_type & InventorySlot_Locked))
                {
                    EmptySlot(server, slot);
                    result = true;
                    break;
                }
            }
            
        }
    }
    
    return result;
}

internal b32 RemoveFromEntity(ServerState* server, EntityID ID, EntityID targetID)
{
    b32 result = false;
    EquipmentComponent* equipment = GetComponent(server, ID, EquipmentComponent);
    if(equipment)
    {
        for(u32 slotIndex = 0; slotIndex < ArrayCount(equipment->slots); ++slotIndex)
        {
            InventorySlot* slot = equipment->slots + slotIndex;
            if(AreEqual(slot, targetID))
            {
                result = true;
                SetBoundedID(slot, {});
                break;
            }
            else
            {
                if(RemoveFromContainer(server, GetComponent(server, GetBoundedID(slot), ContainerComponent), targetID))
                {
                    result= true;
                    break;
                }
            }
        }
    }
    
    if(!result)
    {
        UsingComponent* equipped = GetComponent(server, ID, UsingComponent);
        if(equipped)
        {
            if(AreEqual(equipped->draggingID, targetID))
            {
                result = true;
                SetBoundedID(&equipped->draggingID, {});
            }
            else
            {
                for(u32 usingIndex = 0; usingIndex < ArrayCount(equipped->slots); ++usingIndex)
                {
                    InventorySlot* slot = equipped->slots + usingIndex;
                    if(AreEqual(slot, targetID))
                    {
                        result = true;
                        SetBoundedID(slot, {});
                        break;
                    }
                    else
                    {
                        if(RemoveFromContainer(server, GetComponent(server, GetBoundedID(slot), ContainerComponent), targetID))
                        {
                            result= true;
                            break;
                        }
                    }
                }
            }
        }
    }
    
    if(result)
    {
        MakeTangible(server, targetID);
    }
    
    return result;
}

internal b32 RemoveAccordingToCommand(ServerState* server, EntityID ID, GameCommand* command)
{
    b32 removed = false;
    if(AreEqual(command->containerID, ID))
    {
        if(RemoveFromEntity(server, ID, command->targetID))
        {
            removed = true;
        }
    }
    else
    {
        ContainerComponent* sourceContainer = GetComponent(server, command->containerID, ContainerComponent);
        if(RemoveFromContainer(server, sourceContainer, command->targetID))
        {
            removed = true;
        }
    }
    
    return removed;
}

internal b32 Store_(ServerState* server, InventorySlot* slots, u32 slotCount, u16 index, EntityID ID)
{
    b32 result = false;
    
    Assert(index < slotCount);
    InventorySlot* slot = slots + index;
    
    InteractionComponent* interaction = GetComponent(server, ID, InteractionComponent);
    
    if(!IsValidID(GetBoundedID(slot)) &&
       interaction && CompatibleSlot(interaction, slot))
    {
        SetBoundedID(slot, ID);
        MakeIntangible(server, ID);
        result = true;
    }
    
    return result;
    
}

internal b32 Store(ServerState* server, EntityID containerID, EntityID ID, u16 objectIndex)
{
    ContainerComponent* container = GetComponent(server, containerID, ContainerComponent);
    b32 result = Store_(server, container->storedObjects, ArrayCount(container->storedObjects), 
                        objectIndex, ID);
    return result;
}

internal b32 StoreInUsingSlots(ServerState* server, EntityID containerID, EntityID ID, u16 objectIndex)
{
    ContainerComponent* container = GetComponent(server, containerID, ContainerComponent);
    b32 result = Store_(server, container->usingObjects, ArrayCount(container->usingObjects), 
                        objectIndex, ID);
    return result;
}

internal b32 StoreInContainer(ServerState* server, EntityID containerID, EntityID ID)
{
    b32 result = false;
    
    ContainerComponent* container = GetComponent(server, containerID, ContainerComponent);
    for(u16 slotIndex = 0; slotIndex < ArrayCount(container->usingObjects); ++slotIndex)
    {
        if(StoreInUsingSlots(server, containerID, ID, slotIndex))
        {
            result = true;
            break;
        }
    }
    
    if(!result)
    {
        for(u16 slotIndex = 0; slotIndex < ArrayCount(container->storedObjects); ++slotIndex)
        {
            if(Store(server, containerID, ID, slotIndex))
            {
                result = true;
                break;
            }
        }
    }
    
    return result;
}

internal void SignalCompletedCommand(ServerState* server, EntityID ID, GameCommand* command)
{
    BrainComponent* brain = GetComponent(server, ID, BrainComponent);
    Assert(brain);
	brain->commandCompleted = true;
	if(brain->type == Brain_Player)
	{
		QueueCompletedCommand(server, ID, command);
	}
}

internal void EssenceDelta(ServerState* server, EntityID ID, u16 essence, i16 delta)
{
    DefaultComponent* def = GetComponent(server, ID, DefaultComponent);
    
    i16 newEssenceCount = (i16) def->essences[essence] + delta;
    Assert(newEssenceCount >= 0);
    def->essences[essence] = (u16) newEssenceCount;
    if(HasComponent(ID, PlayerComponent))
    {
        PlayerComponent* player = GetComponent(server, ID, PlayerComponent);
        if(player)
        {
            QueueEssenceUpdate(player, essence, def->essences[essence]);
        }
    }
}

internal void AbsorbEssences(ServerState* server, EntityID ID, EntityID targetID)
{
    DefaultComponent* source = GetComponent(server, targetID, DefaultComponent);
    for(u16 essenceIndex = 0; essenceIndex < ArrayCount(source->essences); ++essenceIndex)
    {
        u16 toAdd = source->essences[essenceIndex];
        if(toAdd)
        {
            EssenceDelta(server, ID, essenceIndex, (i16) toAdd);
        }
    }
}

internal void DispatchCommand(ServerState* server, EntityID ID, GameCommand* command, CommandParameters* parameters, r32 elapsedTime, b32 updateAction)
{
    DefaultComponent* def = GetComponent(server, ID, DefaultComponent);
    PhysicComponent* physic = GetComponent(server, ID, PhysicComponent);
    ActionComponent* action = GetComponent(server, ID, ActionComponent);
    MiscComponent* misc = GetComponent(server, ID, MiscComponent);
    EquipmentComponent* equipment = GetComponent(server, ID, EquipmentComponent);
    UsingComponent* equippedComponent = GetComponent(server, ID, UsingComponent);
    
    EntityRef equipped[Count_usingSlot];
    u32 equippedCount = 0;
    
    if(equippedComponent)
    {
        equippedCount = ArrayCount(equippedComponent->slots);
        for(u32 slotIndex = 0; slotIndex < ArrayCount(equippedComponent->slots); ++slotIndex)
        {
            EntityID slotID = GetBoundedID(equippedComponent->slots + slotIndex);
            equipped[slotIndex] = GetEntityType(server, slotID);
        }
    }
    
    
    
    EntityID targetID = command->targetID;
    DefaultComponent* targetDef = GetComponent(server, command->targetID, DefaultComponent);
    InteractionComponent* interaction = GetComponent(server, command->targetID, InteractionComponent);
    
    r32 distanceSq = R32_MAX;
    if(targetDef)
    {
        distanceSq = LengthSq(SubtractOnSameZChunk(targetDef->P, def->P));
    }
    
    u16 oldAction = GetU16(action->action);
    u16 newAction = command->action;
    
    if(updateAction)
    {
        if(newAction == action->action)
        {
            action->time += elapsedTime;
        }
        else
        {
            action->time = 0;
            SetU16(def, &action->action, newAction);
            if(physic)
            {
                physic->acc = V3(0, 0, 0);
            }
        }
        
        newAction = GetU16(action->action);
    }
    
    b32 resetAction = false;
    b32 resetActionTime = false;
    
    switch(newAction)
    {
        case none:
        case idle:
        {
        } break;
        
        case move:
        {
            if(physic)
            {
                physic->acc = parameters->acceleration;
            }
        } break;
        
        case attack:
        {
            r32 targetTime;
            if(ActionIsPossibleAtDistance(interaction, attack, oldAction, distanceSq, &targetTime, misc, equipped, equippedCount))
            {
                if(action->time >= targetTime)
                {
                    DamageEntityPhysically(server, targetID, 1);
                    resetActionTime = true;
                }
            }
            else
            {
                //resetAction = true;
            }
        } break;
        
        case cast:
        {
            if(equippedComponent)
            {
                if(SkillSlot(command->skillIndex))
                {
                    EntityID skillID = GetBoundedID(equippedComponent->slots + command->skillIndex);
                    if(IsValidID(skillID))
                    {
                        u16* essences = def->essences;
                        SkillDefComponent* active = GetComponent(server, skillID, SkillDefComponent);
                        
                        if(!active->passive)
                        {
                            u16 level = active->level;
                            // TODO(Leonardo): 
                            //SumGemEssencesBasedOnLevel?();
                            
                            UniversePos targetP = Offset(def->P, parameters->targetOffset);
                            DispatchEntityEffects(server, targetP, cast, ID, skillID, elapsedTime, essences);
                        }
                    }
                }
                else
                {
                    resetAction = true;
                }
            }
            else
            {
                resetAction = true;
            }
        } break;
        
        case mine:
        case chop:
        case harvest:
        {
            r32 targetTime;
            if(ActionIsPossibleAtDistance(interaction, newAction, oldAction, distanceSq, &targetTime, misc, equipped, equippedCount))
            {
                if(action->time >= targetTime)
                {
                    DispatchEntityEffects(server, targetDef->P, newAction, ID, targetID, elapsedTime, targetDef->essences);
                    SignalCompletedCommand(server, ID, command);
                    resetAction = true;
                }
            }
            else
            {
                resetAction = true;
            }
        } break;
        
        case pick:
        {
            if(IsValidID(targetID))
            {
                r32 targetTime;
                if(ActionIsPossibleAtDistance(interaction, pick, oldAction, distanceSq, &targetTime, misc, equipped, equippedCount))
                {
                    if(action->time >= targetTime)
                    {
                        if(!EntityHasFlags(targetDef, EntityFlag_notInWorld) && 
                           targetDef->P.chunkZ == def->P.chunkZ)
                        {
                            if(!Use(server, ID, targetID))
                            {
                                if(!Equip(server, ID, targetID))
                                {
                                    if(equipment)
                                    {
                                        for(u32 equipIndex = 0; equipIndex < ArrayCount(equipment->slots); ++equipIndex)
                                        {
                                            EntityID equipID = GetBoundedID(equipment->slots + equipIndex);
                                            if(IsValidID(equipID) && HasComponent(equipID, ContainerComponent))
                                            {
                                                if(StoreInContainer(server, equipID, targetID))
                                                {
                                                    break;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        resetAction = true;
                        SignalCompletedCommand(server, ID, command);
                    }
                }
                else
                {
                    resetAction = true;
                }
            }
        } break;
        
        case drag:
        {
            if(equipped)
            {
                r32 targetTime;
                if(ActionIsPossibleAtDistance(interaction, drag, oldAction, distanceSq, &targetTime, misc, equipped, equippedCount))
                {
                    if(action->time >= targetTime)
                    {
                        if(!IsValidID(equippedComponent->draggingID))
                        {
                            MakeIntangible(server, command->targetID);
                            SetBoundedID(&equippedComponent->draggingID, command->targetID);
                        }
                        
                        SignalCompletedCommand(server, ID, command);
                        resetAction = true;
                    }
                }
                else
                {
                    resetAction = true;
                }
            }
        } break;
        
        case setOnFire:
        {
            if(IsValidID(command->usingID))
            {
                DefaultComponent* usingDef = GetComponent(server, command->usingID, DefaultComponent);
                r32 targetTime;
                if(ActionIsPossibleAtDistance(interaction, setOnFire, oldAction, distanceSq, &targetTime, misc, equipped, equippedCount, usingDef->definitionID))
                {
                    if(action->time >= targetTime)
                    {
                        DeleteEntity(server, command->usingID);
                        SignalCompletedCommand(server, ID, command);
                        resetAction = true;
                    }
                }
                else
                {
                    resetAction = true;
                }
            }
        } break;
        
        case open:
        {
            r32 targetTime;
            if(ActionIsPossibleAtDistance(interaction, open, oldAction, distanceSq, &targetTime, misc, equipped, equippedCount))
            {
                if(action->time >= targetTime)
                {
                    b32 canOpen = true;
                    if(equipment && Find(equipment->slots, ArrayCount(equipment->slots), targetID))
                    {
                        canOpen = false;
                    }
                    
                    if(equipped && Find(equippedComponent->slots, ArrayCount(equippedComponent->slots), targetID))
                    {
                        canOpen = false;
                    }
                    
                    if(canOpen)
                    {
                        ContainerComponent* container = GetComponent(server, targetID, ContainerComponent);
                        if(container)
                        {
                            if(!IsValidID(container->openedBy))
                            {
                                SetBoundedID(&container->openedBy, ID);
                                SignalCompletedCommand(server, ID, command);
                            }
                        }
                    }
                    
                    resetAction = true;
                }
            }
            else
            {
                resetAction = true;
            }
        } break;
        
        case use:
        {
            if(RemoveAccordingToCommand(server, ID, command))
            {
                UseOptionIndex(server, ID, command->targetID, command->optionIndex);
            }
        } break;
        
        case equip:
        {
            if(RemoveAccordingToCommand(server, ID, command))
            {
                EquipOptionIndex(server, ID, command->targetID, command->optionIndex);
            }
        } break;
        
        case disequip:
        {
            RemoveFromEntity(server, ID, command->targetID);
        } break;
        
        case drop:
        {
            RemoveAccordingToCommand(server, ID, command);
        } break;
        
        case storeInventory:
        {
            if(RemoveAccordingToCommand(server, ID, command))
            {
                Store(server, command->targetContainerID, targetID, command->targetObjectIndex);
            }
        } break;
        
        case useInventory:
        {
            if(RemoveAccordingToCommand(server, ID, command))
            {
                if(AreEqual(command->targetContainerID, ID))
                {
                    UseOptionIndex(server, ID, targetID, command->optionIndex);
                }
                else
                {
                    StoreInUsingSlots(server, command->targetContainerID, targetID, command->targetObjectIndex);
                }
            }
        } break;
        
        case absorb:
        {
            if(RemoveAccordingToCommand(server, ID, command))
            {
                AbsorbEssences(server, ID, targetID);
                DeleteEntity(server, targetID);
            }
            else
            {
                r32 targetTime;
                if(ActionIsPossibleAtDistance(interaction, absorb, oldAction, distanceSq, &targetTime, misc, equipped, equippedCount))
                {
                    if(action->time >= targetTime)
                    {
                        AbsorbEssences(server, ID, targetID);
                        DeleteEntity(server, targetID);
                        resetAction = true;
                    }
                }
                else
                {
                    resetAction = true;
                }
            }
        } break;
        
        case craft:
        {
            if(targetDef)
            {
                if(AreEqual(targetDef->definitionID, EntityReference(server->assets, "default", "recipe")))
                {
                    EntityRef type = GetCraftingType(server->assets, targetDef->seed);
                    u32 craftSeed = targetDef->seed;
                    
                    
                    EntityRef components[8];
                    b32 deleteComponent[8];
                    EntityID componentIDs[8];
                    u16 componentCount = GetCraftingComponents(server->assets, type, craftSeed, components, deleteComponent, ArrayCount(components));
                    
                    b32 hasAllComponents = true;
                    for(u16 componentIndex = 0; componentIndex < componentCount; ++componentIndex)
                    {
                        if(EntityHasType(server, ID, components[componentIndex], componentIDs + componentIndex))
                        {
                            
                        }
                        else
                        {
                            hasAllComponents = false;
                            break;
                        }
                    }
                    
                    b32 hasAllEssences = true;
                    u16 essenceCount = GetCraftingEssenceCount(server->assets, type, craftSeed);
                    for(u16 essenceIndex = 0; essenceIndex < essenceCount; ++essenceIndex)
                    {
                        u16 selected = action->selectedCrafingEssences[essenceIndex];
                        if(selected >= ArrayCount(def->essences) || def->essences[selected] == 0)
                        {
                            hasAllEssences = false;
                            break;
                        }
                    }
                    
                    
                    if(hasAllComponents && hasAllEssences)
                    {
                        if(RemoveAccordingToCommand(server, ID, command))
                        {
                            DeleteEntity(server, command->targetID);
                            AddEntityParams params = DefaultAddEntityParams();
                            
                            
                            for(u16 essenceIndex = 0; essenceIndex < essenceCount; ++essenceIndex)
                            {
                                u16 selected = action->selectedCrafingEssences[essenceIndex];
                                ++params.essences[selected];
                                EssenceDelta(server, ID, selected, (i16) -1);
                                
                                action->selectedCrafingEssences[essenceIndex] = 0;
                            }
                            
                            for(u16 slotIndex = 0; slotIndex < ArrayCount(action->selectedCrafingEssences); ++slotIndex)
                            {
                                action->selectedCrafingEssences[slotIndex] = 0;
                            }
                            
                            AddEntity(server, def->P, craftSeed, type, params);
                            for(u16 componentIndex = 0; componentIndex < componentCount; ++componentIndex)
                            {
                                if(deleteComponent[componentIndex])
                                {
                                    if(RemoveFromEntity(server, ID, componentIDs[componentIndex]))
                                    {
                                        DeleteEntity(server, componentIDs[componentIndex]);
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
            }
        } break;
        
        case level_up:
        {
            if(HasComponent(ID, PlayerComponent))
            {
                PlayerComponent* player = GetComponent(server, ID, PlayerComponent);
                if(equipped && player && player->skillPoints > 0)
                {
                    for(u32 slotIndex = 0; slotIndex < ArrayCount(equippedComponent->slots); ++slotIndex)
                    {
                        InventorySlot* slot = equippedComponent->slots + slotIndex;
                        if(AreEqual(GetBoundedID(slot), command->targetID))
                        {
                            SkillDefComponent* skill = GetComponent(server, command->targetID, SkillDefComponent);
                            ++skill->level;
                            --player->skillPoints;
                            
                            slot->flags_type |= InventorySlot_Locked;
                            SetBoundedID(slot, command->targetID);
                            break;
                        }
                    }
                }
            }
        } break;
        InvalidDefaultCase;
    }
    
    if(resetAction)
    {
        action->time = 0;
        SetU16(def, &action->action, idle);
    }
    
    if(resetActionTime)
    {
        action->time = 0;
    }
}

STANDARD_ECS_JOB_SERVER(HandleOpenedContainers)
{
    ContainerComponent* container = GetComponent(server, ID, ContainerComponent);
    if(IsValidID(container->openedBy))
    {
        InteractionComponent* interaction = GetComponent(server, ID, InteractionComponent);
        
        DefaultComponent* opened = GetComponent(server, ID, DefaultComponent);
        DefaultComponent* opener = GetComponent(server, GetBoundedID(container->openedBy), DefaultComponent);
        r32 distanceSq = LengthSq(SubtractOnSameZChunk(opened->P, opener->P));
        r32 targetTime;
        
        MiscComponent* misc = 0;
        if(!ActionIsPossibleAtDistance(interaction, open, open, distanceSq, &targetTime, misc, 0, 0))
        {
            SetBoundedID(&container->openedBy, {});
        }
    }
}

internal r32 GetLightRadiousSq(ServerState* server, EntityID ID)
{
    r32 result = 0;
    
    if(HasComponent(ID, MiscComponent))
    {
        MiscComponent* misc = GetComponent(server, ID, MiscComponent);
        result = Square(GetR32(misc->lightRadious));
    }
    return result;
}

STANDARD_ECS_JOB_SERVER(FillPlayerSpacePartition)
{
    DefaultComponent* def = GetComponent(server, ID, DefaultComponent);
    PlayerComponent* player = GetComponent(server, ID, PlayerComponent);
    if(player)
    {
        PhysicComponent* physic = GetComponent(server, ID, PhysicComponent);
        Rect3 bounds = AddRadius(physic->bounds, UPDATE_DISTANCE * V3(1, 1, 1));
        AddToSpatialPartition(server->frameByFramePool, &server->playerPartition, def->P, bounds, ID);
    }
}

global_variable r32 g_maxDelta = 1.0f;
STANDARD_ECS_JOB_SERVER(FillCollisionSpatialPartition)
{
    DefaultComponent* def = GetComponent(server, ID, DefaultComponent);
    PhysicComponent* physic = GetComponent(server, ID, PhysicComponent);
    
    if(!EntityHasFlags(def, EntityFlag_notInWorld))
    {
        Rect3 bounds = AddRadius(physic->bounds, V3(g_maxDelta, g_maxDelta, g_maxDelta));
        AddToSpatialPartition(server->frameByFramePool, &server->standardPartition, def->P, bounds, ID);
    }
}

internal b32 ShouldCollide(u16 b1, u16 b2)
{
    b32 result = false;
    
    if(b2 < b1)
    {
        u16 temp = b1;
        b1 = b2;
        b2 = temp;
    }
    
    switch(b1)
    {
        case bound_invalid:
        {
            
        } break;
        
        case bound_creature:
        {
            result = (b2 == bound_environment);
        } break;
        
        case bound_environment:
        {
            
        } break;
        
        case bound_object:
        {
            
        } break;
        
        InvalidDefaultCase;
    }
    
    return result;
}

internal b32 ShouldOverlap(u16 b1, u16 b2)
{
    b32 result = false;
    
    if(b2 < b1)
    {
        u16 temp = b1;
        b1 = b2;
        b2 = temp;
    }
    
    switch(b1)
    {
        case bound_invalid:
        {
            
        } break;
        
        case bound_creature:
        {
        } break;
        
        case bound_environment:
        {
            
        } break;
        
        case bound_object:
        {
            
        } break;
        
        InvalidDefaultCase;
    }
    
    return result;
}

internal void HandleEntityMovement(ServerState* server, DefaultComponent* def, PhysicComponent* physic, EntityID ID, r32 elapsedTime)
{
    UniversePos oldP = def->P;
    Vec3 acceleration = Normalize(physic->acc) *physic->accelerationCoeff;
    Vec3 velocity = physic->speed;
    r32 dt = elapsedTime;
    acceleration.xy += physic->drag * velocity.xy;
    Vec3 deltaP = 0.5f * acceleration * Square(dt) + velocity * dt;
    
#if FORGIVENESS_INTERNAL
    deltaP.x = Clamp(-g_maxDelta, deltaP.x, g_maxDelta);
    deltaP.y = Clamp(-g_maxDelta, deltaP.y, g_maxDelta);
    deltaP.z = Clamp(-g_maxDelta, deltaP.z, g_maxDelta);
#else
    Assert(Abs(deltaP.x) <= g_maxDelta);
    Assert(Abs(deltaP.y) <= g_maxDelta);
    Assert(Abs(deltaP.z) <= g_maxDelta);
#endif
    
    physic->speed += acceleration * dt;
    r32 tRemaining = 1.0f;
    for( u32 iteration = 0; (iteration < 2) && tRemaining > 0; iteration++)
    {
        Vec3 wallNormalMin = {};
        r32 tStop = tRemaining;
        
        Rect3 bounds = AddRadius(physic->bounds, deltaP);
        SpatialPartitionQuery collisionQuery = QuerySpatialPartition(&server->standardPartition, def->P, bounds);
        
        EntityID collisionTriggerID = {};
        UniversePos collisionP = {};
        
        for(EntityID testID = GetCurrent(&collisionQuery); IsValid(&collisionQuery); testID = Advance(&collisionQuery))
        {
            DefaultComponent* testDef = GetComponent(server, testID, DefaultComponent);
            PhysicComponent* testPhysic = GetComponent(server, testID, PhysicComponent);
            if(testDef->P.chunkZ == def->P.chunkZ)
            {
                Vec3 testP = SubtractOnSameZChunk(testDef->P, def->P);
                
                b32 shouldCollide = ShouldCollide(physic->boundType, testPhysic->boundType);
                b32 shouldOverlap = ShouldOverlap(physic->boundType, testPhysic->boundType);
                
                r32 oldT = tStop;
                if(HandleVolumeCollision(physic->bounds, deltaP, testP, testPhysic->bounds, &tStop, &wallNormalMin, shouldCollide))
                {
                    if(shouldOverlap)
                    {
                        DispatchOverlappingEffects(server, testDef->P, ID, testID);
                    }
                }
                
                
                if(tStop < oldT)
                {
                    collisionP = testDef->P;
                    collisionTriggerID = testID;
                }
            }
        }
        
        if(IsValidID(collisionTriggerID))
        {
            DispatchCollisitonEffects(server, collisionP, ID, collisionTriggerID);
            DispatchCollisitonEffects(server, collisionP, collisionTriggerID, ID);
        }
        
        Vec3 wallNormal = wallNormalMin;
        def->P.chunkOffset += tStop * deltaP;
        
        physic->speed -= Dot(physic->speed, wallNormal) * wallNormal;
        deltaP -= Dot(deltaP, wallNormal) * wallNormal;
        tRemaining -= tStop;
    }
    
    def->P = NormalizePosition(def->P);
    if(!PositionInsideWorld(&def->P))
    {
        physic->speed = {};
        def->P = oldP;
        AddChangedFlags(def, &def->basicPropertiesChanged, EntityBasics_Position);
    }
    
    if(!EntityHasFlags(def, EntityFlag_canGoIntoWater))
    {
        WorldTile* tile = GetTile(server, def->P);
        if(IsValid(tile->underSeaLevelFluid))
        {
            physic->speed = {};
            def->P = oldP;
            AddChangedFlags(def, &def->basicPropertiesChanged, EntityBasics_Position);
        }
    }
    
    def->updateSent = false;
    if(LengthSq(SubtractOnSameZChunk(def->P, oldP)) > 0)
    {
        AddChangedFlags(def, &def->basicPropertiesChanged, EntityBasics_Position);
    }
    
    if(physic->speed != velocity)
    {
        AddChangedFlags(def, &def->basicPropertiesChanged, EntityBasics_Velocity);
    }
}

internal void UpdateObjectPosition(ServerState* server, BoundedEntityID* ID, UniversePos P)
{
    DefaultComponent* equipmentDef = GetComponent(server, GetBoundedID(*ID), DefaultComponent);
    if(EntityHasFlags(equipmentDef, EntityFlag_deleted))
    {
        SetBoundedID(ID, {});
    }
    else
    {
        equipmentDef->P = P;
        AddEntityFlags(equipmentDef, EntityFlag_notInWorld);
    }
}
internal void UpdateObjectPositions(ServerState* server, UniversePos P, InventorySlot* slots, u32 slotCount)
{
    for(u16 slotIndex = 0; slotIndex < slotCount; ++slotIndex)
    {
        InventorySlot* slot = slots + slotIndex;
        if(IsValidID(GetBoundedID(slot)))
        {
            UpdateObjectPosition(server, &slot->ID_, P);
        }
    }
}

STANDARD_ECS_JOB_SERVER(UpdateEntity)
{
    DefaultComponent* def = GetComponent(server, ID, DefaultComponent);
    PhysicComponent* physic = GetComponent(server, ID, PhysicComponent);
    HandleEntityMovement(server, def, physic, ID, elapsedTime);
    
    UsingComponent* equipped = GetComponent(server, ID, UsingComponent);
    if(equipped)
    {
        if(IsValidID(equipped->draggingID))
        {
            UpdateObjectPosition(server, &equipped->draggingID, def->P);
        }
        UpdateObjectPositions(server, def->P, equipped->slots, ArrayCount(equipped->slots));
        for(u32 usingIndex = 0; usingIndex < ArrayCount(equipped->slots); ++usingIndex)
        {
            InventorySlot* slot = equipped->slots + usingIndex;
            EntityID slotID = GetBoundedID(slot);
            if(IsValidID(slotID))
            {
                ContainerComponent* container = GetComponent(server, slotID, ContainerComponent);
                if(container)
                {
                    UpdateObjectPositions(server, def->P, container->storedObjects, ArrayCount(container->storedObjects));
                    UpdateObjectPositions(server, def->P, container->usingObjects, ArrayCount(container->usingObjects));
                }
            }
        }
    }
    
    EquipmentComponent* equipment = GetComponent(server, ID, EquipmentComponent);
    if(equipment)
    {
        UpdateObjectPositions(server, def->P, equipment->slots, ArrayCount(equipment->slots));
        for(u32 equipIndex = 0; equipIndex < ArrayCount(equipment->slots); ++equipIndex)
        {
            InventorySlot* slot = equipment->slots + equipIndex;
            EntityID slotID = GetBoundedID(slot);
            if(IsValidID(slotID))
            {
                ContainerComponent* container = GetComponent(server, slotID, ContainerComponent);
                if(container)
                {
                    UpdateObjectPositions(server, def->P, container->storedObjects, ArrayCount(container->storedObjects));
                    UpdateObjectPositions(server, def->P, container->usingObjects, ArrayCount(container->usingObjects));
                }
            }
        }
    }
    
    AliveComponent* alive = GetComponent(server, ID, AliveComponent);
    if(alive)
    {
        if(GetU32(alive->physicalHealth) <= 0 || GetU32(alive->mentalHealth) <= 0)
        {
            DeleteEntity(server, ID);
        }
    }
}

STANDARD_ECS_JOB_SERVER(UpdateTempEntity)
{
    TempEntityComponent* temp = GetComponent(server, ID, TempEntityComponent);
    temp->time += elapsedTime;
    if(temp->time >= temp->targetTime)
    {
        DeleteEntity(server, ID);
    }
}

STANDARD_ECS_JOB_SERVER(DeleteAllEntities)
{
    FreeArchetype(server, ID);
}

STANDARD_ECS_JOB_SERVER(DamageEntityFearingLight)
{
    DefaultComponent* def = GetComponent(server, ID, DefaultComponent);
    if(EntityHasFlags(def, EntityFlag_fearsLight))
    {
        ZLayer* layer = server->layers + def->P.chunkZ;
        if(layer->dayTimePhase == DayTime_Day)
        {
            u32 damage = 1;
            DamageEntityPhysically(server, ID, damage);
        }
    }
}

STANDARD_ECS_JOB_SERVER(DealLightDamage)
{
    r32 lightRadiousSq = GetLightRadiousSq(server, ID);
    if(lightRadiousSq > 0)
    {
        DefaultComponent* def = GetComponent(server, ID, DefaultComponent);
        SpatialPartitionQuery query = QuerySpatialPartitionAtPoint(&server->standardPartition, def->P);
        
        for(EntityID testID = GetCurrent(&query); IsValid(&query); testID = Advance(&query))
        {
            DefaultComponent* testDef = GetComponent(server, testID, DefaultComponent);
            if(EntityHasFlags(testDef, EntityFlag_fearsLight))
            {
                Vec3 toTarget = SubtractOnSameZChunk(testDef->P, def->P);
                if(LengthSq(toTarget) <= 0.8f * lightRadiousSq)
                {
                    DamageEntityPhysically(server, testID, 1);
                }
            }
        }
        
    }
}

internal void UpdateWorldBasics(ServerState* server, r32 elapsedTime)
{
    for(i16 chunkZ = 0; chunkZ < server->maxDeepness; ++chunkZ)
    {
        ZLayer* layer = server->layers + chunkZ;
        layer->dayTimeTime += elapsedTime;
        if(layer->dayTimeTime >= DAYPHASE_DURATION)
        {
            layer->dayTimeTime = 0;
            if(++layer->dayTimePhase == Count_DayTime)
            {
                layer->dayTimePhase = 0;
            }
            
            for(CompIterator iter = FirstComponent(server, PlayerComponent); 
                IsValid(iter); iter = Next(iter))
            {
                PlayerComponent* player = GetComponentRaw(server, iter, PlayerComponent);
                if(player->connectionSlot && IsValidID(player->ID))
                {
                    DefaultComponent* def = GetComponent(server, player->ID, DefaultComponent);
                    if(def->P.chunkZ == chunkZ)
                    {
                        QueueDayTime(player, layer->dayTimePhase);
                    }
                }
            }
        }
        
        if(layer->dayTimePhase == DayTime_Night)
        {
            for(CompIterator iter = FirstComponent(server, PlayerComponent); 
                IsValid(iter); iter = Next(iter))
            {
                PlayerComponent* player = GetComponentRaw(server, iter, PlayerComponent);
                if(player->connectionSlot && IsValidID(player->ID))
                {
                    DefaultComponent* playerDefault = GetComponent(server, player->ID, DefaultComponent);
                    PhysicComponent* playerPhysic = GetComponent(server, player->ID, PhysicComponent);
                    
                    if(playerDefault->P.chunkZ == chunkZ)
                    {
                        player->timeSinceLastZombieSpawned += elapsedTime;
                        r32 targetZombieTime = 1.0f;
                        u32 stepCount = 30;
                        if(player->timeSinceLastZombieSpawned >= targetZombieTime)
                        {
                            UniversePos P = FindWalkablePStartingFrom(server, playerDefault->P, stepCount);
                            SpawnZombie(server, P, DefaultAddEntityParams());
                            player->timeSinceLastZombieSpawned = 0;
                        }
                    }
                }
            }
        }
    }
}