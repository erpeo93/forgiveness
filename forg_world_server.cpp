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

internal u32 AddEntity(ServerState* server, UniversePos P, u32 playerID)
{
    Assert(server->entityCount < ArrayCount(server->entities));
    u32 result = server->entityCount++;
    Entity* entity = server->entities + result;
    entity->P = P;
    entity->speed = {};
    entity->acc = {};
    entity->ID = result;
    entity->playerID = playerID;
    
    return result;
}

internal void MoveEntity(ServerState* server, Entity* entity, r32 elapsedTime)
{
    
    
    UniversePos oldP = entity->P;
    
    r32 accelerationCoeff = 27.0f;
    r32 drag = -7.8f;
    
    Vec3 acceleration = Normalize(entity->acc) *accelerationCoeff;
    Vec3 velocity = entity->speed;
    r32 dt = elapsedTime;
    
    acceleration.xy += drag * velocity.xy;
    
    entity->P.chunkOffset += 0.5f * Square(dt) * acceleration + velocity * dt;
    entity->speed += dt * acceleration;
    
    
    entity->P = NormalizePosition(entity->P);
    
    if(!PositionInsideWorld(&entity->P))
    {
        entity->P = oldP;
    }
}

internal void SendEntityUpdate(ServerState* server, Entity* entity)
{
    unsigned char buff_[KiloBytes(2)];
    Assert(sizeof(EntityUpdate) < ArrayCount(buff_));
    u16 totalSize = PrepareEntityUpdate(server, entity, buff_);
    for(u32 entityIndex = 0; entityIndex < server->entityCount; ++entityIndex)
    {
        Entity* targetEntity = server->entities + entityIndex;
        if(targetEntity->playerID)
        {
            Player* player = server->players + targetEntity->playerID;
            SendEntityHeader(player, entity->ID);
            u8* writeHere = ForgReserveSpace(player, false, totalSize, entity->ID);
            Assert(writeHere);
            if(writeHere)
            {
                Copy(totalSize, writeHere, buff_);
            }
            
        }
    }
}


internal void HandlePlayersRequest(ServerState* server)
{
    for(u32 entityIndex = 0; entityIndex < server->entityCount; ++entityIndex)
    {
        Entity* entity = server->entities + entityIndex;
        if(entity->ID)
        {
            Player* player = server->players + entity->playerID;
            
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
                        Assert(entity->playerID);
                        Vec3 acc;
                        unpack(data, "V", &acc);
                        entity->acc = acc;
                    } break;
                    
                    InvalidDefaultCase;
                }
            }
            
            player->requestCount = 0;
        }
    }
}


internal void MoveEntitiesAndSendUpdates(ServerState* server, r32 elapsedTime)
{
    for(u32 entityIndex = 0; entityIndex < server->entityCount; ++entityIndex)
    {
        Entity* entity = server->entities + entityIndex;
        if(entity->ID)
        {
            MoveEntity(server, entity, elapsedTime);
            SendEntityUpdate(server, entity);
        }
    }
}
