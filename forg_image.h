#pragma once
#define STB_IMAGE_RESIZE_IMPLEMENTATION 1
#define STBIR_DEFAULT_FILTER_DOWNSAMPLE STBIR_FILTER_CATMULLROM
#include "stb_resize_image.h"

#define MAX_TEXTURE_DIM 512
global_variable r32 oneOverMaxImageDim = 1.0f / (r32) MAX_TEXTURE_DIM;

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

inline r32 OneOverDownsampleFactor(u32 width, u32 height)
{
    
#if 0
    r32 xDownsampleFactor = (r32) width * oneOverMaxImageDim;
    r32 yDownsampleFactor = (r32) height * oneOverMaxImageDim;
    r32 downsampleFactor = Max(xDownsampleFactor, yDownsampleFactor);
    r32 result = 1.0f / Max(downsampleFactor, 1.0f);
#else
    // TODO(Leonardo): if we now need to optimize this, just pass in one over width and one over height
    r32 oneOverWidth = 1.0f / (r32) width;
    r32 oneOverHeight = 1.0f / (r32) height;
    r32 oneOverXDownsampleFactor = (r32) MAX_TEXTURE_DIM * oneOverWidth;
    r32 oneOverYDownsampleFactor = (r32) MAX_TEXTURE_DIM * oneOverHeight;
    r32 oneOverDownsampleFactor = Min(oneOverXDownsampleFactor, oneOverYDownsampleFactor);
    r32 result = Min(oneOverDownsampleFactor, 1.0f);
#endif
    
    return result;
}

inline void Advance(MIPIterator* iterator)
{
    iterator->image.pixels += (iterator->image.width * iterator->image.height);
    
    if(iterator->image.width > MAX_TEXTURE_DIM || iterator->image.height > MAX_TEXTURE_DIM)
    {
        r32 oneOverDownsampleFactor = OneOverDownsampleFactor(iterator->image.width, iterator->image.height);
        iterator->image.width = RoundReal32ToU32(iterator->image.width * oneOverDownsampleFactor);
        iterator->image.height = RoundReal32ToU32(iterator->image.height * oneOverDownsampleFactor);
    }
    else
    {
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
}

inline Vec2 GetInvUV(u32 width, u32 height)
{
    r32 oneOverDownsampleFactor = OneOverDownsampleFactor(width, height);
    u32 newWidth = RoundReal32ToU32(width * oneOverDownsampleFactor);
    u32 newHeight = RoundReal32ToU32(height * oneOverDownsampleFactor);
    
    Vec2 result = V2((r32) newWidth * oneOverMaxImageDim, (r32) newHeight * oneOverMaxImageDim);
    
    return result;
}
