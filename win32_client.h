#pragma once

#include "win32_forg.h"
struct Win32ScreenBuffer
{
    u16 width;
    u16 height;
    void* memory;
    i32 bytesPerPixel;
    i32 pitch;
    BITMAPINFO info;
};

struct Win32SoundBuffer
{
    LPDIRECTSOUNDBUFFER buffer;
    i32 samplesPerSecond;
    i32 totalBufferSize;
    i32 runningSampleIndex;
    i32 bytesPerSample;
    i32 delaySamples;
};

struct Win32Dimension
{
    i32 width;
    i32 height;
};

struct Win32GameCode
{
    b32 isValid;
    HMODULE gameDLL;
    FILETIME lastWriteTime;
    game_update_and_render* UpdateAndRender;
    game_get_sound_output* GetSoundOutput;
    game_frame_end* DEBUGFrameEnd;
};
