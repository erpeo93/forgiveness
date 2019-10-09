internal b32 ShouldCollide(u64 id1, ForgBoundType b1, u64 id2, ForgBoundType b2)
{
    b32 result = true;
    return result;
}


internal void PointCubeCollision(Vec3 P, Vec3 deltaP, Rect3 minkowski, r32* tMin, Vec3* tMinNormal)
{
    r32 deltaX = deltaP.x;
    r32 deltaY = deltaP.y;
    r32 deltaZ = deltaP.z;
    
    Vec3 minCorner = minkowski.min;
    Vec3 maxCorner = minkowski.max;
    
    Wall walls[] = { 
        {maxCorner.x, 
            deltaX, deltaY, deltaZ, 
            P.x, P.y, P.z,
            minCorner.y, maxCorner.y, 
            minCorner.z, maxCorner.z, V3(1, 0, 0)},
        
        {minCorner.x, 
            deltaX, deltaY, deltaZ, 
            P.x, P.y, P.z,
            minCorner.y, maxCorner.y, 
            minCorner.z, maxCorner.z, V3(-1, 0, 0)},
        
        {maxCorner.y, 
            deltaY, deltaX, deltaZ, 
            P.y, P.x, P.z,
            minCorner.x, maxCorner.x, 
            minCorner.z, maxCorner.z, V3(0, 1, 0)},
        {minCorner.y, 
            deltaY, deltaX, deltaZ, 
            P.y, P.x, P.z,
            minCorner.x, maxCorner.x, 
            minCorner.z, maxCorner.z, V3(0, -1, 0)}
#if 0        
        ,
        {maxCorner.z, 
            deltaZ, deltaX, deltaY, 
            P.z, P.x, P.y,
            minCorner.x, maxCorner.x, 
            minCorner.y, maxCorner.y, V3(0, 0, 1)},
        {minCorner.z, 
            deltaZ, deltaX, deltaY, 
            P.z, P.x, P.y,
            minCorner.x, maxCorner.x, 
            minCorner.y, maxCorner.y, V3(0, 0, -1)}
#endif
        
    };
    
    r32 epsilon = 0.01f;
    for(u32 wallIndex = 0; wallIndex < ArrayCount(walls); wallIndex++)
    {
        Wall* wall = walls + wallIndex;
        if(wall->deltax != 0)
        {
            r32 newT = (wall->x - wall->px) / wall->deltax;
            if(newT >= epsilon)
            {
                newT = newT - epsilon;
                if(newT < *tMin)
                {
                    r32 y = wall->py + newT * wall->deltay;
                    r32 Z = wall->pz + newT * wall->deltaz;
                    if(y >= wall->miny && y <= wall->maxy &&
                       Z >= wall->minz && Z <= wall->maxz)
                    {
                        *tMinNormal = wall->normal;
                        *tMin = newT;
                    }
                }
            }
        }
    }
}

internal void HandleVolumeCollision(Vec3 P, Rect3 bounds, Vec3 deltaP, Vec3 testP, Rect3 testBounds, r32* tMin, Vec3* wallNormalMin)
{
    Vec3 fakeDelta = GetCenter(bounds);
    Vec3 myP = P + fakeDelta;
    deltaP -= fakeDelta;
    
    Vec3 minkowskiCenter = testP + GetCenter(testBounds);
    
    Vec3 minkowskiDimension =  GetDim(bounds) + GetDim(testBounds);
    Rect3 minkowski = RectCenterDim(minkowskiCenter, minkowskiDimension);
    
    if(!PointInRect(minkowski, myP))
    {
        PointCubeCollision(myP, deltaP, minkowski, tMin, wallNormalMin);
    }
}
