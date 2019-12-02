#include "forg_render.h"
#define MIPMAPPING 1
#define GL_MAJOR_VERSION                  0x821B
#define GL_MINOR_VERSION                  0x821C
#define GL_NUM_EXTENSIONS                 0x821D

#define GL_ARRAY_BUFFER                   0x8892
#define GL_ELEMENT_ARRAY_BUFFER           0x8893
#define GL_UNIFORM_BUFFER                 0x8A11
#define GL_STREAM_DRAW                    0x88E0
#define GL_STREAM_READ                    0x88E1
#define GL_STREAM_COPY                    0x88E2
#define GL_STATIC_DRAW                    0x88E4
#define GL_STATIC_READ                    0x88E5
#define GL_STATIC_COPY                    0x88E6
#define GL_DYNAMIC_DRAW                   0x88E8
#define GL_DYNAMIC_READ                   0x88E9
#define GL_DYNAMIC_COPY                   0x88EA

#define GL_DEBUG_OUTPUT_SYNCHRONOUS       0x8242

#define GL_CLAMP_TO_EDGE                  0x812F
#define GL_FRAMEBUFFER_SRGB               0x8DB9
#define GL_SRGB8_ALPHA8                   0x8C43
#define GL_RGBA32F                        0x8814

#define GL_SHADING_LANGUAGE_VERSION       0x8B8C
#define GL_FRAGMENT_SHADER                0x8B30
#define GL_VERTEX_SHADER                  0x8B31
#define GL_COMPILE_STATUS                 0x8B81
#define GL_LINK_STATUS                    0x8B82
#define GL_VALIDATE_STATUS                0x8B83

#define GL_FRAMEBUFFER                    0x8D40
#define GL_READ_FRAMEBUFFER               0x8CA8
#define GL_DRAW_FRAMEBUFFER               0x8CA9
#define GL_COLOR_ATTACHMENT0              0x8CE0
#define GL_DEPTH_ATTACHMENT               0x8D00
#define GL_FRAMEBUFFER_COMPLETE           0x8CD5

#define GL_DEPTH_COMPONENT16              0x81A5
#define GL_DEPTH_COMPONENT24              0x81A6
#define GL_DEPTH_COMPONENT32              0x81A7
#define GL_DEPTH_COMPONENT32F             0x8CAC

#define GL_MULTISAMPLE                    0x809D
#define GL_SAMPLE_ALPHA_TO_COVERAGE       0x809E
#define GL_SAMPLE_ALPHA_TO_ONE            0x809F
#define GL_SAMPLE_COVERAGE                0x80A0
#define GL_SAMPLE_BUFFERS                 0x80A8
#define GL_SAMPLES                        0x80A9
#define GL_SAMPLE_COVERAGE_VALUE          0x80AA
#define GL_SAMPLE_COVERAGE_INVERT         0x80AB
#define GL_TEXTURE_2D_MULTISAMPLE         0x9100
#define GL_MAX_SAMPLES                    0x8D57
#define GL_MAX_COLOR_TEXTURE_SAMPLES      0x910E
#define GL_MAX_DEPTH_TEXTURE_SAMPLES      0x910F

#define GL_TEXTURE0                       0x84C0
#define GL_TEXTURE1                       0x84C1
#define GL_TEXTURE2                       0x84C2
#define GL_TEXTURE3                       0x84C3
#define GL_TEXTURE4                       0x84C4
#define GL_TEXTURE5                       0x84C5
#define GL_TEXTURE6                       0x84C6
#define GL_TEXTURE7                       0x84C7
#define GL_TEXTURE8                       0x84C8
#define GL_TEXTURE9                       0x84C9
#define GL_TEXTURE10                      0x84CA

#define GL_TEXTURE_2D_ARRAY               0x8C1A

// NOTE(Leonardo): windows-specific
#define WGL_CONTEXT_MAJOR_VERSION_ARB           0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB           0x2092
#define WGL_CONTEXT_LAYER_PLANE_ARB             0x2093
#define WGL_CONTEXT_FLAGS_ARB                   0x2094
#define WGL_CONTEXT_PROFILE_MASK_ARB            0x9126

#define WGL_CONTEXT_DEBUG_BIT_ARB               0x0001
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB  0x0002

#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB 0x00000001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x00000002
#define WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB 0x20A9

#define GL_DEBUG_SEVERITY_NOTIFICATION    0x826B
//#define GL_DEBUG_CALLBACK(name) void WINAPI name(GLenum source,GLenum type,GLuint id,GLenum severity,GLsizei length,const GLchar *message,const void *userParam)
GL_DEBUG_CALLBACK(OpenGLDebugCallback)
{
    if(severity != GL_DEBUG_SEVERITY_NOTIFICATION)
    {
        char* errorMessage = (char*) message;
        InvalidCodePath;
        Assert(!"openGL error encountered!");
    }
}

internal OpenGLInfo OpenGLGetInfo(b32 modernContext)
{
    OpenGLInfo result = {};
    result.vendor = (char*) glGetString(GL_VENDOR);
    result.renderer = (char*) glGetString(GL_RENDERER);
    result.version = (char*) glGetString(GL_VERSION);
    
    result.modernContext = modernContext;
    if(modernContext)
    {
        result.shadingLanguageVersion = (char*) glGetString(GL_SHADING_LANGUAGE_VERSION);
    }
    else
    {
        result.shadingLanguageVersion = "none";
    }
    
    if(glGetStringi)
    {
        GLint extensionCount;
        glGetIntegerv(GL_NUM_EXTENSIONS, &extensionCount);
        for(i32 extensionIndex = 0; extensionIndex < extensionCount; ++extensionIndex)
        {
            char* ext = (char*) glGetStringi(GL_EXTENSIONS, extensionIndex);
            
            if(StrEqual(ext, "GL_EXT_texture_sRGB")) { result.GLEXTTextureSRGB = true; }
            else if(StrEqual(ext, "GL_ARB_texture_sRGB")) { result.GLEXTTextureSRGB = true; }
            else if(StrEqual(ext, "GL_EXT_framebuffer_sRGB")) { result.GLARBFrameBufferSRGB = true; }
            else if(StrEqual(ext, "GL_ARB_framebuffer_sRGB")) { result.GLARBFrameBufferSRGB = true; }
            else if(StrEqual(ext, "GL_ARB_framebuffer_object")) { result.GLARBFrameBufferObject = true; }
        }
    }
    
    char* majorAt = result.version;
    char* minorAt = 0;
    
    for(char* atV = result.version; *atV; ++atV)
    {
        if(atV[0] == '.')
        {
            minorAt = atV + 1;
            break;
        }
    }
    GLint major = 0;
    GLint minor = 0;
    
    if(minorAt)
    {
        major = I32FromChar(majorAt);
        minor = I32FromChar(minorAt);
    }
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
    
    if(major >= 3 || (major == 2 && minor >= 1))
    {
        result.GLEXTTextureSRGB = true;
    }
    
    return result;
}

inline void OpenGLBindFramebuffer(OpenGLFramebuffer* frameBuffer, u32 renderWidth, u32 renderHeight)
{
    glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer ? frameBuffer->handle : 0);
    glViewport(0, 0, renderWidth, renderHeight);
}

inline OpenGLFramebuffer OpenGLCreateFramebuffer(u32 flags, u32 width, u32 height)
{
    OpenGLFramebuffer result = {};
    
    u32 maxMultisampleCount = 4;
    b32 isMultisample = (flags & OpenGLFramebuffer_multisample);
    b32 isLinearFilter = (flags & OpenGLFramebuffer_linearFilter);
    b32 hasColor = (flags & OpenGLFramebuffer_hasColor);
    b32 hasDepth = (flags & OpenGLFramebuffer_hasDepth);
    
    GLuint slot = isMultisample ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
    GLint filter = isLinearFilter ? GL_LINEAR : GL_NEAREST;
    
    glGenFramebuffers(1, &result.handle);
    glBindFramebuffer(GL_FRAMEBUFFER, result.handle);
    
    if(hasColor)
    {
        glGenTextures(1, &result.colorHandle);
        glBindTexture(slot, result.colorHandle);
        Assert(glGetError() == GL_NO_ERROR);
        
        if(slot == GL_TEXTURE_2D_MULTISAMPLE)
        {
            glTexImage2DMultisample(slot, maxMultisampleCount,
                                    opengl.defaultFramebufferTextureFormat,
                                    width, height,
                                    GL_FALSE);
        }
        else
        {
            glTexImage2D(slot, 0,
                         opengl.defaultFramebufferTextureFormat,
                         width, height, 0,
                         GL_BGRA_EXT, GL_UNSIGNED_BYTE, 0);
        }
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);    
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);    
        Assert(glGetError() == GL_NO_ERROR);
        
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, slot, result.colorHandle, 0);
    }
    
    if(hasDepth)
    {
        glGenTextures(1, &result.depthHandle);
        glBindTexture(slot, result.depthHandle);
        
        if(slot == GL_TEXTURE_2D_MULTISAMPLE)
        {
            
            glTexImage2DMultisample(slot, maxMultisampleCount,
                                    GL_DEPTH_COMPONENT32F,
                                    width, height,
                                    GL_FALSE);
            
        }
        else
        {
            glTexImage2D(slot, 0,
                         GL_DEPTH_COMPONENT32F,
                         width, height, 0,
                         GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, 0);
        }
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);    
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);    
        
        Assert(glGetError() == GL_NO_ERROR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, slot, result.depthHandle, 0);
    }
    
    glBindTexture(slot, 0);
    
    GLenum Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    Assert(Status == GL_FRAMEBUFFER_COMPLETE);
    
    return result;
}

internal void OpenGLAllocateTexture(RenderTexture texture, void* data)
{
#if 0    
    //GLuint handle;
    //glGenTextures(1, &handle);
    glBindTexture(GL_TEXTURE_2D, textureHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, opengl.defaultSpriteTextureFormat, width, height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, data);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);    
    
    glBindTexture(GL_TEXTURE_2D, 0);
#endif
    
    u16 textureHandle = (u16)texture.index;
    Assert(textureHandle < opengl.maxTextureCount);
    
    glBindTexture(GL_TEXTURE_2D_ARRAY, opengl.textureArray);
    
#if MIPMAPPING
    for(MIPIterator mip = BeginMIPs(texture.width, texture.height, (u32*) data);
        IsValid(&mip);
        Advance(&mip))
    {
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, mip.level, 0, 0, textureHandle, mip.image.width, mip.image.height, 1, GL_BGRA_EXT, GL_UNSIGNED_BYTE, mip.image.pixels);
    }
#else
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, textureHandle, texture.width, texture.height, 1, GL_BGRA_EXT, GL_UNSIGNED_BYTE, data);
#endif
}

inline b32 IsValidArray(GLuint index)
{
    b32 result = (index != -1);
    return result;
}

#define OPENGL_BIND_U16(array, structure, field) if(IsValidArray(array))\
{\
    glEnableVertexAttribArray(array);\
    glVertexAttribIPointer(array, 1, GL_UNSIGNED_SHORT, sizeof(structure),(void*) OffsetOf(structure, field));\
}


#define OPENGL_BIND_U8(array, structure, field) if(IsValidArray(array))\
{\
    glEnableVertexAttribArray(array);\
    glVertexAttribIPointer(array, 1, GL_UNSIGNED_BYTE, sizeof(structure),(void*) OffsetOf(structure, field));\
}

internal void OpenGLUseProgramBegin(OpenGLProgramCommon* prog)
{
    glUseProgram(prog->progHandle);
    GLuint UVArray = prog->vertUVID;
    GLuint CArray = prog->vertColorID;
    GLuint PArray = prog->vertPID;
    
    if(IsValidArray(UVArray))
    {
        glEnableVertexAttribArray(UVArray);
        glVertexAttribPointer(UVArray, 2, GL_FLOAT, false, sizeof(TexturedVertex), (void*) OffsetOf(TexturedVertex, UV));
        
    }
    
    if(IsValidArray(CArray))
    {
        glEnableVertexAttribArray(CArray);
        glVertexAttribPointer(CArray, 4, GL_UNSIGNED_BYTE, true, sizeof(TexturedVertex), (void*) OffsetOf(TexturedVertex, color));
    }
    
    if(IsValidArray(PArray))
    {
        glEnableVertexAttribArray(PArray);
        glVertexAttribPointer(PArray, 4, GL_FLOAT, false, sizeof(TexturedVertex), (void*) OffsetOf(TexturedVertex, P));
    }
    
    OPENGL_BIND_U16(prog->lightStartingIndexID, TexturedVertex, lightStartingIndex);
    OPENGL_BIND_U16(prog->lightEndingIndexID, TexturedVertex, lightEndingIndex);
    OPENGL_BIND_U16(prog->textureIndexID, TexturedVertex, textureIndex);
    
    OPENGL_BIND_U8(prog->modulationID, TexturedVertex, modulationPercentage);
    OPENGL_BIND_U8(prog->lightInfluenceID, TexturedVertex, lightInfluence);
    OPENGL_BIND_U8(prog->lightYInfluenceID, TexturedVertex, lightYInfluence);
    OPENGL_BIND_U8(prog->windInfluenceID, TexturedVertex, windInfluence);
    OPENGL_BIND_U8(prog->windFrequencyID, TexturedVertex, windFrequency);
    OPENGL_BIND_U8(prog->dissolvePercentageID, TexturedVertex, dissolvePercentage);
    OPENGL_BIND_U8(prog->alphaThreesoldID, TexturedVertex, alphaThreesold);
    OPENGL_BIND_U8(prog->seedID, TexturedVertex, seed);
}

internal void OpenGLUseProgramBegin(ZBiasProgram* prog, RenderSetup* setup)
{
    OpenGLUseProgramBegin(&prog->common);
    m4x4 proj = setup->proj;
    glUniformMatrix4fv(prog->GLSLTransformID, 1, GL_TRUE, proj.E[0]);
    glUniform1i(prog->textureSamplerID, 0);
    glUniform1i(prog->lightSource0ID, 1);
    glUniform1i(prog->lightSource1ID, 2);
    glUniform1i(prog->noiseID, 3);
    
    glUniform3fv(prog->ambientLightColorID, 1, setup->ambientLightColor.E);
    glUniform1f(prog->timeID, setup->totalTimeElapsed);
    glUniform3fv(prog->windDirectionID, 1, setup->windDirection.E);
    glUniform1f(prog->windStrengthID, setup->windStrength);
}

internal void OpenGLUseProgramBegin(FinalStretchProgram* prog)
{
    OpenGLUseProgramBegin(&prog->common);
    glUniform1i(prog->textureSamplerID, 0);
}

#define DISABLE_VERTEX_ATTRIBUTE(attribute)if(IsValidArray(attribute)){glDisableVertexAttribArray(attribute);}

internal void OpenGLUseProgramEnd(OpenGLProgramCommon* prog)
{
    glUseProgram(0);
    
    DISABLE_VERTEX_ATTRIBUTE(prog->vertUVID);
    DISABLE_VERTEX_ATTRIBUTE(prog->vertColorID);
    DISABLE_VERTEX_ATTRIBUTE(prog->lightStartingIndexID);
    DISABLE_VERTEX_ATTRIBUTE(prog->lightEndingIndexID);
    DISABLE_VERTEX_ATTRIBUTE(prog->textureIndexID);
    DISABLE_VERTEX_ATTRIBUTE(prog->modulationID);
    DISABLE_VERTEX_ATTRIBUTE(prog->lightInfluenceID);
    DISABLE_VERTEX_ATTRIBUTE(prog->lightYInfluenceID);
    DISABLE_VERTEX_ATTRIBUTE(prog->windInfluenceID);
    DISABLE_VERTEX_ATTRIBUTE(prog->windFrequencyID);
    DISABLE_VERTEX_ATTRIBUTE(prog->dissolvePercentageID);
    DISABLE_VERTEX_ATTRIBUTE(prog->alphaThreesoldID);
    DISABLE_VERTEX_ATTRIBUTE(prog->seedID);
}

internal void OpenGLManageTextures(TextureOp* first)
{
    for(TextureOp* op = first; op; op = op->next)
    {
        OpenGLAllocateTexture(op->update.texture, op->update.data);
    }
}

internal GLuint OpenGLCreateProgram(char* defines, char* headerCode, char* vertexCode, char* fragmentCode, OpenGLProgramCommon* common)
{
    GLuint vertexShaderID = glCreateShader(GL_VERTEX_SHADER);
    GLchar* vertexShaderCode[] =
    {
        defines,
        headerCode,
        vertexCode
    };
    glShaderSource(vertexShaderID, ArrayCount(vertexShaderCode), vertexShaderCode, 0);
    
    
    GLuint fragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);
    GLchar* fragmentShaderCode[] =
    {
        defines,
        headerCode,
        fragmentCode
    };
    glShaderSource(fragmentShaderID, ArrayCount(fragmentShaderCode), fragmentShaderCode, 0);
    
    
    glCompileShader(vertexShaderID);
    glCompileShader(fragmentShaderID);
    
    
    GLuint programID = glCreateProgram();
    glAttachShader(programID, vertexShaderID);
    glAttachShader(programID, fragmentShaderID);
    glLinkProgram(programID);
    
    glValidateProgram(programID);
    GLint linked = false;
    glGetProgramiv(programID, GL_LINK_STATUS, &linked);
    if(!linked)
    {
        GLsizei ignored;
        char vertexErrors[4096];
        char fragmentErrors[4096];
        char programErrors[4096];
        glGetShaderInfoLog(vertexShaderID, sizeof(vertexErrors), &ignored, vertexErrors);
        glGetShaderInfoLog(fragmentShaderID, sizeof(fragmentErrors), &ignored, fragmentErrors);
        glGetProgramInfoLog(programID, sizeof(programErrors), &ignored, programErrors);
        
        Assert(!"Shader validation failed");
    }
    
    
    glDeleteShader(vertexShaderID);
    glDeleteShader(fragmentShaderID);
    
    common->progHandle = programID;
    common->vertUVID = glGetAttribLocation(programID, "vertUV");
    common->vertPID = glGetAttribLocation(programID, "vertP");
    common->vertColorID = glGetAttribLocation(programID, "vertColor");
    common->lightStartingIndexID = glGetAttribLocation(programID, "lightStartingIndex");
    common->lightEndingIndexID = glGetAttribLocation(programID, "lightEndingIndex");
    common->textureIndexID = glGetAttribLocation(programID, "textureIndex");
    common->modulationID = glGetAttribLocation(programID, "modulationPercentage");
    common->lightInfluenceID = glGetAttribLocation(programID, "lightInfluence");
    common->lightYInfluenceID = glGetAttribLocation(programID, "lightYInfluence");
    common->windInfluenceID = glGetAttribLocation(programID, "windInfluence");
    common->windFrequencyID = glGetAttribLocation(programID, "windFrequency");
    common->dissolvePercentageID = glGetAttribLocation(programID, "dissolvePercentage");
    common->alphaThreesoldID = glGetAttribLocation(programID, "alphaThreesold");
    common->seedID = glGetAttribLocation(programID, "seed");
    
    return programID;
}


char* globalHeaderCode = R"FOO(
//header code
#define m4x4 mat4x4
#define u32 uint
#define i32 int
#define r32 float
#define Vec4 vec4
#define Vec3 vec3
#define Vec2 vec2
#define V4 vec4
#define V3 vec3
#define V2 vec2
#define Lerp(a, t, b) mix(a, b, t)
#define Clamp01(t) clamp(t, 0, 1)
#define Clamp(min, t, max) clamp(t, min, max)
#define Dot(a, b) dot(a, b)

r32 Clamp01MapToRange(r32 Min, r32 t, r32 Max)
{
r32 Range = Max - Min;
r32 Result = Clamp01((t - Min) / Range);

return(Result);
}

)FOO";

internal void OpenGLCompileZBiasProgram(ZBiasProgram* result)
{
    
    char defines[4096];
    FormatString(defines, sizeof(defines),  
                 "#version 130\n"
                 "#define shaderSimTexLoadSRGB %d\n"
                 "#define shaderSimTexWriteSRGB %d\n",
                 opengl.shaderSimTexLoadSRGB,
                 opengl.shaderSimTexWriteSRGB);
    
    char* vertexCode = R"FOO(
        //vertex code
        in Vec4 vertP;
        in Vec2 vertUV;
        in Vec4 vertColor;
in int lightStartingIndex;
in int lightEndingIndex;
in int textureIndex;
in int modulationPercentage;
in int lightInfluence;
in int lightYInfluence;
in int windInfluence;
in int windFrequency;
in int dissolvePercentage;
in int alphaThreesold;
in int seed;

    uniform m4x4 transform;
    uniform r32 time;
    uniform Vec3 windDirection;
    uniform r32 windStrength;
         smooth out Vec2 fragUV;
         smooth out Vec2 fragUVNormalized;
         smooth out Vec4 fragColor;
         smooth out Vec3 worldPos;
          flat out int fragLightStartingIndex;
          flat out int fragLightEndingIndex;
          flat out int fragTextureIndex;
            flat out r32 modulationWithFocusColor;
            flat out r32 lightInfluencePercentage;
            flat out r32 lightYInfluencePercentage;
            flat out r32 fragDissolvePercentage;
            flat out r32 fragAlphaThreesold;
            
    void main(void)
          {
    Vec4 inVertex = V4(vertP.xyz, 1.0f);
    
    r32 windInfl = (float(windInfluence) / 0xff);
    r32 windSpeed = windStrength * float(windFrequency);
    
     Vec3 wind = sin((time + seed) * windSpeed) * windDirection.xyz * windInfl;
inVertex.xyz += wind;

           r32 zBias = vertP.w;
           Vec4 zVertex = inVertex;
          zVertex.z += zBias;
          
          Vec4 zMinTransform = transform * inVertex;
          Vec4 zMaxTransform = transform * zVertex;
          
                                           r32 modifiedZ = zMaxTransform.z * (zMinTransform.w / zMaxTransform.w);
                                          gl_Position = vec4(zMinTransform.x, zMinTransform.y, modifiedZ, zMinTransform.w);
                                          
                                          fragUV = vertUV;
                                          
                                          fragUVNormalized = V2(0, 0);
                                          if(vertUV.x > 0)
                                          {
                                          fragUVNormalized.x = 1.0f;
}

if(vertUV.y > 0)
{
fragUVNormalized.y = 1.0f;
}

                                          fragColor = vertColor;
                                          worldPos = inVertex.xyz;
                                            fragLightStartingIndex = lightStartingIndex;
                                            fragLightEndingIndex = lightEndingIndex;
                                           fragTextureIndex = textureIndex;
                                           modulationWithFocusColor = float(modulationPercentage) / 0xff;
                                            lightInfluencePercentage = float(lightInfluence) / 0xff;
                                            lightYInfluencePercentage = float(lightYInfluence) / 0xff;
                                            fragDissolvePercentage = float(dissolvePercentage) / 0xff;
                                            fragAlphaThreesold = float(alphaThreesold) / 0xff;
    }
    
   )FOO";
    
    char* fragmentCode;
    
#if 1    
    fragmentCode = R"FOO(
    //fragment code
    uniform sampler2DArray textureSampler;
    uniform sampler1D lightSource0;
    uniform sampler1D lightSource1;
    uniform sampler2D noise;
    
out Vec4 resultColor;
    smooth in Vec2 fragUV;
    smooth in Vec2 fragUVNormalized;
    smooth in Vec4 fragColor;
    smooth in Vec3 worldPos;
    
    flat in int fragLightStartingIndex;
    flat in int fragLightEndingIndex;
     flat in int fragTextureIndex;
     
      flat in r32 modulationWithFocusColor;
      flat in r32 lightInfluencePercentage;
            flat in r32 lightYInfluencePercentage;
            flat in r32 fragDissolvePercentage;
            flat in r32 fragAlphaThreesold;
            
    uniform Vec3 ambientLightColor;
    void main(void)
     {
     Vec3 arrayUV = V3(fragUV.x, fragUV.y, fragTextureIndex);
     Vec4 texSample = texture(textureSampler, arrayUV);
     
     
     #if shaderSimTexLoadSRGB
texSample.rgb *= texSample.rgb;
     #endif
     
     resultColor = fragColor * texSample;
     if(resultColor.a > fragAlphaThreesold)
     {
     Vec4 noiseSample = texture(noise, fragUVNormalized);
     if(noiseSample.r > fragDissolvePercentage)
     {
     Vec3 modulationLightColor = ambientLightColor;
     for(int index = fragLightStartingIndex; index < fragLightEndingIndex; ++index)
{
Vec4 lightData0 = texelFetch(lightSource0, index, 0);
 Vec4 lightData1 = texelFetch(lightSource1, index, 0);
 
 Vec3 lightP = lightData0.xyz;
 r32 lightStrength = lightData0.a;
 Vec3 lightColor = lightData1.rgb;
 
 r32 lightYDistance = lightP.y - worldPos.y;
r32 lightYModulation = 1.0f - (lightYInfluencePercentage * Clamp01MapToRange(0.0f, lightYDistance, 1.0f));
lightYModulation = max(lightYModulation, 0.4f);

Vec3 toLight = lightP - worldPos;
r32 lightDistance = length(toLight);
     toLight *= (1.0f / lightDistance);
     
#if 0
r32 cosAngle = dot(toLight, worldNorm);
     r32 lightInfluence = cosAngle *(lightStrength / (lightDistance * lightDistance));
     #else
     r32 lightInfluence = lightInfluencePercentage * lightYModulation * lightStrength / (lightDistance * lightDistance);
     #endif
     
     lightInfluence = clamp(lightInfluence, 0, 1);
     modulationLightColor += lightInfluence * lightColor;
}

modulationLightColor = clamp(modulationLightColor, 0, 1);
resultColor.rgb *= modulationLightColor;

resultColor.rgb = Clamp01(resultColor.rgb);
resultColor.rgb = Lerp(resultColor.rgb, modulationWithFocusColor, V3(0.4f, 0.4f, 0.4f));
#if shaderSimTexWriteSRGB
     resultColor.rgb = sqrt(resultColor.rgb);
     #endif
}
else
{
discard;
}
}
else
{
discard;
}

     }
   )FOO";
#else
    fragmentCode = R"FOO(
    //fragment code
    uniform sampler2DArray textureSampler;
out Vec4 resultColor;
    smooth in Vec2 fragUV;
    smooth in Vec4 fragColor;
    flat in int fragTextureIndex;
    
    void main(void)
     {
     Vec3 arrayUV = V3(fragUV.x, fragUV.y, fragTextureIndex);
     Vec4 texSample = texture(textureSampler, arrayUV);
     
     #if shaderSimTexLoadSRGB
texSample.rgb *= texSample.rgb;
     #endif
     
     resultColor = fragColor * texSample;
     if(resultColor.a > 0)
     {
#if shaderSimTexWriteSRGB
     resultColor.rgb = sqrt(resultColor.rgb);
     #endif
}
else
{
discard;
}
     }
   )FOO";
#endif
    
    GLuint prog = OpenGLCreateProgram(defines, globalHeaderCode, vertexCode, fragmentCode, &result->common);
    result->GLSLTransformID = glGetUniformLocation(prog, "transform");
    result->timeID = glGetUniformLocation(prog, "time");
    
    result->textureSamplerID = glGetUniformLocation(prog, "textureSampler");
    result->lightSource0ID = glGetUniformLocation(prog, "lightSource0");
    result->lightSource1ID = glGetUniformLocation(prog, "lightSource1");
    result->noiseID = glGetUniformLocation(prog, "noise");
    
    result->ambientLightColorID = glGetUniformLocation(prog, "ambientLightColor");
    result->windDirectionID = glGetUniformLocation(prog, "windDirection");
    result->windStrengthID = glGetUniformLocation(prog, "windStrength");
}

internal void OpenGLCompileFinalStretch(FinalStretchProgram* result)
{
    char defines[4096];
    FormatString(defines, sizeof(defines),  
                 "#version 130\n"
                 "#define shaderSimTexLoadSRGB %d\n"
                 "#define shaderSimTexWriteSRGB %d\n",
                 opengl.shaderSimTexLoadSRGB,
                 opengl.shaderSimTexWriteSRGB);
    
    char* vertexCode = R"FOO(
        in Vec4 vertP;
        in Vec2 vertUV;
         smooth out Vec2 fragUV;
         
void main(void)
          {
          gl_Position = vertP;
          fragUV = vertUV;
    }
    
   )FOO";
    
    char* fragmentCode = R"FOO(
        //fragment code
        uniform sampler2D textureSampler;
        smooth in Vec2 fragUV;
        out Vec4 resultColor;
        
        void main(void)
         {
          resultColor = texture(textureSampler, fragUV);
         }
       )FOO";
    
    GLuint prog = OpenGLCreateProgram(defines, globalHeaderCode, vertexCode, fragmentCode, &result->common);
    result->textureSamplerID = glGetUniformLocation(prog, "textureSampler");
}

internal void OpenGLInit(OpenGLInfo info, b32 frameBufferSupportSRGB)
{
    opengl.shaderSimTexLoadSRGB = true;
    opengl.shaderSimTexWriteSRGB = true;
    
    
#if 0    
    glGetIntegerv(GL_MAX_COLOR_TEXTURE_SAMPLES, &opengl.maxMultisampleCount);
    if(opengl.maxMultisampleCount > 16)
    {
        opengl.maxMultisampleCount = 16;
    }
    opengl.maxMultisampleCount = 1;
#endif
    
    
    opengl.defaultSpriteTextureFormat = GL_RGBA8;
    opengl.defaultFramebufferTextureFormat = GL_RGBA8;
    
    if(info.GLEXTTextureSRGB)
    {
        opengl.defaultSpriteTextureFormat = GL_SRGB8_ALPHA8;
        opengl.shaderSimTexLoadSRGB = false;
    }
    if(frameBufferSupportSRGB && info.GLARBFrameBufferSRGB) 
    {
        opengl.defaultFramebufferTextureFormat = GL_SRGB8_ALPHA8;
        glEnable(GL_FRAMEBUFFER_SRGB);
        opengl.shaderSimTexWriteSRGB = false;
    }
    
#if FORGIVENESS_INTERNAL
    if(glDebugMessageCallbackARB)
    {
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallbackARB(OpenGLDebugCallback, 0);
    }
#endif
    
    glGenBuffers(1, &opengl.vertexBuffer);
    glGenBuffers(1, &opengl.indexBuffer);
    
    {
        glGenBuffers(1, &opengl.screenFillVertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, opengl.screenFillVertexBuffer);
        TexturedVertex vertices[] = 
        {
            {{-1.0f, 1.0f, 0, 1.0f}, {0.0f, 1.0f}, 0xffffffff},
            {{-1.0f, -1.0f, 0, 1.0f}, {0.0f, 0.0f}, 0xffffffff},
            {{1.0f, 1.0f, 0, 1.0f}, {1.0f, 1.0f}, 0xffffffff},
            {{1.0f, -1.0f, 0, 1.0f}, {1.0f, 0.0f}, 0xffffffff},
        };
        
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    }
    
    
    Assert(opengl.maxTextureCount);
    glGenTextures(1, &opengl.textureArray);
    glBindTexture(GL_TEXTURE_2D_ARRAY, opengl.textureArray);
    
    for(MIPIterator mip = BeginMIPs(MAX_IMAGE_DIM, MAX_IMAGE_DIM, 0);
        IsValid(&mip);
        Advance(&mip))
    {
        glTexImage3D(GL_TEXTURE_2D_ARRAY, mip.level, opengl.defaultSpriteTextureFormat, mip.image.width, mip.image.height, opengl.maxTextureCount, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, 0);
    }
#if MIPMAPPING
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
#else
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
#endif
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);    
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);    
}

internal void FreeFrameBuffer(OpenGLFramebuffer* framebuffer)
{
    if(framebuffer->colorHandle)
    {
        glDeleteTextures(1, &framebuffer->colorHandle);
        framebuffer->colorHandle = 0;
    }
    
    if(framebuffer->depthHandle)
    {
        glDeleteTextures(1, &framebuffer->depthHandle);
        framebuffer->depthHandle = 0;
    }
    
    if(framebuffer->handle)
    {
        glDeleteFramebuffers(1, &framebuffer->handle);
        framebuffer->handle = 0;
    }
}

internal void FreeProgram(OpenGLProgramCommon* program)
{
    glDeleteProgram(program->progHandle);
}

internal void OpenGLPrepareForRenderSettings(GameRenderSettings* settings)
{
    FreeFrameBuffer(&opengl.frameBuffer);
    
    FreeProgram(&opengl.zBias.common);
    FreeProgram(&opengl.finalStretch.common);
    
    glDeleteTextures(1, &opengl.lightSource0);
    glDeleteTextures(1, &opengl.lightSource1);
    glDeleteTextures(1, &opengl.noise);
    
    opengl.settings = *settings;
    
    u32 renderWidth = settings->width;
    u32 renderHeight = settings->height;
    
    OpenGLCompileZBiasProgram(&opengl.zBias);
    OpenGLCompileFinalStretch(&opengl.finalStretch);
    
    opengl.textureGenFrameBuffer = OpenGLCreateFramebuffer(OpenGLFramebuffer_hasColor, MAX_IMAGE_DIM, MAX_IMAGE_DIM);
    
    u32 flags = (OpenGLFramebuffer_hasColor | OpenGLFramebuffer_hasDepth);
    opengl.frameBuffer = OpenGLCreateFramebuffer(flags, renderWidth, renderHeight);
    
    glGenTextures(1, &opengl.lightSource0);
    glBindTexture(GL_TEXTURE_1D, opengl.lightSource0);
    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA32F, MAX_LIGHTS, 0, GL_RGBA, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    glGenTextures(1, &opengl.lightSource1);
    glBindTexture(GL_TEXTURE_1D, opengl.lightSource1);
    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA32F, MAX_LIGHTS, 0, GL_RGBA, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    glBindTexture(GL_TEXTURE_1D, 0);
    
    
    glGenTextures(1, &opengl.noise);
    glBindTexture(GL_TEXTURE_2D, opengl.noise);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, NOISE_WIDTH, NOISE_HEIGHT, 0, GL_RGBA, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    glBindTexture(GL_TEXTURE_2D, 0);
}

enum DrawRenderBufferFlags
{
    DrawRenderBuffer_SortFrontToBack = (1 << 0),
    DrawRenderBuffer_SortBackToFront = (1 << 1),
    DrawRenderBuffer_Blend = (1 << 2),
};

internal void SortRenderBuffer(RenderBuffer* buffer, u32 flags)
{
    for(RenderSetup* setup = buffer->firstSetup; setup; setup = setup->next)
    {
        SortEntry* sortKeys = buffer->sortKeyArray + setup->quadStartingIndex;
        SortEntry* tempSortKeys = buffer->tempSortKeyArray + setup->quadStartingIndex;
        
        if(flags & DrawRenderBuffer_SortBackToFront)
        {
            RadixSort(sortKeys, setup->quadCount, tempSortKeys);
        }
        
        if(flags & DrawRenderBuffer_SortFrontToBack)
        {
            RadixSort(sortKeys, setup->quadCount, tempSortKeys, true);
        }
    }
}

internal void ComputeIndeces(RenderBuffer* buffer)
{
    for(RenderSetup* setup = buffer->firstSetup; setup; setup = setup->next)
    {
        for(u32 quadIndex = 0; quadIndex < setup->quadCount; ++quadIndex)
        {
            SortEntry* sortKey = buffer->sortKeyArray + (setup->quadStartingIndex + quadIndex);
            u32* indeces = buffer->indexArray + ((setup->quadStartingIndex + quadIndex) * 6);
            
            u32 VI = sortKey->index * 4;
            indeces[0] = VI + 0;
            indeces[1] = VI + 1;
            indeces[2] = VI + 3;
            indeces[3] = VI + 1;
            indeces[4] = VI + 2;
            indeces[5] = VI + 3;
        }
    }
}

internal void DrawRenderBuffer(RenderBuffer* buffer, u32 flags, u32 renderWidth, u32 renderHeight)
{
    if(flags & DrawRenderBuffer_Blend)
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    }
    else
    {
        glDisable(GL_BLEND);
    }
    
    u32 totalVertexCount = buffer->quadCount * 4;
    glBindBuffer(GL_ARRAY_BUFFER, opengl.vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, totalVertexCount * sizeof(TexturedVertex), buffer->vertexArray, GL_STREAM_DRAW);
    
    u32 totalIndexCount = buffer->quadCount * 6;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, opengl.indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, totalIndexCount * sizeof(u32), buffer->indexArray, GL_STREAM_DRAW);
    
    for(RenderSetup* setup = buffer->firstSetup; setup; setup = setup->next)
    {
        Rect2i clipRect = setup->rect;    
        glScissor(clipRect.minX, clipRect.minY, clipRect.maxX - clipRect.minX, clipRect.maxY - clipRect.minY);
        
        if(setup->renderTargetIndex > 0)
        {
            OpenGLBindFramebuffer(&opengl.textureGenFrameBuffer, MAX_IMAGE_DIM, MAX_IMAGE_DIM);
            glFramebufferTextureLayer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, opengl.textureArray, 0, setup->renderTargetIndex);
        }
        
        ZBiasProgram* program = &opengl.zBias;
        OpenGLUseProgramBegin(program, setup);
        Assert(setup->quadCount > 0);
        u32 batchCount = (setup->quadCount / maxQuadsPerBatch) + 1;
        u32 indexOffset = setup->indexArrayOffset;
        u32 quadsRemaining = setup->quadCount;
        for(u32 batchIndex = 0; batchIndex < batchCount; ++batchIndex)
        {
            u32 quadCount = Min(quadsRemaining, maxQuadsPerBatch);
            glDrawElements(GL_TRIANGLES, 6 * quadCount, GL_UNSIGNED_INT, (GLvoid*) (indexOffset * sizeof(u32)));
            
            indexOffset += quadCount * 6;
            quadsRemaining -= quadCount;
        }
        
        OpenGLUseProgramEnd(&program->common);

		if(setup->renderTargetIndex > 0)
		{
			OpenGLBindFramebuffer(&opengl.frameBuffer, renderWidth, renderHeight);
		}
    }
}

inline void OpenGLRenderCommands(GameRenderCommands* commands, Rect2i drawRegion, i32 windowWidth, i32 windowHeight)
{
    glDepthMask(GL_TRUE);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glEnable(GL_SCISSOR_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_DEPTH_TEST);
    
    if(!AreEqual(&opengl.settings, &commands->settings))
    {
        OpenGLPrepareForRenderSettings(&commands->settings);
    }
    
    u32 renderWidth = commands->settings.width;
    u32 renderHeight = commands->settings.height;
    
    OpenGLBindFramebuffer(&opengl.frameBuffer, renderWidth, renderHeight);
    glScissor(0, 0, renderWidth, renderHeight);
    glClearDepth(1.0f);
    glClearColor(commands->clearColor.r, commands->clearColor.g, commands->clearColor.b, commands->clearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glBindTexture(GL_TEXTURE_1D, opengl.lightSource0);
    glTexSubImage1D(GL_TEXTURE_1D, 0, 0, MAX_LIGHTS, GL_RGBA, GL_FLOAT, commands->lightSource0);
    glBindTexture(GL_TEXTURE_1D, opengl.lightSource1);
    glTexSubImage1D(GL_TEXTURE_1D, 0, 0, MAX_LIGHTS, GL_RGBA, GL_FLOAT, commands->lightSource1);
    glBindTexture(GL_TEXTURE_1D, 0);
    
    if(!opengl.noiseTextureUploaded)
    {
        glBindTexture(GL_TEXTURE_2D, opengl.noise);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, NOISE_WIDTH, NOISE_HEIGHT, GL_RGBA, GL_FLOAT, commands->noiseTexture);
        glBindTexture(GL_TEXTURE_2D, 0);
        opengl.noiseTextureUploaded = true;
    }
    
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_1D, opengl.lightSource0);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_1D, opengl.lightSource1);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, opengl.noise);
    glActiveTexture(GL_TEXTURE0);
    
    
    glBindTexture(GL_TEXTURE_2D_ARRAY, opengl.textureArray);

    SortRenderBuffer(&commands->opaque, DrawRenderBuffer_SortFrontToBack);
	ComputeIndeces(&commands->opaque);
	DrawRenderBuffer(&commands->opaque, 0, renderWidth, renderHeight);

    SortRenderBuffer(&commands->transparent, DrawRenderBuffer_SortBackToFront);
    ComputeIndeces(&commands->transparent);
    DrawRenderBuffer(&commands->transparent, DrawRenderBuffer_Blend, renderWidth, renderHeight);

    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
    
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glViewport(0, 0, windowWidth, windowHeight);
    glScissor(0, 0, windowWidth, windowHeight);
    
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    glViewport(drawRegion.minX, drawRegion.minY, GetWidth(drawRegion), GetHeight(drawRegion));
    glScissor(drawRegion.minX, drawRegion.minY, GetWidth(drawRegion), GetHeight(drawRegion));
    
    glBindBuffer(GL_ARRAY_BUFFER, opengl.screenFillVertexBuffer);
    OpenGLUseProgramBegin(&opengl.finalStretch);
    glBindTexture(GL_TEXTURE_2D, opengl.frameBuffer.colorHandle);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    OpenGLUseProgramEnd(&opengl.finalStretch.common);
}
