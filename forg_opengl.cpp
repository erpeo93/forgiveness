#include "forg_render.h"
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


global_variable u32 globalFrameBufferCount = 0;

inline void OpenGLBindFramebuffer(OpenGLFramebuffer* frameBuffer, u32 renderWidth, u32 renderHeight)
{
    glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer ? frameBuffer->handle : 0);
    glViewport(0, 0, renderWidth, renderHeight);
}

inline OpenGLFramebuffer OpenGLCreateFramebuffer(u32 flags, u32 width, u32 height)
{
    OpenGLFramebuffer result = {};
    
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
            glTexImage2DMultisample(slot, opengl.maxMultisampleCount,
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
            glTexImage2DMultisample(slot, opengl.maxMultisampleCount,
                                    GL_DEPTH_COMPONENT32F,
                                    width, height,
                                    GL_FALSE);
            
        }
        else
        {
            glTexImage2D(slot, 0,
                         GL_DEPTH_COMPONENT24,
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

inline void GLQuad(Vec3 P0, Vec2 T0, Vec4 C0,
                   Vec3 P1, Vec2 T1, Vec4 C1,
                   Vec3 P2, Vec2 T2, Vec4 C2,
                   Vec3 P3, Vec2 T3, Vec4 C3)
{
    glColor4fv(C0.E);
    glTexCoord2fv(T0.E);
    glVertex3fv(P0.E);
    
    glColor4fv(C1.E);
    glTexCoord2fv(T1.E);
    glVertex3fv(P1.E);
    
    glColor4fv(C2.E);
    glTexCoord2fv(T2.E);
    glVertex3fv(P2.E);
    
    glColor4fv(C0.E);
    glTexCoord2fv(T0.E);
    glVertex3fv(P0.E);
    
    glColor4fv(C2.E);
    glTexCoord2fv(T2.E);
    glVertex3fv(P2.E);
    
    glColor4fv(C3.E);
    glTexCoord2fv(T3.E);
    glVertex3fv(P3.E);
}

inline void GLLine(Vec3 min, Vec3 max, Vec4 c1, Vec4 c2)
{
    glColor4fv(c1.E);
    glVertex3fv(min.E);
    
    glColor4fv(c2.E);
    glVertex3fv(max.E);
}

inline void GLQuadOutline(Vec3 P0, Vec4 C0, Vec3 P1, Vec4 C1, Vec3 P2, Vec4 C2, Vec3 P3, Vec4 C3)
{
    GLLine(P0, P1, C0, C1);
    GLLine(P1, P2, C1, C2);
    GLLine(P2, P3, C2, C3);
    GLLine(P3, P0, C3, C0);
}

inline void OpenGLBitmap(Vec3 P, r32 zBias, Vec3 XAxis, Vec3 YAxis, Vec4 color, Bitmap* buffer, Vec2 minUV = V2(0, 0), Vec2 maxUV = V2(1, 1))
{
    Vec4 minXminY = V4(P, 0.0f);
    Vec4 maxXminY = V4(P + XAxis, zBias);
    Vec4 minXmaxY = V4(P + YAxis, zBias);
    Vec4 maxXmaxY = V4(P + XAxis + YAxis, zBias);
    
    glBegin(GL_TRIANGLES);
    
    glColor4f(color.r, color.g, color.b, color.a);
    
    glTexCoord2f(minUV.x, minUV.y);
    glVertex3fv(minXminY.E);
    
    glTexCoord2f(maxUV.x, minUV.y);
    glVertex3fv(maxXminY.E);
    
    glTexCoord2f(maxUV.x, maxUV.y);
    glVertex3fv(maxXmaxY.E);
    
    glTexCoord2f(minUV.x, minUV.y);
    glVertex3fv(minXminY.E);
    
    glTexCoord2f(maxUV.x, maxUV.y);
    glVertex3fv(maxXmaxY.E);
    
    glTexCoord2f(minUV.x, maxUV.y);
    glVertex3fv(minXmaxY.E);
    
    glEnd();
}

inline void OpenGLDisplayBitmap(u32 bufferWidth, u32 bufferHeight, void* memory, u32 windowWidth, u32 windowHeight, GLuint textureHandle)
{
    glViewport(0, 0, windowWidth, windowHeight);
    
    glBindTexture(GL_TEXTURE_2D, textureHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, bufferWidth, bufferHeight, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, memory);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_SCISSOR_TEST);
    glClearColor(1.0f, 0.0f, 1.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    InvalidCodePath;
    //OpenGLRectangle(V3(0, 0, 0), V2i(windowWidth, windowHeight), V4(1.0f, 1.0f, 1.0f, 1.0f));
    
    glBindTexture(GL_TEXTURE_2D, 0);
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
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, textureHandle, texture.width, texture.height, 1, GL_BGRA_EXT, GL_UNSIGNED_BYTE, data);
}

inline b32 IsValidArray(GLuint index)
{
    b32 result = (index != -1);
    return result;
}

internal void OpenGLUseProgramBegin(OpenGLProgramCommon* prog)
{
    glUseProgram(prog->progHandle);
    GLuint UVArray = prog->vertUVID;
    GLuint CArray = prog->vertColorID;
    GLuint PArray = prog->vertPID;
    GLuint NArray = prog->vertNID;
    GLuint lightStartingArray = prog->lightStartingIndexID;
    GLuint lightEndingArray = prog->lightEndingIndexID;
    GLuint textureArray = prog->textureIndexID;
    GLuint modulationArray = prog->modulationID;
    
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
    
    if(IsValidArray(NArray))
    {
        glEnableVertexAttribArray(NArray);
        glVertexAttribPointer(NArray, 3, GL_FLOAT, false, sizeof(TexturedVertex), (void*) OffsetOf(TexturedVertex, N));
    }
    
    
    if(IsValidArray(lightStartingArray))
    {
        glEnableVertexAttribArray(lightStartingArray);
        glVertexAttribIPointer(lightStartingArray, 1, GL_UNSIGNED_SHORT, sizeof(TexturedVertex), (void*) OffsetOf(TexturedVertex, lightStartingIndex));
    }
    
    if(IsValidArray(lightEndingArray))
    {
        glEnableVertexAttribArray(lightEndingArray);
        glVertexAttribIPointer(lightEndingArray, 1, GL_UNSIGNED_SHORT, sizeof(TexturedVertex), (void*) OffsetOf(TexturedVertex, lightEndingIndex));
    }
    
    
    if(IsValidArray(textureArray))
    {
        glEnableVertexAttribArray(textureArray);
        glVertexAttribIPointer(textureArray, 1, GL_UNSIGNED_SHORT, sizeof(TexturedVertex), (void*) OffsetOf(TexturedVertex, textureIndex));
    }
    
    if(IsValidArray(modulationArray))
    {
        glEnableVertexAttribArray(modulationArray);
        glVertexAttribPointer(modulationArray, 1, GL_FLOAT, false, sizeof(TexturedVertex), (void*) OffsetOf(TexturedVertex, modulationPercentage));
    }
}

internal void OpenGLUseProgramBegin(ZBiasProgram* prog, RenderSetup* setup, r32 alphaThreesold)
{
    OpenGLUseProgramBegin(&prog->common);
    m4x4 proj = setup->proj;
    glUniformMatrix4fv(prog->GLSLTransformID, 1, GL_TRUE, proj.E[0]);
    glUniform1i(prog->textureSamplerID, 0);
    glUniform1i(prog->depthSamplerID, 1);
    glUniform1i(prog->lightSource0ID, 2);
    glUniform1i(prog->lightSource1ID, 3);
    
    glUniform1f(prog->alphaThreesoldID, alphaThreesold);
    glUniform3fv(prog->ambientLightColorID, 1, setup->ambientLightColor.E);
}

internal void OpenGLUseProgramBegin(PeelCompositeProgram* prog)
{
    OpenGLUseProgramBegin(&prog->common);
    
    for(u32 peelIndex = 0; peelIndex < ArrayCount(prog->peelSamplerID); ++peelIndex)
    {
        glUniform1i(prog->peelSamplerID[peelIndex], peelIndex);
    }
}

internal void OpenGLUseProgramBegin(FinalStretchProgram* prog)
{
    OpenGLUseProgramBegin(&prog->common);
    glUniform1i(prog->textureSamplerID, 0);
}

internal void OpenGLUseProgramEnd(OpenGLProgramCommon* prog)
{
    glUseProgram(0);
    
    if(IsValidArray(prog->vertUVID))
    {
        glDisableVertexAttribArray(prog->vertUVID);
    }
    
    if(IsValidArray(prog->vertColorID))
    {
        glDisableVertexAttribArray(prog->vertColorID);
    }
    
    if(IsValidArray(prog->vertPID))
    {
        glDisableVertexAttribArray(prog->vertPID);
    }
    
    if(IsValidArray(prog->lightStartingIndexID))
    {
        glDisableVertexAttribArray(prog->lightStartingIndexID);
    }
    
    if(IsValidArray(prog->lightEndingIndexID))
    {
        glDisableVertexAttribArray(prog->lightEndingIndexID);
    }
    
    if(IsValidArray(prog->textureIndexID))
    {
        glDisableVertexAttribArray(prog->textureIndexID);
    }
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
    common->vertNID = glGetAttribLocation(programID, "vertN");
    common->vertColorID = glGetAttribLocation(programID, "vertColor");
    common->lightStartingIndexID = glGetAttribLocation(programID, "lightStartingIndex");
    common->lightEndingIndexID = glGetAttribLocation(programID, "lightEndingIndex");
    common->textureIndexID = glGetAttribLocation(programID, "textureIndex");
    common->modulationID = glGetAttribLocation(programID, "modulationPercentage");
    
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

internal void OpenGLCompileZBiasProgram(ZBiasProgram* result, b32 depthPeel, b32 light)
{
    
    char defines[4096];
    FormatString(defines, sizeof(defines),  
                 "#version 130\n"
                 "#define shaderSimTexLoadSRGB %d\n"
                 "#define shaderSimTexWriteSRGB %d\n"
                 "#define depthPeeling %d\n",
                 opengl.shaderSimTexLoadSRGB,
                 opengl.shaderSimTexWriteSRGB,
                 depthPeel);
    
    char* vertexCode = R"FOO(
        //vertex code
        in Vec4 vertP;
        in Vec3 vertN;
        in Vec2 vertUV;
        in Vec4 vertColor;
in int lightStartingIndex;
in int lightEndingIndex;
in int textureIndex;
in r32 modulationPercentage;

    uniform m4x4 transform;
         smooth out Vec2 fragUV;
         smooth out Vec4 fragColor;
         smooth out Vec3 worldPos;
         smooth out Vec3 worldNorm;
          flat out int fragLightStartingIndex;
          flat out int fragLightEndingIndex;
          flat out int fragTextureIndex;
           smooth out r32 modulationWithFocusColor;
           
    void main(void)
          {
    Vec4 inVertex = V4(vertP.xyz, 1.0f);
           r32 zBias = vertP.w;
           
           Vec4 zVertex = inVertex;
          zVertex.z += zBias;
          
          Vec4 zMinTransform = transform * inVertex;
          Vec4 zMaxTransform = transform * zVertex;
          
                                           r32 modifiedZ = zMaxTransform.z * (zMinTransform.w / zMaxTransform.w);
                                          gl_Position = vec4(zMinTransform.x, zMinTransform.y, modifiedZ, zMinTransform.w);
                                          
                                          fragUV = vertUV;
                                          fragColor = vertColor;
                                          worldPos = inVertex.xyz;
                                          worldNorm = vertN;
                                            fragLightStartingIndex = lightStartingIndex;
                                            fragLightEndingIndex = lightEndingIndex;
                                           fragTextureIndex = textureIndex;
                                           modulationWithFocusColor = modulationPercentage;
    }
    
   )FOO";
    
    char* fragmentCode;
    if(light)
    {
        fragmentCode = R"FOO(
        //fragment code
        uniform sampler2DArray textureSampler;
        #if depthPeeling
        uniform sampler2D depthSampler;
        #endif
        uniform r32 alphaThreesold;
    out Vec4 resultColor;
        smooth in Vec2 fragUV;
        smooth in Vec4 fragColor;
        smooth in Vec3 worldPos;
        smooth in Vec3 worldNorm;
        
        flat in int fragLightStartingIndex;
        flat in int fragLightEndingIndex;
         flat in int fragTextureIndex;
         
         smooth in r32 modulationWithFocusColor;
        uniform Vec3 ambientLightColor;
        
        uniform sampler1D lightSource0;
        uniform sampler1D lightSource1;
        
        void main(void)
         {
         Vec3 directionalLightColor = V3(1.0f, 1.0f, 0);
         Vec3 lightDir = normalize(V3(0, 1, -1));
         
         
    #if depthPeeling
         r32 clipDepth = texelFetch(depthSampler, ivec2(gl_FragCoord.xy), 0).r;
         r32 fragZ = gl_FragCoord.z;
         if(fragZ <= clipDepth)
         {
         discard;
    }
         #endif
         
         Vec3 arrayUV = V3(fragUV.x, fragUV.y, fragTextureIndex);
         Vec4 texSample = texture(textureSampler, arrayUV);
         #if shaderSimTexLoadSRGB
    texSample.rgb *= texSample.rgb;
         #endif
         
    resultColor = fragColor * texSample;
         if(resultColor.a > alphaThreesold)
         {
         
         
         // NOTE(Leonardo): point light test!
if(fragLightStartingIndex != fragLightEndingIndex)
{
   Vec3 modulationLightColor = ambientLightColor;
         for(int index = fragLightStartingIndex; index < fragLightEndingIndex; ++index)
{
 Vec4 lightData0 = texelFetch(lightSource0, index, 0);
     Vec4 lightData1 = texelFetch(lightSource1, index, 0);
     
     Vec3 lightP = lightData0.xyz;
     Vec3 lightColor = lightData1.rgb;
     r32 lightStrength = lightData0.a;
     
     
Vec3 toLight = lightP - worldPos;
         r32 lightDistance = length(toLight);
         
         r32 lightInfluence = lightStrength / (lightDistance * lightDistance);
         lightInfluence = clamp(lightInfluence, 0, 1);
         modulationLightColor += lightInfluence * lightColor;
}

modulationLightColor = clamp(modulationLightColor, 0, 1);
resultColor.rgb *= modulationLightColor;
}
else
{
resultColor.rgb *= ambientLightColor;
}




         // NOTE(Leonardo): directional light!
         #if 0   
         {
         Vec3 toLight = -lightDir;
         r32 cosAngle = Dot(toLight, worldNorm);
         cosAngle = clamp(cosAngle, 0, 1);
         
         r32 lightInfluence = cosAngle;
         Vec3 modulationLight = Lerp(V3(1, 1, 1), lightInfluence, directionalLightColor);
         resultColor.rgb *= modulationLight;
}
#endif




resultColor.rgb = Clamp01(resultColor.rgb);
resultColor.rgb = Lerp(resultColor.rgb, modulationWithFocusColor, V3(0.7f, 0.7f, 0.7f));
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
    }
    else
    {
        fragmentCode = R"FOO(
        //fragment code
        uniform sampler2DArray textureSampler;
        #if depthPeeling
        uniform sampler2D depthSampler;
        #endif
        uniform r32 alphaThreesold;
    out Vec4 resultColor;
        smooth in Vec2 fragUV;
        smooth in Vec4 fragColor;
        smooth in Vec3 worldPos;
        smooth in Vec3 worldNorm;
         flat in Vec4 fragLightIndex;
         flat in int fragTextureIndex;
         smooth in r32 modulationWithFocusColor;
        uniform Vec3 ambientLightColor;
        
uniform Vec3 pointLightPos[256];
uniform Vec3 pointLightColors[256];
uniform r32 pointLightStrength[256];
        void main(void)
         {
         
    #if depthPeeling
         r32 clipDepth = texelFetch(depthSampler, ivec2(gl_FragCoord.xy), 0).r;
         r32 fragZ = gl_FragCoord.z;
         if(fragZ <= clipDepth)
         {
         discard;
    }
         #endif
         
         Vec3 arrayUV = V3(fragUV.x, fragUV.y, fragTextureIndex);
         Vec4 texSample = texture(textureSampler, arrayUV);
         #if shaderSimTexLoadSRGB
    texSample.rgb *= texSample.rgb;
         #endif
         
    resultColor = fragColor * texSample;
         if(resultColor.a > alphaThreesold)
         {
         
         resultColor.rgb *= ambientLightColor;
         
resultColor.rgb = Clamp01(resultColor.rgb);
resultColor.rgb = Lerp(resultColor.rgb, modulationWithFocusColor, V3(0.7f, 0.7f, 0.7f));
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
    }
    
    GLuint prog = OpenGLCreateProgram(defines, globalHeaderCode, vertexCode, fragmentCode, &result->common);
    result->GLSLTransformID = glGetUniformLocation(prog, "transform");
    result->textureSamplerID = glGetUniformLocation(prog, "textureSampler");
    result->depthSamplerID = glGetUniformLocation(prog, "depthSampler");
    result->alphaThreesoldID = glGetUniformLocation(prog, "alphaThreesold");
    result->ambientLightColorID = glGetUniformLocation(prog, "ambientLightColor");
    result->lightSource0ID = glGetUniformLocation(prog, "lightSource0");
    result->lightSource1ID = glGetUniformLocation(prog, "lightSource1");
    
}


internal void OpenGLCompilePeelComposite(PeelCompositeProgram* result)
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
        in Vec4 vertColor;
         smooth out Vec2 fragUV;
         smooth out Vec4 fragColor;
         
    void main(void)
          {
          gl_Position = vertP;
          fragUV = vertUV;
          fragColor = vertColor;
    }
    
   )FOO";
    
    char* fragmentCode = R"FOO(
        //fragment code
        uniform sampler2D peelSampler[4];
        smooth in Vec2 fragUV;
        smooth in Vec4 fragColor;
        out Vec4 resultColor;
        
        void main(void)
         {
         Vec4 peel0 = texture(peelSampler[0], fragUV);
         Vec4 peel1 = texture(peelSampler[1], fragUV);
         Vec4 peel2 = texture(peelSampler[2], fragUV);
         Vec4 peel3 = texture(peelSampler[3], fragUV);
         
         #if shaderSimTexLoadSRGB
    peel0.rgb *= peel0.rgb;
    peel1.rgb *= peel1.rgb;
         #endif
         
#if 0         
         r32 invA = 0.0f;
         if(peel1.a > 0)
         {
         invA = 1.0f / peel1.a;
    }
    peel1.rgb *= invA;
    #endif
    
    resultColor.rgb = peel3.rgb;
    resultColor.rgb = peel2.rgb * peel2.a + ((1 - peel2.a) * resultColor.rgb); 
    resultColor.rgb = peel1.rgb * peel1.a + ((1 - peel1.a) * resultColor.rgb); 
    resultColor.rgb = fragColor.rgb * (peel0.rgb * peel0.a + ((1 - peel0.a) * resultColor.rgb)); 
    
#if 0
    resultColor.rgb = peel1.rgb;
    resultColor.rgb = peel0.rgb * peel0.a + ((1 - peel0.a) * resultColor.rgb); 
    #endif
    
#if shaderSimTexWriteSRGB
         resultColor.rgb = sqrt(resultColor.rgb);
         #endif
         
         }
       )FOO";
    
    GLuint prog = OpenGLCreateProgram(defines, globalHeaderCode, vertexCode, fragmentCode, &result->common);
    result->peelSamplerID[0] = glGetUniformLocation(prog, "peelSampler[0]");
    result->peelSamplerID[1] = glGetUniformLocation(prog, "peelSampler[1]");
    result->peelSamplerID[2] = glGetUniformLocation(prog, "peelSampler[2]");
    result->peelSamplerID[3] = glGetUniformLocation(prog, "peelSampler[3]");
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
    
    glGetIntegerv(GL_MAX_COLOR_TEXTURE_SAMPLES, &opengl.maxMultisampleCount);
    if(opengl.maxMultisampleCount > 16)
    {
        opengl.maxMultisampleCount = 16;
    }
    
    opengl.defaultSpriteTextureFormat = GL_RGBA8;
    opengl.defaultFramebufferTextureFormat = GL_RGBA8;
    
    if(info.GLEXTTextureSRGB)
    {
        opengl.defaultSpriteTextureFormat = GL_SRGB8_ALPHA8;
        opengl.shaderSimTexLoadSRGB = false;
    }
    if(frameBufferSupportSRGB && info.GLARBFrameBufferSRGB) 
    {
        
#if 0        
        GLuint testTexture;
        glGenTextures(1, &testTexture);
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, testTexture);
        glGetError();
        glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, opengl.maxMultisampleCount,
                                GL_SRGB8_ALPHA8,
                                1920, 1080,
                                GL_FALSE);
        
        if(glGetError() == GL_NO_ERROR)
#endif
        
        {
            opengl.defaultFramebufferTextureFormat = GL_SRGB8_ALPHA8;
            glEnable(GL_FRAMEBUFFER_SRGB);
            opengl.shaderSimTexWriteSRGB = false;
        }
        
#if 0        
        glDeleteTextures(1, &testTexture);
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
#endif
        
    }
    
#if FORGIVENESS_INTERNAL
    if(glDebugMessageCallbackARB)
    {
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallbackARB(OpenGLDebugCallback, 0);
    }
    
    GLuint dummyVertexArray;
    glGenVertexArrays(1, &dummyVertexArray);
    glBindVertexArray(dummyVertexArray);
#endif
    
    glGenBuffers(1, &opengl.vertexBuffer);
    glGenBuffers(1, &opengl.indexBuffer);
    
    
    {
        glGenBuffers(1, &opengl.screenFillVertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, opengl.screenFillVertexBuffer);
        TexturedVertex vertices[] = 
        {
            {{-1.0f, 1.0f, 0, 1.0f} , {}, {0.0f, 1.0f}, 0xffffffff},
            {{-1.0f, -1.0f, 0, 1.0f} ,  {}, {0.0f, 0.0f}, 0xffffffff},
            {{1.0f, 1.0f, 0, 1.0f} , {}, {1.0f, 1.0f}, 0xffffffff},
            {{1.0f, -1.0f, 0, 1.0f} , {}, {1.0f, 0.0f}, 0xffffffff},
        };
        
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    }
    
    
    Assert(opengl.maxTextureCount);
    glGenTextures(1, &opengl.textureArray);
    glBindTexture(GL_TEXTURE_2D_ARRAY, opengl.textureArray);
    
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, opengl.defaultSpriteTextureFormat, TEXTURE_ARRAY_DIM, TEXTURE_ARRAY_DIM, opengl.maxTextureCount, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, 0);
    
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
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
    FreeFrameBuffer(&opengl.resolveFramebuffer);
    for(u32 depthPeelIndex = 0; depthPeelIndex < opengl.depthPeelCount; ++depthPeelIndex)
    {
        FreeFrameBuffer(&opengl.depthPeelBuffer[depthPeelIndex]);
    }
    FreeProgram(&opengl.zBiasDepthPeelLight.common);
    FreeProgram(&opengl.zBiasDepthPeelNoLight.common);
    FreeProgram(&opengl.zBiasNoDepthPeel.common);
    FreeProgram(&opengl.peelComposite.common);
    FreeProgram(&opengl.finalStretch.common);
    
    
    glDeleteTextures(1, &opengl.lightSource0);
    glDeleteTextures(1, &opengl.lightSource1);
    
    
    opengl.settings = *settings;
    
    u32 renderWidth = settings->width;
    u32 renderHeight = settings->height;
    
    OpenGLCompileZBiasProgram(&opengl.zBiasDepthPeelLight, true, true);
    OpenGLCompileZBiasProgram(&opengl.zBiasDepthPeelNoLight, true, false);
    OpenGLCompileZBiasProgram(&opengl.zBiasNoDepthPeel, false, true);
    OpenGLCompilePeelComposite(&opengl.peelComposite);
    OpenGLCompileFinalStretch(&opengl.finalStretch);
    
    u32 resolveFlags = (OpenGLFramebuffer_linearFilter | OpenGLFramebuffer_hasColor);
    opengl.resolveFramebuffer = OpenGLCreateFramebuffer(resolveFlags, renderWidth, renderHeight);
    
    opengl.depthPeelCount = settings->depthPeelCount;
    if(opengl.depthPeelCount > ArrayCount(opengl.depthPeelBuffer))
    {
        opengl.depthPeelCount = ArrayCount(opengl.depthPeelBuffer);
    }
    
    u32 flags = (OpenGLFramebuffer_hasColor | OpenGLFramebuffer_hasDepth);
    for(u32 depthPeelIndex = 0; depthPeelIndex < opengl.depthPeelCount; ++depthPeelIndex)
    {
        opengl.depthPeelBuffer[depthPeelIndex] = OpenGLCreateFramebuffer(flags, renderWidth, renderHeight);
    }
    
    
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
}

inline void OpenGLRenderCommands(GameRenderCommands* commands, Rect2i drawRegion, i32 windowWidth, i32 windowHeight)
{
    TIMED_FUNCTION();
    
    glDepthMask(GL_TRUE);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    
    //glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
    //glEnable(GL_SAMPLE_ALPHA_TO_ONE);
    //glEnable(GL_MULTISAMPLE);
    
    glEnable(GL_SCISSOR_TEST);
    glDisable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    
    b32 useRenderTargets = (glBindFramebuffer != 0);
    Assert(useRenderTargets);
    
    if(!AreEqual(&opengl.settings, &commands->settings))
    {
        OpenGLPrepareForRenderSettings(&commands->settings);
    }
    
    u32 renderWidth = commands->settings.width;
    u32 renderHeight = commands->settings.height;
    
    u32 maxRenderTargetIndex = opengl.depthPeelCount - 1;
    
    for(u32 targetIndex = 0; targetIndex <= maxRenderTargetIndex; ++targetIndex)
    {
        OpenGLBindFramebuffer(opengl.depthPeelBuffer + targetIndex, renderWidth, renderHeight);
        glScissor(0, 0, renderWidth, renderHeight);
        glClearDepth(1.0f);
        if(targetIndex == maxRenderTargetIndex)
        {
            glClearColor(commands->clearColor.r, commands->clearColor.g, commands->clearColor.b, commands->clearColor.a);
        }
        else
        {
            glClearColor(0, 0, 0, 0);
        }
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
    
    OpenGLBindFramebuffer(opengl.depthPeelBuffer + 0, renderWidth, renderHeight);
    b32 peeling = false;
    u32 peelIndex = 0;
    u32 peelWalkedSize = 0;
    GLuint currentRenderTargetIndex = 0xFFFFFFFF;
    
    glBindBuffer(GL_ARRAY_BUFFER, opengl.vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, commands->vertexCount * sizeof(TexturedVertex), commands->vertexArray, GL_STREAM_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, opengl.indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, commands->indexCount * sizeof(u16), commands->indexArray, GL_STREAM_DRAW);
    
    glBindTexture(GL_TEXTURE_1D, opengl.lightSource0);
    glTexSubImage1D(GL_TEXTURE_1D, 0, 0, MAX_LIGHTS, GL_RGBA, GL_FLOAT, commands->lightSource0);
    glBindTexture(GL_TEXTURE_1D, opengl.lightSource1);
    glTexSubImage1D(GL_TEXTURE_1D, 0, 0, MAX_LIGHTS, GL_RGBA, GL_FLOAT, commands->lightSource1);
    glBindTexture(GL_TEXTURE_1D, 0);
    
    
    for(u32 walkedSize = 0; walkedSize < commands->usedSize; walkedSize += sizeof(CommandHeader))
    {
        CommandHeader* header = (CommandHeader*) (commands->pushMemory + walkedSize);
        
        if(useRenderTargets)
        {
            void* data = (header + 1); 
            switch(header->type)
            {
                case CommandType_BeginPeels:
                {
                    peelWalkedSize = walkedSize;
                } break;
                
                case CommandType_EndPeels:
                {
                    if(peelIndex < maxRenderTargetIndex)
                    {
                        walkedSize = peelWalkedSize;
                        ++peelIndex;
                        OpenGLBindFramebuffer(opengl.depthPeelBuffer + peelIndex, renderWidth, renderHeight);
                        peeling = true;
                    }
                    else
                    {
                        Assert(peelIndex == maxRenderTargetIndex);
                        OpenGLBindFramebuffer(opengl.depthPeelBuffer + 0, renderWidth, renderHeight);
                        peeling = false;
                    }
                } break;
                
                case CommandType_TexturedQuadsCommand:
                {
                    glBindBuffer(GL_ARRAY_BUFFER, opengl.vertexBuffer);
                    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, opengl.indexBuffer);
                    
                    TexturedQuadsCommand* element = (TexturedQuadsCommand*) data;
                    Rect2i clipRect = element->setup.rect;
                    
                    glScissor(clipRect.minX, clipRect.minY, clipRect.maxX - clipRect.minX, clipRect.maxY - clipRect.minY);
                    ZBiasProgram* program = &opengl.zBiasNoDepthPeel;
                    r32 alphaThreesold = 0.0f;
                    if(peeling)
                    {
                        if(true || peelIndex <= 1)
                        {
                            program = &opengl.zBiasDepthPeelLight;
                        }
                        else
                        {
                            program = &opengl.zBiasDepthPeelNoLight;
                        }
                        glActiveTexture(GL_TEXTURE1);
                        glBindTexture(GL_TEXTURE_2D, opengl.depthPeelBuffer[peelIndex - 1].depthHandle);
                        glActiveTexture(GL_TEXTURE2);
                        glBindTexture(GL_TEXTURE_1D, opengl.lightSource0);
                        glActiveTexture(GL_TEXTURE3);
                        glBindTexture(GL_TEXTURE_1D, opengl.lightSource1);
                        glActiveTexture(GL_TEXTURE0);
                        
                        if(peelIndex == maxRenderTargetIndex)
                        {
                            alphaThreesold = 0.9f;
                        }
                    }
                    
                    OpenGLUseProgramBegin(program, &element->setup, alphaThreesold);
                    
                    
                    glBindTexture(GL_TEXTURE_2D_ARRAY, opengl.textureArray);
                    glDrawElementsBaseVertex(GL_TRIANGLES, 3 * element->triangleCount, GL_UNSIGNED_SHORT, (GLvoid*) (element->indexArrayOffset * sizeof(u16)), element->vertexArrayOffset);
                    
                    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
                    OpenGLUseProgramEnd(&program->common);
                    
                    if(peeling)
                    {
                        glActiveTexture(GL_TEXTURE1);
                        glBindTexture(GL_TEXTURE_2D, 0);
                        glActiveTexture(GL_TEXTURE0);
                    }
                    walkedSize += sizeof(TexturedQuadsCommand);
                } break;
                
                InvalidDefaultCase;
            }
        }
    }
    
#if 0
    glBindFramebuffer(GL_READ_FRAMEBUFFER, opengl.depthPeelBuffer[3].handle);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glViewport(drawRegion.minX, drawRegion.minY, windowWidth, windowHeight);
    glBlitFramebuffer(0, 0, renderWidth, renderHeight,
                      drawRegion.minX, drawRegion.minY,
                      drawRegion.maxX, drawRegion.maxY,
                      GL_COLOR_BUFFER_BIT,
                      GL_LINEAR);
#else
    
    
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, opengl.resolveFramebuffer.handle);
    glViewport(0, 0, renderWidth, renderHeight);
    glScissor(0, 0, renderWidth, renderHeight);
    
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    
    glDisable(GL_DEPTH_TEST);
    glBindBuffer(GL_ARRAY_BUFFER, opengl.screenFillVertexBuffer);
    OpenGLUseProgramBegin(&opengl.peelComposite);
    
    
    for(u32 peel = 0; peel <= maxRenderTargetIndex; ++peel)
    {
        glActiveTexture(GL_TEXTURE0 + peel);
        glBindTexture(GL_TEXTURE_2D, opengl.depthPeelBuffer[peel].colorHandle);
    }
    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    
    
    for(u32 peel = 0; peel <= maxRenderTargetIndex; ++peel)
    {
        glActiveTexture(GL_TEXTURE0 + peel);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    
    glActiveTexture(GL_TEXTURE0);
    OpenGLUseProgramEnd(&opengl.peelComposite.common);
    
    
    
    
    
    
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glViewport(0, 0, windowWidth, windowHeight);
    glScissor(0, 0, windowWidth, windowHeight);
    
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    
    glViewport(drawRegion.minX, drawRegion.minY, GetWidth(drawRegion), GetHeight(drawRegion));
    glScissor(drawRegion.minX, drawRegion.minY, GetWidth(drawRegion), GetHeight(drawRegion));
    
    glDisable(GL_DEPTH_TEST);
    glBindBuffer(GL_ARRAY_BUFFER, opengl.screenFillVertexBuffer);
    OpenGLUseProgramBegin(&opengl.finalStretch);
    glBindTexture(GL_TEXTURE_2D, opengl.resolveFramebuffer.colorHandle);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    OpenGLUseProgramEnd(&opengl.finalStretch.common);
    
#endif
}
