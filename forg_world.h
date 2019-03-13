#pragma once
struct EntityBlock
{
    u32 countEntity;
    
    u32 usedData;
    u8 data[KiloBytes(64)];
    
    union
    {
        EntityBlock* next;
        EntityBlock* nextFree;
    };
};

struct WorldChunk
{
    b32 initialized;
    
    i32 worldX;
    i32 worldY;
    i32 worldZ;
    
    
    u8 biomeSubChunks[4];
    r32 temperature;
    r32 dryness;
    r32 heights[CHUNK_DIM][CHUNK_DIM];
    r32 waterAmount[CHUNK_DIM][CHUNK_DIM];
    r32 influences[CHUNK_DIM][CHUNK_DIM];
    u8 biomes[CHUNK_DIM][CHUNK_DIM];
    Vec2 tileNormals[CHUNK_DIM][CHUNK_DIM];
    
#ifndef FORG_SERVER
    Vec4 colors[CHUNK_DIM][CHUNK_DIM];
    Vec4 subchunkColor[4];
    u8 lightCount[CHUNK_DIM][CHUNK_DIM];
    Vec4 lightIndexes[CHUNK_DIM][CHUNK_DIM];
#endif
    
    EntityBlock* entities;
    
    WorldChunk* next;
};

struct AddEntityAdditionalParams
{
    b32 dropped;
    u32 playerID;
    u64 ownerID;
    b32 equipped;
    u16 quantity;
    i16 status;
    
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
            u64 recipeIndex;
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


inline AddEntityAdditionalParams Crafting(u64 recipeIndex)
{
    AddEntityAdditionalParams result = {};
	result.status = I16_MAX;
    result.recipeIndex = recipeIndex;
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


inline AddEntityAdditionalParams RecipeObject(u32 recipeTaxonomy, u64 recipeIndex, i16 status)
{
    AddEntityAdditionalParams result = EntityFromObject(0, 1, status);
    result.recipeTaxonomy = recipeTaxonomy;
    result.recipeIndex = recipeIndex;
    
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


inline AddEntityAdditionalParams DroppedRecipeObject(u32 recipeTaxonomy, u64 recipeRecipeIndex, i16 status)
{
    AddEntityAdditionalParams result = RecipeObject(recipeTaxonomy, recipeRecipeIndex, status);
    result.dropped = true;
    
    return result;
}
