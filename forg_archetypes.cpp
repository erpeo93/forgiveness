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

internal u16 ExistMetaPropertyValue(u16 propertyType, Token value);;
internal void AddPossibleActions(PossibleActionList* list, Enumerator* actions, u32 actionCount)
{
    for(u32 actionIndex = 0; actionIndex < actionCount; ++actionIndex)
    {
        if(list->actionCount < ArrayCount(list->actions))
        {
            list->actions[list->actionCount++] = ExistMetaPropertyValue(Property_action, Tokenize(actions[actionIndex].value));
        }
    }
}

#ifdef FORG_SERVER
INIT_COMPONENT_FUNCTION(InitPhysicComponent)
{
    PhysicComponent* physic = (PhysicComponent*) componentPtr;
    
    physic->P = s->P;
    physic->bounds = StandardBounds(common->boundDim, common->boundOffset);
    physic->definitionID = s->definitionID;
    physic->seed = s->seed;
    physic->speed = {};
    physic->acc = {};
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
    dest->maxStoredCount = 1;
    dest->maxUsingCount = ArrayCount(dest->usingIDs);
}

#else
INIT_COMPONENT_FUNCTION(InitBaseComponent)
{
    BaseComponent* base = (BaseComponent*) componentPtr;
    base->seed = c->seed;
    base->bounds = StandardBounds(common->boundDim, common->boundOffset);
    base->nameHash = StringHash(c->name.name);
    base->serverID = c->ID;
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

INIT_COMPONENT_FUNCTION(InitLayoutComponent)
{
    GameModeWorld* worldMode = (GameModeWorld*) state;
    Assets* assets = worldMode->gameState->assets;
    
    LayoutComponent* dest = (LayoutComponent*) componentPtr;
    InitShadow(&dest->shadow, c);
    dest->rootHash = StringHash(c->layoutRootName.name);
    dest->rootScale = V2(1, 1);
    dest->rootAngle = 0;
    
    
    for(u32 pieceIndex = 0; pieceIndex < c->pieceCount; ++pieceIndex)
    {
        LayoutPieceProperties* piece = c->layoutPieces + pieceIndex;
        if(dest->pieceCount < ArrayCount(dest->pieces))
        {
            LayoutPiece* destPiece = dest->pieces + dest->pieceCount++;
            InitImageReference_(assets, &destPiece->image, &piece->properties);
            destPiece->nameHash = StringHash(piece->name.name);
            destPiece->height = piece->height;
        }
    }
    
    for(u32 pieceIndex = 0; pieceIndex < c->openPieceCount; ++pieceIndex)
    {
        LayoutPieceProperties* piece = c->openLayoutPieces + pieceIndex;
        if(dest->openPieceCount < ArrayCount(dest->openPieces))
        {
            LayoutPiece* destPiece = dest->openPieces + dest->openPieceCount++;
            InitImageReference_(assets, &destPiece->image, &piece->properties);
            destPiece->nameHash = StringHash(piece->name.name);
            destPiece->height = piece->height;
        }
    }
    
    GameProperty property = GetProperty(c->seed);
    for(u32 pieceIndex = 0; pieceIndex < dest->pieceCount; ++pieceIndex)
    {
        LayoutPiece* destPiece = dest->pieces + pieceIndex;
        AddGameProperty_(&destPiece->image.properties, property, GameProperty_Optional);
    }
    for(u32 pieceIndex = 0; pieceIndex < dest->openPieceCount; ++pieceIndex)
    {
        LayoutPiece* destPiece = dest->openPieces + pieceIndex;
        AddGameProperty_(&destPiece->image.properties, property, GameProperty_Optional);
    }
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
}
#endif
INIT_COMPONENT_FUNCTION(InitInteractionComponent)
{
    InteractionComponent* dest = (InteractionComponent*) componentPtr;
    AddPossibleActions(dest->actions + Interaction_Ground, common->groundActions, common->groundActionCount);
    AddPossibleActions(dest->actions + Interaction_Equipment, common->equipmentActions, common->equipmentActionCount);
    AddPossibleActions(dest->actions + Interaction_Container, common->containerActions, common->containerActionCount);
    AddPossibleActions(dest->actions + Interaction_Equipped, common->equippedActions, common->equippedActionCount);
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