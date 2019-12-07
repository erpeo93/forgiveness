#pragma once

#include "forg_base.h"
#include "ll_net.h"
#include "forg_platform.h"

#include "forg_pool.h"
#include "forg_resizable_array.h"
#include "forg_debug_interface.h"
#include "forg_shared.h"
#include "forg_token.h"
#include "forg_intrinsics.h"
#include "forg_math.h"
#include "forg_simd.h"
#include "forg_file_formats.h"


struct U16
{
    u16 value;
    i16 flagOffset;
    u16 flag;
};

struct U32
{
    u32 value;
    i16 flagOffset;
    u16 flag;
};

#define Property(name) enum name
#include "../properties/test.properties"
#include "forg_ecs.h"
#include "forg_entity_layout.h"
#include "forg_entity.h"
#include "forg_asset.h"
#include "forg_noise.h"
#include "forg_world_generation.h"
#include "forg_world.h"
#include "forg_editor.h"
#include "asset_builder.h"
#include "forg_random.h"
#include "forg_physics.h"
#include "forg_game_effect.h"
#include "forg_network.h"
#include "forg_meta.h"
#include "forg_skill.h"
#include "forg_brain.h"


#include "forg_sound.h"
#include "forg_game_ui.h"
#include "forg_particles.h"
#include "forg_animation.h"
PlatformAPI platformAPI;

struct GameFile
{
    MemoryPool pool;
    
    u32 uncompressedSize;
    u32 compressedSize;
    u8* content;
    
    u64 dataHash;
    
    b32 valid;
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

struct DefaultComponent
{
    b32 updateSent;
    u16 basicPropertiesChanged;
    u16 healthPropertiesChanged;
    
    EntityRef definitionID;
    u32 seed;
    UniversePos P;
    U32 flags;
    U16 status;
};

struct StaticComponent
{
    SpatialPartitionChunk* chunk;
    SpatialPartitionEntityBlock* block;
};

inline void AddChangedFlags(DefaultComponent* def, u16* flags, u16 flag)
{
    def->updateSent = false;
    *flags = *flags | flag;
}

inline void AddChangedFlags(DefaultComponent* def, u16 flagOffset, u16 flag)
{
    Assert(flagOffset);
    Assert(OffsetOf(DefaultComponent, updateSent) == 0);
    u16* flags = (u16*) ((u8*) def + flagOffset);
    AddChangedFlags(def, flags, flag);
}

inline void SetU16(DefaultComponent* def, U16* value, u16 newValue)
{
    if(value->value != newValue)
    {
        value->value = newValue;
        AddChangedFlags(def, value->flagOffset, value->flag);
    }
}

inline u16 GetU16(U16 value)
{
    u16 result = value.value;
    return result;
}

inline b32 operator == (U16 v1, U16 v2)
{
    b32 result = false;
    if(v1.value == v2.value)
    {
        result = true;
    }
    return result;
}

inline b32 operator == (U16 v1, u16 v2)
{
    b32 result = false;
    if(v1.value == v2)
    {
        result = true;
    }
    return result;
}

inline b32 operator == (u16 v1, U16 v2)
{
    b32 result = false;
    if(v1 == v2.value)
    {
        result = true;
    }
    return result;
}


#define InitU32(flags, flag, value) InitU32_((i16) OffsetOf(DefaultComponent, flags), flag, value) 
inline U32 InitU32_(i16 flagOffset, u16 flag, u32 value)
{
    U32 result = {};
    result.value = value;
    result.flagOffset = flagOffset;
    result.flag = flag;
    
    return result;
}

inline void SetU32(DefaultComponent* def, U32* value, u32 newValue)
{
    if(value->value != newValue)
    {
        value->value = newValue;
        AddChangedFlags(def, value->flagOffset, value->flag);
    }
}

inline u32 GetU32(U32 value)
{
    u32 result = value.value;
    return result;
}

inline b32 operator == (U32 v1, U32 v2)
{
    b32 result = false;
    if(v1.value == v2.value)
    {
        result = true;
    }
    return result;
}

#define InitU16(flags, flag, value) InitU16_((i16) OffsetOf(DefaultComponent, flags), flag, value) 
inline U16 InitU16_(i16 flagOffset, u16 flag, u16 value)
{
    U16 result = {};
    result.value = value;
    result.flagOffset = flagOffset;
    result.flag = flag;
    
    return result;
}

inline void AddEntityFlags(DefaultComponent* def, u32 flags)
{
    u32 currentFlags = GetU32(def->flags);
    u32 newFlags = AddFlags(currentFlags, flags);
    SetU32(def, &def->flags, newFlags);
}

inline b32 EntityHasFlags(DefaultComponent* def, u32 test)
{
    u32 flags = GetU32(def->flags);
    b32 result = IsSet(flags, test);
    return result;
}

inline void ClearEntityFlags(DefaultComponent* def, u32 flags)
{
    u32 currentFlags = GetU32(def->flags);
    u32 newFlags = ClearFlags(currentFlags, flags);
    SetU32(def, &def->flags, newFlags);
}


struct PhysicComponent
{
	Rect3 bounds;
    Vec3 speed;
    Vec3 acc;
    r32 accelerationCoeff;
    r32 drag;
};

struct ActionComponent
{
    U16 action;
    r32 time;
};

struct TempEntityComponent
{
	r32 targetTime;
	r32 time;
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
    EntityID ID;
    b32 justEnteredWorld;
    
    b32 connectionClosed;
    u16 connectionSlot;
    
    ForgNetworkPacketQueue queues[GuaranteedDelivery_Count];
    ForgNetworkReceiver receiver;
    
    u16 expectingCommandIndex;
    
    GameCommand requestCommand;
    GameCommand inventoryCommand;
    CommandParameters commandParameters;
    
    FileHash* firstFileHash;
    
    u32 runningFileIndex;
    FileToSend* firstLoginFileToSend;
    FileToSend* firstReloadedFileToSend;
};

#include "forg_archetypes.h"
#include "client_generated.h"
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

struct AddEntityParams
{
    Vec3 acceleration;
    Vec3 speed;
    u32 playerIndex;
    EntityRef attachedEntityType;
    EntityID targetBrainID;
    b32 spawnFollowingEntity;
    b32 lock;
    b32 ghost;
};

internal AddEntityParams DefaultAddEntityParams()
{
    AddEntityParams result = {};
    return result;
}

internal AddEntityParams GhostEntityParams()
{
    AddEntityParams params = DefaultAddEntityParams();
    params.ghost = true;
    return params;
}

struct NewEntity
{
    UniversePos P;
    u32 seed;
    AssetID definitionID;
    AddEntityParams params;
    union
    {
        NewEntity* next;
        NewEntity* nextFree;
    };
};

enum DeleteEntityReasonType
{
    DeleteEntity_None,
    DeleteEntity_Ghost,
    DeleteEntity_Won,
};

struct DeletedEntity
{
    EntityID ID;
    DeleteEntityReasonType type;
    union
    {
        DeletedEntity* next;
        DeletedEntity* nextFree;
    };
};

struct ServerState
{
    u32 worldSeed;
    
	b32 gamePaused;
    r32 seasonTime;
    u16 season;
    
    TaskWithMemory tasks[6];
    PlatformWorkQueue* fastQueue;
    PlatformWorkQueue* slowQueue;
    
    ResizableArray archetypes[Archetype_Count];
    ResizableArray PlayerComponent_;
    
    WorldTile nullTile;
    u16 maxDeepness;
    WorldChunk* chunks;
    MemoryPool chunkPool;
    
    MemoryPool* frameByFramePool;
    MemoryPool gamePool;
    MemoryPool networkPool;
    
    ReceiveNetworkPacketWork receivePacketWork;
    NetworkInterface clientInterface;
    
    SpatialPartition standardPartition;
    SpatialPartition playerPartition;
    SpatialPartition staticPartition;
    
    RandomSequence entropy;
    
    u32 fileCount;
    GameFile* files;
    
    TimestampHash fileHash;
    
    FileToSend* firstFreeToSendFile;
    FileHash* firstFreeFileHash;
    
    Assets* assets;
    
    NewEntity* firstFreeNewEntity;
    NewEntity* firstNewEntity;
    
    DeletedEntity* firstFreeDeletedEntity;
    DeletedEntity* firstDeletedEntity;
    
    r32 staticUpdateTimer;
    r32 equipmentEffectTimer;
    
#if FORGIVENESS_INTERNAL
    b32 captureFrame;
#endif
};

#define SERVER_SIMULATE_WORLDS(name) void name(PlatformServerMemory* memory, r32 secondElapsed)
typedef SERVER_SIMULATE_WORLDS(server_simulate_worlds);


