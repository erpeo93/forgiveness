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

#define Property(name) enum name
#include "../properties/test.properties"

#include "forg_asset.h"
#include "forg_world.h"
#include "forg_editor.h"
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
#include "forg_world_generation.h"
#include "forg_plant.h"
#include "forg_rock.h"
#include "forg_network.h"
#include "forg_meta.h"
#include "forg_ground.h"
#include "forg_resizable_array.h"

PlatformAPI platformAPI;

struct GameFile
{
    MemoryPool pool;
    
    u32 uncompressedSize;
    u32 compressedSize;
    u8* content;
    
    u64 dataHash;
    
    u16 type;
    char subtype[32];
    
    u32 counter;
};

struct FileToSend
{
    b32 acked;
    u32 playerIndex;
    u32 serverFileIndex;
    u32 sendingOffset;
    
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

struct PhysicComponent
{
    UniversePos P;
    Rect3 bounds;
    AssetID definitionID;
    u32 seed;
    Vec3 speed;
    Vec3 acc;
    GameProperty action;
};

struct FileHash
{
    u16 type;
    u64 subtypeHash;
    u64 dataHash;
    
    union
    {
        FileHash* next;
        FileHash* nextFree;
    };
};

struct PlayerComponent
{
    b32 connectionClosed;
    u16 connectionSlot;
    
    ForgNetworkPacketQueue queues[GuaranteedDelivery_Count];
    ForgNetworkReceiver receiver;
    
    u32 requestCount;
    PlayerRequest requests[8];
    
    
    FileHash* firstFileHash;
    
    u32 runningFileIndex;
    FileToSend* firstLoginFileToSend;
    FileToSend* firstReloadedFileToSend;
};

#include "forg_ecs.h"
#include "forg_archetypes.h"
#include "client_generated.h"
global_variable Initentity* InitFunc[Archetype_Count];
global_variable ArchetypeLayout archetypeLayouts[Archetype_Count];

struct ReceiveNetworkPacketWork
{
    NetworkInterface* network;
    network_platform_receive_data* ReceiveData;
};

struct SavedFileInfoHash
{
    char pathAndName[256];
    
    PlatformFileTimestamp timestamp;
    
    SavedFileInfoHash* next;
};

struct SavedTypeSubtypeCountHash
{
    char typeSubtype[256];
    
    u32 fileCount;
    u32 markupCount;
    
    SavedTypeSubtypeCountHash* next;
};

struct TimestampHash
{
    TicketMutex fileMutex;
    
    MemoryPool pool;
    SavedFileInfoHash* hashSlots[1024];
    SavedTypeSubtypeCountHash* countHashSlots[128];
};

struct ServerState
{
    WorldSeason season;
    r32 seasonLerp;
    r32 seasonTime;
    u32 worldSeed;
    
    TaskWithMemory tasks[6];
    PlatformWorkQueue* fastQueue;
    PlatformWorkQueue* slowQueue;
    
    ResizableArray archetypes[Archetype_Count];
    ResizableArray PlayerComponent_;
    //SpacePartition collisionPartition;
    //SpacePartition playerPartition;
    
    WorldChunk chunks[WORLD_CHUNK_SPAN][WORLD_CHUNK_SPAN];
    
    MemoryPool gamePool;
    MemoryPool networkPool;
    
    ReceiveNetworkPacketWork receivePacketWork;
    NetworkInterface clientInterface;
    
    RandomSequence entropy;
    
    u32 fileCount;
    GameFile* files;
    
    TimestampHash fileHash;
    
    FileToSend* firstFreeToSendFile;
    FileHash* firstFreeFileHash;
    
    Assets* assets;
};

#define SERVER_SIMULATE_WORLDS(name) void name(PlatformServerMemory* memory, r32 secondElapsed)
typedef SERVER_SIMULATE_WORLDS(server_simulate_worlds);

#define SERVER_FRAME_END(name) DebugTable* name(PlatformServerMemory* memory)
typedef SERVER_FRAME_END(server_frame_end);



