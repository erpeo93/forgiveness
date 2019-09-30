#ifdef FORG_SERVER
INIT_ENTITY(AnimalArchetype)
{
    ServerState* server = (ServerState*) state;
    ServerEntityInitParams* params = (ServerEntityInitParams*) par;
    
    PhysicComponent* physic = GetComponent(server, ID, PhysicComponent);
    physic->P = params->P;
    physic->definitionID = params->definitionID;
    physic->seed = params->seed;
    physic->speed = {};
    physic->acc = {};
    
}

INIT_ENTITY(RockArchetype)
{
    ServerState* server = (ServerState*) state;
    ServerEntityInitParams* params = (ServerEntityInitParams*) par;
    
    PhysicComponent* physic = GetComponent(server, ID, PhysicComponent);
    physic->P = params->P;
    physic->definitionID = params->definitionID;
    physic->seed = params->seed;
    physic->speed = {};
    physic->acc = {};
    
}
#else
INIT_ENTITY(AnimalArchetype)
{
    GameModeWorld* worldMode = (GameModeWorld*) state;
    ClientEntityInitParams* params = (ClientEntityInitParams*) par;
    
    AnimationComponent* animation = GetComponent(worldMode, ID, AnimationComponent);
    animation->skeleton = ConvertEnumerator(AssetSkeletonType, params->skeleton);
    animation->skin = ConvertEnumerator(AssetImageType, params->skin);
}

INIT_ENTITY(RockArchetype)
{
    GameModeWorld* worldMode = (GameModeWorld*) state;
    ClientEntityInitParams* params = (ClientEntityInitParams*) par;
    
    Assets* assets = worldMode->gameState->assets;
    
    RockComponent* dest = GetComponent(worldMode, ID, RockComponent);
    
    MemoryPool tempPool = {};
    RandomSequence seq = Seed(1234);
    AssetID model = QueryModels(assets, AssetModel_default, &seq, 0);
    AssetID rock = QueryDataFiles(assets, RockDefinition, 0, &seq, 0);
    if(IsValid(model) && IsValid(rock))
    {
        LoadModel(assets, model, true);
        VertexModel* m = GetModel(assets, model);
        RockDefinition* r = GetData(assets, RockDefinition, rock);
        GenerateRock(dest, m, &tempPool, &seq, r);
    }
    Clear(&tempPool);
}
#endif