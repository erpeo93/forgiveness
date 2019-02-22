#pragma once
#define CLIENT_ENTITY_COUNT 512

#include "forg_basic_types.h"
#include "net.h"
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
#include "forg_rule.h"
#include "forg_plant.h"
#include "forg_crafting.h"
#include "forg_inventory.h"
#include "forg_fluid.h"
#include "forg_animation.h"
#include "forg_action_effect.h"
#include "forg_consideration.h"
#include "forg_taxonomy.h"
#include "forg_editor.h"
#include "forg_memory.h"
#include "forg_AI.h"
#include "forg_physics.h"
#include "forg_region.h"
#include "forg_asset.h"
#include "forg_network.h"
#include "forg_world_generation.h"
#include "forg_world.h"
#include "forg_sound.h"
#include "forg_particles.h"
#include "forg_book.h"
#include "forg_meta.h"
#include "forg_cutscene.h"
#include "forg_plant.h"

struct TileInfo
{
    u8 biome;
    Vec4 color;
    r32 height;
    Vec4 lightIndexes;
};


#define JC_VORONOI_IMPLEMENTATION
#undef internal
#include "jc_voronoi.h"
#define internal static

enum ForgDayPhase
{
    DayPhase_Day,
    DayPhase_Sunset,
    DayPhase_Night,
    
    DayPhase_Count,
};

struct DayPhase
{
    r32 duration;
    Vec3 ambientLightColor;
    ForgDayPhase next;
};

struct ReceiveNetworkPacketWork
{
    NetworkInterface** network;
    network_platform_receive_data_accept* ReceiveData;
};


global_variable PlatformAPI platformAPI;

struct ClientPlayer
{
    b32 spawnAsh;
    
    r32 serverFPS;
    r32 networkTimeElapsed;
    NetworkInterface* network;
    NetworkBuffer appRecv;
    
    b32 changedWorld;
    i32 changedWorldDeltaX;
    i32 changedWorldDeltaY;
    
    u64 identifier;
    i32 universeX;
    i32 universeY;
    UniversePos universeP;
    UniversePos oldUniverseP;
    UniversePos oldVoronoiP;
    
    Vec3 acceleration;
    Vec3 velocity;
    r32 distanceCoeffFromServerP;
    
    u64 targetIdentifier;
    b32 targetPossibleActions[Action_Count];
    
    u64 overlappingIdentifier;
    b32 overlappingPossibleActions[Action_Count];
    
    u64 openedContainerID;
    
    u32 essenceCount;
    EssenceSlot essences[MAX_DIFFERENT_ESSENCES];
    
    EditorTabStack editorStack;
};

struct ClientPlant
{
    RandomSequence sequence;
    PlantSegment* root;
    u32 leafCount;
    
    RandomSequence futureSequence;
    PlantSegment* futureRoot;
    u32 futureLeafCount;
    
    u32 stepDone;
    
    PlantLifeStatus oldStatus;
    r32 oldStatusPercentage;
    
    r32 defaultLeafSize;
    r32 loosedLeafPercentage;
    
    ClientPlant* nextFree;
};

struct ClientRock
{
    Vec3 dim;
    
    u32 vertexCount;
    ColoredVertex vertexes[128];
    
    u32 faceCount;
    ModelFace faces[64];
    
    
    ClientRock* nextFree;
};

enum PredictionType
{
    Prediction_None,
    Prediction_EquipmentRemoved,
    Prediction_EquipmentAdded,
    Prediction_ActionBegan,
};

struct ClientPrediction
{
    PredictionType type;
    
    r32 timeLeft;
    EquipInfo slot;
    EntityAction action;
    
    u32 taxonomy;
    u64 recipeIndex;
    u64 identifier;
};














struct ClientEntity
{
    // NOTE(Leonardo): from server
    u64 identifier; // NOTE(Leonardo): 8 bytes
    
    TaxonomySlot* slot;
    
    u32 flags; // TODO(Leonardo): we only need: deleted, equipped and if to draw it flipped
    u32 taxonomy; // NOTE(Leonardo): 4 bytes
    u64 recipeIndex; // NOTE(Leonardo): 8 bytes
    
    r32 status;
    r32 lifePoints;
    r32 maxLifePoints;
    
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
    u64 targetID;
    
    
    // TODO(Leonardo): send these only when sending a plant
    // TODO(Leonardo): encode these in 4 bytes total
    r32 plantTotalAge;// TODO(Leonardo): 16 bits
    r32 plantStatusPercentage;// TODO(Leonardo): 13 bits
    PlantLifeStatus plantStatus; // TODO(Leonardo): 3 bits?
    
    // TODO(Leonardo): send these only when sending a recipe entity
    u32 recipeTaxonomy;
    u64 recipeRecipeIndex;
    
    
    
    
    u32 effectCount;
    Effect effects[16];
    ContainedObjects objects;
    EquipmentSlot equipment[Slot_Count];
    
    
    
    // NOTE(Leonardo): internal to client
    AnimationState animation;
    r32 modulationWithFocusColor;
    r32 timeFromLastUpdate;
    
    u32 effectReferenceAction;
    AnimationEffect* firstActiveEffect;
    
    ClientRock* rock;
    ClientPlant* plant;
    
    
    ClientPrediction prediction;
    
    
    ClientEntity* next;
};

#include "forg_UI.h"


#define ALPHABET_LETTER_COUNT 24
struct GameModeWorld
{
    b32 editingEnabled;
    b32 gamePaused;
    
    MemoryPool filePool;
    struct DataFileArrived* firstDataFileArrived;
    struct DataFileArrived* firstPakFileArrived;
    struct DataFileArrived* currentFile;
    
    b32 allDataFilesArrived;
    b32 allPakFilesArrived;
    u32 patchSectionArrived;
    b32 canRender;
    
    
    r32 currentPhaseTimer;
    ForgDayPhase currentPhase;
    DayPhase dayPhases[DayPhase_Count];
    
    
    u8 chunkDim;
    r32 voxelSide;
    r32 oneOverVoxelSide;
    r32 chunkSide;
    r32 oneOverChunkSide;
    
    MemoryPool entityPool;
    MemoryPool chunkPool;
    
    MemoryPool* temporaryPool;
    
    ClientEntity* entities[1024];
    PlantSegment* firstFreePlantSegment;
    ClientPlant* firstFreePlant;
    ClientRock* firstFreeRock;
    AnimationEffect* firstFreeEffect;
    
    
    WorldChunk* chunks[1024];
    
    
    ParticleCache* particleCache;
    
    
    TaxonomyTable* table;
    TaxonomyTable* oldTable;
    UIState* UI;
    ClientPlayer player;
    
    Vec3 cameraWorldOffset;
    Vec3 destCameraWorldOffset;
    Vec2 cameraEntityOffset;
    Vec2 destCameraEntityOffset;
    
    r32 defaultCameraZ;
    u64 cameraFocusID;
    
    
    r32 modulationWithFocusColor;
    
    
    VertexModel tetraModel;
    
    
#if FORGIVENESS_INTERNAL
    
    b32 replayingInput;
    b32 fixedTimestep;
    b32 canAdvance;
    b32 editorMode;
#endif
    
    Vec2 lastMouseP;
    Vec3 worldMouseP;
    
    
    r32 colorChunkyness;
    b32 voronoiGenerated;
    jcv_diagram voronoi;
    UniversePos voronoiP;
	Vec3 voronoiDelta;
    
    
    b32 useDebugCamera;
    
    r32 debugCameraOrbit;
    r32 debugCameraPitch;
    r32 debugCameraDolly;
    
    r32 cameraOrbit;
    r32 cameraPitch;
    r32 cameraDolly;
    
    PlayingSound* music;
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
    
    GameMode mode;
    union
    {
        GameModeScene* cutscene;
        GameModeTitleScreen* titleScreen;
        GameModeWorld* world;
    };
    
    
    r32 timeCoeff;
    
    u32 lock;
    
    Assets* assets;
    MemoryPool assetPool;
    
    ReceiveNetworkPacketWork receivePacketWork;
    PlatformWorkQueue* renderQueue;
    PlatformWorkQueue* slowQueue;
    PlatformTextureOpQueue* textureQueue;
    
    TaskWithMemory tasks[6];
    
    SoundState soundState;
};

internal void SetGameMode(GameState* gameState, GameMode mode);
internal void PlayGame(GameState* gameState, PlatformInput* input);