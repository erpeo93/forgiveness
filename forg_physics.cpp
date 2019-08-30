internal b32 ShouldCollide(u64 id1, ForgBoundType b1, u64 id2, ForgBoundType b2)
{
    b32 result = false;
    
    if((id1 != id2) &&
       (b1 > ForgBound_NonPhysical) && (b2 > ForgBound_NonPhysical))
    {
        result = true;
    }
    
#if 0    
    if( e2->type < e1->type )
    {
        SimEntity* temp = e1;
        e1 = e2;
        e2 = temp;
    }
    
    if( !IsSet( e1, Flag_teleported ) )
    {
        if( e1->type == Entity_Human || e1->type == Entity_Enemy )
        {
            // TODO(Leonardo): next time you get here, build a static table that tells us if the two
            //entity type should collide.
            if( e2->type == Entity_Tree ||
               e2->type == Entity_Doungeon ||
               e2->type == Entity_Stairs ||
               e2->type == Entity_House )
            {
                result = true;
            }
        }
        
        if( e1->type == Entity_Doungeon && e2->type == Entity_House )
        {
            result = true;
        }
    }
#endif
    
    return result;
}

inline Rect3 GetMinkowskiRect(Rect3 bounds, Vec3 hisP, Rect3 hisBounds)
{
    Rect3 myVolume = bounds;
    Rect3 testVolume = Offset(hisBounds, hisP);
    Vec3 testVolumeCenter = GetCenter(testVolume);
    
    Vec3 minkowskiDimension =  GetDim(myVolume) + GetDim(testVolume);
    Rect3 result = RectCenterDim(hisP, minkowskiDimension);
    
    return result;
}


internal b32 CheckVolumeCollision(Vec3 P0, Vec3 deltaP, Rect3 minkowski, r32* tMinTest, Vec3* testWallNormal)
{
    b32 result = false;
    
    r32 deltaX = deltaP.x;
    r32 deltaY = deltaP.y;
    r32 deltaZ = deltaP.z;
    
    Vec3 minCorner = minkowski.min;
    Vec3 maxCorner = minkowski.max;
    
    Wall walls[] = { 
        {maxCorner.x, 
            deltaX, deltaY, deltaZ, 
            P0.x, P0.y, P0.z,
            minCorner.y, maxCorner.y, 
            minCorner.z, maxCorner.z, V3(1, 0, 0)},
        
        {minCorner.x, 
            deltaX, deltaY, deltaZ, 
            P0.x, P0.y, P0.z,
            minCorner.y, maxCorner.y, 
            minCorner.z, maxCorner.z, V3(-1, 0, 0)},
        
        {maxCorner.y, 
            deltaY, deltaX, deltaZ, 
            P0.y, P0.x, P0.z,
            minCorner.x, maxCorner.x, 
            minCorner.z, maxCorner.z, V3(0, 1, 0)},
        {minCorner.y, 
            deltaY, deltaX, deltaZ, 
            P0.y, P0.x, P0.z,
            minCorner.x, maxCorner.x, 
            minCorner.z, maxCorner.z, V3(0, -1, 0)},
        {maxCorner.z, 
            deltaZ, deltaX, deltaY, 
            P0.z, P0.x, P0.y,
            minCorner.x, maxCorner.x, 
            minCorner.y, maxCorner.y, V3(0, 0, 1)},
        {minCorner.z, 
            deltaZ, deltaX, deltaY, 
            P0.z, P0.x, P0.y,
            minCorner.x, maxCorner.x, 
            minCorner.y, maxCorner.y, V3(0, 0, -1)}};
    
    for(u32 wallIndex = 0; wallIndex < ArrayCount(walls); wallIndex++)
    {
        Wall* wall = walls + wallIndex;
        if(wall->deltax != 0)
        {
            r32 newT = (wall->x - wall->px) / wall->deltax;
            if(newT >= 0)
            {
                newT = newT - 0.01f;
                if(newT < *tMinTest)
                {
                    r32 y = wall->py + newT * wall->deltay;
                    r32 Z = wall->pz + newT * wall->deltaz;
                    if(y >= wall->miny && y <= wall->maxy &&
                       Z >= wall->minz && Z <= wall->maxz)
                    {
                        result = true;
                        *testWallNormal = wall->normal;
                        *tMinTest = newT;
                    }
                }
            }
        }
    }
    
    return result;
}

internal CheckCollisionCurrent HandleVolumeCollision(Vec3 P, Rect3 bounds, Vec3 deltaP, Vec3 testP, Rect3 testBounds, r32* tMin, Vec3* wallNormalMin, CheckCollisionCurrent in )
{
    CheckCollisionCurrent result = {};
    
    Vec3 myP = P;
    
    Rect3 minkowski = GetMinkowskiRect(bounds, testP, testBounds);
    if(!PointInRect(minkowski, myP))
    {
        r32 tMinTest = *tMin;
        Vec3 testWallNormal = {};
        if(CheckVolumeCollision(myP, deltaP, minkowski, tMin, wallNormalMin))
        {
            result = in;
        }
    }
    
    return result;
}
