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

MetaAsset metaAsset_assetType[] = 
{
{"Invalid", 0},
{"Font", 0},
{"ground_coloration", 1},
{"Image", 2},
{"Model", 3},
{"Skeleton", 4},
{"Sound", 5},
};


enum AssetFontType
{
AssetFont_debug,
AssetFont_game,
AssetFont_Count
};

MetaAsset metaAsset_Font[] = {
{"debug", 0},
{"game", 1},
};
enum Assetground_colorationType
{
Assetground_coloration_default,
Assetground_coloration_Count
};

MetaAsset metaAsset_ground_coloration[] = {
{"default", 0},
};
enum AssetImageType
{
AssetImage_default,
AssetImage_Count
};

MetaAsset metaAsset_Image[] = {
{"default", 0},
};
enum AssetModelType
{
AssetModel_default,
AssetModel_Count
};

MetaAsset metaAsset_Model[] = {
{"default", 0},
};
enum AssetSkeletonType
{
AssetSkeleton_default,
AssetSkeleton_Count
};

MetaAsset metaAsset_Skeleton[] = {
{"default", 0},
};
enum AssetSoundType
{
AssetSound_default,
AssetSound_Count
};

MetaAsset metaAsset_Sound[] = {
{"default", 0},
};
MetaAssetType metaAsset_subTypes[AssetType_Count] = 
{
{0, NULL},
{2, metaAsset_Font},
{1, metaAsset_ground_coloration},
{1, metaAsset_Image},
{1, metaAsset_Model},
{1, metaAsset_Skeleton},
{1, metaAsset_Sound},
};


