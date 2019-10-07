#pragma once
enum MetaType
{
    MetaType_None,
    MetaType_u8,
    MetaType_i8,
    MetaType_u16,
    MetaType_i16,
    MetaType_u32,
    MetaType_i32,
    MetaType_u64,
    MetaType_i64,
    MetaType_Vec2,
    MetaType_Vec3,
    MetaType_Vec4,
    MetaType_Color,
    MetaType_r32,
    MetaType_b32,
    MetaType_ArrayCounter, // NOTE(Leonardo): u16
    MetaType_Enumerator, // NOTE(Leonardo): u32
    MetaType_Hash64,
    MetaType_GameProperty,
    MetaType_GameAssetType,
    MetaType_AssetLabel,
    MetaType_EntityRef,
    MetaType_EntityKind,
    MetaType_EntityType,
    
    
    MetaType_FirstCustomMetaType = MetaType_GameAssetType,
    
    
    
    MetaType_GroundColorationArrayTest,
    MetaType_TileMapping,
    MetaType_tile_definition,
    MetaType_NoiseParams,
    MetaType_NoiseBucket,
    MetaType_MinMaxBucket,
    MetaType_PropertyBucket,
    MetaType_NoiseSelector,
    MetaType_MinMaxSelector,
    MetaType_PropertySelector,
    MetaType_BiomePyramid,
    MetaType_WaterPhase,
    MetaType_ServerEntityInitParams,
    MetaType_ClientEntityInitParams,
    MetaType_CommonEntityInitParams,
    MetaType_UniversePos,
    MetaType_EntityID,
    MetaType_RockDefinition,
    MetaType_RockMineral,
    MetaType_AssetID,
    MetaType_SpawnerOption,
    MetaType_Spawner,
    MetaType_ImageProperty,
};

enum FieldMetaFlags
{
    MetaFlag_Pointer = (1 << 0),
    MetaFlag_Hashed = (1 << 1),
    MetaFlag_Uneditable = (1 << 2),
};

union DefaultFieldValue
{
    u8 def_u8;
    i8 def_i8;
    u16 def_u16;
    u16 def_ArrayCounter;
    Enumerator def_Enumerator;
    Color def_Color;
    i16 def_i16;
    u32 def_u32;
    i32 def_i32;
    u64 def_u64;
    i64 def_i64;
    r32 def_r32;
    b32 def_b32;
    Vec2 def_Vec2;
    Vec3 def_Vec3;
    Vec4 def_Vec4;
    Hash64 def_Hash64;
    GameProperty def_GameProperty;
    GameAssetType def_GameAssetType;
    AssetLabel def_AssetLabel;
    EntityRef def_EntityRef;
};

struct FieldDefinition
{
    u32 flags;
    MetaType type;
    char* typeName;
    char* name;
    u32 offset;
    DefaultFieldValue def;
    u32 size;
    char* optionsName; // NOTE(Leonardo): used by the editor!
    char* counterName;
    u32 counterOffset;
    char* fixedField;
    
    char** options;
    u32 optionCount;
};

struct StructDefinition
{
    char name[64];
    u32 size;
    u32 fieldCount;
    FieldDefinition* fields;
};

struct MetaFlag
{
	char* name;
	u32 value;
};

FieldDefinition fieldDefinitionOfVec2[] = 
{
    {0, MetaType_r32, "r32", "x", (u32) (&((Vec2*)0)->x)}, 
    {0, MetaType_r32, "r32", "y", (u32) (&((Vec2*)0)->y)}, 
};

FieldDefinition fieldDefinitionOfVec3[] = 
{
    {0, MetaType_r32, "r32", "x", (u32) (&((Vec3*)0)->x)}, 
    {0, MetaType_r32, "r32", "y", (u32) (&((Vec3*)0)->y)}, 
    {0, MetaType_r32, "r32", "z", (u32) (&((Vec3*)0)->z)}, 
    
    {0, MetaType_r32, "r32", "r", (u32) (&((Vec3*)0)->x)}, 
    {0, MetaType_r32, "r32", "g", (u32) (&((Vec3*)0)->y)}, 
    {0, MetaType_r32, "r32", "b", (u32) (&((Vec3*)0)->z)}, 
};


FieldDefinition fieldDefinitionOfVec4[] = 
{
    {0, MetaType_r32, "r32", "x", (u32) (&((Vec4*)0)->x)}, 
    {0, MetaType_r32, "r32", "y", (u32) (&((Vec4*)0)->y)}, 
    {0, MetaType_r32, "r32", "z", (u32) (&((Vec4*)0)->z)}, 
    {0, MetaType_r32, "r32", "w", (u32) (&((Vec4*)0)->w)}, 
    
    {0, MetaType_r32, "r32", "r", (u32) (&((Vec4*)0)->x)}, 
    {0, MetaType_r32, "r32", "g", (u32) (&((Vec4*)0)->y)}, 
    {0, MetaType_r32, "r32", "b", (u32) (&((Vec4*)0)->z)}, 
    {0, MetaType_r32, "r32", "a", (u32) (&((Vec4*)0)->w)}, 
};

#define ConvertEnumerator(enum, v) (enum) GetValuePreprocessor_(MetaTable_##enum, ArrayCount(MetaTable_##enum), v.value)

#define GetValuePreprocessor(table, test) GetValuePreprocessor_(MetaTable_##table, ArrayCount(MetaTable_##table), test) 
inline u32 GetValuePreprocessor_(char** values, u32 count, char* test)
{
    u32 result = 0;
    if(test)
    {
        for(u32 valueIndex = 0; valueIndex < count; ++valueIndex)
        {
            if(StrEqual(test, values[valueIndex]))
            {
                result = valueIndex;
                break;
            }
        }
        
    }
    return result;
}


#define GetFlagPreprocessor(flagName, test) GetFlagPreprocessor_(MetaFlags_##flagName, ArrayCount(MetaFlags_##flagName), test)
inline u32 GetFlagPreprocessor_(MetaFlag* values, u32 count, char* test)
{
	u32 result = 0;
    
    for(u32 valueIndex = 0; valueIndex < count; ++valueIndex)
    {
		MetaFlag* flag = values + valueIndex;
        if(StrEqual(test, flag->name))
        {
            result = flag->value;
            break;
        }
    }
    
    return result;
}
