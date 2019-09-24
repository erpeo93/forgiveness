INIT_ENTITY(FirstEntityArchetype)
{
    ServerState* server = (ServerState*) state;
    ServerAnimationComponent* animation = GetComponent(server, ID, ServerAnimationComponent);
    animation->skeleton = (AssetSkeletonType) params->skeleton;
    animation->skin = (AssetImageType) params->skin;
}