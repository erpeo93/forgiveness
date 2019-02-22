#pragma once


enum Asset_Type
{
    Pak_bitmap,
    Pak_sound,
    Pak_animation,
    Pak_font,
    Pak_fontGlyph,
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
    i32 countSamples;
    i16* samples[2];
    
    SoundId nextToPlay;
    
    void* free;
};


struct AnimationHeader
{
    u16 durationMS;
    u16 spawningMS;
    
    u32 loopingType;
    u16 startingPreparationTimeLine;
    u16 endingPreparationTimeLine;
    
    u32 waitingLoopingType;
    u16 startingWaitingTimeLine;
    u16 endingWaitingTimeLine;
    b32 singleCycle;
    r32 barriers[3];
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
    r32 angle;
    Vec2 scale;
    r32 alpha;
    // NOTE(leonardo): if spin is -1, it means that we have to lerp the angle in the opposite way, means going clockwise instead of counter-clockwise
    i32 spin;
};

enum SpriteInfoFlags
{
    Sprite_Composed = (1 << 1),
    Sprite_EmptySpace = (1 << 2),
    Sprite_Entity = (1 << 3),
    Sprite_SubPart = (1 << 4),
};

struct SpriteInfo
{
    Vec2 pivot;
    u64 stringHashID;
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
    b32 singleCycle;
    
    u32 spriteInfoCount;
    SpriteInfo spriteInfos[64];
    
    u32 frameCount;
    FrameData frames[16];
    
    Bone* bones;
    PieceAss* ass;
    
    void* free;
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
    };
};