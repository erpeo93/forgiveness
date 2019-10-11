internal Rect3 StandardBounds(Vec3 dim, Vec3 offset)
{
    offset = {};
    Rect3 result = Offset(RectCenterDim(V3(0, 0, 0), dim), offset + V3(0, 0, 0.5f * dim.z));
    return result;
}

#ifdef FORG_SERVER
internal void InitPhysicComponent(PhysicComponent* physic, UniversePos P, Vec3 boundOffset, Vec3 boundDim, AssetID definitionID, u32 seed)
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

#else

internal void InitBaseComponent(BaseComponent* base, Vec3 boundOffset, Vec3 boundDim, u32 seed)
{
    base->seed = seed;
    base->bounds = StandardBounds(boundDim, boundOffset);
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
    InitBaseComponent(base, common->boundOffset, common->boundDim, params->seed);
    
    AnimationComponent* animation = GetComponent(worldMode, ID, AnimationComponent);
    animation->skeletonHash =StringHash(params->skeleton.value);
    animation->skinHash =StringHash(params->skin.value);
    animation->flipOnYAxis = 0;
}

INIT_ENTITY(RockArchetype)
{
    GameModeWorld* worldMode = (GameModeWorld*) state;
    ClientEntityInitParams* params = (ClientEntityInitParams*) par;
    CommonEntityInitParams* common = (CommonEntityInitParams*) com;
    
    Assets* assets = worldMode->gameState->assets;
    
    
    BaseComponent* base = GetComponent(worldMode, ID, BaseComponent);
    InitBaseComponent(base, common->boundOffset, common->boundDim, params->seed);
    
    RockComponent* dest = GetComponent(worldMode, ID, RockComponent);
    
    ImageComponent* image = GetComponent(worldMode, ID, ImageComponent);
    InitImageReference(assets, &image, &params, entity);
}


INIT_ENTITY(PlantArchetype)
{
    GameModeWorld* worldMode = (GameModeWorld*) state;
    ClientEntityInitParams* params = (ClientEntityInitParams*) par;
    CommonEntityInitParams* common = (CommonEntityInitParams*) com;
    
    Assets* assets = worldMode->gameState->assets;
    
    BaseComponent* base = GetComponent(worldMode, ID, BaseComponent);
    InitBaseComponent(base, common->boundOffset, common->boundDim, params->seed);
    
    PlantComponent* dest = GetComponent(worldMode, ID, PlantComponent);
    InitImageReference(assets, &dest, &params, leaf);
    
    ImageComponent* image = GetComponent(worldMode, ID, ImageComponent);
    InitImageReference(assets, &image, &params, entity);
}

INIT_ENTITY(GrassArchetype)
{
    GameModeWorld* worldMode = (GameModeWorld*) state;
    Assets* assets = worldMode->gameState->assets;
    ClientEntityInitParams* params = (ClientEntityInitParams*) par;
    
    CommonEntityInitParams* common = (CommonEntityInitParams*) com;
    
    BaseComponent* base = GetComponent(worldMode, ID, BaseComponent);
    InitBaseComponent(base, common->boundOffset, common->boundDim, params->seed);
    
    GrassComponent* dest = GetComponent(worldMode, ID, GrassComponent);
    
    ImageComponent* image = GetComponent(worldMode, ID, ImageComponent);
    InitImageReference(assets, &image, &params, entity);
}
#endif