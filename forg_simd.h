#pragma once
#define MMSetExpr(expr) _mm_set_ps(expr, expr, expr, expr)

#define mmSquare(a) _mm_mul_ps(a, a)    
#define M(a, i) (((float *)&(a))[i])
#define Mi(a, i) (((u32 *)&(a))[i])

inline __m128 operator*(__m128 A, __m128 B)
{
    __m128 result = _mm_mul_ps(A, B);
    return result;
}

inline __m128 operator+(__m128 A, __m128 B)
{
    __m128 result = _mm_add_ps(A, B);
    return result;
}

inline __m128 operator-(__m128 A, __m128 B)
{
    __m128 result = _mm_sub_ps(A, B);
    return result;
}

inline __m128& operator+=(__m128& A, __m128 B)
{
    A = _mm_add_ps(A, B);
    return A;
}

inline __m128& operator-=(__m128& A, __m128 B)
{
    A = _mm_sub_ps(A, B);
    return A;
}

inline __m128& operator*=(__m128& A, __m128 B)
{
    A = _mm_mul_ps(A, B);
    return A;
}




struct V3_4x
{
    __m128 x;
    __m128 y;
    __m128 z;
};

struct V4_4x
{
    __m128 r;
    __m128 g;
    __m128 b;
    __m128 a;
};


inline V3_4x operator+(V3_4x A, V3_4x B)
{
    V3_4x result;
    result.x = _mm_add_ps( A.x, B.x );
    result.y = _mm_add_ps( A.y, B.y );
    result.z = _mm_add_ps( A.z, B.z );
    
    return result;
}

inline V3_4x& operator+=(V3_4x& A, V3_4x B)
{
    A.x = _mm_add_ps( A.x, B.x );
    A.y = _mm_add_ps( A.y, B.y );
    A.z = _mm_add_ps( A.z, B.z );
    
    return A;
}

inline V3_4x& operator-=( V3_4x& A, V3_4x B )
{
    A.x = _mm_sub_ps( A.x, B.x );
    A.y = _mm_sub_ps( A.y, B.y );
    A.z = _mm_sub_ps( A.z, B.z );
    
    return A;
}

inline V3_4x operator*(r32 As, V3_4x B)
{
    V3_4x result;
    __m128 A = _mm_set1_ps( As );
    result.x = _mm_mul_ps( A, B.x );
    result.y = _mm_mul_ps( A, B.y );
    result.z = _mm_mul_ps( A, B.z );
    
    return result;
}

inline V3_4x operator*(__m128 As, V3_4x B)
{
    V3_4x result;
    __m128 A = As;
    result.x = _mm_mul_ps( A, B.x );
    result.y = _mm_mul_ps( A, B.y );
    result.z = _mm_mul_ps( A, B.z );
    
    return result;
}

inline V4_4x operator+(V4_4x A, V4_4x B)
{
    V4_4x result;
    result.r = _mm_add_ps( A.r, B.r );
    result.g = _mm_add_ps( A.g, B.g );
    result.b = _mm_add_ps( A.b, B.b );
    result.a = _mm_add_ps( A.a, B.a );
    
    return result;
}

inline V4_4x& operator+=(V4_4x& A, V4_4x B)
{
    A.r = _mm_add_ps( A.r, B.r );
    A.g = _mm_add_ps( A.g, B.g );
    A.b = _mm_add_ps( A.b, B.b );
    A.a = _mm_add_ps( A.a, B.a );
    
    return A;
}

inline V4_4x operator*(r32 As, V4_4x B)
{
    V4_4x result;
    __m128 A = _mm_set1_ps( As );
    result.r = _mm_mul_ps( A, B.r );
    result.g = _mm_mul_ps( A, B.g );
    result.b = _mm_mul_ps( A, B.b );
    result.a = _mm_mul_ps( A, B.a );
    
    return result;
}

inline V4_4x Clamp01( V4_4x A )
{
    V4_4x result;
    
    __m128 zero4x = _mm_set1_ps( 0 );
    __m128 one4x = _mm_set1_ps( 1 );
    
    result.r = _mm_max_ps( _mm_min_ps( A.r, one4x ), zero4x );
    result.g = _mm_max_ps( _mm_min_ps( A.g, one4x ), zero4x );
    result.b = _mm_max_ps( _mm_min_ps( A.b, one4x ), zero4x );
    result.a = _mm_max_ps( _mm_min_ps( A.a, one4x ), zero4x );
    
    return result;
}

inline V3_4x ToV3_4x(Vec3 A)
{
    V3_4x result;
    result.x = _mm_set1_ps( A.x );
    result.y = _mm_set1_ps( A.y );
    result.z = _mm_set1_ps( A.z );
    
    return result;
}


inline V4_4x ToV4_4x(Vec4 A)
{
    V4_4x result;
    result.r = _mm_set1_ps(A.r);
    result.g = _mm_set1_ps(A.g);
    result.b = _mm_set1_ps(A.b);
    result.a = _mm_set1_ps(A.a);
    
    return result;
}
