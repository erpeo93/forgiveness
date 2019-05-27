#pragma once


enum Asset_Type
{
    Pak_bitmap,
    Pak_coloration,
    Pak_sound,
    Pak_animation,
    Pak_font,
    Pak_fontGlyph,
    Pak_model,
};

struct LoadedBitmap
{
    void* pixels;
    
    i32 width;
    i32 height;
    
    Vec2 pivot;
    
    r32 nativeHeight;
    r32 widthOverHeight;
    r32 downsampleFactor;
    
    Vec4 coloration;
    void* free;
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
    
    PakGlyph* glyphs;
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
    
    SoundId nextToPlay;
    
    void* free;
};


struct AnimationHeader
{
    u64 nameHash;
    
    u16 durationMS;
    u16 syncThreesoldMS;
};

struct Bone
{
    u32 timeLineIndex;
    i32 id;
    Vec2 mainAxis;
    i32 parentID;
    r32 parentAngle;
    
    r32 finalAngle;
    Vec2 parentOffset;
    Vec2 finalOriginOffset;
    
    // NOTE(leonardo): if spin is -1, it means that we have to lerp the angle in the opposite way, means going clockwise instead of counter-clockwise
    i32 spin;
};

struct PieceAss
{
    u32 spriteIndex;
    u32 timeLineIndex;
    i32 boneID;
    Vec2 boneOffset;
    r32 additionalZOffset;
    r32 angle;
    Vec2 scale;
    r32 alpha;
    // NOTE(leonardo): if spin is -1, it means that we have to lerp the angle in the opposite way, means going clockwise instead of counter-clockwise
    i32 spin;
};

enum SpriteInfoFlags
{
    Sprite_Composed = (1 << 1),
    Sprite_Entity = (1 << 3),
};

struct SpriteInfo
{
    Vec2 pivot;
    u64 stringHashID;
    u8 index;
    char name[32];
    u32 flags;
};

struct FrameData
{
    u32 firstBoneIndex;
    u8 countBones;
    
    u32 firstAssIndex;
    u8 countAss;
    
    u16 timelineMS;
};

enum AnimationLoopingType
{
    Loop_none,
    Loop_normal,
    Loop_pingPong,
};


struct Animation
{
    AnimationHeader* header;
    
    u32 spriteInfoCount;
    SpriteInfo* spriteInfos;
    
    u32 frameCount;
    FrameData* frames;
    
    Bone* bones;
    PieceAss* ass;
};

struct LoadedAnimation
{
    u64 stringHashID;
    u16 durationMS;
    u16 syncThreesoldMS;
    
    u32 spriteInfoCount;
    SpriteInfo spriteInfos[64];
    
    u32 frameCount;
    FrameData frames[64];
    
    Bone* bones;
    PieceAss* ass;
    
    void* free;
};


struct ColoredVertex
{
    Vec3 P;
    Vec4 color;
};

struct ModelFace
{
    u16 i0;
    u16 i1;
    u16 i2;
};

struct LoadedModel
{
    u32 vertexCount;
    u32 faceCount;
    
    Vec3 dim;
    
    ColoredVertex* vertexes;
    ModelFace* faces;
};

struct VertexModel
{
    u32 vertexCount;
    u32 faceCount;
    
    Vec3 dim;
    
    ColoredVertex* vertexes;
    ModelFace* faces;
};

struct AssetBitmapSource
{
    char filename[64];
    char path[256];
};

struct AssetSoundSource
{
    char filename[64];
    u32 firstSampleIndex;
};

struct AssetAnimationSource
{
    char path[64];
    char filename[64];
    u32 animationIndex;
    
    AnimationHeader header;
};

struct AssetFontSource
{
    LoadedFont* font;
};

struct AssetGlyphSource
{
    LoadedFont* font;
    u32 codePoint;
};

struct AssetModelSource
{
    char filename[64];
    char path[256];
};

struct AssetSource
{
    
    u32 type;
    union
    {
        AssetBitmapSource bitmap;
        AssetSoundSource sound;
        AssetAnimationSource animation;
        AssetFontSource font;
        AssetGlyphSource glyph;
        AssetModelSource model;
    };
};