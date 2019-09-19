enum AssetType
{
AssetType_Invalid,
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
"Font",
"Image",
"Model",
"Skeleton",
"Sound",
"tile_definition",
"WaterParams",
"world_generator",
};


enum AssetFontType
{
AssetFont_debug,
AssetFont_game,
AssetFont_Count
};

char* metaAsset_Font[] = {
"debug",
"game",
};
enum AssetImageType
{
AssetImage_default,
AssetImage_Count
};

char* metaAsset_Image[] = {
"default",
};
enum AssetModelType
{
AssetModel_default,
AssetModel_Count
};

char* metaAsset_Model[] = {
"default",
};
enum AssetSkeletonType
{
AssetSkeleton_default,
AssetSkeleton_Count
};

char* metaAsset_Skeleton[] = {
"default",
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

char* metaAsset_Sound[] = {
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

char* metaAsset_tile_definition[] = {
"default",
};
enum AssetWaterParamsType
{
AssetWaterParams_default,
AssetWaterParams_Count
};

char* metaAsset_WaterParams[] = {
"default",
};
enum Assetworld_generatorType
{
Assetworld_generator_default,
Assetworld_generator_Count
};

char* metaAsset_world_generator[] = {
"default",
};
MetaAssetType metaAsset_subTypes[AssetType_Count] = 
{
{0, NULL},
{2, metaAsset_Font},
{1, metaAsset_Image},
{1, metaAsset_Model},
{1, metaAsset_Skeleton},
{10, metaAsset_Sound},
{1, metaAsset_tile_definition},
{1, metaAsset_WaterParams},
{1, metaAsset_world_generator},
};


