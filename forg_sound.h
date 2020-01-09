#pragma once
struct PlayingSound
{
    AssetID ID;
    r32 playingIndex;
    Vec2 currentVolume;
    Vec2 targetVolume;
    Vec2 dVolume;
    r32 delay;
    r32 dSample;
    PlayingSound* next;
};

struct SoundState
{
    Vec2 masterVolume;
    PlayingSound* firstPlayingSound;
    PlayingSound* firstFreeSound;
    PlayingSound* music;
    MemoryPool* pool;
};

struct SoundTrig
{
    u64 subtypeHash;
    GameProperties properties;
};

struct SoundMapping
{
    u32 count;
    u32 maxCount;
    
    r32 time;
    r32 targetTime;
    SoundTrig trig;
};

introspection() struct SoundMappingDefinition
{
    r32 targetTime MetaDefault("1.0f");
    u32 maxRepeatCount;
    
    GameAssetType asset MetaDefault("{AssetType_Sound, 0}") MetaFixed(type);
    ArrayCounter propertyCount MetaCounter(properties);
    GameProperty* properties;
};

introspection() struct SoundEffectDefinition
{
    r32 time;
    
    b32 musicTrigger;
    u32 musicPriority;
    
    b32 playerTrigger;
    GameProperty triggerType;
    SoundMappingDefinition sound;
};

struct SoundEffect
{
    u32 ID;
    r32 time;
    SoundMapping sound;
};

struct SoundEffectComponent
{
    u32 soundCount;
    SoundEffect sounds[8];
};