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

internal EntityID AddEntity(ServerState* server, UniversePos P, AssetID definitionID,PlayerComponent* player = 0)
{
    u32 seed = GetNextUInt32(&server->entropy);
    
    Assert(IsValid(definitionID));
    EntityDefinition* definition = GetData(server->assets, EntityDefinition, definitionID);
    
    EntityID result = {};
    ServerEntityInitParams params = definition->server;
    params.P = P;
    params.definitionID = definitionID;
    params.seed = seed;
    
    u16 archetype = SafeTruncateToU16(ConvertEnumerator(EntityArchetype, definition->archetype));
    Acquire(server, archetype, (&result));
    InitFunc[result.archetype](server, result, &params); 
    if(HasComponent(result.archetype, PlayerComponent))
    {
        SetComponent(server, result, PlayerComponent, player);
    }
    
    return result;
}

internal EntityID AddEntity(ServerState* server, UniversePos P, u32 type,PlayerComponent* player = 0)
{
    GameProperties properties = {};
    AddOptionalGameProperty(&properties, spawnEntityType, type);
    
    AssetID definitionID = QueryDataFiles(server->assets, EntityDefinition, 0, &server->entropy, &properties);
    
    EntityID result = AddEntity(server, P, definitionID, player);
    return result;
}

STANDARD_ECS_JOB_SERVER(HandlePlayerRequests)
{
    PlayerComponent* player = GetComponent(server, ID, PlayerComponent);
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
                    
                    if(HasComponent(ID.archetype, PhysicComponent))
                    {
                        PhysicComponent* physic = GetComponent(server, ID, PhysicComponent);
                        physic->acc = acc;
                    }
                } break;
                
                InvalidDefaultCase;
            }
        }
        player->requestCount = 0;
    }
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

internal void SendBasicUpdate(ServerState* server, EntityID ID, PhysicComponent* physic)
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
                    SendEntityHeader(player, physic->definitionID, ID, physic->seed);
                    u8* writeHere = ForgReserveSpace(player, GuaranteedDelivery_None, 0, totalSize, physic->definitionID, ID, physic->seed);
                    Assert(writeHere);
                    if(writeHere)
                    {
                        Copy(totalSize, writeHere, buff_);
                    }
                }
            }
        }
    }
}



internal void MoveAndSendUpdate(ServerState* server, EntityID ID, r32 elapsedTime)
{
    PhysicComponent* physic = GetComponent(server, ID, PhysicComponent);
    MoveEntity(server, physic, elapsedTime);
    SendBasicUpdate(server, ID, physic);
}