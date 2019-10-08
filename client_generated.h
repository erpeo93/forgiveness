enum EntityArchetype
{Archetype_GrassArchetype,
Archetype_PlantArchetype,
Archetype_RockArchetype,
Archetype_AnimalArchetype,
Archetype_Count
};
char* MetaTable_EntityArchetype[] = 
{"GrassArchetype",
"PlantArchetype",
"RockArchetype",
"AnimalArchetype",
};
#define META_ARCHETYPES_INIT_FUNC()\
InitFunc[Archetype_GrassArchetype] = InitGrassArchetype;InitFunc[Archetype_PlantArchetype] = InitPlantArchetype;InitFunc[Archetype_RockArchetype] = InitRockArchetype;InitFunc[Archetype_AnimalArchetype] = InitAnimalArchetype;
;
char* MetaTable_tileType[] = 
 {
"rock",
"grass",
"dirt",
"sand",
};

char* MetaTable_action[] = 
 {
"idle",
"move",
"attack",
};

char* MetaTable_EffectIdentifier[] = 
 {
"Damage",
"FireDamage",
"NakedHandsDamage",
"Spawn",
"RestoreLifePoints",
"SpawnRock",
"SpawnTree",
"SpawnCreature",
"SpawnObject",
};

char* MetaTable_EffectTargetRangeType[] = 
 {
"Target",
"Actor",
"AllInActorRange",
"AllInTargetRange",
};

MetaFlag MetaFlags_EffectFlags[] = 
 {
{"Target", (1 << 1)},
{"ResetAfterTimer", (1 << 2)},
{"SendToClientOnTrigger", (1 << 3)},
{"SendToClientOnDelete", (1 << 4)},
};

MetaFlag MetaFlags_CanDoActionFlags[] = 
 {
{"Own", (1 << 1)},
{"EquipmentSlot", (1 << 2)},
{"Empty", (1 << 3)},
};

char* MetaTable_PlantShape[] = 
 {
"Conical",
"Spherical",
"Hemispherical",
"Cylindrical",
"TaperedCylindrical",
"Flame",
"InverseConical",
"TendFlame",
};

char* MetaTable_SlotName[] = 
 {
"None",
"Belly",
"BellySideLeft",
"BellySideRight",
"Leg",
"Shoulder",
"Back",
"RightHand",
"LeftHand",
"Count",
};

FieldDefinition fieldDefinitionOfWaterPhase[] = 
 {
{0, MetaType_r32, "r32", "referenceHeight", (u32) (&((WaterPhase*)0)->referenceHeight), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "heightSpeed", (u32) (&((WaterPhase*)0)->heightSpeed), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "heightSpeedV", (u32) (&((WaterPhase*)0)->heightSpeedV), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "runningTime", (u32) (&((WaterPhase*)0)->runningTime), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "referenceHeightV", (u32) (&((WaterPhase*)0)->referenceHeightV), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "currentHeight", (u32) (&((WaterPhase*)0)->currentHeight), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_Vec3, "Vec3", "minColor", (u32) (&((WaterPhase*)0)->minColor), {}, sizeof(Vec3),"invalid",0, 0, 0}, 
{0, MetaType_Vec3, "Vec3", "maxColor", (u32) (&((WaterPhase*)0)->maxColor), {}, sizeof(Vec3),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "maxAlpha", (u32) (&((WaterPhase*)0)->maxAlpha), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "minAlpha", (u32) (&((WaterPhase*)0)->minAlpha), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "maxColorDisplacement", (u32) (&((WaterPhase*)0)->maxColorDisplacement), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "maxAlphaDisplacement", (u32) (&((WaterPhase*)0)->maxAlphaDisplacement), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "colorSpeed", (u32) (&((WaterPhase*)0)->colorSpeed), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "colorSpeedV", (u32) (&((WaterPhase*)0)->colorSpeedV), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "sineWeight", (u32) (&((WaterPhase*)0)->sineWeight), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_NoiseParams, "NoiseParams", "noise", (u32) (&((WaterPhase*)0)->noise), {}, sizeof(NoiseParams),"invalid",0, 0, 0}, 
};

FieldDefinition fieldDefinitionOfWaterParams[] = 
 {
{0, MetaType_ArrayCounter, "ArrayCounter", "phaseCount", (u32) (&((WaterParams*)0)->phaseCount), {}, sizeof(ArrayCounter),"invalid","phases", (u32)(&((WaterParams*)0)->phases), 0}, 
{MetaFlag_Pointer, MetaType_WaterPhase, "WaterPhase", "phases", (u32) (&((WaterParams*)0)->phases), {}, sizeof(WaterPhase),"invalid",0, 0, 0}, 
};

FieldDefinition fieldDefinitionOfUniversePos[] = 
 {
{0, MetaType_i32, "i32", "chunkX", (u32) (&((UniversePos*)0)->chunkX), {}, sizeof(i32),"invalid",0, 0, 0}, 
{0, MetaType_i32, "i32", "chunkY", (u32) (&((UniversePos*)0)->chunkY), {}, sizeof(i32),"invalid",0, 0, 0}, 
{0, MetaType_Vec3, "Vec3", "chunkOffset", (u32) (&((UniversePos*)0)->chunkOffset), {}, sizeof(Vec3),"invalid",0, 0, 0}, 
};

FieldDefinition fieldDefinitionOfNoiseParams[] = 
 {
{0, MetaType_r32, "r32", "frequency", (u32) (&((NoiseParams*)0)->frequency), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_u32, "u32", "octaves", (u32) (&((NoiseParams*)0)->octaves), {}, sizeof(u32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "persistance", (u32) (&((NoiseParams*)0)->persistance), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "min", (u32) (&((NoiseParams*)0)->min), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "max", (u32) (&((NoiseParams*)0)->max), {}, sizeof(r32),"invalid",0, 0, 0}, 
};

FieldDefinition fieldDefinitionOfNoiseBucket[] = 
 {
{0, MetaType_AssetLabel, "AssetLabel", "name", (u32) (&((NoiseBucket*)0)->name), {}, sizeof(AssetLabel),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "referencePoint", (u32) (&((NoiseBucket*)0)->referencePoint), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_NoiseParams, "NoiseParams", "params", (u32) (&((NoiseBucket*)0)->params), {}, sizeof(NoiseParams),"invalid",0, 0, 0}, 
};

FieldDefinition fieldDefinitionOfPropertyBucket[] = 
 {
{0, MetaType_AssetLabel, "AssetLabel", "name", (u32) (&((PropertyBucket*)0)->name), {}, sizeof(AssetLabel),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "referencePoint", (u32) (&((PropertyBucket*)0)->referencePoint), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_GameProperty, "GameProperty", "property", (u32) (&((PropertyBucket*)0)->property), {}, sizeof(GameProperty),"invalid",0, 0, 0}, 
};

FieldDefinition fieldDefinitionOfNoiseSelector[] = 
 {
{0, MetaType_ArrayCounter, "ArrayCounter", "bucketCount", (u32) (&((NoiseSelector*)0)->bucketCount), {}, sizeof(ArrayCounter),"invalid","buckets", (u32)(&((NoiseSelector*)0)->buckets), 0}, 
{MetaFlag_Pointer, MetaType_NoiseBucket, "NoiseBucket", "buckets", (u32) (&((NoiseSelector*)0)->buckets), {}, sizeof(NoiseBucket),"invalid",0, 0, 0}, 
};

FieldDefinition fieldDefinitionOfPropertySelector[] = 
 {
{0, MetaType_ArrayCounter, "ArrayCounter", "bucketCount", (u32) (&((PropertySelector*)0)->bucketCount), {}, sizeof(ArrayCounter),"invalid","buckets", (u32)(&((PropertySelector*)0)->buckets), 0}, 
{MetaFlag_Pointer, MetaType_PropertyBucket, "PropertyBucket", "buckets", (u32) (&((PropertySelector*)0)->buckets), {}, sizeof(PropertyBucket),"invalid",0, 0, 0}, 
};

FieldDefinition fieldDefinitionOfBiomePyramid[] = 
 {
{0, MetaType_NoiseSelector, "NoiseSelector", "drySelector", (u32) (&((BiomePyramid*)0)->drySelector), {}, sizeof(NoiseSelector),"invalid",0, 0, 0}, 
{0, MetaType_ArrayCounter, "ArrayCounter", "rowCount", (u32) (&((BiomePyramid*)0)->rowCount), {}, sizeof(ArrayCounter),"invalid","temperatureSelectors", (u32)(&((BiomePyramid*)0)->temperatureSelectors), 0}, 
{MetaFlag_Pointer, MetaType_PropertySelector, "PropertySelector", "temperatureSelectors", (u32) (&((BiomePyramid*)0)->temperatureSelectors), {}, sizeof(PropertySelector),"invalid",0, 0, 0}, 
};

FieldDefinition fieldDefinitionOfworld_generator[] = 
 {
{0, MetaType_NoiseParams, "NoiseParams", "landscapeNoise", (u32) (&((world_generator*)0)->landscapeNoise), {}, sizeof(NoiseParams),"invalid",0, 0, 0}, 
{0, MetaType_NoiseSelector, "NoiseSelector", "landscapeSelect", (u32) (&((world_generator*)0)->landscapeSelect), {}, sizeof(NoiseSelector),"invalid",0, 0, 0}, 
{0, MetaType_NoiseParams, "NoiseParams", "temperatureNoise", (u32) (&((world_generator*)0)->temperatureNoise), {}, sizeof(NoiseParams),"invalid",0, 0, 0}, 
{0, MetaType_NoiseSelector, "NoiseSelector", "temperatureSelect", (u32) (&((world_generator*)0)->temperatureSelect), {}, sizeof(NoiseSelector),"invalid",0, 0, 0}, 
{0, MetaType_NoiseParams, "NoiseParams", "precipitationNoise", (u32) (&((world_generator*)0)->precipitationNoise), {}, sizeof(NoiseParams),"invalid",0, 0, 0}, 
{0, MetaType_NoiseParams, "NoiseParams", "elevationNoise", (u32) (&((world_generator*)0)->elevationNoise), {}, sizeof(NoiseParams),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "elevationPower", (u32) (&((world_generator*)0)->elevationPower), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "elevationNormOffset", (u32) (&((world_generator*)0)->elevationNormOffset), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "waterSafetyMargin", (u32) (&((world_generator*)0)->waterSafetyMargin), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_BiomePyramid, "BiomePyramid", "biomePyramid", (u32) (&((world_generator*)0)->biomePyramid), {}, sizeof(BiomePyramid),"invalid",0, 0, 0}, 
};

FieldDefinition fieldDefinitionOfEntityRef[] = 
 {
{0, MetaType_u32, "u32", "subtypeHashIndex", (u32) (&((EntityRef*)0)->subtypeHashIndex), {}, sizeof(u32),"invalid",0, 0, 0}, 
{0, MetaType_u16, "u16", "index", (u32) (&((EntityRef*)0)->index), {}, sizeof(u16),"invalid",0, 0, 0}, 
};

FieldDefinition fieldDefinitionOfSpawnerEntity[] = 
 {
{0, MetaType_EntityRef, "EntityRef", "type", (u32) (&((SpawnerEntity*)0)->type), {}, sizeof(EntityRef),"invalid",0, 0, 0}, 
{0, MetaType_Vec2, "Vec2", "maxClusterOffset", (u32) (&((SpawnerEntity*)0)->maxClusterOffset), {}, sizeof(Vec2),"invalid",0, 0, 0}, 
{0, MetaType_Vec2, "Vec2", "maxOffset", (u32) (&((SpawnerEntity*)0)->maxOffset), {}, sizeof(Vec2),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "minClusterDistance", (u32) (&((SpawnerEntity*)0)->minClusterDistance), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "minEntityDistance", (u32) (&((SpawnerEntity*)0)->minEntityDistance), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_i32, "i32", "clusterCount", (u32) (&((SpawnerEntity*)0)->clusterCount), {}, sizeof(i32),"invalid",0, 0, 0}, 
{0, MetaType_i32, "i32", "clusterCountV", (u32) (&((SpawnerEntity*)0)->clusterCountV), {}, sizeof(i32),"invalid",0, 0, 0}, 
{0, MetaType_i32, "i32", "count", (u32) (&((SpawnerEntity*)0)->count), {}, sizeof(i32),"invalid",0, 0, 0}, 
{0, MetaType_i32, "i32", "countV", (u32) (&((SpawnerEntity*)0)->countV), {}, sizeof(i32),"invalid",0, 0, 0}, 
};

FieldDefinition fieldDefinitionOfSpawnerOption[] = 
 {
{0, MetaType_r32, "r32", "weight", (u32) (&((SpawnerOption*)0)->weight), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_ArrayCounter, "ArrayCounter", "entityCount", (u32) (&((SpawnerOption*)0)->entityCount), {}, sizeof(ArrayCounter),"invalid","entities", (u32)(&((SpawnerOption*)0)->entities), 0}, 
{MetaFlag_Pointer, MetaType_SpawnerEntity, "SpawnerEntity", "entities", (u32) (&((SpawnerOption*)0)->entities), {}, sizeof(SpawnerEntity),"invalid",0, 0, 0}, 
};

FieldDefinition fieldDefinitionOfSpawner[] = 
 {
{MetaFlag_Uneditable, MetaType_r32, "r32", "time", (u32) (&((Spawner*)0)->time), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "targetTime", (u32) (&((Spawner*)0)->targetTime), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "cellDim", (u32) (&((Spawner*)0)->cellDim), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "percentageOfStartingCells", (u32) (&((Spawner*)0)->percentageOfStartingCells), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_ArrayCounter, "ArrayCounter", "optionCount", (u32) (&((Spawner*)0)->optionCount), {}, sizeof(ArrayCounter),"invalid","options", (u32)(&((Spawner*)0)->options), 0}, 
{MetaFlag_Pointer, MetaType_SpawnerOption, "SpawnerOption", "options", (u32) (&((Spawner*)0)->options), {}, sizeof(SpawnerOption),"invalid",0, 0, 0}, 
};

MetaFlag MetaFlags_EditorRole[] = 
 {
{"SoundDesigner", (1 << 1)},
{"Composer", (1 << 2)},
{"GameDesigner", (1 << 3)},
{"Writer", (1 << 4)},
{"Animator", (1 << 5)},
{"Artist", (1 << 6)},
{"WebDeveloper", (1 << 7)},
{"3DModeller", (1 << 8)},
};

char* MetaTable_ParticleUpdaterType[] = 
 {
"Standard",
"Sine",
"Count",
};

char* MetaTable_ParticleUpdaterEndPosition[] = 
 {
"FixedOffset",
"DestPos",
};

FieldDefinition fieldDefinitionOfAssetID[] = 
 {
{0, MetaType_u16, "u16", "type", (u32) (&((AssetID*)0)->type), {}, sizeof(u16),"invalid",0, 0, 0}, 
{0, MetaType_u32, "u32", "subtypeHashIndex", (u32) (&((AssetID*)0)->subtypeHashIndex), {}, sizeof(u32),"invalid",0, 0, 0}, 
{0, MetaType_u16, "u16", "index", (u32) (&((AssetID*)0)->index), {}, sizeof(u16),"invalid",0, 0, 0}, 
};

FieldDefinition fieldDefinitionOfGameAssetType[] = 
 {
{0, MetaType_u16, "u16", "type", (u32) (&((GameAssetType*)0)->type), {}, sizeof(u16),"invalid",0, 0, 0}, 
{0, MetaType_u64, "u64", "subtypeHash", (u32) (&((GameAssetType*)0)->subtypeHash), {}, sizeof(u64),"invalid",0, 0, 0}, 
};

FieldDefinition fieldDefinitionOfGroundColorationArrayTest[] = 
 {
{0, MetaType_u32, "u32", "p1", (u32) (&((GroundColorationArrayTest*)0)->p1), {}, sizeof(u32),"invalid",0, 0, 0}, 
{0, MetaType_u32, "u32", "p2", (u32) (&((GroundColorationArrayTest*)0)->p2), {}, sizeof(u32),"invalid",0, 0, 0}, 
{0, MetaType_GameProperty, "GameProperty", "property", (u32) (&((GroundColorationArrayTest*)0)->property), {}, sizeof(GameProperty),"invalid",0, 0, 0}, 
};

FieldDefinition fieldDefinitionOfground_coloration[] = 
 {
{0, MetaType_Vec4, "Vec4", "color", (u32) (&((ground_coloration*)0)->color), {}, sizeof(Vec4),"invalid",0, 0, 0}, 
{0, MetaType_ArrayCounter, "ArrayCounter", "testCounter", (u32) (&((ground_coloration*)0)->testCounter), {}, sizeof(ArrayCounter),"invalid","a1", (u32)(&((ground_coloration*)0)->a1), 0}, 
{MetaFlag_Pointer, MetaType_GroundColorationArrayTest, "GroundColorationArrayTest", "a1", (u32) (&((ground_coloration*)0)->a1), {}, sizeof(GroundColorationArrayTest),"invalid",0, 0, 0}, 
{0, MetaType_GameAssetType, "GameAssetType", "asset", (u32) (&((ground_coloration*)0)->asset), {}, sizeof(GameAssetType),"invalid",0, 0, "type"}, 
{0, MetaType_ArrayCounter, "ArrayCounter", "propertyCount", (u32) (&((ground_coloration*)0)->propertyCount), {}, sizeof(ArrayCounter),"invalid","properties", (u32)(&((ground_coloration*)0)->properties), 0}, 
{MetaFlag_Pointer, MetaType_GameProperty, "GameProperty", "properties", (u32) (&((ground_coloration*)0)->properties), {}, sizeof(GameProperty),"invalid",0, 0, 0}, 
};

FieldDefinition fieldDefinitionOftile_definition[] = 
 {
{0, MetaType_GameAssetType, "GameAssetType", "asset", (u32) (&((tile_definition*)0)->asset), {}, sizeof(GameAssetType),"invalid",0, 0, "type"}, 
{0, MetaType_GameProperty, "GameProperty", "property", (u32) (&((tile_definition*)0)->property), {}, sizeof(GameProperty),"invalid",0, 0, 0}, 
{0, MetaType_Vec4, "Vec4", "color", (u32) (&((tile_definition*)0)->color), {}, sizeof(Vec4),"invalid",0, 0, 0}, 
};

FieldDefinition fieldDefinitionOfTileMapping[] = 
 {
{0, MetaType_tile_definition, "tile_definition", "tile", (u32) (&((TileMapping*)0)->tile), {}, sizeof(tile_definition),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "weight", (u32) (&((TileMapping*)0)->weight), {}, sizeof(r32),"invalid",0, 0, 0}, 
};

FieldDefinition fieldDefinitionOfground_generator[] = 
 {
{0, MetaType_ArrayCounter, "ArrayCounter", "tileTypeCount", (u32) (&((ground_generator*)0)->tileTypeCount), {}, sizeof(ArrayCounter),"invalid","tiles", (u32)(&((ground_generator*)0)->tiles), 0}, 
{MetaFlag_Pointer, MetaType_TileMapping, "TileMapping", "tiles", (u32) (&((ground_generator*)0)->tiles), {}, sizeof(TileMapping),"invalid",0, 0, 0}, 
};

FieldDefinition fieldDefinitionOfEntityID[] = 
 {
{0, MetaType_u16, "u16", "archetype", (u32) (&((EntityID*)0)->archetype), {}, sizeof(u16),"invalid",0, 0, 0}, 
{0, MetaType_u32, "u32", "archetypeIndex", (u32) (&((EntityID*)0)->archetypeIndex), {}, sizeof(u32),"invalid",0, 0, 0}, 
};

FieldDefinition fieldDefinitionOfCommonEntityInitParams[] = 
 {
{0, MetaType_Vec3, "Vec3", "boundOffset", (u32) (&((CommonEntityInitParams*)0)->boundOffset), {}, sizeof(Vec3),"invalid",0, 0, 0}, 
{0, MetaType_Vec3, "Vec3", "boundDim", (u32) (&((CommonEntityInitParams*)0)->boundDim), {}, sizeof(Vec3),"invalid",0, 0, 0}, 
};

FieldDefinition fieldDefinitionOfServerEntityInitParams[] = 
 {
{MetaFlag_Uneditable, MetaType_UniversePos, "UniversePos", "P", (u32) (&((ServerEntityInitParams*)0)->P), {}, sizeof(UniversePos),"invalid",0, 0, 0}, 
{MetaFlag_Uneditable, MetaType_u32, "u32", "seed", (u32) (&((ServerEntityInitParams*)0)->seed), {}, sizeof(u32),"invalid",0, 0, 0}, 
{MetaFlag_Uneditable, MetaType_AssetID, "AssetID", "definitionID", (u32) (&((ServerEntityInitParams*)0)->definitionID), {}, sizeof(AssetID),"invalid",0, 0, 0}, 
};

FieldDefinition fieldDefinitionOfImageProperty[] = 
 {
{0, MetaType_b32, "b32", "optional", (u32) (&((ImageProperty*)0)->optional), {}, sizeof(b32),"invalid",0, 0, 0}, 
{0, MetaType_GameProperty, "GameProperty", "property", (u32) (&((ImageProperty*)0)->property), {}, sizeof(GameProperty),"invalid",0, 0, 0}, 
};

FieldDefinition fieldDefinitionOfClientEntityInitParams[] = 
 {
{MetaFlag_Uneditable, MetaType_EntityID, "EntityID", "ID", (u32) (&((ClientEntityInitParams*)0)->ID), {}, sizeof(EntityID),"invalid",0, 0, 0}, 
{MetaFlag_Uneditable, MetaType_u32, "u32", "seed", (u32) (&((ClientEntityInitParams*)0)->seed), {}, sizeof(u32),"invalid",0, 0, 0}, 
{0, MetaType_Enumerator, "Enumerator", "skeleton", (u32) (&((ClientEntityInitParams*)0)->skeleton), {}, sizeof(Enumerator),"invalid",0, 0, 0}, 
{0, MetaType_Enumerator, "Enumerator", "skin", (u32) (&((ClientEntityInitParams*)0)->skin), {}, sizeof(Enumerator),"invalid",0, 0, 0}, 
{0, MetaType_ArrayCounter, "ArrayCounter", "propertyCount", (u32) (&((ClientEntityInitParams*)0)->propertyCount), {}, sizeof(ArrayCounter),"invalid","properties", (u32)(&((ClientEntityInitParams*)0)->properties), 0}, 
{MetaFlag_Pointer, MetaType_ImageProperty, "ImageProperty", "properties", (u32) (&((ClientEntityInitParams*)0)->properties), {}, sizeof(ImageProperty),"invalid",0, 0, 0}, 
};

FieldDefinition fieldDefinitionOfEntityDefinition[] = 
 {
{0, MetaType_Enumerator, "Enumerator", "archetype", (u32) (&((EntityDefinition*)0)->archetype), {}, sizeof(Enumerator),"invalid",0, 0, 0, MetaTable_EntityArchetype, ArrayCount(MetaTable_EntityArchetype)}, 
{0, MetaType_CommonEntityInitParams, "CommonEntityInitParams", "common", (u32) (&((EntityDefinition*)0)->common), {}, sizeof(CommonEntityInitParams),"invalid",0, 0, 0}, 
{0, MetaType_ServerEntityInitParams, "ServerEntityInitParams", "server", (u32) (&((EntityDefinition*)0)->server), {}, sizeof(ServerEntityInitParams),"invalid",0, 0, 0}, 
{0, MetaType_ClientEntityInitParams, "ClientEntityInitParams", "client", (u32) (&((EntityDefinition*)0)->client), {}, sizeof(ClientEntityInitParams),"invalid",0, 0, 0}, 
};

FieldDefinition fieldDefinitionOfRockMineral[] = 
 {
{0, MetaType_r32, "r32", "lerp", (u32) (&((RockMineral*)0)->lerp), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "lerpDelta", (u32) (&((RockMineral*)0)->lerpDelta), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_Vec4, "Vec4", "color", (u32) (&((RockMineral*)0)->color), {}, sizeof(Vec4),"invalid",0, 0, 0}, 
};

FieldDefinition fieldDefinitionOfRockDefinition[] = 
 {
{0, MetaType_b32, "b32", "collides", (u32) (&((RockDefinition*)0)->collides), {}, sizeof(b32),"invalid",0, 0, 0}, 
{0, MetaType_Vec4, "Vec4", "color", (u32) (&((RockDefinition*)0)->color), {}, sizeof(Vec4),"invalid",0, 0, 0}, 
{0, MetaType_Vec4, "Vec4", "startingColorDelta", (u32) (&((RockDefinition*)0)->startingColorDelta), {}, sizeof(Vec4),"invalid",0, 0, 0}, 
{0, MetaType_Vec4, "Vec4", "perVertexColorDelta", (u32) (&((RockDefinition*)0)->perVertexColorDelta), {}, sizeof(Vec4),"invalid",0, 0, 0}, 
{0, MetaType_Vec3, "Vec3", "scale", (u32) (&((RockDefinition*)0)->scale), {}, sizeof(Vec3),"invalid",0, 0, 0}, 
{0, MetaType_Vec3, "Vec3", "scaleDelta", (u32) (&((RockDefinition*)0)->scaleDelta), {}, sizeof(Vec3),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "smoothness", (u32) (&((RockDefinition*)0)->smoothness), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "smoothnessDelta", (u32) (&((RockDefinition*)0)->smoothnessDelta), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "normalSmoothness", (u32) (&((RockDefinition*)0)->normalSmoothness), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_u32, "u32", "iterationCount", (u32) (&((RockDefinition*)0)->iterationCount), {}, sizeof(u32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "minDisplacementY", (u32) (&((RockDefinition*)0)->minDisplacementY), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "maxDisplacementY", (u32) (&((RockDefinition*)0)->maxDisplacementY), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "minDisplacementZ", (u32) (&((RockDefinition*)0)->minDisplacementZ), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "maxDisplacementZ", (u32) (&((RockDefinition*)0)->maxDisplacementZ), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "percentageOfMineralVertexes", (u32) (&((RockDefinition*)0)->percentageOfMineralVertexes), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_ArrayCounter, "ArrayCounter", "mineralCount", (u32) (&((RockDefinition*)0)->mineralCount), {}, sizeof(ArrayCounter),"invalid","minerals", (u32)(&((RockDefinition*)0)->minerals), 0}, 
{MetaFlag_Pointer, MetaType_RockMineral, "RockMineral", "minerals", (u32) (&((RockDefinition*)0)->minerals), {}, sizeof(RockMineral),"invalid",0, 0, 0}, 
{0, MetaType_u32, "u32", "renderingRocksCount", (u32) (&((RockDefinition*)0)->renderingRocksCount), {}, sizeof(u32),"invalid",0, 0, 0}, 
{0, MetaType_u32, "u32", "renderingRocksDelta", (u32) (&((RockDefinition*)0)->renderingRocksDelta), {}, sizeof(u32),"invalid",0, 0, 0}, 
{0, MetaType_Vec3, "Vec3", "renderingRocksRandomOffset", (u32) (&((RockDefinition*)0)->renderingRocksRandomOffset), {}, sizeof(Vec3),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "scaleRandomness", (u32) (&((RockDefinition*)0)->scaleRandomness), {}, sizeof(r32),"invalid",0, 0, 0}, 
};

#define META_HANDLE_ADD_TO_DEFINITION_HASH()\
AddToMetaDefinitions(RockDefinition, fieldDefinitionOfRockDefinition);\
AddToMetaDefinitions(RockMineral, fieldDefinitionOfRockMineral);\
AddToMetaDefinitions(EntityDefinition, fieldDefinitionOfEntityDefinition);\
AddToMetaDefinitions(ClientEntityInitParams, fieldDefinitionOfClientEntityInitParams);\
AddToMetaDefinitions(ImageProperty, fieldDefinitionOfImageProperty);\
AddToMetaDefinitions(ServerEntityInitParams, fieldDefinitionOfServerEntityInitParams);\
AddToMetaDefinitions(CommonEntityInitParams, fieldDefinitionOfCommonEntityInitParams);\
AddToMetaDefinitions(EntityID, fieldDefinitionOfEntityID);\
AddToMetaDefinitions(ground_generator, fieldDefinitionOfground_generator);\
AddToMetaDefinitions(TileMapping, fieldDefinitionOfTileMapping);\
AddToMetaDefinitions(tile_definition, fieldDefinitionOftile_definition);\
AddToMetaDefinitions(ground_coloration, fieldDefinitionOfground_coloration);\
AddToMetaDefinitions(GroundColorationArrayTest, fieldDefinitionOfGroundColorationArrayTest);\
AddToMetaDefinitions(GameAssetType, fieldDefinitionOfGameAssetType);\
AddToMetaDefinitions(AssetID, fieldDefinitionOfAssetID);\
AddToMetaDefinitions(Spawner, fieldDefinitionOfSpawner);\
AddToMetaDefinitions(SpawnerOption, fieldDefinitionOfSpawnerOption);\
AddToMetaDefinitions(SpawnerEntity, fieldDefinitionOfSpawnerEntity);\
AddToMetaDefinitions(EntityRef, fieldDefinitionOfEntityRef);\
AddToMetaDefinitions(world_generator, fieldDefinitionOfworld_generator);\
AddToMetaDefinitions(BiomePyramid, fieldDefinitionOfBiomePyramid);\
AddToMetaDefinitions(PropertySelector, fieldDefinitionOfPropertySelector);\
AddToMetaDefinitions(NoiseSelector, fieldDefinitionOfNoiseSelector);\
AddToMetaDefinitions(PropertyBucket, fieldDefinitionOfPropertyBucket);\
AddToMetaDefinitions(NoiseBucket, fieldDefinitionOfNoiseBucket);\
AddToMetaDefinitions(NoiseParams, fieldDefinitionOfNoiseParams);\
AddToMetaDefinitions(UniversePos, fieldDefinitionOfUniversePos);\
AddToMetaDefinitions(WaterParams, fieldDefinitionOfWaterParams);\
AddToMetaDefinitions(WaterPhase, fieldDefinitionOfWaterPhase);

#define META_PROPERTIES_ADD()\
AddToMetaProperties(action, MetaTable_action);\
AddToMetaProperties(tileType, MetaTable_tileType);

enum PropertyType
{
Property_Invalid,
Property_action,
Property_tileType,
Property_Count,
};
#define META_ASSET_PROPERTIES_STRINGS()\
meta_propertiesString[Property_action] = "action";\
meta_propertiesString[Property_tileType] = "tileType";\

#define META_DEFAULT_VALUES_CPP_SUCKS()\
fieldDefinitionOfWaterPhase[0].def.def_r32 =0;fieldDefinitionOfWaterPhase[6].def.def_Vec3 =V3(0, 0, 1);fieldDefinitionOfWaterPhase[7].def.def_Vec3 =V3(0, 0, 1);fieldDefinitionOfWaterPhase[8].def.def_r32 =1.0f;fieldDefinitionOfWaterPhase[9].def.def_r32 =0.5f;fieldDefinitionOfNoiseParams[0].def.def_r32 =1;fieldDefinitionOfNoiseParams[1].def.def_u32 =1;fieldDefinitionOfNoiseParams[3].def.def_r32 =0;fieldDefinitionOfNoiseParams[4].def.def_r32 =1;fieldDefinitionOfworld_generator[6].def.def_r32 =1;fieldDefinitionOfworld_generator[7].def.def_r32 =1;fieldDefinitionOfworld_generator[8].def.def_r32 =0.05f;fieldDefinitionOfSpawnerEntity[3].def.def_r32 =1;fieldDefinitionOfSpawnerEntity[4].def.def_r32 =1;fieldDefinitionOfSpawnerEntity[5].def.def_i32 =1;fieldDefinitionOfSpawnerEntity[6].def.def_i32 =0;fieldDefinitionOfSpawnerEntity[7].def.def_i32 =1;fieldDefinitionOfSpawnerOption[0].def.def_r32 =1.0f;fieldDefinitionOfSpawner[2].def.def_r32 =1.0f;fieldDefinitionOfSpawner[3].def.def_r32 =1.0f;fieldDefinitionOfGroundColorationArrayTest[0].def.def_u32 =2;fieldDefinitionOfGroundColorationArrayTest[1].def.def_u32 =3;fieldDefinitionOfground_coloration[0].def.def_Vec4 =V4(1, 0, 1, 1);fieldDefinitionOfground_coloration[3].def.def_GameAssetType ={AssetType_Font, 0};fieldDefinitionOftile_definition[0].def.def_GameAssetType ={AssetType_Image, 0};fieldDefinitionOftile_definition[2].def.def_Vec4 =V4(1, 1, 1, 1);fieldDefinitionOfCommonEntityInitParams[1].def.def_Vec3 =V3(1, 1, 1);fieldDefinitionOfRockDefinition[1].def.def_Vec4 =V4(1, 1, 1, 1);fieldDefinitionOfRockDefinition[4].def.def_Vec3 =V3(1, 1, 1);fieldDefinitionOfRockDefinition[6].def.def_r32 =0.5f;fieldDefinitionOfRockDefinition[9].def.def_u32 =1;fieldDefinitionOfRockDefinition[10].def.def_r32 =0.5f;fieldDefinitionOfRockDefinition[11].def.def_r32 =0.5f;fieldDefinitionOfRockDefinition[12].def.def_r32 =0.6f;fieldDefinitionOfRockDefinition[13].def.def_r32 =0.6f;fieldDefinitionOfRockDefinition[17].def.def_u32 =1;
;
#define META_ARCHETYPES_BOTH()\
archetypeLayouts[Archetype_AnimalArchetype].totalSize = sizeof(AnimalArchetype); archetypeLayouts[Archetype_RockArchetype].totalSize = sizeof(RockArchetype); archetypeLayouts[Archetype_PlantArchetype].totalSize = sizeof(PlantArchetype); archetypeLayouts[Archetype_GrassArchetype].totalSize = sizeof(GrassArchetype); 
;
#define META_ARCHETYPES_SERVER()\
archetypeLayouts[Archetype_AnimalArchetype].hasPhysicComponent.exists = true; archetypeLayouts[Archetype_AnimalArchetype].hasPhysicComponent.offset = OffsetOf(AnimalArchetype, physic);  archetypeLayouts[Archetype_AnimalArchetype].hasPlayerComponent.exists = true; archetypeLayouts[Archetype_AnimalArchetype].hasPlayerComponent.pointer = true; archetypeLayouts[Archetype_AnimalArchetype].hasPlayerComponent.offset = OffsetOf(AnimalArchetype, player);  archetypeLayouts[Archetype_RockArchetype].hasPhysicComponent.exists = true; archetypeLayouts[Archetype_RockArchetype].hasPhysicComponent.offset = OffsetOf(RockArchetype, physic);  archetypeLayouts[Archetype_PlantArchetype].hasPhysicComponent.exists = true; archetypeLayouts[Archetype_PlantArchetype].hasPhysicComponent.offset = OffsetOf(PlantArchetype, physic);  archetypeLayouts[Archetype_PlantArchetype].hasPlayerComponent.exists = true; archetypeLayouts[Archetype_PlantArchetype].hasPlayerComponent.pointer = true; archetypeLayouts[Archetype_PlantArchetype].hasPlayerComponent.offset = OffsetOf(PlantArchetype, player);  archetypeLayouts[Archetype_GrassArchetype].hasPhysicComponent.exists = true; archetypeLayouts[Archetype_GrassArchetype].hasPhysicComponent.offset = OffsetOf(GrassArchetype, physic);  archetypeLayouts[Archetype_GrassArchetype].hasPlayerComponent.exists = true; archetypeLayouts[Archetype_GrassArchetype].hasPlayerComponent.pointer = true; archetypeLayouts[Archetype_GrassArchetype].hasPlayerComponent.offset = OffsetOf(GrassArchetype, player);  
;
#define META_ARCHETYPES_CLIENT()\
archetypeLayouts[Archetype_AnimalArchetype].hasBaseComponent.exists = true; archetypeLayouts[Archetype_AnimalArchetype].hasBaseComponent.offset = OffsetOf(AnimalArchetype, base);  archetypeLayouts[Archetype_AnimalArchetype].hasAnimationComponent.exists = true; archetypeLayouts[Archetype_AnimalArchetype].hasAnimationComponent.offset = OffsetOf(AnimalArchetype, animation);  archetypeLayouts[Archetype_RockArchetype].hasBaseComponent.exists = true; archetypeLayouts[Archetype_RockArchetype].hasBaseComponent.offset = OffsetOf(RockArchetype, base);  archetypeLayouts[Archetype_RockArchetype].hasRockComponent.exists = true; archetypeLayouts[Archetype_RockArchetype].hasRockComponent.offset = OffsetOf(RockArchetype, rock);  archetypeLayouts[Archetype_RockArchetype].hasImageComponent.exists = true; archetypeLayouts[Archetype_RockArchetype].hasImageComponent.offset = OffsetOf(RockArchetype, image);  archetypeLayouts[Archetype_PlantArchetype].hasBaseComponent.exists = true; archetypeLayouts[Archetype_PlantArchetype].hasBaseComponent.offset = OffsetOf(PlantArchetype, base);  archetypeLayouts[Archetype_PlantArchetype].hasPlantComponent.exists = true; archetypeLayouts[Archetype_PlantArchetype].hasPlantComponent.offset = OffsetOf(PlantArchetype, rock);  archetypeLayouts[Archetype_PlantArchetype].hasImageComponent.exists = true; archetypeLayouts[Archetype_PlantArchetype].hasImageComponent.offset = OffsetOf(PlantArchetype, image);  archetypeLayouts[Archetype_GrassArchetype].hasBaseComponent.exists = true; archetypeLayouts[Archetype_GrassArchetype].hasBaseComponent.offset = OffsetOf(GrassArchetype, base);  archetypeLayouts[Archetype_GrassArchetype].hasGrassComponent.exists = true; archetypeLayouts[Archetype_GrassArchetype].hasGrassComponent.offset = OffsetOf(GrassArchetype, rock);  archetypeLayouts[Archetype_GrassArchetype].hasImageComponent.exists = true; archetypeLayouts[Archetype_GrassArchetype].hasImageComponent.offset = OffsetOf(GrassArchetype, image);  
;
