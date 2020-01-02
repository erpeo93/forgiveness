internal Rect3 StandardBounds(RandomSequence* seq, Vec3 dim, Vec3 offset, Vec3 dimV)
{
    offset = {};
    
    Vec3 actualDim = RandomizeVec3(dim, dimV, seq);
    Rect3 result = Offset(RectCenterDim(V3(0, 0, 0), actualDim), offset + V3(0, 0, 0.5f * actualDim.z));
    return result;
}

#define PropertyToU16(property, enum) ExistMetaPropertyValue(Property_##property, Tokenize((enum).value))
internal u16 ExistMetaPropertyValue(u16 propertyType, Token value);;
internal void AddPossibleActions(Assets* assets, PossibleActionList* list, PossibleActionDefinition* actions, u32 actionCount)
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
                dest->requiredUsingType = GetEntityType(assets, source->requiredUsingType);
                dest->requiredEquippedType = GetEntityType(assets, source->requiredEquippedType);
            }
        }
    }
}

#ifdef FORG_SERVER
internal u16 GetRandomEssence(RandomSequence* seq);
INIT_COMPONENT_FUNCTION(InitDefaultComponent)
{
    RandomSequence seq = Seed(s->seed);
    
    ServerState* server = (ServerState*) state;
    DefaultComponent* def = (DefaultComponent*) componentPtr;
    def->updateSent = false;
    def->networkFlags = 0xffff;
    def->P = s->P;
    SetU32(&def->flags, 0);
    def->seed = s->seed;
    def->type = common->type;
    def->spawnerID = {};
    def->boundType = common->boundType.value;
    def->bounds = StandardBounds(&seq, common->boundDim, common->boundOffset, common->boundDimV);
    if(s->canGoIntoWater)
    {
        AddEntityFlags(def, EntityFlag_canGoIntoWater);
    }
    
    if(s->fearsLight)
    {
        AddEntityFlags(def, EntityFlag_fearsLight);
    }
    
    for(u16 essenceIndex = 0; essenceIndex < Count_essence; ++essenceIndex)
    {
        def->essences[essenceIndex] = common->essences[essenceIndex];
    }
}

INIT_COMPONENT_FUNCTION(InitMovementComponent)
{
    MovementComponent* movement = (MovementComponent*) componentPtr;
    movement->acc = s->startingAcceleration;
    movement->speed = s->startingSpeed;
    movement->accelerationCoeff = s->accelerationCoeff;
    movement->drag = s->drag;
}

INIT_COMPONENT_FUNCTION(InitActionComponent)
{
    ActionComponent* action = (ActionComponent*) componentPtr;
    SetU16(&action->action, idle);
    action->time = 0;
}

internal GameEffectInstance InstanceEffect(Assets* assets, GameEffect* effect)
{
    GameEffectInstance result = {};
    result.timer = effect->timer;
    result.targetEffect = effect->targetEffect;
    result.action = effect->action.value;
    result.type = effect->effectType.value;
    result.spawnType = GetEntityType(assets, effect->spawnType);
    result.power = effect->power;
    return result;
}

internal void AddRandomEffects(Assets* assets, EffectComponent* effects, EffectBinding* bindings, ArrayCounter bindingCount, u16* essences)
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
                        GameEffectInstance* dest = effects->effects + effects->effectCount++;
                        *dest = InstanceEffect(assets, &binding->effect);
                    }
                    break;
                }
            }
        }
    }
}

internal GameEffect* RollProbabilityEffect(RandomSequence* seq, ProbabilityEffect* effect)
{
    GameEffect* result = 0;
    
    r32 roll = RandomUni(seq);
    if(roll <= effect->probability)
    {
        r32 totalWeight = 0;
        
        for(ArrayCounter optionIndex = 0; optionIndex < effect->optionCount; ++optionIndex)
        {
            ProbabilityEffectOption* option = effect->options + optionIndex;
            totalWeight += option->weight;
        }
        r32 choice = RandomUni(seq) * totalWeight;
        
        r32 runningWeight = 0;
        for(ArrayCounter optionIndex = 0; optionIndex < effect->optionCount; ++optionIndex)
        {
            ProbabilityEffectOption* option = effect->options + optionIndex;
            r32 oldWeight = runningWeight;
            r32 newWeight = oldWeight + option->weight;
            if(choice > oldWeight && choice <= newWeight)
            {
                result = &option->effect;
                break;
            }
            
            runningWeight += option->weight;
        }
    }
    return result;
}

INIT_COMPONENT_FUNCTION(InitEffectComponent)
{
    ServerState* server = (ServerState*) state;
    EffectComponent* effect = (EffectComponent*) componentPtr;
    effect->effectCount = 0;
    
    for(ArrayCounter effectIndex = 0; effectIndex < s->defaultEffectsCount; ++effectIndex)
    {
        if(effect->effectCount < ArrayCount(effect->effects))
        {
            GameEffectInstance* dest = effect->effects + effect->effectCount++;
            *dest = InstanceEffect(assets, s->defaultEffects + effectIndex);
        }
    }
    
    
    RandomSequence seq = Seed(s->seed);
    for(ArrayCounter effectIndex = 0; effectIndex < s->probabilityEffectsCount; ++effectIndex)
    {
        GameEffect* rolled = RollProbabilityEffect(&seq, s->probabilityEffects + effectIndex);
        if(rolled)
        {
            if(effect->effectCount < ArrayCount(effect->effects))
            {
                GameEffectInstance* dest = effect->effects + effect->effectCount++;
                *dest = InstanceEffect(assets, rolled);
            }
        }
    }
    
    u16* essences = common->essences;
    AddRandomEffects(assets, effect, s->bindings, s->bindingCount, essences);
}

INIT_COMPONENT_FUNCTION(InitCollisionEffectsComponent)
{
    CollisionEffectsComponent* collision = (CollisionEffectsComponent*) componentPtr;
    collision->effectCount = 0;
    for(u32 effectIndex = 0; effectIndex < s->collisionEffectsCount; ++effectIndex)
    {
        if(collision->effectCount < ArrayCount(collision->effects))
        {
            GameEffectInstance* dest = collision->effects + collision->effectCount++;
            *dest = InstanceEffect(assets, s->collisionEffects + effectIndex);
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
    ServerState* server = (ServerState*) state;
    
    *brain = {};
    
    brain->state = BrainState_Wandering;
    brain->type = PropertyToU16(brainType, s->brainType);
    brain->targetID = {};
    brain->wanderDirection = V3(RandomBilV2(&server->entropy), 0);
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

INIT_COMPONENT_FUNCTION(InitHealthComponent)
{
    HealthComponent* health = (HealthComponent*) componentPtr;
    
    r32 maxPhysical = s->maxPhysicalHealth;
    r32 maxMental = s->maxMentalHealth;
    SetR32Default(health, maxPhysicalHealth, maxPhysical);
    SetR32(&health->physicalHealth, maxPhysical);
    
    SetR32Default(health, maxMentalHealth, maxMental);
    SetR32(&health->mentalHealth, maxMental);
}

INIT_COMPONENT_FUNCTION(InitCombatComponent)
{
    CombatComponent* combat = (CombatComponent*) componentPtr;
    
    r32 defaultAttackDistance = 1.0f;
    r32 defaultAttackContinueCoeff = 2.0f;
    
    SetR32Default(combat, attackDistance, defaultAttackDistance);
    SetR32Default(combat, attackContinueCoeff, defaultAttackContinueCoeff);
}

INIT_COMPONENT_FUNCTION(InitLightComponent)
{
    LightComponent* light = (LightComponent*) componentPtr;
    SetR32Default(light, lightRadious, s->lightRadious);
}

INIT_COMPONENT_FUNCTION(InitVegetationComponent)
{
    VegetationComponent* dest = (VegetationComponent*) componentPtr;
    
    SetR32Default(dest, flowerGrowingSpeed, s->flowerGrowingSpeed);
    SetR32Default(dest, fruitGrowingSpeed, s->fruitGrowingSpeed);
    
    dest->requiredFlowerDensity = s->requiredFlowerDensity;
    dest->requiredFruitDensity = s->requiredFruitDensity;
    
    SetR32(&dest->flowerDensity, common->flowerDensity);
    SetR32(&dest->fruitDensity, common->fruitDensity);
}

#else
INIT_COMPONENT_FUNCTION(InitBaseComponent)
{
    BaseComponent* base = (BaseComponent*) componentPtr;
    RandomSequence seq = Seed(c->seed);
    base->type = common->type;
    base->seed = c->seed;
    base->universeP = {};
    base->velocity = {};
    base->flags = 0;
    base->bounds = StandardBounds(&seq, common->boundDim, common->boundOffset, common->boundDimV);
    base->worldBounds = {};
    base->projectedOnScreen = {};
    base->serverID = c->ID;
    base->draggingID = {};
    base->timeSinceLastUpdate = 0;
    base->totalLifeTime = 0;
    base->deletedTime = 0;
    
    base->fadeInTime = c->fadeInTime;
    base->fadeOutTime = c->fadeOutTime;
    
    for(u32 essenceIndex = 0; essenceIndex < Count_essence; ++essenceIndex)
    {
        base->essences[essenceIndex] = common->essences[essenceIndex];
    }
}

internal void InitShadow(ShadowComponent* shadow, ClientEntityInitParams* params)
{
    shadow->offset = params->shadowOffset;
    shadow->height = params->shadowHeight;
    shadow->scale = params->shadowScale;
    shadow->color = params->shadowColor;
}
#define InitImageReference(assets, dest, source, reference) InitImageReference_(assets, dest->reference, source->reference##Properties)
internal void InitImageReference_(Assets* assets, ImageReference* dest,ImageProperties* sourceProperties)
{
    dest->typeHash = sourceProperties->imageType.subtypeHash;
    dest->emittors = sourceProperties->emittors;
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
    
    RandomSequence seq = Seed(c->seed);
    if(c->possibleSkinCount > 0)
    {
        u16 skinIndex = SafeTruncateToU16(RandomChoice(&seq, c->possibleSkinCount));
        PossibleSkin* skin = c->possibleSkins + skinIndex;
        animation->skinHash =skin->skin.subtypeHash;
    }
    else
    {
        animation->skinHash = 0;
    }
    animation->skinProperties = {};
    animation->skeletonHash =c->skeleton.subtypeHash;
    animation->skeletonProperties = {};
    animation->flipOnYAxis = 0;
    animation->cameraZOffsetWhenOnFocus = c->cameraZOffsetWhenOnFocus;
    animation->scaleCoeffWhenOnFocus = c->scaleCoeffWhenOnFocus;
    animation->defaultScaleComputed = false;
    animation->scale = 0;
    animation->speed = 1.0f;
    animation->spawnProjectileOffset = c->spawnProjectileOffset;
    
    
    for(u16 colorationIndex = 0; colorationIndex < ArrayCount(animation->colorations); ++colorationIndex)
    {
        animation->colorations[colorationIndex] = V4(1, 1, 1, 1);
    }
    
    for(ArrayCounter colorationIndex = 0; colorationIndex < c->colorationCount; ++colorationIndex)
    {
        Coloration* coloration = c->colorations + colorationIndex;
        if(coloration->optionCount > 0 && colorationIndex < ArrayCount(animation->colorations))
        {
            Vec4 choice = coloration->options[RandomChoice(&seq, coloration->optionCount)];
            animation->colorations[colorationIndex] = choice;
        }
    }
}

INIT_COMPONENT_FUNCTION(InitRockComponent)
{
    RockComponent* dest = (RockComponent*) componentPtr;
    
    RandomSequence seq = Seed(c->seed);
    InitImageReference(assets, &dest, &c, rock);
    InitImageReference(assets, &dest, &c, mineral);
    dest->color = RandomizeColor(c->rockColor, c->rockColorV, &seq);
    
    dest->mineralDensity = 1.0f;
}

INIT_COMPONENT_FUNCTION(InitGrassComponent)
{
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
    
    dest->leafWindInfluence = c->leafWindInfluence;
    dest->flowerWindInfluence = c->flowerWindInfluence;
    dest->fruitWindInfluence = c->fruitWindInfluence;
    dest->dissolveDuration = c->dissolveDuration;
}

INIT_COMPONENT_FUNCTION(InitStandardImageComponent)
{
    StandardImageComponent* dest = (StandardImageComponent*) componentPtr;
    InitImageReference(assets, &dest, &c, entity);
}

INIT_COMPONENT_FUNCTION(InitSegmentImageComponent)
{
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
    RandomSequence seq = Seed(c->seed);
    
    MagicQuadComponent* dest = (MagicQuadComponent*) componentPtr;
    
    dest->color = 0xffffffff;
    InitImageReference(assets, &dest, &c, entity);
    
    dest->bitmapID = GetImageFromReference(assets, &dest->entity, &seq);
    
    Rect3 bounds = StandardBounds(&seq, common->boundDim, common->boundOffset, common->boundDimV);
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

internal void InitFrameByFrameComponent_(FrameByFrameAnimationComponent* dest, r32 speed, u64 subtypeHash, b32 emittors, b32 overridesPivot, Vec2 pivot)
{
    dest->runningTime = 0;
    dest->speed = speed;
    dest->typeHash = subtypeHash;
    dest->overridesPivot = overridesPivot;
    dest->emittors = emittors;
    dest->pivot = pivot;
}

INIT_COMPONENT_FUNCTION(InitFrameByFrameAnimationComponent)
{
    FrameByFrameAnimationComponent* dest = (FrameByFrameAnimationComponent*) componentPtr;
    
    InitFrameByFrameComponent_(dest, c->frameByFrameSpeed, c->frameByFrameImageType.subtypeHash, c->frameByFrameEmittors, c->frameByFrameOverridesPivot, c->frameByFramePivot);
}

INIT_COMPONENT_FUNCTION(InitMultipartAnimationComponent)
{
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
            InitFrameByFrameComponent_(destFrame, source->speed, source->image.subtypeHash, source->emittors, 0, {});
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
            destPiece->color = piece->color;
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
    dest->zoomSpeed = c->lootingZoomSpeed;
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
    animation->outlineWidth = c->outlineWidth;
    animation->speedOnFocus = c->speedOnFocus;
    animation->speedOnNoFocus = c->speedOnNoFocus;
    animation->offsetMaxOnFocus = c->offsetMaxOnFocus;
    animation->scaleMaxOnFocus = c->scaleMaxOnFocus;
}

INIT_COMPONENT_FUNCTION(InitSoundEffectComponent)
{
    SoundEffectComponent* sounds = (SoundEffectComponent*) componentPtr;
    sounds->soundCount = 0;
}


INIT_COMPONENT_FUNCTION(InitActionComponent)
{
    ActionComponent* action = (ActionComponent*) componentPtr;
    action->action = idle;
}

INIT_COMPONENT_FUNCTION(InitHealthComponent)
{
    HealthComponent* health = (HealthComponent*) componentPtr;
    
    health->physicalHealth = 0;
    health->maxPhysicalHealth = 0;
    
    health->mentalHealth = 0;
    health->maxMentalHealth = 0;
}

INIT_COMPONENT_FUNCTION(InitCombatComponent)
{
    CombatComponent* combat = (CombatComponent*) componentPtr;
    combat->attackDistance = 0;
    combat->attackContinueCoeff = 0;
}

INIT_COMPONENT_FUNCTION(InitLightComponent)
{
    LightComponent* light = (LightComponent*) componentPtr;
    light->lightRadious = 0;
    light->lightColor = common->lightColor;
}

INIT_COMPONENT_FUNCTION(InitVegetationComponent)
{
    VegetationComponent* vegetation = (VegetationComponent*) componentPtr;
    vegetation->flowerDensity = common->flowerDensity;
    vegetation->fruitDensity = common->fruitDensity;
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
    AddPossibleActions(assets, dest->actions + InteractionList_Ground, common->groundActions, common->groundActionCount);
    AddPossibleActions(assets, dest->actions + InteractionList_Equipment, common->equipmentActions, common->equipmentActionCount);
    AddPossibleActions(assets, dest->actions + InteractionList_Container, common->containerActions, common->containerActionCount);
    AddPossibleActions(assets, dest->actions + InteractionList_Equipped, common->equippedActions, common->equippedActionCount);
    AddPossibleActions(assets, dest->actions + InteractionList_Dragging, common->draggingActions, common->draggingActionCount);
    
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
            component->init(server, server->assets, componentPtr, ID, common, s, c);
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
            component->init(worldMode, worldMode->gameState->assets, componentPtr, ID, common, s, c);
        }
    }
}
#endif