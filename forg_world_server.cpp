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

internal EntityID AddEntity_(ServerState* server, UniversePos P, AssetID definitionID,u32 seed, PlayerComponent* player = 0)
{
    Assert(IsValid(definitionID));
    EntityDefinition* definition = GetData(server->assets, EntityDefinition, definitionID);
    EntityID result = {};
    ServerEntityInitParams params = definition->server;
    params.P = P;
    params.definitionID = definitionID;
    params.seed = seed;
    
    u16 archetype = SafeTruncateToU16(ConvertEnumerator(EntityArchetype, definition->archetype));
    AcquireArchetype(server, archetype, (&result));
    InitFunc[result.archetype](server, result, &definition->common, &params); 
    if(HasComponent(result.archetype, PlayerComponent))
    {
        SetComponent(server, result, PlayerComponent, player);
    }
    return result;
}

internal EntityID AddEntity(ServerState* server, UniversePos P, RandomSequence* seq, GameProperties* definitionProperties, PlayerComponent* player = 0)
{
    AssetID definitionID = QueryDataFiles(server->assets, EntityDefinition, "default", seq, definitionProperties);
    
    u32 seed = GetNextUInt32(seq);
    EntityID result = AddEntity_(server, P, definitionID, seed, player);
    return result;
}

internal EntityID AddEntity(ServerState* server, UniversePos P, RandomSequence* seq, EntityRef type, PlayerComponent* player = 0)
{
    AssetID definitionID;
    definitionID.type = AssetType_EntityDefinition;
    definitionID.subtypeHashIndex = type.subtypeHashIndex;
    definitionID.index = type.index;
    
    u32 seed = GetNextUInt32(seq);
    EntityID result = AddEntity_(server, P, definitionID, seed, player);
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
                
                case Type_MoveChunkZ:
                {
                    if(HasComponent(ID.archetype, PhysicComponent))
                    {
                        PhysicComponent* physic = GetComponent(server, ID, PhysicComponent);
                        if(++physic->P.chunkZ == (i32) server->maxDeepness)
                        {
                            physic->P.chunkZ = 0;
                        }
                    }
                } break;
                InvalidDefaultCase;
            }
        }
        player->requestCount = 0;
    }
}

STANDARD_ECS_JOB_SERVER(FillPlayerSpacePartition)
{
    PlayerComponent* player = GetComponent(server, ID, PlayerComponent);
    if(player)
    {
        PhysicComponent* physic = GetComponent(server, ID, PhysicComponent);
        r32 maxDistance = 3.0f * CHUNK_DIM;
        Rect3 bounds = AddRadius(physic->bounds, V3(maxDistance, maxDistance, maxDistance));
        AddToSpatialPartition(server->frameByFramePool, &server->playerPartition, physic->P, bounds, ID);
    }
}

global_variable r32 g_maxDelta = 1.0f;
STANDARD_ECS_JOB_SERVER(FillCollisionSpatialPartition)
{
    PhysicComponent* physic = GetComponent(server, ID, PhysicComponent);
    Rect3 bounds = AddRadius(physic->bounds, V3(g_maxDelta, g_maxDelta, g_maxDelta));
    AddToSpatialPartition(server->frameByFramePool, &server->collisionPartition, physic->P, bounds, ID);
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
    
    Vec3 deltaP = 0.5f * acceleration * Square(dt) + velocity * dt;
    
    Assert(Abs(deltaP.x) <= g_maxDelta);
    Assert(Abs(deltaP.y) <= g_maxDelta);
    Assert(Abs(deltaP.z) <= g_maxDelta);
    
    physic->speed += acceleration * dt;
    
    
    r32 tRemaining = 1.0f;
    for( u32 iteration = 0; (iteration < 2) && tRemaining > 0; iteration++)
    {
        Vec3 wallNormalMin = {};
        r32 tStop = tRemaining;
        
        Rect3 bounds = AddRadius(physic->bounds, deltaP);
        SpatialPartitionQuery collisionQuery = QuerySpatialPartition(&server->collisionPartition, physic->P, bounds);
        
        for(EntityID testID = GetCurrent(&collisionQuery); IsValid(&collisionQuery); testID = Advance(&collisionQuery))
        {
            PhysicComponent* testPhysic = GetComponent(server, testID, PhysicComponent);
            if(testPhysic->P.chunkZ == physic->P.chunkZ)
            {
                Vec3 testP = SubtractOnSameZChunk(testPhysic->P, physic->P);
                HandleVolumeCollision(physic->bounds, deltaP, testP, testPhysic->bounds, &tStop, &wallNormalMin);
            }
        }
        
        Vec3 wallNormal = wallNormalMin;
        
        physic->P.chunkOffset += tStop * deltaP;
        
        physic->speed -= Dot(physic->speed, wallNormal) * wallNormal;
        deltaP -= Dot(deltaP, wallNormal) * wallNormal;
        tRemaining -= tStop;
    }
    
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
    
    r32 maxDistance = 3.0f * CHUNK_DIM;
    r32 maxDistanceSq = Square(maxDistance);
    
    Rect3 bounds = {};
    
    SpatialPartitionQuery playerQuery = QuerySpatialPartition(&server->playerPartition, physic->P, bounds);
    
    for(EntityID playerID = GetCurrent(&playerQuery); IsValid(&playerQuery); playerID = Advance(&playerQuery))
    {
        PlayerComponent* player = GetComponent(server, playerID, PlayerComponent);
        PhysicComponent* playerPhysic = GetComponent(server, playerID, PhysicComponent);
        
        if(physic->P.chunkZ == playerPhysic->P.chunkZ)
        {
            Vec3 distance = SubtractOnSameZChunk(physic->P, playerPhysic->P);
            if(LengthSq(distance) < maxDistanceSq)
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


#define GameProp(property, value) GameProp_(Property_##property, value)
internal GameProperty GameProp_(u16 property, u16 value)
{
    GameProperty result;
    result.property = property;
    result.value = value;
    
    return result;
}

internal void MoveAndSendUpdate(ServerState* server, EntityID ID, r32 elapsedTime)
{
    PhysicComponent* physic = GetComponent(server, ID, PhysicComponent);
    MoveEntity(server, physic, elapsedTime);
    
    physic->action = {};
    if(LengthSq(physic->speed) > 0.1f)
    {
        physic->action = GameProp(action, move);
    }
    else
    {
        physic->action = GameProp(action, idle);
    }
    
    
    SendBasicUpdate(server, ID, physic);
}


internal UniversePos BuildPFromSpawnerGrid(r32 cellDim, u32 cellX, u32 cellY, i32 chunkZ)
{
    UniversePos result = {};
    result.chunkZ = chunkZ;
    result.chunkOffset.x = cellDim * cellX;
    result.chunkOffset.y = cellDim * cellY;
    
    result = NormalizePosition(result);
    return result;
}

struct PoissonP
{
    UniversePos P;
    PoissonP* next;
};

internal b32 Valid(PoissonP* positions, UniversePos P, r32 maxDelta)
{
    b32 result = true;
    
    r32 maxDistanceSq = Square(maxDelta);
    for(PoissonP* poisson = positions; poisson; poisson = poisson->next)
    {
        Vec3 delta = SubtractOnSameZChunk(P, poisson->P);
        if(LengthSq(delta) < maxDistanceSq)
        {
            result = false;
            break;
        }
    }
    
    return result;
}

internal void AddToPoission(PoissonP** positions, MemoryPool* pool, UniversePos P)
{
    PoissonP* newP = PushStruct(pool, PoissonP);
    newP->P = P;
    newP->next = *positions;
    *positions = newP;
}

internal void TriggerSpawner(ServerState* server, Spawner* spawner, UniversePos referenceP, RandomSequence* seq)
{
    if(spawner->optionCount)
    {
        r32 totalWeight = 0;
        for(u32 optionIndex = 0; optionIndex < spawner->optionCount; ++optionIndex)
        {
            totalWeight += spawner->options[optionIndex].weight;
        }
        r32 weight = totalWeight * RandomUni(seq);
        
        
        SpawnerOption* option = 0;
        
        r32 runningWeight = 0;
        for(u32 optionIndex = 0; optionIndex < spawner->optionCount; ++optionIndex)
        {
            SpawnerOption* test = spawner->options + optionIndex;
            runningWeight += test->weight;
            if(weight <= runningWeight)
            {
                option = test;
                break;
            }
        }
        
        PoissonP* entities = 0;
        PoissonP* clusters = 0;
        
        MemoryPool tempPool = {};
        for(u32 entityIndex = 0; entityIndex < option->entityCount; ++entityIndex)
        {
            SpawnerEntity* spawn = option->entities + entityIndex;
            i32 clusterCount = spawn->clusterCount + RoundReal32ToI32(spawn->clusterCountV * RandomBil(seq));
            
            for(i32 clusterIndex = 0; clusterIndex < clusterCount; ++clusterIndex)
            {
                UniversePos P = referenceP;
                Vec3 maxClusterOffset = V3(VOXEL_SIZE * spawn->maxClusterOffset, 0);
                
                b32 valid = false;
                u32 tries = 0;
                while(!valid && tries++ < 100)
                {
                    P = referenceP;
                    P.chunkOffset += Hadamart(maxClusterOffset, RandomBilV3(seq));
                    P = NormalizePosition(P);
                    if(Valid(clusters, P, spawn->minClusterDistance))
                    {
                        valid = true;
                        AddToPoission(&clusters, &tempPool, P);
                    }
                }
                
                if(valid)
                {
                    UniversePos clusterP = P;
                    i32 count = spawn->count + RoundReal32ToI32(spawn->countV * RandomBil(seq));
                    for(i32 index = 0; index < count; ++index)
                    {
                        Vec3 maxOffset = V3(VOXEL_SIZE * spawn->maxOffset, 0);
                        
                        valid = false;
                        tries = 0;
                        while(!valid && tries++ < 100)
                        {
                            P = clusterP;
                            P.chunkOffset += Hadamart(maxOffset, RandomBilV3(seq));
                            P = NormalizePosition(P);
                            
                            if(Valid(entities, P, spawn->minEntityDistance))
                            {
                                AddToPoission(&entities, &tempPool, P);
                                valid = true;
                            }
                        }
                        
                        
                        if(valid)
                        {
                            AddEntity(server, P, seq, spawn->type, 0);
                        }
                    }
                }
            }
        }
        
        Clear(&tempPool);
    }
}

internal void Craft()
{
    PickRandomEffects();
    Pick Essences based on seed();
    
}

internal void DispatchGameEffect(ServerState* server, Vec3 P, GameEffect* effect)
{
    switch(effect->type)
    {
        case spawnAnimal:
        {
            
        } break;
        
        case spawnGrass:
        {
            
        } break;
        
        case spawnRock:
        {
            
        } break;
    }
}

internal void SpawnEntities(ServerState* server, r32 elapsedTime)
{
    MemoryPool tempPool = {};
    u16 spawnerCount;
    AssetID* spawners = GetAllDataAsset(&tempPool, server->assets, Spawner, "default", 0, &spawnerCount);
    for(u32 spawnerIndex = 0; spawnerIndex < spawnerCount; ++spawnerIndex)
    {
        AssetID sID = spawners[spawnerIndex];
        Spawner* spawner = GetData(server->assets, Spawner, sID);
        if(spawner->targetTime > 0)
        {
            spawner->time += elapsedTime;
            if(spawner->time >= spawner->targetTime)
            {
                spawner->time = 0;
                r32 cellDim = VOXEL_SIZE * spawner->cellDim;
                u32 cellCount = TruncateReal32ToU32(WORLD_SIDE / cellDim);
                
                u32 cellX = RandomChoice(&server->entropy, cellCount);
                u32 cellY = RandomChoice(&server->entropy, cellCount);
                i32 Z = 0;
                UniversePos P = BuildPFromSpawnerGrid(cellDim, cellX, cellY, Z);
                TriggerSpawner(server, spawner, P, &server->entropy);
            }
        }
    }
    
    Clear(&tempPool);
}

internal void BuildWorld(ServerState* server, b32 spawnEntities)
{
    
    RandomSequence generatorSeq = Seed(server->worldSeed);
    GameProperties properties = {};
    AssetID ID = QueryDataFiles(server->assets, world_generator, "default", &generatorSeq, &properties);
    if(IsValid(ID))
    {
        world_generator* generator = GetData(server->assets, world_generator, ID);
        server->nullTile = NullTile(generator);
        
        server->maxDeepness = generator->maxDeepness;
        
        Clear(&server->chunkPool);
        server->chunks = PushArray(&server->chunkPool, WorldChunk, generator->maxDeepness * WORLD_CHUNK_SPAN * WORLD_CHUNK_SPAN);
        WorldChunk* chunk = server->chunks;
        for(u32 chunkZ = 0; chunkZ < generator->maxDeepness; ++chunkZ)
        {
            for(u32 chunkY = 0; chunkY < WORLD_CHUNK_SPAN; ++chunkY)
            {
                for(u32 chunkX = 0; chunkX < WORLD_CHUNK_SPAN; ++chunkX)
                {
                    BuildChunk(server->assets, &server->chunkPool, generator,
                               chunk, chunkX, chunkY, chunkZ, server->worldSeed, 0);
                    ++chunk;
                }
            }
        }
    }
    else
    {
        InvalidCodePath;
    }
    
    RandomSequence seq = Seed(server->worldSeed);
    if(spawnEntities)
    {
        MemoryPool tempPool = {};
        u16 spawnerCount;
        AssetID* spawners = GetAllDataAsset(&tempPool, server->assets, Spawner, "default", 0, &spawnerCount);
        
        for(u32 spawnerIndex = 0; spawnerIndex < spawnerCount; ++spawnerIndex)
        {
            AssetID sID = spawners[spawnerIndex];
            Spawner* spawner = GetData(server->assets, Spawner, sID);
            r32 cellDim = VOXEL_SIZE * spawner->cellDim;
            u32 cellCount = TruncateReal32ToU32(WORLD_SIDE / cellDim);
            for(i32 chunkZ = 0; chunkZ < (i32) server->maxDeepness; ++chunkZ)
            {
                for(u32 Y = 0; Y < cellCount; ++Y)
                {
                    for(u32 X = 0; X < cellCount; ++X)
                    {
                        if(RandomUni(&seq) <= spawner->percentageOfStartingCells)
                        {
                            UniversePos P = BuildPFromSpawnerGrid(cellDim, X, Y, chunkZ);
                            TriggerSpawner(server, spawner, P, &seq);
                        }
                    }
                }
            }
        }
        
        Clear(&tempPool);
    }
}

STANDARD_ECS_JOB_SERVER(DeleteAllEntities)
{
    FreeArchetype(server, &ID);
}
