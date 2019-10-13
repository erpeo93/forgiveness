
#define AddLightToGridCurrentFrame(worldMode, P, lightColor, strength) AddLightToGrid_(worldMode, P, lightColor, strength, false)
#define AddLightToGridNextFrame(worldMode, P, lightColor, strength) AddLightToGrid_(worldMode, P, lightColor, strength, true)
inline void AddLightToGrid_(GameModeWorld* worldMode, Vec3 P, Vec3 lightColor, r32 strength, b32 nextFrame, u32 chunkApron = 4)
{
    r32 chunkSide = VOXEL_SIZE * CHUNK_DIM;
    for(r32 offsetY = -chunkSide * chunkApron; offsetY <= chunkSide * chunkApron; offsetY += chunkSide)
    {
        for(r32 offsetX = -chunkSide * chunkApron; offsetX <= chunkSide * chunkApron; offsetX += chunkSide)
        {
            GetUniversePosQuery query = TranslateRelativePos(worldMode, worldMode->player.universeP, P.xy + V2(offsetX, offsetY));
            if(query.chunk)
            {
                TempLight* light;
                FREELIST_ALLOC(light, worldMode->firstFreeTempLight, PushStruct(worldMode->persistentPool, TempLight));
                light->P = P;
                light->color = lightColor;
                light->strength = strength;
                
				if(nextFrame)
				{
					FREELIST_INSERT(light, query.chunk->firstTempLightNextFrame);
				}
				else
				{
					FREELIST_INSERT(light, query.chunk->firstTempLight);
				}
            }
        }
    }
}

inline void ResetLightGrid(GameModeWorld* worldMode, b32 nextFrame = false)
{
    for(u32 chunkIndex = 0; chunkIndex < ArrayCount(worldMode->chunks); ++chunkIndex)
    {
        WorldChunk* chunk = worldMode->chunks[chunkIndex]; 
        while(chunk)
        {
			if(nextFrame)
			{
				FREELIST_FREE(chunk->firstTempLightNextFrame, TempLight, worldMode->firstFreeTempLight);
			}
			else
			{
				FREELIST_FREE(chunk->firstTempLight, TempLight, worldMode->firstFreeTempLight);
			}
            
            chunk = chunk->next;
        }
    }
}

inline u16 PushPointLight(RenderGroup* renderGroup, Vec3 P, Vec3 color, r32 strength);
inline void FinalizeLightGrid(GameModeWorld* worldMode, RenderGroup* group)
{
    for(u32 chunkIndex = 0; chunkIndex < ArrayCount(worldMode->chunks); ++chunkIndex)
    {
        WorldChunk* chunk = worldMode->chunks[chunkIndex]; 
        while(chunk)
        {
            b32 first = true;
            u16 startingIndex = 0;
            u16 endingIndex = 0;
            for(TempLight* light = chunk->firstTempLight; light; light = light->next)
            {
                u16 lightIndex = PushPointLight(group, light->P, light->color, light->strength);
                if(first)
                {
                    first = false;
                    startingIndex = lightIndex;
                }
                
                endingIndex = lightIndex + 1;
                
            }
            for(TempLight* light = chunk->firstTempLightNextFrame; light; light = light->next)
            {
                u16 lightIndex = PushPointLight(group, light->P, light->color, light->strength);
                if(first)
                {
                    first = false;
                    startingIndex = lightIndex;
                }
                
                endingIndex = lightIndex + 1;
                
            }
            
            chunk->lights.startingIndex = startingIndex;
            chunk->lights.endingIndex = endingIndex;
            
            for(u32 Y = 0; Y < CHUNK_DIM; ++Y)
            {
                for(u32 X = 0; X < CHUNK_DIM; ++X)
                {
                    WorldTile* tile = GetTile(worldMode, chunk, X, Y);
                    tile->lights = chunk->lights;
                }
            }
            chunk = chunk->next;
        }
    }
    
	ResetLightGrid(worldMode, true);
}

inline Lights GetLights(GameModeWorld* worldMode, Vec3 P)
{
    Lights result = {};
    GetUniversePosQuery query = TranslateRelativePos(worldMode, worldMode->player.universeP, P.xy);
    if(query.chunk)
    {
        result = query.chunk->lights;
    }
    
    return result;
}