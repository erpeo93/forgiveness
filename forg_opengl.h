#pragma once

struct OpenGLInfo
{
    b32 modernContext;
    char* vendor;
    char* renderer;
    char* version;
    char* shadingLanguageVersion;
    
    b32 GLEXTTextureSRGB;
    b32 GLARBFrameBufferSRGB;
    b32 GLARBFrameBufferObject;
};

struct OpenGLProgramCommon
{
    GLuint progHandle;
    GLuint vertUVID;
    GLuint vertPID;
    GLuint vertNID;
    GLuint vertColorID;
    GLuint lightIndexID;
    GLuint textureIndexID;
    GLuint modulationID;
};

struct GLPointLight
{
    GLuint colorID;
    GLuint posID;
    GLuint strengthID;
};

struct ZBiasProgram
{
    OpenGLProgramCommon common;
    
    GLuint GLSLTransformID;
    GLuint textureSamplerID;
    GLuint depthSamplerID;
    GLuint alphaThreesoldID;
    GLuint ambientLightColorID;
    
    GLPointLight pointLights[256];
};

struct PeelCompositeProgram
{
    OpenGLProgramCommon common;
    
    GLuint peelSamplerID[4];
};

struct FinalStretchProgram
{
    OpenGLProgramCommon common;
    GLuint textureSamplerID;
};


struct OpenGLFramebuffer
{
    GLuint handle;
    GLuint colorHandle;
    GLuint depthHandle;
};


enum OpenGLFramebufferFlags
{
    OpenGLFramebuffer_multisample = (1 << 1 ),
    OpenGLFramebuffer_linearFilter = (1 << 2 ),
    OpenGLFramebuffer_hasColor = (1 << 3 ),
    OpenGLFramebuffer_hasDepth = (1 << 4 )
};

enum OpenGLSpecialTextures
{
    OpenGLSpecial_GroundTexture,
    
    OpenGLSpecial_Count,
};

struct Opengl
{
    GameRenderSettings settings;
    b32 multisampling;
    u32 depthPeelCount;
    
    OpenGLFramebuffer depthPeelBuffer[16];
    OpenGLFramebuffer resolveFramebuffer;
    
    OpenGLFramebuffer specialTextures[OpenGLSpecial_Count];
    
    
    b32 shaderSimTexLoadSRGB;
    b32 shaderSimTexWriteSRGB;
    
    GLuint blitTextureHandle;
    GLuint defaultSpriteTextureFormat;
    GLuint defaultFramebufferTextureFormat;
    
    GLint maxMultisampleCount;
    b32 sRGBSupport;
    b32 supportSRGBFrameBuffer;
    
    ZBiasProgram zBiasNoDepthPeel;
    ZBiasProgram zBiasDepthPeel;
    PeelCompositeProgram peelComposite;
    FinalStretchProgram finalStretch;
    
    GLuint vertexBuffer;
    GLuint indexBuffer;
    GLuint screenFillVertexBuffer;
    
    u16 maxTextureCount;
    GLuint textureArray;
};
