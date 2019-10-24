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

#ifdef FORG_SERVER
internal void InitPhysicComponent(PhysicComponent* physic, UniversePos P, Vec3 boundOffset, Vec3 boundDim, EntityRef definitionID, u32 seed)
{
    physic->P = P;
    physic->bounds = StandardBounds(boundDim, boundOffset);
    physic->definitionID = definitionID;
    physic->seed = seed;
    physic->speed = {};
    physic->acc = {};
}


INIT_ENTITY(AnimalArchetype)
{
    ServerState* server = (ServerState*) state;
    
    ServerEntityInitParams* params = (ServerEntityInitParams*) par;
    CommonEntityInitParams* common = (CommonEntityInitParams*) com;
    
    PhysicComponent* physic = GetComponent(server, ID, PhysicComponent);
    InitPhysicComponent(physic, params->P, common->boundOffset, common->boundDim, params->definitionID, params->seed);
}

INIT_ENTITY(RockArchetype)
{
    ServerState* server = (ServerState*) state;
    ServerEntityInitParams* params = (ServerEntityInitParams*) par;
    CommonEntityInitParams* common = (CommonEntityInitParams*) com;
    
    PhysicComponent* physic = GetComponent(server, ID, PhysicComponent);
    InitPhysicComponent(physic, params->P, common->boundOffset, common->boundDim, params->definitionID, params->seed);
}

INIT_ENTITY(PlantArchetype)
{
    ServerState* server = (ServerState*) state;
    ServerEntityInitParams* params = (ServerEntityInitParams*) par;
    CommonEntityInitParams* common = (CommonEntityInitParams*) com;
    
    PhysicComponent* physic = GetComponent(server, ID, PhysicComponent);
    InitPhysicComponent(physic, params->P, common->boundOffset, common->boundDim, params->definitionID, params->seed);
}

INIT_ENTITY(GrassArchetype)
{
    ServerState* server = (ServerState*) state;
    ServerEntityInitParams* params = (ServerEntityInitParams*) par;
    CommonEntityInitParams* common = (CommonEntityInitParams*) com;
    
    PhysicComponent* physic = GetComponent(server, ID, PhysicComponent);
    InitPhysicComponent(physic, params->P, common->boundOffset, common->boundDim, params->definitionID, params->seed);
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

INIT_ENTITY(ObjectArchetype)
{
    ServerState* server = (ServerState*) state;
    ServerEntityInitParams* params = (ServerEntityInitParams*) par;
    CommonEntityInitParams* common = (CommonEntityInitParams*) com;
    
    PhysicComponent* physic = GetComponent(server, ID, PhysicComponent);
    InitPhysicComponent(physic, params->P, common->boundOffset, common->boundDim, params->definitionID, params->seed);
    
    EffectComponent* effect = GetComponent(server, ID, EffectComponent);
    GameProperty property = GetProperty(params->seed);
    AddRandomEffects(effect, params->bindings, params->bindingCount, property);
}

INIT_ENTITY(PortalArchetype)
{
    ServerState* server = (ServerState*) state;
    ServerEntityInitParams* params = (ServerEntityInitParams*) par;
    CommonEntityInitParams* common = (CommonEntityInitParams*) com;
    
    PhysicComponent* physic = GetComponent(server, ID, PhysicComponent);
    InitPhysicComponent(physic, params->P, common->boundOffset, common->boundDim, params->definitionID, params->seed);
    
    CollisionEffectsComponent* collision = GetComponent(server, ID, CollisionEffectsComponent);
    for(u32 effectIndex = 0; effectIndex < params->collisionEffectsCount; ++effectIndex)
    {
        if(collision->effectCount < ArrayCount(collision->effects))
        {
            GameEffect* dest = collision->effects + collision->effectCount++;
            *dest = params->collisionEffects[effectIndex];
        }
    }
}

#else

internal void InitBaseComponent(BaseComponent* base, Vec3 boundOffset, Vec3 boundDim, u32 seed, u64 nameHash)
{
    base->seed = seed;
    base->bounds = StandardBounds(boundDim, boundOffset);
    base->nameHash = nameHash;
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

INIT_ENTITY(AnimalArchetype)
{
    GameModeWorld* worldMode = (GameModeWorld*) state;
    ClientEntityInitParams* params = (ClientEntityInitParams*) par;
    CommonEntityInitParams* common = (CommonEntityInitParams*) com;
    
    BaseComponent* base = GetComponent(worldMode, ID, BaseComponent);
    InitBaseComponent(base, common->boundOffset, common->boundDim, params->seed, StringHash(params->name.name));
    
    AnimationComponent* animation = GetComponent(worldMode, ID, AnimationComponent);
    animation->skeletonHash =params->skeleton.subtypeHash;
    animation->skinHash =params->skin.subtypeHash;
    animation->flipOnYAxis = 0;
    
    InitShadow(&animation->shadow, params);
}

INIT_ENTITY(RockArchetype)
{
    GameModeWorld* worldMode = (GameModeWorld*) state;
    ClientEntityInitParams* params = (ClientEntityInitParams*) par;
    CommonEntityInitParams* common = (CommonEntityInitParams*) com;
    
    Assets* assets = worldMode->gameState->assets;
    
    
    BaseComponent* base = GetComponent(worldMode, ID, BaseComponent);
    InitBaseComponent(base, common->boundOffset, common->boundDim, params->seed, StringHash(params->name.name));
    
    RockComponent* dest = GetComponent(worldMode, ID, RockComponent);
    
    ImageComponent* image = GetComponent(worldMode, ID, ImageComponent);
    InitImageReference(assets, &image, &params, entity);
    
    InitShadow(&image->shadow, params);
}


INIT_ENTITY(PlantArchetype)
{
    GameModeWorld* worldMode = (GameModeWorld*) state;
    ClientEntityInitParams* params = (ClientEntityInitParams*) par;
    CommonEntityInitParams* common = (CommonEntityInitParams*) com;
    
    Assets* assets = worldMode->gameState->assets;
    
    BaseComponent* base = GetComponent(worldMode, ID, BaseComponent);
    InitBaseComponent(base, common->boundOffset, common->boundDim, params->seed, StringHash(params->name.name));
    
    PlantComponent* dest = GetComponent(worldMode, ID, PlantComponent);
    InitImageReference(assets, &dest, &params, leaf);
    
    ImageComponent* image = GetComponent(worldMode, ID, ImageComponent);
    InitImageReference(assets, &image, &params, entity);
    
    InitShadow(&image->shadow, params);
}

INIT_ENTITY(GrassArchetype)
{
    GameModeWorld* worldMode = (GameModeWorld*) state;
    Assets* assets = worldMode->gameState->assets;
    ClientEntityInitParams* params = (ClientEntityInitParams*) par;
    
    CommonEntityInitParams* common = (CommonEntityInitParams*) com;
    
    BaseComponent* base = GetComponent(worldMode, ID, BaseComponent);
    InitBaseComponent(base, common->boundOffset, common->boundDim, params->seed, StringHash(params->name.name));
    
    GrassComponent* dest = GetComponent(worldMode, ID, GrassComponent);
    
    ImageComponent* image = GetComponent(worldMode, ID, ImageComponent);
    InitImageReference(assets, &image, &params, entity);
    
    InitShadow(&image->shadow, params);
}

INIT_ENTITY(ObjectArchetype)
{
    GameModeWorld* worldMode = (GameModeWorld*) state;
    Assets* assets = worldMode->gameState->assets;
    ClientEntityInitParams* params = (ClientEntityInitParams*) par;
    CommonEntityInitParams* common = (CommonEntityInitParams*) com;
    
    BaseComponent* base = GetComponent(worldMode, ID, BaseComponent);
    InitBaseComponent(base, common->boundOffset, common->boundDim, params->seed, StringHash(params->name.name));
    
    LayoutComponent* dest = GetComponent(worldMode, ID, LayoutComponent);
    InitShadow(&dest->shadow, params);
    dest->rootHash = StringHash(params->layoutRootName.name);
    dest->rootScale = V2(1, 1);
    dest->rootAngle = 0;
    for(u32 pieceIndex = 0; pieceIndex < params->pieceCount; ++pieceIndex)
    {
        LayoutPieceProperties* piece = params->layoutPieces + pieceIndex;
        if(dest->pieceCount < ArrayCount(dest->pieces))
        {
            LayoutPiece* destPiece = dest->pieces + dest->pieceCount++;
            InitImageReference_(assets, &destPiece->image, &piece->properties);
            destPiece->nameHash = StringHash(piece->name.name);
            destPiece->height = piece->height;
        }
    }
    
    GameProperty property = GetProperty(params->seed);
    for(u32 pieceIndex = 0; pieceIndex < dest->pieceCount; ++pieceIndex)
    {
        LayoutPiece* destPiece = dest->pieces + pieceIndex;
        AddGameProperty_(&destPiece->image.properties, property, GameProperty_Optional);
    }
}

INIT_ENTITY(PortalArchetype)
{
    GameModeWorld* worldMode = (GameModeWorld*) state;
    ClientEntityInitParams* params = (ClientEntityInitParams*) par;
    CommonEntityInitParams* common = (CommonEntityInitParams*) com;
    
    Assets* assets = worldMode->gameState->assets;
    
    BaseComponent* base = GetComponent(worldMode, ID, BaseComponent);
    InitBaseComponent(base, common->boundOffset, common->boundDim, params->seed, StringHash(params->name.name));
    
    ImageComponent* image = GetComponent(worldMode, ID, ImageComponent);
    InitImageReference(assets, &image, &params, entity);
    
    InitShadow(&image->shadow, params);
}

#endif