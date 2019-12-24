internal Rect3 StandardBounds(Vec3 dim, Vec3 offset)
{
    offset = {};
    Rect3 result = Offset(RectCenterDim(V3(0, 0, 0), dim), offset + V3(0, 0, 0.5f * dim.z));
    return result;
}

#define PropertyToU16(property, enum) ExistMetaPropertyValue(Property_##property, Tokenize((enum).value))
internal u16 ExistMetaPropertyValue(u16 propertyType, Token value);;
internal void AddPossibleActions(PossibleActionList* list, PossibleActionDefinition* actions, u32 actionCount)
{
    list->actionCount = 0;
    for(u32 actionIndex = 0; actionIndex < actionCount; ++actionIndex)
    {
        if(list->actionCount < ArrayCount(list->actions))
        {
            PossibleActionDefinition* source = actions + actionIndex;
            u16 action = PropertyToU16(action, source->action);
            if(action != 0xffff)
            {
                PossibleAction* dest = list->actions + list->actionCount++;
                
                dest->action = action;
                
                if(source->special.value != Special_Invalid)
                {
                    dest->distance.type = ActionDistance_Special;
                    dest->distance.propertyIndex = source->special.value;
                }
                else
                {
                    dest->distance.type = ActionDistance_Standard;
                    dest->distance.startDistance = source->distance;
                    dest->distance.continueCoeff = source->continueDistanceCoeff;
                }
                
                dest->time = source->time;
                dest->requiredUsingType = source->requiredUsingType;
                dest->requiredEquippedType = source->requiredEquippedType;
            }
        }
    }
}

#ifdef FORG_SERVER
internal u16 GetRandomEssence(RandomSequence* seq);
INIT_COMPONENT_FUNCTION(InitDefaultComponent)
{
    ServerState* server = (ServerState*) state;
    DefaultComponent* def = (DefaultComponent*) componentPtr;
    def->updateSent = false;
    
    def->basicPropertiesChanged = 0;
    def->basicPropertiesChanged |= EntityBasics_Definition; 
    def->basicPropertiesChanged |= EntityBasics_Position;
    
    def->healthPropertiesChanged = 0xffff;
    
    def->miscPropertiesChanged = 0xffff;
    
    def->P = s->P;
    def->flags = InitU32(basicPropertiesChanged, EntityBasics_Flags, 0);
    def->seed = s->seed;
    def->definitionID = common->definitionID;
    def->status = InitU16(basicPropertiesChanged, EntityBasics_Status, 0);
    def->spawnerID = {};
    
    if(s->canGoIntoWater)
    {
        AddEntityFlags(def, EntityFlag_canGoIntoWater);
    }
    
    if(s->fearsLight)
    {
        AddEntityFlags(def, EntityFlag_fearsLight);
    }
    
    b32 addedSomething = false;
    for(u16 essenceIndex = 0; essenceIndex < Count_essence; ++essenceIndex)
    {
        if(common->essences[essenceIndex] > 0)
        {
            addedSomething = true;
        }
        def->essences[essenceIndex] = common->essences[essenceIndex];
    }
    
    if(!addedSomething)
    {
        RandomSequence seq = Seed(def->seed);
        for(u32 essenceIndex = 0; essenceIndex < s->defaultEssenceCount; ++essenceIndex)
        {
            u16 essence = GetRandomEssence(&seq);
            ++def->essences[essence];
        }
    }
}

INIT_COMPONENT_FUNCTION(InitPhysicComponent)
{
    PhysicComponent* physic = (PhysicComponent*) componentPtr;
    
    physic->boundType = common->boundType.value;
    physic->bounds = StandardBounds(common->boundDim, common->boundOffset);
    physic->acc = s->startingAcceleration;
    physic->speed = s->startingSpeed;
    physic->accelerationCoeff = s->accelerationCoeff;
    physic->drag = s->drag;
}

INIT_COMPONENT_FUNCTION(InitActionComponent)
{
    ActionComponent* action = (ActionComponent*) componentPtr;
    action->action = InitU16(basicPropertiesChanged, EntityBasics_Action, 0);
    action->time = 0;
}

internal void AddRandomEffects(EffectComponent* effects, EffectBinding* bindings, ArrayCounter bindingCount, u16* essences)
{
    for(u16 essenceIndex = 0; essenceIndex < Count_essence; ++essenceIndex)
    {
        u16 essenceQuantity = essences[essenceIndex];
        if(essenceQuantity > 0)
        {
            //Assert(essenceQuantity == 1);
            GameProperty property = GameProp(essence, essenceIndex);
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
    }
}

INIT_COMPONENT_FUNCTION(InitEffectComponent)
{
    ServerState* server = (ServerState*) state;
    EffectComponent* effect = (EffectComponent*) componentPtr;
    
    u16* essences = common->essences;
    AddRandomEffects(effect, s->bindings, s->bindingCount, essences);
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
    
    u16 runningCount = 0;
    for(ArrayCounter slotBatchIndex = 0; slotBatchIndex < s->storeSlotCounter; ++slotBatchIndex)
    {
        InventorySlots* slots = s->storeSlots + slotBatchIndex;
        for(u16 index = 0; index < slots->count; ++index)
        {
            if(runningCount < ArrayCount(dest->storedObjects))
            {
                InventorySlot* slot = dest->storedObjects + runningCount++;
                slot->flags_type = slots->type.value; 
                SetBoundedID(slot, {});
            }
        }
    }
    
    runningCount = 0;
    for(ArrayCounter slotBatchIndex = 0; slotBatchIndex < s->usingSlotCounter; ++slotBatchIndex)
    {
        InventorySlots* slots = s->usingSlots + slotBatchIndex;
        for(u16 index = 0; index < slots->count; ++index)
        {
            if(runningCount < ArrayCount(dest->usingObjects))
            {
                InventorySlot* slot = dest->usingObjects + runningCount++;
                slot->flags_type = slots->type.value; 
                SetBoundedID(slot, {});
            }
        }
    }
}

INIT_COMPONENT_FUNCTION(InitSkillComponent)
{
}

INIT_COMPONENT_FUNCTION(InitBrainComponent)
{
    BrainComponent* brain = (BrainComponent*) componentPtr;
    
    *brain = {};
    
    brain->state = BrainState_Wandering;
    brain->type = PropertyToU16(brainType, s->brainType);
    brain->ID = {};
}

INIT_COMPONENT_FUNCTION(InitTempEntityComponent)
{
    TempEntityComponent* temp = (TempEntityComponent*) componentPtr;
    temp->time = 0.0f;
    temp->targetTime = 1.0f;
}

INIT_COMPONENT_FUNCTION(InitStaticComponent)
{
    ServerState* server = (ServerState*) state;
    StaticComponent* staticComp = (StaticComponent*) componentPtr;
    AddToPartitionResult addResult = AddToSpatialPartition(&server->gamePool, &server->staticPartition, s->P, {}, ID);
    staticComp->block = addResult.block;
}

INIT_COMPONENT_FUNCTION(InitAliveComponent)
{
    AliveComponent* alive = (AliveComponent*) componentPtr;
    
    u32 maxPhysical = 100;
    u32 maxMental = 100;
    alive->maxPhysicalHealth = InitU32(healthPropertiesChanged, HealthFlag_MaxPhysical, maxPhysical);
    alive->physicalHealth = InitU32(healthPropertiesChanged, HealthFlag_Physical, maxPhysical);
    
    alive->maxMentalHealth = InitU32(healthPropertiesChanged, HealthFlag_MaxMental, maxMental);
    alive->mentalHealth = InitU32(healthPropertiesChanged, HealthFlag_Mental, maxMental);
}

INIT_COMPONENT_FUNCTION(InitMiscComponent)
{
    MiscComponent* misc = (MiscComponent*) componentPtr;
    misc->attackDistance = InitR32(miscPropertiesChanged, MiscFlag_AttackDistance, 1.0f);
    misc->attackContinueCoeff = InitR32(miscPropertiesChanged, MiscFlag_AttackContinueCoeff, 2.0f);
    misc->lightRadious = InitR32(miscPropertiesChanged, MiscFlag_LightRadious, s->lightRadious);
}

#else
INIT_COMPONENT_FUNCTION(InitBaseComponent)
{
    BaseComponent* base = (BaseComponent*) componentPtr;
    base->definitionID = common->definitionID;
    base->seed = c->seed;
    base->universeP = {};
    base->velocity = {};
    base->flags = 0;
    base->bounds = StandardBounds(common->boundDim, common->boundOffset);
    base->worldBounds = {};
    base->projectedOnScreen = {};
    base->serverID = c->ID;
    base->draggingID = {};
    base->timeSinceLastUpdate = 0;
    base->totalLifeTime = 0;
    base->deletedTime = 0;
    
    base->fadeInTime = c->fadeInTime;
    base->fadeOutTime = c->fadeOutTime;
    
    for(u32 propertyIndex = 0; propertyIndex < ArrayCount(base->properties); ++propertyIndex)
    {
        base->properties[propertyIndex] = {};
    }
    
    for(u32 essenceIndex = 0; essenceIndex < Count_essence; ++essenceIndex)
    {
        base->essences[essenceIndex] = common->essences[essenceIndex];
    }
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
    animation->currentAnimation = 0;
    animation->totalTime = 0;
    animation->time = 0;
    animation->oldTime = 0;
    animation->backward = false;
    animation->skinHash =c->skin.subtypeHash;
    animation->skinProperties = {};
    animation->skeletonHash =c->skeleton.subtypeHash;
    animation->skeletonProperties = {};
    animation->flipOnYAxis = 0;
    animation->scale = 0;
    animation->speed = 1.0f;
    animation->defaultScaleComputed = false;
    animation->spawnProjectileOffset = c->spawnProjectileOffset;
}

INIT_COMPONENT_FUNCTION(InitRockComponent)
{
    GameModeWorld* worldMode = (GameModeWorld*) state;
    Assets* assets = worldMode->gameState->assets;
    RockComponent* dest = (RockComponent*) componentPtr;
    
    RandomSequence seq = Seed(c->seed);
    InitImageReference(assets, &dest, &c, rock);
    InitImageReference(assets, &dest, &c, mineral);
    dest->color = RandomizeColor(c->rockColor, c->rockColorV, &seq);
    
    dest->mineralDensity = 1.0f;
}

INIT_COMPONENT_FUNCTION(InitGrassComponent)
{
    GameModeWorld* worldMode = (GameModeWorld*) state;
    GrassComponent* dest = (GrassComponent*) componentPtr;
    dest->windInfluence = c->windInfluence;
    dest->windFrequencyStandard = (u8) c->windFrequencyStandard;
    dest->windFrequencyOverlap = (u8) c->windFrequencyOverlap;
    dest->count = c->instanceCount;
    dest->maxOffset = c->instanceMaxOffset;
    dest->bounds = RectCenterDim(V3(0, 0, 0), c->grassBounds);
    
}

INIT_COMPONENT_FUNCTION(InitPlantComponent)
{
    GameModeWorld* worldMode = (GameModeWorld*) state;
    Assets* assets = worldMode->gameState->assets;
    PlantComponent* dest = (PlantComponent*) componentPtr;
    
    RandomSequence seq = Seed(c->seed);
    InitImageReference(assets, &dest, &c, trunk);
    dest->hasBranchVariant = c->hasBranchVariant;
    dest->branchColor = RandomizeColor(c->branchColor, c->branchColorV, &seq);
    InitImageReference(assets, &dest, &c, branch);
    
    dest->hasLeafVariant = c->hasLeafVariant;
    dest->leafColor = RandomizeColor(c->leafColor, c->leafColorV, &seq);
    InitImageReference(assets, &dest, &c, leaf);
    
    dest->hasFlowerVariant = c->hasFlowerVariant;
    dest->flowerColor = RandomizeColor(c->flowerColor, c->flowerColorV, &seq);
    InitImageReference(assets, &dest, &c, flower);
    
    dest->hasFruitVariant = c->hasFruitVariant;
    dest->fruitColor = RandomizeColor(c->fruitColor, c->fruitColorV, &seq);
    InitImageReference(assets, &dest, &c, fruit);
    
    dest->windInfluence = c->windInfluence;
    dest->leafDensity = 1.0f;
    dest->flowerDensity = 1.0f;
    dest->fruitDensity = 1.0f;
}

INIT_COMPONENT_FUNCTION(InitStandardImageComponent)
{
    GameModeWorld* worldMode = (GameModeWorld*) state;
    Assets* assets = worldMode->gameState->assets;
    StandardImageComponent* dest = (StandardImageComponent*) componentPtr;
    InitImageReference(assets, &dest, &c, entity);
}

INIT_COMPONENT_FUNCTION(InitSegmentImageComponent)
{
    GameModeWorld* worldMode = (GameModeWorld*) state;
    Assets* assets = worldMode->gameState->assets;
    StandardImageComponent* dest = (StandardImageComponent*) componentPtr;
    InitImageReference(assets, &dest, &c, entity);
}

INIT_COMPONENT_FUNCTION(InitShadowComponent)
{
    ShadowComponent* dest = (ShadowComponent*) componentPtr;
    InitShadow(dest, c);
}

internal BitmapId GetImageFromReference(Assets* assets, ImageReference* reference, RandomSequence* seq);
INIT_COMPONENT_FUNCTION(InitMagicQuadComponent)
{
    GameModeWorld* worldMode = (GameModeWorld*) state;
    Assets* assets = worldMode->gameState->assets;
    
    MagicQuadComponent* dest = (MagicQuadComponent*) componentPtr;
    
    dest->color = 0xffffffff;
    InitImageReference(assets, &dest, &c, entity);
    
    RandomSequence seq = Seed(c->seed);
    dest->bitmapID = GetImageFromReference(assets, &dest->entity, &seq);
    
    Rect3 bounds = StandardBounds(common->boundDim, common->boundOffset);
    r32 height = GetHeight(bounds);
    r32 width = GetWidth(bounds);
    
    dest->lateral = width * magicLateralVector;
    dest->up = height * magicUpVector;
    dest->offset = {};
    dest->alphaThreesold = 0;
    dest->invUV = {};
    if(IsValid(dest->bitmapID))
    {
        PAKBitmap* bitmap = GetBitmapInfo(assets, dest->bitmapID);
        dest->offset = (bitmap->align[0] - 0.5f) * dest->lateral + (bitmap->align[1] - 0.5f) * dest->up;
        dest->alphaThreesold = bitmap->alphaThreesold;
        dest->invUV = GetInvUV(bitmap->dimension[0], bitmap->dimension[1]);
    }
    
    dest->lateral *= 0.5f;
    dest->up *= 0.5f;
}

internal void InitFrameByFrameComponent_(FrameByFrameAnimationComponent* dest, r32 speed, u64 subtypeHash, b32 overridesPivot, Vec2 pivot)
{
    dest->runningTime = 0;
    dest->speed = speed;
    dest->typeHash = subtypeHash;
    dest->overridesPivot = overridesPivot;
    dest->pivot = pivot;
}

INIT_COMPONENT_FUNCTION(InitFrameByFrameAnimationComponent)
{
    GameModeWorld* worldMode = (GameModeWorld*) state;
    Assets* assets = worldMode->gameState->assets;
    FrameByFrameAnimationComponent* dest = (FrameByFrameAnimationComponent*) componentPtr;
    
    InitFrameByFrameComponent_(dest, c->frameByFrameSpeed, c->frameByFrameImageType.subtypeHash, c->frameByFrameOverridesPivot, c->frameByFramePivot);
}

INIT_COMPONENT_FUNCTION(InitMultipartAnimationComponent)
{
    GameModeWorld* worldMode = (GameModeWorld*) state;
    Assets* assets = worldMode->gameState->assets;
    
    MultipartAnimationComponent* dest = (MultipartAnimationComponent*) componentPtr;
    dest->staticCount = 0;
    dest->frameByFrameCount = 0;
    for(u16 staticIndex = 0; staticIndex < c->multipartStaticCount; ++staticIndex)
    {
        if(dest->staticCount < ArrayCount(dest->staticParts))
        {
            MultipartStaticPiece* source = c->multipartStaticPieces + staticIndex;
            ImageReference* destImage = dest->staticParts + dest->staticCount++;
            InitImageReference_(assets, destImage, &source->properties);
        }
    }
    
    for(u16 frameByFrameIndex = 0; 
        frameByFrameIndex < c->multipartFrameByFrameCount;
        ++frameByFrameIndex)
    {
        if(dest->frameByFrameCount < ArrayCount(dest->frameByFrameParts))
        {
            FrameByFrameAnimationComponent* destFrame = dest->frameByFrameParts + dest->frameByFrameCount++;
            
            MultipartFrameByFramePiece* source = c->multipartFrameByFramePieces + frameByFrameIndex;
            InitFrameByFrameComponent_(destFrame, source->speed, source->image.subtypeHash, 0, {});
        }
    }
}

internal void InitLayout(Assets* assets, LayoutPiece* destPieces, u32* destPieceCount, u32 maxDestPieceCount, LayoutPieceProperties* pieces, u32 pieceCount, u16* essences)
{
    *destPieceCount = 0;
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
    
    if(essences)
    {
        for(u32 pieceIndex = 0; pieceIndex < *destPieceCount; ++pieceIndex)
        {
            LayoutPiece* destPiece = destPieces + pieceIndex;
            for(u16 essenceIndex = 0; essenceIndex < Count_essence; ++essenceIndex)
            {
                u16 essenceQuantity = essences[essenceIndex];
                if(essenceQuantity > 0)
                {
                    GameProperty property = GameProp(essence, essenceIndex);
                    AddGameProperty_(&destPiece->image.properties, property, GameProperty_Optional);
                }
            }
        }
    }
}

INIT_COMPONENT_FUNCTION(InitLayoutComponent)
{
    GameModeWorld* worldMode = (GameModeWorld*) state;
    Assets* assets = worldMode->gameState->assets;
    
    LayoutComponent* dest = (LayoutComponent*) componentPtr;
    dest->rootHash = StringHash(c->layoutRootName.name);
    dest->rootScale = V2(1, 1);
    dest->rootAngle = 0;
    
    u16* essences = common->essences;
    
    InitLayout(assets, dest->pieces, &dest->pieceCount, ArrayCount(dest->pieces), c->layoutPieces, c->pieceCount, essences);
    InitLayout(assets, dest->openPieces, &dest->openPieceCount, ArrayCount(dest->openPieces), c->openLayoutPieces, c->openPieceCount, essences);
    InitLayout(assets, dest->usingPieces, &dest->usingPieceCount, ArrayCount(dest->usingPieces), c->usingLayoutPieces, c->usingPieceCount, essences);
    
    InitLayout(assets, dest->equippedPieces, &dest->equippedPieceCount, ArrayCount(dest->equippedPieces), c->equippedLayoutPieces, c->equippedPieceCount, essences);
}

INIT_COMPONENT_FUNCTION(InitEquipmentComponent)
{
    
}

INIT_COMPONENT_FUNCTION(InitUsingComponent)
{
    
}

INIT_COMPONENT_FUNCTION(InitAnimationMappingComponent)
{
    
}

INIT_COMPONENT_FUNCTION(InitContainerComponent)
{
    ContainerComponent* dest = (ContainerComponent*) componentPtr;
    dest->zoomCoeff = c->lootingZoomCoeff;
    dest->displayInStandardMode = c->displayInStandardMode;
    dest->desiredOpenedDim = c->desiredOpenedDim;
    dest->desiredUsingDim = c->desiredUsingDim;
}

INIT_COMPONENT_FUNCTION(InitSkillMappingComponent)
{
}

internal void FreeAnimationEffect(AnimationEffect* effect);
INIT_COMPONENT_FUNCTION(InitAnimationEffectComponent)
{
    AnimationEffectComponent* animation = (AnimationEffectComponent*) componentPtr;
    animation->params = DefaultAnimationParams();
    animation->lightIntensity = 2.0f;
    animation->lightColor = V3(1, 1, 1);
    for(u32 effectIndex = 0; effectIndex < animation->effectCount; ++effectIndex)
    {
        AnimationEffect* effect = animation->effects + effectIndex;
        FreeAnimationEffect(effect);
    }
    animation->effectCount = 0;
}

INIT_COMPONENT_FUNCTION(InitSoundEffectComponent)
{
    SoundEffectComponent* sounds = (SoundEffectComponent*) componentPtr;
    sounds->soundCount = 0;
}



INIT_COMPONENT_FUNCTION(InitBoltComponent)
{
    GameModeWorld* worldMode = (GameModeWorld*) state;
    
    BoltComponent* bolt = (BoltComponent*) componentPtr;
    bolt->ttl = R32_MAX;
    bolt->seed = GetNextUInt32(&worldMode->entropy);
    bolt->timeSinceLastAnimationTick = R32_MAX;
    bolt->animationSeq = Seed(bolt->seed);
    bolt->highOffset = V3(0, 0, 4);
}

INIT_COMPONENT_FUNCTION(InitAliveComponent)
{
    AliveComponent* alive = (AliveComponent*) componentPtr;
    
    alive->physicalHealth = 0;
    alive->maxPhysicalHealth = 0;
    
    alive->mentalHealth = 0;
    alive->maxMentalHealth = 0;
}

INIT_COMPONENT_FUNCTION(InitMiscComponent)
{
    MiscComponent* misc = (MiscComponent*) componentPtr;
    misc->attackDistance = 0;
    misc->attackContinueCoeff = 0;
    misc->lightRadious = 0;
}

INIT_COMPONENT_FUNCTION(InitSkillComponent)
{
}


INIT_COMPONENT_FUNCTION(InitRecipeEssenceComponent)
{
    RecipeEssenceComponent* essences = (RecipeEssenceComponent*) componentPtr;
    for(u32 essenceIndex = 0; essenceIndex < ArrayCount(essences->essences); ++essenceIndex)
    {
        essences->projectedOnScreen[essenceIndex] = InvertedInfinityRect2();
        essences->essences[essenceIndex] = 0;
    }
}

#endif

INIT_COMPONENT_FUNCTION(InitSkillDefComponent)
{
    SkillDefComponent* skill = (SkillDefComponent*) componentPtr;
    skill->targetSkill = common->targetSkill;
    skill->passive = common->passive;
    skill->level = 0;
}

INIT_COMPONENT_FUNCTION(InitInteractionComponent)
{
    InteractionComponent* dest = (InteractionComponent*) componentPtr;
    AddPossibleActions(dest->actions + InteractionList_Ground, common->groundActions, common->groundActionCount);
    AddPossibleActions(dest->actions + InteractionList_Equipment, common->equipmentActions, common->equipmentActionCount);
    AddPossibleActions(dest->actions + InteractionList_Container, common->containerActions, common->containerActionCount);
    AddPossibleActions(dest->actions + InteractionList_Equipped, common->equippedActions, common->equippedActionCount);
    AddPossibleActions(dest->actions + InteractionList_Dragging, common->draggingActions, common->draggingActionCount);
    
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