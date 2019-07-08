#define STB_IMAGE_RESIZE_IMPLEMENTATION 1
#include "stb_resize_image.h"

internal void LerpTextures(u32* t0, u32* t1, u32* dest, u32 width, u32 height, Vec2 refP, r32 refAngle, r32 refLength)
{
    Vec2 refUnit = Arm2(DegToRad(refAngle));
    for(u32 Y = 0; Y < height; ++Y)
    {
        for(u32 X = 0; X < width; ++X)
        {
            u32 index = Y * width + X;
            
            Vec2 pixelP = V2i(X, Y);
            Vec2 toRefP = pixelP - refP;
            
            r32 dot = Dot(toRefP, refUnit);
            r32 lerp = Clamp01(dot / refLength);
            
            Vec4 p0 = RGBAUnpack4x8(t0[index]);
            Vec4 p1 = RGBAUnpack4x8(t1[index]);
            
            Vec4 res = Lerp(p0, lerp, p1);
            
            dest[index] = RGBAPack8x4(res);
        }
    }
}

inline Vec4 HeightLerp(Vec4 c0, r32 h0, Vec4 c1, r32 h1, Vec4 c2, r32 h2, Vec4 c3, r32 h3, r32 blendFactor)
{
    r32 hStart = (Max(Max(Max(h0, h1), h2), h3)) - blendFactor;
    
    r32 level0 = Max(h0 - hStart, 0);
    r32 level1 = Max(h1 - hStart, 0);
	r32 level2 = Max(h2 - hStart, 0);
	r32 level3 = Max(h3 - hStart, 0);
    
    Vec4 result = ((c0 * level0) + (c1 * level1) + (c2 * level2) + (c3 * level3)) * (1.0f / (level0 + level1 + level2 + level3));
    
    return result;
}

internal void Lerp9TexturesWithHeightmaps(u32* textures[9], u32* heightmaps[9], u32* dest, u32 width, u32 height, r32 blendFactor, r32 influences[9])
{
	u32 halfWidth = width / 2;
	u32 halfHeight = height / 2;
	r32 oneOverRefLength = 1.0f / (r32) halfWidth;
    
	for(u32 Y = 0; Y < height; ++Y)
    {
        for(u32 X = 0; X < width; ++X)
        {
            u32 index = Y * width + X;
            Vec2 pixelP = V2i(X, Y);
			
			Vec2 lowLeftP;
			u32 startingIndex;
			if(X < halfWidth)
			{
				if(Y < halfHeight)
				{
					lowLeftP = V2i(0, 0);
					startingIndex = 0;
				}
				else
				{
					lowLeftP = V2i(0, halfHeight);
					startingIndex = 3;
				}
			}
			else
			{
				if(Y < halfHeight)
				{
					lowLeftP = V2i(halfWidth, 0);
					startingIndex = 1;
				}
				else
				{
					lowLeftP = V2i(halfWidth, halfHeight);
					startingIndex = 4;
				}
			}
            
			u32 i0 = startingIndex;
			u32 i1 = i0 + 1;
			u32 i2 = i0 + 3;
			u32 i3 = i2 + 1;
            
			Vec2 P0 = lowLeftP;
			Vec2 P1 = P0 + V2i(halfWidth, 0);
			Vec2 P2 = P0 + V2i(0, halfHeight);
			Vec2 P3 = P2 + V2i(halfWidth, 0);
            
			r32 w0 = 1.0f - Clamp01(Length(pixelP - P0) * oneOverRefLength * influences[i0]);
			r32 w1 = 1.0f - Clamp01(Length(pixelP - P1) * oneOverRefLength * influences[i1]);
			r32 w2 = 1.0f - Clamp01(Length(pixelP - P2) * oneOverRefLength * influences[i2]);
			r32 w3 = 1.0f - Clamp01(Length(pixelP - P3) * oneOverRefLength * influences[i3]);
            
			u32* T0 = textures[i0];
			u32* T1 = textures[i1];
			u32* T2 = textures[i2];
			u32* T3 = textures[i3];
            
			u32* H0 = heightmaps[i0];
			u32* H1 = heightmaps[i1];
			u32* H2 = heightmaps[i2];
			u32* H3 = heightmaps[i3];
            
            
            Vec4 p0 = RGBAUnpack4x8(T0[index]);
            Vec4 p1 = RGBAUnpack4x8(T1[index]);
			Vec4 p2 = RGBAUnpack4x8(T2[index]);
            Vec4 p3 = RGBAUnpack4x8(T3[index]);
            
            r32 h0 = RGBAUnpack4x8(H0[index]).r;
            r32 h1 = RGBAUnpack4x8(H1[index]).r;
			r32 h2 = RGBAUnpack4x8(H2[index]).r;
			r32 h3 = RGBAUnpack4x8(H3[index]).r;
            
            Vec4 final = HeightLerp(p0, h0 * w0, p1, h1 * w1, p2, h2 * w2, p3, h3 * w3, blendFactor);
            
            dest[index] = RGBAPack8x4(final);
        }
    }
}



