#ifdef FORG_SERVER
INIT_ENTITY(FirstEntityArchetype)
{
    ServerState* server = (ServerState*) state;
    ServerEntityInitParams* params = (ServerEntityInitParams*) par;
    
    PhysicComponent* physic = GetComponent(server, ID, PhysicComponent);
    physic->P = params->P;
    physic->seed = params->seed;
    physic->speed = {};
    physic->acc = {};
    
}

INIT_ENTITY(SecondEntityArchetype)
{
    ServerState* server = (ServerState*) state;
    ServerEntityInitParams* params = (ServerEntityInitParams*) par;
    
    PhysicComponent* physic = GetComponent(server, ID, PhysicComponent);
    physic->P = params->P;
    physic->seed = params->seed;
    physic->speed = {};
    physic->acc = {};
    
}
#else
INIT_ENTITY(FirstEntityArchetype)
{
    GameModeWorld* worldMode = (GameModeWorld*) state;
    ClientEntityInitParams* params = (ClientEntityInitParams*) par;
    
    AnimationComponent* animation = GetComponent(worldMode, ID, AnimationComponent);
    animation->skeleton = ConvertEnumerator(AssetSkeletonType, params->skeleton);
    animation->skin = ConvertEnumerator(AssetImageType, params->skin);
}

INIT_ENTITY(SecondEntityArchetype)
{
    GameModeWorld* worldMode = (GameModeWorld*) state;
    ClientEntityInitParams* params = (ClientEntityInitParams*) par;
    
    AnimationComponent* animation = GetComponent(worldMode, ID, AnimationComponent);
    animation->skeleton = (AssetSkeletonType) AssetSkeleton_crocodile;
    animation->skin = (AssetImageType) AssetImage_crocodile;
}
#endif