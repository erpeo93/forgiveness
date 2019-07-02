#pragma once
#define SERVER_MAX_FPS 10
#define SERVER_MIN_MSEC_PER_FRAME 1000.0f / (r32) (SERVER_MAX_FPS)
// NOTE(Leonardo): has to be a power of 2!
#define HASH_UPDATE_COUNT 4096

#define VOXEL_SIZE 1.3f
#define CHUNK_DIM 8
#define SERVER_REGION_SPAN 4
#define SIM_REGION_CHUNK_SPAN 8
#define UNIVERSE_DIM 1


struct UniversePos
{
    i32 chunkX;
    i32 chunkY;
    
    Vec3 chunkOffset;
};

enum Entity_flags
{
    Flag_floating = (1 << 1),
    Flag_distanceLimit = (1 << 2),
    Flag_deleteWhenDistanceCovered = (1 << 3),
    Flag_insideRegion = (1 << 4),
    Flag_Attached = (1 << 5),
    
    Flag_deleted = (1 << 20),
};

struct SkillSlot
{
    u32 taxonomy;
    u32 level;
};

#define MAX_PASSIVE_SKILLS_ACTIVE 6


#ifdef FORG_SERVER
struct EffectComponent
{
    u32 effectCount;
    Effect effects[16];
    
    u32 nextFree;
};

struct PlantComponent
{
	r32 age;
    r32 life;
    
    r32 leafDensity;
    r32 flowerDensity;
    r32 fruitDensity;
    
    u32 nextFree;
};

struct ContainerComponent
{
	ContainedObjects objects;
    
    ContainerInteraction insideInteraction;
    r32 updateTime;
    
    u32 nextFree;
};

struct FluidComponent
{
	Fluid fluid;
    
    u32 nextFree;
};

struct PassiveSkillEffects
{
    u8 effectCount;
    Effect effects[8];
};

introspection() struct CreatureComponent
{
    EquipmentSlot equipment[Slot_Count];
    Brain brain;
    r32 skillCooldown; 
    r32 strength;
    r32 lifePoints;
    r32 maxLifePoints;
    
    r32 stamina;
    r32 maxStamina;
    
    u64 openedContainerID;
    u64 externalDraggingID;
    Vec3 customTargetP;
    SimEntity* draggingEntity;
    i32 activeSkillIndex;
    
    u32 skillCount;
    SkillSlot skills[256];
    
    u32 recipeCount;
    Recipe recipes[256];
    
    SkillSlot passiveSkills[MAX_PASSIVE_SKILLS_ACTIVE];
    PassiveSkillEffects passiveSkillEffects[MAX_PASSIVE_SKILLS_ACTIVE];
	EssenceSlot essences[MAX_DIFFERENT_ESSENCES];
    
    u8 startedAction;
    u64 startedActionTarget;
    
    u8 completedAction;
    u64 completedActionTarget;
    
    u32 nextFree;
};

enum EntityComponentType
{
    Component_Effect,
    Component_Plant,
    Component_Container,
    Component_Fluid,
    Component_Creature,
    
    Component_Count,
};

introspection() struct SimEntity
{
    u32 taxonomy;
    r32 generationIntensity;
    u32 flags;
    u64 identifier;
    u32 playerID;
    Vec3 P;
    b32 flipOnYAxis;
    r32 facingDirection;
    Vec3 velocity;
    Vec3 acceleration;
    EntityAction action;
    r32 actionTime;
    u64 targetID;
    r32 distanceToTravel;
    ForgBoundType boundType;
    Rect3 bounds;
    r32 quantity;
    r32 status;
	u32 recipeTaxonomy;
    GenerationData gen;
    u64 ownerID;
    
    u32 IDs[Component_Count];
};


#define Entity_(comp, ID) (EntityComponent*) GetComponent_(comp, Component_Entity, ID)
#define Effects_(comp, ID) (EffectComponent*) GetComponent_(comp, Component_Effect, ID)
#define Plant_(comp, ID) (PlantComponent*) GetComponent_(comp, Component_Plant, ID)
#define Container_(comp, ID) (ContainerComponent*) GetComponent_(comp, Component_Container, ID)
#define Fluid_(comp, ID) (FluidComponent*) GetComponent_(comp, Component_Fluid, ID)
#define Creature_(comp, ID) (CreatureComponent*) GetComponent_(comp, Component_Creature, ID)

#define Entity(region, entity) Entity_(region->components, (entity)->IDs[Component_Entity])
#define Effects(region, entity) Effects_(region->components, (entity)->IDs[Component_Effect])
#define Plant(region, entity) Plant_(region->components, (entity)->IDs[Component_Plant])
#define Container(region, entity) Container_(region->components, (entity)->IDs[Component_Container])
#define Fluid(region, entity) Fluid_(region->components, (entity)->IDs[Component_Fluid])
#define Creature(region, entity) Creature_(region->components, (entity)->IDs[Component_Creature])


// NOTE(Leonardo): every world has a certain number of these: when is time to simulate that world, a context is assigned to every simulation region, so that there is no need of thread-lock
struct EntityBlock;
struct RegionWorkContext
{
    b32 used;
    b32 immediateSpawn;
    
    MemoryPool pool;
    EntityBlock* firstFreeBlock;
};

enum BorderType
{
    Border_None,
    Border_Mirror,
    Border_Right,
    Border_RightDown,
    Border_Down,
    Border_DownLeft,
    Border_Left,
    Border_LeftUp,
    Border_Up,
    Border_UpRight
};

struct RegionTile
{
    b32 valid;
    
    u32 X;
    u32 Y;
    Vec3 P;
    
    r32 waterAmount;
    b32 dirty;
};

struct RegionFluidHash
{
    u64 id1;
    u64 id2;
    
    u32 segmentIndex1;
    u32 segmentIndex2;
    
    u32 tileX;
    u32 tileY;
    i32 Z;
    
    u32 entityIndex;
    RegionFluidHash* nextInHash;
};

struct RegionIDHash
{
    u64 ID;
    u32 regionIndex;
    
    RegionIDHash* nextInHash;
};

struct CollisionData
{
    u32 entityIndex;
	b32 insideRegion;
    r32 radiousSq;
    Vec3 P;
};

struct PartitionSurfaceEntityBlock
{
    u32 entityCount;
    CollisionData colliders[32];
    PartitionSurfaceEntityBlock* next;
};

struct RegionPartitionSurface
{
    PartitionSurfaceEntityBlock* first;
};

struct RegionPartitionQueryResult
{
    u32 surfaceIndexes[64];
};

struct SpacePartition
{
    r32 oneOverSurfaceDim;
    u32 partitionSurfaceDim;
    RegionPartitionSurface* partitionSurfaces;
};

struct SimEntityBlock
{
    u32 entityCount;
    
    u32 IDs[256];
    SimEntity* entities[256];
    SimEntityBlock* next;
};

struct EffectTriggeredToSend
{
    Vec3 P;
    
    u64 actor;
    u64 target;
    u32 ID;
};

struct ServerState;
struct SimRegion
{
    ServerState* server;
    TaxonomyTable* taxTable;
    struct EntityComponentArray* components;
    
    RegionWorkContext* context;
    
    b32 simulating;
    
    BorderType border;
    i32 regionX;
    i32 regionY;
    
    r32 timeToUpdate;
    Rect2 updateBounds;
    u32 playerCount;
    
    struct HashEntityUpdate* updateHash; 
    
    u32 fluidHashCount;
    RegionFluidHash** fluidHash;
    
    u32 idHashCount;
    RegionIDHash** IDHash;
    
    SimEntityBlock* firstEntityBlock;
    
    r32 halfGridDim;
    u32 gridSide;
    RegionTile* tiles;
    
    RandomSequence entropy;
    UniversePos origin;
    
    SpacePartition collisionPartition;
    SpacePartition playerPartition;
    
    u32 effectTriggeredCount;
    EffectTriggeredToSend effectToSend[1024];
    
};

inline void AddFlags(SimEntity* entity, u32 flags)
{
    entity->flags |= flags;
}

inline b32 IsSet(SimEntity* entity, i32 flag)
{
    b32 result = (entity->flags & flag);
    return result;
}

inline void ClearFlags(SimEntity* entity, i32 flags)
{
    entity->flags &= ~flags;
}

inline RegionTile* GetRegionTile( SimRegion* region, Vec2 tileXY )
{
    RegionTile* result = 0;
    
    r32 totalSpan = 2.0f * region->halfGridDim;
    r32 normalizedX = ( tileXY.x + region->halfGridDim ) / totalSpan;
    r32 normalizedY = ( tileXY.y + region->halfGridDim ) / totalSpan;
    
    if( Normalized( normalizedX ) && Normalized( normalizedY ) )
    {
        u32 indexX = ( u32 ) ( normalizedX * ( r32 ) region->gridSide ); 
        u32 indexY = ( u32 ) ( normalizedY * ( r32 ) region->gridSide ); 
        
        u32 index = ( indexY * region->gridSide ) + indexX;
        result = region->tiles + index;
        
    }
    
    return result;
}

inline RegionTile* GetRegionTile( SimRegion* region, Vec3 origin, u32 worldTileX, u32 worldTileY, b32 clampAllowed = false )
{
    RegionTile* originTile = GetRegionTile( region, origin.xy );
    Vec2 delta = V2i( worldTileX - originTile->X, worldTileY - originTile->Y ) * VOXEL_SIZE;
    
    if( clampAllowed )
    {
        delta.x = Clamp( -region->halfGridDim, delta.x, region->halfGridDim );
        delta.y = Clamp( -region->halfGridDim, delta.y, region->halfGridDim );
    }
    
    RegionTile* result = GetRegionTile( region, delta );
    
    return result;
}

inline Vec3 GetDirectionTowards( SimRegion* region, Vec3 P, u32 worldTileX, u32 worldTileY )
{
    RegionTile* tile = GetRegionTile( region, P, worldTileX, worldTileY, true );
    Vec3 result = tile->P - P;
    return result;
}

inline SimEntity* GetRegionEntity(SimRegion* region, u32 index)
{
    Assert(region->firstEntityBlock);
    
    SimEntity* result = 0;
    for(SimEntityBlock* block = region->firstEntityBlock; block; block = block->next)
    {
        if(index < ArrayCount(block->entities))
        {
            result = block->entities[index];
            break;
        }
        
        index -= ArrayCount(block->entities);
    }
    
    Assert(result);
    return result;
}

inline SimEntity* GetRegionEntityByID( SimRegion* region, u64 ID )
{
    SimEntity* result = 0;
    
    u32 index = ( u32 ) ID & ( region->idHashCount - 1 );
    RegionIDHash* hash = region->IDHash[index];
    
    while( hash )
    {
        if(hash->ID == ID)
        {
            result = GetRegionEntity(region, hash->regionIndex);
            break;
        }
        
        hash = hash->nextInHash;
    }
    
    return result;
}
#endif