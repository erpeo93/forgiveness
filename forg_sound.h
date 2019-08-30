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
    
    MemoryPool* pool;
};


struct SoundLabel
{
    u64 hashID;
    r32 value;
};

struct LabeledSound
{
    u64 typeHash;
    u64 nameHash;
    
    r32 delay;
    r32 decibelOffset;
    r32 pitch;
    r32 toleranceDistance;
    r32 distanceFalloffCoeff;
    union
    {
        LabeledSound* next;
        LabeledSound* nextFree;
    };
    
    u32 labelCount;
    SoundLabel labels[8];
};

printTable(noPrefix) enum SoundContainerType
{
    SoundContainer_Random,
    SoundContainer_Labeled,
    SoundContainer_Sequence,
};

struct SoundContainer
{
    SoundContainerType type;
    
    u32 soundCount;
    LabeledSound* firstSound;
    
    u32 containerCount;
    SoundContainer* firstChildContainer;
    
    union
    {
        SoundContainer* next;
        SoundContainer* nextFree;
    };
    
    u32 labelCount;
    SoundLabel labels[8];
};
