internal void Resize(ImageU32 source, ImageU32 dest)
{
#if 1
    stbir_resize_uint8((unsigned char*) source.pixels, source.width, source.height, 0, (unsigned char*) dest.pixels, dest.width, dest.height, 0, 4);
#else
    stbir_resize_uint8_srgb_edgemode((unsigned char*) source.pixels, source.width, source.height, 0, (unsigned char*) dest.pixels, dest.width, dest.height, 0, 4, 0, 4, STBIR_EDGE_CLAMP);
#endif
}


internal u32 GetTotalMIPsSize(u32 width, u32 height)
{
    u32 result = 0;
    
    for(MIPIterator mip = BeginMIPs(width, height, 0);
        IsValid(&mip);
        Advance(&mip))
    {
        result += (mip.image.width * mip.image.height * 4);
    }
    
    return result;
}

internal void GenerateSequentialMIPMAPs(u32 width, u32 height, u32* pixels)
{
    MIPIterator mip = BeginMIPs(width, height, pixels);
    ImageU32 source = mip.image;
    Advance(&mip);
    while(IsValid(&mip))
    {
        Resize(source, mip.image);
        source = mip.image;
        Advance(&mip);
    }
}



