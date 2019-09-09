#pragma once

#include "forg_base.h"
#include "ll_net.h"
#include "forg_platform.h"

#include "forg_pool.h"
#include "forg_debug_interface.h"
#include "forg_shared.h"
#include "forg_token.h"
#include "forg_intrinsics.h"
#include "forg_math.h"
#include "forg_file_formats.h"

#define Property(name) enum Property_##name
#include "../properties/test.properties"
#include "forg_asset.h"
#include "forg_editor.h"
#include "forg_animation.h"
#include "asset_builder.h"
#include "forg_random.h"
#include "forg_physics.h"
#include "forg_bound.h"
#include "forg_action_effect.h"
#include "forg_object.h"
//#include "forg_inventory.h"
#include "forg_AI.h"
#include "forg_essence.h"
#include "forg_skill.h"
#include "forg_crafting.h"
//#include "forg_entity.h"
//#include "forg_memory.h"
#include "forg_world.h"
#include "forg_world_generation.h"
#include "forg_plant.h"
#include "forg_rock.h"
#include "forg_network.h"
#include "forg_meta.h"
#include "client_generated.h"

PlatformAPI platformAPI;

struct GameFile
{
    u32 uncompressedSize;
    u32 compressedSize;
    u8* content;
    
    u16 type;
    u16 subtype;
};

struct FileToSend
{
    u32 index;
    
    union
    {
        FileToSend* next;
        FileToSend* nextFree;
    };
};

struct PlayerRequest
{
    u8 data[512];
};

struct Player
{
    b32 connectionClosed;
    u16 connectionSlot;
    
    ForgNetworkPacketQueue standardPacketQueue;
    ForgNetworkPacketQueue reliablePacketQueue;
    
    ForgNetworkReceiver receiver;
    
    u32 requestCount;
    PlayerRequest requests[8];
    
    u32 sendingFileOffset;
    FileToSend* firstLoginFileToSend;
    FileToSend* firstReloadedFileToSend;
};

struct ReceiveNetworkPacketWork
{
    NetworkInterface* network;
    network_platform_receive_data* ReceiveData;
};

struct Entity
{
    u32 ID;
    UniversePos P;
    
    Vec3 speed;
    Vec3 acc;
    
    u32 playerID;
};

#define MAXIMUM_SERVER_PLAYERS 0xff
struct ServerState
{
    WorldSeason season;
    r32 seasonLerp;
    r32 seasonTime;
    
    r32 elapsedTime;
    u32 worldSeed;
    
    TaskWithMemory tasks[6];
    PlatformWorkQueue* fastQueue;
    PlatformWorkQueue* slowQueue;
    
    u32 playerCount;
    Player players[MAXIMUM_SERVER_PLAYERS];
    
    u32 entityCount;
    Entity entities[0xffff];
    
    //SpacePartition collisionPartition;
    //SpacePartition playerPartition;
    
    MemoryPool gamePool;
    MemoryPool networkPool;
    
    ReceiveNetworkPacketWork receivePacketWork;
    NetworkInterface clientInterface;
    
    RandomSequence instantiateSequence;
    RandomSequence randomSequence;
    RandomSequence objectSequence;
    
    u32 fileCount;
    GameFile* files;
    
    FileToSend* firstFreeToSendFile;
};

#define SERVER_SIMULATE_WORLDS(name) void name(PlatformServerMemory* memory, r32 secondElapsed)
typedef SERVER_SIMULATE_WORLDS(server_simulate_worlds);

#define SERVER_FRAME_END(name) DebugTable* name(PlatformServerMemory* memory)
typedef SERVER_FRAME_END(server_frame_end);



