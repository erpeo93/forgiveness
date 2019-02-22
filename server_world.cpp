inline u64 GetIdentifier( ServerState* server )
{
    u64 result = AtomicIncrementU64( &server->currentIdentifier, 1 );
    return result;
}

internal void PackEntityIntoChunk(SimRegion* region, SimEntity* entity)
{
    UniversePos newP = GetUniverseP(region, entity->P);
    
    ServerState* server = region->server;
    Assert(ChunkValid(server->lateralChunkSpan, newP.chunkX, newP.chunkY));
    WorldChunk* chunk = GetChunk(server->chunks, ArrayCount(server->chunks), newP.chunkX, newP.chunkY, &server->worldPool);
    
    Assert(chunk->initialized);
    Assert(chunk->worldX == newP.chunkX);
    Assert(chunk->worldY == newP.chunkY);
    
    
    entity->P = newP.chunkOffset;
    EntityBlock* block = chunk->entities;
    
    u32 desiredSize = sizeof(SimEntity);
    if(!block || (block->usedData + desiredSize > ArrayCount(block->data)))
    {
        FREELIST_ALLOC(block, region->context->firstFreeBlock, PushStruct(&region->context->pool, EntityBlock));
        FREELIST_INSERT(block, chunk->entities);
        block->countEntity = 0;
    }
    
    Assert(desiredSize <= ArrayCount( block->data));
    SimEntity* blockEntity = (SimEntity*) (block->data + block->usedData);
    *blockEntity = *entity;
    
    block->usedData += desiredSize;
    ++block->countEntity;
}

inline void NORUNTIMEAddSkill(SimRegion* region, SimEntity* entity, char* skillName, u32 level, r32 power)
{
    CreatureComponent* creature = Creature(region, entity);
    Assert(creature->skillCount < ArrayCount(creature->skills));
    SkillSlot* slot = creature->skills + creature->skillCount++;
    slot->taxonomy = NORUNTIMEGetTaxonomySlotByName(region->taxTable, skillName)->taxonomy;
    slot->level = level;
    slot->power = power;
    
    
    ServerPlayer* player = region->server->players + entity->playerID;
    u32 parentTaxonomy = GetParentTaxonomy(region->taxTable, slot->taxonomy);
    while(parentTaxonomy)
    {
        b32 alreadyPresent = false;
        for(u32 skillCategoryIndex = 0; skillCategoryIndex < player->unlockedCategoryCount; ++skillCategoryIndex)
        {
            if(player->unlockedSkillCategories[skillCategoryIndex] == parentTaxonomy)
            {
                alreadyPresent = true;
                break;
            }
        }
        
        if(!alreadyPresent)
        {
            Assert(player->unlockedCategoryCount < ArrayCount(player->unlockedSkillCategories));
            player->unlockedSkillCategories[player->unlockedCategoryCount++] = parentTaxonomy;
        }
        parentTaxonomy = GetParentTaxonomy(region->taxTable, parentTaxonomy);
    }
}

internal void SendSkills(SimRegion* region, SimEntity* entity, ServerPlayer* player, TaxonomyTable* table);
internal void AddPlayersSkills(SimRegion* region, SimEntity* player)
{
    NORUNTIMEAddSkill(region, player, "elemental 1", 2, 1.0f);
    NORUNTIMEAddSkill(region, player, "elemental 4", 2, 1.0f);
    NORUNTIMEAddSkill(region, player, "elemental 2", 1, 1.0f);
    NORUNTIMEAddSkill(region, player, "elemental 3", 3, 1.0f);
}


inline Vec3 DroppingVelocity(RandomSequence* seq)
{
    Vec3 result = V3(Hadamart(RandomBilV2(seq), V2(20, 20)), 20.0f + RandomBil(seq) * 5.0f);
    return result;
}

inline u8 GetMaxObjectCount(TaxonomyTable* table, u32 taxonomy)
{
    u8 result = 0;
    TaxonomySlot* slot = GetSlotForTaxonomy(table, taxonomy);
    while(slot->taxonomy)
    {
        if(slot->gridDimX)
        {
            Assert(slot->gridDimY);
            result = slot->gridDimX * slot->gridDimY;
        }
        
        slot = GetParentSlot(table, slot);
    }
    
    return result;
}


inline void InitPlayerEntity(SimRegion* region, ServerPlayer* player, SimEntity* entity)
{
    player->requestCount = 0;
    player->ignoredActionCount = 0;
    player->unlockedCategoryCount = 0;
    player->recipeCount = 0;
    
    TaxonomyTable* taxTable = region->taxTable;
    AddPlayersSkills(region, entity);
    SendSkills(region, entity, player, taxTable);
    SendAvailableRecipes(player, taxTable);
    CreatureComponent* creature = Creature(region, entity);
    creature->activeSkillIndex = -1;
}

internal void AddEntitySingleThread( SimRegion* region, u32 taxonomy, Vec3 P, u64 identifier, AddEntityAdditionalParams params)
{
    ServerState* server = region->server;
    UniversePos universeP = GetUniverseP(region, P);
    if(PositionInsideWorld(server->lateralChunkSpan, &universeP))
    {
        TaxonomyTable* taxTable = region->taxTable;
        RegionWorkContext* context = region->context;
        SimEntity entity = {};
        
        entity.P = P;
        entity.identifier = identifier;
        entity.playerID = params.playerID;
        entity.quantity = params.quantity;
        entity.status = params.status;
        
        entity.IDs[Component_Effect] = GetFreeComponent(server->components, Component_Effect);
        
        if(IsPlant(taxTable, taxonomy))
        {
            entity.IDs[Component_Plant] = GetFreeComponent(server->components, Component_Plant);
        }
        else if(IsObject(taxTable, taxonomy))
        {
            entity.IDs[Component_Object] = GetFreeComponent(server->components, Component_Object);
        }
        else if(IsFluid(taxTable, taxonomy))
        {
            entity.IDs[Component_Fluid] = GetFreeComponent(server->components, Component_Fluid);
        }
        else if(IsCreature(taxTable, taxonomy))
        {
            entity.IDs[Component_Creature] = GetFreeComponent(server->components, Component_Creature);
        }
        
        if(params.dropped)
        {
            entity.velocity = DroppingVelocity(&region->entropy);
        }
        
        if(entity.playerID)
        {
            ServerPlayer* player = region->server->players + params.playerID;
            InitPlayerEntity(region, player, &entity);
        }
        
        if(params.recipeTaxonomy)
        {
            entity.recipeTaxonomy = params.recipeTaxonomy;
            entity.recipeIndex = params.recipeIndex;
        }
        
        
        TaxonomySlot* slot_ = GetSlotForTaxonomy( taxTable, taxonomy );
        Assert(!slot_->subTaxonomiesCount);
        
        entity.taxonomy = taxonomy;
        entity.recipeIndex = params.recipeIndex;
        
        u32 currentTaxonomy = 0;
        
        
        u8 maxObjectCount = GetMaxObjectCount(taxTable, taxonomy);
        
        if(maxObjectCount)
        {
            ObjectComponent* object = Object(region, &entity);
            object->objects.maxObjectCount = maxObjectCount;
        }
        
        b32 running = true;
        while( running )
        {
            TaxonomySlot* attributeSlot = GetSlotForTaxonomy(taxTable, currentTaxonomy);
            Assert(attributeSlot);
            
            if(currentTaxonomy == taxonomy)
            {
                running = false;
            }
            
            if(IsCreature(taxTable, entity.taxonomy))
            {
                CreatureComponent* creature = Creature(region, &entity);
                for(u32 attributeIndex = 0; attributeIndex < ArrayCount(attributeSlot->attributeHashmap); ++attributeIndex)
                {
                    AttributeSlot* attr = attributeSlot->attributeHashmap + attributeIndex;
                    if( attr->offsetFromBase )
                    {
                        * ((r32*) ((u8*) creature + attr->offsetFromBase)) = attr->valueR32;
                    }
                }
            }
            
            
            EffectComponent* effects = Effects(region, &entity);
            for(TaxonomyEffect* effectSlot = slot_->firstEffect; effectSlot; effectSlot = effectSlot->next)
            {
                Effect* effect = &effectSlot->effect;
                
                b32 addEffect = true;
                for(u32 entityEffectIndex = 0; entityEffectIndex < effects->effectCount; ++entityEffectIndex)
                {
                    Effect* entityEffect = effects->effects + entityEffectIndex;
                    if(entityEffect->ID == effect->ID)
                    {
                        *entityEffect = *effect;
                        addEffect = false;
                        break;
                    }
                }
                
                if(addEffect)
                {
                    Effect* dest = effects->effects + effects->effectCount++;
                    *dest = *effect;
                }
            }
            currentTaxonomy = GetChildTaxonomy( attributeSlot, taxonomy );
        }
        
        if(params.objectCount)
        {
            ObjectComponent* object = Object(region, &entity);
            ContainedObjects* objects = &object->objects;
            Assert(params.objectCount <= objects->maxObjectCount);
            objects->objectCount = params.objectCount;
            
            for(u8 objectIndex = 0; objectIndex < params.objectCount; ++objectIndex)
            {
                objects->objects[objectIndex] = params.objects[objectIndex];
            }
        }
        
        
        
        if(params.ownerID)
        {
            entity.ownerID = params.ownerID;
            if(params.equipped)
            {
                AddFlags(&entity, Flag_Equipped);
            }
        }
        
        
        
#if 0    
        if(entity.fluid->fluid.type)
        {
            Fluid* fluid = &entity.fluid->fluid;
            fluid->direction = params.rotation;
            fluid->source1 = params.fluidId1;
            fluid->sourceSegmentIndex1 = params.segmentIndex1;
            fluid->source2 = params.fluidId2;
            fluid->sourceSegmentIndex2 = params.segmentIndex2;
            
            for( u32 rayIndex = 0; rayIndex < ArrayCount( fluid->rays ); ++rayIndex )
            {
                fluid->rays[rayIndex].lengthPercentage = 1.0f;
            }
        }
#endif
        
        u32 startingBehaviorTaxonomy = taxonomy;
        while(startingBehaviorTaxonomy)
        {
            TaxonomySlot* behaviorSlot = GetSlotForTaxonomy(taxTable, startingBehaviorTaxonomy);
            if(behaviorSlot->startingBehavior)
            {
                CreatureComponent* creature = Creature(region, &entity);
                TaxonomyBehavior* defaultBehavior = behaviorSlot->startingBehavior;
                PushAIBehavior(taxTable, &creature->brain, defaultBehavior->specificTaxonomy);
                creature->brain.valid = true;
                
                Mem* memory = &creature->brain.memory;
                memory->shortTerm.firstIndex = 1;
                memory->shortTerm.onePastLastIndex = ASSOCIATION_COUNT;
                memory->shortTerm.secondsBetweenUpdates = 10;
                memory->shortTerm.decadenceFactor = 1.0f;
                break;
            }
            
            startingBehaviorTaxonomy = GetParentTaxonomy(behaviorSlot);
        }
        
        
        GetPhysicalProperties(taxTable, slot_->taxonomy, entity.identifier, &entity.boundType, &entity.bounds);
        
        if(entity.IDs[Component_Creature])
        {
            CreatureComponent* creature = Creature(region, &entity);
            creature->lifePoints = creature->maxLifePoints;
        }
        
        if(slot_->firstComponent)
        {
            Craft(region, &entity, entity.taxonomy, entity.recipeIndex);
        }
        PackEntityIntoChunk( region, &entity );
    }
}

inline u64 AddEntity(SimRegion* region, Vec3 P, u32 taxonomy, u64 recipeIndex = 0, AddEntityAdditionalParams params = DefaultAddEntityParams())
{
	u64 result = 0;
	if(region->border != Border_Mirror)
	{
		ServerState* server = region->server;
		Assert(server->newEntityCount < ArrayCount(server->newEntities));
		u32 index = AtomicIncrementU32(&server->newEntityCount, 1);
		Assert(index < ArrayCount(server->newEntities));
        
		NewEntity* newEntity = server->newEntities + index;
		newEntity->region = region;
		newEntity->taxonomy = taxonomy;
		newEntity->recipeIndex = recipeIndex;
		newEntity->P = P;
		newEntity->identifier = GetIdentifier(server);
		newEntity->params = params;
        
		result = newEntity->identifier;
	}
    
	return result;
}

inline void DeleteEntityComponents(SimRegion* region, SimEntity* entity)
{
    ServerState* server = region->server;
    Assert(server->deletedEntityCount < ArrayCount(server->deletedEntities));
    u32 index = AtomicIncrementU32(&server->deletedEntityCount, 1);
    Assert(index < ArrayCount(server->deletedEntities));
    
    DeletedEntity* deletedEntity = server->deletedEntities + index;
    for(u32 componentIndex = 0; componentIndex < Component_Count; ++componentIndex)
    {
        deletedEntity->IDs[componentIndex] = entity->IDs[componentIndex];
    }
}


inline u64 AddRandomEntity(SimRegion* region, RandomSequence* sequence, Vec3 P, u32 taxonomy, AddEntityAdditionalParams params = DefaultAddEntityParams(), b32 immediate = false)
{
    TaxonomyTable* table = region->taxTable;
    u32 finalTaxonomy = GetRandomChild(table, sequence, taxonomy);
    u64 recipeIndex = 0;
    
    u64 result;
    if(region->context->immediateSpawn)
    {
        result = GetIdentifier(region->server);
        AddEntitySingleThread( region, finalTaxonomy, P, result, params);
    }
    else
    {
        
        result = AddEntity(region, P, finalTaxonomy, recipeIndex, params);
    }
    return result;
}

internal void BuildSimpleTestWorld(ServerState* server)
{
#define GENERATE_ENEMIES 0
#define GENERATE_ENVIRONMENT 0
#define GENERATE_OBJECTS 0
    
    
    RegionWorkContext* context = server->threadContext + 0;
    context->immediateSpawn = true;
    
    for(u32 regionY = 0; regionY < SERVER_REGION_SPAN; ++regionY)
    {
        for(u32 regionX = 0; regionX < SERVER_REGION_SPAN; ++regionX)
        {
            SimRegion* region = GetServerRegion( server, regionX, regionY);
            region->context = context;
            
            TaxonomyTable* taxTable = server->activeTable;
            Vec3 P = V3(0, 0, 0);
            
            TaxonomySlot* testSlot = 0;
            
            r32 regionSpan = VOXEL_SIZE * CHUNK_DIM * SIM_REGION_CHUNK_SPAN;
            r32 hrs = 0.45f * regionSpan;
            RandomSequence grassSeq = Seed(regionX * regionY + (1 << regionX));
            
#if GENERATE_ENEMIES
            u32 goblinCount = 10;
            
            for(u32 goblinIndex = 0; goblinIndex < goblinCount; ++goblinIndex)
            {
                r32 offsetX = RandomRangeFloat( &grassSeq, -hrs, hrs);
                r32 offsetY = RandomRangeFloat( &grassSeq, -hrs, hrs);
                
                testSlot = NORUNTIMEGetTaxonomySlotByName(taxTable, "wolf");
                AddRandomEntity(region, &region->server->randomSequence, P + V3(offsetX, offsetY, 0.0f), testSlot->taxonomy);
            }
            
            for(u32 goblinIndex = 0; goblinIndex < goblinCount; ++goblinIndex)
            {
                r32 offsetX = RandomRangeFloat( &grassSeq, -hrs, hrs);
                r32 offsetY = RandomRangeFloat( &grassSeq, -hrs, hrs);
                
                testSlot = NORUNTIMEGetTaxonomySlotByName(taxTable, "warriorgoblin");
                AddRandomEntity(region, &region->server->randomSequence, P + V3(offsetX, offsetY, 0.0f), testSlot->taxonomy);
            }
#endif
            
#if GENERATE_ENVIRONMENT
            
#if FORGIVENESS_INTERNAL
            u32 treeCount = 2;
            u32 grassCount = 2;
            u32 rockCount = 2;
#else
            u32 treeCount = 200;
            u32 grassCount = 200;
            u32 rockCount = 200;
#endif
            
            testSlot = NORUNTIMEGetTaxonomySlotByName( taxTable, "pine" );
            AddRandomEntity(region, &region->server->randomSequence, P, testSlot->taxonomy);
            for( u32 index = 0; index < treeCount; ++index )
            {
                r32 offsetX = RandomRangeFloat( &grassSeq, -hrs, hrs);
                r32 offsetY = RandomRangeFloat( &grassSeq, -hrs, hrs);
                Vec3 pineP = P + V3( offsetX, offsetY, 0 );
                AddRandomEntity(region, &region->server->randomSequence, pineP, testSlot->taxonomy);
            }
            
            
            testSlot = NORUNTIMEGetTaxonomySlotByName( taxTable, "campfire" );
            Vec3 fireP = P + V3( -2.0f, 1.5f, 0.0f );
            
            for(u32 campFireIndex = 0; campFireIndex < 3; ++campFireIndex)
            {
                Vec3 campFireP = fireP + V3(2.0f * campFireIndex, 0, 0);
                AddRandomEntity(region, &region->server->randomSequence, campFireP, testSlot->taxonomy, FluidDirection(Identity(), 1, 0, 2, 0));
            }
            
            testSlot = NORUNTIMEGetTaxonomySlotByName( taxTable, "testGrass" );
            grassSeq = Seed(0);
            for( u32 index = 0; index < grassCount; ++index )
            {
                r32 offsetX = RandomRangeFloat( &grassSeq, -hrs, hrs);
                r32 offsetY = RandomRangeFloat( &grassSeq, -hrs, hrs);
                Vec3 grassP = P + V3( offsetX, offsetY, 0 );
                AddRandomEntity( region, &region->server->randomSequence, grassP, testSlot->taxonomy);
            }
            
            testSlot = NORUNTIMEGetTaxonomySlotByName( taxTable, "testRock" );
            for( u32 index = 0; index < rockCount; ++index )
            {
                r32 offsetX = RandomRangeFloat( &grassSeq, -hrs, hrs);
                r32 offsetY = RandomRangeFloat( &grassSeq, -hrs, hrs);
                Vec3 rockP = P + V3( offsetX, offsetY, 0 );
                AddRandomEntity( region, &region->server->randomSequence, rockP, testSlot->taxonomy);
            }
#endif
            
#if GENERATE_OBJECTS
            testSlot = NORUNTIMEGetTaxonomySlotByName( taxTable, "arm armour" );
            //AddRandomEntity( region, &region->server->randomSequence, P + V3( 1, 4, 0 ), testSlot->taxonomy );
            
            testSlot = NORUNTIMEGetTaxonomySlotByName( taxTable, "standard leather" );
            AddRandomEntity( region, &region->server->randomSequence, P + V3( 2, 5, 0 ), testSlot->taxonomy );
            
            testSlot = NORUNTIMEGetTaxonomySlotByName( taxTable, "standard leather" );
            AddRandomEntity( region, &region->server->randomSequence, P + V3( 2, 6, 0 ), testSlot->taxonomy );
            
            testSlot = NORUNTIMEGetTaxonomySlotByName( taxTable, "special leather" );
            AddRandomEntity( region, &region->server->randomSequence, P + V3( 3, 5, 0 ), testSlot->taxonomy );
            
            testSlot = NORUNTIMEGetTaxonomySlotByName( taxTable, "special leather" );
            AddRandomEntity( region, &region->server->randomSequence, P + V3( 3, 6, 0 ), testSlot->taxonomy);
            
            testSlot = NORUNTIMEGetTaxonomySlotByName( taxTable, "pant" );
            AddRandomEntity( region, &region->server->randomSequence, P + V3( 2, 4, 0 ), testSlot->taxonomy);
            
            testSlot = NORUNTIMEGetTaxonomySlotByName( taxTable, "chest" );
            AddRandomEntity( region, &region->server->randomSequence, P + V3( 1, 1, 0 ), testSlot->taxonomy );
            
            testSlot = NORUNTIMEGetTaxonomySlotByName( taxTable, "bag" );
            AddRandomEntity( region, &region->server->randomSequence, P + V3( 2, 3, 0 ), testSlot->taxonomy );
            
            testSlot = NORUNTIMEGetTaxonomySlotByName( taxTable, "bag" );
            AddRandomEntity( region, &region->server->randomSequence, P + V3( 4, 3, 0 ), testSlot->taxonomy );
            
            testSlot = NORUNTIMEGetTaxonomySlotByName( taxTable, "bag" );
            AddRandomEntity( region, &region->server->randomSequence, P + V3( 6, 3, 0 ), testSlot->taxonomy );
            
            testSlot = NORUNTIMEGetTaxonomySlotByName( taxTable, "apple" );
            AddRandomEntity( region, &region->server->randomSequence, P + V3( 4.0f, 0.0f, 0.0f ), testSlot->taxonomy );
            
            for(u32 appleIndex = 0; appleIndex < 0; ++appleIndex)
            {
                r32 offsetX = RandomRangeFloat( &grassSeq, -15.0f, 15.0f );
                r32 offsetY = RandomRangeFloat( &grassSeq, -15.0f, 15.0f );
                AddRandomEntity( region, &region->server->randomSequence, P + V3( offsetX, offsetY, 0.0f ), testSlot->taxonomy );
            }
            
            
            
            testSlot = NORUNTIMEGetTaxonomySlotByName( taxTable, "smithing tools" );
            AddRandomEntity( region, &region->server->randomSequence, P + V3( 4.0f, 0.0f, 0.0f ), testSlot->taxonomy );
            
            testSlot = NORUNTIMEGetTaxonomySlotByName( taxTable, "strength" );
            AddRandomEntity( region, &region->server->randomSequence, P + V3( 5.0f, 0.0f, 0.0f ), testSlot->taxonomy );
            
            TaxonomySlot* recipeSlot = NORUNTIMEGetTaxonomySlotByName(taxTable, "sword");
            testSlot = NORUNTIMEGetTaxonomySlotByName( taxTable, "recipe" );
            AddEntity( region, P + V3( 4.0f, 0.0f, 0.0f ), testSlot->taxonomy, 0, RecipeObject(recipeSlot->taxonomy, 12, I16_MAX));
#endif
        }
        
    }
    
    context->immediateSpawn = false;
}

internal void BuildWorld(ServerState * server, u32 universeIndex)
{
    server->universeX = universeIndex % UNIVERSE_DIM;
    server->universeY = universeIndex / UNIVERSE_DIM;
    Assert( server->universeY < UNIVERSE_DIM );
    
    server->chunkDim = CHUNK_DIM;
    server->chunkSide = server->chunkDim * VOXEL_SIZE;
    server->oneOverChunkSide = 1.0f / server->chunkSide;
    server->lateralChunkSpan = SERVER_REGION_SPAN * SIM_REGION_CHUNK_SPAN;
    Assert( SIM_REGION_CHUNK_SPAN % 2 == 0 );
    i32 realServerRegionSpan = SERVER_REGION_SPAN + 2;
    
    for( i32 regionY = 0; regionY < realServerRegionSpan; regionY++ )
    {
        for( i32 regionX = 0; regionX < realServerRegionSpan; regionX++ )
        {
            i32 X = regionX - 1;
            i32 Y = regionY - 1;
            SimRegion * region = GetServerRegion(server, X, Y);
            
            region->regionX = X;
            region->regionY = Y;
            region->server = server;
            region->taxTable = server->activeTable;
            region->components = server->components;
            
            region->origin.chunkX = (X * SIM_REGION_CHUNK_SPAN) + SIM_REGION_CHUNK_SPAN / 2;
            region->origin.chunkY = (Y * SIM_REGION_CHUNK_SPAN) + SIM_REGION_CHUNK_SPAN / 2;
            
            if( X == -1 || X == SERVER_REGION_SPAN ||
               Y == -1 || Y == SERVER_REGION_SPAN )
            {
                region->border = Border_Mirror;
                region->updateHash = PushArray(&server->worldPool, HashEntityUpdate, HASH_UPDATE_COUNT);
            }
            else
            {
                if( X == 0 && Y == 0 )
                {
                    region->border = Border_DownLeft;
                }
                else if( X == ( SERVER_REGION_SPAN - 1 ) && Y == 0 )
                {
                    region->border = Border_RightDown;
                }
                else if( X == ( SERVER_REGION_SPAN - 1 ) && Y == ( SERVER_REGION_SPAN - 1 ) )
                {
                    region->border = Border_UpRight;
                }
                else if( X == 0 && Y == ( SERVER_REGION_SPAN - 1 ) )
                {
                    region->border = Border_LeftUp;
                }
                else if( X == 0 )
                {
                    region->border = Border_Left;
                }
                else if( X == SERVER_REGION_SPAN - 1 )
                {
                    region->border = Border_Right;
                }
                else if( Y == 0 )
                {
                    region->border = Border_Down;
                }
                else if( Y == SERVER_REGION_SPAN - 1 )
                {
                    region->border = Border_Up;
                }
            }
        }
    }
    
    u32 seed = server->universeX * server->universeY;
    r32 chunkSide = VOXEL_SIZE * server->chunkDim;
    server->randomSequence = Seed( seed );
    server->objectSequence = Seed( seed + 1 );
    
    i32 offset = SIM_REGION_CHUNK_SPAN;
    for( i32 Y = -offset; Y < server->lateralChunkSpan + offset; Y++ )
    {
        for( i32 X = -offset; X < server->lateralChunkSpan + offset; X++ )
        {
            Assert( ChunkValid( server->lateralChunkSpan, X, Y ) );
            WorldChunk * chunk = GetChunk( server->chunks, ArrayCount( server->chunks ), X, Y, &server->worldPool );
            BuildChunk(chunk, X, Y, server->universeX, server->universeY, server->lateralChunkSpan);
        }
    }
    
    // TODO(Leonardo): how do we pick one from those?
    RegionWorkContext *context = server->threadContext + 0;
    //SQLDeleteAllEntitites( server->conn, world->id );
    //SQLGenWorld( server->conn, world->id );
    for( u32 groupIndex = 0; groupIndex < 1; groupIndex++ )
    {
        SimRegion* region = GetServerRegion( server, 0, 0 );
        region->context = context;
        u32 groupType = Group_environment;
        //SpawnGroup( region, groupType );
    }
    
    BuildSimpleTestWorld( server );
}

internal void BuildDoungeon(ServerState * server, UniversePos doungeonOrigin)
{
    InvalidCodePath;
#if 0    
    server->worlds[server->countWorlds + server->countDoungeons++] = PushStruct(&server->pool, World);
    World * doungeon = server->worlds[ server->countWorlds + server->countDoungeons - 1 ];
    
    WorldComponent worldData = GetWorldData( doungeonOrigin.id );
    Assert( worldData.type == World_doungeon );
    
    InitializeWorld( server, doungeon, worldData.x, worldData.y, worldData.type );
    AddWorldInfoToWorld( server, generator, doungeonOrigin.id );
    AddWorldInfoToWorld( server, doungeon, generator->id );
    BuildWorld( server, doungeon );
#endif
    
}