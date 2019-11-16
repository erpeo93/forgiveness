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
    GLuint lightStartingIndexID;
    GLuint lightEndingIndexID;
    GLuint textureIndexID;
    GLuint modulationID;
    GLuint lightInfluenceID;
    GLuint lightYInfluenceID;
    GLuint windInfluenceID;
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
    GLuint timeID;
    GLuint textureSamplerID;
    GLuint depthSamplerID;
    GLuint alphaThreesoldID;
    GLuint ambientLightColorID;
    GLuint directionalLightColorID;
    GLuint directionalLightDirectionID;
    GLuint directionalLightIntensityID;
    GLuint lightSource0ID;
    GLuint lightSource1ID;
    
    GLuint windDirectionID;
    GLuint windStrengthID;
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

struct TextureGenProgram
{
    OpenGLProgramCommon common;
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

struct Opengl
{
    GameRenderSettings settings;
    b32 multisampling;
    u32 depthPeelCount;
    
    OpenGLFramebuffer depthPeelBuffer[16];
    OpenGLFramebuffer resolveFramebuffer;
    OpenGLFramebuffer textureGenFrameBuffer;
    
    b32 shaderSimTexLoadSRGB;
    b32 shaderSimTexWriteSRGB;
    
    GLuint lightSource0;
    GLuint lightSource1;
    
    GLuint blitTextureHandle;
    GLuint defaultSpriteTextureFormat;
    GLuint defaultFramebufferTextureFormat;
    
    GLint maxMultisampleCount;
    b32 sRGBSupport;
    b32 supportSRGBFrameBuffer;
    
    ZBiasProgram zBiasNoDepthPeel;
    ZBiasProgram zBiasDepthPeelLight;
    PeelCompositeProgram peelComposite;
    FinalStretchProgram finalStretch;
    
    GLuint vertexBuffer;
    GLuint indexBuffer;
    GLuint screenFillVertexBuffer;
    
    u16 maxTextureCount;
    GLuint textureArray;
};
