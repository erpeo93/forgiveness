internal Rect3 StandardBounds(Vec3 dim, Vec3 offset)
{
    offset = {};
    Rect3 result = Offset(RectCenterDim(V3(0, 0, 0), dim), offset + V3(0, 0, 0.5f * dim.z));
    return result;
}

internal GameProperty GetProperty(u32 seed)
{
    GameProperty result;
    result.property = Property_essence;
    
    RandomSequence seq = Seed(seed);
    u32 choice = RandomChoice(&seq, earth + 1);
    result.value = SafeTruncateToU16(choice);
    
    return result;
}

#define PropertyToU16(property, enum) ExistMetaPropertyValue(Property_##property, Tokenize((enum).value))
internal u16 ExistMetaPropertyValue(u16 propertyType, Token value);;
internal void AddPossibleActions(PossibleActionList* list, Enumerator* actions, u32 actionCount)
{
    for(u32 actionIndex = 0; actionIndex < actionCount; ++actionIndex)
    {
        if(list->actionCount < ArrayCount(list->actions))
        {
            list->actions[list->actionCount++] = PropertyToU16(action, actions[actionIndex]);
        }
    }
}

#ifdef FORG_SERVER
INIT_COMPONENT_FUNCTION(InitPhysicComponent)
{
    PhysicComponent* physic = (PhysicComponent*) componentPtr;
    
    physic->P = s->P;
    physic->bounds = StandardBounds(common->boundDim, common->boundOffset);
    physic->definitionID = common->definitionID;
    physic->seed = s->seed;
    physic->speed = {};
    physic->acc = {};
    physic->action = GameProp(action, idle);
}

internal void AddRandomEffects(EffectComponent* effects, EffectBinding* bindings, ArrayCounter bindingCount, GameProperty property)
{
    for(ArrayCounter bindingIndex = 0; bindingIndex < bindingCount; ++bindingIndex)
    {
        EffectBinding* binding = bindings + bindingIndex;
        if(AreEqual(binding->property, property))
        {
            if(effects->effectCount < ArrayCount(effects->effects))
            {
                GameEffect* dest = effects->effects + effects->effectCount++;
                *dest = binding->effect;
            }
            break;
        }
    }
}

INIT_COMPONENT_FUNCTION(InitEffectComponent)
{
    ServerState* server = (ServerState*) state;
    EffectComponent* effect = (EffectComponent*) componentPtr;
    GameProperty property = GetProperty(s->seed);
    AddRandomEffects(effect, s->bindings, s->bindingCount, property);
}

INIT_COMPONENT_FUNCTION(InitCollisionEffectsComponent)
{
    CollisionEffectsComponent* collision = (CollisionEffectsComponent*) componentPtr;
    for(u32 effectIndex = 0; effectIndex < s->collisionEffectsCount; ++effectIndex)
    {
        if(collision->effectCount < ArrayCount(collision->effects))
        {
            GameEffect* dest = collision->effects + collision->effectCount++;
            *dest = s->collisionEffects[effectIndex];
        }
    }
}

INIT_COMPONENT_FUNCTION(InitPlayerComponent)
{
    
}

INIT_COMPONENT_FUNCTION(InitEquipmentComponent)
{
    
}

INIT_COMPONENT_FUNCTION(InitUsingComponent)
{
    
}

INIT_COMPONENT_FUNCTION(InitContainerComponent)
{
    ServerState* server = (ServerState*) state;
    ContainerComponent* dest = (ContainerComponent*) componentPtr;
    
    u16 totalStoreCount = s->storeCount + s->specialStoreCount;
    Assert(totalStoreCount <= ArrayCount(dest->storedObjects));
    for(u16 index = 0; index < totalStoreCount; ++index)
    {
        InventorySlot* slot = dest->storedObjects + index;
        slot->type = (index < s->storeCount) ? (u16) InventorySlot_Standard : (u16) InventorySlot_Special;
        slot->ID = {};
    }
    
    u16 totalUsingCount = s->usingCount + s->specialUsingCount;
    Assert(totalUsingCount <= ArrayCount(dest->usingObjects));
    for(u16 index = 0; index < totalUsingCount; ++index)
    {
        InventorySlot* slot = dest->usingObjects + index;
        slot->type = index < s->usingCount ? (u16) InventorySlot_Standard : (u16) InventorySlot_Special;
        slot->ID = {};
    }
    
}

internal EntityRef EntityReference(Assets* assets, char* kind, char* type);
INIT_COMPONENT_FUNCTION(InitSkillComponent)
{
    ServerState* server = (ServerState*) state;
    
    SkillComponent* skills = (SkillComponent*) componentPtr;
    
    for(u32 skillIndex = 0; skillIndex < ArrayCount(skills->activeSkills); ++skillIndex)
    {
        skills->activeSkills[skillIndex].effect.effectType = GameProp(gameEffect, moveOnZSlice);
    }
    
    skills->activeSkills[0].effect.spawnType = EntityReference(server->assets, "default", "wolf");
    skills->activeSkills[1].effect.spawnType = EntityReference(server->assets, "default", "crocodile");
    skills->activeSkills[2].effect.spawnType = EntityReference(server->assets, "default", "turtle");
    skills->activeSkills[3].effect.spawnType = EntityReference(server->assets, "default", "human");
    skills->activeSkills[4].effect.spawnType = EntityReference(server->assets, "default", "sword");
    skills->activeSkills[5].effect.spawnType = EntityReference(server->assets, "default", "bag");
}
#else
INIT_COMPONENT_FUNCTION(InitBaseComponent)
{
    BaseComponent* base = (BaseComponent*) componentPtr;
    base->seed = c->seed;
    base->bounds = StandardBounds(common->boundDim, common->boundOffset);
    base->nameHash = StringHash(c->name.name);
    base->serverID = c->ID;
    base->definitionID = common->definitionID;
}

internal void InitShadow(ShadowComponent* shadow, ClientEntityInitParams* params)
{
    shadow->offset = params->shadowOffset;
    shadow->scale = params->shadowScale;
    shadow->color = params->shadowColor;
}
#define InitImageReference(assets, dest, source, reference) InitImageReference_(assets, dest->reference, source->reference##Properties)
internal void InitImageReference_(Assets* assets, ImageReference* dest,ImageProperties* sourceProperties)
{
    dest->typeHash = sourceProperties->imageType.subtypeHash;
    
    dest->properties = {};
    for(u16 propertyIndex = 0; propertyIndex < sourceProperties->propertyCount; ++propertyIndex)
    {
        u32 flags = 0;
        ImageProperty* source = sourceProperties->properties + propertyIndex;
        if(source->optional)
        {
            flags |= GameProperty_Optional;
        }
        AddGameProperty_(&dest->properties, source->property, flags);
    }
}

INIT_COMPONENT_FUNCTION(InitAnimationComponent)
{
    AnimationComponent* animation = (AnimationComponent*) componentPtr;
    animation->skeletonHash =c->skeleton.subtypeHash;
    animation->skinHash =c->skin.subtypeHash;
    animation->flipOnYAxis = 0;
    animation->scale = 1.0f;
    InitShadow(&animation->shadow, c);
}

INIT_COMPONENT_FUNCTION(InitRockComponent)
{
    
}

INIT_COMPONENT_FUNCTION(InitGrassComponent)
{
    
}

INIT_COMPONENT_FUNCTION(InitPlantComponent)
{
    GameModeWorld* worldMode = (GameModeWorld*) state;
    Assets* assets = worldMode->gameState->assets;
    PlantComponent* dest = (PlantComponent*) componentPtr;
    InitImageReference(assets, &dest, &c, leaf);
}

INIT_COMPONENT_FUNCTION(InitStandardImageComponent)
{
    GameModeWorld* worldMode = (GameModeWorld*) state;
    Assets* assets = worldMode->gameState->assets;
    StandardImageComponent* dest = (StandardImageComponent*) componentPtr;
    InitImageReference(assets, &dest, &c, entity);
}

internal void InitLayout(Assets* assets, LayoutPiece* destPieces, u32* destPieceCount, u32 maxDestPieceCount, LayoutPieceProperties* pieces, u32 pieceCount, GameProperty property)
{
    for(u32 pieceIndex = 0; pieceIndex < pieceCount; ++pieceIndex)
    {
        LayoutPieceProperties* piece = pieces + pieceIndex;
        if(*destPieceCount < maxDestPieceCount)
        {
            LayoutPiece* destPiece = destPieces + (*destPieceCount)++;
            InitImageReference_(assets, &destPiece->image, &piece->properties);
            destPiece->nameHash = StringHash(piece->name.name);
            destPiece->height = piece->height;
            destPiece->inventorySlotType = PropertyToU16(inventorySlotType, piece->inventorySlotType);
            if(destPiece->inventorySlotType == 0xffff)
            {
                destPiece->inventorySlotType = InventorySlot_Standard;
            }
        }
    }
    
    for(u32 pieceIndex = 0; pieceIndex < *destPieceCount; ++pieceIndex)
    {
        LayoutPiece* destPiece = destPieces + pieceIndex;
        AddGameProperty_(&destPiece->image.properties, property, GameProperty_Optional);
    }
}

INIT_COMPONENT_FUNCTION(InitLayoutComponent)
{
    GameModeWorld* worldMode = (GameModeWorld*) state;
    Assets* assets = worldMode->gameState->assets;
    
    LayoutComponent* dest = (LayoutComponent*) componentPtr;
    InitShadow(&dest->shadow, c);
    dest->rootHash = StringHash(c->layoutRootName.name);
    dest->rootScale = V2(1, 1);
    dest->rootAngle = 0;
    GameProperty property = GetProperty(c->seed);
    
    InitLayout(assets, dest->pieces, &dest->pieceCount, ArrayCount(dest->pieces), c->layoutPieces, c->pieceCount, property);
    InitLayout(assets, dest->openPieces, &dest->openPieceCount, ArrayCount(dest->openPieces), c->openLayoutPieces, c->openPieceCount, property);
    InitLayout(assets, dest->usingPieces, &dest->usingPieceCount, ArrayCount(dest->usingPieces), c->usingLayoutPieces, c->usingPieceCount, property);
}

INIT_COMPONENT_FUNCTION(InitEquipmentMappingComponent)
{
    
}

INIT_COMPONENT_FUNCTION(InitUsingMappingComponent)
{
    
}

INIT_COMPONENT_FUNCTION(InitAnimationMappingComponent)
{
    
}

INIT_COMPONENT_FUNCTION(InitAnimationEffectsComponent)
{
    
}

INIT_COMPONENT_FUNCTION(InitContainerMappingComponent)
{
    ContainerMappingComponent* dest = (ContainerMappingComponent*) componentPtr;
    dest->zoomCoeff = c->lootingZoomCoeff;
    dest->desiredOpenedDim = c->desiredOpenedDim;
    dest->desiredUsingDim = c->desiredUsingDim;
}
#endif
INIT_COMPONENT_FUNCTION(InitInteractionComponent)
{
    InteractionComponent* dest = (InteractionComponent*) componentPtr;
    AddPossibleActions(dest->actions + Interaction_Ground, common->groundActions, common->groundActionCount);
    AddPossibleActions(dest->actions + Interaction_Equipment, common->equipmentActions, common->equipmentActionCount);
    AddPossibleActions(dest->actions + Interaction_Container, common->containerActions, common->containerActionCount);
    AddPossibleActions(dest->actions + Interaction_Equipped, common->equippedActions, common->equippedActionCount);
    
    for(u32 usingOption = 0; usingOption < common->usingConfigurationCount; ++usingOption)
    {
        UseLayout* source = common->usingConfigurations + usingOption;
        if(dest->usingConfigurationCount < ArrayCount(dest->usingConfigurations))
        {
            UsingEquipOption* destOption = dest->usingConfigurations + dest->usingConfigurationCount++;
            
            for(u32 slotIndex = 0; slotIndex < ArrayCount(destOption->slots); ++slotIndex)
            {
                destOption->slots[slotIndex] = 0xffff;
            }
            
            for(u32 slotIndex = 0; slotIndex < source->slotCount; ++slotIndex)
            {
                if(slotIndex < ArrayCount(destOption->slots))
                {
                    destOption->slots[slotIndex] = PropertyToU16(usingSlot, source->slots[slotIndex]);
                }
            }
            
        }
    }
    
    for(u32 equipOption = 0; equipOption < common->equipConfigurationCount; ++equipOption)
    {
        EquipLayout* source = common->equipConfigurations + equipOption;
        if(dest->equipConfigurationCount < ArrayCount(dest->equipConfigurations))
        {
            UsingEquipOption* destOption = dest->equipConfigurations + dest->equipConfigurationCount++;
            
            for(u32 slotIndex = 0; slotIndex < ArrayCount(destOption->slots); ++slotIndex)
            {
                destOption->slots[slotIndex] = 0xffff;
            }
            
            for(u32 slotIndex = 0; slotIndex < source->slotCount; ++slotIndex)
            {
                if(slotIndex < ArrayCount(destOption->slots))
                {
                    destOption->slots[slotIndex] = PropertyToU16(equipmentSlot, source->slots[slotIndex]);
                }
            }
            
        }
    }
    
    dest->inventorySlotType = PropertyToU16(inventorySlotType, common->inventorySlotType);
}

#ifdef FORG_SERVER
internal void InitEntity(ServerState* server, EntityID ID, 
                         CommonEntityInitParams* common, 
                         ServerEntityInitParams* s, 
                         ClientEntityInitParams* c)
{
    ArchetypeLayout* layout = archetypeLayouts + GetArchetype(ID);
    for(u32 componentIndex = 0; componentIndex < ArrayCount(layout->hasComponents); ++componentIndex)
    {
        ArchetypeComponent* component = layout->hasComponents + componentIndex;
        if(component->exists)
        {
            void* componentPtr = AdvanceVoidPtrBytes(GetPtr(server, ID), component->offset);
            component->init(server, componentPtr, ID, common, s, c);
        }
    }
}
#else
internal void InitEntity(GameModeWorld* worldMode, EntityID ID, 
                         CommonEntityInitParams* common, 
                         ServerEntityInitParams* s, 
                         ClientEntityInitParams* c)
{
    ArchetypeLayout* layout = archetypeLayouts + GetArchetype(ID);
    for(u32 componentIndex = 0; componentIndex < ArrayCount(layout->hasComponents); ++componentIndex)
    {
        ArchetypeComponent* component = layout->hasComponents + componentIndex;
        if(component->exists)
        {
            void* componentPtr = AdvanceVoidPtrBytes(GetPtr(worldMode, ID), component->offset);
            component->init(worldMode, componentPtr, ID, common, s, c);
        }
    }
}
#endif