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

inline Vec4 HeightLerp(Vec4 c0, r32 h0, Vec4 c1, r32 h1, r32 blendFactor)
{
    r32 hStart = Max(h0, h1) - blendFactor;
    
    r32 level0 = Max(h0 - hStart, 0);
    r32 level1 = Max(h1 - hStart, 0);
    
    Vec4 result = (c0 * level0 + c1 * level1) * (1.0f / (level0 + level1));
    
    return result;
}

internal void LerpTexturesWithHeightmaps(u32* t0, u32* h0, u32* t1, u32* h1, u32* dest, u32 width, u32 height, Vec2 refP, r32 refAngle, r32 refLength)
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
            
            r32 height0 = RGBAUnpack4x8(h0[index]).r;
            r32 height1 = RGBAUnpack4x8(h1[index]).r;
            
            r32 blendFactor = 0.05f;
            Vec4 final = HeightLerp(p0, height0 * (1.0f - lerp), p1, height1 * lerp, blendFactor);
            
            dest[index] = RGBAPack8x4(final);
        }
    }
}

internal void LerpTileTextures()
{
    BitmapId T0 = GetFirstBitmap(assets, Asset_Forest);
    BitmapId T1 = GetFirstBitmap(assets, Asset_Desert);
    BitmapId H0 = GetFirstBitmap(assets, Asset_ForestHeightmap);
    BitmapId H1 = GetFirstBitmap(assets, Asset_DesertHeightmap);
    
    if(IsValid(T0) && IsValid(T1) && 
       IsValid(H0) && IsValid(H0))
    {
        Bitmap* t0 = GetBitmap(assets, T0);
        Bitmap* t1 = GetBitmap(assets, T1);
        
        Bitmap* h0 = GetBitmap(assets, H0);
        Bitmap* h1 = GetBitmap(assets, H1);
        
        if(!t0)
        {
            result = false;
            LoadBitmap(assets, T0, false);
        }
        
        if(!t1)
        {
            result = false;
            LoadBitmap(assets, T1, false);
        }
        
        
        if(!h0)
        {
            result = false;
            LoadBitmap(assets, H0, false);
        }
        
        
        if(!h1)
        {
            result = false;
            LoadBitmap(assets, H1, false);
        }
        
        if(result)
        {
            if(t0->width == width && t0->height == height &&
               t1->width == width && t1->height == height &&
               h0->width == width && h0->height == height &&
               h1->width == width && h1->height == height )
            {
                LerpTexturesWithHeightmaps((u32*) t0->pixels, (u32*) h0->pixels, 
                                           (u32*) t1->pixels, (u32*) h1->pixels, 
                                           pixels, width, height, V2(0, 0.5f * height), 0, (r32) width);
            }
        }
    }
}

internal b32 GenerateTileTexture(u32* destPixels)
{
    b32 result = CopyStartingPixels();
    
    if(result)
    {
        for(each sorrounding Tile)
        {
            if(!mytile)
            {
                PickTileInfo();
                result = LerpTileTextures();
                if(!result)
                {
                    break;
                }
            }
        }
    }
    return result;
}

global_variable u32 textureTempPixels[TEXTURE_ARRAY_DIM][TEXTURE_ARRAY_DIM];
internal b32 GenerateChunkTexture(Assets* assets, GameModeWorld* worldMode, WorldChunk* chunk, u32* pixels, u32 width, u32 height)
{
    b32 result = true;
    u32* dest = pixels;
    
    Assert(TEXTURE_ARRAY_DIM % CHUNK_DIM == 0);
    u32 tileDim = TEXTURE_ARRAY_DIM / CHUNK_DIM;
    for(u32 Y = 0; Y < CHUNK_DIM && result; ++Y)
    {
        for(u32 X = 0; X < CHUNK_DIM && result; ++X)
        {
            if(!GenTileTexture(textureTempPixels))
            {
                result = false;
                break;
            }
            
            stbir_resize_uint8(textureTempPixels, TEXTURE_ARRAY_DIM, TEXTURE_ARRAY_DIM, 0, (unsigned char* ) dest, tileDim, tileDim, 0, 4);
            dest += tileDim * tileDim ?;
        }
    }
    
    return result;
}