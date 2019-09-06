enum AssetType
{
AssetType_Invalid,
AssetType_Font,
AssetType_ground_coloration,
AssetType_Image,
AssetType_Model,
AssetType_Skeleton,
AssetType_Sound,
AssetType_Count
};

char* metaAsset_assetType[] = 
{
"Invalid", 
"Font",
"ground_coloration",
"Image",
"Model",
"Skeleton",
"Sound",
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
enum Assetground_colorationType
{
Assetground_coloration_default,
Assetground_coloration_Count
};

char* metaAsset_ground_coloration[] = {
"default",
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
MetaAssetType metaAsset_subTypes[AssetType_Count] = 
{
{0, NULL},
{2, metaAsset_Font},
{1, metaAsset_ground_coloration},
{1, metaAsset_Image},
{1, metaAsset_Model},
{1, metaAsset_Skeleton},
{10, metaAsset_Sound},
};


