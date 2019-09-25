enum AssetType
{
AssetType_Invalid,
AssetType_EntityDefinition,
AssetType_Font,
AssetType_Image,
AssetType_Model,
AssetType_Skeleton,
AssetType_Sound,
AssetType_tile_definition,
AssetType_WaterParams,
AssetType_world_generator,
AssetType_Count
};

char* metaAsset_assetType[] = 
{
"Invalid", 
"EntityDefinition",
"Font",
"Image",
"Model",
"Skeleton",
"Sound",
"tile_definition",
"WaterParams",
"world_generator",
};


enum AssetEntityDefinitionType
{
AssetEntityDefinition_default,
AssetEntityDefinition_Count
};

char* MetaTable_AssetEntityDefinitionType[] = {
"default",
};
enum AssetFontType
{
AssetFont_debug,
AssetFont_game,
AssetFont_Count
};

char* MetaTable_AssetFontType[] = {
"debug",
"game",
};
enum AssetImageType
{
AssetImage_crocodile,
AssetImage_default,
AssetImage_turtle,
AssetImage_wolf,
AssetImage_Count
};

char* MetaTable_AssetImageType[] = {
"crocodile",
"default",
"turtle",
"wolf",
};
enum AssetModelType
{
AssetModel_default,
AssetModel_Count
};

char* MetaTable_AssetModelType[] = {
"default",
};
enum AssetSkeletonType
{
AssetSkeleton_crocodile,
AssetSkeleton_default,
AssetSkeleton_turtle,
AssetSkeleton_wolf,
AssetSkeleton_Count
};

char* MetaTable_AssetSkeletonType[] = {
"crocodile",
"default",
"turtle",
"wolf",
};
enum AssetSoundType
{
AssetSound_bloop,
AssetSound_crack,
AssetSound_default,
AssetSound_drop,
AssetSound_exploration,
AssetSound_forest,
AssetSound_glide,
AssetSound_gliding,
AssetSound_puhp,
AssetSound_thunder,
AssetSound_Count
};

char* MetaTable_AssetSoundType[] = {
"bloop",
"crack",
"default",
"drop",
"exploration",
"forest",
"glide",
"gliding",
"puhp",
"thunder",
};
enum Assettile_definitionType
{
Assettile_definition_default,
Assettile_definition_Count
};

char* MetaTable_Assettile_definitionType[] = {
"default",
};
enum AssetWaterParamsType
{
AssetWaterParams_default,
AssetWaterParams_Count
};

char* MetaTable_AssetWaterParamsType[] = {
"default",
};
enum Assetworld_generatorType
{
Assetworld_generator_default,
Assetworld_generator_Count
};

char* MetaTable_Assetworld_generatorType[] = {
"default",
};
MetaAssetType metaAsset_subTypes[AssetType_Count] = 
{
{0, NULL},
{1, MetaTable_AssetEntityDefinitionType},
{2, MetaTable_AssetFontType},
{4, MetaTable_AssetImageType},
{1, MetaTable_AssetModelType},
{4, MetaTable_AssetSkeletonType},
{10, MetaTable_AssetSoundType},
{1, MetaTable_Assettile_definitionType},
{1, MetaTable_AssetWaterParamsType},
{1, MetaTable_Assetworld_generatorType},
};


