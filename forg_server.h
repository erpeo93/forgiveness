#pragma once
#define SERVER_MAX_FPS 10
#define SERVER_MIN_MSEC_PER_FRAME 1000.0f / (r32) (SERVER_MAX_FPS)
#include "forg_basic_types.h"
#include "ll_net.h"
#include "forg_platform.h"
#include "forg_shared.h"
#include "forg_token.h"
#include "forg_intrinsics.h"
#include "forg_math.h"
#include "forg_random.h"
#include "forg_asset_enum.h"
#include "forg_physics.h"
#include "forg_bound.h"
#include "forg_taxonomy.h"
#include "forg_action_effect.h"
#include "forg_object.h"
#include "forg_inventory.h"
#include "forg_AI.h"
#include "forg_essence.h"
#include "forg_skill.h"
#include "forg_crafting.h"
#include "forg_entity.h"
#include "forg_editor.h"
#include "forg_memory.h"
#include "forg_world.h"
#include "forg_world_generation.h"
#include "forg_region.h"
#include "forg_plant.h"
#include "forg_rock.h"
#include "forg_network.h"
#include "forg_meta.h"
#include "client_generated.h"
#include "win32_thread.cpp"
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

struct PlayerComponent
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
    
    u32 requestCount;
    PlayerRequest requests[8];
};

struct ReceiveNetworkPacketWork
{
    NetworkInterface* network;
    network_platform_receive_data* ReceiveData;
};

struct Entity
{
    u32 chunkX;
    u32 chunkY;
    Vec3 pos;
    Vec3 speed;
    Vec3 acc;
};

struct ServerState
{
    WorldSeason season;
    r32 seasonLerp;
    r32 seasonTime;
    
    r32 elapsedTime;
    u32 worldSeed;
    
    TaskWithMemory tasks[6];
    PlatformWorkQueue fastQueue;
    PlatformWorkQueue slowQueue;
    
    u32 entityCount;
    Entity entities[0xffff];
    
    SpacePartition collisionPartition;
    SpacePartition playerPartition;
    
    MemoryPool worldPool;
    MemoryPool networkPool;
    
    ReceiveNetworkPacketWork receivePacketWork;
    NetworkInterface clientInterface;
    
    RandomSequence instantiateSequence;
    RandomSequence randomSequence;
    RandomSequence objectSequence;
    
    PlatformProcessHandle assetBuilder;
    
    ForgFile* files;
    ForgFile* firstFreeFile;
    
    MemoryPool filePool;
    MemoryPool scratchPool;
};

#define SERVER_SIMULATE_WORLDS( name ) void name(PlatformServerMemory* memory, r32 secondElapsed)
typedef SERVER_SIMULATE_WORLDS( server_simulate_worlds );

#define SERVER_FRAME_END( name ) DebugTable* name( PlatformServerMemory* memory )
typedef SERVER_FRAME_END( server_frame_end );



