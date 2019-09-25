INIT_ENTITY(FirstEntityArchetype)
{
    ServerState* server = (ServerState*) state;
    ServerAnimationComponent* animation = GetComponent(server, ID, ServerAnimationComponent);
    animation->skeleton = (AssetSkeletonType) params->skeleton;
    animation->skin = (AssetImageType) params->skin;
}

INIT_ENTITY(SecondEntityArchetype)
{
    ServerState* server = (ServerState*) state;
    ServerAnimationComponent* animation = GetComponent(server, ID, ServerAnimationComponent);
    animation->skeleton = (AssetSkeletonType) AssetSkeleton_crocodile;
    animation->skin = (AssetImageType) AssetImage_crocodile;
}