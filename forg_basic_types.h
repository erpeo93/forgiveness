#pragma once


#include <stdint.h>
// TODO(Leonardo): move these!
#include <windows.h>
#include <intrin.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float r32;
typedef double r64;

typedef i32 b32;

typedef size_t memory_index;
typedef uintptr_t unm;
typedef intptr_t snm;


union Vec2
{
    struct
    {
        r32 x;
        r32 y;
    };
    
    r32 E[2];
    
    inline Vec2& operator *= ( r32 c );
    inline Vec2& operator += ( Vec2 v );
    inline Vec2& Vec2::operator -= ( Vec2 v );
};

union Vec3
{
    struct
    {
        r32 x;
        r32 y;
        r32 z;
    };
    
    struct
    {
        r32 r;
        r32 g;
        r32 b;
    };
    
    struct
    {
        Vec2 xy;
        r32 ignored;
    };
    
    struct
    {
        r32 ignored;
        Vec2 yz;
    };
    
    r32 E[3];
    
    inline Vec3& operator *= ( r32 c );
    inline Vec3& operator += ( Vec3 v );
    inline Vec3& Vec3::operator -= ( Vec3 v );
};

union Vec4
{
    struct
    {
        r32 x;
        r32 y;
        r32 z;
        r32 w;
    };
    
    struct
    {
        union
        {
            Vec3 rgb;
            struct
            {
                r32 r;
                r32 g;
                r32 b;
            };
        };
        r32 a;
    };
    
    struct
    {
        Vec3 xyz;
        r32 ignoredw;
    };
    
    struct
    {
        Vec2 xy;
        r32 ignored_0;
        r32 ignored_1;
    };
    
    struct
    {
        r32 ignored_2;
        Vec2 yz;
        r32 ignored_3;
    };
    
    struct
    {
        r32 ignored_4;
        r32 ignored_5;
        Vec2 zw;
    };
    
    r32 E[4];
    
    inline Vec4& operator *= ( r32 c );
    inline Vec4& operator += ( Vec4 v );
    inline Vec4& Vec4::operator -= ( Vec4 v );
};


#if FORGIVENESS_SLOW
#define Assert( expression ) if( !( expression ) ) {*((int*) 0 ) = 0; }
#else
#define Assert( expression )
#endif

#define InvalidCodePath Assert(!"noooo");
#define InvalidDefaultCase default: { InvalidCodePath; }

#if FORGIVENESS_INTERNAL
#define NotImplemented InvalidCodePath
#else
#define NotImplemented NotImplemented!!!!!!!!!!!!
#endif

#if 0
#undef NotImplemented
#define NotImplemented InvalidCodePath
#endif
