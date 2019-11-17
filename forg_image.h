#pragma once
#define STB_IMAGE_RESIZE_IMPLEMENTATION 1
#define STBIR_DEFAULT_FILTER_DOWNSAMPLE STBIR_FILTER_CATMULLROM
#include "stb_resize_image.h"

struct ImageU32
{
    u32 width;
    u32 height;
    u32* pixels;
};

struct MIPIterator
{
    u32 level;
    ImageU32 image;
};

inline MIPIterator BeginMIPs(u32 width, u32 height, u32* pixels)
{
    MIPIterator result = {};
    result.level = 0;
    result.image.width = width;
    result.image.height = height;
    result.image.pixels = pixels;
    
    return result;
}

inline b32 IsValid(MIPIterator* iterator)
{
    b32 result = (iterator->image.width > 0 && iterator->image.height > 0);
    return result;
}

inline void Advance(MIPIterator* iterator)
{
    iterator->image.pixels += (iterator->image.width * iterator->image.height);
    ++iterator->level;
    if(iterator->image.width == 1 && iterator->image.height == 1)
    {
        iterator->image.height = iterator->image.width = 0;
    }
    else
    {
        if(iterator->image.width > 1)
        {
            iterator->image.width = iterator->image.width / 2;
        }
        
        if(iterator->image.height > 1)
        {
            iterator->image.height = iterator->image.height / 2;
        }
    }
}
