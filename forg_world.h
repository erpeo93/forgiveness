#pragma once
struct EntityBlock
{
    u32 countEntity;
    u32 entityIDs[512];
    
    union
    {
        EntityBlock* next;
        EntityBlock* nextFree;
    };
};


struct WorldTile
{
    r32 height;
    r32 waterLevel;
    u32 taxonomy;
    
#ifndef FORG_SERVER
    r32 layoutNoise;
    
    r32 waterPhase;
    r32 waterSine;
    b32 movingNegative;
    RandomSequence waterSeq;
    r32 blueNoise;
    r32 alphaNoise;
    Lights lights;
    r32 chunkynessSame;
    r32 chunkynessOther;
    Vec4 baseColor;
    Vec4 colorDelta;
    r32 colorRandomness;
    Vec4 borderColor;
#endif
};

struct TempLight
{
    Vec3 P;
    Vec3 color;
    r32 strength;
    
    union
    {
        TempLight* next;
        TempLight* nextFree;
    };
};

struct WorldChunk
{
    b32 initialized;
    
    i32 worldX;
    i32 worldY;
    i32 worldZ;
    
#ifndef FORG_SERVER
    Lights lights;
#endif
    WorldTile tiles[CHUNK_DIM][CHUNK_DIM];
    
    TempLight* firstTempLight;
    
    EntityBlock* entities;
    WorldChunk* next;
};

inline WorldTile* GetTile(WorldChunk* chunk, u32 tileX, u32 tileY)
{
    Assert(tileY < CHUNK_DIM);
    Assert(tileX < CHUNK_DIM);
    
    WorldTile* result = &chunk->tiles[tileY][tileX];
    
    return result;
}

struct AddEntityAdditionalParams
{
    b32 dropped;
    u32 playerID;
    u64 ownerID;
    b32 equipped;
    u16 quantity;
    i16 status;
    
    Vec3 speed;
    r32 generationIntensity;
    r32 timeToLive;
    
    union
    {
        struct
        {
            u64 fluidId1;
            u64 fluidId2;
            u32 segmentIndex1;
            u32 segmentIndex2;
            m4x4 rotation;
        };
        
        struct
        {
            u32 recipeTaxonomy;
        };
    };
    
    u8 objectCount;
    Object objects[16];
};


inline AddEntityAdditionalParams DefaultAddEntityParams()
{
    AddEntityAdditionalParams result = {};
	result.status = I16_MAX;
    return result;
}


inline AddEntityAdditionalParams Crafting()
{
    AddEntityAdditionalParams result = {};
	result.status = I16_MAX;
    return result;
}

inline AddEntityAdditionalParams FluidDirection( m4x4 rotation, u64 id1, u32 segmentIndex1, u64 id2, u32 segmentIndex2 )
{
    AddEntityAdditionalParams result = DefaultAddEntityParams();
    result.rotation = rotation;
    result.fluidId1 = id1;
    result.segmentIndex1 = segmentIndex1;
    result.fluidId2 = id2;
    result.segmentIndex2 = segmentIndex2;
    return result;
}

inline AddEntityAdditionalParams FluidDirection( u64 id1, u32 segmentIndex1, u64 id2, u32 segmentIndex2 )
{
    AddEntityAdditionalParams result = DefaultAddEntityParams();
    result.fluidId1 = id1;
    result.segmentIndex1 = segmentIndex1;
    result.fluidId2 = id2;
    result.segmentIndex2 = segmentIndex2;
    return result;
}


inline AddEntityAdditionalParams PlayerAddEntityParams(u32 playerID)
{
    AddEntityAdditionalParams result = DefaultAddEntityParams();
    result.playerID = playerID;
    
    return result;
}

inline AddEntityAdditionalParams EntityFromObject(u64 ownerID, u16 quantity, i16 status)
{
    AddEntityAdditionalParams result = DefaultAddEntityParams();
    result.quantity = quantity;
    result.status = status;
    result.ownerID = ownerID;
    
    return result;
}


inline AddEntityAdditionalParams RecipeObject(u32 recipeTaxonomy, i16 status = I16_MAX)
{
    AddEntityAdditionalParams result = EntityFromObject(0, 1, status);
    result.recipeTaxonomy = recipeTaxonomy;
    
    return result;
}


inline AddEntityAdditionalParams Dropped(u16 quantity, i16 status, ContainedObjects* objects)
{
    AddEntityAdditionalParams result = EntityFromObject(0, quantity, status);
    result.dropped = true;
    if(objects)
    {
        Assert(objects->objectCount <= ArrayCount(result.objects));
        result.objectCount = objects->objectCount;
        
        for(u32 objectIndex = 0; objectIndex < objects->objectCount; ++objectIndex)
        {
            result.objects[objectIndex] = objects->objects[objectIndex];
        }
    }
    
    return result;
}


inline AddEntityAdditionalParams EquippedBy(u64 ownerID, u16 quantity, i16 status, ContainedObjects* objects)
{
    AddEntityAdditionalParams result = EntityFromObject(ownerID, quantity, status);
    result.equipped = true;
    if(objects)
    {
        Assert(objects->objectCount <= ArrayCount(result.objects));
        result.objectCount = objects->objectCount;
        
        for(u32 objectIndex = 0; objectIndex < objects->objectCount; ++objectIndex)
        {
            result.objects[objectIndex] = objects->objects[objectIndex];
        }
    }
    
    return result;
}

inline AddEntityAdditionalParams Incomplete(u64 owner, u16 quantity, i16 status)
{
    Assert(status < 0);
    AddEntityAdditionalParams result = EntityFromObject(owner, quantity, status);
    result.equipped = true;
    return result;
}


inline AddEntityAdditionalParams DroppedRecipeObject(u32 recipeTaxonomy, i16 status)
{
    AddEntityAdditionalParams result = RecipeObject(recipeTaxonomy, status);
    result.dropped = true;
    
    return result;
}
