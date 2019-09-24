internal void UpdateSeasonTimer(ServerState* server, r32 elapsedTime)
{
    // NOTE(Leonardo): advance server "status"
    server->seasonTime += elapsedTime;
    if(server->seasonTime >= SEASON_DURATION)
    {
        server->seasonTime = 0;
        server->season = (WorldSeason) ((server->season == Season_Count - 1) ? 0 : server->season + 1);
    }
    server->seasonLerp = Clamp01MapToRange(0.5f * SEASON_DURATION, server->seasonTime, SEASON_DURATION);
}

internal EntityID AddEntity(ServerState* server, UniversePos P, EntityArchetype type, PlayerComponent* player = 0)
{
    EntityID ID = {};
    Acquire(server, SafeTruncateToU16(type), (&ID));
    PhysicComponent* physic = GetComponent(server, ID, PhysicComponent);
    physic->P = P;
    physic->speed = {};
    physic->acc = {};
    
    ServerAnimationComponent* animation = GetComponent(server, ID, ServerAnimationComponent);
    animation->skeleton = AssetSkeleton_wolf;
    animation->skin = AssetImage_wolf;
    
    SetComponent(server, ID, PlayerComponent, player);
    
    return ID;
}

internal void MoveEntity(ServerState* server, PhysicComponent* physic, r32 elapsedTime)
{
    
    
    UniversePos oldP = physic->P;
    
    r32 accelerationCoeff = 27.0f;
    r32 drag = -7.8f;
    
    Vec3 acceleration = Normalize(physic->acc) *accelerationCoeff;
    Vec3 velocity = physic->speed;
    r32 dt = elapsedTime;
    
    acceleration.xy += drag * velocity.xy;
    
    physic->P.chunkOffset += 0.5f * Square(dt) * acceleration + velocity * dt;
    physic->speed += dt * acceleration;
    
    
    physic->P = NormalizePosition(physic->P);
    
    if(!PositionInsideWorld(&physic->P))
    {
        physic->P = oldP;
    }
}

internal void SendEntityUpdates(ServerState* server, EntityID ID, PhysicComponent* physic)
{
    unsigned char buff_[KiloBytes(2)];
    Assert(sizeof(EntityUpdate) < ArrayCount(buff_));
    
    u16 totalSize = PrepareEntityUpdate(server, physic, buff_);
    for(u16 archetypeIndex = 0; archetypeIndex < Archetype_Count; ++archetypeIndex)
    {
        if(HasComponent(archetypeIndex, PlayerComponent))
        {
            for(ArchIterator iter = First(server, archetypeIndex); 
                IsValid(iter); 
                iter = Next(iter))
            {
                PlayerComponent* player = GetComponent(server, iter.ID, PlayerComponent);
                if(player)
                {
                    SendEntityHeader(player, ID);
                    u8* writeHere = ForgReserveSpace(player, GuaranteedDelivery_None, 0, totalSize, ID);
                    Assert(writeHere);
                    if(writeHere)
                    {
                        Copy(totalSize, writeHere, buff_);
                    }
                    
                    if(HasComponent(archetypeIndex, ServerAnimationComponent))
                    {
                        ServerAnimationComponent* animationComponent = GetComponent(server, iter.ID, ServerAnimationComponent);
                        SendAnimationComponent(player, ID, animationComponent);
                    }
                }
            }
        }
    }
}


internal void HandlePlayersRequest(ServerState* server)
{
    for(u16 archetypeIndex = 0; archetypeIndex < Archetype_Count; ++archetypeIndex)
    {
        if(HasComponent(archetypeIndex, PlayerComponent))
        {
            for(ArchIterator iter = First(server, archetypeIndex); 
                IsValid(iter); 
                iter = Next(iter))
            {
                PlayerComponent* player = GetComponent(server, iter.ID, PlayerComponent);
                if(player)
                {
                    for(u32 requestIndex = 0; requestIndex < player->requestCount; ++requestIndex)
                    {
                        PlayerRequest* request = player->requests + requestIndex;
                        unsigned char* data = request->data;
                        ForgNetworkHeader header;
                        data = ForgUnpackHeader(data, &header);
                        switch(header.packetType)
                        {
                            case Type_ActionRequest:
                            {
                                Vec3 acc;
                                unpack(data, "V", &acc);
                                
                                if(HasComponent(archetypeIndex, PhysicComponent))
                                {
                                    PhysicComponent* physic = GetComponent(server, iter.ID, PhysicComponent);
                                    physic->acc = acc;
                                }
                            } break;
                            
                            InvalidDefaultCase;
                        }
                    }
                    player->requestCount = 0;
                }
            }
        }
    }
}


internal void MoveEntitiesAndSendUpdates(ServerState* server, r32 elapsedTime)
{
    for(u16 archetypeIndex = 0; archetypeIndex < Archetype_Count; ++archetypeIndex)
    {
        if(HasComponent(archetypeIndex, PhysicComponent))
        {
            for(ArchIterator iter = First(server, archetypeIndex); 
                IsValid(iter); 
                iter = Next(iter))
            {
                PhysicComponent* physic = GetComponent(server, iter.ID, PhysicComponent);
                MoveEntity(server, physic, elapsedTime);
                SendEntityUpdates(server, iter.ID, physic);
            }
        }
    }
}
