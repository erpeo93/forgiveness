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
#include "../properties/test.properties"
#include "forg_world_generation.h"
#include "forg_game_effect.h"
#include "forg_ecs.h"
#include "forg_entity_layout.h"
#include "forg_meta.h"
#include "forg_render_tier.h"
#include "forg_render.h"
#include "forg_animation.h"
#include "forg_sound.h"
#include "forg_AI.h"
#include "forg_world.h"
#include "forg_editor.h"
#include "forg_network.h"
#include "forg_network_client.h"
#include "forg_particles.h"
#include "forg_bolt.h"
#include "forg_book.h"
#include "forg_cutscene.h"
#include "forg_game_ui.h"

#if FORGIVENESS_INTERNAL
#include "forg_debug.h"
#endif

struct BaseComponent
{
    EntityRef definitionID;
    u32 seed;
    u64 nameHash;
    UniversePos universeP;
    Vec3 velocity;
    Rect3 bounds;
    GameProperty action;
    u32 flags;
    Rect3 worldBounds;
    Rect2 projectedOnScreen;
    EntityID serverID;
};

internal r32 GetHeight(BaseComponent* base)
{
    r32 height = GetDim(base->bounds).z;
    return height;
}

internal r32 GetWidth(BaseComponent* base)
{
    r32 height = GetDim(base->bounds).x;
    return height;
}

internal r32 GetDeepness(BaseComponent* base)
{
    r32 height = GetDim(base->bounds).y;
    return height;
}

struct ImageReference
{
    u64 typeHash;
    GameProperties properties;
};

struct PlantComponent
{
    ImageReference leaf;
};

struct GrassComponent
{
    
};

struct StandardImageComponent
{
    ShadowComponent shadow;
    ImageReference entity;
};

struct LayoutPiece
{
    u64 nameHash;
    r32 height;
    ImageReference image;
};

struct LayoutComponent
{
    ShadowComponent shadow;
    Vec2 rootScale;
    r32 rootAngle;
    u64 rootHash;
    
    u32 pieceCount;
    LayoutPiece pieces[8];
    
    u32 openPieceCount;
    LayoutPiece openPieces[8];
    
    u32 usingPieceCount;
    LayoutPiece usingPieces[8];
};

enum LayoutContainerDrawMode
{
    LayoutContainerDraw_Standard,
    LayoutContainerDraw_Open,
    LayoutContainerDraw_Using,
    
};
struct LayoutContainer
{
    LayoutContainerDrawMode drawMode;
    u32 currentObjectIndex;
    ContainerMappingComponent* container;
};

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
    GameCommand currentCommand;
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

struct GameModeWorld
{
    struct GameState* gameState;
    
    u32 worldSeed;
    b32 editingEnabled;
    
    b32 gamePaused;
    r32 originalTimeToAdvance;
    r32 totalRunningTime;
    
    MemoryPool* persistentPool;
    MemoryPool* temporaryPool;
    
    WorldTile nullTile;
    WorldChunk* chunks[1024];
    ServerClientIDMapping* mappings[1024];
    ServerClientIDMapping* firstFreeMapping;
    
    ResizableArray archetypes[Archetype_Count];
    
    TempLight* firstFreeTempLight;
    
    RandomSequence entropy;
    ParticleCache* particleCache;
    BoltCache* boltCache;
    
    u32 loginFileToReceiveCount;
    u32 loginReceivedFileCount;
    ClientPlayer player;
    
#if FORGIVENESS_INTERNAL
    b32 replayingInput;
    b32 fixedTimestep;
    b32 canAdvance;
#endif
    
    Vec2 relativeMouseP;
    Vec2 deltaMouseP;
    
    u32 chunkApron;
    b32 worldTileView;
    b32 worldChunkView;
    Vec3 editorCameraOffset;
    r32 defaultCameraZ;
    
    b32 useDebugCamera;
    
    r32 debugCameraOrbit;
    r32 debugCameraPitch;
    r32 debugCameraDolly;
    
    r32 cameraOrbit;
    r32 cameraPitch;
    r32 cameraDolly;
    r32 cameraSpeed;
    Vec3 cameraWorldOffset;
    Vec3 destCameraWorldOffset;
    Vec2 cameraEntityOffset;
    Vec2 destCameraEntityOffset;
    
    SoundState* soundState;
    
    EditorUIContext editorUI;
    GameUIContext gameUI;
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