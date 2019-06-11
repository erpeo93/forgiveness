#pragma once

//
//
//SCALAR OPERATIONS
//
//

inline b32 Normalized( r32 value )
{
    b32 result = ( value >= 0.0f && value <= 1.0f );
    return result;
}

inline r32 SafeRatioN( r32 dividend, r32 n, r32 divisor )
{
    r32 result = n;
    if( divisor != 0.0f )
    {
        result = dividend / divisor;
    }
    
    return result;
}

inline r32 SafeRatio1( r32 dividend, r32 divisor )
{
    r32 result = SafeRatioN( dividend, 1.0f, divisor );
    return result;
}

inline r32 SafeRatio0( r32 dividend, r32 divisor )
{
    r32 result = SafeRatioN( dividend, 0.0f, divisor );
    return result;
}

inline r32 Square( r32 value )
{
    r32 result = value * value;
    return result;
}

inline r32 Square( i32 value )
{
    r32 result = (r32 ) ( value * value );
    return result;
}


inline i32 Squarei( i32 value )
{
    i32 result = value * value;
    return result;
}

inline r32 Dot( Vec2 v1, Vec2 v2 )
{
    r32 result = v1.x * v2.x + v1.y * v2.y;
    return result;
}

inline r32 Dot( Vec3 v1, Vec3 v2 )
{
    r32 result = v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
    return result;
}

inline r32 Dot( Vec4 v1, Vec4 v2 )
{
    r32 result = v1.x * v2.x + v1.y * v2.y + v1.z * v2.z + v1.w * v2.w;
    return result;
}

inline Vec3 Cross( Vec3 A, Vec3 B )
{
    Vec3 result;
    result.x = A.y * B.z - A.z * B.y;
    result.y = A.z * B.x - A.x * B.z;
    result.z = A.x * B.y - A.y * B.x;
    
    
    return result;
}

inline r32 Outer(Vec2 from, Vec2 to, Vec2 p)
{
    r32 result = (p.x - from.x) * (to.y - from.y) - (p.y - from.y) * (to.x - from.x);
    
    return result;
}

inline r32 LengthSq( Vec2 v )
{
    r32 result = Dot( v, v );
    return result;
}

inline r32 LengthSq( Vec3 v )
{
    r32 result = Dot( v, v );
    return result;
}

inline r32 LengthSq( Vec4 v )
{
    r32 result = Dot( v, v );
    return result;
}

inline r32 Length( Vec2 v )
{
    r32 result = SquareRoot( LengthSq( v ) );
    return result;
}

inline r32 Length( Vec3 v )
{
    r32 result = SquareRoot( LengthSq( v ) );
    return result;
}

inline r32 DegToRad( r32 degrees )
{
    r32 result = degrees / 180.0f * PI32;
    return result;
}

inline r32 RadToDeg( r32 rad )
{
    r32 result = rad * 180.0f / PI32;
    return result;
}

r32 RadiantsBetweenVectors( Vec2 v1, Vec2 v2 )
{
	r32 result = 0;
    
	r32 divisor = Length(v1) * Length(v2);
    
	if(divisor)
	{
        r32 cosine = Dot(v1, v2) /  divisor;
        result = ArcCos(cosine);
	}
    
    return result;
}

inline r32 AngleBetweenVectors( Vec2 v1, Vec2 v2 )
{
    r32 result = RadiantsBetweenVectors( v1, v2 );
    result = RadToDeg( result );
    return result;
    
}

r32 RadiantsBetweenVectors( Vec3 v1, Vec3 v2 )
{
    r32 cosine = Dot( v1, v2 ) / ( Length( v1 ) * Length( v2 ) );
    r32 result = ArcCos( cosine );
    
    return result;
}

inline r32 AngleBetweenVectors( Vec3 v1, Vec3 v2 )
{
    r32 result = RadiantsBetweenVectors(v1, v2);
    result = RadToDeg(result);
    return result;
    
}

inline r32 ATan2(r32 y, r32 x)
{
    r32 result = atan2f( y , x );
    result = Mod( result + 2.0f * PI32, 2.0f * PI32 ); 
    return result;
}

inline r32 ACoTan2(r32 y, r32 x)
{
    Assert( y != 0.0f );
    r32 result = x / y;
    return result;
}

inline i32 Wrap( i32 min, i32 value, i32 max, i32* addDeltaTo, i32 deltaToApply = 1 )
{
    if( value < min )
    {
        value = max + value;
        *addDeltaTo = *addDeltaTo - deltaToApply;
    }
    else if( value >= max )
    {
        value = value - max;
        *addDeltaTo = *addDeltaTo + deltaToApply;
    }
    return value;
}

inline i32 Wrap(i32 min, i32 value, i32 maxPlusOne)
{
    i32 ignored;
    i32 result = min;
    result = Wrap( min, value, maxPlusOne, &ignored );
    return result;
}

inline i32 Bounds( i32 minCase, i32 maxCase, i32 min, i32 value, i32 max )
{
    i32 result = value;
    if( value < min )
    {
        result = minCase;
    }
    else if( value >= max )
    {
        result = maxCase;
    }
    
    return result;
}

inline i32 Bounds11( i32 min, i32 value, i32 max )
{
    i32 result = Bounds( -1, 1, min, value, max );
    return result;
}

inline i32 Bounds( i32 min, i32 value, i32 max )
{
    i32 result = Bounds( min, max, min, value, max );
    return result;
}

inline r32 Clamp( r32 min, r32 value, r32 max )
{
    r32 result = value;
    if( result < min )
    {
        result = min;
    }
    else if( result > max )
    {
        result = max;
    }
    return result;
}

inline r32 Clamp11( r32 value )
{
    r32 result = Clamp( -1.0f, value, 1.0f );
    return result;
}

inline r32 Clamp01( r32 value )
{
    r32 result = Clamp( 0.0f, value, 1.0f );
    return result;
}

inline r32 UnilateralToBilateral(r32 value)
{
    r32 result = Clamp01(value);
    
    result = (result - 0.5f) * 2.0f;
    return result;
}

inline r32 BilateralToUnilateral(r32 value)
{
    r32 result = (value / 2.0f) + 0.5f;
    return result;
}

inline r32 Lerp( r32 value1, r32 lerp, r32 value2 )
{
    r32 result = ( 1.0f - lerp ) * value1 + lerp * value2;
    return result;
}

inline r32 BiLerp( r32 a, r32 b, r32 c, r32 d, r32 lerpX, r32 lerpY )
{
    r32 result = Lerp( Lerp( a, lerpX, b ), 
                      lerpY, 
                      Lerp( c, lerpX, d ) );
    return result;
}


inline r32 Clamp01MapToRange(r32 min, r32 t, r32 max)
{
    r32 result = min;
    
    r32 range = max - min;
    if(range != 0.0f)
    {
        result = Clamp01((t - min ) / range);
    }
    
    return result;
}

//
//
//Vec2
//
//

inline Vec2 V2( r32 X, r32 Y ){
    Vec2 result = {};
    result.x = X;
    result.y = Y;
    return result;
}

inline Vec2 V2i( i32 X, i32 Y )
{
    Vec2 result = {};
    result.x = ( r32 ) X;
    result.y = ( r32 ) Y;
    return result;
}

inline Vec2 operator + ( Vec2 v1, Vec2 v2 )
{
    Vec2 result = {};
    result.x = v1.x + v2.x;
    result.y = v1.y + v2.y;
    return result;
}

inline Vec2 operator - ( Vec2 v1, Vec2 v2 )
{
    Vec2 result = {};
    result.x = v1.x - v2.x;
    result.y = v1.y - v2.y;
    return result;
}

inline Vec2 operator - ( Vec2 v )
{
    Vec2 result = {};
    result.x = -v.x;
    result.y = -v.y;
    return result;
}

inline Vec2 operator * ( r32 c, Vec2 v )
{
    Vec2 result = {};
    result.x = v.x * c;
    result.y = v.y * c;
    return result;
}

inline b32 operator == ( Vec2 v1, Vec2 v2 )
{
    b32 result = false;
    if( ( v1.x == v2.x ) && ( v1.y == v2.y ) )
    {
        result = true;
    }
    return result;
}

inline b32 operator != ( Vec2 v1, Vec2 v2 )
{
    b32 result = false;
    if( ( v1.x != v2.x ) || ( v1.y != v2.y ) )
    {
        result = true;
    }
    return result;
}

inline Vec2 operator * ( Vec2 v, r32 c )
{
    Vec2 result = {};
    result.x = v.x * c;
    result.y = v.y * c;
    return result;
}

inline Vec2& Vec2::operator *= ( r32 c )
{
    *this = c * ( *this );
    return *this;
}

inline Vec2& Vec2::operator += ( Vec2 v )
{
    *this = v + ( *this );
    return *this;
}

inline Vec2& Vec2::operator -= ( Vec2 v )
{
    *this = ( *this ) - v;
    return *this;
}

inline Vec2 Normalize( Vec2 v )
{
    r32 length2 = LengthSq( v );
    Vec2 result = v;
    if( length2 != 0 )
    {
        result = v * ( 1.0f / SquareRoot( length2 ) );
    }
    return result;
}

inline Vec2 Perp( Vec2 v )
{
    Vec2 result;
    result.y = v.x; 
    result.x = -v.y;
    return result;
}

inline Vec2 Lerp( Vec2 a, r32 t, Vec2 b )
{
    Vec2 result;
    result.x = ( 1.0f - t ) * a.x + t * b.x;
    result.y = ( 1.0f - t ) * a.y + t * b.y;
    return result;
}

inline Vec2 Hadamart( Vec2 a, Vec2 b )
{
    Vec2 result = V2( a.x * b.x, a.y * b.y );
    return result;
}

//
//
//Vec3
//
//


inline Vec3 V3( r32 X, r32 Y, r32 Z )
{
    Vec3 result = {};
    result.x = X;
    result.y = Y;
    result.z = Z;
    return result;
}

inline Vec3 V3i( i32 X, i32 Y, i32 Z )
{
    Vec3 result = {};
    result.x = ( r32 ) X;
    result.y = ( r32 ) Y;
    result.z = ( r32 ) Z;
    return result;
}

inline Vec3 V3( Vec2 xy, r32 z )
{
    Vec3 result;
    result.xy = xy;
    result.z = z;
    return result;
}

inline Vec3 operator + ( Vec3 v1, Vec3 v2 )
{
    Vec3 result = {};
    result.x = v1.x + v2.x;
    result.y = v1.y + v2.y;
    result.z = v1.z + v2.z;
    return result;
}

inline Vec3 operator - ( Vec3 v1, Vec3 v2 )
{
    Vec3 result = {};
    result.x = v1.x - v2.x;
    result.y = v1.y - v2.y;
    result.z = v1.z - v2.z;
    return result;
}

inline Vec3 operator - ( Vec3 v )
{
    Vec3 result = {};
    result.x = -v.x;
    result.y = -v.y;
    result.z = -v.z;
    return result;
}

inline Vec3 operator * ( r32 c, Vec3 v )
{
    Vec3 result = {};
    result.x = v.x * c;
    result.y = v.y * c;
    result.z = v.z * c;
    return result;
}

inline b32 operator == ( Vec3 v1, Vec3 v2 )
{
    b32 result = false;
    if( ( v1.x == v2.x ) && ( v1.y == v2.y ) && ( v1.z == v2.z ) )
    {
        result = true;
    }
    return result;
}

inline b32 operator != ( Vec3 v1, Vec3 v2 )
{
    b32 result = !( v1 == v2 );
    return result;
}

inline Vec3 operator * ( Vec3 v, r32 c )
{
    Vec3 result = {};
    result.x = v.x * c;
    result.y = v.y * c;
    result.z = v.z * c;
    return result;
}

inline Vec3 operator / ( Vec3 v, r32 c )
{
    Vec3 result = v * ( 1.0f / c );
    return result;
}

inline Vec3& Vec3::operator *= ( r32 c )
{
    *this = c * ( *this );
    return *this;
}

inline Vec3& operator /= ( Vec3& v, r32 c )
{
    v = v / c;
    return v;
}

inline Vec3& Vec3::operator += ( Vec3 v )
{
    *this = v + ( *this );
    return *this;
}

inline Vec3& Vec3::operator -= ( Vec3 v )
{
    *this = (*this) - v;
    return *this;
}

inline Vec3 Normalize( Vec3 v )
{
    r32 length2 = LengthSq( v );
    Vec3 result = v;
    if( length2 != 0 )
    {
        result = v * ( 1.0f / SquareRoot( length2 ) );
    }
    return result;
}

inline Vec3 Lerp( Vec3 a, r32 t, Vec3 b )
{
    Vec3 result;
    result.x = ( 1.0f - t ) * a.x + ( t * b.x );
    result.y = ( 1.0f - t ) * a.y + ( t * b.y );
    result.z = ( 1.0f - t ) * a.z + ( t * b.z );
    return result;
}

inline Vec3 ClampV3(Vec3 min, Vec3 value, Vec3 max)
{
    Vec3 result;
    result.x = Clamp(min.x, value.x, max.x);
    result.y = Clamp(min.y, value.y, max.y);
    result.z = Clamp(min.z, value.z, max.z);
    
    return result;
}

inline Vec3 Hadamart( Vec3 a, Vec3 b )
{
    Vec3 result = V3( a.x * b.x, a.y * b.y, a.z * b.z );
    return result;
}

inline Vec3 ToV3( Vec2 v )
{
    Vec3 result = V3( v, 0.0f );
    return result;
}

inline Vec3 ToV3( r32 x, r32 y )
{
    Vec3 result = V3( x, y, 0.0f );
    return result;
}
//
//
//VEC4
//
//


inline Vec4 V4( r32 X, r32 Y, r32 z, r32 W )
{
    Vec4 result = {};
    result.x = X;
    result.y = Y;
    result.z = z;
    result.w = W;
    return result;
}

inline Vec4 V4i( i32 X, i32 Y, i32 z, i32 W )
{
    Vec4 result = {};
    result.x = (r32 ) X;
    result.y = (r32 ) Y;
    result.z = (r32 ) z;
    result.w = (r32 ) W;
    return result;
}

inline Vec4 V4( Vec3 v, r32 c )
{
    Vec4 result = {};
    result.xyz = v;
    result.w = c;
    return result;
}

inline Vec4 V4( Vec2 v, r32 z, r32 w )
{
    Vec4 result = {};
    result.xy = v;
    result.z = z;
    result.w = w;
    return result;
}

inline Vec4 operator + ( Vec4 v1, Vec4 v2 )
{
    Vec4 result = {};
    result.x = v1.x + v2.x;
    result.y = v1.y + v2.y;
    result.z = v1.z + v2.z;
    result.w = v1.w + v2.w;
    return result;
}

inline Vec4 operator - ( Vec4 v1, Vec4 v2 )
{
    Vec4 result = {};
    result.x = v1.x - v2.x;
    result.y = v1.y - v2.y;
    result.z = v1.z - v2.z;
    result.w = v1.w - v2.w;
    return result;
}

inline Vec4 operator - ( Vec4 v )
{
    Vec4 result = {};
    result.x = -v.x;
    result.y = -v.y;
    result.z = -v.z;
    result.w = -v.w;
    return result;
}

inline Vec4 operator * ( r32 c, Vec4 v )
{
    Vec4 result = {};
    result.x = v.x * c;
    result.y = v.y * c;
    result.z = v.z * c;
    result.w = v.w * c;
    return result;
}

inline Vec4 operator * ( Vec4 v, r32 c )
{
    Vec4 result = {};
    result.x = v.x * c;
    result.y = v.y * c;
    result.z = v.z * c;
    result.w = v.w * c;
    return result;
}

inline Vec4& Vec4::operator *= ( r32 c )
{
    *this = c * ( *this );
    return *this;
}

inline Vec4& Vec4::operator += ( Vec4 v )
{
    *this = v + ( *this );
    return *this;
}

inline Vec4& Vec4::operator -= ( Vec4 v )
{
    *this = (*this) - v;
    return *this;
}

inline Vec4 Normalize( Vec4 v )
{
    r32 length2 = LengthSq( v );
    Vec4 result = v;
    if( length2 != 0 )
    {
        result = v * ( 1.0f / SquareRoot( length2 ) );
    }
    return result;
}

inline Vec4 Clamp01(Vec4 v)
{
    Vec4 result = v;
    result.r = Clamp01(result.r);
    result.g = Clamp01(result.g);
    result.b = Clamp01(result.b);
    result.a = Clamp01(result.a);
    
    return result;
}

inline Vec4 Lerp( Vec4 a, r32 t, Vec4 b )
{
    Vec4 result;
    result.x = ( 1.0f - t ) * a.x + ( t * b.x );
    result.y = ( 1.0f - t ) * a.y + ( t * b.y );
    result.z = ( 1.0f - t ) * a.z + ( t * b.z );
    result.w = ( 1.0f - t ) * a.w + ( t * b.w );
    return result;
}

inline Vec4 BiLerp(Vec4 a, Vec4 b, Vec4 c, Vec4 d, r32 lerpX, r32 lerpY)
{
    Vec4 result = Lerp( Lerp( a, lerpX, b ), 
                       lerpY, 
                       Lerp( c, lerpX, d ) );
    return result;
}

inline Vec4 Hadamart( Vec4 a, Vec4 b )
{
    Vec4 result = V4( a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w );
    return result;
}


inline Vec4 SRGB255ToLinear1( Vec4 v )
{
    r32 inv255 = 1.0f / 255.0f;
    Vec4 result;
    result.a = v.a * inv255;
    result.r = Square( v.r * inv255 ); 
    result.g = Square( v.g * inv255 ); 
    result.b = Square( v.b * inv255 );
    return result;
}

inline Vec4 Linear1ToSRGB255( Vec4 v )
{
    Vec4 result;
    result.a = v.a * 255.0f;
    result.r = SquareRoot( v.r ) * 255.0f;
    result.g = SquareRoot( v.g ) * 255.0f;
    result.b = SquareRoot( v.b ) * 255.0f;
    return result;
}

inline Vec4 SRGBLinearize( Vec4 c )
{
    Vec4 result;
    result.r = Square( c.r );
    result.g = Square( c.g );
    result.b = Square( c.b );
    result.a = c.a;
    
    return result;
}

inline Vec4 SRGBLinearize( r32 r, r32 g, r32 b, r32 a )
{
    Vec4 result = SRGBLinearize( V4( r, g, b, a ) );
    return result;
}

//
//
//Rect2
//
//

inline Rect2 RectMinMax( Vec2 min, Vec2 max )
{
    Rect2 result = {};
    result.min = min;
    result.max = max;
    return result;
}

inline Rect2 RectCenterHalfDim( Vec2 center, Vec2 halfDim )
{
    Rect2 result = {};
    result.min = center - halfDim;
    result.max = center + halfDim;
    return result;
}

inline Rect2 RectCenterDim( Vec2 center, Vec2 dim )
{
    Rect2 result = {};
    result = RectCenterHalfDim( center, 0.5f * dim );
    return result;
}

inline Rect2 RectMinDim( Vec2 min, Vec2 dim )
{
    Rect2 result = {};
    result.min = min;
    result.max = min + dim;
    return result;
}

inline Vec2 GetCenter( Rect2 rect )
{
    Vec2 result = 0.5f * ( rect.min + rect.max );
    return result;
}

inline Vec2 GetDim(Rect2 rect)
{
    Vec2 result = rect.max - rect.min;
    return result;
}

inline Rect2 Offset( Rect2 rect, Vec2 offset )
{
    Rect2 result = rect;
    result.min += offset;
    result.max += offset;
    return result;
}

inline Rect2 AddRadius( Rect2 rect, Vec2 dim )
{
    Rect2 result = rect;
    result.min -= dim;
    result.max += dim;
    return result;
}

inline Rect2 Scale(Rect2 rect, r32 coeff)
{
    Rect2 result = RectCenterDim(GetCenter(rect), coeff * GetDim(rect));
    return result;
}

//NOTE(leonardo):the check needs to be inclusive on the low part and exclusive on the
//high part, cause the activeRect thing rely on that.
inline b32 PointInRect( Rect2 rect, Vec2 p )
{
    b32 result = false;
    if( p.x >= rect.min.x && p.x < rect.max.x &&
       p.y >= rect.min.y && p.y < rect.max.y )
    {
        result = true;
    }
    return result;
}

inline b32 PointInUnalignedRect( Vec2 min, Vec2 XAxis, Vec2 YAxis, Vec2 point )
{
    b32 result = false;
    Vec2 startingPos = point - min;
    
    r32 edge0 = Dot( startingPos, -YAxis ); 
    r32 edge1 = Dot( startingPos - XAxis, XAxis ); 
    r32 edge2 = Dot( startingPos - XAxis - YAxis, YAxis ); 
    r32 edge3 = Dot( startingPos - YAxis, -XAxis ); 
    
    result = ( edge0 < 0 && edge1 < 0 && edge2 < 0 && edge3 < 0 );
    return result;
    
}

inline Rect2 Intersect( Rect2 a, Rect2 b )
{
    Rect2 result = a;
    if(a.min.x < b.min.x)
    { 
        result.min.x = b.min.x;
    }
    if(a.min.y < b.min.y)
    {
        result.min.y = b.min.y;
    }
    if(a.max.x > b.max.x ) 
    {
        result.max.x = b.max.x;
    }
    if(a.max.y > b.max.y)
    {
        result.max.y = b.max.y;
    }
    return result;
}

inline Rect2 Union( Rect2 a, Rect2 b )
{
    Rect2 result = a;
    if(a.min.x > b.min.x)
    { 
        result.min.x = b.min.x;
    }
    if(a.min.y > b.min.y)
    {
        result.min.y = b.min.y;
    }
    if(a.max.x < b.max.x) 
    {
        result.max.x = b.max.x;
    }
    if(a.max.y < b.max.y)
    {
        result.max.y = b.max.y;
    }
    return result;
}

inline Rect2 Union(Rect2 rect, Vec2 P)
{
    Rect2 result = rect;
    result.min.x = Min(result.min.x, P.x);
    result.min.y = Min(result.min.y, P.y);
    
    result.max.x = Max(result.max.x, P.x);
    result.max.y = Max(result.max.y, P.y);
    
    return result;
}

inline r32 GetClampedArea( Rect2 a )
{
    r32 result = 0;
    r32 width = a.max.x - a.min.x;
    r32 height = a.max.y - a.min.y;
    
    if( width > 0 && height > 0 )
    {
        result = width * height;
    }
    return result;
}

inline b32 HasArea(Rect2 rect)
{
    b32 result = ( rect.min.x < rect.max.x ) && ( rect.min.y < rect.max.y );
    return result;
}

inline b32 HasArea(Rect3 rect)
{
    b32 result = (rect.min.x < rect.max.x) && (rect.min.y < rect.max.y) && (rect.min.z < rect.max.z);
    return result;
}

inline Rect2 InvertedInfinityRect2()
{
    Rect2 result;
    result.min.x = result.min.y = R32_MAX;
    result.max.x = result.max.y = R32_MIN;
    return result;
}

inline Rect3 InvertedInfinityRect3()
{
    Rect3 result;
    result.min.x = result.min.y = result.min.z = R32_MAX;
    result.max.x = result.max.y = result.max.z = R32_MIN;
    return result;
}



//
//
//Rect3
//
//


inline Rect3 RectMinMax( Vec3 min, Vec3 max )
{
    Rect3 result = {};
    result.min = min;
    result.max = max;
    return result;
}

inline Rect3 RectCenterHalfDim( Vec3 center, Vec3 halfDim )
{
    Rect3 result = {};
    result.min = center - halfDim;
    result.max = center + halfDim;
    return result;
}

inline Rect3 RectCenterDim( Vec3 center, Vec3 dim )
{
    Rect3 result = {};
    result = RectCenterHalfDim( center, 0.5f * dim );
    return result;
}

inline Rect3 RectMinDim( Vec3 min, Vec3 dim )
{
    Rect3 result = {};
    result.min = min;
    result.max = min + dim;
    return result;
}

inline Vec3 GetCenter( Rect3 rect )
{
    Vec3 result = 0.5f * ( rect.min + rect.max );
    return result;
}

inline Vec3 GetDim( Rect3 rect )
{
    Vec3 result = rect.max - rect.min;
    return result;
}

inline Rect3 Offset( Rect3 rect, Vec3 offset )
{
    Rect3 result = rect;
    result.min += offset;
    result.max += offset;
    return result;
}


inline Rect3 Intersect( Rect3 a, Rect3 b )
{
    Rect3 result = a;
    if(a.min.x < b.min.x)
    { 
        result.min.x = b.min.x;
    }
    if(a.min.y < b.min.y)
    {
        result.min.y = b.min.y;
    }
    if(a.min.z < b.min.z)
    {
        result.min.z = b.min.z;
    }
    if(a.max.x > b.max.x ) 
    {
        result.max.x = b.max.x;
    }
    if(a.max.y > b.max.y)
    {
        result.max.y = b.max.y;
    }
    if(a.max.z > b.max.z)
    {
        result.max.z = b.max.z;
    }
    
    return result;
}

inline Rect3 Union( Rect3 a, Rect3 b )
{
    Rect3 result = a;
    if(a.min.x > b.min.x)
    { 
        result.min.x = b.min.x;
    }
    if(a.min.y > b.min.y)
    {
        result.min.y = b.min.y;
    }
    if(a.min.z > b.min.z)
    {
        result.min.z = b.min.z;
    }
    if(a.max.x < b.max.x) 
    {
        result.max.x = b.max.x;
    }
    if(a.max.y < b.max.y)
    {
        result.max.y = b.max.y;
    }
    if(a.max.z < b.max.z)
    {
        result.max.z = b.max.z;
    }
    return result;
}

inline Rect3 AddRadius(Rect3 rect, Vec3 dim)
{
    Rect3 result = rect;
    result.min -= dim;
    result.max += dim;
    return result;
}

inline Rect3 Scale(Rect3 rect, r32 coeff)
{
    Rect3 result = RectCenterDim(GetCenter(rect), coeff * GetDim(rect));
    return result;
}

inline b32 PointInRect(Rect3 rect, Vec3 p)
{
    b32 result = false;
    if( p.x >= rect.min.x && p.x < rect.max.x &&
       p.y >= rect.min.y && p.y < rect.max.y &&
       p.z >= rect.min.z && p.z < rect.max.z )
    {
        result = true;
    }
    return result;
}

inline Vec3 RotateVectorAroundAxis(Vec3 A, Vec3 B, r32 angle)
{
    Vec3 baseParallel = B * Dot(A, B);
    Vec3 basePerp = A - baseParallel;
    Vec3 w = Cross(B, basePerp);
    r32 perpLength = Length(basePerp);
    
    r32 x1 = Cos(angle) / perpLength;
    r32 x2 = Sin(angle) / Length(w);
    
    Vec3 newPerp = perpLength * (x1 * basePerp + x2 * w);
    Vec3 result = baseParallel + newPerp;
    
    return result;
}



//
//
//RECT2i
//
//

struct Rect2i
{
    i32 minX;
    i32 minY;
    i32 maxX;
    i32 maxY;
};

inline Rect2i RectMinMax( i32 minX, i32 minY, i32 maxX, i32 maxY )
{
    Rect2i result;
    result.minX = minX;
    result.minY = minY;
    result.maxX = maxX;
    result.maxY = maxY;
    
    return result;
}

inline u32 GetWidth( Rect2i rect )
{
    u32 result = ( u32 ) ( rect.maxX - rect.minX );
    return result;
}

inline u32 GetHeight( Rect2i rect )
{
    u32 result = ( u32 ) ( rect.maxY - rect.minY );
    return result;
}

inline Rect2i Offset( Rect2i rect, i32 x, i32 y )
{
    Rect2i result = rect;
    result.minX += x;
    result.maxX += x;
    
    result.minY += y;
    result.maxY += y;
    
    return result;
}

inline Rect2i Intersect( Rect2i a, Rect2i b )
{
    Rect2i result = a;
    if(a.minX < b.minX)
    { 
        result.minX = b.minX;
    }
    if(a.minY < b.minY)
    {
        result.minY = b.minY;
    }
    if(a.maxX > b.maxX) 
    {
        result.maxX = b.maxX;
    }
    if(a.maxY > b.maxY)
    {
        result.maxY = b.maxY;
    }
    return result;
}

inline Rect2i Union( Rect2i a, Rect2i b )
{
    Rect2i result = a;
    if(a.minX > b.minX)
    { 
        result.minX = b.minX;
    }
    if(a.minY > b.minY)
    {
        result.minY = b.minY;
    }
    if(a.maxX < b.maxX) 
    {
        result.maxX = b.maxX;
    }
    if(a.maxY < b.maxY)
    {
        result.maxY = b.maxY;
    }
    return result;
}

inline i32 GetClampedArea( Rect2i a )
{
    i32 result = 0;
    i32 width = a.maxX - a.minX;
    i32 height = a.maxY - a.minY;
    
    if( width > 0 && height > 0 )
    {
        result = width * height;
    }
    return result;
}

inline b32 HasArea( Rect2i rect )
{
    b32 result = ( rect.minX < rect.maxX ) && ( rect.minY < rect.maxY );
    return result;
}

inline Rect2i InvertedInfinityRect2i()
{
    Rect2i result;
    result.minX = result.minY = I32_MAX;
    result.maxX = result.maxY = I32_MIN;
    return result;
}

inline Vec2 Arm2(r32 radiants)
{
    Vec2 result = V2(Cos(radiants), Sin(radiants));
    return result;
}

inline r32 AArm2(Vec2 v)
{
    Vec2 norm = Normalize(v);
    r32 result = ATan2(v.y, v.x);
    return result;
}

// NOTE(Leonardo): m4x4
inline m4x4 operator *(m4x4 A, m4x4 B )
{
    m4x4 result = {};
    for( u32 r = 0; r < 4; r++ ) // NOTE(Leonardo): rows of A
    {
        for( u32 c = 0; c < 4; c++ ) // NOTE(Leonardo): columns of B
        {
            r32 sum = 0;
            for( u32 i = 0; i < 4; i++ ) // NOTE(Leonardo): columns of A AND rows of B
            {
                result.E[r][c] += A.E[r][i] * B.E[i][c];
            }
        }
    }
    return result;
}

void M4x4_SSE(float *A, float *B, float *C) {
    __m128 row1 = _mm_load_ps(&B[0]);
    __m128 row2 = _mm_load_ps(&B[4]);
    __m128 row3 = _mm_load_ps(&B[8]);
    __m128 row4 = _mm_load_ps(&B[12]);
    for(int i=0; i<4; i++) {
        __m128 brod1 = _mm_set1_ps(A[4*i + 0]);
        __m128 brod2 = _mm_set1_ps(A[4*i + 1]);
        __m128 brod3 = _mm_set1_ps(A[4*i + 2]);
        __m128 brod4 = _mm_set1_ps(A[4*i + 3]);
        __m128 row = _mm_add_ps(
            _mm_add_ps(
            _mm_mul_ps(brod1, row1),
            _mm_mul_ps(brod2, row2)),
            _mm_add_ps(
            _mm_mul_ps(brod3, row3),
            _mm_mul_ps(brod4, row4)));
        _mm_store_ps(&C[4*i], row);
    }
}

inline Vec4 Transform( m4x4 A, Vec4 p )
{
    Vec4 R;
    R.x = p.x * A.E[0][0] + p.y * A.E[0][1] + p.z * A.E[0][2] + p.w * A.E[0][3];
    R.y = p.x * A.E[1][0] + p.y * A.E[1][1] + p.z * A.E[1][2] + p.w * A.E[1][3];
    R.z = p.x * A.E[2][0] + p.y * A.E[2][1] + p.z * A.E[2][2] + p.w * A.E[2][3];
    R.w = p.x * A.E[3][0] + p.y * A.E[3][1] + p.z * A.E[3][2] + p.w * A.E[3][3];
    
    return R;
}

inline Vec3 operator *(m4x4 A, Vec3 p )
{
    Vec3 result = Transform( A, V4( p, 1.0f ) ).xyz;
    return result;
}

inline Vec4 operator *(m4x4 A, Vec4 p )
{
    Vec4 result = Transform( A, p );
    return result;
}

inline m4x4 Identity( )
{
    m4x4 result = 
    {
        {
            { 1, 0, 0, 0 },
            { 0, 1, 0, 0 },
            { 0, 0, 1, 0 },
            { 0, 0, 0, 1 },
        }
    };
    
    return result;
}

inline m4x4 Transpose( m4x4 A )
{
    m4x4 result;
    for( u32 j = 0; j < 4; ++j )
    {
        for( u32 i = 0; i < 4; ++i )
        {
            result.E[j][i] = A.E[i][j];
        }
    }
    
    return result;
}

inline m4x4 XRotation(r32 a)
{
    r32 c = Cos(a);
    r32 s = Sin(a);
    m4x4 result = 
    {
        {
            { 1, 0, 0, 0 },
            { 0, c, -s, 0 },
            { 0, s, c, 0 },
            { 0, 0, 0, 1 },
        }
    };
    
    return result;
}

inline m4x4 YRotation(r32 a)
{
    r32 c = Cos(a);
    r32 s = Sin(a);
    m4x4 result = 
    {
        {
            { c, 0, s, 0 },
            { 0, 1, 0, 0 },
            { -s, 0, c, 0 },
            { 0, 0, 0, 1 },
        }
    };
    
    return result;
}


inline m4x4 ZRotation( r32 a )
{
    r32 c = Cos( a );
    r32 s = Sin( a );
    m4x4 result = 
    {
        {
            { c, -s, 0, 0 },
            { s, c, 0, 0 },
            { 0, 0, 1, 0 },
            { 0, 0, 0, 1 },
        }
    };
    
    return result;
}

inline m4x4 Translation(Vec3 v)
{
    m4x4 result = 
    {
        {
            {1, 0, 0, v.x},
            {0, 1, 0, v.y},
            {0, 0, 1, v.z},
            {0, 0, 0, 1},
        }
    };
    
    return result;
}

struct m4x4_inv
{
    m4x4 forward;
    m4x4 backward;
};

// NOTE(Leonardo): this produce a matrix that gives open gl the ndc that it wants
inline m4x4_inv PerspectiveProjection( r32 aspectWidthOverHeight, r32 focalLength )
{
    r32 a = 1.0f;
    r32 b = aspectWidthOverHeight;
    r32 c = focalLength;
    
    r32 n = 0.05f; // NOTE(Leonardo): near clip plane
    r32 f = 20000.0f; // NOTE(Leonardo): far clip plane
    
    r32 d = (n + f) / ( n - f);
    r32 e = (2 * f * n ) / (n - f);
    
    m4x4_inv result;
    result.forward = {
        {
            { a*c, 0, 0, 0 },
            { 0, b*c, 0, 0 },
            { 0, 0, d, e },
            { 0, 0, -1, 0 },
        }
    };
    
    result.backward = {
        {
            { 1/(a*c), 0, 0, 0 },
            { 0, 1/(b*c), 0, 0 },
            { 0, 0, 0, -1 },
            { 0, 0, 1/e, d/e },
        }
    };
    
#if FORGIVENESS_SLOW
    m4x4 I = result.backward * result.forward;
#endif
    
    return result;
}

inline m4x4_inv OrthographicProjection( r32 aspectWidthOverHeight )
{
    r32 a = 1.0f;
    r32 b = aspectWidthOverHeight;
    
    r32 n = -100.0f; // NOTE(Leonardo): near clip plane
    r32 f = 100.0f; // NOTE(Leonardo): far clip plane
    
    r32 d = 2.0f / ( n - f );
    r32 e = ( n + f ) / ( n - f );
    
    m4x4_inv result;
    result.forward = {
        {
            { a, 0, 0, 0 },
            { 0, b, 0, 0 },
            { 0, 0, d, e },
            { 0, 0, 0, 1 },
        }
    };
    
    result.backward = {
        {
            { 1/a, 0, 0, 0 },
            { 0, 1/b, 0, 0 },
            { 0, 0, 1/d, -e/d },
            { 0, 0, 0, 1 },
        }
    };
    
#if FORGIVENESS_SLOW
    m4x4 I = result.backward * result.forward;
#endif
    
    return result;
}

internal m4x4 Columns3x3( Vec3 X, Vec3 Y, Vec3 Z )
{
    m4x4 result = 
    {
        {
            { X.x, Y.x, Z.x },
            { X.y, Y.y, Z.y },
            { X.z, Y.z, Z.z },
            { 0, 0, 0, 1 }
        }
    };
    
    return result;
}

internal m4x4 Rows3x3( Vec3 X, Vec3 Y, Vec3 Z )
{
    m4x4 result = 
    {
        {
            { X.x, X.y, X.z },
            { Y.x, Y.y, Y.z },
            { Z.x, Z.y, Z.z },
            { 0, 0, 0, 1 }
        }
    };
    
    return result;
}

internal m4x4 Translate( m4x4 M, Vec3 T )
{
    m4x4 R = M;
    R.E[0][3] += T.x;
    R.E[1][3] += T.y;
    R.E[2][3] += T.z;
    
    return R;
}

inline Vec3 GetColumn( m4x4 A, u32 column )
{
    Vec3 result = { A.E[0][column], A.E[1][column], A.E[2][column] };
    return result;
}

inline Vec3 GetRow( m4x4 A, u32 row )
{
    Vec3 result = { A.E[row][0], A.E[row][1], A.E[row][2] };
    return result;
}

internal m4x4_inv CameraTransformMatrix( Vec3 x, Vec3 y, Vec3 z, Vec3 p )
{
    m4x4_inv result;
    m4x4 A = Rows3x3(x, y, z);
    
    Vec3 AP = -(A * p);
    A = Translate(A, AP);
    result.forward = A;
    
    Vec3 iX = x / LengthSq(x);
    Vec3 iY = y / LengthSq(y);
    Vec3 iZ = z / LengthSq(z);
    m4x4 B = Columns3x3(iX, iY, iZ);
    
    Vec3 iP = { 
        AP.x * iX.x + AP.y * iY.x + AP.z * iZ.x,
        AP.x * iX.y + AP.y * iY.y + AP.z * iZ.y,
        AP.x * iX.z + AP.y * iY.z + AP.z * iZ.z };
    
    B = Translate( B, -iP );
    
    result.backward = B;
    
#if FORGIVENESS_SLOW
    m4x4 I = result.backward * result.forward;
#endif
    return result;
}

internal Vec2 RayIntersection( Vec2 Pa, Vec2 Ra, Vec2 Pb, Vec2 Rb )
{
    Vec2 result = {};
    r32 d = ( Rb.x * Ra.y - Rb.y * Ra.x );
    if( d != 0.0f )
    {
        r32 Ta = ( ( Pb.y - Pa.y ) * Rb.x + ( Pa.x - Pb.x ) * Rb.y ) / d; 
        r32 Tb = ( ( Pa.x - Pb.x ) * Ra.y + ( Pb.y - Pa.y ) * Ra.x ) / d;
        result = V2( Ta, Tb );
    }
    
    return result;
}

inline Vec3 RayPlaneIntersection(Vec3 rayOrigin, Vec3 rayDirection, Vec3 planeOrigin, Vec3 planeNormal)
{
    Vec3 result = {};
    r32 dot = Dot(rayDirection, planeNormal);
    if(dot != 0)
    {
        r32 t = Dot(planeOrigin - rayOrigin, planeNormal) / dot;
        result = rayOrigin + t * rayDirection;
    }
    
    return result;
}

inline Vec4 BGRAUnpack4x8( u32 value )
{
    Vec4 result = { ( r32 ) ( ( value >> 16 ) & 0xff ),
        ( r32 ) ( ( value >> 8 ) & 0xff ),
        ( r32 ) ( ( value >> 0 ) & 0xff ),
        ( r32 ) ( ( value >> 24 ) & 0xff ) };
    return result;
}

inline u32 BGRAPack8x4( Vec4 value )
{
    u32 result = ( ( u32 ) value.a << 24 | 
                  ( u32 ) value.r << 16 |
                  ( u32 ) value.g << 8 |
                  ( u32 ) value.b << 0 );
    return result;
}

inline Vec4 RGBAUnpack4x8( u32 value )
{
    Vec4 result = { ( r32 ) ( ( value >> 0 ) & 0xff ),
        ( r32 ) ( ( value >> 8 ) & 0xff ),
        ( r32 ) ( ( value >> 16 ) & 0xff ),
        ( r32 ) ( ( value >> 24 ) & 0xff ) };
    return result;
}

inline u32 RGBAPack8x4( Vec4 value )
{
    u32 result = ( ( u32 ) value.a << 24 | 
                  ( u32 ) value.b << 16 |
                  ( u32 ) value.g << 8 |
                  ( u32 ) value.r << 0 );
    return result;
}

inline u32 StoreColor( Vec4 color )
{
    u32 result = RGBAPack8x4( color * 255.0f );
    return result;
}

struct DiceThrow
{
    i32 faces;
    u32 throwCount;
    i32 offset;
};

struct RangeDistribution
{
    r32 min;
    r32 max;
    
    r32 normalizationCoeff;
    DiceThrow distr;
};

struct BucketDistribution
{
    u8 buckets[100];
};

inline DiceThrow Dice( i32 diceFaces, i32 diceThrows, i32 offset )
{
    DiceThrow result = {};
    Assert(diceFaces > 0);
    Assert(diceThrows > 0);
    
    result.faces = diceFaces + 1;
    result.throwCount = diceThrows;
    result.offset = offset;
    
    return result;
}

inline RangeDistribution RangeDistr( r32 min, r32 max, i8 diceFaces, i8 diceThrows, i8 offset = 0 )
{
    RangeDistribution result = {};
    
    result.distr = Dice( diceFaces, diceThrows, offset );
    result.min = min;
    result.max = max;
    
    r32 maxValue = ( r32 ) ( diceFaces * diceThrows + offset );
    result.normalizationCoeff = 1.0f / maxValue;
    
    return result;
}

inline RangeDistribution RangeDistr( r32 value )
{
    RangeDistribution result = {};
    
    result.min = value;
    result.max = value;
    
    return result;
}

struct Bucket
{
    u8 value;
    u8 count;
};

struct InitializerBuckets
{
    Bucket buckets[16];
};

inline BucketDistribution BucketDistr( InitializerBuckets initializer )
{
    BucketDistribution result = {};
    
    u32 bucketCount = 0;
    for( u32 bucketIndex = 0; bucketIndex < ArrayCount( initializer.buckets ); ++bucketIndex )
    {
        Bucket* bucket = initializer.buckets + bucketIndex;
        Assert( bucketCount + bucket->count <= 100 );
        u8 value = bucket->value;
        for( u32 valueIndex = 0; valueIndex < bucket->count; ++valueIndex )
        {
            result.buckets[bucketCount++] = value;
        }
    }
    
    Assert( bucketCount == 100 );
    
    return result;
};
