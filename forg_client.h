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
#include "forg_random.h"
#include "forg_file_formats.h"
#include "forg_asset.h"
#define Property(name) enum name
#include "test.properties"
#include "forg_noise.h"
#include "forg_world_generation.h"
#include "forg_game_effect.h"
#include "forg_ecs.h"
#include "forg_entity_layout.h"
typedef u32 U32;
typedef u16 U16;
typedef r32 R32;
#define GetR32(value) value
#include "forg_skill.h"
#include "forg_entity.h"
#include "forg_meta.h"
#include "forg_render_tier.h"
#include "forg_render.h"
#include "forg_animation.h"
#include "forg_sound.h"
#include "forg_config.h"
#include "forg_world.h"
#include "forg_editor.h"
#include "forg_network.h"
#include "forg_network_client.h"
#include "forg_particles.h"
#include "forg_bolt.h"
#include "forg_cutscene.h"
#include "forg_game_ui.h"
#include "forg_render_entity.h"



#if FORGIVENESS_INTERNAL
#include "forg_debug.h"
#endif

struct BaseComponent
{
    EntityType type;
    u32 seed;
    UniversePos universeP;
    Vec3 velocity;
    u32 flags;
    Rect3 bounds;
    Rect3 worldBounds;
    Rect2 projectedOnScreen;
    EntityID serverID;
    EntityID draggingID;
    r32 timeSinceLastUpdate;
	r32 totalLifeTime;
    r32 lifeTimeSpeed;
    r32 deletedTime;
    
    r32 fadeInTime;
    r32 fadeOutTime;
    
    r32 occludingTime;
    b32 occludePlayerVisual;
    r32 occludeBoundsScale;
    
    u16 essences[Count_essence];
};

internal r32 GetHeight(Rect3 bounds)
{
    r32 height = GetDim(bounds).z;
    return height;
}

internal r32 GetWidth(Rect3 bounds)
{
    r32 height = GetDim(bounds).x;
    return height;
}

internal r32 GetDeepness(Rect3 bounds)
{
    r32 height = GetDim(bounds).y;
    return height;
}

struct PlantComponent
{
    Vec4 branchColor;
    b32 hasBranchVariant;
    
    ImageReference trunk;
    ImageReference branch;
    
    b32 hasLeafVariant;
    Vec4 leafColor;
    ImageReference leaf;
    
    b32 hasFlowerVariant;
    Vec4 flowerColor;
    ImageReference flower;
    
    b32 hasFruitVariant;
    Vec4 fruitColor;
    ImageReference fruit;
    
    r32 leafWindInfluence;
    r32 flowerWindInfluence;
    r32 fruitWindInfluence;
    r32 dissolveDuration;
    
    b32 scaleBranchesAccordingToTrunk;
    b32 scaleLeafAccordingToTrunk;
    
    r32 leafRandomAngle;
    r32 flowerRandomAngle;
    r32 fruitRandomAngle;
};

struct RockComponent
{
    Vec4 color;
    ImageReference rock;
    ImageReference mineral;
    
    r32 mineralDensity;
};

struct GrassComponent
{
    r32 windInfluence;
    u8 windFrequencyStandard;
    u8 windFrequencyOverlap;
    Vec3 maxOffset;
    u32 count;
    Rect3 bounds;
};

#include "forg_brain.h"
#include "forg_archetypes.h"
#include "client_generated.h"
global_variable ArchetypeLayout archetypeLayouts[Archetype_Count];
global_variable PlatformAPI platformAPI;

struct ReceivingAssetFile
{
    MemoryPool memory;
    
    u32 index;
    u16 type;
    char subtype[32];
    
    u32 uncompressedSize;
    u32 compressedSize;
    
    u32 chunkSize;
    u32 receivedSize;
    u8* content;
    
    u32 chunkCount;
    b32* receivedChunks;
    
    ReceivingAssetFile* prev;
    ReceivingAssetFile* next;
};

struct ClientPlayer
{
    EntityID serverID;
    EntityID clientID;
    
    UniversePos universeP;
    UniversePos oldUniverseP;
    
    ReceivingAssetFile receiveFileSentinel;
    
    u16 currentCommandIndex;
    GameCommand lastCommand;
};

struct ServerClientIDMapping
{
    EntityID serverID;
    EntityID clientID;
    
    union
    {
        ServerClientIDMapping* next;
        ServerClientIDMapping* nextFree;
    };
};

enum PlayingGameState
{
    PlayingGame_None,
    PlayingGame_Over,
    PlayingGame_Won,
};

enum RenderMode
{
    RenderMode_World,
    RenderMode_Entity,
    RenderMode_Particle,
    RenderMode_ParticleEntity,
    
    RenderMode_Count
};

struct GameModeWorld
{
    PlayingGameState state;
    u32 renderMode;
    
    ParticleEffectInstance* testEffect;
    
    r32 stateTime;
    
    u16 season;
    
    b32 resetDayTime;
    r32 dayTimeTime;
    u16 previousDayTime;
    u16 dayTime;
    
    r32 defaultZoomCoeff;
    r32 defaultZoomSpeed;
    
    r32 equipmentZoomCoeff;
    r32 equipmentZoomSpeed;
    
    Vec3 ambientLightColor;
    Vec3 windDirection;
    r32 windStrength;
    r32 windTime;
    
    struct GameState* gameState;
    
    u32 worldSeed;
    b32 editingEnabled;
    
    b32 gamePaused;
    r32 originalTimeToAdvance;
    
    MemoryPool* persistentPool;
    MemoryPool* temporaryPool;
    
    WorldChunk* chunks[1024];
    WorldChunk* firstFreeChunk;
    
    ServerClientIDMapping* mappings[1024];
    ServerClientIDMapping* firstFreeMapping;
    
    ResizableArray archetypes[Archetype_Count];
    
    TempLight* firstFreeTempLight;
    
    RandomSequence entropy;
    ParticleCache* particleCache;
    ParticleEffectInstance* weatherEffects[Weather_count];
    
    BoltDefinition boltDefinition;
    
    u32 loginFileToReceiveCount;
    u32 loginReceivedFileCount;
    ClientPlayer player;
    
#if FORGIVENESS_INTERNAL
    b32 replayingInput;
    b32 fixedTimestep;
    b32 canAdvance;
#endif
    
    Vec3 groundMouseP;
    Vec2 relativeMouseP;
    Vec2 deltaMouseP;
    
    b32 worldTileView;
    b32 worldChunkView;
    Vec3 editorCameraOffset;
    r32 defaultCameraZ;
    
    b32 useDebugCamera;
    
    r32 debugCameraOrbit;
    r32 debugCameraPitch;
    r32 debugCameraDolly;
    
    r32 cameraSpeed;
    Vec3 cameraWorldOffset;
    Vec3 destCameraWorldOffset;
    Vec2 cameraEntityOffset;
    Vec2 destCameraEntityOffset;
    
    EditorUIContext editorUI;
    GameUIContext gameUI;
};

inline void SetState(GameModeWorld* worldMode, PlayingGameState state)
{
    worldMode->state = state;
    worldMode->stateTime = 0;
}

struct GameState
{
    MemoryPool totalPool;
    MemoryPool modePool;
    MemoryPool audioPool;
    MemoryPool framePool;
    MemoryPool persistentPool;
    MemoryPool visualEffectsPool;
    MemoryPool assetsPool;
    
    union
    {
        //GameModeScene* cutscene;
        //GameModeTitleScreen* titleScreen;
        GameModeWorld* world;
    };
    
    Assets* assets;
    
    ReceiveNetworkPacketWork receiveNetworkPackets;
    ClientNetworkInterface networkInterface;
    
    PlatformWorkQueue* renderQueue;
    PlatformWorkQueue* slowQueue;
    PlatformTextureOpQueue* textureQueue;
    
    TaskWithMemory tasks[6];
    
    u32 musicPriority;
    SoundState soundState;
};

internal void PlayGame(GameState* gameState, PlatformInput* input);
//internal void SetGameMode(GameState* gameState, GameMode mode);