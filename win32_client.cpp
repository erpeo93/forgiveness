#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <process.h>
#include <Tlhelp32.h>
#include <winbase.h>
#include <DbgHelp.h>
#include <stdio.h>
#include <time.h>

#include "forg_base.h"
#define LL_NET_IMPLEMENTATION
#include "ll_net.h"

#include "forg_platform.h"
global_variable PlatformAPI platformAPI;
#include "forg_pool.h"
#include "forg_debug_interface.h"
#include "forg_token.h"
#include "forg_shared.h"
#include "forg_intrinsics.h"
#include "forg_math.h"
#include "forg_pool.cpp"

#include "win32_forg.h"
#include "win32_file.cpp"
#include "win32_thread.cpp"

#include <xinput.h>
#include <dsound.h>
#include <gl/gl.h>

#pragma comment( lib, "wsock32.lib" )
#pragma comment (lib, "Ws2_32.lib")

#include "forg_noise.h"

struct Win32ScreenBuffer
{
    u16 width;
    u16 height;
    void* memory;
    i32 bytesPerPixel;
    i32 pitch;
    BITMAPINFO info;
};

struct Win32SoundBuffer
{
    LPDIRECTSOUNDBUFFER buffer;
    i32 samplesPerSecond;
    i32 totalBufferSize;
    i32 runningSampleIndex;
    i32 bytesPerSample;
    i32 delaySamples;
};

struct Win32Dimension
{
    i32 width;
    i32 height;
};

struct Win32GameCode
{
    b32 isValid;
    HMODULE gameDLL;
    FILETIME lastWriteTime;
    game_update_and_render* UpdateAndRender;
    game_get_sound_output* GetSoundOutput;
    game_frame_end* DEBUGFrameEnd;
};

global_variable b32 running;
global_variable b32 freezing;
global_variable r32 targetSecPerFrame;
global_variable Win32GameCode game;
global_variable Win32ScreenBuffer globalScreenBuffer;
global_variable PlatformInput gameInput;
global_variable PlatformClientMemory gameMemory;
global_variable Win32SoundBuffer winSoundBuffer;
global_variable i16* soundSamples;
global_variable DWORD lastPlayCursor;
global_variable DWORD writeCursor;
global_variable i64 globalFrequency;
global_variable HWND globalWindow;
#define IsInLoop() false



typedef BOOL WINAPI wgl_swap_interval_ext( int interval );
typedef HGLRC WINAPI wgl_create_context_attribs_arb( HDC hDC, HGLRC hShareContext, const int *attribList );
typedef BOOL WINAPI wgl_choose_pixel_format_arb( HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats );
typedef char* WINAPI wgl_get_extensions_string_ext();

typedef char GLchar;
typedef ptrdiff_t GLsizeiptr;
typedef ptrdiff_t GLintptr;

typedef void gl_attach_shader( GLuint program, GLuint shader );
typedef void gl_bind_attrib_location( GLuint program, GLuint index, const GLchar *name );
typedef void gl_compile_shader( GLuint shader );
typedef GLuint gl_create_program( void );
typedef GLuint gl_create_shader( GLenum type );
typedef void gl_link_program ( GLuint program );
typedef void gl_shader_source ( GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length );
typedef void gl_use_program ( GLuint program );
typedef void gl_validate_program( GLuint program );
typedef void gl_get_program_iv( GLuint program, GLenum pname, GLint *params );
typedef void gl_get_program_info_log( GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog );
typedef void gl_get_shader_info_log( GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog );

typedef GLint gl_get_uniform_location( GLuint program, const GLchar *name );
typedef void gl_uniform_matrix_4fv( GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
typedef void gl_uniform_4f( GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3 );
typedef void gl_uniform_1i( GLint location, GLint v0 );
typedef void type_glUniform1f (GLint location, GLfloat v0);
typedef void type_glUniform3fv (GLint location, GLsizei count, const GLfloat *value);
typedef GLuint type_glGetUniformBlockIndex (GLuint program, const GLchar *uniformBlockName);
typedef void type_glUniformBlockBinding (GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding);
typedef void type_glBindBufferBase (GLenum target, GLuint index, GLuint buffer);

typedef void type_glDisableVertexAttribArray(GLuint index);
typedef void type_glEnableVertexAttribArray(GLuint index);
typedef void type_glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer);
typedef void type_glVertexAttribIPointer (GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer);
typedef GLint type_glGetAttribLocation(GLuint program, const GLchar *name);
typedef void type_glBindVertexArray (GLuint array);
typedef void type_glGenVertexArrays (GLsizei n, GLuint *arrays);

typedef void type_glTexImage2DMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations);

typedef void type_glTexStorage3D (GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth);

typedef void type_glTexImage3D (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void *pixels);

typedef void type_glTexSubImage3D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels);

typedef void type_glBindFramebuffer (GLenum target, GLuint framebuffer);
typedef void type_glGenFramebuffers (GLsizei n, GLuint *framebuffers);
typedef void type_glFramebufferTexture2D (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
typedef void type_glBlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
typedef GLenum type_glCheckFramebufferStatus(GLenum target);

typedef void type_glDeleteProgram (GLuint program);
typedef void type_glDeleteShader (GLuint shader);
typedef void type_glDeleteFramebuffers (GLsizei n, const GLuint *framebuffers);
typedef void type_glFramebufferTextureLayer (GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer);
typedef void type_glDrawElementsBaseVertex (GLenum mode, GLsizei count, GLenum type, const void *indices, GLint basevertex);

#define GL_DEBUG_CALLBACK( name ) void WINAPI name( GLenum source,GLenum type,GLuint id,GLenum severity,GLsizei length,const GLchar *message,const void *userParam )
typedef GL_DEBUG_CALLBACK( GLDEBUGPROC );
typedef void type_glDebugMessageCallbackARB( GLDEBUGPROC callback, const void *userParam );

typedef void APIENTRY type_glBufferData (GLenum target, GLsizeiptr size, const void *data, GLenum usage);
typedef void APIENTRY type_glGenBuffers (GLsizei n, GLuint *buffers);
typedef void APIENTRY type_glBindBuffer (GLenum target, GLuint buffer);
typedef void APIENTRY type_glActiveTexture (GLenum texture);

typedef const GLubyte *APIENTRY type_glGetStringi (GLenum name, GLuint index);

#define OpenGLGlobalFunction( name ) global_variable type_##name* name;

global_variable wgl_swap_interval_ext* wglSwapInterval;
global_variable wgl_create_context_attribs_arb* WGLCreateContextAttribsARB;
global_variable wgl_choose_pixel_format_arb* WGLChoosePixelFormatARB;
global_variable wgl_get_extensions_string_ext* WGLGetExtensionsStringEXT;
global_variable b32 globalSoftwareRendering = false;

global_variable gl_attach_shader* glAttachShader;
global_variable gl_bind_attrib_location* glBindAttribLocation;
global_variable gl_compile_shader* glCompileShader;
global_variable gl_create_program* glCreateProgram;
global_variable gl_create_shader* glCreateShader;
global_variable gl_link_program* glLinkProgram;
global_variable gl_shader_source* glShaderSource;
global_variable gl_use_program* glUseProgram;
global_variable gl_validate_program* glValidateProgram;
global_variable gl_get_program_iv* glGetProgramiv;
global_variable gl_get_program_info_log* glGetProgramInfoLog;
global_variable gl_get_shader_info_log* glGetShaderInfoLog;
global_variable gl_get_uniform_location* glGetUniformLocation;
global_variable gl_uniform_matrix_4fv* glUniformMatrix4fv;
global_variable gl_uniform_4f* glUniform4f;
global_variable gl_uniform_1i* glUniform1i;
OpenGLGlobalFunction(glUniform1f);
OpenGLGlobalFunction(glUniform3fv);
OpenGLGlobalFunction(glGetUniformBlockIndex);
OpenGLGlobalFunction(glUniformBlockBinding);
OpenGLGlobalFunction(glBindBufferBase);

OpenGLGlobalFunction( glDisableVertexAttribArray );
OpenGLGlobalFunction( glEnableVertexAttribArray );
OpenGLGlobalFunction( glVertexAttribPointer );
OpenGLGlobalFunction( glVertexAttribIPointer );
OpenGLGlobalFunction( glGetAttribLocation );
OpenGLGlobalFunction( glDebugMessageCallbackARB );
OpenGLGlobalFunction( glBufferData );
OpenGLGlobalFunction( glGenBuffers );
OpenGLGlobalFunction( glBindBuffer );

OpenGLGlobalFunction( glBindVertexArray );
OpenGLGlobalFunction( glGenVertexArrays );
OpenGLGlobalFunction( glBindFramebuffer );
OpenGLGlobalFunction( glActiveTexture );
OpenGLGlobalFunction( glGenFramebuffers );
OpenGLGlobalFunction( glFramebufferTexture2D );
OpenGLGlobalFunction( glTexImage2DMultisample );
OpenGLGlobalFunction( glTexSubImage3D);
OpenGLGlobalFunction( glTexImage3D);
OpenGLGlobalFunction( glBlitFramebuffer );
OpenGLGlobalFunction( glCheckFramebufferStatus );
OpenGLGlobalFunction( glGetStringi );
OpenGLGlobalFunction( glDeleteProgram );
OpenGLGlobalFunction( glDeleteShader );
OpenGLGlobalFunction(glDeleteFramebuffers);
OpenGLGlobalFunction(glFramebufferTextureLayer);

OpenGLGlobalFunction(glDrawElementsBaseVertex);

#define WGL_DRAW_TO_WINDOW_ARB 0x2001
#define WGL_ACCELERATION_ARB 0x2003
#define WGL_SUPPORT_OPENGL_ARB 0x2010
#define WGL_DOUBLE_BUFFER_ARB 0x2011
#define WGL_PIXEL_TYPE_ARB 0x2013

#define WGL_FULL_ACCELERATION_ARB 0x2027
#define WGL_TYPE_RGBA_ARB 0x202B

#define WGL_RED_BITS_ARB 0x2015
#define WGL_GREEN_BITS_ARB 0x2017
#define WGL_BLUE_BITS_ARB 0x2019
#define WGL_ALPHA_BITS_ARB 0x201B
#define WGL_DEPTH_BITS_ARB 0x2022

#include "forg_file_formats.h"
#include "forg_asset.h"
#include "forg_render_tier.h"
#include "forg_opengl.h"
global_variable Opengl opengl;

#include "forg_image.h"
#include "forg_image.cpp"
#include "forg_opengl.cpp"
#include "forg_render_tier.cpp"


WINDOWPLACEMENT windowPlacement = {sizeof(WINDOWPLACEMENT)};




global_variable Win32MemoryBlock globalMemorySentinel;
global_variable TicketMutex memoryMutex;
#include "win32_memory_callback.cpp"
#include "win32_process.cpp"









internal Win32GameCode Win32LoadGameCode( char* DLLName, char* tempDLLName, char* lockName )
{
    Win32GameCode result = {};
    
    char* toLoad = DLLName;
#if FORGIVENESS_INTERNAL
    CopyFile(DLLName, tempDLLName, 0);
    toLoad = tempDLLName;
#endif
    
    WIN32_FILE_ATTRIBUTE_DATA fileAttributes;
    WIN32_FILE_ATTRIBUTE_DATA ignored;
    
    if( !GetFileAttributesEx( lockName, GetFileExInfoStandard, &ignored ) )
    {
        if( GetFileAttributesEx(DLLName, GetFileExInfoStandard, &fileAttributes))
        {
            result.lastWriteTime = fileAttributes.ftLastWriteTime;
        }
        result.gameDLL = LoadLibrary(toLoad);
        if( result.gameDLL )
        {
            result.UpdateAndRender = ( game_update_and_render* ) GetProcAddress( result.gameDLL, "GameUpdateAndRender" );
            result.GetSoundOutput = ( game_get_sound_output* ) GetProcAddress( result.gameDLL, "GameGetSoundOutput" );
            result.DEBUGFrameEnd = ( game_frame_end* ) GetProcAddress( result.gameDLL, "GameDEBUGFrameEnd" );
            result.isValid = ( result.UpdateAndRender && result.GetSoundOutput && result.DEBUGFrameEnd );
        }
    }
    
    
    if( !result.isValid )
    {
        result.UpdateAndRender = false;
        result.GetSoundOutput = false;
        result.DEBUGFrameEnd = false;
    }
    return result;
}

internal void Win32UnloadGameCode( Win32GameCode* gameCode )
{
    FreeLibrary( gameCode->gameDLL );
    gameCode->gameDLL = 0;
    gameCode->UpdateAndRender = 0;
    gameCode->GetSoundOutput = 0;
    gameCode->isValid = false;
}

#define XINPUT_GET_STATE( name ) DWORD WINAPI name( DWORD dwUserIndex, XINPUT_STATE* pState ) 
typedef XINPUT_GET_STATE( x_input_get_state );
global_variable x_input_get_state* XInputGetState_;
#define XInputGetState XInputGetState_

XINPUT_GET_STATE( x_input_get_state_stub )
{
    return ERROR_DEVICE_NOT_CONNECTED;
}

#define XINPUT_SET_STATE( name ) DWORD WINAPI name( DWORD dwUserIndex, XINPUT_VIBRATION* pVibration )
typedef XINPUT_SET_STATE( x_input_set_state );
global_variable x_input_set_state* XInputSetState_;
#define XInputSetState XInputSetState_

XINPUT_SET_STATE( x_input_set_state_stub )
{
    return ERROR_DEVICE_NOT_CONNECTED;
}


int win32OpenGLAttribs[] = 
{
    WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
    WGL_CONTEXT_MINOR_VERSION_ARB, 3,
    WGL_CONTEXT_FLAGS_ARB, 0//WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB
#if FORGIVENESS_INTERNAL
        | WGL_CONTEXT_DEBUG_BIT_ARB
#endif
        ,
    WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
    //WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
    0
};

internal void Win32SetPixelFormat( HDC windowDC )
{
    int suggestedPixelFormatIndex = 0;
    GLuint extendedPick = 0;
    
    if( WGLChoosePixelFormatARB )
    {
        int intAttribList[] =
        {
            WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
            WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
            WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
#if FORGIVENESS_STREAMING
            WGL_DOUBLE_BUFFER_ARB, GL_FALSE,
#else
            WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
#endif
            WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
            WGL_RED_BITS_ARB, 8,
            WGL_GREEN_BITS_ARB, 8,
            WGL_BLUE_BITS_ARB, 8,
            WGL_ALPHA_BITS_ARB, 8,
            WGL_DEPTH_BITS_ARB, 24,
            WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB, GL_TRUE,
            0,
        };
        
        if( !opengl.supportSRGBFrameBuffer )
        {
            intAttribList[20] = 0;
        }
        
        WGLChoosePixelFormatARB( windowDC, intAttribList, 0, 1, &suggestedPixelFormatIndex, &extendedPick );
    }
    
    if( extendedPick == 0 )
    {
        PIXELFORMATDESCRIPTOR desiredPixelFormat = {};
        desiredPixelFormat.nSize = sizeof( desiredPixelFormat );
        desiredPixelFormat.nVersion = 1;
        
#if FORGIVENESS_STREAMING
        desiredPixelFormat.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW;
#else
        desiredPixelFormat.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
#endif
        desiredPixelFormat.iPixelType = PFD_TYPE_RGBA;
        desiredPixelFormat.cColorBits = 24;
        desiredPixelFormat.cAlphaBits = 8;
        desiredPixelFormat.cDepthBits = 24;
        desiredPixelFormat.iLayerType = PFD_MAIN_PLANE;
        
        suggestedPixelFormatIndex = ChoosePixelFormat( windowDC, &desiredPixelFormat );
    }
    PIXELFORMATDESCRIPTOR suggestedPixelFormat;
    DescribePixelFormat( windowDC, suggestedPixelFormatIndex, sizeof( suggestedPixelFormat ), &suggestedPixelFormat );
    SetPixelFormat( windowDC, suggestedPixelFormatIndex, &suggestedPixelFormat );
}

internal void Win32LoadWGLExtension()
{
    WNDCLASS windowClass  = {};
    windowClass.lpfnWndProc = DefWindowProcA;
    windowClass.hInstance = GetModuleHandle( 0 );
    windowClass.lpszClassName = "forgivenessWGLLoader";
    
    if( RegisterClass( &windowClass) )
    {
        HWND window = CreateWindowEx( 0, windowClass.lpszClassName, "forgiveness", 0, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, windowClass.hInstance, 0 );
        HDC windowDC = GetDC( window );
        Win32SetPixelFormat( windowDC );
        HGLRC openGLRC = wglCreateContext( windowDC );
        if( wglMakeCurrent( windowDC, openGLRC ) )
        {
#define GetOpenGLFunction( name ) name = ( type_##name* ) wglGetProcAddress( #name )
            WGLChoosePixelFormatARB = ( wgl_choose_pixel_format_arb* ) wglGetProcAddress( "wglChoosePixelFormatARB" );
            WGLCreateContextAttribsARB = ( wgl_create_context_attribs_arb* ) wglGetProcAddress( "wglCreateContextAttribsARB" );
            wglSwapInterval = ( wgl_swap_interval_ext* ) wglGetProcAddress( "wglSwapIntervalEXT" );
            WGLGetExtensionsStringEXT = ( wgl_get_extensions_string_ext* ) wglGetProcAddress( "wglGetExtensionsStringEXT" );
            
            glAttachShader = ( gl_attach_shader* ) wglGetProcAddress( "glAttachShader" );
            glBindAttribLocation = ( gl_bind_attrib_location* ) wglGetProcAddress( "glBindAttribLocation" );
            glCompileShader = ( gl_compile_shader* ) wglGetProcAddress( "glCompileShader" );
            glCreateProgram = ( gl_create_program* ) wglGetProcAddress( "glCreateProgram" );
            glCreateShader = ( gl_create_shader* ) wglGetProcAddress( "glCreateShader" );
            glLinkProgram = ( gl_link_program* ) wglGetProcAddress( "glLinkProgram" );
            glShaderSource = ( gl_shader_source* ) wglGetProcAddress( "glShaderSource" );
            glUseProgram = ( gl_use_program* ) wglGetProcAddress( "glUseProgram" );
            
            glValidateProgram = ( gl_validate_program* ) wglGetProcAddress( "glValidateProgram" );
            glGetProgramiv = ( gl_get_program_iv* ) wglGetProcAddress( "glGetProgramiv" );
            glGetProgramInfoLog = ( gl_get_program_info_log* ) wglGetProcAddress( "glGetProgramInfoLog" );
            glGetShaderInfoLog = ( gl_get_shader_info_log* ) wglGetProcAddress( "glGetShaderInfoLog" );
            
            glGetUniformLocation = ( gl_get_uniform_location* ) wglGetProcAddress( "glGetUniformLocation" );
            glUniformMatrix4fv = ( gl_uniform_matrix_4fv* ) wglGetProcAddress( "glUniformMatrix4fv" );
            glUniform4f = ( gl_uniform_4f* ) wglGetProcAddress( "glUniform4f" );
            glUniform1i = ( gl_uniform_1i* ) wglGetProcAddress( "glUniform1i" );
            GetOpenGLFunction(glUniform1f);
            GetOpenGLFunction(glUniform3fv);
            GetOpenGLFunction(glGetUniformBlockIndex);
            GetOpenGLFunction(glUniformBlockBinding);
            GetOpenGLFunction(glBindBufferBase);
            
            GetOpenGLFunction( glDisableVertexAttribArray );
            GetOpenGLFunction( glEnableVertexAttribArray );
            GetOpenGLFunction( glVertexAttribPointer );
            GetOpenGLFunction( glVertexAttribIPointer );
            GetOpenGLFunction( glGetAttribLocation );
            GetOpenGLFunction( glGenVertexArrays );
            GetOpenGLFunction( glBindVertexArray );
            
            GetOpenGLFunction( glGenFramebuffers );
            GetOpenGLFunction( glBindFramebuffer );
            GetOpenGLFunction( glActiveTexture );
            
            GetOpenGLFunction( glDebugMessageCallbackARB );
            GetOpenGLFunction( glTexImage2DMultisample );
            GetOpenGLFunction( glTexSubImage3D);
            GetOpenGLFunction( glTexImage3D);
            GetOpenGLFunction( glBlitFramebuffer );
            GetOpenGLFunction( glCheckFramebufferStatus );
            GetOpenGLFunction( glFramebufferTexture2D );
            
            GetOpenGLFunction( glBufferData );
            GetOpenGLFunction( glGenBuffers );
            GetOpenGLFunction( glBindBuffer );
            GetOpenGLFunction( glGetStringi );
            GetOpenGLFunction( glDeleteProgram );
            GetOpenGLFunction( glDeleteShader );
            GetOpenGLFunction(glDeleteFramebuffers);
            GetOpenGLFunction(glFramebufferTextureLayer);
            
            GetOpenGLFunction(glDrawElementsBaseVertex);
            
            
            if( WGLGetExtensionsStringEXT )
            {
                char* extensions = WGLGetExtensionsStringEXT();
                char* at = extensions; 
                while( *at )
                {
                    while( IsWhiteSpace( *at ) ) { ++at; }
                    char* end = at;
                    while( *end && !IsWhiteSpace( *end ) ){ ++end; }
                    
                    unm count = ( end - at );
                    if( StrEqual( count, at, "WGL_EXT_framebuffer_sRGB" ) ){ opengl.supportSRGBFrameBuffer = true; }
                    else if( StrEqual( count, at, "WGL_ARB_framebuffer_sRGB" ) ){ opengl.supportSRGBFrameBuffer = true; }
                    at = end;
                }
                
            }
            
            wglMakeCurrent( 0, 0 );
        }
        
        wglDeleteContext( openGLRC );
        ReleaseDC( window, windowDC );
        DestroyWindow( window );
    }
}

internal HGLRC Win32InitOpenGL(HDC windowDC, u16 maxTextureCount)
{
    Win32LoadWGLExtension();
    Win32SetPixelFormat( windowDC );
    
    opengl.maxTextureCount = maxTextureCount;
    
    HGLRC openGLRC = 0;
    b32 modernContext = true;
    
    if( WGLCreateContextAttribsARB )
    {
        openGLRC = WGLCreateContextAttribsARB(windowDC, 0, win32OpenGLAttribs);
    }
    
    if( !openGLRC )
    {
        modernContext = false;
        openGLRC = wglCreateContext( windowDC );
    }
    
    if( wglMakeCurrent( windowDC, openGLRC ) )
    {
        OpenGLInfo info = OpenGLGetInfo( modernContext );
        OpenGLInit( info, opengl.supportSRGBFrameBuffer );
        if(wglSwapInterval)
        {
            wglSwapInterval(1);
        }
    }
    
    glGenTextures(1, &opengl.blitTextureHandle);
    return openGLRC;
}

internal void Win32InitDSound( HWND window, i32 samplesPerSecond, i32 totalBufferSize )
{
    typedef HRESULT WINAPI direct_sound_create( LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter);
    direct_sound_create* DirectSoundCreate;
    HMODULE DSoundLib = LoadLibrary( "dsound.dll" );
    if( DSoundLib )
    {
        DirectSoundCreate = ( direct_sound_create* ) GetProcAddress( DSoundLib, "DirectSoundCreate" );
        if( DirectSoundCreate )
        {
            LPDIRECTSOUND directSound;
            if( SUCCEEDED( DirectSoundCreate( 0, &directSound, 0 ) ) )
            {
                if( SUCCEEDED( directSound->SetCooperativeLevel( window, DSSCL_PRIORITY ) ) )
                {
                    DSBUFFERDESC primaryBufferDesc = {};
                    primaryBufferDesc.dwSize = sizeof( primaryBufferDesc );
                    primaryBufferDesc.dwFlags = DSBCAPS_PRIMARYBUFFER;
                    LPDIRECTSOUNDBUFFER primaryBuffer;
                    
                    WAVEFORMATEX waveFormat = {};
                    waveFormat.wFormatTag = WAVE_FORMAT_PCM;
                    waveFormat.nChannels = 2;
                    waveFormat.wBitsPerSample = 16;
                    waveFormat.nSamplesPerSec = samplesPerSecond;
                    waveFormat.nBlockAlign = waveFormat.nChannels * waveFormat.wBitsPerSample / 8;
                    waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
                    
                    if( SUCCEEDED( directSound->CreateSoundBuffer( &primaryBufferDesc, &primaryBuffer, 0 ) ) )
                    {
                        
                        if( SUCCEEDED( primaryBuffer->SetFormat( &waveFormat ) ) )
                        {
                            //primary buffer created successfully!
                            OutputDebugString( "primary buffer created!\n" );
                        }
                    }
                    
                    DSBUFFERDESC secondaryBufferDesc = {};
                    secondaryBufferDesc.dwSize = sizeof( secondaryBufferDesc );
                    secondaryBufferDesc.dwFlags = DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_GLOBALFOCUS;
                    secondaryBufferDesc.dwBufferBytes = totalBufferSize;
                    secondaryBufferDesc.lpwfxFormat = &waveFormat;
                    
                    if( SUCCEEDED( directSound->CreateSoundBuffer( &secondaryBufferDesc,
                                                                  &winSoundBuffer.buffer, 0 ) ) )
                    {
                        //secondary buffer created successfully!
                        OutputDebugString( "secondary buffer created!\n" );
                    }					
                }
            }
        }
    }
}

internal void Win32ResizeWindow( Win32ScreenBuffer* screenBuffer, u16 newWidth, u16 newHeight )
{
    if( screenBuffer->memory )
    {
        VirtualFree( screenBuffer->memory, 0, MEM_RELEASE );
    }
    
    screenBuffer->info.bmiHeader.biSize = sizeof( screenBuffer->info.bmiHeader );
    screenBuffer->info.bmiHeader.biWidth = newWidth;
    screenBuffer->info.bmiHeader.biHeight = newHeight;
    screenBuffer->info.bmiHeader.biPlanes = 1;
    screenBuffer->info.bmiHeader.biBitCount = 32;
    screenBuffer->info.bmiHeader.biCompression = BI_RGB;
    screenBuffer->width = newWidth;
    screenBuffer->height = newHeight;
    screenBuffer->bytesPerPixel = 4;
    
    screenBuffer->pitch = Align16( screenBuffer->bytesPerPixel * screenBuffer->width );
    
    i32 totalBufferSize = screenBuffer->pitch * newHeight;
    screenBuffer->memory = VirtualAlloc( 0, totalBufferSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE );
}

internal void Win32ToggleFullScreen( HWND window )
{
    DWORD dwStyle = GetWindowLong(window, GWL_STYLE);
    if ( dwStyle & WS_OVERLAPPEDWINDOW )
    {
        MONITORINFO mi = { sizeof(mi) };
        if ( GetWindowPlacement( window, &windowPlacement ) &&
            GetMonitorInfo( MonitorFromWindow( window,
                                              MONITOR_DEFAULTTOPRIMARY ),&mi ) )
        {
            SetWindowLong( window, GWL_STYLE,
                          dwStyle & ~WS_OVERLAPPEDWINDOW );
            SetWindowPos( window, HWND_TOP,
                         mi.rcMonitor.left, mi.rcMonitor.top,
                         mi.rcMonitor.right - mi.rcMonitor.left,
                         mi.rcMonitor.bottom - mi.rcMonitor.top,
                         SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }
        
    } 
    else
    {
        SetWindowLong(window, GWL_STYLE,
                      dwStyle | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement( window, &windowPlacement );
        SetWindowPos( window, NULL, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                     SWP_NOOWNERZORDER | SWP_FRAMECHANGED );
    }
}

internal void Win32LoadXInput()
{	
    HMODULE XInputLib = LoadLibrary( "xinput1_4.dll" );
    if( !XInputLib )
    {
        XInputLib = LoadLibrary( "xinput9_1_0.dll" );
    }
    XInputGetState = ( x_input_get_state* ) GetProcAddress( XInputLib, "XInputGetState" );
    if( !XInputGetState )
    {
        XInputGetState = x_input_get_state_stub;
    }
    
    XInputSetState_ = ( x_input_set_state* ) GetProcAddress( XInputLib, "XInputSetState" );
    if( !XInputSetState )
    {
        XInputSetState = x_input_set_state_stub;
    }
}

internal void Win32ProcessButton( PlatformButton* button, b32 isDown )
{
    button->changedState[0] = isDown != button->endedDown[1];
    button->endedDown[0] = isDown;
}

internal void Win32ProcessKeyboardMessage( HWND window, PlatformInput* input, MSG message )
{
    b32 isDown = ( ( message.lParam & ( 1UL << 31 ) ) == 0 );
    b32 wasDown = ( ( message.lParam & ( 1UL << 30 ) ) != 0 );
    b32 altKeyDown = message.lParam & ( 1UL << 29 );
    if( isDown != wasDown )
    {
        switch( message.wParam )
        {
            case VK_OEM_COMMA:
            {
                // TODO(Leonardo): action left/right?
            } break;
            
            case 'W':
            {
                Win32ProcessButton( &input->moveUp, isDown );
            } break;
            
            case 'S':
            {
                Win32ProcessButton( &input->moveDown, isDown );
            } break;
            
            case 'D':
            {
                Win32ProcessButton( &input->moveRight, isDown );
            } break;
            
            case 'A':
            {
                Win32ProcessButton( &input->moveLeft, isDown );
            } break;
            
#if 0            
            
            case VK_SPACE:
            {
                Win32ProcessButton( &input->actionDown, isDown );
            } break;
#endif
            
            case VK_UP:
            {
                Win32ProcessButton(&input->actionUp, isDown);
            } break;
            
            case VK_DOWN:
            {
                Win32ProcessButton(&input->actionDown, isDown);
            } break;
            
            case VK_RIGHT:
            {
                Win32ProcessButton(&input->actionRight, isDown);
            } break;
            
            case VK_LEFT:
            {
                Win32ProcessButton(&input->actionLeft, isDown);
            } break;
            
            
            case VK_DELETE:
            {
                
            } break;
            
            case VK_BACK:
            {
                Win32ProcessButton(&input->backButton, isDown);
            } break;
            
            case VK_RETURN:
            {
                Win32ProcessButton(&input->confirmButton, isDown);
            } break;
            
            case VK_ESCAPE:
            {
                Win32ProcessButton( &input->escButton, isDown );
            } break;
            
            case VK_TAB:
            {
                Win32ProcessButton( &input->switchButton, isDown );
            } break;
            
            case 'I':
            {
                Win32ProcessButton( &input->inventoryButton, isDown );
            } break;
            
            case 'E':
            {
                Win32ProcessButton( &input->equipmentButton, isDown );
            } break;
            
            case VK_OEM_3:
            {
                Win32ProcessButton(&input->editorButton, isDown);
            } break;
            
            case '0':
            {
                Win32ProcessButton(input->slotButtons + 0, isDown);
            } break;
            
            case '1':
            {
                Win32ProcessButton(input->slotButtons + 1, isDown);
            } break;
            
            
            case '2':
            {
                Win32ProcessButton(input->slotButtons + 2, isDown);
            } break;
            
            
            case '3':
            {
                Win32ProcessButton(input->slotButtons + 3, isDown);
            } break;
            
            
            case '4':
            {
                Win32ProcessButton(input->slotButtons + 4, isDown);
            } break;
            
            
            case '5':
            {
                Win32ProcessButton(input->slotButtons + 5, isDown);
            } break;
            
            
            case '6':
            {
                Win32ProcessButton(input->slotButtons + 6, isDown);
            } break;
            
            
            case '7':
            {
                Win32ProcessButton(input->slotButtons + 7, isDown);
            } break;
            
            case '8':
            {
                Win32ProcessButton(input->slotButtons + 8, isDown);
            } break;
            
            case '9':
            {
                Win32ProcessButton(input->slotButtons + 9, isDown);
            } break;
            
            case 'C':
            {
                if(altKeyDown)
                {
                    Win32ProcessButton(&input->copyButton, isDown);
                }
            } break;
            
            case 'V':
            {
                if(altKeyDown)
                {
                    Win32ProcessButton(&input->pasteButton, isDown);
                }
            } break;
            
            case 'P':
            {
                if(altKeyDown)
                {
                    Win32ProcessButton(&input->pauseButton, isDown);
                }
            } break;
            
            case 'Z':
            {
                if(altKeyDown)
                {
                    Win32ProcessButton(&input->undo, isDown);
                }
            } break;
            
            case 'Y':
            {
                if(altKeyDown)
                {
                    Win32ProcessButton(&input->redo, isDown);
                }
            } break;
            
#if FORGIVENESS_INTERNAL
            case VK_F1:
            {
                if( altKeyDown )
                {
                    Win32ProcessButton( &input->reloadButton, isDown );
                }
            } break;		
#endif
            
            case VK_F3:
            {
                Win32ProcessButton(&input->exitButton, isDown);
            } break;
            
            
            case VK_F4:
            {
                if(altKeyDown)
                {
                    if(gameInput.allowedToQuit)
                    {
                        running = false;
                    }
                    else
                    {
                        if(MessageBox(window, "You have unsaved changes, do you want to quit anyway?", 0, MB_YESNO) == IDYES)
                        {
                            running = false;
                        }
                    }
                }
            } break;		
            
            case VK_F5:
            {
                if( isDown && altKeyDown )
                {
                    Win32ToggleFullScreen(window);
                }
            } break;
            
#if FORGIVENESS_INTERNAL
            
            case VK_F8:
            {
                Win32ProcessButton( &input->debugButton1, isDown );
            } break;
            
            case VK_F9:
            {
                Win32ProcessButton( &input->debugButton2, isDown );
            } break;
            
            case VK_END:
            {
                Win32ProcessButton( &input->debugCancButton, isDown );
            } break;
#endif
        }
    }
}

internal void Win32ClearSoundBuffer( Win32SoundBuffer* soundBuffer )
{
    void* region1;
    void* region2;
    DWORD region1Size;
    DWORD region2Size;
    soundBuffer->buffer->Lock( 0, soundBuffer->totalBufferSize, &region1, &region1Size, &region2, &region2Size, 0 );
    
    u8* samples = ( u8* ) region1;
    for( u32 counter = 0; counter < region1Size; counter++ )
    {
        *samples++ = 0;
    }
    
    samples = ( u8* ) region2;
    for( u32 counter = 0; counter < region2Size; counter++ )
    {
        *samples++ = 0;
    }
    
    soundBuffer->buffer->Unlock( region1, region1Size, region2, region2Size );
}

internal void Win32CopySamplesInBuffer( Win32SoundBuffer* soundBuffer, PlatformSoundBuffer* gameSoundBuffer,
                                       DWORD byteToLock, DWORD bytesToWrite )
{
    void* region1;
    void* region2;
    DWORD region1Size;
    DWORD region2Size;
    soundBuffer->buffer->Lock( byteToLock, bytesToWrite,
                              &region1, &region1Size, &region2, &region2Size, 0 );
    
    
    i16* sourceSample = gameSoundBuffer->samples;
    
    i32 countRegion1Samples = region1Size / soundBuffer->bytesPerSample;
    i16* destSample = ( i16* ) region1;
    for( i32 counter = 0; counter < countRegion1Samples; counter++ )
    {
        *destSample++ = *sourceSample++;
        *destSample++ = *sourceSample++;
        ++soundBuffer->runningSampleIndex;
    }
    
    i32 countRegion2Samples = region2Size / ( sizeof( i16 ) * 2 );
    destSample = ( i16* ) region2;
    for( i32 counter = 0; counter < countRegion2Samples; counter++ )
    {
        *destSample++ = *sourceSample++;
        *destSample++ = *sourceSample++;
        ++soundBuffer->runningSampleIndex;
    }
    
    soundBuffer->buffer->Unlock( region1, region1Size, region2, region2Size );
}

internal Win32Dimension Win32GetWindowDimension( HWND window )
{
    Win32Dimension result;
    RECT rect;
    GetClientRect( window, &rect );
    result.width = rect.right - rect.left;
    result.height = rect.bottom - rect.top;
    return result;
}

internal void Win32UpdateWindow( PlatformWorkQueue* renderQueue, GameRenderCommands* commands, HDC deviceContext, Rect2i drawRegion, i32 windowWidth, i32 windowHeight )
{
    TIMED_FUNCTION();
    if( !globalSoftwareRendering )
    {
        OpenGLRenderCommands( commands, drawRegion, windowWidth, windowHeight );
        
        BEGIN_BLOCK("swap buffer");
        SwapBuffers(deviceContext);
        END_BLOCK();
    }
    else
    {
        InvalidCodePath;
        
        Bitmap dest_ = {};
        dest_.width = globalScreenBuffer.width;
        dest_.height = globalScreenBuffer.height;
        dest_.pixels = globalScreenBuffer.memory;
        Bitmap* dest = &dest_;
        
        SoftwareRenderCommands( renderQueue, commands, dest );
        if( true )
        {
            //OpenGLDisplayBitmap( dest->width, dest->height, dest->pixels, windowWidth, windowHeight, opengl.blitTextureHandle );
            SwapBuffers( deviceContext );
        }
        else
        {
#if 0            
            if( windowWidth == 2 * dest->width && windowHeight == 2 * dest->height )
            {
                StretchDIBits( deviceContext,
                              0, 0, windowWidth, windowHeight,
                              0, 0, globalScreenBuffer.width, globalScreenBuffer.height,
                              globalScreenBuffer.memory, &globalScreenBuffer.info, DIB_RGB_COLORS, SRCCOPY );
            }
            else
            {
                StretchDIBits( deviceContext,
                              0, 0, globalScreenBuffer.width, globalScreenBuffer.height,
                              0, 0, globalScreenBuffer.width, globalScreenBuffer.height,
                              globalScreenBuffer.memory, &globalScreenBuffer.info, DIB_RGB_COLORS, SRCCOPY );
            }
#endif
            
        }
    }
}

internal void ResetInput()
{
    for( i32 buttonIndex = 0;
        buttonIndex < ArrayCount( gameInput.buttons );
        buttonIndex++ )
    {
        PlatformButton* button = gameInput.buttons + buttonIndex;
        Assert(ArrayCount(button->endedDown) == ArrayCount(button->changedState));
        for( u32 frameIndex = 0; frameIndex < ArrayCount ( button->endedDown );
            ++frameIndex )
        {
            button->endedDown[frameIndex] = 0;
            button->changedState[frameIndex] = 0;
        }
    }
}

internal void SlideInput()
{
    for( i32 buttonIndex = 0;
        buttonIndex < ArrayCount( gameInput.buttons );
        buttonIndex++ )
    {
        PlatformButton* button = gameInput.buttons + buttonIndex;
        
        for( u32 frameIndex = ArrayCount( button->endedDown ) - 1;
            frameIndex > 0;
            frameIndex-- )
        {
            button->endedDown[frameIndex] = button->endedDown[frameIndex - 1];
            button->changedState[frameIndex] = button->changedState[frameIndex - 1];
        }
        
        button->changedState[0] = false;
    }
    
    for(u8 charIndex = 0; charIndex < 0xff; ++charIndex)
    {
        gameInput.wasDown[charIndex] = gameInput.isDown[charIndex];
        gameInput.isDown[charIndex] = false;
    }
}


internal void Win32RunGame( GameRenderCommands* commands )
{
    DWORD targetCursor = ( lastPlayCursor + ( winSoundBuffer.delaySamples * winSoundBuffer.bytesPerSample ) ) % winSoundBuffer.totalBufferSize;
    u32 byteToLock = ( winSoundBuffer.runningSampleIndex *
                      winSoundBuffer.bytesPerSample ) % winSoundBuffer.totalBufferSize; 
    i32 bytesToWrite = 0;
    if( byteToLock > targetCursor )
    {
        bytesToWrite = winSoundBuffer.totalBufferSize - byteToLock;
        bytesToWrite += targetCursor;
    }
    else
    {
        bytesToWrite = targetCursor - byteToLock;
    }
    
    PlatformSoundBuffer gameSoundBuffer;
    gameSoundBuffer.samplesPerSecond = winSoundBuffer.samplesPerSecond;
    gameSoundBuffer.bytesPerSample = winSoundBuffer.bytesPerSample;
    gameSoundBuffer.sampleCount = Align8( bytesToWrite / gameSoundBuffer.bytesPerSample );
    bytesToWrite = gameSoundBuffer.sampleCount * gameSoundBuffer.bytesPerSample;
    gameSoundBuffer.samples = soundSamples;
    
    if( game.UpdateAndRender )
    {
        game.UpdateAndRender( &gameMemory, &gameInput, commands );
    }
    if( game.GetSoundOutput )
    {
        game.GetSoundOutput( &gameMemory, &gameInput, &gameSoundBuffer );
    }
    
    Win32CopySamplesInBuffer( &winSoundBuffer, &gameSoundBuffer, byteToLock, bytesToWrite );
    
    if( gameInput.quitRequested )
    {
        running = false;
    }
    
}

LRESULT CALLBACK Win32MainWindowCallBack( HWND window, UINT message, WPARAM wParam, LPARAM lParam )
{
    LRESULT result = 0;
    switch ( message )
    {
        case WM_CLOSE:
        {
            //TODO(leonardo): ask the player if he's sure about exiting the game
            running = false;
        } break;
        
        case WM_SETCURSOR:
        {
#ifndef FORGIVENESS_INTERNAL
            SetCursor( 0 );
#endif
        } break;
        
        case WM_DESTROY:
        {
            OutputDebugString("destroy\n");
        } break;
        
        case WM_QUIT:
        {
            //TODO(leonardo): what's the difference between wquit and close?
            running = false;
        } break;
        
        case WM_WINDOWPOSCHANGING:
        {
            WINDOWPOS* newPos = ( WINDOWPOS* ) lParam;
            
            if( !gameInput.mouseLeft.endedDown && globalScreenBuffer.width && globalScreenBuffer.height )
            {
                RECT windowRect;
                RECT clientRect;
                
                GetWindowRect( window, &windowRect );
                GetClientRect( window, &clientRect );
                
                i32 widthAdd = ( windowRect.right - windowRect.left ) - ( clientRect.right - clientRect.left );
                i32 heightAdd = ( windowRect.top - windowRect.bottom ) - ( clientRect.top - clientRect.bottom );
                
                i32 renderWidth = globalScreenBuffer.width;
                i32 renderHeight = globalScreenBuffer.height;
                
                i32 newCx = ( renderWidth * ( newPos->cy - heightAdd ) ) / renderHeight;
                i32 newCy = ( renderHeight * ( newPos->cx - widthAdd ) ) / renderWidth;
                
                if( Abs( newPos->cx - newCx ) > Abs( newPos->cy - newCy ) )
                {
                    newPos->cx = newCx + widthAdd;
                }
                else
                {
                    newPos->cy = newCy + heightAdd;
                }
            }
            result = DefWindowProcA( window, message, wParam, lParam );
        } break;
        
        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYUP:
        case WM_SYSKEYDOWN:
        {
            Assert( !"input must not come across this way!!" );
        } break;
        
        case WM_SIZE:
        {
            //NOTE(leonardo):we are not resizing the buffer for now, we just want the buffer to be the same
            //size from the beginning of the program
        } break;
        
        case WM_ENTERSIZEMOVE:
        {
            freezing = true;
            SetTimer( window, 1, 33, 0 );
        } break;
        
        case WM_EXITSIZEMOVE:
        {
            freezing = false;
        } break;
        
        case WM_TIMER:
        {
            // TODO( Leonardo ): at the moment the hack is not working...`
            if( freezing )
            {
                KillTimer( window, 1 );
                //Win32RunGame( true );
                SetTimer( window, 1, 33, 0 );
            }
        } break;
        
#if 0
        case WM_PAINT:
        {
            
            PAINTSTRUCT paint;
            HDC deviceContext = BeginPaint( window, &paint );
            LONG x = paint.rcPaint.left;
            LONG y = paint.rcPaint.top;
            LONG width = paint.rcPaint.right - paint.rcPaint.left;
            LONG height = paint.rcPaint.bottom - paint.rcPaint.top;
            Win32UpdateWindow( &globalScreenBuffer, deviceContext, width, height );
            EndPaint( window, &paint );
        } break;
#endif
        
        case WM_ACTIVATEAPP:
        {
            OutputDebugString("active\n");
        } break;
        
        default:
        {
            result = DefWindowProc( window, message, wParam, lParam );
        }
    }
    return result;
}

inline LARGE_INTEGER Win32GetWallClock()
{
    LARGE_INTEGER result;
    QueryPerformanceCounter( &result );
    return result;
}

internal r32 Win32GetSecondsElapsed( LARGE_INTEGER lastCounter, LARGE_INTEGER endCounter )
{
    i64 elapsedCount = endCounter.QuadPart - lastCounter.QuadPart;
    r32 secElapsed = ( ( r32 ) elapsedCount ) / globalFrequency;
    return secElapsed;
}


PLATFORM_ERROR_MESSAGE(Win32ErrorMessage)
{
    MessageBox(globalWindow, message, 0, MB_OK);
    InvalidCodePath;
}


#if 0
PLATFORM_GET_CLIPBOARD(Win32GetClipboardText)
{
    HANDLE h;
    
    if (!OpenClipboard(NULL))
    {
        InvalidCodePath;
    }
    
    h = GetClipboardData(CF_TEXT);
    FormatString(buffer, bufferLength, "%s", (char*) h);
    
    CloseClipboard();
}

PLATFORM_SET_CLIPBOARD(Win32SetClipboardText)
{
    HGLOBAL hMem =  GlobalAlloc(GMEM_MOVEABLE, textLength);
    memcpy(GlobalLock(hMem), text, textLength);
    GlobalUnlock(hMem);
    
    OpenClipboard(0);
    EmptyClipboard();
    SetClipboardData(CF_TEXT, hMem);
    CloseClipboard();
}
#endif

#if FORGIVENESS_INTERNAL
global_variable DebugTable globalDebugTable_;
DebugTable* globalDebugTable = &globalDebugTable_;
#endif

internal InitRenderBufferParams AllocateQuads(u32 maxQuadCount)
{
    InitRenderBufferParams result = {};
    result.maxQuadCount = maxQuadCount;
    
    u32 maxVertexCount = maxQuadCount * 4;
    u32 maxIndexCount = maxQuadCount * 6;
    
    PlatformMemoryBlock* vertexBlock = Win32AllocateMemory( maxVertexCount * sizeof( TexturedVertex ), 0 );
    result.vertexes = (TexturedVertex*) vertexBlock->base;
    
    PlatformMemoryBlock* indexBlock = Win32AllocateMemory( maxIndexCount * sizeof(u16), 0 );
    result.indeces = (u16*) indexBlock->base;
    
    PlatformMemoryBlock* sortBlock = Win32AllocateMemory( maxQuadCount * sizeof(r32), 0 );
    result.sortKeys = (r32*) sortBlock->base;
    
    return result;
}

int CALLBACK WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR commandLine, int showCode)
{
    SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER) CreateMiniDump);
    
    globalMemorySentinel.next = &globalMemorySentinel;
    globalMemorySentinel.prev = &globalMemorySentinel;
    
    Assert( sizeof( r32 ) == 4 );
    
    char* DLLName = "forg_client.dll";
    char* tempDLLName = "forg_client_temp.dll";
    char* lockName = "lock.tmp";
    char ExeFullPath[MAX_PATH];
    GetModuleFileName( 0, ExeFullPath, sizeof( ExeFullPath ) );
    
    char DLLFullName[MAX_PATH];
    char tempDLLFullName[MAX_PATH];	
    char lockFullName[MAX_PATH];	
    Win32BuildFullPath( ExeFullPath, DLLName, DLLFullName, sizeof( DLLFullName ) );
    Win32BuildFullPath( ExeFullPath, tempDLLName, 
                       tempDLLFullName, sizeof( tempDLLFullName ) );
    Win32BuildFullPath( ExeFullPath, lockName, 
                       lockFullName, sizeof( lockFullName ) );
    
    game = Win32LoadGameCode( DLLFullName, 
                             tempDLLFullName, 
                             lockFullName );
    
    Win32LoadXInput();
    
    WNDCLASS windowClass  = {};
    windowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    windowClass.lpfnWndProc = Win32MainWindowCallBack;
    windowClass.hInstance = hPrevInstance;
    windowClass.hCursor = LoadCursor( 0, IDC_ARROW );
    windowClass.lpszClassName = "forgiveness";
    windowClass.hbrBackground = ( HBRUSH ) GetStockObject( BLACK_BRUSH );
    
    if( RegisterClass( &windowClass) )
    {
        HWND window = CreateWindowEx( 0, 
                                     windowClass.lpszClassName, 
                                     "forgiveness", 
                                     WS_OVERLAPPEDWINDOW,
                                     CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,CW_USEDEFAULT,
                                     0, 0, hInstance, 0 );
        
        if( window )
        {
            HDC openGLDC = GetDC( window );
            
            u16 maxTextureCount = MAX_TEXTURE_COUNT + MAX_SPECIAL_TEXTURE_COUNT;
            HGLRC openGLRC = Win32InitOpenGL(openGLDC, maxTextureCount);
            
            Win32ThreadStartup highPriorityStartups[6] = {};
            PlatformWorkQueue highQueue = {};
            Win32MakeQueue( &highQueue, ArrayCount( highPriorityStartups ), highPriorityStartups );
            
            Win32ThreadStartup lowPriorityStartups[2] = {};
            PlatformWorkQueue lowQueue = {};
            Win32MakeQueue( &lowQueue, ArrayCount( lowPriorityStartups ), lowPriorityStartups );
            
            
            globalWindow = window;
            
            int monitorRefreshRate = 30;
            
#if 1     
            HDC refreshDC = GetDC( window );
            int win32RefreshRate = GetDeviceCaps( refreshDC, VREFRESH );
            ReleaseDC( window, refreshDC );
            if( win32RefreshRate > 1 )
            {
                monitorRefreshRate = win32RefreshRate;
            }
#endif
            
            r32 gameUpdateHz = ( r32 ) monitorRefreshRate;
            int expectedFramesPerUpdate = 1;
            targetSecPerFrame = ( r32 ) expectedFramesPerUpdate / monitorRefreshRate;
            
            b32 sleepIsGranular = ( timeBeginPeriod( 1 ) == TIMERR_NOERROR );
            Assert( sleepIsGranular );
            
            
            
            
            RECT clientRect;
            GetClientRect( window, &clientRect );
            
            u16 windowWidth = ( u16 ) ( clientRect.right - clientRect.left );
            u16 windowHeight = ( u16 ) ( clientRect.top - clientRect.bottom );
            //Win32ResizeWindow( &globalScreenBuffer, windowWidth, windowHeight);
            //Win32ResizeWindow(&globalScreenBuffer, 960, 540);
            Win32ResizeWindow(&globalScreenBuffer, 1920, 1080);
            
            //NOTE(leonardo): sound initialization
            winSoundBuffer.samplesPerSecond = 48000;
            winSoundBuffer.bytesPerSample = sizeof( i16 ) * 2;
            winSoundBuffer.totalBufferSize = winSoundBuffer.samplesPerSecond * winSoundBuffer.bytesPerSample;
            winSoundBuffer.runningSampleIndex = 0;
            winSoundBuffer.delaySamples = (i32) (0.6f * (winSoundBuffer.samplesPerSecond / targetSecPerFrame * 1000.0f));
            
            u32 maxOverrun = 8 * 2 * sizeof( i16 );
            soundSamples = ( i16* ) VirtualAlloc( 0, winSoundBuffer.totalBufferSize + maxOverrun,
                                                 MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE );
            
            Win32InitDSound( window, winSoundBuffer.samplesPerSecond, winSoundBuffer.totalBufferSize );
            Win32ClearSoundBuffer( &winSoundBuffer );
            winSoundBuffer.buffer->Play( 0, 0, DSBPLAY_LOOPING );
            
            
            
            gameMemory.gameState = 0;
#if FORGIVENESS_INTERNAL
            gameMemory.debugTable = globalDebugTable;
            gameMemory.debugState = 0;
#endif
            
            gameMemory.api.net = Win32NetworkAPI;
            
            gameMemory.api.CompleteQueueWork = Win32CompleteQueueWork;
            gameMemory.api.PushWork = Win32PushWork;
            
            
            gameMemory.api.GetAllSubdirectories = Win32GetAllSubdirectories;
            gameMemory.api.CloseFile = Win32CloseFile;
            gameMemory.api.GetAllFilesBegin = Win32GetAllFilesBegin;
            gameMemory.api.OpenFile = Win32OpenFile;
            gameMemory.api.GetAllFilesEnd = Win32GetAllFilesEnd;
            gameMemory.api.ReadFromFile = Win32ReadFromFile;
            gameMemory.api.FileError = Win32FileError;
            gameMemory.api.ReplaceFile = Win32ReplaceFile;
            
            gameMemory.api.AllocateMemory = Win32AllocateMemory;
            gameMemory.api.DeallocateMemory = Win32DeallocateMemory;
            gameMemory.api.DEBUGMemoryStats = Win32GetMemoryStats;
            
            
            gameMemory.highPriorityQueue = &highQueue;
            gameMemory.lowPriorityQueue = &lowQueue;
            
            gameMemory.api.DEBUGExecuteSystemCommand = DEBUGWin32ExecuteCommand;
            gameMemory.api.DEBUGGetProcessState = DEBUGWin32GetProcessState;
            gameMemory.api.DEBUGExistsProcessWithName = Win32ExistsProcessWithName;
            gameMemory.api.DEBUGKillProcessByName = Win32KillProcessByName;
            
            gameMemory.api.ErrorMessage = Win32ErrorMessage;
            
            
            u32 textureOpCount = 1024;
            PlatformTextureOpQueue* textureQueue = &gameMemory.textureQueue;
            textureQueue->firstFree = ( TextureOp* ) VirtualAlloc( 0, textureOpCount * sizeof( TextureOp ), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE );
            for( u32 opIndex = 0; opIndex < textureOpCount - 1; ++opIndex )
            {
                TextureOp* op = textureQueue->firstFree + opIndex;
                op->next = textureQueue->firstFree + ( opIndex + 1 ); 
            }
            
            platformAPI = gameMemory.api;
            
            LPVOID baseAddress = 0;
#if FORGIVENESS_INTERNAL
            baseAddress = ( LPVOID ) TeraBytes( ( u64 ) 2 );
#endif
            if(soundSamples)
            {
                if(Win32InitNetwork())
                {
                    PlatformMemoryBlock* networkBlock = Win32AllocateMemory(sizeof(NetworkInterface), PlatformMemory_NotRestored);
                    Assert(networkBlock->base);
                    gameInput.network = (NetworkInterface*) networkBlock->base;
                    
                    running = true;
                    HDC deviceContext = GetDC( window );
                    
                    LARGE_INTEGER perfFrequency;
                    QueryPerformanceFrequency( &perfFrequency );
                    globalFrequency = perfFrequency.QuadPart;
                    
                    LARGE_INTEGER lastCounter = Win32GetWallClock();
                    
                    u32 pushBufferSize = MegaBytes( 16 );
                    PlatformMemoryBlock* pushBufferBlock = Win32AllocateMemory(pushBufferSize, PlatformMemory_NotRestored);
                    u8* pushBuffer = pushBufferBlock->base;
                    
                    InitRenderBufferParams opaque = AllocateQuads(1 << 16);
                    InitRenderBufferParams transparent = AllocateQuads(1 << 12);
                    
                    
                    b32 xboxControllerPresent = true;
                    b32 executableNeedsToBeRealoaded = false;
                    //Win32ToggleFullScreen( window );
                    ShowWindow( window, SW_SHOW );
                    
                    PlatformMemoryBlock* noiseTextureBlock =Win32AllocateMemory(NOISE_WIDTH * NOISE_HEIGHT * sizeof(Vec4), 0);
                    
                    for(u32 y = 0; y < NOISE_HEIGHT; ++y)
                    {
                        for(u32 x = 0; x < NOISE_WIDTH; ++x)
                        {
                            r32 dx = (r32) x / (r32) NOISE_WIDTH;
                            r32 dy = (r32) y / (r32) NOISE_HEIGHT;
                            Vec4* dest = ((Vec4*) noiseTextureBlock->base) + (y * NOISE_WIDTH + x);
                            r32 noise = UnilateralNoise(dx, dy, 20.0f, 0);
                            *dest = V4(noise, noise, noise, noise);
                        }
                    }
                    
                    while(running)
                    {
                        BEGIN_BLOCK("setup renderer");
                        gameInput.timeToAdvance = targetSecPerFrame;
                        
                        GameRenderCommands renderCommands =DefaultRenderCommands( pushBuffer, pushBufferSize, globalScreenBuffer.width, globalScreenBuffer.height, opaque, transparent, V4( 0, 0, 0, 1 ), noiseTextureBlock->base);
                        Win32Dimension dimension = Win32GetWindowDimension( window );
                        Rect2i drawRegion = AspectRatioFit( renderCommands.settings.width, renderCommands.settings.height, dimension.width, dimension.height );
                        END_BLOCK();
                        
                        BEGIN_BLOCK("input processing");
                        POINT mousePos;
                        GetCursorPos( &mousePos );
                        ScreenToClient( window, &mousePos);
                        
                        SlideInput();
                        
                        gameInput.serverEXE = commandLine;
                        
                        if(StrLen(gameInput.serverEXE) == 0)
                        {
                            gameInput.serverEXE = "build/win32_server.exe";
                        }
                        
                        gameInput.mouseWheelOffset = 0;
                        r32 mouseX = ( r32 ) mousePos.x;
                        r32 mouseY = ( ( r32 ) dimension.height - 1.0f) - ( r32 ) mousePos.y;
                        
                        gameInput.normalizedMouseX = Clamp01MapToRange( ( r32 ) drawRegion.minX, mouseX, ( r32 ) drawRegion.maxX );
                        gameInput.normalizedMouseY = Clamp01MapToRange( ( r32 ) drawRegion.minY, mouseY, ( r32 ) drawRegion.maxY );
                        
                        gameInput.mouseX = renderCommands.settings.width * gameInput.normalizedMouseX;
                        gameInput.mouseY = renderCommands.settings.height * gameInput.normalizedMouseY;
                        
                        gameInput.shiftDown = ( GetKeyState( VK_SHIFT ) & ( 1 << 15 ) );
                        gameInput.altDown = ( GetKeyState( VK_MENU ) & ( 1 << 15 ) );
                        gameInput.ctrlDown = ( GetKeyState( VK_CONTROL ) & ( 1 << 15 ) );
                        
                        
                        for(u8 charIndex = '0'; charIndex <= '9'; ++charIndex)
                        {
                            if(GetAsyncKeyState(charIndex) & 0x8000)
                            {
                                gameInput.isDown[charIndex] = true;
                            }
                        }
                        
                        u8 deltaLetters = 'a' - 'A';
                        for(u8 charIndex = 'A'; charIndex <= 'Z'; ++charIndex)
                        {
                            if(GetAsyncKeyState(charIndex) & 0x8000)
                            {
                                if(gameInput.shiftDown)
                                {
                                    gameInput.isDown[charIndex] = true;
                                }
                                else
                                {
                                    gameInput.isDown[charIndex + deltaLetters] = true;
                                }
                            }
                        }
                        if(GetAsyncKeyState(' ') & 0x8000)
                        {
                            gameInput.isDown[' '] = true;
                        }
                        
                        
						if(GetAsyncKeyState(VK_OEM_PLUS) & 0x8000)
                        {
							if(gameInput.shiftDown)
							{
								gameInput.isDown['+'] = true;	
							}
                        }
                        
						if(GetAsyncKeyState(VK_OEM_MINUS) & 0x8000)
                        {
                            if(gameInput.shiftDown)
                            {
                                gameInput.isDown['_'] = true;
                            }
                            else
                            {
                                gameInput.isDown['-'] = true;
                            }
                        }
                        
						if(GetAsyncKeyState(VK_OEM_PERIOD) & 0x8000)
                        {
                            gameInput.isDown['.'] = true;
                        }
                        
                        
                        
                        MSG message;
                        while( PeekMessage( &message, 0, 0, 0, PM_REMOVE ) > 0 )
                        {
                            switch( message.message )
                            {
                                case WM_KEYUP:
                                case WM_SYSKEYUP:
                                case WM_KEYDOWN:
                                case WM_SYSKEYDOWN:
                                {
                                    Win32ProcessKeyboardMessage( window, &gameInput, message );
                                } break;
                                
                                case WM_LBUTTONDOWN:
                                {
                                    Win32ProcessButton( &gameInput.mouseLeft, true );
                                } break;
                                
                                case WM_LBUTTONUP:
                                {
                                    Win32ProcessButton( &gameInput.mouseLeft, false );
                                } break;
                                
                                case WM_RBUTTONDOWN:
                                {
                                    Win32ProcessButton( &gameInput.mouseRight, true );
                                } break;
                                
                                case WM_RBUTTONUP:
                                {
                                    Win32ProcessButton( &gameInput.mouseRight, false );
                                } break;
                                
                                case WM_MBUTTONDOWN:
                                {
                                    Win32ProcessButton( &gameInput.mouseCenter, true );
                                } break;
                                
                                case WM_MBUTTONUP:
                                {
                                    Win32ProcessButton( &gameInput.mouseCenter, false );
                                } break;
                                
                                case WM_MOUSEWHEEL:
                                {
                                    gameInput.mouseWheelOffset = ( i32 ) ( ( i16 ) ( message.wParam >> 16 ) / WHEEL_DELTA );
                                } break;
                                
                                default:
                                {
                                    TranslateMessage( &message );
                                    DispatchMessage( &message );
                                } break;
                            }	
                        }
                        
                        //TODO(leonardo): check if this way of getting input is correct also with controller.
                        //NOTE: we are polling just from the first controller!
                        for (DWORD countController = 0; countController < 1; countController++ )
                        {
                            XINPUT_STATE state;
                            if( xboxControllerPresent && XInputGetState( countController, &state ) == ERROR_SUCCESS )
                            {
                                XINPUT_GAMEPAD* pad = &state.Gamepad;
                                
                                r32 stickThresold = 0.5f;
                                
                                PlatformButton* moveUp = &gameInput.moveUp;
                                PlatformButton* moveDown = &gameInput.moveDown;
                                PlatformButton* moveRight = &gameInput.moveRight;
                                PlatformButton* moveLeft = &gameInput.moveLeft;
                                
                                if( pad->sThumbLX > 0 )
                                {
                                    if( pad->sThumbLX > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE )
                                    {
                                        ( pad->sThumbLX / 32767 >= stickThresold ) ?
                                            moveRight->endedDown[0] = true : moveRight->endedDown[0] = false;
                                    }
                                }
                                else
                                {
                                    if( pad->sThumbLX < -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE )
                                    {
                                        ( pad->sThumbLX / 32768 <= -stickThresold ) ?
                                            moveLeft->endedDown[0] = true : moveLeft->endedDown[0] = false;
                                    }
                                }
                                
                                if( pad->sThumbLY > 0 )
                                {
                                    if( pad->sThumbLY > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE )
                                    {
                                        ( pad->sThumbLY / 32767 >= stickThresold ) ?
                                            moveUp->endedDown[0] = true : moveDown->endedDown[0] = false;
                                    }
                                }
                                else
                                {
                                    if( pad->sThumbLY < -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE )
                                    {
                                        ( pad->sThumbLY / 32768 <= -stickThresold ) ?
                                            moveDown->endedDown[0] = true : moveDown->endedDown[0] = false;
                                    }
                                }
                                
                                Win32ProcessButton( moveUp, ( pad->wButtons & XINPUT_GAMEPAD_DPAD_UP ) );
                                Win32ProcessButton( moveDown, ( pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN ) );
                                Win32ProcessButton( moveRight, ( pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT ) );
                                Win32ProcessButton( moveLeft, ( pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT ) );
                                //Win32ProcessButton( &gameInput.actionDown, ( pad->wButtons & XINPUT_GAMEPAD_A ) );
                                //Win32ProcessButton( &gameInput.actionRight, ( pad->wButtons & XINPUT_GAMEPAD_B ) );
                                //Win32ProcessButton( &gameInput.actionLeft, ( pad->wButtons & XINPUT_GAMEPAD_X ) );
                                //Win32ProcessButton( &gameInput.actionUp, ( pad->wButtons & XINPUT_GAMEPAD_Y ) );
                                //Win32ProcessButton( &gameInput.startButton, ( pad->wButtons & XINPUT_GAMEPAD_START ) );
                                Win32ProcessButton( &gameInput.escButton, ( pad->wButtons & XINPUT_GAMEPAD_BACK ) );
                            }
                            else
                            {
                                xboxControllerPresent = false;
                            }
                        }
                        END_BLOCK();
                        
                        BEGIN_BLOCK( "game updated" );
                        Win32RunGame(&renderCommands);
                        END_BLOCK();
                        
#if FORGIVENESS_INTERNAL
                        BEGIN_BLOCK("dll reload");
                        gameMemory.DLLReloaded = false;
                        executableNeedsToBeRealoaded = false;
                        
                        WIN32_FILE_ATTRIBUTE_DATA DLLFileAttributes;
                        if( GetFileAttributesEx( DLLFullName, GetFileExInfoStandard, &DLLFileAttributes ) )
                        {
                            FILETIME DLLLastWriteTime = DLLFileAttributes.ftLastWriteTime;
                            if( CompareFileTime( &game.lastWriteTime, &DLLLastWriteTime ) != 0 )
                            {
                                executableNeedsToBeRealoaded = true;
                                Win32CompleteQueueWork( &highQueue );
                                Win32CompleteQueueWork( &lowQueue );
                            }
                        }
                        if(executableNeedsToBeRealoaded)
                        {
                            Win32UnloadGameCode( &game );
                            while( !game.isValid )
                            {
                                game = Win32LoadGameCode( DLLFullName, tempDLLFullName, lockFullName );
                            }
                            gameMemory.DLLReloaded = true;
                        }
                        END_BLOCK();
#endif
                        
#if 0
                        r32 secElapsed = Win32GetSecondsElapsed(lastCounter, Win32GetWallClock());
                        BEGIN_BLOCK( "sleep" );
                        if(secElapsed < targetSecPerFrame)
                        {
                            while(secElapsed < targetSecPerFrame)
                            {
                                if(sleepIsGranular)
                                {
                                    r32 secToSleep = targetSecPerFrame - secElapsed;
                                    Sleep( ( i32 ) ( secToSleep * 1000.0f ) );
                                }
                                
                                secElapsed = Win32GetSecondsElapsed(lastCounter, Win32GetWallClock());
                            }
                        }
                        END_BLOCK();
#endif
                        
                        BEGIN_BLOCK( "texture manage" );
                        
                        BeginTicketMutex( &textureQueue->mutex );
                        TextureOp* firstOp = textureQueue->first;
                        TextureOp* lastOp = textureQueue->last;
                        textureQueue->last = textureQueue->first = 0;
                        EndTicketMutex( &textureQueue->mutex );
                        
                        if(firstOp)
                        {
                            Assert(lastOp);
                            OpenGLManageTextures(firstOp);
                            BeginTicketMutex(&textureQueue->mutex);
                            lastOp->next = textureQueue->firstFree;
                            textureQueue->firstFree = firstOp;
                            EndTicketMutex(&textureQueue->mutex);
                        }
                        END_BLOCK();
                        
                        BEGIN_BLOCK("update window");
                        Win32UpdateWindow(&highQueue, &renderCommands, deviceContext, drawRegion, dimension.width, dimension.height);
                        END_BLOCK();
                        winSoundBuffer.buffer->GetCurrentPosition( &lastPlayCursor, &writeCursor );
                        
                        BEGIN_BLOCK("debug system");
                        if(game.DEBUGFrameEnd)
                        {
                            game.DEBUGFrameEnd();
                        }
                        END_BLOCK();
                        
                        LARGE_INTEGER endCounter = Win32GetWallClock();
                        r32 measuredSecondsPerFrame = Win32GetSecondsElapsed(lastCounter, endCounter);
                        r32 exactTargetFramesPerUpdate = measuredSecondsPerFrame * monitorRefreshRate;
                        u32 newExpectedFramesPerUpdate = RoundReal32ToI32(exactTargetFramesPerUpdate);
                        expectedFramesPerUpdate = newExpectedFramesPerUpdate;
                        
                        targetSecPerFrame = measuredSecondsPerFrame;
                        lastCounter = endCounter;
                        
                        FRAME_MARKER(measuredSecondsPerFrame);
                    }	
                    
                    Win32CloseConnection(gameInput.network, 0);
                    WSACleanup();
                }
            }
            else
            {
                //TODO(leonardo): error initializing socket
            }
            
        }
        else
        {
            //TODO(leonardo): diagnostic
        }
        
    }
    else
    {
        //TODO(leonardo):diagnostic;
    }
}