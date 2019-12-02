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
