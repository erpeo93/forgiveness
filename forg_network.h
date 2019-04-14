#pragma once

#define LOGIN_PORT 1313
#define MTU KiloBytes(1) + 1
#pragma pack(push, 1)

enum ForgNetworkFlags
{
    ForgNetworkFlag_Ordered = (1 << 1),
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


#define WINDOW_SIZE 256
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
    Type_gameAccess,
    Type_serverInfo,
    
    
    
    Type_ActionRequest,
    Type_EquipRequest,
    Type_DisequipRequest,
    Type_MoveRequest,
    Type_DropRequest,
    Type_SwapRequest,
    Type_DragEquipmentRequest,
    Type_EquipDraggingRequest,
    Type_CraftRequest,
    Type_ActiveSkillRequest,
    Type_UnlockSkillCategoryRequest,
    Type_SkillLevelUpRequest,
    Type_PassiveSkillRequest,
    Type_LearnRequest,
    Type_ConsumeRequest,
    Type_CraftFromInventoryRequest,
    
    Type_containerHeader,
    Type_containerInfo,
    Type_objectRemoved,
    Type_objectAdded,
    
    Type_entityHeader,
    Type_entityBasics,
    Type_equipmentSlot,
    Type_deletedEntity,
    Type_tileUpdate,
    Type_essenceDelta,
    Type_effectTriggered,
    Type_possibleActions,
    Type_AvailableRecipes,
    Type_NewRecipe,
    Type_SkillCategories,
    Type_SkillLevel,
    Type_CompletedAction,
    Type_StartedAction,
    
    Type_DataFileHeader,
    Type_PakFileHeader,
    Type_FileChunk,
    
    Type_AllDataFileSent,
    Type_AllPakFileSent,
    
    Type_NewEditorTab,
    Type_EditorElement,
    Type_PopEditorElement,
    Type_EditTab,
    Type_ReloadEditingSlot,
    Type_AddTaxonomy,
    Type_DeleteTaxonomy,
    Type_ReviveTaxonomy,
    Type_InstantiateTaxonomy,
    Type_InstantiateRecipe,
    Type_DeleteEntity,
    Type_ImpersonateEntity,
    Type_PauseToggle,
    Type_ReloadAssets,
    Type_SaveSlotTabToFile,
    Type_PatchLocalServer,
    Type_PatchCheck,
    Type_SaveAssetFadFile,
    
    Type_EditorP,
    
#if FORGIVENESS_INTERNAL
    Type_debugSpawnEntity,
    Type_debugDeleteEntity,
    Type_debugAIInfo,
    Type_debugEvent,
    Type_memoryStats,
    Type_InputRecording,
#endif
};

struct SkillCategory
{
    b32 unlocked;
    u32 taxonomy;
};

struct ActionRequest
{
    u32 sequenceNumber;
    
    Vec3 acceleration;
    u32 desiredAction;
    u64 targetEntityID;
    u64 overlappingEntityID;
};

struct MoveRequest
{
    u64 sourceContainerID;
    u8 sourceObjectIndex;
    
    u64 destContainerID;
    u8 destObjectIndex;
};

struct EquipRequest
{
    u64 sourceContainerID;
    u8 sourceObjectIndex;
};

struct DisequipRequest
{
    u32 slotIndex;
    
    u64 destContainerID;
    u8 destObjectIndex;
};

struct DropRequest
{
    u64 sourceContainerID;
    u8 sourceObjectIndex;
};

struct SwapRequest
{
    u64 sourceContainerID;
    u8 sourceObjectIndex;
};

struct DragEquipmentRequest
{
    u32 slotIndex;
};

struct EquipDraggingRequest
{
    u32 slotIndex;
};

struct CraftRequest
{
    u32 taxonomy;
    GenerationData gen;
};

struct CraftFromInventoryRequest
{
    u64 containerID;
    u32 objectIndex;
};

struct LearnRequest
{
    u64 containerID;
    u32 objectIndex;
};

struct ConsumeRequest
{
    u64 containerID;
    u32 objectIndex;
};

struct SetActiveRequest
{
    u32 taxonomy;
};

struct PassiveSkillRequest
{
    u32 taxonomy;
};

struct UnlockCategoryRequest
{
    u32 taxonomy;
};

struct LevelUpRequest
{
    u32 taxonomy;
};

struct InstantiateRequest
{
    u32 taxonomy;
    Vec3 offset;
};

struct InstantiateRecipeRequest
{
    u32 taxonomy;
    GenerationData gen;
    Vec3 offset;
};

struct TileUpdate
{
    u32 chunkX;
    u32 chunkY;
    
    u32 tileX;
    u32 tileY;
    
    r32 waterAmount;
};

struct EntityUpdate
{
    Vec3 relativePos;
    u64 identifier;
    
    u32 equipmentPieceCount;
    u32 effectCount;
    u32 objectCount;
    
    
    
    u32 flags;
    u32 taxonomy;
    u64 recipeIndex;
    u8 action;
    
    r32 plantTotalAge;
    r32 plantStatusPercentage;
    u8 plantStatus;
    
    u32 recipeTaxonomy;
    u64 recipeRecipeIndex;
};

#if 0
enum EntityUpdateType
{
    PlayerEntityUpdate_Standard,
    PartialUpdate_Standard,
    PartialUpdate_Objects,
    CompleteUpdate_Standard,
    CompleteUpdate_Object,
    CompleteUpdate_Recipe,
    COmpleteUpdate_Plant,
    CompleteUpdate_Objects,
};

struct PlayerEntityUpdate
{
    u64 openedContainerID;
    ObjectsDelta;
    essencesDelta;
};

struct PartialEntityUpdate
{
    u64 identifier;
    Vec3 P;
    EntityAction action;
    u8 effectCount;
    // NOTE(Leonardo): additional data: _effectCount_ effects to add
};

struct PartialObjectsUpdate
{
    u64 identifier;
    u8 objectsDelta;
    // NOTE(Leonardo): additional data: objectCount to add
}

struct CompleteEntityUpdateStandard
{
    u64 identifier; // NOTE(Leonardo): 8 bytes
    u32 taxonomy; // NOTE(Leonardo): 4 bytes
    Vec3 P;
    u8 action;
    
    u32 effectCount;
    u32 equipmentCount;
};


struct CompleteEntityUpdateObject
{
    u64 identifier; // NOTE(Leonardo): 8 bytes
    u32 taxonomy; // NOTE(Leonardo): 4 bytes
    u64 recipeIndex;
    Vec3 P;
};


struct CompleteEntityUpdateRecipe
{
    u64 identifier; // NOTE(Leonardo): 8 bytes
    u32 recipeTaxonomy; // NOTE(Leonardo): 4 bytes
    u64 recipeRecipeIndex; // NOTE(Leonardo): 8 bytes
    Vec3 P;
};

struct CompleteEntityUpdatePlant
{
    u64 identifier; // NOTE(Leonardo): 8 bytes
    u32 taxonomy; // NOTE(Leonardo): 4 bytes
    
    // TODO(Leonardo): encode P in 6 bytes total when sending complete update
    // TODO(Leonardo): encode P in 3 bytes total when sending intra-frame update
    Vec3 P;
    
    r32 plantTotalAge;// TODO(Leonardo): 16 bits
    r32 plantStatusPercentage;// TODO(Leonardo): 13 bits
    PlantLifeStatus plantStatus; // TODO(Leonardo): 3 bits?
    u32 effectCount;
};

// NOTE(Leonardo): this is sent when player loots
struct CompleteEntityUpdateObjects
{
    u64 identifier;
    u32 objectCount;
    // NOTE(Leonardo): this contains all the objects of the entity
};
#endif

#ifdef FORG_SERVER
struct HashEntityUpdate
{
    b32 valid;
    u64 identifier;
    SimEntity entity;
};
#endif

struct EntityPossibleActions
{
    u64 identifier;
    u32 actionCount;
    b32 overlapping;
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
    u32 loggingIndex;
    u32 challenge;
};

struct UpdateAck
{
    UniversePos p;
    u32 sequenceNumber;
};

struct ServerInfo
{
    u16 port;
    u16 userPort;
    //TODO(leonardo):IP!
};

struct CompletedSpawning
{
    u64 identifier;
};





#if FORGIVENESS_INTERNAL
struct EditorModeRequest
{
    UniversePos p;
    i32 delta;
};

struct DebugEventVariableChanged
{
    u32 debugVariableIndex;
    u64 overNetwork[2];
};

struct DebugChangedAttributes
{
    u32 globalID;
    i32 worldID;
    u32 attributeIndex;
    r32 value;
};

struct DebugSpawnRequest
{
    u32 taxonomy;
    Vec2 offsetFromPlayer;
};
#endif
