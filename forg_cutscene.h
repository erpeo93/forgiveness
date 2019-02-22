#define CUTSCENE_WARMUP_TIME 1.0f

enum SceneLayerFlags
{
    SceneLayerFlag_AtInfinity = ( 1 << 1 ),
    SceneLayerFlag_CounterX = ( 1 << 2 ),
    SceneLayerFlag_CounterY = ( 1 << 3 ),
    SceneLayerFlag_CounterZ = ( 1 << 4 ),
    SceneLayerFlag_Transient = ( 1 << 5 ),
};

struct SceneLayer
{
    Vec3 P;
    r32 height;
    u32 flags;
    
    r32 minTime;
    r32 maxTime;
};

struct LayeredScene
{
    r32 duration;
    
    u32 layerCount;
    u32 shotIndex;
    
    SceneLayer* layers;
    
    Vec3 cameraStart;
    Vec3 cameraEnd;
    
    r32 tFade;
};


struct Cutscene
{
    u32 shotCount;
    LayeredScene* shots;
};

enum CutsceneID
{
    CutsceneID_Intro,
};

struct GameModeScene
{
    CutsceneID ID;
    r32 t;
};

struct GameModeTitleScreen
{
    r32 t;
};