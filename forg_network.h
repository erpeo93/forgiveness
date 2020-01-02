#pragma once

#define LOGIN_PORT 1313
#define MTU KiloBytes(1) + 32
#define CHUNK_SIZE KiloBytes(1)
#define STATIC_UPDATE_TIME 4.0f

#pragma pack(push, 1)
enum ForgNetworkFlags
{
    ForgNetworkFlag_Ordered = (1 << 1),
    ForgNetworkFlag_FileChunk = (1 << 2),
};

struct ForgNetworkApplicationData
{
    u32 index;
    u8 flags;
};

inline unsigned char* ForgPackApplicationData(unsigned char* buff, ForgNetworkApplicationData data)
{
    buff += pack(buff, "LC", data.index, data.flags);
    return buff;
}

inline unsigned char* ForgUnpackApplicationData(unsigned char* buff, ForgNetworkApplicationData* data)
{
    buff = unpack(buff, "LC", &data->index, &data->flags);
    return buff;
}

struct ForgNetworkHeader
{
    u8 packetType;
};

inline unsigned char* ForgPackHeader(unsigned char* buff, u8 packetType)
{
    buff += pack(buff, "C", packetType);
    return buff;
}

inline unsigned char* ForgUnpackHeader(unsigned char* buff, ForgNetworkHeader* header)
{
    buff = unpack(buff, "C", &header->packetType);
    return buff;
}

struct EntityHeader
{
    u64 identifier;
};

struct ContainerHeader
{
    u64 identifier;
};

#pragma pack(pop)
struct ForgNetworkPacket
{
    u16 size;
    u8 data[MTU + sizeof(ForgNetworkHeader) + sizeof(u64) + sizeof(ForgNetworkApplicationData)];
    
    union
    {
        ForgNetworkPacket* next;
        ForgNetworkPacket* nextFree;
    };
};

struct ForgNetworkPacketQueue
{
    ForgNetworkApplicationData nextSendApplicationData;
    
    ForgNetworkPacket* firstPacket;
    ForgNetworkPacket* lastPacket;
    
    MemoryPool tempPool;
};


#define WINDOW_SIZE 2048
struct ForgNetworkReceiver
{
    ForgNetworkApplicationData unorderedBiggestReceived;
    ForgNetworkApplicationData orderedBiggestReceived;
    
    u32 circularStartingIndex;
    u32 circularEndingIndex;
    NetworkPacketReceived orderedWindow[WINDOW_SIZE];
};

inline void ResetReceiver(ForgNetworkReceiver* receiver)
{
    receiver->unorderedBiggestReceived.index = 0xffffffff;
    receiver->orderedBiggestReceived.index = 0xffffffff;
    
    receiver->circularStartingIndex = 0;
    receiver->circularEndingIndex = 0;
    
    for(u32 packetIndex = 0; packetIndex < ArrayCount(receiver->orderedWindow); ++packetIndex)
    {
        receiver->orderedWindow[packetIndex].dataSize = 0;
    }
    
}

inline b32 ApplicationIndexGreater(ForgNetworkApplicationData s1, ForgNetworkApplicationData s2)
{
    b32 result = (( s1.index > s2.index) && (s1.index - s2.index <= 2147483647)) || 
        ((s1.index < s2.index) && (s2.index - s1.index  > 2147483647));
    return result;
}


inline b32 ApplicationIndexSmaller(ForgNetworkApplicationData s1, ForgNetworkApplicationData s2)
{
    b32 result = (s1.index != s2.index) && (!ApplicationIndexGreater(s1, s2));
    return result;
}

inline u32 ApplicationDelta(ForgNetworkApplicationData packetIndex1, ForgNetworkApplicationData packetIndex2)
{
    u32 result = 0;
    if(ApplicationIndexSmaller(packetIndex2, packetIndex1))
    {
        u64 p1 = (u64) packetIndex1.index;
        u64 p2 = (u64) packetIndex2.index;
        
        if(packetIndex2.index > packetIndex1.index)
        {
            p1 += 0xffffffff;
            p1 += 1;
            
        }
        
        result = (u32) (p1 - p2);
    }
    return result;
}



inline u16 ForgEndPacket_(unsigned char* original, unsigned char* current)
{
    u16 result = (u16) (current - original);
    return result;
}

enum Packet_Type
{
    Type_invalid,
    Type_login,
    Type_loginFileTransferBegin,
    Type_gameAccess,
    Type_GameOver,
    Type_GameWon,
    
    Type_Command,
    Type_InventoryCommand,
    Type_CommandParameters,
    Type_selectRecipeEssence,
    Type_CompletedCommand,
    
    Type_entityHeader,
    Type_entityBasics,
    
    Type_Action,
    Type_Health,
    Type_Combat,
    Type_Vegetation,
    Type_Light,
    
    Type_Mappings,
    Type_EssenceDelta,
    Type_deletedEntity,
    Type_DayTime,
    
    
    Type_FileHeader,
    Type_FileChunk,
    Type_FileHash,
    
    Type_MoveChunkZ,
    Type_MovePlayerInOtherRegion,
    
    Type_PauseToggle,
    Type_SpawnEntity,
    Type_RecreateWorld,
    Type_RegenerateWorldChunks,
    
#if FORGIVENESS_INTERNAL
    Type_CaptureFrame,
    Type_debugEvent,
    Type_InputRecording,
#endif
};

enum NetworkMappingType
{
    Mapping_Equipment,
    Mapping_Using,
    Mapping_ContainerStored,
    Mapping_ContainerUsing,
    Mapping_Dragging,
    Mapping_OpenedBy,
};

enum NetworkProperties
{
    Network_Action,
    Network_Count
};

enum BasicNetworkFlags
{
	BasicFlags_Definition = (1 << 0),
	BasicFlags_Position = (1 << 1),
	BasicFlags_Velocity = (1 << 2),
	BasicFlags_Flags = (1 << 3),
	BasicFlags_Spawner = (1 << 4),
};

enum ActionNetworkFlags
{
    ActionFlags_Action = (1 << 0),
};

enum HealthNetworkFlags
{
    HealthFlag_Physical = (1 << 0),
    HealthFlag_MaxPhysical = (1 << 1),
    HealthFlag_Mental = (1 << 2),
    HealthFlag_MaxMental = (1 << 3),
};

enum CombatNetworkFlags
{
    CombatFlag_AttackDistance = (1 << 0),
    CombatFlag_AttackContinueCoeff = (1 << 1),
};

enum LightNetworkFlags
{
    LightFlag_LightRadious = (1 << 0),
};

enum VegetationNetworkFlags
{
    VegetationFlag_FlowerDensity = (1 << 0),
    VegetationFlag_FruitDensity = (1 << 1),
};

struct LoginResponse
{
    u16 port;
    u32 challenge;
    b32 editingEnabled;
};

struct LoginRequest
{
    i32 password;
};

struct GameAccessRequest
{
    u32 challenge;
};

struct GameCommand
{
    u16 action;
    u16 skillIndex;
    EntityID targetID;
    EntityID usingID;
    
    // NOTE(Leonardo): only for inventory commands!
    EntityID containerID;
    u16 targetObjectIndex;
    EntityID targetContainerID;
    u16 optionIndex;
};

struct CommandParameters
{
    Vec3 acceleration;
    Vec3 targetOffset;
};

inline b32 AreEqual(GameCommand c1, GameCommand c2)
{
    b32 result = (c1.action == c2.action &&
                  c1.skillIndex == c2.skillIndex &&
                  AreEqual(c1.targetID, c2.targetID) &&
                  AreEqual(c1.usingID, c2.usingID));
    return result;
}

inline b32 CommandIndexValid(u16 s1, u16 s2)
{
    b32 result = (s1 == s2 || (( s1 > s2) && (s1 - s2 <= 32768)) || 
                  ((s1 < s2) && (s2 - s1  > 32768)));
    return result;
}
