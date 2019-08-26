#pragma once
#include "forg_basic_types.h"
#include "ll_net.h"
#include "forg_platform.h"
#include "forg_shared.h"
#include "forg_token.h"
#include "forg_intrinsics.h"
#include "forg_math.h"
#include "forg_simd.h"
#include "forg_random.h"
#include "forg_asset_enum.h"
#include "forg_file_formats.h"
#include "asset_builder.h"
#include "forg_render_tier.h"
#include "forg_render.h"
#include "forg_mesh.h"
#include "forg_animation.h"
#include "forg_bound.h"
#include "forg_sound.h"
#include "forg_taxonomy.h"
#include "forg_entity.h"
#include "forg_object.h"
#include "forg_crafting.h"
#include "forg_inventory.h"
#include "forg_action_effect.h"
#include "forg_AI.h"
#include "forg_editor.h"
#include "forg_asset.h"
#include "forg_world.h"
#include "forg_memory.h"
#include "forg_physics.h"
#include "forg_network.h"
#include "forg_network_client.h"
#include "forg_world_generation.h"
#include "forg_particles.h"
#include "forg_bolt.h"
#include "forg_book.h"
#include "forg_meta.h"
#include "forg_cutscene.h"
#include "forg_plant.h"
#include "forg_rock.h"
#include "forg_essence.h"
#include "forg_skill.h"
#include "forg_ground.h"
#include "forg_region.h"


global_variable PlatformAPI platformAPI;


printTable(noPrefix) enum ForgDayPhase
{
    DayPhase_Day,
    DayPhase_Sunrise,
    DayPhase_Morning,
    DayPhase_Sunset,
    DayPhase_Dusk,
    DayPhase_Night,
};

struct DayPhase
{
    r32 duration;
    Vec3 ambientLightColor;
    ForgDayPhase next;
};

struct ClientPlayer
{
    b32 spawnAsh;
    
    u64 identifier;
    UniversePos universeP;
    UniversePos oldUniverseP;
    UniversePos oldVoronoiP;
    
    Vec3 acceleration;
    Vec3 velocity;
    r32 distanceCoeffFromServerP;
    
    u64 targetIdentifier;
    PossibleActionType targetPossibleActions[Action_Count];
    
    u64 overlappingIdentifier;
    PossibleActionType overlappingPossibleActions[Action_Count];
    
    u64 openedContainerID;
    
    u32 essenceCount;
    EssenceSlot essences[MAX_DIFFERENT_ESSENCES];
};












struct ClientEntity
{
    // NOTE(Leonardo): from server
    u64 identifier; // NOTE(Leonardo): 8 bytes
    
    b32 beingDeleted;
    u32 flags;
    u32 taxonomy; // NOTE(Leonardo): 4 bytes
    GenerationData gen; // NOTE(Leonardo): 8 bytes
    r32 generationIntensity;
    
    r32 status;
    
    r32 lifePoints;
    r32 maxLifePoints;
    
    r32 stamina;
    r32 maxStamina;
    
    b32 showHUD;
    r32 lifePointsTriggerTime;
    r32 staminaTriggerTime;
    
    r32 lightIntensity;
    
    
    // TODO(Leonardo): encode P in 6 bytes total when sending complete update
    // TODO(Leonardo): encode P in 3 bytes total when sending intra-frame update
    UniversePos universeP;
    Vec3 velocity;
    Vec3 P;
    
    ForgBoundType boundType;
    Rect3 bounds;
    
    // TODO(Leonardo): encode this as a single byte
    EntityAction action;
    r32 actionTime;
    u64 actionID;
    u64 draggingID;
    
    
    EntityAction ownerAction;
    SlotName ownerSlot;
    
    // TODO(Leonardo): send these only when sending a recipe entity
    u32 recipeTaxonomy;
    GenerationData recipeGen;
    
    
    
    
    u32 effectCount;
    Effect effects[16];
    ContainedObjects objects;
    EquipmentSlot equipment[Slot_Count];
    
    
    
    // NOTE(Leonardo): internal to client
    AnimationState animation;
    r32 modulationWithFocusColor;
    r32 timeFromLastUpdate;
    
    u32 effectReferenceAction;
    
    ClientAnimationEffect* firstActiveEffect;
    
    Rock* rock;
    Plant* plant;
    
    
    ClientPrediction prediction;
    
    
    ClientEntity* next;
};

inline void AddFlags(ClientEntity* entity, u32 flags)
{
    entity->flags |= flags;
}

inline b32 IsSet(ClientEntity* entity, i32 flag)
{
    b32 result = (entity->flags & flag);
    return result;
}

inline void ClearFlags(ClientEntity* entity, i32 flags)
{
    entity->flags &= ~flags;
}

struct ToDeleteFile
{
    char filename[64];
    b32 toDelete;
    
    union
    {
        ToDeleteFile* next;
        ToDeleteFile* nextFree;
    };
};

struct GameModeWorld
{
    struct GameState* gameState;
    
    ClientEntity* nearestEntities[8];
    r32 nearestCameraZ[8];
    
    WorldGeneratorDefinition* generator;
    WorldSeason season;
    r32 seasonLerp;
    
    
    u32 worldSeed;
    
    b32 editingEnabled;
    u32 editorRoles;
    
    b32 gamePaused;
    r32 originalTimeToAdvance;
    
    MemoryPool filePool;
    struct DataFileArrived* firstDataFileArrived;
    struct DataFileArrived* firstPakFileArrived;
    struct DataFileArrived* currentFile;
    
    
    MemoryPool deletedFilesPool;
    ToDeleteFile* firstFileToDelete;
    ToDeleteFile* firstFreeFileToDelete;
    
    
    DataFileSentType dataFileSent;
    
    b32 patchingLocalServer;
    b32 allPakFilesArrived;
    u32 patchSectionArrived;
    
    
    r32 currentPhaseTimer;
    ForgDayPhase currentPhase;
    
    TempLight* firstFreeTempLight;
    
    
    b32 forceVoronoiRegeneration;
    u8 chunkDim;
    r32 voxelSide;
    r32 oneOverVoxelSide;
    r32 chunkSide;
    r32 oneOverChunkSide;
    
    MemoryPool* persistentPool;
    MemoryPool* temporaryPool;
    
    TicketMutex entityMutex;
    ClientEntity* entities[1024];
    
    
    WorldChunk* chunks[1024];
    
    
    TicketMutex plantMutex;
    PlantSegment* firstFreePlantSegment;
    PlantStem* firstFreePlantStem;
    Plant* firstFreePlant;
    Rock* firstFreeRock;
    ClientAnimationEffect* firstFreeEffect;
    
    RandomSequence leafFlowerFruitSequence;
    RandomSequence waterRipplesSequence;
    ParticleCache* particleCache;
    
    RandomSequence boltSequence;
    r32 boltTime;
    BoltCache* boltCache;
    
    TaxonomyTable* table;
    TaxonomyTable* oldTable;
    
    ClientPlayer player;
    
    r32 modulationWithFocusColor;
    
    
#if FORGIVENESS_INTERNAL
    b32 replayingInput;
    b32 fixedTimestep;
    b32 canAdvance;
#endif
    
    Vec3 worldMouseP;
    Vec2 relativeScreenMouseP;
    
    
    
    
    b32 voronoiValid;
    b32 generatingVoronoi;
    
    VoronoiDiagram voronoiPingPong[2];
    VoronoiDiagram* activeDiagram;
    WorldTile nullTile;
    
    
    
    
    Vec3 cameraWorldOffset;
    Vec3 destCameraWorldOffset;
    Vec2 cameraEntityOffset;
    Vec2 destCameraEntityOffset;
    
    r32 defaultCameraZ;
    u64 cameraFocusID;
    
    b32 useDebugCamera;
    
    r32 debugCameraOrbit;
    r32 debugCameraPitch;
    r32 debugCameraDolly;
    
    r32 cameraOrbit;
    r32 cameraPitch;
    r32 cameraDolly;
    
    
    
    SoundState* soundState;
    
};

enum GameMode
{
    GameMode_Launcher,
    GameMode_TitleScreen,
    GameMode_Cutscene,
    GameMode_Playing,
};

struct GameState
{
    MemoryPool totalPool;
    MemoryPool modePool;
    MemoryPool audioPool;
    MemoryPool framePool;
    MemoryPool persistentPool;
    MemoryPool visualEffectsPool;
    MemoryPool assetsPool;
    
    GameMode mode;
    union
    {
        GameModeScene* cutscene;
        GameModeTitleScreen* titleScreen;
        GameModeWorld* world;
    };
    
    
    r32 timeCoeff;
    u32 lock;
    
    u32 editorRoles;
    
    Assets* assets;
    
    ReceiveNetworkPacketWork receiveNetworkPackets;
    ClientNetworkInterface networkInterface;
    
    PlatformWorkQueue* renderQueue;
    PlatformWorkQueue* slowQueue;
    PlatformTextureOpQueue* textureQueue;
    
    TaskWithMemory tasks[6];
    
    PlayingSound* music;
    SoundState soundState;
};

internal void PlayGame(GameState* gameState, PlatformInput* input);
internal void SetGameMode(GameState* gameState, GameMode mode);