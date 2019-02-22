#define internal
#include "forg_client.h"

#if 0
#include <iacaMarks.h>
#else
#define IACA_VC64_START
#define IACA_VC64_END
#endif

void DrawRectangleQuickly( Bitmap * buffer, Bitmap* texture, 
                          Vec2 origin, Vec2 XAxis, Vec2 YAxis, Vec4 color,Rect2i clipRect, b32 even )
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
    
    Rect2i fillRect = InvertedInfinityRect();
    
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
    b32 evenRow = ( fillRect.minY & 1 );
    if( !even == evenRow )
    {
        fillRect.minY += 1;
    }
    
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
            Y += 2)
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
            
            Row += ( buffer->width * 4 * 2 );
            
        }
    }
}