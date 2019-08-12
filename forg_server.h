#pragma once
#define MAX_OTHER_SERVERS 10
#define MAXIMUM_SERVER_PLAYERS 1000
#define INTROSPECTION

#include "forg_basic_types.h"
#include "net.h"
#include "forg_platform.h"
#include "forg_shared.h"
#include "forg_token.h"
#include "forg_intrinsics.h"
#include "forg_math.h"
#include "forg_random.h"
#include "forg_asset_enum.h"
#include "forg_physics.h"
#include "forg_rule.h"
#include "forg_fluid.h"
#include "forg_crafting.h"
#include "forg_inventory.h"
#include "forg_action_effect.h"
#include "forg_consideration.h"
#include "forg_AI.h"
#include "forg_taxonomy.h"
#include "forg_editor.h"
#include "forg_memory.h"
#include "forg_region.h"
#include "forg_world.h"
#include "forg_world_generation.h"
#include "forg_plant.h"
#include "forg_network.h"
#include "forg_meta.h"
#include "client_generated.h"
#include "win32_thread.cpp"

global_variable PlatformAPI platformAPI;
global_variable u16 globalPorts[] =
{
    8888
};

global_variable u16 globalUserPorts[] = 
{
    LOGIN_PORT
};


struct PlayerPermanent
{
    u32 regionX;
    u32 regionY;
    Vec3 P;
};

struct PlayerRequest
{
    u8 data[512];
};

struct ForgFile
{
    char filename[64];
    u64 hash;
    
    union
    {
        ForgFile* next;
        ForgFile* nextFree;
    };
};

struct ServerPlayer
{
    b32 connectionClosed;
    u16 connectionSlot;
    
    b32 allPakFileSent;
    b32 allDataFileSent;
    u32 pakFileIndex;
    u32 pakFileOffset;
    
    ForgNetworkPacketQueue standardPacketQueue;
    ForgNetworkPacketQueue reliablePacketQueue;
    
    ForgNetworkReceiver receiver;
    
    u32 playerID;
    u64 overlappingEntityID;
    
    u32 requestCount;
    PlayerRequest requests[8];
    
    u32 ignoredActionCount;
    EntityAction ignoredActions[4];
    
    SimEntity draggingEntity;
    
    u32 unlockedCategoryCount;
    u32 unlockedSkillCategories[256];
    
    u32 recipeCount;
    Recipe recipes[256];
    
    ForgFile* files;
    
    union
    {
        ServerPlayer* next;
        ServerPlayer* nextFree;
    };
};

struct MovingPlayer
{
    u32 oldEntityID;
    u32 oldPlayerID;
    
    u32 newEntityID;
    u32 newPlayerID;
    
    u32 newPlayerRecvSeed;
    u32 newPlayerSendSeed;
    u32 newPlayerRecvOffsetSeed;
    u32 newPlayerSendOffsetSeed;
    
    union
    {
        MovingPlayer* next;
        MovingPlayer* nextFree;
    };
};


struct RegionSlot
{
    SimRegion* region;
};

struct EntityComponentArray
{
    u32 nextID;
    u16 componentSize;
    u32 componentCount;
    u32 maxComponentCount;
    void* components;
    
    u32 firstFree;
    u32 nextFreeOffset;
};

inline u32* GetNextFreePointer(EntityComponentArray* array, u32 ID)
{
    u32* nextFree = (u32*) ((u8*) array->components + (ID * array->componentSize + array->nextFreeOffset));
    return nextFree;
}

inline void* GetComponent_(EntityComponentArray* components, EntityComponentType type, u32 ID)
{
    EntityComponentArray* array = components + type;
    Assert(ID);
    Assert(ID < array->componentCount);
    
    void* result = (void*) ((u8*) array->components + (ID * array->componentSize));
    return result;
}

inline u32 GetFreeComponent(EntityComponentArray* components, EntityComponentType type)
{
    EntityComponentArray* array = components + type;
    u32 result = array->firstFree;
    
    if(!result)
    {
        Assert(array->componentCount < array->maxComponentCount);
        result = array->componentCount++;
    }
    else
    {
        u32* nextFree = GetNextFreePointer(array, result);
        array->firstFree = *nextFree;
    }
    
    void* data = (void*) ((u8*) array->components + (result * array->componentSize));
    ZeroSize(array->componentSize, data);
    return result;
}

inline void FreeComponent(EntityComponentArray* components, EntityComponentType type, u32 ID)
{
    EntityComponentArray* array = components + type;
    u32* nextFreePtr = GetNextFreePointer(array, ID);
    *nextFreePtr = array->firstFree;
    array->firstFree = ID;
}


struct NewEntity
{
    SimRegion* region;
    u32 taxonomy;
    Vec3 P;
    u64 identifier;
    GenerationData gen;
    AddEntityAdditionalParams params;
    
	union
	{
		NewEntity* next;
		NewEntity* nextFree;
	};
};

struct DeletedEntity
{
    u32 entityID;
    u32 IDs[Component_Count];
    
    
	union
	{
		DeletedEntity* next;
		DeletedEntity* nextFree;
	};
};

struct ReceiveNetworkPacketWork
{
    NetworkInterface* network;
    network_platform_receive_data* ReceiveData;
};

struct FreeEntity
{
    u32 ID;
    
    union
    {
        FreeEntity* nextFree;
        FreeEntity* next;
    };
};

struct AIOperationNode;
struct ServerState
{
    RandomSequence instantiateSequence;
    
    WorldSeason season;
    r32 seasonLerp;
    r32 seasonTime;
    
    
    EditorTabStack editorStack;
    WorldGeneratorDefinition* generator;
    b32 editor;
    b32 gamePaused;
    GenerateWorldMode generateMode;
    
    r32 elapsedTime;
    u8 elapsedMS5x;
    
    u32 worldSeed;
    
#if FORGIVENESS_INTERNAL
    b32 recompiled;
    ServerPlayer* debugPlayer;
    
    b32 simulationStepDone;
    b32 fixedTimestep;
    b32 canAdvance;
#endif
    
    
    TaskWithMemory tasks[6];
    PlatformWorkQueue fastQueue;
    PlatformWorkQueue slowQueue;
    
    u32 regionSlotCount;
    RegionSlot regionSlots[256];   
    RegionWorkContext threadContext[16];
    
    EntityComponentArray components[Component_Count];
    
    u32 entityCount;
    SimEntity entities[0xffff];
    
    FreeEntity* firstFreeEntity;
    FreeEntity* firstFreeFreeEntity;
    
    MemoryPool worldPool;
    MemoryPool networkPool;
    
    ReceiveNetworkPacketWork receivePacketWork;
    NetworkInterface clientInterface;
    
    u32 currentPlayerIndex;
    ServerPlayer* players;
    ServerPlayer* firstFree;
    
    
    u64 currentIdentifier;
    TaxonomyTable* activeTable;
    TaxonomyTable* oldTable;
    
	TicketMutex newDeletedMutex;
    
    NewEntity* firstNewEntity;
	DeletedEntity* firstDeletedEntity;
    
	NewEntity* firstFreeNewEntity;
	DeletedEntity* firstFreeDeletedEntity;
    
    u8 chunkDim;
    r32 chunkSide;
    r32 oneOverChunkSide;
    i32 lateralChunkSpan;
    r32 regionSpan;
    WorldChunk* chunks[4096];
    SimRegion regions[SERVER_REGION_SPAN + 2][SERVER_REGION_SPAN + 2];
    
    RandomSequence randomSequence;
    RandomSequence objectSequence;
    
    PlayerPermanent editorPlayerPermanent;
    
    b32 reloadingAssets;
    PlatformProcessHandle assetBuilder;
    
    ForgFile* files;
    ForgFile* firstFreeFile;
    
    MemoryPool filePool;
    MemoryPool scratchPool;
};

#define SERVER_NETWORK_STUFF( name ) void name( PlatformServerMemory* memory, r32 secondElapsed )
typedef SERVER_NETWORK_STUFF( server_network_stuff );

#define SERVER_INITIALIZE( name ) void name(PlatformServerMemory* memory, u32 universeIndex, b32 editor)
typedef SERVER_INITIALIZE( server_initialize );

#define SERVER_SIMULATE_WORLDS( name ) void name(PlatformServerMemory* memory, r32 secondElapsed)
typedef SERVER_SIMULATE_WORLDS( server_simulate_worlds );

#define SERVER_FRAME_END( name ) DebugTable* name( PlatformServerMemory* memory )
typedef SERVER_FRAME_END( server_frame_end );