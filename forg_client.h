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
#include "asset_builder.h"
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
#include "forg_mesh.h"
#include "forg_animation.h"
#include "forg_sound.h"
#include "forg_AI.h"
#include "forg_world.h"
#include "forg_editor.h"
#include "forg_physics.h"
#include "forg_network.h"
#include "forg_network_client.h"
#include "forg_particles.h"
#include "forg_bolt.h"
#include "forg_book.h"
#include "forg_cutscene.h"
#include "forg_plant.h"
#include "forg_rock.h"
#include "forg_ground.h"

#if FORGIVENESS_INTERNAL
#include "forg_debug.h"
#endif

struct BaseComponent
{
    u32 seed;
    u64 nameHash;
    UniversePos universeP;
    Vec3 velocity;
    Rect3 bounds;
    GameProperty action;
    u32 flags;
    b32 isOnFocus;
    EntityID serverID;
};

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
};

struct LayoutContainer
{
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

enum EntityInteractionType
{
    EntityInteraction_Standard,
    EntityInteraction_Inventory,
    EntityInteraction_Container,
};

struct EntityHotInteraction
{
    u32 type;
    EntityID ID;
};

inline b32 AreEqual(EntityHotInteraction i1, EntityHotInteraction i2)
{
    b32 result = (i1.type == i2.type && AreEqual(i1.ID, i2.ID));
    return result;
}

struct GameModeWorld
{
    struct GameState* gameState;
    
    b32 inventoryMode;
    EntityID openIDLeft;
    EntityID openIDRight;
    
    u32 worldSeed;
    b32 editingEnabled;
    u32 editorRoles;
    
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
    
    TicketMutex plantMutex;
    PlantSegment* firstFreePlantSegment;
    PlantStem* firstFreePlantStem;
    Plant* firstFreePlant;
    
    RandomSequence entropy;
    RandomSequence leafFlowerFruitSequence;
    ParticleCache* particleCache;
    
    RandomSequence boltSequence;
    r32 boltTime;
    BoltCache* boltCache;
    
    u32 loginFileToReceiveCount;
    u32 loginReceivedFileCount;
    ClientPlayer player;
    
#if FORGIVENESS_INTERNAL
    b32 replayingInput;
    b32 fixedTimestep;
    b32 canAdvance;
#endif
    
    Vec2 screenMouseP;
    
    
    b32 voronoiValid;
    b32 generatingVoronoi;
    VoronoiDiagram voronoiPingPong[2];
    VoronoiDiagram* activeDiagram;
    
    
    Vec3 cameraWorldOffset;
    Vec3 destCameraWorldOffset;
    Vec2 cameraEntityOffset;
    Vec2 destCameraEntityOffset;
    
    u32 chunkApron;
    b32 worldTileView;
    b32 worldChunkView;
    Vec3 additionalCameraOffset;
    r32 defaultCameraZ;
    
    b32 useDebugCamera;
    
    r32 debugCameraOrbit;
    r32 debugCameraPitch;
    r32 debugCameraDolly;
    
    r32 cameraOrbit;
    r32 cameraPitch;
    r32 cameraDolly;
    
    SoundState* soundState;
    
    EditorUIContext editorUI;
    
    i32 currentHotIndex;
    u32 hotCount;
    EntityHotInteraction hotInteractions[8];
    EntityHotInteraction lastFrameHotInteraction;
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