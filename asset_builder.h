#pragma once
struct LoadedBitmap
{
    void* pixels;
    
    i32 width;
    i32 height;
    
    Vec2 pivot;
    
    r32 nativeHeight;
    r32 widthOverHeight;
    r32 downsampleFactor;
    
    void* free;
};

struct LoadedColoration
{
    Vec4 color;
    char imageName[64];
};

struct LoadedFont
{
    HFONT win32Font;
    TEXTMETRIC metrics;
    
    u32 onePastHighestCodePoint;
    
    r32 ascenderHeight;
    r32 descenderHeight;
    r32 externalLeading;
    
    u32 glyphsCount;
    u32 maximumGlyphsCount;
    
    u32 minCodePoint;
    u32 maxCodePoint;
    
    u32* glyphs;
    r32* horizontalAdvancement;
    
    u32* glyphIndexForCodePoint;
};

struct LoadedSound
{
    u32 countChannels;
    u32 countSamples;
    i16* samples[2];
    
    i16 maxSampleValue;
    r32 decibelLevel;
};


struct LoadedAnimation
{
    u16 durationMS;
    u16 syncThreesoldMS;
    u16 preparationThreesoldMS;
    
    u32 spriteInfoCount;
    u32 frameCount;
    
    struct SpriteInfo* spriteInfos;
    struct FrameData* frames;
    struct Bone* bones;
    struct PieceAss* ass;
    
    char name[64];
};

struct LoadedSkeleton
{
    u16 animationCount;
};

struct LoadedModel
{
    u32 vertexCount;
    u32 faceCount;
    
    Vec3 dim;
    
    struct ColoredVertex* vertexes;
    struct ModelFace* faces;
};
