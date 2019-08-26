struct BilinearSample
{
    i32 a;
    i32 b;
    i32 c;
    i32 d;
};

inline BilinearSample BiSample( Bitmap* texture, i32 X, i32 Y )
{
    BilinearSample result;
    
    i32* texelPtr =( i32* ) texture->pixels + ( Y * texture->width + X );
    
    result.a = *( texelPtr );
    result.b = *( texelPtr + 1 );
    result.c = *( texelPtr + texture->width );
    result.d = *( texelPtr + texture->width + 1 );
    return result;
}

inline Vec4 BiBlend( BilinearSample texelSample, r32 lerpX, r32 lerpY )
{
    Vec4 texel0 = BGRAUnpack4x8( texelSample.a );
    Vec4 texel1 = BGRAUnpack4x8( texelSample.b );
    Vec4 texel2 = BGRAUnpack4x8( texelSample.c );
    Vec4 texel3 = BGRAUnpack4x8( texelSample.d );
    
    texel0 = SRGB255ToLinear1( texel0 );
    texel1 = SRGB255ToLinear1( texel1 );
    texel2 = SRGB255ToLinear1( texel2 );
    texel3 = SRGB255ToLinear1( texel3 );
    
    Vec4 result = Lerp( Lerp( texel0, lerpX, texel1 ), 
                       lerpY, 
                       Lerp( texel2, lerpX, texel3 ) );
    return result;
}

inline Vec4 UnscaleAndBiasNormal( Vec4 normal )
{
    r32 inv255 = 1.0f / 255.0f;
    Vec4 result;
    result.x = -1.0f + 2.0f * ( normal.x * inv255 );
    result.y = -1.0f + 2.0f * ( normal.y * inv255 );
    result.z = -1.0f + 2.0f * ( normal.z * inv255 );
    result.w = normal.w * inv255;
    return result;
}


#if 0
#include <iacaMarks.h>
#else
#define IACA_VC64_START
#define IACA_VC64_END
#endif

#define mmSquare(a) _mm_mul_ps(a, a)    
#define M(a, i) (((float *)&(a))[i])
#define Mi(a, i) (((u32 *)&(a))[i])

internal void DrawRectangle( Bitmap* buffer, Vec2 min, Vec2 max, Vec4 color, Rect2i clip )
{
    Rect2i fillRect = { RoundReal32ToI32( min.x ), RoundReal32ToI32( min.y ), RoundReal32ToI32( max.x ), RoundReal32ToI32( max.y ) };
    fillRect = Intersect( fillRect, clip );
#if 0
    i32 red = RoundReal32ToUi32( color.r * 255.0f );
    i32 green = RoundReal32ToUi32( color.g * 255.0f );
    i32 blue = RoundReal32ToUi32( color.b * 255.0f );
    i32 alpha = RoundReal32ToUi32( color.a * 255.0f );
    
    u32 color = ( alpha << 24 | red << 16 | green << 8 | blue << 0 );
    u32* row = ( u32* ) ( ( u32* ) buffer->pixels + fillRect.minY * buffer->width + fillRect.minX );
    
    for( i32 Y = fillRect.minY; Y < fillRect.maxY; ++Y )
    {
        u32* pixels = row;
        for( i32 X = fillRect.minX; X < fillRect.maxX; X++ )
        {
            *pixels++ = color;
        }
        row += buffer->width;
    }
#else
    color.rgb *= color.a;
    
    if( HasArea( fillRect ) )
    {
        __m128i startClipMask = _mm_set1_epi8(-1);
        __m128i endClipMask = _mm_set1_epi8(-1);
        
        __m128i startClipMasks[] = {
            _mm_slli_si128( startClipMask, 0 * 4 ),
            _mm_slli_si128( startClipMask, 1 * 4 ),
            _mm_slli_si128( startClipMask, 2 * 4 ),
            _mm_slli_si128( startClipMask, 3 * 4 ) };
        
        __m128i endClipMasks[] = {
            _mm_srli_si128( endClipMask, 0 * 4 ),
            _mm_srli_si128( endClipMask, 3 * 4 ),
            _mm_srli_si128( endClipMask, 2 * 4 ),
            _mm_srli_si128( endClipMask, 1 * 4 ) };
        
        if( fillRect.minX & 3 )
        {
            fillRect.minX = fillRect.minX & ~3;
            startClipMask = startClipMasks[fillRect.minX & 3];
        }
        
        if( fillRect.maxX & 3 )
        {
            fillRect.maxX = ( fillRect.maxX & ~3 ) + 4;
            endClipMask = endClipMasks[fillRect.maxX & 3];
        }
        
        r32 InVec255 = 1.0f / 255.0f;
        __m128 InVec255_4x = _mm_set1_ps(InVec255);
        r32 One255 = 255.0f;
        __m128 One = _mm_set1_ps(1.0f);
        __m128 One255_4x = _mm_set1_ps(255.0f);
        __m128 Zero = _mm_set1_ps(0.0f);
        __m128 half = _mm_set1_ps( 0.5f );
        __m128i MaskFF = _mm_set1_epi32(0xFF);
        __m128 Colorr_4x = _mm_set1_ps(color.r);
        __m128 Colorg_4x = _mm_set1_ps(color.g);
        __m128 Colorb_4x = _mm_set1_ps(color.b);
        __m128 Colora_4x = _mm_set1_ps(color.a);
        
        i32 maxX = fillRect.maxX;
        i32 minX = fillRect.minX;
        i32 minY = fillRect.minY;
        i32 maxY = fillRect.maxY;
        u8 *Row = ((u8 *)buffer->pixels +
                   minX*4 +
                   minY*buffer->width * 4);
        //TIMED_BLOCK(FillRectangle, GetClampedArea( fillRect ) / 2);
        for(int Y = minY;
            Y < maxY;
            ++Y)
        {
            __m128i ClipMask = startClipMask;
            __m128 PixelPy = _mm_set1_ps((r32)Y);
            
            u32 *Pixel = (u32 *)Row;
            for(int XI = minX;
                XI < maxX;
                XI += 4)
            {            
                __m128i WriteMask = ClipMask;
                // TODO(leonardo): Later, re-check if this helps
                //            if(_mm_movemask_epi8(WriteMask))
                {
                    __m128i originalDest = _mm_load_si128((__m128i *)Pixel);
                    
                    // NOTE(leonardo): Load destination
                    __m128 Destb = _mm_cvtepi32_ps(_mm_and_si128(originalDest, MaskFF));
                    __m128 Destg = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(originalDest, 8), MaskFF));
                    __m128 Destr = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(originalDest, 16), MaskFF));
                    __m128 Desta = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(originalDest, 24), MaskFF));
                    
                    __m128 Texelr = Colorr_4x;
                    __m128 Texelg = Colorg_4x;
                    __m128 Texelb = Colorb_4x;
                    __m128 Texela = Colora_4x;
                    
                    Texelr = _mm_min_ps(_mm_max_ps(Texelr, Zero), One);
                    Texelg = _mm_min_ps(_mm_max_ps(Texelg, Zero), One);
                    Texelb = _mm_min_ps(_mm_max_ps(Texelb, Zero), One);
                    
                    // NOTE(leonardo): Go from sRGB to "linear" brightness space
                    Destr = mmSquare(_mm_mul_ps(InVec255_4x, Destr));
                    Destg = mmSquare(_mm_mul_ps(InVec255_4x, Destg));
                    Destb = mmSquare(_mm_mul_ps(InVec255_4x, Destb));
                    Desta = _mm_mul_ps(InVec255_4x, Desta);
                    
                    // NOTE(leonardo): Destination blend
                    __m128 InvTexelA = _mm_sub_ps(One, Texela);
                    __m128 Blendedr = _mm_add_ps(_mm_mul_ps(InvTexelA, Destr), Texelr);
                    __m128 Blendedg = _mm_add_ps(_mm_mul_ps(InvTexelA, Destg), Texelg);
                    __m128 Blendedb = _mm_add_ps(_mm_mul_ps(InvTexelA, Destb), Texelb);
                    __m128 Blendeda = _mm_add_ps(_mm_mul_ps(InvTexelA, Desta), Texela);
                    
                    // NOTE(leonardo): Go from "linear" 0-1 brightness space to sRGB 0-255
                    Blendedr = _mm_mul_ps(One255_4x, _mm_sqrt_ps(Blendedr));
                    Blendedg = _mm_mul_ps(One255_4x, _mm_sqrt_ps(Blendedg));
                    Blendedb = _mm_mul_ps(One255_4x, _mm_sqrt_ps(Blendedb));
                    Blendeda = _mm_mul_ps(One255_4x, Blendeda);
                    
                    __m128i Intr = _mm_cvtps_epi32(Blendedr);
                    __m128i Intg = _mm_cvtps_epi32(Blendedg);
                    __m128i Intb = _mm_cvtps_epi32(Blendedb);
                    __m128i Inta = _mm_cvtps_epi32(Blendeda);
                    
                    __m128i Sr = _mm_slli_epi32(Intr, 16);
                    __m128i Sg = _mm_slli_epi32(Intg, 8);
                    __m128i Sb = Intb;
                    __m128i Sa = _mm_slli_epi32(Inta, 24);
                    
                    __m128i Out = _mm_or_si128(_mm_or_si128(Sr, Sg), _mm_or_si128(Sb, Sa));
                    
#if 1
                    __m128i MaskedOut = _mm_or_si128(_mm_and_si128(WriteMask, Out),
                                                     _mm_andnot_si128(WriteMask, originalDest));
                    _mm_store_si128((__m128i *)Pixel, MaskedOut);
#else
                    _mm_maskmoveu_si128(Out, WriteMask, (char *)Pixel);
#endif
                    IACA_VC64_END;
                }
                
                Pixel += 4;
                ClipMask = _mm_set1_epi8(-1);
                
                if( ( XI + 8 ) >= maxX )
                {
                    ClipMask = endClipMask;
                }
            }
            
            Row += ( buffer->width * 4 );
            
        }
    }
#endif
}



#if 0
internal void DrawRectangleSlowly( Bitmap* buffer, Bitmap* texture, 
                                  Vec2 origin, Vec2 XAxis, Vec2 YAxis, Vec4 color,
                                  Bitmap* normalMap,
                                  r32 meterForPixel )
{
    r32 XAxisLength = Length( XAxis );
    r32 YAxisLength = Length( YAxis );
    Vec2 NXAxis = ( YAxisLength / XAxisLength ) * XAxis;
    Vec2 NYAxis = ( XAxisLength / YAxisLength ) * YAxis;
    
    r32 NzScale = ( XAxisLength + YAxisLength ) * 0.5f;
    
    // NOTE(leonardo): premultiplied alpha also on the modulation color!
    color.rgb *= color.a;
    
    i32 maxWidth = texture->width - 2;
    i32 maxHeight = texture->height - 2;
    
    r32 invMaxWidth = 1.0f / ( buffer->width - 1 );
    r32 invMaxHeight = 1.0f / ( buffer->height - 1 );
    
    r32 originY = ( origin + 0.5f * XAxis + 0.5f * YAxis ).y;
    r32 originZ = 0.0f;
    r32 fixedCastY = invMaxHeight * originY ;
    
    i32 maxX = 0;
    i32 maxY = 0;
    i32 minX = buffer->width;
    i32 minY = buffer->height;
    
    Vec2 P[4] = { origin, origin + XAxis, origin + XAxis + YAxis, origin + YAxis };
    
    for( i32 index = 0; index < ArrayCount( P ); index++ )
    {
        Vec2 p = P[index];
        
        i32 floorX = Floor( p.x ); 
        i32 ceilX = Ceil( p.x );
        
        i32 floorY = Floor( p.y ); 
        i32 ceilY = Ceil( p.y );
        
        if( floorX < minX )
        {
            minX = floorX;
        }
        if( ceilX > maxX )
        {
            maxX = ceilX;
        }
        
        if( floorY < minY )
        {
            minY = floorY;
        }
        if( ceilY > maxY )
        {
            maxY = ceilY;
        }
    }
    
    if( minX < 0 )
    {
        minX = 0;
    }
    
    if( minY < 0 )
    {
        minY = 0;
    }
    
    if( maxX > buffer->width - 1 )
    {
        maxX = buffer->width - 1;
    }
    
    if( maxY > buffer->height - 1 )
    {
        maxY = buffer->height - 1;
    }
    
    u32* row = ( u32* ) buffer->pixels + ( buffer->width * minY ) + minX; 
    for( i32 Y = minY; Y <= maxY; Y++ )
    {
        u32* pixel = row;
        for( i32 X = minX; X <= maxX; X++ )
        {
            Vec2 screenSpaceUV = V2( ( r32 ) X * invMaxWidth, ( r32 ) fixedCastY );
            r32 ZDiff = meterForPixel * ( ( r32 ) Y - originY ); 
            
            Vec2 pixelP = V2i( X, Y );
            Vec2 d = pixelP - origin;
            
            r32 edge0 = Dot( d, -YAxis ); 
            r32 edge1 = Dot( d - XAxis, XAxis ); 
            r32 edge2 = Dot( d - XAxis - YAxis, YAxis ); 
            r32 edge3 = Dot( d - YAxis, -XAxis ); 
            
            if( edge0 < 0 &&
               edge1 < 0 && 
               edge2 < 0 && 
               edge3 < 0 )
            {
                r32 u = Dot( d, XAxis ) / LengthSq( XAxis );
                r32 v = Dot( d, YAxis ) / LengthSq( YAxis );
                
                Assert( u >= 0.0f && u <= 1.0f );
                Assert( v >= 0.0f && v <= 1.0f );
                
                r32 tX = 1.0f + u * ( maxWidth );
                r32 tY = 1.0f + v * ( maxHeight );
                
                i32 intX = ( i32 ) tX;
                i32 intY = ( i32 ) tY;
                
                Assert( intX >= 0 && intX < texture->width );
                Assert( intY >= 0 && intY < texture->height );
                
                r32 lerpX = tX - intX;
                r32 lerpY = tY - intY;
                
                BilinearSample texelSample = BiSample( texture, intX, intY );
                Vec4 texel = BiBlend( texelSample, lerpX, lerpY );
                
                if( normalMap )
                {
                    BilinearSample normalSample = BiSample( normalMap, intX, intY );
                    Vec4 normal0 = BGRAUnpack4x8( normalSample.a );
                    Vec4 normal1 = BGRAUnpack4x8( normalSample.b );
                    Vec4 normal2 = BGRAUnpack4x8( normalSample.c );
                    Vec4 normal3 = BGRAUnpack4x8( normalSample.d );
                    
                    Vec4 normal = Lerp( Lerp( normal0, lerpX, normal1 ), 
                                       lerpY, 
                                       Lerp( normal2, lerpX, normal3 ) );
                    
                    normal = UnscaleAndBiasNormal( normal );
                    
                    normal.xy = normal.x * NXAxis + normal.y * NYAxis; 
                    
                    //NOTE(leonardo): NzScale could become a parameter specified by the rendered user!
                    normal.z *= NzScale;
                    
                    normal.xyz = Normalize( normal.xyz );
                    
                    // NOTE(leonardo): optimized version of calculation the _opposite + 2 times the dot with the normal_ vector
                    Vec3 bounceDirection = 2.0f * normal.z * normal.xyz;
                    bounceDirection.z -= 1.0f;
                    
                    
                    /*NOTE(leonardo): this is a bit complicated to explain: remember that out bitmaps are like cards standing vertically in a 3d world:
                    our bitmap is encoded to have the minor Y in the bottom and the major Y in the top of the bitmap.
                    But if we don't negate the bounceDirection.z, then when the bounceDirection.z is negative we are picking the bottom pixels of the map, while whe would like to take what's _behind_ the card: but those pixels are _more positive_
                    
                    */
                    bounceDirection.z = -bounceDirection.z;
                    
                    r32 pZ = originZ + ZDiff;
                    
                    r32 envMapZ = bounceDirection.y;
                    r32 sampleMapRatio = 0.0f;
                    EnvironmentMap* sampleMap = 0;
                    if( envMapZ < -0.5f )
                    {
                        sampleMap = bottom;
                        sampleMapRatio = 1.0f - ( envMapZ + 1.0f ) / 0.5f;
                    }
                    else if( envMapZ > 0.5f )
                    {
                        sampleMap = top;
                        sampleMapRatio = ( envMapZ - 0.5f ) / 0.5f ;
                    }
                    
                    sampleMapRatio *= sampleMapRatio;
                    
#if 0
                    //TODO(leonardo):sample from middle map!
                    Vec3 lightColor = V3( 0, 0, 0 ); 
                    if( sampleMap )
                    {
                        r32 distanceFromMapInZ = sampleMap->distanceZ - pZ;
                        Vec3 farMapColor = SampleEnvMap( sampleMap, screenSpaceUV, bounceDirection, normal.w, distanceFromMapInZ );
                        lightColor = Lerp( lightColor, sampleMapRatio, farMapColor );
                    }
                    
                    texel.rgb = texel.rgb + texel.a * lightColor;
#endif
                    
                }
                
                texel.r = Clamp01( texel.r );
                texel.g = Clamp01( texel.g );
                texel.b = Clamp01( texel.b );
                
                Vec4 dest = V4( ( r32 ) ( ( *pixel >> 16 ) & 0xff ),
                               ( r32 ) ( ( *pixel >> 8 ) & 0xff ),
                               ( r32 ) ( ( *pixel >> 0 ) & 0xff ),
                               ( r32 ) ( ( *pixel >> 24 ) &0xff ) );
                
                dest = SRGB255ToLinear1( dest );
                
                texel = Hadamart( texel, color );
                r32 invRSA = 1.0f - texel.a;
                
                Vec4 final = V4( invRSA * dest.r + texel.r,
                                invRSA * dest.g + texel.g,
                                invRSA * dest.b + texel.b,
                                ( texel.a + dest.a - ( texel.a * dest.a ) ) );
                
                final = Linear1ToSRGB255( final );
                *pixel = ( RoundReal32ToU32( final.a ) << 24 ) |( RoundReal32ToU32 ( final.r ) << 16 ) | 
                    ( RoundReal32ToU32( final.g ) << 8 ) |
                    ( RoundReal32ToU32 ( final.b ) << 0 );
            }
            pixel++;
        }
        row += buffer->width;
        
    }
}

void DrawRectangleQuickly( Bitmap * buffer, Bitmap* texture, 
                          Vec2 origin, Vec2 XAxis, Vec2 YAxis, Vec4 color,Rect2i clipRect )
{
    //TIMED_BLOCK(DrawRectangleQuickly);
    color.rgb *= color.a;
    
    r32 XAxisLength = Length(XAxis);
    r32 YAxisLength = Length(YAxis);
    
    Vec2 NxAxis = (YAxisLength / XAxisLength) * XAxis;
    Vec2 NyAxis = (XAxisLength / YAxisLength) * YAxis;
    
    // NOTE(leonardo): NzScale could be a parameter if we want people to
    // have control over the amount of scaling in the Z direction
    // that the normals appear to have.
    r32 NzScale = 0.5f*(XAxisLength + YAxisLength);
    
    r32 InvXAxisLengthSq = 1.0f / LengthSq(XAxis);
    r32 InvYAxisLengthSq = 1.0f / LengthSq(YAxis);
    
    Rect2i fillRect = InvertedInfinityRect2i();
    
    Vec2 P[4] = {origin, origin + XAxis, origin + XAxis + YAxis, origin + YAxis};
    for(int PIndex = 0;
        PIndex < ArrayCount(P);
        ++PIndex)
    {
        Vec2 TestP = P[PIndex];
        int FloorX = Floor(TestP.x);
        int CeilX = Ceil(TestP.x);
        int FloorY = Floor(TestP.y);
        int CeilY = Ceil(TestP.y);
        
        if(fillRect.minX > FloorX) {fillRect.minX = FloorX;}
        if(fillRect.minY > FloorY) {fillRect.minY = FloorY;}
        if(fillRect.maxX < CeilX) {fillRect.maxX = CeilX;}
        if(fillRect.maxY < CeilY) {fillRect.maxY = CeilY;}
    }
    
    fillRect = Intersect( fillRect, clipRect );
    if( HasArea( fillRect ) )
    {
        __m128i startClipMask = _mm_set1_epi8(-1);
        __m128i endClipMask = _mm_set1_epi8(-1);
        
        __m128i startClipMasks[] = {
            _mm_slli_si128( startClipMask, 0 * 4 ),
            _mm_slli_si128( startClipMask, 1 * 4 ),
            _mm_slli_si128( startClipMask, 2 * 4 ),
            _mm_slli_si128( startClipMask, 3 * 4 ) };
        
        __m128i endClipMasks[] = {
            _mm_srli_si128( endClipMask, 0 * 4 ),
            _mm_srli_si128( endClipMask, 3 * 4 ),
            _mm_srli_si128( endClipMask, 2 * 4 ),
            _mm_srli_si128( endClipMask, 1 * 4 ) };
        
        if( fillRect.minX & 3 )
        {
            fillRect.minX = fillRect.minX & ~3;
            startClipMask = startClipMasks[fillRect.minX & 3];
        }
        
        if( fillRect.maxX & 3 )
        {
            fillRect.maxX = ( fillRect.maxX & ~3 ) + 4;
            endClipMask = endClipMasks[fillRect.maxX & 3];
        }
        
        Vec2 nXAxis = InvXAxisLengthSq*XAxis;
        Vec2 nYAxis = InvYAxisLengthSq*YAxis;
        
        r32 InVec255 = 1.0f / 255.0f;
        __m128 InVec255_4x = _mm_set1_ps(InVec255);
        r32 One255 = 255.0f;
        i32 texPitch = texture->width * 4;
        __m128i texPitch_4x = _mm_set1_epi32(texPitch);
        __m128 One = _mm_set1_ps(1.0f);
        __m128 One255_4x = _mm_set1_ps(255.0f);
        __m128 Zero = _mm_set1_ps(0.0f);
        __m128 half = _mm_set1_ps( 0.5f );
        __m128i MaskFF = _mm_set1_epi32(0xFF);
        __m128 Colorr_4x = _mm_set1_ps(color.r);
        __m128 Colorg_4x = _mm_set1_ps(color.g);
        __m128 Colorb_4x = _mm_set1_ps(color.b);
        __m128 Colora_4x = _mm_set1_ps(color.a);
        __m128 nXAxisx_4x = _mm_set1_ps(nXAxis.x);
        __m128 nXAxisy_4x = _mm_set1_ps(nXAxis.y);
        __m128 nYAxisx_4x = _mm_set1_ps(nYAxis.x);
        __m128 nYAxisy_4x = _mm_set1_ps(nYAxis.y);
        __m128 originx_4x = _mm_set1_ps(origin.x);
        __m128 originy_4x = _mm_set1_ps(origin.y);
        
        __m128 WidthM2 = _mm_set1_ps((r32)(texture->width - 2));
        __m128 HeightM2 = _mm_set1_ps((r32)(texture->height - 2));
        
        i32 maxX = fillRect.maxX;
        i32 minX = fillRect.minX;
        i32 minY = fillRect.minY;
        i32 maxY = fillRect.maxY;
        u8 *Row = ((u8 *)buffer->pixels +
                   minX*4 +
                   minY*buffer->width * 4);
        //TIMED_BLOCK(FillRectangle, GetClampedArea( fillRect ) / 2);
        for(int Y = minY;
            Y < maxY;
            ++Y)
        {
            __m128i ClipMask = startClipMask;
            __m128 PixelPy = _mm_set1_ps((r32)Y);
            __m128 dy = _mm_sub_ps(PixelPy, originy_4x);
            
            u32 *Pixel = (u32 *)Row;
            for(int XI = minX;
                XI < maxX;
                XI += 4)
            {            
                __m128 PixelPx = _mm_set_ps((r32)(XI + 3),
                                            (r32)(XI + 2),
                                            (r32)(XI + 1),
                                            (r32)(XI + 0));
                __m128 dx = _mm_sub_ps(PixelPx, originx_4x);
                __m128 U = _mm_add_ps(_mm_mul_ps(dx, nXAxisx_4x), _mm_mul_ps(dy, nXAxisy_4x));
                __m128 V = _mm_add_ps(_mm_mul_ps(dx, nYAxisx_4x), _mm_mul_ps(dy, nYAxisy_4x));
                
                __m128i WriteMask = _mm_castps_si128(_mm_and_ps(_mm_and_ps(_mm_cmpge_ps(U, Zero),
                                                                           _mm_cmple_ps(U, One)),
                                                                _mm_and_ps(_mm_cmpge_ps(V, Zero),
                                                                           _mm_cmple_ps(V, One))));
                
                WriteMask = _mm_and_si128( WriteMask, ClipMask );
                // TODO(leonardo): Later, re-check if this helps
                //            if(_mm_movemask_epi8(WriteMask))
                {
                    IACA_VC64_START;
                    __m128i originalDest = _mm_load_si128((__m128i *)Pixel);
                    
                    U = _mm_min_ps(_mm_max_ps(U, Zero), One);
                    V = _mm_min_ps(_mm_max_ps(V, Zero), One);
                    
                    __m128 tX = _mm_add_ps( _mm_mul_ps(U, WidthM2), half );
                    __m128 tY = _mm_add_ps( _mm_mul_ps(V, HeightM2), half );
                    
                    __m128i FetchX_4x = _mm_cvttps_epi32(tX);
                    __m128i FetchY_4x = _mm_cvttps_epi32(tY);
                    
                    __m128 fX = _mm_sub_ps(tX, _mm_cvtepi32_ps(FetchX_4x));
                    __m128 fY = _mm_sub_ps(tY, _mm_cvtepi32_ps(FetchY_4x));
                    
                    FetchX_4x = _mm_slli_epi32( FetchX_4x, 2 );
                    
#if 0
                    FetchY_4x = _mm_mullo_epi32( FetchY_4x, texPitch_4x );
#else
                    
                    
                    *( ( u32* ) ( &FetchY_4x ) + 0 ) = (u32)(((u64)(Mi( FetchY_4x, 0))*(u64)(Mi(texPitch_4x, 0))) & 0xffffffff  );
                    *( ( u32* ) ( &FetchY_4x ) + 1 ) = (u32)(((u64)(Mi( FetchY_4x, 1))*(u64)(Mi(texPitch_4x, 1))) & 0xffffffff  );
                    *( ( u32* ) ( &FetchY_4x ) + 2 ) = (u32)(((u64)(Mi( FetchY_4x, 2))*(u64)(Mi(texPitch_4x, 2))) & 0xffffffff  );
                    *( ( u32* ) ( &FetchY_4x ) + 3 ) = (u32)(((u64)(Mi( FetchY_4x, 3))*(u64)(Mi(texPitch_4x, 3))) & 0xffffffff  );
                    
#endif
                    __m128i Fetch_4x = _mm_add_epi32( FetchY_4x, FetchX_4x );
                    
                    i32 Fetch0 = Mi(Fetch_4x, 0);
                    i32 Fetch1 = Mi(Fetch_4x, 1);
                    i32 Fetch2 = Mi(Fetch_4x, 2);
                    i32 Fetch3 = Mi(Fetch_4x, 3);
                    
                    u8 *TexelPtr0 = ((u8 *)texture->pixels) + Fetch0;
                    u8 *TexelPtr1 = ((u8 *)texture->pixels) + Fetch1;
                    u8 *TexelPtr2 = ((u8 *)texture->pixels) + Fetch2;
                    u8 *TexelPtr3 = ((u8 *)texture->pixels) + Fetch3;
                    
                    __m128i SampleA = _mm_setr_epi32(*(u32 *)(TexelPtr0),
                                                     *(u32 *)(TexelPtr1),
                                                     *(u32 *)(TexelPtr2),
                                                     *(u32 *)(TexelPtr3));
                    
                    __m128i SampleB = _mm_setr_epi32( *(u32 *)(TexelPtr0 + sizeof(u32)),
                                                     *(u32 *)(TexelPtr1 + sizeof(u32)),
                                                     *(u32 *)(TexelPtr2 + sizeof(u32)),
                                                     *(u32 *)(TexelPtr3 + sizeof(u32)) );
                    
                    __m128i SampleC = _mm_setr_epi32(
                        *(u32 *) (TexelPtr0 + texPitch),
                        *(u32 *) (TexelPtr1 + texPitch),
                        *(u32 *) (TexelPtr2 + texPitch),
                        *(u32 *) (TexelPtr3 + texPitch) );
                    
                    i32 sizeU = sizeof(u32);
                    __m128i SampleD = _mm_setr_epi32(
                        *(u32 * )(TexelPtr0 + texPitch + sizeU), *(u32* )(TexelPtr1 + texPitch + sizeU),
                        *(u32 *)(TexelPtr2 + texPitch + sizeU),
                        *(u32 *)(TexelPtr3 + texPitch + sizeU) );
                    
                    // NOTE(leonardo): Unpack bilinear samples
                    __m128 TexelAb = _mm_cvtepi32_ps(_mm_and_si128(SampleA, MaskFF));
                    __m128 TexelAg = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(SampleA, 8), MaskFF));
                    __m128 TexelAr = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(SampleA, 16), MaskFF));
                    __m128 TexelAa = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(SampleA, 24), MaskFF));
                    
                    __m128 TexelBb = _mm_cvtepi32_ps(_mm_and_si128(SampleB, MaskFF));
                    __m128 TexelBg = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(SampleB, 8), MaskFF));
                    __m128 TexelBr = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(SampleB, 16), MaskFF));
                    __m128 TexelBa = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(SampleB, 24), MaskFF));
                    
                    __m128 TexelCb = _mm_cvtepi32_ps(_mm_and_si128(SampleC, MaskFF));
                    __m128 TexelCg = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(SampleC, 8), MaskFF));
                    __m128 TexelCr = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(SampleC, 16), MaskFF));
                    __m128 TexelCa = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(SampleC, 24), MaskFF));
                    
                    __m128 TexelDb = _mm_cvtepi32_ps(_mm_and_si128(SampleD, MaskFF));
                    __m128 TexelDg = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(SampleD, 8), MaskFF));
                    __m128 TexelDr = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(SampleD, 16), MaskFF));
                    __m128 TexelDa = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(SampleD, 24), MaskFF));
                    
                    // NOTE(leonardo): Load destination
                    __m128 Destb = _mm_cvtepi32_ps(_mm_and_si128(originalDest, MaskFF));
                    __m128 Destg = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(originalDest, 8), MaskFF));
                    __m128 Destr = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(originalDest, 16), MaskFF));
                    __m128 Desta = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(originalDest, 24), MaskFF));
                    
                    // NOTE(leonardo): Convert texture from 0-255 sRGB to "linear" 0-1 brightness space
                    TexelAr = mmSquare(_mm_mul_ps(InVec255_4x, TexelAr));
                    TexelAg = mmSquare(_mm_mul_ps(InVec255_4x, TexelAg));
                    TexelAb = mmSquare(_mm_mul_ps(InVec255_4x, TexelAb));
                    TexelAa = _mm_mul_ps(InVec255_4x, TexelAa);
                    
                    TexelBr = mmSquare(_mm_mul_ps(InVec255_4x, TexelBr));
                    TexelBg = mmSquare(_mm_mul_ps(InVec255_4x, TexelBg));
                    TexelBb = mmSquare(_mm_mul_ps(InVec255_4x, TexelBb));
                    TexelBa = _mm_mul_ps(InVec255_4x, TexelBa);
                    
                    TexelCr = mmSquare(_mm_mul_ps(InVec255_4x, TexelCr));
                    TexelCg = mmSquare(_mm_mul_ps(InVec255_4x, TexelCg));
                    TexelCb = mmSquare(_mm_mul_ps(InVec255_4x, TexelCb));
                    TexelCa = _mm_mul_ps(InVec255_4x, TexelCa);
                    
                    TexelDr = mmSquare(_mm_mul_ps(InVec255_4x, TexelDr));
                    TexelDg = mmSquare(_mm_mul_ps(InVec255_4x, TexelDg));
                    TexelDb = mmSquare(_mm_mul_ps(InVec255_4x, TexelDb));
                    TexelDa = _mm_mul_ps(InVec255_4x, TexelDa);
                    
                    // NOTE(leonardo): Bilinear texture blend
                    __m128 ifX = _mm_sub_ps(One, fX);
                    __m128 ifY = _mm_sub_ps(One, fY);
                    
                    __m128 l0 = _mm_mul_ps(ifY, ifX);
                    __m128 l1 = _mm_mul_ps(ifY, fX);
                    __m128 l2 = _mm_mul_ps(fY, ifX);
                    __m128 l3 = _mm_mul_ps(fY, fX);
                    
                    __m128 Texelr = _mm_add_ps(_mm_add_ps(_mm_mul_ps(l0, TexelAr), _mm_mul_ps(l1, TexelBr)),
                                               _mm_add_ps(_mm_mul_ps(l2, TexelCr), _mm_mul_ps(l3, TexelDr)));
                    __m128 Texelg = _mm_add_ps(_mm_add_ps(_mm_mul_ps(l0, TexelAg), _mm_mul_ps(l1, TexelBg)),
                                               _mm_add_ps(_mm_mul_ps(l2, TexelCg), _mm_mul_ps(l3, TexelDg)));
                    __m128 Texelb = _mm_add_ps(_mm_add_ps(_mm_mul_ps(l0, TexelAb), _mm_mul_ps(l1, TexelBb)),
                                               _mm_add_ps(_mm_mul_ps(l2, TexelCb), _mm_mul_ps(l3, TexelDb)));
                    __m128 Texela = _mm_add_ps(_mm_add_ps(_mm_mul_ps(l0, TexelAa), _mm_mul_ps(l1, TexelBa)),
                                               _mm_add_ps(_mm_mul_ps(l2, TexelCa), _mm_mul_ps(l3, TexelDa)));
                    
                    // NOTE(leonardo): Modulate by incoming color
                    Texelr = _mm_mul_ps(Texelr, Colorr_4x);
                    Texelg = _mm_mul_ps(Texelg, Colorg_4x);
                    Texelb = _mm_mul_ps(Texelb, Colorb_4x);
                    Texela = _mm_mul_ps(Texela, Colora_4x);
                    
                    Texelr = _mm_min_ps(_mm_max_ps(Texelr, Zero), One);
                    Texelg = _mm_min_ps(_mm_max_ps(Texelg, Zero), One);
                    Texelb = _mm_min_ps(_mm_max_ps(Texelb, Zero), One);
                    
                    // NOTE(leonardo): Go from sRGB to "linear" brightness space
                    Destr = mmSquare(_mm_mul_ps(InVec255_4x, Destr));
                    Destg = mmSquare(_mm_mul_ps(InVec255_4x, Destg));
                    Destb = mmSquare(_mm_mul_ps(InVec255_4x, Destb));
                    Desta = _mm_mul_ps(InVec255_4x, Desta);
                    
                    // NOTE(leonardo): Destination blend
                    __m128 InvTexelA = _mm_sub_ps(One, Texela);
                    __m128 Blendedr = _mm_add_ps(_mm_mul_ps(InvTexelA, Destr), Texelr);
                    __m128 Blendedg = _mm_add_ps(_mm_mul_ps(InvTexelA, Destg), Texelg);
                    __m128 Blendedb = _mm_add_ps(_mm_mul_ps(InvTexelA, Destb), Texelb);
                    __m128 Blendeda = _mm_add_ps(_mm_mul_ps(InvTexelA, Desta), Texela);
                    
                    // NOTE(leonardo): Go from "linear" 0-1 brightness space to sRGB 0-255
                    Blendedr = _mm_mul_ps(One255_4x, _mm_sqrt_ps(Blendedr));
                    Blendedg = _mm_mul_ps(One255_4x, _mm_sqrt_ps(Blendedg));
                    Blendedb = _mm_mul_ps(One255_4x, _mm_sqrt_ps(Blendedb));
                    Blendeda = _mm_mul_ps(One255_4x, Blendeda);
                    
                    __m128i Intr = _mm_cvtps_epi32(Blendedr);
                    __m128i Intg = _mm_cvtps_epi32(Blendedg);
                    __m128i Intb = _mm_cvtps_epi32(Blendedb);
                    __m128i Inta = _mm_cvtps_epi32(Blendeda);
                    
                    __m128i Sr = _mm_slli_epi32(Intr, 16);
                    __m128i Sg = _mm_slli_epi32(Intg, 8);
                    __m128i Sb = Intb;
                    __m128i Sa = _mm_slli_epi32(Inta, 24);
                    
                    __m128i Out = _mm_or_si128(_mm_or_si128(Sr, Sg), _mm_or_si128(Sb, Sa));
                    
#if 1
                    __m128i MaskedOut = _mm_or_si128(_mm_and_si128(WriteMask, Out),
                                                     _mm_andnot_si128(WriteMask, originalDest));
                    _mm_store_si128((__m128i *)Pixel, MaskedOut);
#else
                    _mm_maskmoveu_si128(Out, WriteMask, (char *)Pixel);
#endif
                    IACA_VC64_END;
                }
                
                Pixel += 4;
                ClipMask = _mm_set1_epi8(-1);
                
                if( ( XI + 8 ) >= maxX )
                {
                    ClipMask = endClipMask;
                }
            }
            
            Row += ( buffer->width * 4 );
            
        }
    }
}
#endif

inline void RenderCommands( GameRenderCommands* commands, Bitmap* dest, Rect2i baseClip )
{
    
#if 0    
    u16 clipRectIndex = 0xFFFF;
    Rect2i clip = baseClip;
    
    u32 walkedSize = 0;
    for( u32 elementIndex = 0; elementIndex < commands->bufferElementCount; ++elementIndex )
    {
        CommandHeader* header = ( CommandHeader* ) ( commands->pushMemory + walkedSize );
        walkedSize += sizeof( CommandHeader );
        if( header->clipRectIndex != clipRectIndex )
        {
            clipRectIndex = header->clipRectIndex;
            Assert( clipRectIndex < commands->clipRectCount );
            
            ClipRectCommand* clipCommand = commands->clipRects + clipRectIndex;
            clip = Intersect( clip, clipCommand->rect );
        }
        
        void* data = ( header + 1 ); 
        
#if 0        
        switch( header->type )
        {
            
            case CommandType_ClearCommand:
            {
                ClearCommand* clear = ( ClearCommand* ) data;
                Vec4 color = clear->color;
                DrawRectangle( dest, V2( 0, 0 ), V2i( dest->width, dest->height ), color, clip );
                walkedSize += sizeof( ClearCommand );
            } break;
            
            case CommandType_RectCommand:
            {
                RectCommand* element = ( RectCommand* ) data;
                DrawRectangle( dest, element->min, element->max, element->color, clip );
                walkedSize += sizeof( RectCommand );
            } break;
            
            case CommandType_TrapezoidCommand:
            {
                InvalidCodePath;
                walkedSize += sizeof( TrapezoidCommand );
            } break;
            
            case CommandType_BitmapCommand:
            {
                BitmapCommand* element = ( BitmapCommand* ) data;
                if( element->bitmap )
                {
                    DrawRectangleQuickly( dest, element->bitmap, element->P, 
                                         element->finalXAxis, element->finalYAxis, 
                                         element->color, clip ); 
                }
                walkedSize += sizeof( BitmapCommand );
            } break;
            
            case CommandType_ClipRectCommand:
            {
                walkedSize += sizeof( ClipRectCommand );
            } break;
            
            default:
            {
                InvalidCodePath;
            }
        }
#endif
        
    }
#endif
    
}

struct TiledRenderWork
{
    Rect2i clip;
    GameRenderCommands* commands;
    Bitmap* dest;
};

PLATFORM_WORK_CALLBACK( RenderTileToOutput )
{
    TIMED_FUNCTION();
    TiledRenderWork* work = ( TiledRenderWork* ) param;
    RenderCommands( work->commands, work->dest, work->clip );
}

inline void SoftwareRenderCommands( PlatformWorkQueue* renderQueue, GameRenderCommands* commands, Bitmap* dest )
{
    Assert( ( ( ( unm ) dest->pixels ) & 15 ) == 0 );
    i32 const countTileX = 4;
    i32 const countTileY = 4;
    i32 tileWidth = dest->width / countTileX;
    i32 tileHeight = dest->height / countTileY;
    
    tileWidth = ( tileWidth + 3 ) / 4 * 4;
    TiledRenderWork works[countTileX * countTileY];
    
    TiledRenderWork* work = works;
    for( i32 tileY = 0; tileY < countTileY; tileY++ )
    {
        for( i32 tileX = 0; tileX < countTileX; tileX++ )
        {
            Rect2i clip;
            clip.minX = tileX * tileWidth;
            clip.maxX = tileX * tileWidth + tileWidth;
            clip.minY = tileY * tileHeight;
            clip.maxY = tileY * tileHeight + tileHeight;
            
            if( tileX == ( countTileX - 1 ) )
            {
                clip.maxX = dest->width;
            }
            
            if( tileY == ( countTileY - 1 ) )
            {
                clip.maxY = dest->height;
            }
            
            work->commands = commands;
            work->dest = dest;
            work->clip = clip;
            
            platform.PushWork( renderQueue, RenderTileToOutput, work );
            work++;
        }
    }
    platform.CompleteQueueWork( renderQueue );
}

internal Rect2i AspectRatioFit( i32 renderWidth, i32 renderHeight, i32 windowWidth, i32 windowHeight )
{
    Rect2i result = {};
    if( ( renderWidth > 0 && renderHeight > 0 && windowWidth > 0 && windowHeight > 0 ) )
    {
        r32 optimalWindowWidth = ( r32 ) windowHeight * ( ( r32 ) renderWidth / ( r32 ) renderHeight );
        r32 optimalWindowHeight = ( r32 ) windowWidth * ( ( r32 ) renderHeight / ( r32 ) renderWidth );
        
        if( optimalWindowWidth > ( r32 ) windowWidth )
        {
            // NOTE(Leonardo): width constrained display - top and bottom black bars
            result.minX = 0;
            result.maxX = windowWidth;
            
            r32 empty = ( r32 ) windowHeight - optimalWindowHeight;
            i32 halfEmpty = RoundReal32ToI32( empty * 0.5f );
            i32 useHeight = RoundReal32ToI32( optimalWindowHeight );
            
            result.minY = halfEmpty;
            result.maxY = result.minY + useHeight;
        }
        else
        {
            // NOTE(Leonardo): height constrained display - left and right black bars
            result.minY = 0;
            result.maxY = windowHeight;
            
            r32 empty = ( r32 ) windowWidth - optimalWindowWidth;
            i32 halfEmpty = RoundReal32ToI32( empty * 0.5f );
            i32 useWidth = RoundReal32ToI32( optimalWindowWidth );
            
            result.minX = halfEmpty;
            result.maxX = result.minX + useWidth;
        }
    }
    
    return result;
}