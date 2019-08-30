#pragma once
#define MAX_TEXTURE_COUNT 256
#define MAX_SPECIAL_TEXTURE_COUNT 32

struct RenderTexture
{
    u32 index;
    u16 width;
    u16 height;
};

struct Bitmap
{
    void* pixels;
    u16 width;
    u16 height;
    
    Vec2 pivot;
    r32 nativeHeight;
    r32 widthOverHeight;
    
    RenderTexture textureHandle;
};

inline RenderTexture TextureHandle(u32 index, u16 width, u16 height)
{
    RenderTexture result;
    result.index = index;
    result.width = width;
    result.height = height;
    
    return result;
}

inline RenderTexture SpecialTextureHandle(u32 index, u16 width, u16 height)
{
    RenderTexture result;
    result.index = MAX_TEXTURE_COUNT + index;
    result.width = width;
    result.height = height;
}

inline void Clear(RenderTexture* texture)
{
    texture->index = 0;
    texture->width = 0;
    texture->height = 0;
}

inline b32 IsValid(RenderTexture* texture)
{
    b32 result = (texture->index > 0);
    return result;
}

struct TextureOpUpdate
{
    RenderTexture texture;
    void* data;
};

struct TextureOp
{
    TextureOpUpdate update;
    TextureOp* next;
};

#define PLATFORM_ALLOCATE_TEXTURE( name ) void* name( u32 width, u32 height, void* memory )
typedef PLATFORM_ALLOCATE_TEXTURE( platform_allocate_texture );

#define PLATFORM_DEALLOCATE_TEXTURE( name ) void name( void* texture )
typedef PLATFORM_DEALLOCATE_TEXTURE( platform_deallocate_texture );

