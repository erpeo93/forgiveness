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
#include "forg_simd.h"
#include "forg_random.h"
#include "forg_file_formats.h"
#include "asset_builder.h"

#define Property(name) enum Property_##name
#include "../properties/test.properties"

#include "forg_asset.h"
#include "forg_meta.h"
#include "forg_render_tier.h"
#include "forg_render.h"
#include "forg_mesh.h"
#include "forg_animation.h"
#include "forg_bound.h"
#include "forg_sound.h"
#include "forg_action_effect.h"
#include "forg_AI.h"
#include "forg_editor.h"
#include "forg_world.h"
#include "forg_physics.h"
#include "forg_network.h"
#include "forg_network_client.h"
#include "forg_world_generation.h"
#include "forg_particles.h"
#include "forg_bolt.h"
#include "forg_book.h"
#include "forg_cutscene.h"
#include "forg_plant.h"
#include "forg_rock.h"
#include "forg_essence.h"
#include "forg_skill.h"
#include "forg_ground.h"


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


struct ReceivingAssetFile
{
    MemoryPool memory;
    
    u16 type;
    u16 subtype;
    
    u32 uncompressedSize;
    u32 compressedSize;
    
    u32 chunkSize;
    u32 runningSize;
    u8* content;
};

struct ClientPlayer
{
    u32 identifier;
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
    
    ReceivingAssetFile* receiving;
};








struct ClientEntity
{
    u32 identifier;
    UniversePos universeP;
    Vec3 velocity;
    
    r32 timeFromLastUpdate;
    
    ClientEntity* next;
};

struct GameModeWorld
{
    struct GameState* gameState;
    
    
    ClientEntity* nearestEntities[8];
    r32 nearestCameraZ[8];
    
    WorldGeneratorDefinition* generator;
    WorldSeason season;
    r32 seasonLerp;
    
    
    u64 worldSeed;
    
    b32 editingEnabled;
    u32 editorRoles;
    
    b32 gamePaused;
    r32 originalTimeToAdvance;
    
    MemoryPool* persistentPool;
    MemoryPool* temporaryPool;
    
    ClientEntity* entities[1024];
    WorldChunk* chunks[1024];
    
    
    TicketMutex plantMutex;
    PlantSegment* firstFreePlantSegment;
    PlantStem* firstFreePlantStem;
    Plant* firstFreePlant;
    Rock* firstFreeRock;
    ClientAnimationEffect* firstFreeEffect;
    
    RandomSequence leafFlowerFruitSequence;
    ParticleCache* particleCache;
    
    RandomSequence boltSequence;
    r32 boltTime;
    BoltCache* boltCache;
    
    ClientPlayer player;
    
    
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
    
    EditorUIContext editorUI;
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