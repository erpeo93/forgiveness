enum EntityArchetype
{Archetype_NullArchetype,
Archetype_PlaceholderArchetype,
Archetype_LightArchetype,
Archetype_ProjectileArchetype,
Archetype_PortalArchetype,
Archetype_ObjectArchetype,
Archetype_EssenceArchetype,
Archetype_RuneArchetype,
Archetype_GrassArchetype,
Archetype_PlantArchetype,
Archetype_RockArchetype,
Archetype_AnimalArchetype,
Archetype_Count
};
char* MetaTable_EntityArchetype[] = 
{"NullArchetype",
"PlaceholderArchetype",
"LightArchetype",
"ProjectileArchetype",
"PortalArchetype",
"ObjectArchetype",
"EssenceArchetype",
"RuneArchetype",
"GrassArchetype",
"PlantArchetype",
"RockArchetype",
"AnimalArchetype",
};

char* MetaTable_tileType[] = 
 {
"tile_invalid",
"rock",
"grass",
"dirt",
"sand",
};

char* MetaTable_fluid[] = 
 {
"fluid_invalid",
"ocean",
"snow",
"ice",
"lava",
"mud",
};

char* MetaTable_action[] = 
 {
"none",
"idle",
"move",
"attack",
"pick",
"mine",
"chop",
"harvest",
"touch",
"sacrifice",
"use",
"equip",
"disequip",
"open",
"drop",
"storeInventory",
"useInventory",
"drink",
"cast",
"drag",
"setOnFire",
"absorb",
"craft",
"level_up",
"selectSkill",
};

char* MetaTable_essence[] = 
 {
"shadow",
"light",
"chaos",
"strength",
"quickness",
"resistance",
"will",
"perception",
"control",
};

char* MetaTable_gameEffect[] = 
 {
"invalid_game_effect",
"spawnEntity",
"spawnRecipe",
"spawnProjectileTowardTarget",
"spawnProjectileTowardDirection",
"moveOnZSlice",
"deleteTarget",
"deleteSelf",
"damagePhysically",
"damageMentally",
"addSkillPoint",
"lightRadious",
"dropFlowers",
"dropFruits",
"sacrificeRandomEssence",
"restorePhysicalHealth",
"giveEssences",
};

char* MetaTable_animationEffect[] = 
 {
"addLight",
"tintWithColor",
};

char* MetaTable_equipmentSlot[] = 
 {
"legs",
"belly_right",
"belly_left",
"head",
};

char* MetaTable_usingSlot[] = 
 {
"leftHand",
"rightHand",
"skill_1",
"skill_2",
"skill_3",
"skill_4",
};

char* MetaTable_layoutType[] = 
 {
"onGround",
"onGroundOpen",
"equipped",
"equippedOpen",
};

char* MetaTable_inventorySlotType[] = 
 {
"InventorySlot_Invalid",
"InventorySlot_Generic",
"InventorySlot_Standard",
"InventorySlot_Skill",
"InventorySlot_Gem",
"InventorySlot_Special",
};

char* MetaTable_brainType[] = 
 {
"Brain_invalid",
"Brain_Portal",
"Brain_Player",
"Brain_stateMachine",
};

char* MetaTable_weather[] = 
 {
"Weather_rain",
"Weather_count",
};

char* MetaTable_season[] = 
 {
"Season_Spring",
"Season_Summer",
"Season_Autumn",
"Season_Winter",
};

char* MetaTable_dayTime[] = 
 {
"DayTime_Day",
"DayTime_Night",
};

char* MetaTable_frameIndex[] = 
 {
"FrameIndex_1",
"FrameIndex_2",
"FrameIndex_3",
"FrameIndex_4",
"FrameIndex_5",
"FrameIndex_6",
"FrameIndex_7",
"FrameIndex_8",
"FrameIndex_9",
"FrameIndex_10",
"FrameIndex_11",
"FrameIndex_12",
"FrameIndex_13",
"FrameIndex_14",
"FrameIndex_15",
"FrameIndex_16",
"FrameIndex_17",
"FrameIndex_18",
"FrameIndex_19",
"FrameIndex_20",
"FrameIndex_21",
"FrameIndex_22",
"FrameIndex_23",
"FrameIndex_24",
"FrameIndex_25",
"FrameIndex_26",
"FrameIndex_27",
"FrameIndex_28",
"FrameIndex_29",
"FrameIndex_30",
"FrameIndex_31",
};

char* MetaTable_soundTriggerType[] = 
 {
"soundTrigger_test1",
"soundTrigger_test2",
};

char* MetaTable_specialPropertyType[] = 
 {
"Special_Invalid",
"Special_AttackDistance",
};

char* MetaTable_boundType[] = 
 {
"bound_invalid",
"bound_creature",
"bound_environment",
"bound_object",
};

char* MetaTable_fenotype[] = 
 {
"Fenotype_1",
"Fenotype_2",
"Fenotype_3",
"Fenotype_4",
"Fenotype_5",
"Fenotype_6",
"Fenotype_7",
"Fenotype_8",
"Fenotype_9",
"Fenotype_10",
"Fenotype_11",
"Fenotype_12",
"Fenotype_13",
"Fenotype_14",
"Fenotype_15",
"Fenotype_16",
};

char* MetaTable_variant[] = 
 {
"Variant_1",
"Variant_2",
"Variant_3",
"Variant_4",
"Variant_5",
"Variant_6",
"Variant_7",
"Variant_8",
"Variant_9",
"Variant_10",
"Variant_11",
"Variant_12",
"Variant_13",
"Variant_14",
"Variant_15",
"Variant_16",
};

FieldDefinition fieldDefinitionOfGameEffectInstance[] = 
 {
{0, MetaType_r32, "r32", "timer", (u32) (&((GameEffectInstance*)0)->timer), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_b32, "b32", "targetEffect", (u32) (&((GameEffectInstance*)0)->targetEffect), {}, sizeof(b32),"invalid",0, 0, 0}, 
{0, MetaType_u16, "u16", "action", (u32) (&((GameEffectInstance*)0)->action), {}, sizeof(u16),"invalid",0, 0, 0}, 
{0, MetaType_u16, "u16", "type", (u32) (&((GameEffectInstance*)0)->type), {}, sizeof(u16),"invalid",0, 0, 0}, 
{0, MetaType_EntityType, "EntityType", "spawnType", (u32) (&((GameEffectInstance*)0)->spawnType), {}, sizeof(EntityType),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "power", (u32) (&((GameEffectInstance*)0)->power), {}, sizeof(r32),"invalid",0, 0, 0}, 
};

FieldDefinition fieldDefinitionOfGameEffect[] = 
 {
{0, MetaType_r32, "r32", "timer", (u32) (&((GameEffect*)0)->timer), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_b32, "b32", "targetEffect", (u32) (&((GameEffect*)0)->targetEffect), {}, sizeof(b32),"invalid",0, 0, 0}, 
{0, MetaType_GameProperty, "GameProperty", "action", (u32) (&((GameEffect*)0)->action), {}, sizeof(GameProperty),"invalid",0, 0, "property"}, 
{0, MetaType_GameProperty, "GameProperty", "effectType", (u32) (&((GameEffect*)0)->effectType), {}, sizeof(GameProperty),"invalid",0, 0, "property"}, 
{0, MetaType_EntityName, "EntityName", "spawnType", (u32) (&((GameEffect*)0)->spawnType), {}, sizeof(EntityName),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "power", (u32) (&((GameEffect*)0)->power), {}, sizeof(r32),"invalid",0, 0, 0}, 
};

FieldDefinition fieldDefinitionOfEffectBinding[] = 
 {
{0, MetaType_GameProperty, "GameProperty", "property", (u32) (&((EffectBinding*)0)->property), {}, sizeof(GameProperty),"invalid",0, 0, 0}, 
{0, MetaType_GameEffect, "GameEffect", "effect", (u32) (&((EffectBinding*)0)->effect), {}, sizeof(GameEffect),"invalid",0, 0, 0}, 
};

FieldDefinition fieldDefinitionOfProbabilityEffectOption[] = 
 {
{0, MetaType_r32, "r32", "weight", (u32) (&((ProbabilityEffectOption*)0)->weight), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_GameEffect, "GameEffect", "effect", (u32) (&((ProbabilityEffectOption*)0)->effect), {}, sizeof(GameEffect),"invalid",0, 0, 0}, 
};

FieldDefinition fieldDefinitionOfProbabilityEffect[] = 
 {
{0, MetaType_r32, "r32", "probability", (u32) (&((ProbabilityEffect*)0)->probability), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_ArrayCounter, "ArrayCounter", "optionCount", (u32) (&((ProbabilityEffect*)0)->optionCount), {}, sizeof(ArrayCounter),"invalid","options", (u32)(&((ProbabilityEffect*)0)->options), 0}, 
{MetaFlag_Pointer, MetaType_ProbabilityEffectOption, "ProbabilityEffectOption", "options", (u32) (&((ProbabilityEffect*)0)->options), {}, sizeof(ProbabilityEffectOption),"invalid",0, 0, 0}, 
};

FieldDefinition fieldDefinitionOfUniversePos[] = 
 {
{0, MetaType_i16, "i16", "chunkX", (u32) (&((UniversePos*)0)->chunkX), {}, sizeof(i16),"invalid",0, 0, 0}, 
{0, MetaType_i16, "i16", "chunkY", (u32) (&((UniversePos*)0)->chunkY), {}, sizeof(i16),"invalid",0, 0, 0}, 
{0, MetaType_i16, "i16", "chunkZ", (u32) (&((UniversePos*)0)->chunkZ), {}, sizeof(i16),"invalid",0, 0, 0}, 
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

FieldDefinition fieldDefinitionOfDrynessSelector[] = 
 {
{0, MetaType_NoiseSelector, "NoiseSelector", "drynessSelector", (u32) (&((DrynessSelector*)0)->drynessSelector), {}, sizeof(NoiseSelector),"invalid",0, 0, 0}, 
{0, MetaType_ArrayCounter, "ArrayCounter", "rowCount", (u32) (&((DrynessSelector*)0)->rowCount), {}, sizeof(ArrayCounter),"invalid","temperatureSelectors", (u32)(&((DrynessSelector*)0)->temperatureSelectors), 0}, 
{MetaFlag_Pointer, MetaType_PropertySelector, "PropertySelector", "temperatureSelectors", (u32) (&((DrynessSelector*)0)->temperatureSelectors), {}, sizeof(PropertySelector),"invalid",0, 0, 0}, 
};

FieldDefinition fieldDefinitionOfBiomePyramid[] = 
 {
{0, MetaType_NoiseSelector, "NoiseSelector", "darknessSelector", (u32) (&((BiomePyramid*)0)->darknessSelector), {}, sizeof(NoiseSelector),"invalid",0, 0, 0}, 
{0, MetaType_ArrayCounter, "ArrayCounter", "drynessCount", (u32) (&((BiomePyramid*)0)->drynessCount), {}, sizeof(ArrayCounter),"invalid","drynessSelectors", (u32)(&((BiomePyramid*)0)->drynessSelectors), 0}, 
{MetaFlag_Pointer, MetaType_DrynessSelector, "DrynessSelector", "drynessSelectors", (u32) (&((BiomePyramid*)0)->drynessSelectors), {}, sizeof(DrynessSelector),"invalid",0, 0, 0}, 
};

FieldDefinition fieldDefinitionOfZSlice[] = 
 {
{0, MetaType_r32, "r32", "referenceZ", (u32) (&((ZSlice*)0)->referenceZ), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_NoiseParams, "NoiseParams", "precipitationNoise", (u32) (&((ZSlice*)0)->precipitationNoise), {}, sizeof(NoiseParams),"invalid",0, 0, 0}, 
{0, MetaType_NoiseParams, "NoiseParams", "darknessNoise", (u32) (&((ZSlice*)0)->darknessNoise), {}, sizeof(NoiseParams),"invalid",0, 0, 0}, 
};

FieldDefinition fieldDefinitionOfBiomeBand[] = 
 {
{0, MetaType_r32, "r32", "referenceHeight", (u32) (&((BiomeBand*)0)->referenceHeight), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_GameProperty, "GameProperty", "tile", (u32) (&((BiomeBand*)0)->tile), {}, sizeof(GameProperty),"invalid",0, 0, "property"}, 
};

FieldDefinition fieldDefinitionOfBiomeConfiguration[] = 
 {
{0, MetaType_ArrayCounter, "ArrayCounter", "biomeBandCount", (u32) (&((BiomeConfiguration*)0)->biomeBandCount), {}, sizeof(ArrayCounter),"invalid","biomeBands", (u32)(&((BiomeConfiguration*)0)->biomeBands), 0}, 
{MetaFlag_Pointer, MetaType_BiomeBand, "BiomeBand", "biomeBands", (u32) (&((BiomeConfiguration*)0)->biomeBands), {}, sizeof(BiomeBand),"invalid",0, 0, 0}, 
{0, MetaType_GameProperty, "GameProperty", "underSeaLevelFluid", (u32) (&((BiomeConfiguration*)0)->underSeaLevelFluid), {}, sizeof(GameProperty),"invalid",0, 0, 0}, 
{0, MetaType_ArrayCounter, "ArrayCounter", "underwaterSoundCount", (u32) (&((BiomeConfiguration*)0)->underwaterSoundCount), {}, sizeof(ArrayCounter),"invalid","underwaterSounds", (u32)(&((BiomeConfiguration*)0)->underwaterSounds), 0}, 
{MetaFlag_Pointer, MetaType_SoundMappingDefinition, "SoundMappingDefinition", "underwaterSounds", (u32) (&((BiomeConfiguration*)0)->underwaterSounds), {}, sizeof(SoundMappingDefinition),"invalid",0, 0, 0}, 
};

FieldDefinition fieldDefinitionOfworld_generator[] = 
 {
{0, MetaType_NoiseParams, "NoiseParams", "landscapeNoise", (u32) (&((world_generator*)0)->landscapeNoise), {}, sizeof(NoiseParams),"invalid",0, 0, 0}, 
{0, MetaType_NoiseSelector, "NoiseSelector", "landscapeSelect", (u32) (&((world_generator*)0)->landscapeSelect), {}, sizeof(NoiseSelector),"invalid",0, 0, 0}, 
{0, MetaType_NoiseParams, "NoiseParams", "temperatureNoise", (u32) (&((world_generator*)0)->temperatureNoise), {}, sizeof(NoiseParams),"invalid",0, 0, 0}, 
{0, MetaType_NoiseSelector, "NoiseSelector", "temperatureSelect", (u32) (&((world_generator*)0)->temperatureSelect), {}, sizeof(NoiseSelector),"invalid",0, 0, 0}, 
{0, MetaType_NoiseParams, "NoiseParams", "elevationNoise", (u32) (&((world_generator*)0)->elevationNoise), {}, sizeof(NoiseParams),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "elevationPower", (u32) (&((world_generator*)0)->elevationPower), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "elevationNormOffset", (u32) (&((world_generator*)0)->elevationNormOffset), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "waterSafetyMargin", (u32) (&((world_generator*)0)->waterSafetyMargin), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_BiomePyramid, "BiomePyramid", "biomePyramid", (u32) (&((world_generator*)0)->biomePyramid), {}, sizeof(BiomePyramid),"invalid",0, 0, 0}, 
{0, MetaType_u32, "u32", "maxDeepness", (u32) (&((world_generator*)0)->maxDeepness), {}, sizeof(u32),"invalid",0, 0, 0}, 
{0, MetaType_ArrayCounter, "ArrayCounter", "zSlicesCount", (u32) (&((world_generator*)0)->zSlicesCount), {}, sizeof(ArrayCounter),"invalid","zSlices", (u32)(&((world_generator*)0)->zSlices), 0}, 
{MetaFlag_Pointer, MetaType_ZSlice, "ZSlice", "zSlices", (u32) (&((world_generator*)0)->zSlices), {}, sizeof(ZSlice),"invalid",0, 0, 0}, 
{0, MetaType_ArrayCounter, "ArrayCounter", "biomeConfigurationCount", (u32) (&((world_generator*)0)->biomeConfigurationCount), {}, sizeof(ArrayCounter),"invalid","biomeConfigurations", (u32)(&((world_generator*)0)->biomeConfigurations), 0}, 
{MetaFlag_Pointer, MetaType_BiomeConfiguration, "BiomeConfiguration", "biomeConfigurations", (u32) (&((world_generator*)0)->biomeConfigurations), {}, sizeof(BiomeConfiguration),"invalid",0, 0, 0}, 
};

FieldDefinition fieldDefinitionOfSpawnerEntity[] = 
 {
{0, MetaType_EntityName, "EntityName", "type", (u32) (&((SpawnerEntity*)0)->type), {}, sizeof(EntityName),"invalid",0, 0, 0}, 
{0, MetaType_b32, "b32", "occupiesTile", (u32) (&((SpawnerEntity*)0)->occupiesTile), {}, sizeof(b32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "entityOffsetCellDimCoeff", (u32) (&((SpawnerEntity*)0)->entityOffsetCellDimCoeff), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "minEntityDistance", (u32) (&((SpawnerEntity*)0)->minEntityDistance), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_i32, "i32", "count", (u32) (&((SpawnerEntity*)0)->count), {}, sizeof(i32),"invalid",0, 0, 0}, 
{0, MetaType_i32, "i32", "countV", (u32) (&((SpawnerEntity*)0)->countV), {}, sizeof(i32),"invalid",0, 0, 0}, 
{0, MetaType_b32, "b32", "attachedBrainEntity", (u32) (&((SpawnerEntity*)0)->attachedBrainEntity), {}, sizeof(b32),"invalid",0, 0, 0}, 
{0, MetaType_EntityName, "EntityName", "attachedBrainType", (u32) (&((SpawnerEntity*)0)->attachedBrainType), {}, sizeof(EntityName),"invalid",0, 0, 0}, 
{0, MetaType_GameProperty, "GameProperty", "repulsionTile", (u32) (&((SpawnerEntity*)0)->repulsionTile), {}, sizeof(GameProperty),"invalid",0, 0, "property"}, 
{0, MetaType_GameProperty, "GameProperty", "repulsionFluid", (u32) (&((SpawnerEntity*)0)->repulsionFluid), {}, sizeof(GameProperty),"invalid",0, 0, "property"}, 
{0, MetaType_u32, "u32", "repulsionRadious", (u32) (&((SpawnerEntity*)0)->repulsionRadious), {}, sizeof(u32),"invalid",0, 0, 0}, 
{0, MetaType_GameProperty, "GameProperty", "requiredTile", (u32) (&((SpawnerEntity*)0)->requiredTile), {}, sizeof(GameProperty),"invalid",0, 0, "property"}, 
{0, MetaType_GameProperty, "GameProperty", "requiredFluid", (u32) (&((SpawnerEntity*)0)->requiredFluid), {}, sizeof(GameProperty),"invalid",0, 0, "property"}, 
{0, MetaType_u32, "u32", "requiredRadious", (u32) (&((SpawnerEntity*)0)->requiredRadious), {}, sizeof(u32),"invalid",0, 0, 0}, 
};

FieldDefinition fieldDefinitionOfSpawnerOption[] = 
 {
{0, MetaType_r32, "r32", "weight", (u32) (&((SpawnerOption*)0)->weight), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_GameProperty, "GameProperty", "repulsionTile", (u32) (&((SpawnerOption*)0)->repulsionTile), {}, sizeof(GameProperty),"invalid",0, 0, "property"}, 
{0, MetaType_GameProperty, "GameProperty", "repulsionFluid", (u32) (&((SpawnerOption*)0)->repulsionFluid), {}, sizeof(GameProperty),"invalid",0, 0, "property"}, 
{0, MetaType_u32, "u32", "repulsionRadious", (u32) (&((SpawnerOption*)0)->repulsionRadious), {}, sizeof(u32),"invalid",0, 0, 0}, 
{0, MetaType_GameProperty, "GameProperty", "requiredTile", (u32) (&((SpawnerOption*)0)->requiredTile), {}, sizeof(GameProperty),"invalid",0, 0, "property"}, 
{0, MetaType_GameProperty, "GameProperty", "requiredFluid", (u32) (&((SpawnerOption*)0)->requiredFluid), {}, sizeof(GameProperty),"invalid",0, 0, "property"}, 
{0, MetaType_u32, "u32", "requiredRadious", (u32) (&((SpawnerOption*)0)->requiredRadious), {}, sizeof(u32),"invalid",0, 0, 0}, 
{0, MetaType_ArrayCounter, "ArrayCounter", "entityCount", (u32) (&((SpawnerOption*)0)->entityCount), {}, sizeof(ArrayCounter),"invalid","entities", (u32)(&((SpawnerOption*)0)->entities), 0}, 
{MetaFlag_Pointer, MetaType_SpawnerEntity, "SpawnerEntity", "entities", (u32) (&((SpawnerOption*)0)->entities), {}, sizeof(SpawnerEntity),"invalid",0, 0, 0}, 
};

FieldDefinition fieldDefinitionOfSpawner[] = 
 {
{MetaFlag_Uneditable, MetaType_r32, "r32", "time", (u32) (&((Spawner*)0)->time), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "targetTime", (u32) (&((Spawner*)0)->targetTime), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "cellDim", (u32) (&((Spawner*)0)->cellDim), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "percentageOfStartingCells", (u32) (&((Spawner*)0)->percentageOfStartingCells), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "clusterOffsetCellDimCoeff", (u32) (&((Spawner*)0)->clusterOffsetCellDimCoeff), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "minClusterDistance", (u32) (&((Spawner*)0)->minClusterDistance), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_i32, "i32", "clusterCount", (u32) (&((Spawner*)0)->clusterCount), {}, sizeof(i32),"invalid",0, 0, 0}, 
{0, MetaType_i32, "i32", "clusterCountV", (u32) (&((Spawner*)0)->clusterCountV), {}, sizeof(i32),"invalid",0, 0, 0}, 
{0, MetaType_ArrayCounter, "ArrayCounter", "optionCount", (u32) (&((Spawner*)0)->optionCount), {}, sizeof(ArrayCounter),"invalid","options", (u32)(&((Spawner*)0)->options), 0}, 
{MetaFlag_Pointer, MetaType_SpawnerOption, "SpawnerOption", "options", (u32) (&((Spawner*)0)->options), {}, sizeof(SpawnerOption),"invalid",0, 0, 0}, 
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
{0, MetaType_ArrayCounter, "ArrayCounter", "soundCount", (u32) (&((tile_definition*)0)->soundCount), {}, sizeof(ArrayCounter),"invalid","sounds", (u32)(&((tile_definition*)0)->sounds), 0}, 
{MetaFlag_Pointer, MetaType_SoundMappingDefinition, "SoundMappingDefinition", "sounds", (u32) (&((tile_definition*)0)->sounds), {}, sizeof(SoundMappingDefinition),"invalid",0, 0, 0}, 
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

FieldDefinition fieldDefinitionOfEntityType[] = 
 {
{0, MetaType_u32, "u32", "subtypeHashIndex", (u32) (&((EntityType*)0)->subtypeHashIndex), {}, sizeof(u32),"invalid",0, 0, 0}, 
{0, MetaType_u32, "u32", "index", (u32) (&((EntityType*)0)->index), {}, sizeof(u32),"invalid",0, 0, 0}, 
};

FieldDefinition fieldDefinitionOfTileAnimationEffect[] = 
 {
{0, MetaType_Vec3, "Vec3", "maxOffset", (u32) (&((TileAnimationEffect*)0)->maxOffset), {}, sizeof(Vec3),"invalid",0, 0, 0}, 
{0, MetaType_Vec4, "Vec4", "color", (u32) (&((TileAnimationEffect*)0)->color), {}, sizeof(Vec4),"invalid",0, 0, 0}, 
{0, MetaType_Vec4, "Vec4", "colorV", (u32) (&((TileAnimationEffect*)0)->colorV), {}, sizeof(Vec4),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "scale", (u32) (&((TileAnimationEffect*)0)->scale), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "scaleV", (u32) (&((TileAnimationEffect*)0)->scaleV), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "sineSpeed", (u32) (&((TileAnimationEffect*)0)->sineSpeed), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_u32, "u32", "patchCount", (u32) (&((TileAnimationEffect*)0)->patchCount), {}, sizeof(u32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "tileZBias", (u32) (&((TileAnimationEffect*)0)->tileZBias), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "patchZBias", (u32) (&((TileAnimationEffect*)0)->patchZBias), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_GameAssetType, "GameAssetType", "asset", (u32) (&((TileAnimationEffect*)0)->asset), {}, sizeof(GameAssetType),"invalid",0, 0, "type"}, 
};

FieldDefinition fieldDefinitionOfEntityID[] = 
 {
{0, MetaType_u32, "u32", "archetype_archetypeIndex", (u32) (&((EntityID*)0)->archetype_archetypeIndex), {}, sizeof(u32),"invalid",0, 0, 0}, 
};

FieldDefinition fieldDefinitionOfUseLayout[] = 
 {
{0, MetaType_ArrayCounter, "ArrayCounter", "slotCount", (u32) (&((UseLayout*)0)->slotCount), {}, sizeof(ArrayCounter),"invalid","slots", (u32)(&((UseLayout*)0)->slots), 0}, 
{MetaFlag_Pointer, MetaType_Enumerator, "Enumerator", "slots", (u32) (&((UseLayout*)0)->slots), {}, sizeof(Enumerator),"invalid",0, 0, 0, MetaTable_usingSlot, ArrayCount(MetaTable_usingSlot)}, 
};

FieldDefinition fieldDefinitionOfEquipLayout[] = 
 {
{0, MetaType_ArrayCounter, "ArrayCounter", "slotCount", (u32) (&((EquipLayout*)0)->slotCount), {}, sizeof(ArrayCounter),"invalid","slots", (u32)(&((EquipLayout*)0)->slots), 0}, 
{MetaFlag_Pointer, MetaType_Enumerator, "Enumerator", "slots", (u32) (&((EquipLayout*)0)->slots), {}, sizeof(Enumerator),"invalid",0, 0, 0, MetaTable_equipmentSlot, ArrayCount(MetaTable_equipmentSlot)}, 
};

FieldDefinition fieldDefinitionOfPossibleActionDefinition[] = 
 {
{0, MetaType_Enumerator, "Enumerator", "action", (u32) (&((PossibleActionDefinition*)0)->action), {}, sizeof(Enumerator),"invalid",0, 0, 0, MetaTable_action, ArrayCount(MetaTable_action)}, 
{0, MetaType_r32, "r32", "distance", (u32) (&((PossibleActionDefinition*)0)->distance), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "continueDistanceCoeff", (u32) (&((PossibleActionDefinition*)0)->continueDistanceCoeff), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_GameProperty, "GameProperty", "special", (u32) (&((PossibleActionDefinition*)0)->special), {}, sizeof(GameProperty),"invalid",0, 0, "property"}, 
{0, MetaType_r32, "r32", "time", (u32) (&((PossibleActionDefinition*)0)->time), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_EntityName, "EntityName", "requiredUsingType", (u32) (&((PossibleActionDefinition*)0)->requiredUsingType), {}, sizeof(EntityName),"invalid",0, 0, 0}, 
{0, MetaType_EntityName, "EntityName", "requiredEquippedType", (u32) (&((PossibleActionDefinition*)0)->requiredEquippedType), {}, sizeof(EntityName),"invalid",0, 0, 0}, 
};

FieldDefinition fieldDefinitionOfCraftingComponent[] = 
 {
{0, MetaType_ArrayCounter, "ArrayCounter", "optionCount", (u32) (&((CraftingComponent*)0)->optionCount), {}, sizeof(ArrayCounter),"invalid","options", (u32)(&((CraftingComponent*)0)->options), 0}, 
{MetaFlag_Pointer, MetaType_EntityName, "EntityName", "options", (u32) (&((CraftingComponent*)0)->options), {}, sizeof(EntityName),"invalid",0, 0, 0}, 
{0, MetaType_b32, "b32", "deleteAfterCrafting", (u32) (&((CraftingComponent*)0)->deleteAfterCrafting), {}, sizeof(b32),"invalid",0, 0, 0}, 
};

FieldDefinition fieldDefinitionOfCommonEntityInitParams[] = 
 {
{MetaFlag_Uneditable, MetaType_EntityType, "EntityType", "type", (u32) (&((CommonEntityInitParams*)0)->type), {}, sizeof(EntityType),"invalid",0, 0, 0}, 
{MetaFlag_Pointer|MetaFlag_Uneditable, MetaType_u16, "u16", "essences", (u32) (&((CommonEntityInitParams*)0)->essences), {}, sizeof(u16),"invalid",0, 0, 0}, 
{0, MetaType_b32, "b32", "craftable", (u32) (&((CommonEntityInitParams*)0)->craftable), {}, sizeof(b32),"invalid",0, 0, 0}, 
{0, MetaType_u16, "u16", "essenceCountRef", (u32) (&((CommonEntityInitParams*)0)->essenceCountRef), {}, sizeof(u16),"invalid",0, 0, 0}, 
{0, MetaType_u16, "u16", "essenceCountV", (u32) (&((CommonEntityInitParams*)0)->essenceCountV), {}, sizeof(u16),"invalid",0, 0, 0}, 
{0, MetaType_ArrayCounter, "ArrayCounter", "componentCount", (u32) (&((CommonEntityInitParams*)0)->componentCount), {}, sizeof(ArrayCounter),"invalid","components", (u32)(&((CommonEntityInitParams*)0)->components), 0}, 
{MetaFlag_Pointer, MetaType_CraftingComponent, "CraftingComponent", "components", (u32) (&((CommonEntityInitParams*)0)->components), {}, sizeof(CraftingComponent),"invalid",0, 0, 0}, 
{0, MetaType_GameProperty, "GameProperty", "boundType", (u32) (&((CommonEntityInitParams*)0)->boundType), {}, sizeof(GameProperty),"invalid",0, 0, "property"}, 
{0, MetaType_Vec3, "Vec3", "boundOffset", (u32) (&((CommonEntityInitParams*)0)->boundOffset), {}, sizeof(Vec3),"invalid",0, 0, 0}, 
{0, MetaType_Vec3, "Vec3", "boundDim", (u32) (&((CommonEntityInitParams*)0)->boundDim), {}, sizeof(Vec3),"invalid",0, 0, 0}, 
{0, MetaType_Vec3, "Vec3", "boundDimV", (u32) (&((CommonEntityInitParams*)0)->boundDimV), {}, sizeof(Vec3),"invalid",0, 0, 0}, 
{0, MetaType_ArrayCounter, "ArrayCounter", "groundActionCount", (u32) (&((CommonEntityInitParams*)0)->groundActionCount), {}, sizeof(ArrayCounter),"invalid","groundActions", (u32)(&((CommonEntityInitParams*)0)->groundActions), 0}, 
{MetaFlag_Pointer, MetaType_PossibleActionDefinition, "PossibleActionDefinition", "groundActions", (u32) (&((CommonEntityInitParams*)0)->groundActions), {}, sizeof(PossibleActionDefinition),"invalid",0, 0, 0}, 
{0, MetaType_ArrayCounter, "ArrayCounter", "equipmentActionCount", (u32) (&((CommonEntityInitParams*)0)->equipmentActionCount), {}, sizeof(ArrayCounter),"invalid","equipmentActions", (u32)(&((CommonEntityInitParams*)0)->equipmentActions), 0}, 
{MetaFlag_Pointer, MetaType_PossibleActionDefinition, "PossibleActionDefinition", "equipmentActions", (u32) (&((CommonEntityInitParams*)0)->equipmentActions), {}, sizeof(PossibleActionDefinition),"invalid",0, 0, 0}, 
{0, MetaType_ArrayCounter, "ArrayCounter", "containerActionCount", (u32) (&((CommonEntityInitParams*)0)->containerActionCount), {}, sizeof(ArrayCounter),"invalid","containerActions", (u32)(&((CommonEntityInitParams*)0)->containerActions), 0}, 
{MetaFlag_Pointer, MetaType_PossibleActionDefinition, "PossibleActionDefinition", "containerActions", (u32) (&((CommonEntityInitParams*)0)->containerActions), {}, sizeof(PossibleActionDefinition),"invalid",0, 0, 0}, 
{0, MetaType_ArrayCounter, "ArrayCounter", "equippedActionCount", (u32) (&((CommonEntityInitParams*)0)->equippedActionCount), {}, sizeof(ArrayCounter),"invalid","equippedActions", (u32)(&((CommonEntityInitParams*)0)->equippedActions), 0}, 
{MetaFlag_Pointer, MetaType_PossibleActionDefinition, "PossibleActionDefinition", "equippedActions", (u32) (&((CommonEntityInitParams*)0)->equippedActions), {}, sizeof(PossibleActionDefinition),"invalid",0, 0, 0}, 
{0, MetaType_ArrayCounter, "ArrayCounter", "draggingActionCount", (u32) (&((CommonEntityInitParams*)0)->draggingActionCount), {}, sizeof(ArrayCounter),"invalid","draggingActions", (u32)(&((CommonEntityInitParams*)0)->draggingActions), 0}, 
{MetaFlag_Pointer, MetaType_PossibleActionDefinition, "PossibleActionDefinition", "draggingActions", (u32) (&((CommonEntityInitParams*)0)->draggingActions), {}, sizeof(PossibleActionDefinition),"invalid",0, 0, 0}, 
{0, MetaType_ArrayCounter, "ArrayCounter", "usingConfigurationCount", (u32) (&((CommonEntityInitParams*)0)->usingConfigurationCount), {}, sizeof(ArrayCounter),"invalid","usingConfigurations", (u32)(&((CommonEntityInitParams*)0)->usingConfigurations), 0}, 
{MetaFlag_Pointer, MetaType_UseLayout, "UseLayout", "usingConfigurations", (u32) (&((CommonEntityInitParams*)0)->usingConfigurations), {}, sizeof(UseLayout),"invalid",0, 0, 0}, 
{0, MetaType_ArrayCounter, "ArrayCounter", "equipConfigurationCount", (u32) (&((CommonEntityInitParams*)0)->equipConfigurationCount), {}, sizeof(ArrayCounter),"invalid","equipConfigurations", (u32)(&((CommonEntityInitParams*)0)->equipConfigurations), 0}, 
{MetaFlag_Pointer, MetaType_EquipLayout, "EquipLayout", "equipConfigurations", (u32) (&((CommonEntityInitParams*)0)->equipConfigurations), {}, sizeof(EquipLayout),"invalid",0, 0, 0}, 
{0, MetaType_Enumerator, "Enumerator", "inventorySlotType", (u32) (&((CommonEntityInitParams*)0)->inventorySlotType), {}, sizeof(Enumerator),"invalid",0, 0, 0, MetaTable_inventorySlotType, ArrayCount(MetaTable_inventorySlotType)}, 
{0, MetaType_b32, "b32", "targetSkill", (u32) (&((CommonEntityInitParams*)0)->targetSkill), {}, sizeof(b32),"invalid",0, 0, 0}, 
{0, MetaType_b32, "b32", "passive", (u32) (&((CommonEntityInitParams*)0)->passive), {}, sizeof(b32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "flowerDensity", (u32) (&((CommonEntityInitParams*)0)->flowerDensity), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "fruitDensity", (u32) (&((CommonEntityInitParams*)0)->fruitDensity), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_Vec3, "Vec3", "lightColor", (u32) (&((CommonEntityInitParams*)0)->lightColor), {}, sizeof(Vec3),"invalid",0, 0, 0}, 
};

FieldDefinition fieldDefinitionOfInventorySlots[] = 
 {
{0, MetaType_GameProperty, "GameProperty", "type", (u32) (&((InventorySlots*)0)->type), {}, sizeof(GameProperty),"invalid",0, 0, "property"}, 
{0, MetaType_u16, "u16", "count", (u32) (&((InventorySlots*)0)->count), {}, sizeof(u16),"invalid",0, 0, 0}, 
};

FieldDefinition fieldDefinitionOfServerEntityInitParams[] = 
 {
{MetaFlag_Uneditable, MetaType_UniversePos, "UniversePos", "P", (u32) (&((ServerEntityInitParams*)0)->P), {}, sizeof(UniversePos),"invalid",0, 0, 0}, 
{MetaFlag_Uneditable, MetaType_u32, "u32", "seed", (u32) (&((ServerEntityInitParams*)0)->seed), {}, sizeof(u32),"invalid",0, 0, 0}, 
{MetaFlag_Uneditable, MetaType_Vec3, "Vec3", "startingAcceleration", (u32) (&((ServerEntityInitParams*)0)->startingAcceleration), {}, sizeof(Vec3),"invalid",0, 0, 0}, 
{MetaFlag_Uneditable, MetaType_Vec3, "Vec3", "startingSpeed", (u32) (&((ServerEntityInitParams*)0)->startingSpeed), {}, sizeof(Vec3),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "accelerationCoeff", (u32) (&((ServerEntityInitParams*)0)->accelerationCoeff), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "drag", (u32) (&((ServerEntityInitParams*)0)->drag), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_ArrayCounter, "ArrayCounter", "bindingCount", (u32) (&((ServerEntityInitParams*)0)->bindingCount), {}, sizeof(ArrayCounter),"invalid","bindings", (u32)(&((ServerEntityInitParams*)0)->bindings), 0}, 
{MetaFlag_Pointer, MetaType_EffectBinding, "EffectBinding", "bindings", (u32) (&((ServerEntityInitParams*)0)->bindings), {}, sizeof(EffectBinding),"invalid",0, 0, 0}, 
{0, MetaType_ArrayCounter, "ArrayCounter", "storeSlotCounter", (u32) (&((ServerEntityInitParams*)0)->storeSlotCounter), {}, sizeof(ArrayCounter),"invalid","storeSlots", (u32)(&((ServerEntityInitParams*)0)->storeSlots), 0}, 
{MetaFlag_Pointer, MetaType_InventorySlots, "InventorySlots", "storeSlots", (u32) (&((ServerEntityInitParams*)0)->storeSlots), {}, sizeof(InventorySlots),"invalid",0, 0, 0}, 
{0, MetaType_ArrayCounter, "ArrayCounter", "usingSlotCounter", (u32) (&((ServerEntityInitParams*)0)->usingSlotCounter), {}, sizeof(ArrayCounter),"invalid","usingSlots", (u32)(&((ServerEntityInitParams*)0)->usingSlots), 0}, 
{MetaFlag_Pointer, MetaType_InventorySlots, "InventorySlots", "usingSlots", (u32) (&((ServerEntityInitParams*)0)->usingSlots), {}, sizeof(InventorySlots),"invalid",0, 0, 0}, 
{0, MetaType_ArrayCounter, "ArrayCounter", "collisionEffectsCount", (u32) (&((ServerEntityInitParams*)0)->collisionEffectsCount), {}, sizeof(ArrayCounter),"invalid","collisionEffects", (u32)(&((ServerEntityInitParams*)0)->collisionEffects), 0}, 
{MetaFlag_Pointer, MetaType_GameEffect, "GameEffect", "collisionEffects", (u32) (&((ServerEntityInitParams*)0)->collisionEffects), {}, sizeof(GameEffect),"invalid",0, 0, 0}, 
{0, MetaType_ArrayCounter, "ArrayCounter", "defaultEffectsCount", (u32) (&((ServerEntityInitParams*)0)->defaultEffectsCount), {}, sizeof(ArrayCounter),"invalid","defaultEffects", (u32)(&((ServerEntityInitParams*)0)->defaultEffects), 0}, 
{MetaFlag_Pointer, MetaType_GameEffect, "GameEffect", "defaultEffects", (u32) (&((ServerEntityInitParams*)0)->defaultEffects), {}, sizeof(GameEffect),"invalid",0, 0, 0}, 
{0, MetaType_ArrayCounter, "ArrayCounter", "probabilityEffectsCount", (u32) (&((ServerEntityInitParams*)0)->probabilityEffectsCount), {}, sizeof(ArrayCounter),"invalid","probabilityEffects", (u32)(&((ServerEntityInitParams*)0)->probabilityEffects), 0}, 
{MetaFlag_Pointer, MetaType_ProbabilityEffect, "ProbabilityEffect", "probabilityEffects", (u32) (&((ServerEntityInitParams*)0)->probabilityEffects), {}, sizeof(ProbabilityEffect),"invalid",0, 0, 0}, 
{0, MetaType_Enumerator, "Enumerator", "brainType", (u32) (&((ServerEntityInitParams*)0)->brainType), {}, sizeof(Enumerator),"invalid",0, 0, 0, MetaTable_brainType, ArrayCount(MetaTable_brainType)}, 
{0, MetaType_BrainParams, "BrainParams", "brainParams", (u32) (&((ServerEntityInitParams*)0)->brainParams), {}, sizeof(BrainParams),"invalid",0, 0, 0}, 
{0, MetaType_b32, "b32", "canGoIntoWater", (u32) (&((ServerEntityInitParams*)0)->canGoIntoWater), {}, sizeof(b32),"invalid",0, 0, 0}, 
{0, MetaType_b32, "b32", "fearsLight", (u32) (&((ServerEntityInitParams*)0)->fearsLight), {}, sizeof(b32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "lightRadious", (u32) (&((ServerEntityInitParams*)0)->lightRadious), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_u32, "u32", "defaultEssenceCount", (u32) (&((ServerEntityInitParams*)0)->defaultEssenceCount), {}, sizeof(u32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "flowerGrowingSpeed", (u32) (&((ServerEntityInitParams*)0)->flowerGrowingSpeed), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "requiredFlowerDensity", (u32) (&((ServerEntityInitParams*)0)->requiredFlowerDensity), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "fruitGrowingSpeed", (u32) (&((ServerEntityInitParams*)0)->fruitGrowingSpeed), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "requiredFruitDensity", (u32) (&((ServerEntityInitParams*)0)->requiredFruitDensity), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "maxPhysicalHealth", (u32) (&((ServerEntityInitParams*)0)->maxPhysicalHealth), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "maxMentalHealth", (u32) (&((ServerEntityInitParams*)0)->maxMentalHealth), {}, sizeof(r32),"invalid",0, 0, 0}, 
};

FieldDefinition fieldDefinitionOfImageProperty[] = 
 {
{0, MetaType_b32, "b32", "optional", (u32) (&((ImageProperty*)0)->optional), {}, sizeof(b32),"invalid",0, 0, 0}, 
{0, MetaType_GameProperty, "GameProperty", "property", (u32) (&((ImageProperty*)0)->property), {}, sizeof(GameProperty),"invalid",0, 0, 0}, 
};

FieldDefinition fieldDefinitionOfImageProperties[] = 
 {
{0, MetaType_GameAssetType, "GameAssetType", "imageType", (u32) (&((ImageProperties*)0)->imageType), {}, sizeof(GameAssetType),"invalid",0, 0, "type"}, 
{0, MetaType_b32, "b32", "emittors", (u32) (&((ImageProperties*)0)->emittors), {}, sizeof(b32),"invalid",0, 0, 0}, 
{0, MetaType_ArrayCounter, "ArrayCounter", "propertyCount", (u32) (&((ImageProperties*)0)->propertyCount), {}, sizeof(ArrayCounter),"invalid","properties", (u32)(&((ImageProperties*)0)->properties), 0}, 
{MetaFlag_Pointer, MetaType_ImageProperty, "ImageProperty", "properties", (u32) (&((ImageProperties*)0)->properties), {}, sizeof(ImageProperty),"invalid",0, 0, 0}, 
};

FieldDefinition fieldDefinitionOfLayoutPieceProperties[] = 
 {
{0, MetaType_AssetLabel, "AssetLabel", "name", (u32) (&((LayoutPieceProperties*)0)->name), {}, sizeof(AssetLabel),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "height", (u32) (&((LayoutPieceProperties*)0)->height), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_Color, "Color", "color", (u32) (&((LayoutPieceProperties*)0)->color), {}, sizeof(Color),"invalid",0, 0, 0}, 
{0, MetaType_ImageProperties, "ImageProperties", "properties", (u32) (&((LayoutPieceProperties*)0)->properties), {}, sizeof(ImageProperties),"invalid",0, 0, 0}, 
{0, MetaType_Enumerator, "Enumerator", "inventorySlotType", (u32) (&((LayoutPieceProperties*)0)->inventorySlotType), {}, sizeof(Enumerator),"invalid",0, 0, 0, MetaTable_inventorySlotType, ArrayCount(MetaTable_inventorySlotType)}, 
};

FieldDefinition fieldDefinitionOfMultipartFrameByFramePiece[] = 
 {
{0, MetaType_r32, "r32", "speed", (u32) (&((MultipartFrameByFramePiece*)0)->speed), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_b32, "b32", "emittors", (u32) (&((MultipartFrameByFramePiece*)0)->emittors), {}, sizeof(b32),"invalid",0, 0, 0}, 
{0, MetaType_GameAssetType, "GameAssetType", "image", (u32) (&((MultipartFrameByFramePiece*)0)->image), {}, sizeof(GameAssetType),"invalid",0, 0, "type"}, 
};

FieldDefinition fieldDefinitionOfMultipartStaticPiece[] = 
 {
{0, MetaType_ImageProperties, "ImageProperties", "properties", (u32) (&((MultipartStaticPiece*)0)->properties), {}, sizeof(ImageProperties),"invalid",0, 0, 0}, 
};

FieldDefinition fieldDefinitionOfPossibleSkin[] = 
 {
{0, MetaType_GameAssetType, "GameAssetType", "skin", (u32) (&((PossibleSkin*)0)->skin), {}, sizeof(GameAssetType),"invalid",0, 0, "type"}, 
};

FieldDefinition fieldDefinitionOfColoration[] = 
 {
{0, MetaType_ArrayCounter, "ArrayCounter", "optionCount", (u32) (&((Coloration*)0)->optionCount), {}, sizeof(ArrayCounter),"invalid","options", (u32)(&((Coloration*)0)->options), 0}, 
{MetaFlag_Pointer, MetaType_Color, "Color", "options", (u32) (&((Coloration*)0)->options), {}, sizeof(Color),"invalid",0, 0, 0}, 
};

FieldDefinition fieldDefinitionOfClientEntityInitParams[] = 
 {
{MetaFlag_Uneditable, MetaType_EntityID, "EntityID", "ID", (u32) (&((ClientEntityInitParams*)0)->ID), {}, sizeof(EntityID),"invalid",0, 0, 0}, 
{MetaFlag_Uneditable, MetaType_u32, "u32", "seed", (u32) (&((ClientEntityInitParams*)0)->seed), {}, sizeof(u32),"invalid",0, 0, 0}, 
{0, MetaType_GameAssetType, "GameAssetType", "skeleton", (u32) (&((ClientEntityInitParams*)0)->skeleton), {}, sizeof(GameAssetType),"invalid",0, 0, "type"}, 
{0, MetaType_ArrayCounter, "ArrayCounter", "possibleSkinCount", (u32) (&((ClientEntityInitParams*)0)->possibleSkinCount), {}, sizeof(ArrayCounter),"invalid","possibleSkins", (u32)(&((ClientEntityInitParams*)0)->possibleSkins), 0}, 
{MetaFlag_Pointer, MetaType_PossibleSkin, "PossibleSkin", "possibleSkins", (u32) (&((ClientEntityInitParams*)0)->possibleSkins), {}, sizeof(PossibleSkin),"invalid",0, 0, 0}, 
{0, MetaType_ImageProperties, "ImageProperties", "entityProperties", (u32) (&((ClientEntityInitParams*)0)->entityProperties), {}, sizeof(ImageProperties),"invalid",0, 0, 0}, 
{0, MetaType_b32, "b32", "hasBranchVariant", (u32) (&((ClientEntityInitParams*)0)->hasBranchVariant), {}, sizeof(b32),"invalid",0, 0, 0}, 
{0, MetaType_Color, "Color", "branchColor", (u32) (&((ClientEntityInitParams*)0)->branchColor), {}, sizeof(Color),"invalid",0, 0, 0}, 
{0, MetaType_Vec4, "Vec4", "branchColorV", (u32) (&((ClientEntityInitParams*)0)->branchColorV), {}, sizeof(Vec4),"invalid",0, 0, 0}, 
{0, MetaType_ImageProperties, "ImageProperties", "trunkProperties", (u32) (&((ClientEntityInitParams*)0)->trunkProperties), {}, sizeof(ImageProperties),"invalid",0, 0, 0}, 
{0, MetaType_ImageProperties, "ImageProperties", "branchProperties", (u32) (&((ClientEntityInitParams*)0)->branchProperties), {}, sizeof(ImageProperties),"invalid",0, 0, 0}, 
{0, MetaType_b32, "b32", "hasLeafVariant", (u32) (&((ClientEntityInitParams*)0)->hasLeafVariant), {}, sizeof(b32),"invalid",0, 0, 0}, 
{0, MetaType_Color, "Color", "leafColor", (u32) (&((ClientEntityInitParams*)0)->leafColor), {}, sizeof(Color),"invalid",0, 0, 0}, 
{0, MetaType_Vec4, "Vec4", "leafColorV", (u32) (&((ClientEntityInitParams*)0)->leafColorV), {}, sizeof(Vec4),"invalid",0, 0, 0}, 
{0, MetaType_ImageProperties, "ImageProperties", "leafProperties", (u32) (&((ClientEntityInitParams*)0)->leafProperties), {}, sizeof(ImageProperties),"invalid",0, 0, 0}, 
{0, MetaType_b32, "b32", "hasFlowerVariant", (u32) (&((ClientEntityInitParams*)0)->hasFlowerVariant), {}, sizeof(b32),"invalid",0, 0, 0}, 
{0, MetaType_Color, "Color", "flowerColor", (u32) (&((ClientEntityInitParams*)0)->flowerColor), {}, sizeof(Color),"invalid",0, 0, 0}, 
{0, MetaType_Vec4, "Vec4", "flowerColorV", (u32) (&((ClientEntityInitParams*)0)->flowerColorV), {}, sizeof(Vec4),"invalid",0, 0, 0}, 
{0, MetaType_ImageProperties, "ImageProperties", "flowerProperties", (u32) (&((ClientEntityInitParams*)0)->flowerProperties), {}, sizeof(ImageProperties),"invalid",0, 0, 0}, 
{0, MetaType_b32, "b32", "hasFruitVariant", (u32) (&((ClientEntityInitParams*)0)->hasFruitVariant), {}, sizeof(b32),"invalid",0, 0, 0}, 
{0, MetaType_Color, "Color", "fruitColor", (u32) (&((ClientEntityInitParams*)0)->fruitColor), {}, sizeof(Color),"invalid",0, 0, 0}, 
{0, MetaType_Vec4, "Vec4", "fruitColorV", (u32) (&((ClientEntityInitParams*)0)->fruitColorV), {}, sizeof(Vec4),"invalid",0, 0, 0}, 
{0, MetaType_ImageProperties, "ImageProperties", "fruitProperties", (u32) (&((ClientEntityInitParams*)0)->fruitProperties), {}, sizeof(ImageProperties),"invalid",0, 0, 0}, 
{0, MetaType_ImageProperties, "ImageProperties", "rockProperties", (u32) (&((ClientEntityInitParams*)0)->rockProperties), {}, sizeof(ImageProperties),"invalid",0, 0, 0}, 
{0, MetaType_ImageProperties, "ImageProperties", "mineralProperties", (u32) (&((ClientEntityInitParams*)0)->mineralProperties), {}, sizeof(ImageProperties),"invalid",0, 0, 0}, 
{0, MetaType_Color, "Color", "rockColor", (u32) (&((ClientEntityInitParams*)0)->rockColor), {}, sizeof(Color),"invalid",0, 0, 0}, 
{0, MetaType_Vec4, "Vec4", "rockColorV", (u32) (&((ClientEntityInitParams*)0)->rockColorV), {}, sizeof(Vec4),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "windInfluence", (u32) (&((ClientEntityInitParams*)0)->windInfluence), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "leafWindInfluence", (u32) (&((ClientEntityInitParams*)0)->leafWindInfluence), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "flowerWindInfluence", (u32) (&((ClientEntityInitParams*)0)->flowerWindInfluence), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "fruitWindInfluence", (u32) (&((ClientEntityInitParams*)0)->fruitWindInfluence), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "dissolveDuration", (u32) (&((ClientEntityInitParams*)0)->dissolveDuration), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_u32, "u32", "windFrequencyStandard", (u32) (&((ClientEntityInitParams*)0)->windFrequencyStandard), {}, sizeof(u32),"invalid",0, 0, 0}, 
{0, MetaType_u32, "u32", "windFrequencyOverlap", (u32) (&((ClientEntityInitParams*)0)->windFrequencyOverlap), {}, sizeof(u32),"invalid",0, 0, 0}, 
{0, MetaType_u32, "u32", "instanceCount", (u32) (&((ClientEntityInitParams*)0)->instanceCount), {}, sizeof(u32),"invalid",0, 0, 0}, 
{0, MetaType_Vec3, "Vec3", "instanceMaxOffset", (u32) (&((ClientEntityInitParams*)0)->instanceMaxOffset), {}, sizeof(Vec3),"invalid",0, 0, 0}, 
{0, MetaType_Vec3, "Vec3", "grassBounds", (u32) (&((ClientEntityInitParams*)0)->grassBounds), {}, sizeof(Vec3),"invalid",0, 0, 0}, 
{0, MetaType_AssetLabel, "AssetLabel", "name", (u32) (&((ClientEntityInitParams*)0)->name), {}, sizeof(AssetLabel),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "slotZoomSpeed", (u32) (&((ClientEntityInitParams*)0)->slotZoomSpeed), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "maxSlotZoom", (u32) (&((ClientEntityInitParams*)0)->maxSlotZoom), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_AssetLabel, "AssetLabel", "layoutRootName", (u32) (&((ClientEntityInitParams*)0)->layoutRootName), {}, sizeof(AssetLabel),"invalid",0, 0, 0}, 
{0, MetaType_ArrayCounter, "ArrayCounter", "pieceCount", (u32) (&((ClientEntityInitParams*)0)->pieceCount), {}, sizeof(ArrayCounter),"invalid","layoutPieces", (u32)(&((ClientEntityInitParams*)0)->layoutPieces), 0}, 
{MetaFlag_Pointer, MetaType_LayoutPieceProperties, "LayoutPieceProperties", "layoutPieces", (u32) (&((ClientEntityInitParams*)0)->layoutPieces), {}, sizeof(LayoutPieceProperties),"invalid",0, 0, 0}, 
{0, MetaType_ArrayCounter, "ArrayCounter", "openPieceCount", (u32) (&((ClientEntityInitParams*)0)->openPieceCount), {}, sizeof(ArrayCounter),"invalid","openLayoutPieces", (u32)(&((ClientEntityInitParams*)0)->openLayoutPieces), 0}, 
{MetaFlag_Pointer, MetaType_LayoutPieceProperties, "LayoutPieceProperties", "openLayoutPieces", (u32) (&((ClientEntityInitParams*)0)->openLayoutPieces), {}, sizeof(LayoutPieceProperties),"invalid",0, 0, 0}, 
{0, MetaType_ArrayCounter, "ArrayCounter", "usingPieceCount", (u32) (&((ClientEntityInitParams*)0)->usingPieceCount), {}, sizeof(ArrayCounter),"invalid","usingLayoutPieces", (u32)(&((ClientEntityInitParams*)0)->usingLayoutPieces), 0}, 
{MetaFlag_Pointer, MetaType_LayoutPieceProperties, "LayoutPieceProperties", "usingLayoutPieces", (u32) (&((ClientEntityInitParams*)0)->usingLayoutPieces), {}, sizeof(LayoutPieceProperties),"invalid",0, 0, 0}, 
{0, MetaType_ArrayCounter, "ArrayCounter", "equippedPieceCount", (u32) (&((ClientEntityInitParams*)0)->equippedPieceCount), {}, sizeof(ArrayCounter),"invalid","equippedLayoutPieces", (u32)(&((ClientEntityInitParams*)0)->equippedLayoutPieces), 0}, 
{MetaFlag_Pointer, MetaType_LayoutPieceProperties, "LayoutPieceProperties", "equippedLayoutPieces", (u32) (&((ClientEntityInitParams*)0)->equippedLayoutPieces), {}, sizeof(LayoutPieceProperties),"invalid",0, 0, 0}, 
{0, MetaType_Vec3, "Vec3", "shadowOffset", (u32) (&((ClientEntityInitParams*)0)->shadowOffset), {}, sizeof(Vec3),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "shadowHeight", (u32) (&((ClientEntityInitParams*)0)->shadowHeight), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_Vec2, "Vec2", "shadowScale", (u32) (&((ClientEntityInitParams*)0)->shadowScale), {}, sizeof(Vec2),"invalid",0, 0, 0}, 
{0, MetaType_Vec4, "Vec4", "shadowColor", (u32) (&((ClientEntityInitParams*)0)->shadowColor), {}, sizeof(Vec4),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "lootingZoomCoeff", (u32) (&((ClientEntityInitParams*)0)->lootingZoomCoeff), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "lootingZoomSpeed", (u32) (&((ClientEntityInitParams*)0)->lootingZoomSpeed), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_b32, "b32", "displayInStandardMode", (u32) (&((ClientEntityInitParams*)0)->displayInStandardMode), {}, sizeof(b32),"invalid",0, 0, 0}, 
{0, MetaType_Vec2, "Vec2", "desiredOpenedDim", (u32) (&((ClientEntityInitParams*)0)->desiredOpenedDim), {}, sizeof(Vec2),"invalid",0, 0, 0}, 
{0, MetaType_Vec2, "Vec2", "desiredUsingDim", (u32) (&((ClientEntityInitParams*)0)->desiredUsingDim), {}, sizeof(Vec2),"invalid",0, 0, 0}, 
{0, MetaType_ArrayCounter, "ArrayCounter", "animationEffectsCount", (u32) (&((ClientEntityInitParams*)0)->animationEffectsCount), {}, sizeof(ArrayCounter),"invalid","animationEffects", (u32)(&((ClientEntityInitParams*)0)->animationEffects), 0}, 
{MetaFlag_Pointer, MetaType_AnimationEffectDefinition, "AnimationEffectDefinition", "animationEffects", (u32) (&((ClientEntityInitParams*)0)->animationEffects), {}, sizeof(AnimationEffectDefinition),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "frameByFrameSpeed", (u32) (&((ClientEntityInitParams*)0)->frameByFrameSpeed), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_b32, "b32", "frameByFrameEmittors", (u32) (&((ClientEntityInitParams*)0)->frameByFrameEmittors), {}, sizeof(b32),"invalid",0, 0, 0}, 
{0, MetaType_GameAssetType, "GameAssetType", "frameByFrameImageType", (u32) (&((ClientEntityInitParams*)0)->frameByFrameImageType), {}, sizeof(GameAssetType),"invalid",0, 0, "type"}, 
{0, MetaType_b32, "b32", "frameByFrameOverridesPivot", (u32) (&((ClientEntityInitParams*)0)->frameByFrameOverridesPivot), {}, sizeof(b32),"invalid",0, 0, 0}, 
{0, MetaType_Vec2, "Vec2", "frameByFramePivot", (u32) (&((ClientEntityInitParams*)0)->frameByFramePivot), {}, sizeof(Vec2),"invalid",0, 0, 0}, 
{0, MetaType_ArrayCounter, "ArrayCounter", "multipartStaticCount", (u32) (&((ClientEntityInitParams*)0)->multipartStaticCount), {}, sizeof(ArrayCounter),"invalid","multipartStaticPieces", (u32)(&((ClientEntityInitParams*)0)->multipartStaticPieces), 0}, 
{MetaFlag_Pointer, MetaType_MultipartStaticPiece, "MultipartStaticPiece", "multipartStaticPieces", (u32) (&((ClientEntityInitParams*)0)->multipartStaticPieces), {}, sizeof(MultipartStaticPiece),"invalid",0, 0, 0}, 
{0, MetaType_ArrayCounter, "ArrayCounter", "multipartFrameByFrameCount", (u32) (&((ClientEntityInitParams*)0)->multipartFrameByFrameCount), {}, sizeof(ArrayCounter),"invalid","multipartFrameByFramePieces", (u32)(&((ClientEntityInitParams*)0)->multipartFrameByFramePieces), 0}, 
{MetaFlag_Pointer, MetaType_MultipartFrameByFramePiece, "MultipartFrameByFramePiece", "multipartFrameByFramePieces", (u32) (&((ClientEntityInitParams*)0)->multipartFrameByFramePieces), {}, sizeof(MultipartFrameByFramePiece),"invalid",0, 0, 0}, 
{0, MetaType_ArrayCounter, "ArrayCounter", "replacementCount", (u32) (&((ClientEntityInitParams*)0)->replacementCount), {}, sizeof(ArrayCounter),"invalid","animationReplacements", (u32)(&((ClientEntityInitParams*)0)->animationReplacements), 0}, 
{MetaFlag_Pointer, MetaType_AnimationReplacement, "AnimationReplacement", "animationReplacements", (u32) (&((ClientEntityInitParams*)0)->animationReplacements), {}, sizeof(AnimationReplacement),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "fadeInTime", (u32) (&((ClientEntityInitParams*)0)->fadeInTime), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "fadeOutTime", (u32) (&((ClientEntityInitParams*)0)->fadeOutTime), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_ArrayCounter, "ArrayCounter", "soundEffectsCount", (u32) (&((ClientEntityInitParams*)0)->soundEffectsCount), {}, sizeof(ArrayCounter),"invalid","soundEffects", (u32)(&((ClientEntityInitParams*)0)->soundEffects), 0}, 
{MetaFlag_Pointer, MetaType_SoundEffectDefinition, "SoundEffectDefinition", "soundEffects", (u32) (&((ClientEntityInitParams*)0)->soundEffects), {}, sizeof(SoundEffectDefinition),"invalid",0, 0, 0}, 
{0, MetaType_Vec3, "Vec3", "spawnProjectileOffset", (u32) (&((ClientEntityInitParams*)0)->spawnProjectileOffset), {}, sizeof(Vec3),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "cameraZOffsetWhenOnFocus", (u32) (&((ClientEntityInitParams*)0)->cameraZOffsetWhenOnFocus), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "scaleCoeffWhenOnFocus", (u32) (&((ClientEntityInitParams*)0)->scaleCoeffWhenOnFocus), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "outlineWidth", (u32) (&((ClientEntityInitParams*)0)->outlineWidth), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_ArrayCounter, "ArrayCounter", "colorationCount", (u32) (&((ClientEntityInitParams*)0)->colorationCount), {}, sizeof(ArrayCounter),"invalid","colorations", (u32)(&((ClientEntityInitParams*)0)->colorations), 0}, 
{MetaFlag_Pointer, MetaType_Coloration, "Coloration", "colorations", (u32) (&((ClientEntityInitParams*)0)->colorations), {}, sizeof(Coloration),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "speedOnFocus", (u32) (&((ClientEntityInitParams*)0)->speedOnFocus), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "speedOnNoFocus", (u32) (&((ClientEntityInitParams*)0)->speedOnNoFocus), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_Vec3, "Vec3", "offsetMaxOnFocus", (u32) (&((ClientEntityInitParams*)0)->offsetMaxOnFocus), {}, sizeof(Vec3),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "scaleMaxOnFocus", (u32) (&((ClientEntityInitParams*)0)->scaleMaxOnFocus), {}, sizeof(r32),"invalid",0, 0, 0}, 
};

FieldDefinition fieldDefinitionOfEntityDefinition[] = 
 {
{0, MetaType_Enumerator, "Enumerator", "archetype", (u32) (&((EntityDefinition*)0)->archetype), {}, sizeof(Enumerator),"invalid",0, 0, 0, MetaTable_EntityArchetype, ArrayCount(MetaTable_EntityArchetype)}, 
{0, MetaType_CommonEntityInitParams, "CommonEntityInitParams", "common", (u32) (&((EntityDefinition*)0)->common), {}, sizeof(CommonEntityInitParams),"invalid",0, 0, 0}, 
{0, MetaType_ServerEntityInitParams, "ServerEntityInitParams", "server", (u32) (&((EntityDefinition*)0)->server), {}, sizeof(ServerEntityInitParams),"invalid",0, 0, 0}, 
{0, MetaType_ClientEntityInitParams, "ClientEntityInitParams", "client", (u32) (&((EntityDefinition*)0)->client), {}, sizeof(ClientEntityInitParams),"invalid",0, 0, 0}, 
};

char* MetaTable_AnimationEffectType[] = 
 {
"Tint",
"Light",
"Particles",
"SlowDown",
};

FieldDefinition fieldDefinitionOfAnimationEffectDefinition[] = 
 {
{0, MetaType_ArrayCounter, "ArrayCounter", "propertyCount", (u32) (&((AnimationEffectDefinition*)0)->propertyCount), {}, sizeof(ArrayCounter),"invalid","properties", (u32)(&((AnimationEffectDefinition*)0)->properties), 0}, 
{MetaFlag_Pointer, MetaType_GameProperty, "GameProperty", "properties", (u32) (&((AnimationEffectDefinition*)0)->properties), {}, sizeof(GameProperty),"invalid",0, 0, 0}, 
{0, MetaType_Enumerator, "Enumerator", "type", (u32) (&((AnimationEffectDefinition*)0)->type), {}, sizeof(Enumerator),"invalid",0, 0, 0, MetaTable_AnimationEffectType, ArrayCount(MetaTable_AnimationEffectType)}, 
{0, MetaType_r32, "r32", "time", (u32) (&((AnimationEffectDefinition*)0)->time), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_Vec4, "Vec4", "tint", (u32) (&((AnimationEffectDefinition*)0)->tint), {}, sizeof(Vec4),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "lightIntensity", (u32) (&((AnimationEffectDefinition*)0)->lightIntensity), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_Vec3, "Vec3", "lightColor", (u32) (&((AnimationEffectDefinition*)0)->lightColor), {}, sizeof(Vec3),"invalid",0, 0, 0}, 
{0, MetaType_ArrayCounter, "ArrayCounter", "particlePropertyCount", (u32) (&((AnimationEffectDefinition*)0)->particlePropertyCount), {}, sizeof(ArrayCounter),"invalid","particleProperties", (u32)(&((AnimationEffectDefinition*)0)->particleProperties), 0}, 
{MetaFlag_Pointer, MetaType_GameProperty, "GameProperty", "particleProperties", (u32) (&((AnimationEffectDefinition*)0)->particleProperties), {}, sizeof(GameProperty),"invalid",0, 0, 0}, 
{0, MetaType_Vec3, "Vec3", "particleEndOffset", (u32) (&((AnimationEffectDefinition*)0)->particleEndOffset), {}, sizeof(Vec3),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "slowDownCoeff", (u32) (&((AnimationEffectDefinition*)0)->slowDownCoeff), {}, sizeof(r32),"invalid",0, 0, 0}, 
};

FieldDefinition fieldDefinitionOfPieceReplacement[] = 
 {
{0, MetaType_GameAssetType, "GameAssetType", "imageType", (u32) (&((PieceReplacement*)0)->imageType), {}, sizeof(GameAssetType),"invalid",0, 0, "type"}, 
{0, MetaType_Vec3, "Vec3", "offset", (u32) (&((PieceReplacement*)0)->offset), {}, sizeof(Vec3),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "scale", (u32) (&((PieceReplacement*)0)->scale), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_b32, "b32", "inheritPivot", (u32) (&((PieceReplacement*)0)->inheritPivot), {}, sizeof(b32),"invalid",0, 0, 0}, 
{0, MetaType_Vec2, "Vec2", "pivot", (u32) (&((PieceReplacement*)0)->pivot), {}, sizeof(Vec2),"invalid",0, 0, 0}, 
{0, MetaType_b32, "b32", "inheritHeight", (u32) (&((PieceReplacement*)0)->inheritHeight), {}, sizeof(b32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "height", (u32) (&((PieceReplacement*)0)->height), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_u16, "u16", "colorationIndex", (u32) (&((PieceReplacement*)0)->colorationIndex), {}, sizeof(u16),"invalid",0, 0, 0}, 
};

FieldDefinition fieldDefinitionOfAnimationReplacement[] = 
 {
{0, MetaType_AssetLabel, "AssetLabel", "name", (u32) (&((AnimationReplacement*)0)->name), {}, sizeof(AssetLabel),"invalid",0, 0, 0}, 
{0, MetaType_ArrayCounter, "ArrayCounter", "pieceCount", (u32) (&((AnimationReplacement*)0)->pieceCount), {}, sizeof(ArrayCounter),"invalid","pieces", (u32)(&((AnimationReplacement*)0)->pieces), 0}, 
{MetaFlag_Pointer, MetaType_PieceReplacement, "PieceReplacement", "pieces", (u32) (&((AnimationReplacement*)0)->pieces), {}, sizeof(PieceReplacement),"invalid",0, 0, 0}, 
};

FieldDefinition fieldDefinitionOfParticleUpdater[] = 
 {
{MetaFlag_Uneditable, MetaType_AssetID, "AssetID", "bitmapID", (u32) (&((ParticleUpdater*)0)->bitmapID), {}, sizeof(AssetID),"invalid",0, 0, 0}, 
{MetaFlag_Pointer|MetaFlag_Uneditable, MetaType_RenderTexture, "RenderTexture", "texture", (u32) (&((ParticleUpdater*)0)->texture), {}, sizeof(RenderTexture),"invalid",0, 0, 0}, 
{MetaFlag_Uneditable, MetaType_Vec2, "Vec2", "textureInvUV", (u32) (&((ParticleUpdater*)0)->textureInvUV), {}, sizeof(Vec2),"invalid",0, 0, 0}, 
{0, MetaType_GameAssetType, "GameAssetType", "asset", (u32) (&((ParticleUpdater*)0)->asset), {}, sizeof(GameAssetType),"invalid",0, 0, "type"}, 
{0, MetaType_b32, "b32", "sineUpdater", (u32) (&((ParticleUpdater*)0)->sineUpdater), {}, sizeof(b32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "totalRadiants", (u32) (&((ParticleUpdater*)0)->totalRadiants), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_u32, "u32", "sineSubdivisions", (u32) (&((ParticleUpdater*)0)->sineSubdivisions), {}, sizeof(u32),"invalid",0, 0, 0}, 
{0, MetaType_Vec3, "Vec3", "ddP", (u32) (&((ParticleUpdater*)0)->ddP), {}, sizeof(Vec3),"invalid",0, 0, 0}, 
{0, MetaType_Vec3, "Vec3", "finalAcceleration", (u32) (&((ParticleUpdater*)0)->finalAcceleration), {}, sizeof(Vec3),"invalid",0, 0, 0}, 
{0, MetaType_Vec4, "Vec4", "ddC", (u32) (&((ParticleUpdater*)0)->ddC), {}, sizeof(Vec4),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "dScaleX", (u32) (&((ParticleUpdater*)0)->dScaleX), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "dScaleY", (u32) (&((ParticleUpdater*)0)->dScaleY), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "dAngle", (u32) (&((ParticleUpdater*)0)->dAngle), {}, sizeof(r32),"invalid",0, 0, 0}, 
};

FieldDefinition fieldDefinitionOfParticleEmitter[] = 
 {
{0, MetaType_r32, "r32", "particlesPerSec", (u32) (&((ParticleEmitter*)0)->particlesPerSec), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "lifeTime", (u32) (&((ParticleEmitter*)0)->lifeTime), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "lifeTimeV", (u32) (&((ParticleEmitter*)0)->lifeTimeV), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_Vec3, "Vec3", "startPV", (u32) (&((ParticleEmitter*)0)->startPV), {}, sizeof(Vec3),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "lerpWithUpVector", (u32) (&((ParticleEmitter*)0)->lerpWithUpVector), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_Vec3, "Vec3", "dP", (u32) (&((ParticleEmitter*)0)->dP), {}, sizeof(Vec3),"invalid",0, 0, 0}, 
{0, MetaType_Vec3, "Vec3", "dPV", (u32) (&((ParticleEmitter*)0)->dPV), {}, sizeof(Vec3),"invalid",0, 0, 0}, 
{0, MetaType_Vec4, "Vec4", "C", (u32) (&((ParticleEmitter*)0)->C), {}, sizeof(Vec4),"invalid",0, 0, 0}, 
{0, MetaType_Vec4, "Vec4", "CV", (u32) (&((ParticleEmitter*)0)->CV), {}, sizeof(Vec4),"invalid",0, 0, 0}, 
{0, MetaType_Vec4, "Vec4", "dC", (u32) (&((ParticleEmitter*)0)->dC), {}, sizeof(Vec4),"invalid",0, 0, 0}, 
{0, MetaType_Vec4, "Vec4", "dCV", (u32) (&((ParticleEmitter*)0)->dCV), {}, sizeof(Vec4),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "angle", (u32) (&((ParticleEmitter*)0)->angle), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "angleV", (u32) (&((ParticleEmitter*)0)->angleV), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "scaleX", (u32) (&((ParticleEmitter*)0)->scaleX), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "scaleXV", (u32) (&((ParticleEmitter*)0)->scaleXV), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "scaleY", (u32) (&((ParticleEmitter*)0)->scaleY), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "scaleYV", (u32) (&((ParticleEmitter*)0)->scaleYV), {}, sizeof(r32),"invalid",0, 0, 0}, 
};

FieldDefinition fieldDefinitionOfParticlePhase[] = 
 {
{0, MetaType_r32, "r32", "ttl", (u32) (&((ParticlePhase*)0)->ttl), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_ParticleUpdater, "ParticleUpdater", "updater", (u32) (&((ParticlePhase*)0)->updater), {}, sizeof(ParticleUpdater),"invalid",0, 0, 0}, 
};

FieldDefinition fieldDefinitionOfParticleEffect[] = 
 {
{0, MetaType_ParticleEmitter, "ParticleEmitter", "emitter", (u32) (&((ParticleEffect*)0)->emitter), {}, sizeof(ParticleEmitter),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "dAngleSineUpdaters", (u32) (&((ParticleEffect*)0)->dAngleSineUpdaters), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_ArrayCounter, "ArrayCounter", "phaseCount", (u32) (&((ParticleEffect*)0)->phaseCount), {}, sizeof(ArrayCounter),"invalid","phases", (u32)(&((ParticleEffect*)0)->phases), 0}, 
{MetaFlag_Pointer, MetaType_ParticlePhase, "ParticlePhase", "phases", (u32) (&((ParticleEffect*)0)->phases), {}, sizeof(ParticlePhase),"invalid",0, 0, 0}, 
{0, MetaType_ArrayCounter, "ArrayCounter", "soundCount", (u32) (&((ParticleEffect*)0)->soundCount), {}, sizeof(ArrayCounter),"invalid","sounds", (u32)(&((ParticleEffect*)0)->sounds), 0}, 
{MetaFlag_Pointer, MetaType_SoundMappingDefinition, "SoundMappingDefinition", "sounds", (u32) (&((ParticleEffect*)0)->sounds), {}, sizeof(SoundMappingDefinition),"invalid",0, 0, 0}, 
};

FieldDefinition fieldDefinitionOfSoundMappingDefinition[] = 
 {
{0, MetaType_r32, "r32", "targetTime", (u32) (&((SoundMappingDefinition*)0)->targetTime), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_u32, "u32", "maxRepeatCount", (u32) (&((SoundMappingDefinition*)0)->maxRepeatCount), {}, sizeof(u32),"invalid",0, 0, 0}, 
{0, MetaType_GameAssetType, "GameAssetType", "asset", (u32) (&((SoundMappingDefinition*)0)->asset), {}, sizeof(GameAssetType),"invalid",0, 0, "type"}, 
{0, MetaType_ArrayCounter, "ArrayCounter", "propertyCount", (u32) (&((SoundMappingDefinition*)0)->propertyCount), {}, sizeof(ArrayCounter),"invalid","properties", (u32)(&((SoundMappingDefinition*)0)->properties), 0}, 
{MetaFlag_Pointer, MetaType_GameProperty, "GameProperty", "properties", (u32) (&((SoundMappingDefinition*)0)->properties), {}, sizeof(GameProperty),"invalid",0, 0, 0}, 
};

FieldDefinition fieldDefinitionOfSoundEffectDefinition[] = 
 {
{0, MetaType_r32, "r32", "time", (u32) (&((SoundEffectDefinition*)0)->time), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_GameProperty, "GameProperty", "triggerType", (u32) (&((SoundEffectDefinition*)0)->triggerType), {}, sizeof(GameProperty),"invalid",0, 0, 0}, 
{0, MetaType_SoundMappingDefinition, "SoundMappingDefinition", "sound", (u32) (&((SoundEffectDefinition*)0)->sound), {}, sizeof(SoundMappingDefinition),"invalid",0, 0, 0}, 
{0, MetaType_ArrayCounter, "ArrayCounter", "propertyCount", (u32) (&((SoundEffectDefinition*)0)->propertyCount), {}, sizeof(ArrayCounter),"invalid","properties", (u32)(&((SoundEffectDefinition*)0)->properties), 0}, 
{MetaFlag_Pointer, MetaType_GameProperty, "GameProperty", "properties", (u32) (&((SoundEffectDefinition*)0)->properties), {}, sizeof(GameProperty),"invalid",0, 0, 0}, 
};

FieldDefinition fieldDefinitionOfBrainParams[] = 
 {
{0, MetaType_r32, "r32", "hostileDistanceRadious", (u32) (&((BrainParams*)0)->hostileDistanceRadious), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "maintainDistanceRadious", (u32) (&((BrainParams*)0)->maintainDistanceRadious), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "maintainDistanceModulationCoeff", (u32) (&((BrainParams*)0)->maintainDistanceModulationCoeff), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "maintainDistanceModulationAdiacentCoeff", (u32) (&((BrainParams*)0)->maintainDistanceModulationAdiacentCoeff), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "scaryDistanceRadious", (u32) (&((BrainParams*)0)->scaryDistanceRadious), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "safeDistanceRadious", (u32) (&((BrainParams*)0)->safeDistanceRadious), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "idleTimeWhenWandering", (u32) (&((BrainParams*)0)->idleTimeWhenWandering), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "wanderTargetTime", (u32) (&((BrainParams*)0)->wanderTargetTime), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "safetyLightRadious", (u32) (&((BrainParams*)0)->safetyLightRadious), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_EntityName, "EntityName", "hostileType", (u32) (&((BrainParams*)0)->hostileType), {}, sizeof(EntityName),"invalid",0, 0, 0}, 
{0, MetaType_EntityName, "EntityName", "maintainDistanceType", (u32) (&((BrainParams*)0)->maintainDistanceType), {}, sizeof(EntityName),"invalid",0, 0, 0}, 
{0, MetaType_EntityName, "EntityName", "scaryType", (u32) (&((BrainParams*)0)->scaryType), {}, sizeof(EntityName),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "minHomeDistance", (u32) (&((BrainParams*)0)->minHomeDistance), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "maxHomeDistance", (u32) (&((BrainParams*)0)->maxHomeDistance), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "reachableCellDim", (u32) (&((BrainParams*)0)->reachableCellDim), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_b32, "b32", "fearsLight", (u32) (&((BrainParams*)0)->fearsLight), {}, sizeof(b32),"invalid",0, 0, 0}, 
};

#define META_HANDLE_ADD_TO_DEFINITION_HASH()\
AddToMetaDefinitions(BrainParams, fieldDefinitionOfBrainParams);\
AddToMetaDefinitions(SoundEffectDefinition, fieldDefinitionOfSoundEffectDefinition);\
AddToMetaDefinitions(SoundMappingDefinition, fieldDefinitionOfSoundMappingDefinition);\
AddToMetaDefinitions(ParticleEffect, fieldDefinitionOfParticleEffect);\
AddToMetaDefinitions(ParticlePhase, fieldDefinitionOfParticlePhase);\
AddToMetaDefinitions(ParticleEmitter, fieldDefinitionOfParticleEmitter);\
AddToMetaDefinitions(ParticleUpdater, fieldDefinitionOfParticleUpdater);\
AddToMetaDefinitions(AnimationReplacement, fieldDefinitionOfAnimationReplacement);\
AddToMetaDefinitions(PieceReplacement, fieldDefinitionOfPieceReplacement);\
AddToMetaDefinitions(AnimationEffectDefinition, fieldDefinitionOfAnimationEffectDefinition);\
AddToMetaDefinitions(EntityDefinition, fieldDefinitionOfEntityDefinition);\
AddToMetaDefinitions(ClientEntityInitParams, fieldDefinitionOfClientEntityInitParams);\
AddToMetaDefinitions(Coloration, fieldDefinitionOfColoration);\
AddToMetaDefinitions(PossibleSkin, fieldDefinitionOfPossibleSkin);\
AddToMetaDefinitions(MultipartStaticPiece, fieldDefinitionOfMultipartStaticPiece);\
AddToMetaDefinitions(MultipartFrameByFramePiece, fieldDefinitionOfMultipartFrameByFramePiece);\
AddToMetaDefinitions(LayoutPieceProperties, fieldDefinitionOfLayoutPieceProperties);\
AddToMetaDefinitions(ImageProperties, fieldDefinitionOfImageProperties);\
AddToMetaDefinitions(ImageProperty, fieldDefinitionOfImageProperty);\
AddToMetaDefinitions(ServerEntityInitParams, fieldDefinitionOfServerEntityInitParams);\
AddToMetaDefinitions(InventorySlots, fieldDefinitionOfInventorySlots);\
AddToMetaDefinitions(CommonEntityInitParams, fieldDefinitionOfCommonEntityInitParams);\
AddToMetaDefinitions(CraftingComponent, fieldDefinitionOfCraftingComponent);\
AddToMetaDefinitions(PossibleActionDefinition, fieldDefinitionOfPossibleActionDefinition);\
AddToMetaDefinitions(EquipLayout, fieldDefinitionOfEquipLayout);\
AddToMetaDefinitions(UseLayout, fieldDefinitionOfUseLayout);\
AddToMetaDefinitions(EntityID, fieldDefinitionOfEntityID);\
AddToMetaDefinitions(TileAnimationEffect, fieldDefinitionOfTileAnimationEffect);\
AddToMetaDefinitions(EntityType, fieldDefinitionOfEntityType);\
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
AddToMetaDefinitions(world_generator, fieldDefinitionOfworld_generator);\
AddToMetaDefinitions(BiomeConfiguration, fieldDefinitionOfBiomeConfiguration);\
AddToMetaDefinitions(BiomeBand, fieldDefinitionOfBiomeBand);\
AddToMetaDefinitions(ZSlice, fieldDefinitionOfZSlice);\
AddToMetaDefinitions(BiomePyramid, fieldDefinitionOfBiomePyramid);\
AddToMetaDefinitions(DrynessSelector, fieldDefinitionOfDrynessSelector);\
AddToMetaDefinitions(PropertySelector, fieldDefinitionOfPropertySelector);\
AddToMetaDefinitions(NoiseSelector, fieldDefinitionOfNoiseSelector);\
AddToMetaDefinitions(PropertyBucket, fieldDefinitionOfPropertyBucket);\
AddToMetaDefinitions(NoiseBucket, fieldDefinitionOfNoiseBucket);\
AddToMetaDefinitions(NoiseParams, fieldDefinitionOfNoiseParams);\
AddToMetaDefinitions(UniversePos, fieldDefinitionOfUniversePos);\
AddToMetaDefinitions(ProbabilityEffect, fieldDefinitionOfProbabilityEffect);\
AddToMetaDefinitions(ProbabilityEffectOption, fieldDefinitionOfProbabilityEffectOption);\
AddToMetaDefinitions(EffectBinding, fieldDefinitionOfEffectBinding);\
AddToMetaDefinitions(GameEffect, fieldDefinitionOfGameEffect);\
AddToMetaDefinitions(GameEffectInstance, fieldDefinitionOfGameEffectInstance);

#define META_PROPERTIES_ADD()\
AddToMetaProperties(variant, MetaTable_variant);\
AddToMetaProperties(fenotype, MetaTable_fenotype);\
AddToMetaProperties(boundType, MetaTable_boundType);\
AddToMetaProperties(specialPropertyType, MetaTable_specialPropertyType);\
AddToMetaProperties(soundTriggerType, MetaTable_soundTriggerType);\
AddToMetaProperties(frameIndex, MetaTable_frameIndex);\
AddToMetaProperties(dayTime, MetaTable_dayTime);\
AddToMetaProperties(season, MetaTable_season);\
AddToMetaProperties(weather, MetaTable_weather);\
AddToMetaProperties(brainType, MetaTable_brainType);\
AddToMetaProperties(inventorySlotType, MetaTable_inventorySlotType);\
AddToMetaProperties(layoutType, MetaTable_layoutType);\
AddToMetaProperties(usingSlot, MetaTable_usingSlot);\
AddToMetaProperties(equipmentSlot, MetaTable_equipmentSlot);\
AddToMetaProperties(animationEffect, MetaTable_animationEffect);\
AddToMetaProperties(gameEffect, MetaTable_gameEffect);\
AddToMetaProperties(essence, MetaTable_essence);\
AddToMetaProperties(action, MetaTable_action);\
AddToMetaProperties(fluid, MetaTable_fluid);\
AddToMetaProperties(tileType, MetaTable_tileType);

enum PropertyType
{
Property_Invalid,
Property_variant,
Property_fenotype,
Property_boundType,
Property_specialPropertyType,
Property_soundTriggerType,
Property_frameIndex,
Property_dayTime,
Property_season,
Property_weather,
Property_brainType,
Property_inventorySlotType,
Property_layoutType,
Property_usingSlot,
Property_equipmentSlot,
Property_animationEffect,
Property_gameEffect,
Property_essence,
Property_action,
Property_fluid,
Property_tileType,
Property_Count,
};
#define META_ASSET_PROPERTIES_STRINGS()\
meta_propertiesString[Property_variant] = "variant";\
meta_propertiesString[Property_fenotype] = "fenotype";\
meta_propertiesString[Property_boundType] = "boundType";\
meta_propertiesString[Property_specialPropertyType] = "specialPropertyType";\
meta_propertiesString[Property_soundTriggerType] = "soundTriggerType";\
meta_propertiesString[Property_frameIndex] = "frameIndex";\
meta_propertiesString[Property_dayTime] = "dayTime";\
meta_propertiesString[Property_season] = "season";\
meta_propertiesString[Property_weather] = "weather";\
meta_propertiesString[Property_brainType] = "brainType";\
meta_propertiesString[Property_inventorySlotType] = "inventorySlotType";\
meta_propertiesString[Property_layoutType] = "layoutType";\
meta_propertiesString[Property_usingSlot] = "usingSlot";\
meta_propertiesString[Property_equipmentSlot] = "equipmentSlot";\
meta_propertiesString[Property_animationEffect] = "animationEffect";\
meta_propertiesString[Property_gameEffect] = "gameEffect";\
meta_propertiesString[Property_essence] = "essence";\
meta_propertiesString[Property_action] = "action";\
meta_propertiesString[Property_fluid] = "fluid";\
meta_propertiesString[Property_tileType] = "tileType";\

#define META_DEFAULT_VALUES_CPP_SUCKS()\
fieldDefinitionOfGameEffect[2].def.def_GameProperty ={Property_action, none};fieldDefinitionOfGameEffect[3].def.def_GameProperty ={Property_gameEffect};fieldDefinitionOfNoiseParams[0].def.def_r32 =1;fieldDefinitionOfNoiseParams[1].def.def_u32 =1;fieldDefinitionOfNoiseParams[3].def.def_r32 =0;fieldDefinitionOfNoiseParams[4].def.def_r32 =1;fieldDefinitionOfBiomeBand[1].def.def_GameProperty ={Property_tileType, tile_invalid};fieldDefinitionOfworld_generator[5].def.def_r32 =1;fieldDefinitionOfworld_generator[6].def.def_r32 =1;fieldDefinitionOfworld_generator[7].def.def_r32 =0.05f;fieldDefinitionOfworld_generator[9].def.def_u32 =1;fieldDefinitionOfSpawnerEntity[2].def.def_r32 =0.5f;fieldDefinitionOfSpawnerEntity[3].def.def_r32 =VOXEL_SIZE;fieldDefinitionOfSpawnerEntity[4].def.def_i32 =1;fieldDefinitionOfSpawnerEntity[8].def.def_GameProperty ={Property_tileType, tile_invalid};fieldDefinitionOfSpawnerEntity[9].def.def_GameProperty ={Property_fluid, fluid_invalid};fieldDefinitionOfSpawnerEntity[11].def.def_GameProperty ={Property_tileType, tile_invalid};fieldDefinitionOfSpawnerEntity[12].def.def_GameProperty ={Property_fluid, fluid_invalid};fieldDefinitionOfSpawnerOption[0].def.def_r32 =1.0f;fieldDefinitionOfSpawnerOption[1].def.def_GameProperty ={Property_tileType, tile_invalid};fieldDefinitionOfSpawnerOption[2].def.def_GameProperty ={Property_fluid, fluid_invalid};fieldDefinitionOfSpawnerOption[4].def.def_GameProperty ={Property_tileType, tile_invalid};fieldDefinitionOfSpawnerOption[5].def.def_GameProperty ={Property_fluid, fluid_invalid};fieldDefinitionOfSpawner[2].def.def_r32 =R32_MAX;fieldDefinitionOfSpawner[3].def.def_r32 =1.0f;fieldDefinitionOfSpawner[4].def.def_r32 =0.5f;fieldDefinitionOfSpawner[5].def.def_r32 =0.5f * R32_MAX;fieldDefinitionOfSpawner[6].def.def_i32 =1;fieldDefinitionOfSpawner[7].def.def_i32 =0;fieldDefinitionOfGroundColorationArrayTest[0].def.def_u32 =2;fieldDefinitionOfGroundColorationArrayTest[1].def.def_u32 =3;fieldDefinitionOfground_coloration[0].def.def_Vec4 =V4(1, 0, 1, 1);fieldDefinitionOfground_coloration[3].def.def_GameAssetType ={AssetType_Font, 0};fieldDefinitionOftile_definition[0].def.def_GameAssetType ={AssetType_Image, 0};fieldDefinitionOfTileAnimationEffect[1].def.def_Vec4 =V4(1, 1, 1, 1);fieldDefinitionOfTileAnimationEffect[3].def.def_r32 =1.0f;fieldDefinitionOfTileAnimationEffect[6].def.def_u32 =1;fieldDefinitionOfTileAnimationEffect[7].def.def_r32 =0.01f;fieldDefinitionOfTileAnimationEffect[8].def.def_r32 =0.001f;fieldDefinitionOfTileAnimationEffect[9].def.def_GameAssetType ={AssetType_Image, 0};fieldDefinitionOfPossibleActionDefinition[1].def.def_r32 =1.0f;fieldDefinitionOfPossibleActionDefinition[2].def.def_r32 =1.0f;fieldDefinitionOfPossibleActionDefinition[3].def.def_GameProperty ={Property_specialPropertyType, Special_Invalid};fieldDefinitionOfCraftingComponent[2].def.def_b32 =true;fieldDefinitionOfCommonEntityInitParams[7].def.def_GameProperty ={Property_boundType, bound_invalid};fieldDefinitionOfCommonEntityInitParams[9].def.def_Vec3 =V3(1, 1, 1);fieldDefinitionOfCommonEntityInitParams[28].def.def_r32 =1.0f;fieldDefinitionOfCommonEntityInitParams[29].def.def_r32 =1.0f;fieldDefinitionOfCommonEntityInitParams[30].def.def_Vec3 =V3(1, 1, 1);fieldDefinitionOfInventorySlots[0].def.def_GameProperty ={Property_inventorySlotType, InventorySlot_Standard};fieldDefinitionOfServerEntityInitParams[4].def.def_r32 =27.0f;fieldDefinitionOfServerEntityInitParams[5].def.def_r32 =-7.8f;fieldDefinitionOfServerEntityInitParams[25].def.def_r32 =1.0f;fieldDefinitionOfServerEntityInitParams[27].def.def_r32 =1.0f;fieldDefinitionOfServerEntityInitParams[28].def.def_r32 =100.0f;fieldDefinitionOfServerEntityInitParams[29].def.def_r32 =100.0f;fieldDefinitionOfImageProperties[0].def.def_GameAssetType ={AssetType_Image, 0};fieldDefinitionOfLayoutPieceProperties[1].def.def_r32 =1.0f;fieldDefinitionOfLayoutPieceProperties[2].def.def_Color =V4(1, 1, 1, 1);fieldDefinitionOfMultipartFrameByFramePiece[0].def.def_r32 =1.0f;fieldDefinitionOfMultipartFrameByFramePiece[2].def.def_GameAssetType ={AssetType_Image, 0};fieldDefinitionOfPossibleSkin[0].def.def_GameAssetType ={AssetType_Image, 0};fieldDefinitionOfClientEntityInitParams[2].def.def_GameAssetType ={AssetType_Skeleton, 0};fieldDefinitionOfClientEntityInitParams[7].def.def_Color =V4(1, 1, 1, 1);fieldDefinitionOfClientEntityInitParams[12].def.def_Color =V4(1, 1, 1, 1);fieldDefinitionOfClientEntityInitParams[16].def.def_Color =V4(1, 1, 1, 1);fieldDefinitionOfClientEntityInitParams[20].def.def_Color =V4(1, 1, 1, 1);fieldDefinitionOfClientEntityInitParams[25].def.def_Color =V4(1, 1, 1, 1);fieldDefinitionOfClientEntityInitParams[27].def.def_r32 =0;fieldDefinitionOfClientEntityInitParams[28].def.def_r32 =0;fieldDefinitionOfClientEntityInitParams[29].def.def_r32 =0;fieldDefinitionOfClientEntityInitParams[30].def.def_r32 =0;fieldDefinitionOfClientEntityInitParams[31].def.def_r32 =0;fieldDefinitionOfClientEntityInitParams[32].def.def_u32 =1;fieldDefinitionOfClientEntityInitParams[33].def.def_u32 =10;fieldDefinitionOfClientEntityInitParams[34].def.def_u32 =1;fieldDefinitionOfClientEntityInitParams[36].def.def_Vec3 =V3(1, 1, 1);fieldDefinitionOfClientEntityInitParams[38].def.def_r32 =1.0f;fieldDefinitionOfClientEntityInitParams[39].def.def_r32 =1.0f;fieldDefinitionOfClientEntityInitParams[50].def.def_r32 =0.5f;fieldDefinitionOfClientEntityInitParams[51].def.def_Vec2 =V2(1, 1);fieldDefinitionOfClientEntityInitParams[52].def.def_Vec4 =V4(0, 0, 0, 0.5f);fieldDefinitionOfClientEntityInitParams[53].def.def_r32 =3.0f;fieldDefinitionOfClientEntityInitParams[54].def.def_r32 =3.0f;fieldDefinitionOfClientEntityInitParams[56].def.def_Vec2 =V2(400, 400);fieldDefinitionOfClientEntityInitParams[57].def.def_Vec2 =V2(200, 200);fieldDefinitionOfClientEntityInitParams[60].def.def_r32 =1.0f;fieldDefinitionOfClientEntityInitParams[62].def.def_GameAssetType ={AssetType_Image, 0};fieldDefinitionOfClientEntityInitParams[71].def.def_r32 =1.0f;fieldDefinitionOfClientEntityInitParams[72].def.def_r32 =1.0f;fieldDefinitionOfClientEntityInitParams[76].def.def_r32 =-0.05f;fieldDefinitionOfClientEntityInitParams[77].def.def_r32 =1.0f;fieldDefinitionOfClientEntityInitParams[78].def.def_r32 =1.0f;fieldDefinitionOfClientEntityInitParams[84].def.def_r32 =1.0f;fieldDefinitionOfAnimationEffectDefinition[4].def.def_Vec4 =V4(1, 1, 1, 1);fieldDefinitionOfAnimationEffectDefinition[5].def.def_r32 =1.0f;fieldDefinitionOfAnimationEffectDefinition[6].def.def_Vec3 =V3(1, 1, 1);fieldDefinitionOfPieceReplacement[0].def.def_GameAssetType ={AssetType_Image, 0};fieldDefinitionOfPieceReplacement[2].def.def_r32 =1.0f;fieldDefinitionOfPieceReplacement[3].def.def_b32 =true;fieldDefinitionOfPieceReplacement[5].def.def_b32 =true;fieldDefinitionOfParticleUpdater[3].def.def_GameAssetType ={AssetType_Image, 0};fieldDefinitionOfParticleUpdater[6].def.def_u32 =1;fieldDefinitionOfParticleEmitter[0].def.def_r32 =1.0f;fieldDefinitionOfParticleEmitter[1].def.def_r32 =1.0f;fieldDefinitionOfParticleEmitter[5].def.def_Vec3 =V3(0, 0, 1);fieldDefinitionOfParticleEmitter[7].def.def_Vec4 =V4(1, 1, 1, 1);fieldDefinitionOfParticleEmitter[13].def.def_r32 =1.0f;fieldDefinitionOfParticleEmitter[15].def.def_r32 =1.0f;fieldDefinitionOfParticlePhase[0].def.def_r32 =1.0f;fieldDefinitionOfSoundMappingDefinition[0].def.def_r32 =1.0f;fieldDefinitionOfSoundMappingDefinition[2].def.def_GameAssetType ={AssetType_Sound, 0};fieldDefinitionOfBrainParams[0].def.def_r32 =5.0f;fieldDefinitionOfBrainParams[1].def.def_r32 =5.0f;fieldDefinitionOfBrainParams[2].def.def_r32 =0.5f;fieldDefinitionOfBrainParams[3].def.def_r32 =0.5f;fieldDefinitionOfBrainParams[4].def.def_r32 =3.0f;fieldDefinitionOfBrainParams[5].def.def_r32 =8.0f;fieldDefinitionOfBrainParams[7].def.def_r32 =2.0f;fieldDefinitionOfBrainParams[8].def.def_r32 =1.0f;fieldDefinitionOfBrainParams[14].def.def_r32 =0.5f;
;
#define META_ARCHETYPES_BOTH()\
archetypeLayouts[Archetype_AnimalArchetype].totalSize = sizeof(AnimalArchetype); archetypeLayouts[Archetype_AnimalArchetype].hasUsingComponent.exists = true; archetypeLayouts[Archetype_AnimalArchetype].hasUsingComponent.init = InitUsingComponent; archetypeLayouts[Archetype_AnimalArchetype].hasUsingComponent.offset = OffsetOf(AnimalArchetype, equipped);  archetypeLayouts[Archetype_AnimalArchetype].hasEquipmentComponent.exists = true; archetypeLayouts[Archetype_AnimalArchetype].hasEquipmentComponent.init = InitEquipmentComponent; archetypeLayouts[Archetype_AnimalArchetype].hasEquipmentComponent.offset = OffsetOf(AnimalArchetype, equipment);  archetypeLayouts[Archetype_AnimalArchetype].hasInteractionComponent.exists = true; archetypeLayouts[Archetype_AnimalArchetype].hasInteractionComponent.init = InitInteractionComponent; archetypeLayouts[Archetype_AnimalArchetype].hasInteractionComponent.offset = OffsetOf(AnimalArchetype, interaction);  archetypeLayouts[Archetype_AnimalArchetype].hasHealthComponent.exists = true; archetypeLayouts[Archetype_AnimalArchetype].hasHealthComponent.init = InitHealthComponent; archetypeLayouts[Archetype_AnimalArchetype].hasHealthComponent.offset = OffsetOf(AnimalArchetype, alive);  archetypeLayouts[Archetype_AnimalArchetype].hasCombatComponent.exists = true; archetypeLayouts[Archetype_AnimalArchetype].hasCombatComponent.init = InitCombatComponent; archetypeLayouts[Archetype_AnimalArchetype].hasCombatComponent.offset = OffsetOf(AnimalArchetype, combat);  archetypeLayouts[Archetype_AnimalArchetype].hasLightComponent.exists = true; archetypeLayouts[Archetype_AnimalArchetype].hasLightComponent.init = InitLightComponent; archetypeLayouts[Archetype_AnimalArchetype].hasLightComponent.offset = OffsetOf(AnimalArchetype, light);  archetypeLayouts[Archetype_AnimalArchetype].hasActionComponent.exists = true; archetypeLayouts[Archetype_AnimalArchetype].hasActionComponent.init = InitActionComponent; archetypeLayouts[Archetype_AnimalArchetype].hasActionComponent.offset = OffsetOf(AnimalArchetype, action);  archetypeLayouts[Archetype_RockArchetype].totalSize = sizeof(RockArchetype); archetypeLayouts[Archetype_RockArchetype].hasInteractionComponent.exists = true; archetypeLayouts[Archetype_RockArchetype].hasInteractionComponent.init = InitInteractionComponent; archetypeLayouts[Archetype_RockArchetype].hasInteractionComponent.offset = OffsetOf(RockArchetype, interaction);  archetypeLayouts[Archetype_PlantArchetype].totalSize = sizeof(PlantArchetype); archetypeLayouts[Archetype_PlantArchetype].hasInteractionComponent.exists = true; archetypeLayouts[Archetype_PlantArchetype].hasInteractionComponent.init = InitInteractionComponent; archetypeLayouts[Archetype_PlantArchetype].hasInteractionComponent.offset = OffsetOf(PlantArchetype, interaction);  archetypeLayouts[Archetype_PlantArchetype].hasVegetationComponent.exists = true; archetypeLayouts[Archetype_PlantArchetype].hasVegetationComponent.init = InitVegetationComponent; archetypeLayouts[Archetype_PlantArchetype].hasVegetationComponent.offset = OffsetOf(PlantArchetype, vegetation);  archetypeLayouts[Archetype_GrassArchetype].totalSize = sizeof(GrassArchetype); archetypeLayouts[Archetype_RuneArchetype].totalSize = sizeof(RuneArchetype); archetypeLayouts[Archetype_RuneArchetype].hasContainerComponent.exists = true; archetypeLayouts[Archetype_RuneArchetype].hasContainerComponent.init = InitContainerComponent; archetypeLayouts[Archetype_RuneArchetype].hasContainerComponent.offset = OffsetOf(RuneArchetype, container);  archetypeLayouts[Archetype_RuneArchetype].hasInteractionComponent.exists = true; archetypeLayouts[Archetype_RuneArchetype].hasInteractionComponent.init = InitInteractionComponent; archetypeLayouts[Archetype_RuneArchetype].hasInteractionComponent.offset = OffsetOf(RuneArchetype, interaction);  archetypeLayouts[Archetype_RuneArchetype].hasSkillDefComponent.exists = true; archetypeLayouts[Archetype_RuneArchetype].hasSkillDefComponent.init = InitSkillDefComponent; archetypeLayouts[Archetype_RuneArchetype].hasSkillDefComponent.offset = OffsetOf(RuneArchetype, skill);  archetypeLayouts[Archetype_EssenceArchetype].totalSize = sizeof(EssenceArchetype); archetypeLayouts[Archetype_EssenceArchetype].hasInteractionComponent.exists = true; archetypeLayouts[Archetype_EssenceArchetype].hasInteractionComponent.init = InitInteractionComponent; archetypeLayouts[Archetype_EssenceArchetype].hasInteractionComponent.offset = OffsetOf(EssenceArchetype, interaction);  archetypeLayouts[Archetype_EssenceArchetype].hasLightComponent.exists = true; archetypeLayouts[Archetype_EssenceArchetype].hasLightComponent.init = InitLightComponent; archetypeLayouts[Archetype_EssenceArchetype].hasLightComponent.offset = OffsetOf(EssenceArchetype, light);  archetypeLayouts[Archetype_ObjectArchetype].totalSize = sizeof(ObjectArchetype); archetypeLayouts[Archetype_ObjectArchetype].hasContainerComponent.exists = true; archetypeLayouts[Archetype_ObjectArchetype].hasContainerComponent.init = InitContainerComponent; archetypeLayouts[Archetype_ObjectArchetype].hasContainerComponent.offset = OffsetOf(ObjectArchetype, container);  archetypeLayouts[Archetype_ObjectArchetype].hasInteractionComponent.exists = true; archetypeLayouts[Archetype_ObjectArchetype].hasInteractionComponent.init = InitInteractionComponent; archetypeLayouts[Archetype_ObjectArchetype].hasInteractionComponent.offset = OffsetOf(ObjectArchetype, interaction);  archetypeLayouts[Archetype_ObjectArchetype].hasLightComponent.exists = true; archetypeLayouts[Archetype_ObjectArchetype].hasLightComponent.init = InitLightComponent; archetypeLayouts[Archetype_ObjectArchetype].hasLightComponent.offset = OffsetOf(ObjectArchetype, light);  archetypeLayouts[Archetype_PortalArchetype].totalSize = sizeof(PortalArchetype); archetypeLayouts[Archetype_ProjectileArchetype].totalSize = sizeof(ProjectileArchetype); archetypeLayouts[Archetype_LightArchetype].totalSize = sizeof(LightArchetype); archetypeLayouts[Archetype_LightArchetype].hasLightComponent.exists = true; archetypeLayouts[Archetype_LightArchetype].hasLightComponent.init = InitLightComponent; archetypeLayouts[Archetype_LightArchetype].hasLightComponent.offset = OffsetOf(LightArchetype, light);  archetypeLayouts[Archetype_PlaceholderArchetype].totalSize = sizeof(PlaceholderArchetype); archetypeLayouts[Archetype_NullArchetype].totalSize = sizeof(NullArchetype); 
;
#define META_ARCHETYPES_SERVER()\
archetypeLayouts[Archetype_AnimalArchetype].hasDefaultComponent.exists = true; archetypeLayouts[Archetype_AnimalArchetype].hasDefaultComponent.init = InitDefaultComponent; archetypeLayouts[Archetype_AnimalArchetype].hasDefaultComponent.offset = OffsetOf(AnimalArchetype, default);  archetypeLayouts[Archetype_AnimalArchetype].hasMovementComponent.exists = true; archetypeLayouts[Archetype_AnimalArchetype].hasMovementComponent.init = InitMovementComponent; archetypeLayouts[Archetype_AnimalArchetype].hasMovementComponent.offset = OffsetOf(AnimalArchetype, movement);  archetypeLayouts[Archetype_AnimalArchetype].hasPlayerComponent.exists = true; archetypeLayouts[Archetype_AnimalArchetype].hasPlayerComponent.init = InitPlayerComponent; archetypeLayouts[Archetype_AnimalArchetype].hasPlayerComponent.pointer = true; archetypeLayouts[Archetype_AnimalArchetype].hasPlayerComponent.offset = OffsetOf(AnimalArchetype, player);  archetypeLayouts[Archetype_AnimalArchetype].hasBrainComponent.exists = true; archetypeLayouts[Archetype_AnimalArchetype].hasBrainComponent.init = InitBrainComponent; archetypeLayouts[Archetype_AnimalArchetype].hasBrainComponent.offset = OffsetOf(AnimalArchetype, brain);  archetypeLayouts[Archetype_AnimalArchetype].hasReachableMapComponent.exists = true; archetypeLayouts[Archetype_AnimalArchetype].hasReachableMapComponent.init = InitReachableMapComponent; archetypeLayouts[Archetype_AnimalArchetype].hasReachableMapComponent.offset = OffsetOf(AnimalArchetype, reachableMap);  archetypeLayouts[Archetype_AnimalArchetype].hasEffectComponent.exists = true; archetypeLayouts[Archetype_AnimalArchetype].hasEffectComponent.init = InitEffectComponent; archetypeLayouts[Archetype_AnimalArchetype].hasEffectComponent.offset = OffsetOf(AnimalArchetype, effects);  archetypeLayouts[Archetype_RockArchetype].hasDefaultComponent.exists = true; archetypeLayouts[Archetype_RockArchetype].hasDefaultComponent.init = InitDefaultComponent; archetypeLayouts[Archetype_RockArchetype].hasDefaultComponent.offset = OffsetOf(RockArchetype, default);  archetypeLayouts[Archetype_RockArchetype].hasEffectComponent.exists = true; archetypeLayouts[Archetype_RockArchetype].hasEffectComponent.init = InitEffectComponent; archetypeLayouts[Archetype_RockArchetype].hasEffectComponent.offset = OffsetOf(RockArchetype, effect);  archetypeLayouts[Archetype_PlantArchetype].hasDefaultComponent.exists = true; archetypeLayouts[Archetype_PlantArchetype].hasDefaultComponent.init = InitDefaultComponent; archetypeLayouts[Archetype_PlantArchetype].hasDefaultComponent.offset = OffsetOf(PlantArchetype, default);  archetypeLayouts[Archetype_PlantArchetype].hasEffectComponent.exists = true; archetypeLayouts[Archetype_PlantArchetype].hasEffectComponent.init = InitEffectComponent; archetypeLayouts[Archetype_PlantArchetype].hasEffectComponent.offset = OffsetOf(PlantArchetype, effect);  archetypeLayouts[Archetype_GrassArchetype].hasDefaultComponent.exists = true; archetypeLayouts[Archetype_GrassArchetype].hasDefaultComponent.init = InitDefaultComponent; archetypeLayouts[Archetype_GrassArchetype].hasDefaultComponent.offset = OffsetOf(GrassArchetype, default);  archetypeLayouts[Archetype_GrassArchetype].hasStaticComponent.exists = true; archetypeLayouts[Archetype_GrassArchetype].hasStaticComponent.init = InitStaticComponent; archetypeLayouts[Archetype_GrassArchetype].hasStaticComponent.offset = OffsetOf(GrassArchetype, staticUpdate);  archetypeLayouts[Archetype_RuneArchetype].hasDefaultComponent.exists = true; archetypeLayouts[Archetype_RuneArchetype].hasDefaultComponent.init = InitDefaultComponent; archetypeLayouts[Archetype_RuneArchetype].hasDefaultComponent.offset = OffsetOf(RuneArchetype, default);  archetypeLayouts[Archetype_RuneArchetype].hasEffectComponent.exists = true; archetypeLayouts[Archetype_RuneArchetype].hasEffectComponent.init = InitEffectComponent; archetypeLayouts[Archetype_RuneArchetype].hasEffectComponent.offset = OffsetOf(RuneArchetype, effect);  archetypeLayouts[Archetype_EssenceArchetype].hasDefaultComponent.exists = true; archetypeLayouts[Archetype_EssenceArchetype].hasDefaultComponent.init = InitDefaultComponent; archetypeLayouts[Archetype_EssenceArchetype].hasDefaultComponent.offset = OffsetOf(EssenceArchetype, default);  archetypeLayouts[Archetype_EssenceArchetype].hasEffectComponent.exists = true; archetypeLayouts[Archetype_EssenceArchetype].hasEffectComponent.init = InitEffectComponent; archetypeLayouts[Archetype_EssenceArchetype].hasEffectComponent.offset = OffsetOf(EssenceArchetype, effect);  archetypeLayouts[Archetype_ObjectArchetype].hasDefaultComponent.exists = true; archetypeLayouts[Archetype_ObjectArchetype].hasDefaultComponent.init = InitDefaultComponent; archetypeLayouts[Archetype_ObjectArchetype].hasDefaultComponent.offset = OffsetOf(ObjectArchetype, default);  archetypeLayouts[Archetype_ObjectArchetype].hasEffectComponent.exists = true; archetypeLayouts[Archetype_ObjectArchetype].hasEffectComponent.init = InitEffectComponent; archetypeLayouts[Archetype_ObjectArchetype].hasEffectComponent.offset = OffsetOf(ObjectArchetype, effect);  archetypeLayouts[Archetype_PortalArchetype].hasDefaultComponent.exists = true; archetypeLayouts[Archetype_PortalArchetype].hasDefaultComponent.init = InitDefaultComponent; archetypeLayouts[Archetype_PortalArchetype].hasDefaultComponent.offset = OffsetOf(PortalArchetype, default);  archetypeLayouts[Archetype_PortalArchetype].hasCollisionEffectsComponent.exists = true; archetypeLayouts[Archetype_PortalArchetype].hasCollisionEffectsComponent.init = InitCollisionEffectsComponent; archetypeLayouts[Archetype_PortalArchetype].hasCollisionEffectsComponent.offset = OffsetOf(PortalArchetype, collision);  archetypeLayouts[Archetype_ProjectileArchetype].hasDefaultComponent.exists = true; archetypeLayouts[Archetype_ProjectileArchetype].hasDefaultComponent.init = InitDefaultComponent; archetypeLayouts[Archetype_ProjectileArchetype].hasDefaultComponent.offset = OffsetOf(ProjectileArchetype, default);  archetypeLayouts[Archetype_ProjectileArchetype].hasMovementComponent.exists = true; archetypeLayouts[Archetype_ProjectileArchetype].hasMovementComponent.init = InitMovementComponent; archetypeLayouts[Archetype_ProjectileArchetype].hasMovementComponent.offset = OffsetOf(ProjectileArchetype, movement);  archetypeLayouts[Archetype_ProjectileArchetype].hasTempEntityComponent.exists = true; archetypeLayouts[Archetype_ProjectileArchetype].hasTempEntityComponent.init = InitTempEntityComponent; archetypeLayouts[Archetype_ProjectileArchetype].hasTempEntityComponent.offset = OffsetOf(ProjectileArchetype, temp);  archetypeLayouts[Archetype_ProjectileArchetype].hasCollisionEffectsComponent.exists = true; archetypeLayouts[Archetype_ProjectileArchetype].hasCollisionEffectsComponent.init = InitCollisionEffectsComponent; archetypeLayouts[Archetype_ProjectileArchetype].hasCollisionEffectsComponent.offset = OffsetOf(ProjectileArchetype, collision);  archetypeLayouts[Archetype_LightArchetype].hasDefaultComponent.exists = true; archetypeLayouts[Archetype_LightArchetype].hasDefaultComponent.init = InitDefaultComponent; archetypeLayouts[Archetype_LightArchetype].hasDefaultComponent.offset = OffsetOf(LightArchetype, default);  archetypeLayouts[Archetype_PlaceholderArchetype].hasDefaultComponent.exists = true; archetypeLayouts[Archetype_PlaceholderArchetype].hasDefaultComponent.init = InitDefaultComponent; archetypeLayouts[Archetype_PlaceholderArchetype].hasDefaultComponent.offset = OffsetOf(PlaceholderArchetype, default);  archetypeLayouts[Archetype_PlaceholderArchetype].hasPlayerComponent.exists = true; archetypeLayouts[Archetype_PlaceholderArchetype].hasPlayerComponent.init = InitPlayerComponent; archetypeLayouts[Archetype_PlaceholderArchetype].hasPlayerComponent.pointer = true; archetypeLayouts[Archetype_PlaceholderArchetype].hasPlayerComponent.offset = OffsetOf(PlaceholderArchetype, player);  
;
#define META_ARCHETYPES_CLIENT()\
archetypeLayouts[Archetype_AnimalArchetype].hasBaseComponent.exists = true; archetypeLayouts[Archetype_AnimalArchetype].hasBaseComponent.init = InitBaseComponent; archetypeLayouts[Archetype_AnimalArchetype].hasBaseComponent.offset = OffsetOf(AnimalArchetype, base);  archetypeLayouts[Archetype_AnimalArchetype].hasAnimationComponent.exists = true; archetypeLayouts[Archetype_AnimalArchetype].hasAnimationComponent.init = InitAnimationComponent; archetypeLayouts[Archetype_AnimalArchetype].hasAnimationComponent.offset = OffsetOf(AnimalArchetype, animation);  archetypeLayouts[Archetype_AnimalArchetype].hasAnimationEffectComponent.exists = true; archetypeLayouts[Archetype_AnimalArchetype].hasAnimationEffectComponent.init = InitAnimationEffectComponent; archetypeLayouts[Archetype_AnimalArchetype].hasAnimationEffectComponent.offset = OffsetOf(AnimalArchetype, animationEffects);  archetypeLayouts[Archetype_AnimalArchetype].hasSoundEffectComponent.exists = true; archetypeLayouts[Archetype_AnimalArchetype].hasSoundEffectComponent.init = InitSoundEffectComponent; archetypeLayouts[Archetype_AnimalArchetype].hasSoundEffectComponent.offset = OffsetOf(AnimalArchetype, soundEffects);  archetypeLayouts[Archetype_AnimalArchetype].hasShadowComponent.exists = true; archetypeLayouts[Archetype_AnimalArchetype].hasShadowComponent.init = InitShadowComponent; archetypeLayouts[Archetype_AnimalArchetype].hasShadowComponent.offset = OffsetOf(AnimalArchetype, shadow);  archetypeLayouts[Archetype_RockArchetype].hasBaseComponent.exists = true; archetypeLayouts[Archetype_RockArchetype].hasBaseComponent.init = InitBaseComponent; archetypeLayouts[Archetype_RockArchetype].hasBaseComponent.offset = OffsetOf(RockArchetype, base);  archetypeLayouts[Archetype_RockArchetype].hasRockComponent.exists = true; archetypeLayouts[Archetype_RockArchetype].hasRockComponent.init = InitRockComponent; archetypeLayouts[Archetype_RockArchetype].hasRockComponent.offset = OffsetOf(RockArchetype, rock);  archetypeLayouts[Archetype_RockArchetype].hasAnimationEffectComponent.exists = true; archetypeLayouts[Archetype_RockArchetype].hasAnimationEffectComponent.init = InitAnimationEffectComponent; archetypeLayouts[Archetype_RockArchetype].hasAnimationEffectComponent.offset = OffsetOf(RockArchetype, animationEffects);  archetypeLayouts[Archetype_RockArchetype].hasShadowComponent.exists = true; archetypeLayouts[Archetype_RockArchetype].hasShadowComponent.init = InitShadowComponent; archetypeLayouts[Archetype_RockArchetype].hasShadowComponent.offset = OffsetOf(RockArchetype, shadow);  archetypeLayouts[Archetype_PlantArchetype].hasBaseComponent.exists = true; archetypeLayouts[Archetype_PlantArchetype].hasBaseComponent.init = InitBaseComponent; archetypeLayouts[Archetype_PlantArchetype].hasBaseComponent.offset = OffsetOf(PlantArchetype, base);  archetypeLayouts[Archetype_PlantArchetype].hasPlantComponent.exists = true; archetypeLayouts[Archetype_PlantArchetype].hasPlantComponent.init = InitPlantComponent; archetypeLayouts[Archetype_PlantArchetype].hasPlantComponent.offset = OffsetOf(PlantArchetype, plant);  archetypeLayouts[Archetype_PlantArchetype].hasAnimationEffectComponent.exists = true; archetypeLayouts[Archetype_PlantArchetype].hasAnimationEffectComponent.init = InitAnimationEffectComponent; archetypeLayouts[Archetype_PlantArchetype].hasAnimationEffectComponent.offset = OffsetOf(PlantArchetype, animationEffects);  archetypeLayouts[Archetype_PlantArchetype].hasShadowComponent.exists = true; archetypeLayouts[Archetype_PlantArchetype].hasShadowComponent.init = InitShadowComponent; archetypeLayouts[Archetype_PlantArchetype].hasShadowComponent.offset = OffsetOf(PlantArchetype, shadow);  archetypeLayouts[Archetype_GrassArchetype].hasBaseComponent.exists = true; archetypeLayouts[Archetype_GrassArchetype].hasBaseComponent.init = InitBaseComponent; archetypeLayouts[Archetype_GrassArchetype].hasBaseComponent.offset = OffsetOf(GrassArchetype, base);  archetypeLayouts[Archetype_GrassArchetype].hasGrassComponent.exists = true; archetypeLayouts[Archetype_GrassArchetype].hasGrassComponent.init = InitGrassComponent; archetypeLayouts[Archetype_GrassArchetype].hasGrassComponent.offset = OffsetOf(GrassArchetype, grass);  archetypeLayouts[Archetype_GrassArchetype].hasMagicQuadComponent.exists = true; archetypeLayouts[Archetype_GrassArchetype].hasMagicQuadComponent.init = InitMagicQuadComponent; archetypeLayouts[Archetype_GrassArchetype].hasMagicQuadComponent.offset = OffsetOf(GrassArchetype, image);  archetypeLayouts[Archetype_RuneArchetype].hasBaseComponent.exists = true; archetypeLayouts[Archetype_RuneArchetype].hasBaseComponent.init = InitBaseComponent; archetypeLayouts[Archetype_RuneArchetype].hasBaseComponent.offset = OffsetOf(RuneArchetype, base);  archetypeLayouts[Archetype_RuneArchetype].hasLayoutComponent.exists = true; archetypeLayouts[Archetype_RuneArchetype].hasLayoutComponent.init = InitLayoutComponent; archetypeLayouts[Archetype_RuneArchetype].hasLayoutComponent.offset = OffsetOf(RuneArchetype, layout);  archetypeLayouts[Archetype_RuneArchetype].hasAnimationEffectComponent.exists = true; archetypeLayouts[Archetype_RuneArchetype].hasAnimationEffectComponent.init = InitAnimationEffectComponent; archetypeLayouts[Archetype_RuneArchetype].hasAnimationEffectComponent.offset = OffsetOf(RuneArchetype, animationEffects);  archetypeLayouts[Archetype_RuneArchetype].hasShadowComponent.exists = true; archetypeLayouts[Archetype_RuneArchetype].hasShadowComponent.init = InitShadowComponent; archetypeLayouts[Archetype_RuneArchetype].hasShadowComponent.offset = OffsetOf(RuneArchetype, shadow);  archetypeLayouts[Archetype_EssenceArchetype].hasBaseComponent.exists = true; archetypeLayouts[Archetype_EssenceArchetype].hasBaseComponent.init = InitBaseComponent; archetypeLayouts[Archetype_EssenceArchetype].hasBaseComponent.offset = OffsetOf(EssenceArchetype, base);  archetypeLayouts[Archetype_EssenceArchetype].hasLayoutComponent.exists = true; archetypeLayouts[Archetype_EssenceArchetype].hasLayoutComponent.init = InitLayoutComponent; archetypeLayouts[Archetype_EssenceArchetype].hasLayoutComponent.offset = OffsetOf(EssenceArchetype, layout);  archetypeLayouts[Archetype_EssenceArchetype].hasAnimationEffectComponent.exists = true; archetypeLayouts[Archetype_EssenceArchetype].hasAnimationEffectComponent.init = InitAnimationEffectComponent; archetypeLayouts[Archetype_EssenceArchetype].hasAnimationEffectComponent.offset = OffsetOf(EssenceArchetype, animationEffects);  archetypeLayouts[Archetype_ObjectArchetype].hasBaseComponent.exists = true; archetypeLayouts[Archetype_ObjectArchetype].hasBaseComponent.init = InitBaseComponent; archetypeLayouts[Archetype_ObjectArchetype].hasBaseComponent.offset = OffsetOf(ObjectArchetype, base);  archetypeLayouts[Archetype_ObjectArchetype].hasLayoutComponent.exists = true; archetypeLayouts[Archetype_ObjectArchetype].hasLayoutComponent.init = InitLayoutComponent; archetypeLayouts[Archetype_ObjectArchetype].hasLayoutComponent.offset = OffsetOf(ObjectArchetype, layout);  archetypeLayouts[Archetype_ObjectArchetype].hasAnimationEffectComponent.exists = true; archetypeLayouts[Archetype_ObjectArchetype].hasAnimationEffectComponent.init = InitAnimationEffectComponent; archetypeLayouts[Archetype_ObjectArchetype].hasAnimationEffectComponent.offset = OffsetOf(ObjectArchetype, animationEffects);  archetypeLayouts[Archetype_ObjectArchetype].hasRecipeEssenceComponent.exists = true; archetypeLayouts[Archetype_ObjectArchetype].hasRecipeEssenceComponent.init = InitRecipeEssenceComponent; archetypeLayouts[Archetype_ObjectArchetype].hasRecipeEssenceComponent.offset = OffsetOf(ObjectArchetype, recipeEssences);  archetypeLayouts[Archetype_ObjectArchetype].hasShadowComponent.exists = true; archetypeLayouts[Archetype_ObjectArchetype].hasShadowComponent.init = InitShadowComponent; archetypeLayouts[Archetype_ObjectArchetype].hasShadowComponent.offset = OffsetOf(ObjectArchetype, shadow);  archetypeLayouts[Archetype_PortalArchetype].hasBaseComponent.exists = true; archetypeLayouts[Archetype_PortalArchetype].hasBaseComponent.init = InitBaseComponent; archetypeLayouts[Archetype_PortalArchetype].hasBaseComponent.offset = OffsetOf(PortalArchetype, base);  archetypeLayouts[Archetype_PortalArchetype].hasFrameByFrameAnimationComponent.exists = true; archetypeLayouts[Archetype_PortalArchetype].hasFrameByFrameAnimationComponent.init = InitFrameByFrameAnimationComponent; archetypeLayouts[Archetype_PortalArchetype].hasFrameByFrameAnimationComponent.offset = OffsetOf(PortalArchetype, animation);  archetypeLayouts[Archetype_PortalArchetype].hasAnimationEffectComponent.exists = true; archetypeLayouts[Archetype_PortalArchetype].hasAnimationEffectComponent.init = InitAnimationEffectComponent; archetypeLayouts[Archetype_PortalArchetype].hasAnimationEffectComponent.offset = OffsetOf(PortalArchetype, animationEffects);  archetypeLayouts[Archetype_PortalArchetype].hasShadowComponent.exists = true; archetypeLayouts[Archetype_PortalArchetype].hasShadowComponent.init = InitShadowComponent; archetypeLayouts[Archetype_PortalArchetype].hasShadowComponent.offset = OffsetOf(PortalArchetype, shadow);  archetypeLayouts[Archetype_ProjectileArchetype].hasBaseComponent.exists = true; archetypeLayouts[Archetype_ProjectileArchetype].hasBaseComponent.init = InitBaseComponent; archetypeLayouts[Archetype_ProjectileArchetype].hasBaseComponent.offset = OffsetOf(ProjectileArchetype, base);  archetypeLayouts[Archetype_ProjectileArchetype].hasSegmentImageComponent.exists = true; archetypeLayouts[Archetype_ProjectileArchetype].hasSegmentImageComponent.init = InitSegmentImageComponent; archetypeLayouts[Archetype_ProjectileArchetype].hasSegmentImageComponent.offset = OffsetOf(ProjectileArchetype, image);  archetypeLayouts[Archetype_ProjectileArchetype].hasAnimationEffectComponent.exists = true; archetypeLayouts[Archetype_ProjectileArchetype].hasAnimationEffectComponent.init = InitAnimationEffectComponent; archetypeLayouts[Archetype_ProjectileArchetype].hasAnimationEffectComponent.offset = OffsetOf(ProjectileArchetype, animationEffects);  archetypeLayouts[Archetype_LightArchetype].hasBaseComponent.exists = true; archetypeLayouts[Archetype_LightArchetype].hasBaseComponent.init = InitBaseComponent; archetypeLayouts[Archetype_LightArchetype].hasBaseComponent.offset = OffsetOf(LightArchetype, base);  archetypeLayouts[Archetype_LightArchetype].hasStandardImageComponent.exists = true; archetypeLayouts[Archetype_LightArchetype].hasStandardImageComponent.init = InitStandardImageComponent; archetypeLayouts[Archetype_LightArchetype].hasStandardImageComponent.offset = OffsetOf(LightArchetype, image);  archetypeLayouts[Archetype_PlaceholderArchetype].hasBaseComponent.exists = true; archetypeLayouts[Archetype_PlaceholderArchetype].hasBaseComponent.init = InitBaseComponent; archetypeLayouts[Archetype_PlaceholderArchetype].hasBaseComponent.offset = OffsetOf(PlaceholderArchetype, base);  
;
