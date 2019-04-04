#pragma once

#include <windows.h>
#include <intrin.h>
#include <math.h>

inline r32 U8ToR32(u8 value, r32 max)
{
    r32 result = (r32) value / (r32) 0xff * max;
    return result;
}

inline i32 RoundReal32ToI32( r32 value )
{
    i32 result = _mm_cvtss_si32( _mm_set_ss( value ) );
    return result;
}

inline u32 RoundReal32ToU32( r32 value )
{
    u32 result = ( u32 ) _mm_cvtss_si32( _mm_set_ss( value ) );
    return result;
}

inline r32 SquareRoot( r32 value )
{
    r32 result = _mm_cvtss_f32( _mm_sqrt_ss( _mm_set_ss( value ) ) );
    
    return result;
}

inline u32 SafeTruncateUInt64ToU32( u64 value )
{
    Assert( value <= 0xFFFFFFFF );
    return ( u32 ) value;
}

inline i32 SafeTruncateInt64ToInt32( i64 value )
{
    Assert(value <= 0xFFFFFFFF);
    return (i32) value;
}

inline u16 SafeTruncateToU16(i32 value)
{
    Assert( value <= 65535 );
    Assert( value >= 0 );
    return ( u16 ) value;
}

inline u8 SafeTruncateToU8( i32 value )
{
    Assert( value <= 255 );
    Assert( value >= 0 );
    return ( u8 ) value;
}

inline i32 TruncateReal32ToI32( r32 value )
{
    i32 result = ( i32 ) value;
    return result;
}

inline u32 TruncateReal32ToU32( r32 value )
{
    u32 result = ( u32 ) value;
    return result;
}

inline i16 TruncateReal32ToI16(r32 value)
{
    i16 result = (i16) value;
    return result;
}

inline i32 Floor( r32 value )
{
    __m128 f = _mm_set_ss(value);
    __m128 one = _mm_set_ss(1.0f);
    
    __m128 t = _mm_cvtepi32_ps(_mm_cvttps_epi32(f));
    __m128 r = _mm_sub_ps(t, _mm_and_ps(_mm_cmplt_ps(f, t), one));
    
    return (i32) _mm_cvtss_f32(r);
}

inline i32 Ceil( r32 value )
{
    i32 result = ( i32 ) ceilf( value );
    return result;
}

//TODO(leonardo):irinsics on this!!
inline u32 CountLastSignificantBitSet( u32 value )
{
    u32 result = 0;
    u32 counter = 0;
    while( !( value & ( 1 << counter++ ) ) )
    {
        result++;
    }
    return result;
}

//TODO(leonardo):irinsics!
inline r32 Abs( r32 value )
{
    r32 result = value;
    if( value < 0 )
    {
        result = -value;
    }
    return result;
}

inline i32 Abs( i32 value )
{
    i32 result = value;
    if( value < 0 )
    {
        result = -value;
    }
    return result;
}

inline r32 Sin(r32 value)
{
    r32 result = sinf(value);
    return result;
}

inline r32 Cos(r32 value)
{
    r32 result = cosf(value);
    return result;
}

inline r32 ArcSin(r32 value)
{
    r32 result = asinf(value);
    return result;
}

inline r32 ArcCos(r32 value)
{
    r32 result = acosf( value );
    return result;
}

inline r32 Mod(r32 value, r32 max)
{
    r32 result = (r32) fmod(value, max);
    return result;
}

inline r32 Log10(r32 value)
{
    r32 result = (r32) log10(value);
    return result;
}

inline r32 Pow(r32 value, r32 exp)
{
    r32 result = (r32) pow(value, exp);
    return result;
}

