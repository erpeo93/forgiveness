enum EntityArchetype
{Archetype_NullArchetype,
Archetype_BoltArchetype,
Archetype_ProjectileArchetype,
Archetype_PortalArchetype,
Archetype_ContainerArchetype,
Archetype_ObjectArchetype,
Archetype_GrassArchetype,
Archetype_PlantArchetype,
Archetype_RockArchetype,
Archetype_AnimalArchetype,
Archetype_Count
};
char* MetaTable_EntityArchetype[] = 
{"NullArchetype",
"BoltArchetype",
"ProjectileArchetype",
"PortalArchetype",
"ContainerArchetype",
"ObjectArchetype",
"GrassArchetype",
"PlantArchetype",
"RockArchetype",
"AnimalArchetype",
};

char* MetaTable_tileType[] = 
 {
"rock",
"grass",
"dirt",
"sand",
};

char* MetaTable_action[] = 
 {
"none",
"idle",
"move",
"attack",
"pick",
"use",
"equip",
"disequip",
"open",
"drop",
"storeInventory",
"useInventory",
"cast",
"drag",
"setOnFire",
};

char* MetaTable_status[] = 
 {
"status_frozen",
"status_poisoned",
};

char* MetaTable_essence[] = 
 {
"fire",
"water",
"earth",
};

char* MetaTable_gameEffect[] = 
 {
"spawnEntity",
"spawnEntityTowardTarget",
"spawnEntityTowardDirection",
"moveOnZSlice",
"deleteTarget",
"deleteSelf",
};

char* MetaTable_animationEffect[] = 
 {
"addLight",
"tintWithColor",
};

char* MetaTable_equipmentSlot[] = 
 {
"back",
"legs",
"arms",
};

char* MetaTable_usingSlot[] = 
 {
"leftHand",
"rightHand",
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
"InventorySlot_Standard",
"InventorySlot_Special",
};

char* MetaTable_brainType[] = 
 {
"Brain_invalid",
"Brain_test",
};

char* MetaTable_weather[] = 
 {
"Weather_rain",
"Weather_count",
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
};

FieldDefinition fieldDefinitionOfGameEffect[] = 
 {
{0, MetaType_r32, "r32", "timer", (u32) (&((GameEffect*)0)->timer), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_GameProperty, "GameProperty", "effectType", (u32) (&((GameEffect*)0)->effectType), {}, sizeof(GameProperty),"invalid",0, 0, "property"}, 
{0, MetaType_EntityRef, "EntityRef", "spawnType", (u32) (&((GameEffect*)0)->spawnType), {}, sizeof(EntityRef),"invalid",0, 0, 0}, 
};

FieldDefinition fieldDefinitionOfEffectBinding[] = 
 {
{0, MetaType_GameProperty, "GameProperty", "property", (u32) (&((EffectBinding*)0)->property), {}, sizeof(GameProperty),"invalid",0, 0, 0}, 
{0, MetaType_GameEffect, "GameEffect", "effect", (u32) (&((EffectBinding*)0)->effect), {}, sizeof(GameEffect),"invalid",0, 0, 0}, 
};

FieldDefinition fieldDefinitionOfUniversePos[] = 
 {
{0, MetaType_i32, "i32", "chunkX", (u32) (&((UniversePos*)0)->chunkX), {}, sizeof(i32),"invalid",0, 0, 0}, 
{0, MetaType_i32, "i32", "chunkY", (u32) (&((UniversePos*)0)->chunkY), {}, sizeof(i32),"invalid",0, 0, 0}, 
{0, MetaType_i32, "i32", "chunkZ", (u32) (&((UniversePos*)0)->chunkZ), {}, sizeof(i32),"invalid",0, 0, 0}, 
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

FieldDefinition fieldDefinitionOfEntityRef[] = 
 {
{0, MetaType_u32, "u32", "subtypeHashIndex", (u32) (&((EntityRef*)0)->subtypeHashIndex), {}, sizeof(u32),"invalid",0, 0, 0}, 
{0, MetaType_u16, "u16", "index", (u32) (&((EntityRef*)0)->index), {}, sizeof(u16),"invalid",0, 0, 0}, 
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
{0, MetaType_r32, "r32", "time", (u32) (&((PossibleActionDefinition*)0)->time), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_EntityRef, "EntityRef", "requiredUsingType", (u32) (&((PossibleActionDefinition*)0)->requiredUsingType), {}, sizeof(EntityRef),"invalid",0, 0, 0}, 
};

FieldDefinition fieldDefinitionOfCommonEntityInitParams[] = 
 {
{MetaFlag_Uneditable, MetaType_EntityRef, "EntityRef", "definitionID", (u32) (&((CommonEntityInitParams*)0)->definitionID), {}, sizeof(EntityRef),"invalid",0, 0, 0}, 
{0, MetaType_Vec3, "Vec3", "boundOffset", (u32) (&((CommonEntityInitParams*)0)->boundOffset), {}, sizeof(Vec3),"invalid",0, 0, 0}, 
{0, MetaType_Vec3, "Vec3", "boundDim", (u32) (&((CommonEntityInitParams*)0)->boundDim), {}, sizeof(Vec3),"invalid",0, 0, 0}, 
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
{0, MetaType_u16, "u16", "storeCount", (u32) (&((ServerEntityInitParams*)0)->storeCount), {}, sizeof(u16),"invalid",0, 0, 0}, 
{0, MetaType_u16, "u16", "specialStoreCount", (u32) (&((ServerEntityInitParams*)0)->specialStoreCount), {}, sizeof(u16),"invalid",0, 0, 0}, 
{0, MetaType_u16, "u16", "usingCount", (u32) (&((ServerEntityInitParams*)0)->usingCount), {}, sizeof(u16),"invalid",0, 0, 0}, 
{0, MetaType_u16, "u16", "specialUsingCount", (u32) (&((ServerEntityInitParams*)0)->specialUsingCount), {}, sizeof(u16),"invalid",0, 0, 0}, 
{0, MetaType_ArrayCounter, "ArrayCounter", "collisionEffectsCount", (u32) (&((ServerEntityInitParams*)0)->collisionEffectsCount), {}, sizeof(ArrayCounter),"invalid","collisionEffects", (u32)(&((ServerEntityInitParams*)0)->collisionEffects), 0}, 
{MetaFlag_Pointer, MetaType_GameEffect, "GameEffect", "collisionEffects", (u32) (&((ServerEntityInitParams*)0)->collisionEffects), {}, sizeof(GameEffect),"invalid",0, 0, 0}, 
};

FieldDefinition fieldDefinitionOfImageProperty[] = 
 {
{0, MetaType_b32, "b32", "optional", (u32) (&((ImageProperty*)0)->optional), {}, sizeof(b32),"invalid",0, 0, 0}, 
{0, MetaType_GameProperty, "GameProperty", "property", (u32) (&((ImageProperty*)0)->property), {}, sizeof(GameProperty),"invalid",0, 0, 0}, 
};

FieldDefinition fieldDefinitionOfImageProperties[] = 
 {
{0, MetaType_GameAssetType, "GameAssetType", "imageType", (u32) (&((ImageProperties*)0)->imageType), {}, sizeof(GameAssetType),"invalid",0, 0, "type"}, 
{0, MetaType_ArrayCounter, "ArrayCounter", "propertyCount", (u32) (&((ImageProperties*)0)->propertyCount), {}, sizeof(ArrayCounter),"invalid","properties", (u32)(&((ImageProperties*)0)->properties), 0}, 
{MetaFlag_Pointer, MetaType_ImageProperty, "ImageProperty", "properties", (u32) (&((ImageProperties*)0)->properties), {}, sizeof(ImageProperty),"invalid",0, 0, 0}, 
};

FieldDefinition fieldDefinitionOfLayoutPieceProperties[] = 
 {
{0, MetaType_AssetLabel, "AssetLabel", "name", (u32) (&((LayoutPieceProperties*)0)->name), {}, sizeof(AssetLabel),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "height", (u32) (&((LayoutPieceProperties*)0)->height), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_ImageProperties, "ImageProperties", "properties", (u32) (&((LayoutPieceProperties*)0)->properties), {}, sizeof(ImageProperties),"invalid",0, 0, 0}, 
{0, MetaType_Enumerator, "Enumerator", "inventorySlotType", (u32) (&((LayoutPieceProperties*)0)->inventorySlotType), {}, sizeof(Enumerator),"invalid",0, 0, 0, MetaTable_inventorySlotType, ArrayCount(MetaTable_inventorySlotType)}, 
};

FieldDefinition fieldDefinitionOfClientEntityInitParams[] = 
 {
{MetaFlag_Uneditable, MetaType_EntityID, "EntityID", "ID", (u32) (&((ClientEntityInitParams*)0)->ID), {}, sizeof(EntityID),"invalid",0, 0, 0}, 
{MetaFlag_Uneditable, MetaType_u32, "u32", "seed", (u32) (&((ClientEntityInitParams*)0)->seed), {}, sizeof(u32),"invalid",0, 0, 0}, 
{0, MetaType_GameAssetType, "GameAssetType", "skeleton", (u32) (&((ClientEntityInitParams*)0)->skeleton), {}, sizeof(GameAssetType),"invalid",0, 0, "type"}, 
{0, MetaType_GameAssetType, "GameAssetType", "skin", (u32) (&((ClientEntityInitParams*)0)->skin), {}, sizeof(GameAssetType),"invalid",0, 0, "type"}, 
{0, MetaType_ImageProperties, "ImageProperties", "entityProperties", (u32) (&((ClientEntityInitParams*)0)->entityProperties), {}, sizeof(ImageProperties),"invalid",0, 0, 0}, 
{0, MetaType_ImageProperties, "ImageProperties", "leafProperties", (u32) (&((ClientEntityInitParams*)0)->leafProperties), {}, sizeof(ImageProperties),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "windInfluence", (u32) (&((ClientEntityInitParams*)0)->windInfluence), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_AssetLabel, "AssetLabel", "name", (u32) (&((ClientEntityInitParams*)0)->name), {}, sizeof(AssetLabel),"invalid",0, 0, 0}, 
{0, MetaType_AssetLabel, "AssetLabel", "layoutRootName", (u32) (&((ClientEntityInitParams*)0)->layoutRootName), {}, sizeof(AssetLabel),"invalid",0, 0, 0}, 
{0, MetaType_ArrayCounter, "ArrayCounter", "pieceCount", (u32) (&((ClientEntityInitParams*)0)->pieceCount), {}, sizeof(ArrayCounter),"invalid","layoutPieces", (u32)(&((ClientEntityInitParams*)0)->layoutPieces), 0}, 
{MetaFlag_Pointer, MetaType_LayoutPieceProperties, "LayoutPieceProperties", "layoutPieces", (u32) (&((ClientEntityInitParams*)0)->layoutPieces), {}, sizeof(LayoutPieceProperties),"invalid",0, 0, 0}, 
{0, MetaType_ArrayCounter, "ArrayCounter", "openPieceCount", (u32) (&((ClientEntityInitParams*)0)->openPieceCount), {}, sizeof(ArrayCounter),"invalid","openLayoutPieces", (u32)(&((ClientEntityInitParams*)0)->openLayoutPieces), 0}, 
{MetaFlag_Pointer, MetaType_LayoutPieceProperties, "LayoutPieceProperties", "openLayoutPieces", (u32) (&((ClientEntityInitParams*)0)->openLayoutPieces), {}, sizeof(LayoutPieceProperties),"invalid",0, 0, 0}, 
{0, MetaType_ArrayCounter, "ArrayCounter", "usingPieceCount", (u32) (&((ClientEntityInitParams*)0)->usingPieceCount), {}, sizeof(ArrayCounter),"invalid","usingLayoutPieces", (u32)(&((ClientEntityInitParams*)0)->usingLayoutPieces), 0}, 
{MetaFlag_Pointer, MetaType_LayoutPieceProperties, "LayoutPieceProperties", "usingLayoutPieces", (u32) (&((ClientEntityInitParams*)0)->usingLayoutPieces), {}, sizeof(LayoutPieceProperties),"invalid",0, 0, 0}, 
{0, MetaType_Vec3, "Vec3", "shadowOffset", (u32) (&((ClientEntityInitParams*)0)->shadowOffset), {}, sizeof(Vec3),"invalid",0, 0, 0}, 
{0, MetaType_Vec2, "Vec2", "shadowScale", (u32) (&((ClientEntityInitParams*)0)->shadowScale), {}, sizeof(Vec2),"invalid",0, 0, 0}, 
{0, MetaType_Vec4, "Vec4", "shadowColor", (u32) (&((ClientEntityInitParams*)0)->shadowColor), {}, sizeof(Vec4),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "lootingZoomCoeff", (u32) (&((ClientEntityInitParams*)0)->lootingZoomCoeff), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_Vec2, "Vec2", "desiredOpenedDim", (u32) (&((ClientEntityInitParams*)0)->desiredOpenedDim), {}, sizeof(Vec2),"invalid",0, 0, 0}, 
{0, MetaType_Vec2, "Vec2", "desiredUsingDim", (u32) (&((ClientEntityInitParams*)0)->desiredUsingDim), {}, sizeof(Vec2),"invalid",0, 0, 0}, 
{0, MetaType_ArrayCounter, "ArrayCounter", "animationEffectsCount", (u32) (&((ClientEntityInitParams*)0)->animationEffectsCount), {}, sizeof(ArrayCounter),"invalid","animationEffects", (u32)(&((ClientEntityInitParams*)0)->animationEffects), 0}, 
{MetaFlag_Pointer, MetaType_AnimationEffectDefinition, "AnimationEffectDefinition", "animationEffects", (u32) (&((ClientEntityInitParams*)0)->animationEffects), {}, sizeof(AnimationEffectDefinition),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "frameByFrameSpeed", (u32) (&((ClientEntityInitParams*)0)->frameByFrameSpeed), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_GameAssetType, "GameAssetType", "frameByFrameImageType", (u32) (&((ClientEntityInitParams*)0)->frameByFrameImageType), {}, sizeof(GameAssetType),"invalid",0, 0, "type"}, 
};

FieldDefinition fieldDefinitionOfEntityDefinition[] = 
 {
{0, MetaType_Enumerator, "Enumerator", "archetype", (u32) (&((EntityDefinition*)0)->archetype), {}, sizeof(Enumerator),"invalid",0, 0, 0, MetaTable_EntityArchetype, ArrayCount(MetaTable_EntityArchetype)}, 
{0, MetaType_CommonEntityInitParams, "CommonEntityInitParams", "common", (u32) (&((EntityDefinition*)0)->common), {}, sizeof(CommonEntityInitParams),"invalid",0, 0, 0}, 
{0, MetaType_ServerEntityInitParams, "ServerEntityInitParams", "server", (u32) (&((EntityDefinition*)0)->server), {}, sizeof(ServerEntityInitParams),"invalid",0, 0, 0}, 
{0, MetaType_ClientEntityInitParams, "ClientEntityInitParams", "client", (u32) (&((EntityDefinition*)0)->client), {}, sizeof(ClientEntityInitParams),"invalid",0, 0, 0}, 
};

FieldDefinition fieldDefinitionOfSkillDefinition[] = 
 {
{0, MetaType_GameEffect, "GameEffect", "effect", (u32) (&((SkillDefinition*)0)->effect), {}, sizeof(GameEffect),"invalid",0, 0, 0}, 
{0, MetaType_AssetLabel, "AssetLabel", "name", (u32) (&((SkillDefinition*)0)->name), {}, sizeof(AssetLabel),"invalid",0, 0, 0}, 
{0, MetaType_b32, "b32", "targetSkill", (u32) (&((SkillDefinition*)0)->targetSkill), {}, sizeof(b32),"invalid",0, 0, 0}, 
{0, MetaType_r32, "r32", "targetTime", (u32) (&((SkillDefinition*)0)->targetTime), {}, sizeof(r32),"invalid",0, 0, 0}, 
{0, MetaType_Vec4, "Vec4", "color", (u32) (&((SkillDefinition*)0)->color), {}, sizeof(Vec4),"invalid",0, 0, 0}, 
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

FieldDefinition fieldDefinitionOfParticleUpdater[] = 
 {
{MetaFlag_Uneditable, MetaType_AssetID, "AssetID", "bitmapID", (u32) (&((ParticleUpdater*)0)->bitmapID), {}, sizeof(AssetID),"invalid",0, 0, 0}, 
{MetaFlag_Pointer|MetaFlag_Uneditable, MetaType_RenderTexture, "RenderTexture", "texture", (u32) (&((ParticleUpdater*)0)->texture), {}, sizeof(RenderTexture),"invalid",0, 0, 0}, 
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
};

#define META_HANDLE_ADD_TO_DEFINITION_HASH()\
AddToMetaDefinitions(ParticleEffect, fieldDefinitionOfParticleEffect);\
AddToMetaDefinitions(ParticlePhase, fieldDefinitionOfParticlePhase);\
AddToMetaDefinitions(ParticleEmitter, fieldDefinitionOfParticleEmitter);\
AddToMetaDefinitions(ParticleUpdater, fieldDefinitionOfParticleUpdater);\
AddToMetaDefinitions(AnimationEffectDefinition, fieldDefinitionOfAnimationEffectDefinition);\
AddToMetaDefinitions(SkillDefinition, fieldDefinitionOfSkillDefinition);\
AddToMetaDefinitions(EntityDefinition, fieldDefinitionOfEntityDefinition);\
AddToMetaDefinitions(ClientEntityInitParams, fieldDefinitionOfClientEntityInitParams);\
AddToMetaDefinitions(LayoutPieceProperties, fieldDefinitionOfLayoutPieceProperties);\
AddToMetaDefinitions(ImageProperties, fieldDefinitionOfImageProperties);\
AddToMetaDefinitions(ImageProperty, fieldDefinitionOfImageProperty);\
AddToMetaDefinitions(ServerEntityInitParams, fieldDefinitionOfServerEntityInitParams);\
AddToMetaDefinitions(CommonEntityInitParams, fieldDefinitionOfCommonEntityInitParams);\
AddToMetaDefinitions(PossibleActionDefinition, fieldDefinitionOfPossibleActionDefinition);\
AddToMetaDefinitions(EquipLayout, fieldDefinitionOfEquipLayout);\
AddToMetaDefinitions(UseLayout, fieldDefinitionOfUseLayout);\
AddToMetaDefinitions(EntityID, fieldDefinitionOfEntityID);\
AddToMetaDefinitions(EntityRef, fieldDefinitionOfEntityRef);\
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
AddToMetaDefinitions(ZSlice, fieldDefinitionOfZSlice);\
AddToMetaDefinitions(BiomePyramid, fieldDefinitionOfBiomePyramid);\
AddToMetaDefinitions(DrynessSelector, fieldDefinitionOfDrynessSelector);\
AddToMetaDefinitions(PropertySelector, fieldDefinitionOfPropertySelector);\
AddToMetaDefinitions(NoiseSelector, fieldDefinitionOfNoiseSelector);\
AddToMetaDefinitions(PropertyBucket, fieldDefinitionOfPropertyBucket);\
AddToMetaDefinitions(NoiseBucket, fieldDefinitionOfNoiseBucket);\
AddToMetaDefinitions(NoiseParams, fieldDefinitionOfNoiseParams);\
AddToMetaDefinitions(UniversePos, fieldDefinitionOfUniversePos);\
AddToMetaDefinitions(EffectBinding, fieldDefinitionOfEffectBinding);\
AddToMetaDefinitions(GameEffect, fieldDefinitionOfGameEffect);

#define META_PROPERTIES_ADD()\
AddToMetaProperties(frameIndex, MetaTable_frameIndex);\
AddToMetaProperties(weather, MetaTable_weather);\
AddToMetaProperties(brainType, MetaTable_brainType);\
AddToMetaProperties(inventorySlotType, MetaTable_inventorySlotType);\
AddToMetaProperties(layoutType, MetaTable_layoutType);\
AddToMetaProperties(usingSlot, MetaTable_usingSlot);\
AddToMetaProperties(equipmentSlot, MetaTable_equipmentSlot);\
AddToMetaProperties(animationEffect, MetaTable_animationEffect);\
AddToMetaProperties(gameEffect, MetaTable_gameEffect);\
AddToMetaProperties(essence, MetaTable_essence);\
AddToMetaProperties(status, MetaTable_status);\
AddToMetaProperties(action, MetaTable_action);\
AddToMetaProperties(tileType, MetaTable_tileType);

enum PropertyType
{
Property_Invalid,
Property_frameIndex,
Property_weather,
Property_brainType,
Property_inventorySlotType,
Property_layoutType,
Property_usingSlot,
Property_equipmentSlot,
Property_animationEffect,
Property_gameEffect,
Property_essence,
Property_status,
Property_action,
Property_tileType,
Property_Count,
};
#define META_ASSET_PROPERTIES_STRINGS()\
meta_propertiesString[Property_frameIndex] = "frameIndex";\
meta_propertiesString[Property_weather] = "weather";\
meta_propertiesString[Property_brainType] = "brainType";\
meta_propertiesString[Property_inventorySlotType] = "inventorySlotType";\
meta_propertiesString[Property_layoutType] = "layoutType";\
meta_propertiesString[Property_usingSlot] = "usingSlot";\
meta_propertiesString[Property_equipmentSlot] = "equipmentSlot";\
meta_propertiesString[Property_animationEffect] = "animationEffect";\
meta_propertiesString[Property_gameEffect] = "gameEffect";\
meta_propertiesString[Property_essence] = "essence";\
meta_propertiesString[Property_status] = "status";\
meta_propertiesString[Property_action] = "action";\
meta_propertiesString[Property_tileType] = "tileType";\

#define META_DEFAULT_VALUES_CPP_SUCKS()\
fieldDefinitionOfGameEffect[0].def.def_r32 =1;fieldDefinitionOfGameEffect[1].def.def_GameProperty ={Property_gameEffect};fieldDefinitionOfNoiseParams[0].def.def_r32 =1;fieldDefinitionOfNoiseParams[1].def.def_u32 =1;fieldDefinitionOfNoiseParams[3].def.def_r32 =0;fieldDefinitionOfNoiseParams[4].def.def_r32 =1;fieldDefinitionOfworld_generator[5].def.def_r32 =1;fieldDefinitionOfworld_generator[6].def.def_r32 =1;fieldDefinitionOfworld_generator[7].def.def_r32 =0.05f;fieldDefinitionOfworld_generator[9].def.def_u32 =1;fieldDefinitionOfSpawnerEntity[3].def.def_r32 =1;fieldDefinitionOfSpawnerEntity[4].def.def_r32 =1;fieldDefinitionOfSpawnerEntity[5].def.def_i32 =1;fieldDefinitionOfSpawnerEntity[6].def.def_i32 =0;fieldDefinitionOfSpawnerEntity[7].def.def_i32 =1;fieldDefinitionOfSpawnerOption[0].def.def_r32 =1.0f;fieldDefinitionOfSpawner[2].def.def_r32 =1.0f;fieldDefinitionOfSpawner[3].def.def_r32 =1.0f;fieldDefinitionOfGroundColorationArrayTest[0].def.def_u32 =2;fieldDefinitionOfGroundColorationArrayTest[1].def.def_u32 =3;fieldDefinitionOfground_coloration[0].def.def_Vec4 =V4(1, 0, 1, 1);fieldDefinitionOfground_coloration[3].def.def_GameAssetType ={AssetType_Font, 0};fieldDefinitionOftile_definition[0].def.def_GameAssetType ={AssetType_Image, 0};fieldDefinitionOftile_definition[2].def.def_Vec4 =V4(1, 1, 1, 1);fieldDefinitionOfPossibleActionDefinition[1].def.def_r32 =1.0f;fieldDefinitionOfCommonEntityInitParams[2].def.def_Vec3 =V3(1, 1, 1);fieldDefinitionOfServerEntityInitParams[4].def.def_r32 =27.0f;fieldDefinitionOfServerEntityInitParams[5].def.def_r32 =-7.8f;fieldDefinitionOfImageProperties[0].def.def_GameAssetType ={AssetType_Image, 0};fieldDefinitionOfLayoutPieceProperties[1].def.def_r32 =1.0f;fieldDefinitionOfClientEntityInitParams[2].def.def_GameAssetType ={AssetType_Skeleton, 0};fieldDefinitionOfClientEntityInitParams[3].def.def_GameAssetType ={AssetType_Image, 0};fieldDefinitionOfClientEntityInitParams[6].def.def_r32 =0;fieldDefinitionOfClientEntityInitParams[16].def.def_Vec2 =V2(1, 1);fieldDefinitionOfClientEntityInitParams[17].def.def_Vec4 =V4(1, 1, 1, 0.5f);fieldDefinitionOfClientEntityInitParams[18].def.def_r32 =3.0f;fieldDefinitionOfClientEntityInitParams[19].def.def_Vec2 =V2(400, 400);fieldDefinitionOfClientEntityInitParams[20].def.def_Vec2 =V2(200, 200);fieldDefinitionOfClientEntityInitParams[23].def.def_r32 =1.0f;fieldDefinitionOfClientEntityInitParams[24].def.def_GameAssetType ={AssetType_Image, 0};fieldDefinitionOfSkillDefinition[3].def.def_r32 =1.0f;fieldDefinitionOfSkillDefinition[4].def.def_Vec4 =V4(1, 0, 0, 1);fieldDefinitionOfAnimationEffectDefinition[4].def.def_Vec4 =V4(1, 1, 1, 1);fieldDefinitionOfAnimationEffectDefinition[5].def.def_r32 =1.0f;fieldDefinitionOfAnimationEffectDefinition[6].def.def_Vec3 =V3(1, 1, 1);fieldDefinitionOfParticleUpdater[2].def.def_GameAssetType ={AssetType_Image, 0};fieldDefinitionOfParticleUpdater[5].def.def_u32 =1;fieldDefinitionOfParticleEmitter[0].def.def_r32 =1.0f;fieldDefinitionOfParticleEmitter[1].def.def_r32 =1.0f;fieldDefinitionOfParticleEmitter[5].def.def_Vec3 =V3(0, 0, 1);fieldDefinitionOfParticleEmitter[7].def.def_Vec4 =V4(1, 1, 1, 1);fieldDefinitionOfParticleEmitter[13].def.def_r32 =1.0f;fieldDefinitionOfParticleEmitter[15].def.def_r32 =1.0f;fieldDefinitionOfParticlePhase[0].def.def_r32 =1.0f;
;
#define META_ARCHETYPES_BOTH()\
archetypeLayouts[Archetype_AnimalArchetype].totalSize = sizeof(AnimalArchetype); archetypeLayouts[Archetype_RockArchetype].totalSize = sizeof(RockArchetype); archetypeLayouts[Archetype_PlantArchetype].totalSize = sizeof(PlantArchetype); archetypeLayouts[Archetype_GrassArchetype].totalSize = sizeof(GrassArchetype); archetypeLayouts[Archetype_ObjectArchetype].totalSize = sizeof(ObjectArchetype); archetypeLayouts[Archetype_ContainerArchetype].totalSize = sizeof(ContainerArchetype); archetypeLayouts[Archetype_PortalArchetype].totalSize = sizeof(PortalArchetype); archetypeLayouts[Archetype_ProjectileArchetype].totalSize = sizeof(ProjectileArchetype); archetypeLayouts[Archetype_BoltArchetype].totalSize = sizeof(BoltArchetype); archetypeLayouts[Archetype_NullArchetype].totalSize = sizeof(NullArchetype); 
;
#define META_ARCHETYPES_SERVER()\
archetypeLayouts[Archetype_AnimalArchetype].hasPhysicComponent.exists = true; archetypeLayouts[Archetype_AnimalArchetype].hasPhysicComponent.init = InitPhysicComponent; archetypeLayouts[Archetype_AnimalArchetype].hasPhysicComponent.offset = OffsetOf(AnimalArchetype, physic);  archetypeLayouts[Archetype_AnimalArchetype].hasPlayerComponent.exists = true; archetypeLayouts[Archetype_AnimalArchetype].hasPlayerComponent.init = InitPlayerComponent; archetypeLayouts[Archetype_AnimalArchetype].hasPlayerComponent.pointer = true; archetypeLayouts[Archetype_AnimalArchetype].hasPlayerComponent.offset = OffsetOf(AnimalArchetype, player);  archetypeLayouts[Archetype_AnimalArchetype].hasEquipmentComponent.exists = true; archetypeLayouts[Archetype_AnimalArchetype].hasEquipmentComponent.init = InitEquipmentComponent; archetypeLayouts[Archetype_AnimalArchetype].hasEquipmentComponent.offset = OffsetOf(AnimalArchetype, equipment);  archetypeLayouts[Archetype_AnimalArchetype].hasUsingComponent.exists = true; archetypeLayouts[Archetype_AnimalArchetype].hasUsingComponent.init = InitUsingComponent; archetypeLayouts[Archetype_AnimalArchetype].hasUsingComponent.offset = OffsetOf(AnimalArchetype, equipped);  archetypeLayouts[Archetype_AnimalArchetype].hasInteractionComponent.exists = true; archetypeLayouts[Archetype_AnimalArchetype].hasInteractionComponent.init = InitInteractionComponent; archetypeLayouts[Archetype_AnimalArchetype].hasInteractionComponent.offset = OffsetOf(AnimalArchetype, interaction);  archetypeLayouts[Archetype_AnimalArchetype].hasSkillComponent.exists = true; archetypeLayouts[Archetype_AnimalArchetype].hasSkillComponent.init = InitSkillComponent; archetypeLayouts[Archetype_AnimalArchetype].hasSkillComponent.offset = OffsetOf(AnimalArchetype, skills);  archetypeLayouts[Archetype_RockArchetype].hasPhysicComponent.exists = true; archetypeLayouts[Archetype_RockArchetype].hasPhysicComponent.init = InitPhysicComponent; archetypeLayouts[Archetype_RockArchetype].hasPhysicComponent.offset = OffsetOf(RockArchetype, physic);  archetypeLayouts[Archetype_RockArchetype].hasInteractionComponent.exists = true; archetypeLayouts[Archetype_RockArchetype].hasInteractionComponent.init = InitInteractionComponent; archetypeLayouts[Archetype_RockArchetype].hasInteractionComponent.offset = OffsetOf(RockArchetype, interaction);  archetypeLayouts[Archetype_PlantArchetype].hasPhysicComponent.exists = true; archetypeLayouts[Archetype_PlantArchetype].hasPhysicComponent.init = InitPhysicComponent; archetypeLayouts[Archetype_PlantArchetype].hasPhysicComponent.offset = OffsetOf(PlantArchetype, physic);  archetypeLayouts[Archetype_PlantArchetype].hasPlayerComponent.exists = true; archetypeLayouts[Archetype_PlantArchetype].hasPlayerComponent.init = InitPlayerComponent; archetypeLayouts[Archetype_PlantArchetype].hasPlayerComponent.pointer = true; archetypeLayouts[Archetype_PlantArchetype].hasPlayerComponent.offset = OffsetOf(PlantArchetype, player);  archetypeLayouts[Archetype_PlantArchetype].hasInteractionComponent.exists = true; archetypeLayouts[Archetype_PlantArchetype].hasInteractionComponent.init = InitInteractionComponent; archetypeLayouts[Archetype_PlantArchetype].hasInteractionComponent.offset = OffsetOf(PlantArchetype, interaction);  archetypeLayouts[Archetype_GrassArchetype].hasPhysicComponent.exists = true; archetypeLayouts[Archetype_GrassArchetype].hasPhysicComponent.init = InitPhysicComponent; archetypeLayouts[Archetype_GrassArchetype].hasPhysicComponent.offset = OffsetOf(GrassArchetype, physic);  archetypeLayouts[Archetype_GrassArchetype].hasPlayerComponent.exists = true; archetypeLayouts[Archetype_GrassArchetype].hasPlayerComponent.init = InitPlayerComponent; archetypeLayouts[Archetype_GrassArchetype].hasPlayerComponent.pointer = true; archetypeLayouts[Archetype_GrassArchetype].hasPlayerComponent.offset = OffsetOf(GrassArchetype, player);  archetypeLayouts[Archetype_ObjectArchetype].hasPhysicComponent.exists = true; archetypeLayouts[Archetype_ObjectArchetype].hasPhysicComponent.init = InitPhysicComponent; archetypeLayouts[Archetype_ObjectArchetype].hasPhysicComponent.offset = OffsetOf(ObjectArchetype, physic);  archetypeLayouts[Archetype_ObjectArchetype].hasPlayerComponent.exists = true; archetypeLayouts[Archetype_ObjectArchetype].hasPlayerComponent.init = InitPlayerComponent; archetypeLayouts[Archetype_ObjectArchetype].hasPlayerComponent.pointer = true; archetypeLayouts[Archetype_ObjectArchetype].hasPlayerComponent.offset = OffsetOf(ObjectArchetype, player);  archetypeLayouts[Archetype_ObjectArchetype].hasEffectComponent.exists = true; archetypeLayouts[Archetype_ObjectArchetype].hasEffectComponent.init = InitEffectComponent; archetypeLayouts[Archetype_ObjectArchetype].hasEffectComponent.offset = OffsetOf(ObjectArchetype, effect);  archetypeLayouts[Archetype_ObjectArchetype].hasInteractionComponent.exists = true; archetypeLayouts[Archetype_ObjectArchetype].hasInteractionComponent.init = InitInteractionComponent; archetypeLayouts[Archetype_ObjectArchetype].hasInteractionComponent.offset = OffsetOf(ObjectArchetype, interaction);  archetypeLayouts[Archetype_ContainerArchetype].hasPhysicComponent.exists = true; archetypeLayouts[Archetype_ContainerArchetype].hasPhysicComponent.init = InitPhysicComponent; archetypeLayouts[Archetype_ContainerArchetype].hasPhysicComponent.offset = OffsetOf(ContainerArchetype, physic);  archetypeLayouts[Archetype_ContainerArchetype].hasEffectComponent.exists = true; archetypeLayouts[Archetype_ContainerArchetype].hasEffectComponent.init = InitEffectComponent; archetypeLayouts[Archetype_ContainerArchetype].hasEffectComponent.offset = OffsetOf(ContainerArchetype, effect);  archetypeLayouts[Archetype_ContainerArchetype].hasContainerComponent.exists = true; archetypeLayouts[Archetype_ContainerArchetype].hasContainerComponent.init = InitContainerComponent; archetypeLayouts[Archetype_ContainerArchetype].hasContainerComponent.offset = OffsetOf(ContainerArchetype, container);  archetypeLayouts[Archetype_ContainerArchetype].hasInteractionComponent.exists = true; archetypeLayouts[Archetype_ContainerArchetype].hasInteractionComponent.init = InitInteractionComponent; archetypeLayouts[Archetype_ContainerArchetype].hasInteractionComponent.offset = OffsetOf(ContainerArchetype, interaction);  archetypeLayouts[Archetype_ContainerArchetype].hasBrainComponent.exists = true; archetypeLayouts[Archetype_ContainerArchetype].hasBrainComponent.init = InitBrainComponent; archetypeLayouts[Archetype_ContainerArchetype].hasBrainComponent.offset = OffsetOf(ContainerArchetype, brain);  archetypeLayouts[Archetype_PortalArchetype].hasPhysicComponent.exists = true; archetypeLayouts[Archetype_PortalArchetype].hasPhysicComponent.init = InitPhysicComponent; archetypeLayouts[Archetype_PortalArchetype].hasPhysicComponent.offset = OffsetOf(PortalArchetype, physic);  archetypeLayouts[Archetype_PortalArchetype].hasCollisionEffectsComponent.exists = true; archetypeLayouts[Archetype_PortalArchetype].hasCollisionEffectsComponent.init = InitCollisionEffectsComponent; archetypeLayouts[Archetype_PortalArchetype].hasCollisionEffectsComponent.offset = OffsetOf(PortalArchetype, collision);  archetypeLayouts[Archetype_ProjectileArchetype].hasPhysicComponent.exists = true; archetypeLayouts[Archetype_ProjectileArchetype].hasPhysicComponent.init = InitPhysicComponent; archetypeLayouts[Archetype_ProjectileArchetype].hasPhysicComponent.offset = OffsetOf(ProjectileArchetype, physic);  archetypeLayouts[Archetype_ProjectileArchetype].hasTempEntityComponent.exists = true; archetypeLayouts[Archetype_ProjectileArchetype].hasTempEntityComponent.init = InitTempEntityComponent; archetypeLayouts[Archetype_ProjectileArchetype].hasTempEntityComponent.offset = OffsetOf(ProjectileArchetype, temp);  archetypeLayouts[Archetype_ProjectileArchetype].hasCollisionEffectsComponent.exists = true; archetypeLayouts[Archetype_ProjectileArchetype].hasCollisionEffectsComponent.init = InitCollisionEffectsComponent; archetypeLayouts[Archetype_ProjectileArchetype].hasCollisionEffectsComponent.offset = OffsetOf(ProjectileArchetype, collision);  archetypeLayouts[Archetype_BoltArchetype].hasPhysicComponent.exists = true; archetypeLayouts[Archetype_BoltArchetype].hasPhysicComponent.init = InitPhysicComponent; archetypeLayouts[Archetype_BoltArchetype].hasPhysicComponent.offset = OffsetOf(BoltArchetype, physic);  archetypeLayouts[Archetype_BoltArchetype].hasTempEntityComponent.exists = true; archetypeLayouts[Archetype_BoltArchetype].hasTempEntityComponent.init = InitTempEntityComponent; archetypeLayouts[Archetype_BoltArchetype].hasTempEntityComponent.offset = OffsetOf(BoltArchetype, temp);  archetypeLayouts[Archetype_BoltArchetype].hasCollisionEffectsComponent.exists = true; archetypeLayouts[Archetype_BoltArchetype].hasCollisionEffectsComponent.init = InitCollisionEffectsComponent; archetypeLayouts[Archetype_BoltArchetype].hasCollisionEffectsComponent.offset = OffsetOf(BoltArchetype, collision);  
;
#define META_ARCHETYPES_CLIENT()\
archetypeLayouts[Archetype_AnimalArchetype].hasBaseComponent.exists = true; archetypeLayouts[Archetype_AnimalArchetype].hasBaseComponent.init = InitBaseComponent; archetypeLayouts[Archetype_AnimalArchetype].hasBaseComponent.offset = OffsetOf(AnimalArchetype, base);  archetypeLayouts[Archetype_AnimalArchetype].hasAnimationComponent.exists = true; archetypeLayouts[Archetype_AnimalArchetype].hasAnimationComponent.init = InitAnimationComponent; archetypeLayouts[Archetype_AnimalArchetype].hasAnimationComponent.offset = OffsetOf(AnimalArchetype, animation);  archetypeLayouts[Archetype_AnimalArchetype].hasEquipmentMappingComponent.exists = true; archetypeLayouts[Archetype_AnimalArchetype].hasEquipmentMappingComponent.init = InitEquipmentMappingComponent; archetypeLayouts[Archetype_AnimalArchetype].hasEquipmentMappingComponent.offset = OffsetOf(AnimalArchetype, equipment);  archetypeLayouts[Archetype_AnimalArchetype].hasUsingMappingComponent.exists = true; archetypeLayouts[Archetype_AnimalArchetype].hasUsingMappingComponent.init = InitUsingMappingComponent; archetypeLayouts[Archetype_AnimalArchetype].hasUsingMappingComponent.offset = OffsetOf(AnimalArchetype, equipped);  archetypeLayouts[Archetype_AnimalArchetype].hasInteractionComponent.exists = true; archetypeLayouts[Archetype_AnimalArchetype].hasInteractionComponent.init = InitInteractionComponent; archetypeLayouts[Archetype_AnimalArchetype].hasInteractionComponent.offset = OffsetOf(AnimalArchetype, interaction);  archetypeLayouts[Archetype_AnimalArchetype].hasSkillMappingComponent.exists = true; archetypeLayouts[Archetype_AnimalArchetype].hasSkillMappingComponent.init = InitSkillMappingComponent; archetypeLayouts[Archetype_AnimalArchetype].hasSkillMappingComponent.offset = OffsetOf(AnimalArchetype, skillMappings);  archetypeLayouts[Archetype_AnimalArchetype].hasAnimationEffectComponent.exists = true; archetypeLayouts[Archetype_AnimalArchetype].hasAnimationEffectComponent.init = InitAnimationEffectComponent; archetypeLayouts[Archetype_AnimalArchetype].hasAnimationEffectComponent.offset = OffsetOf(AnimalArchetype, animationEffects);  archetypeLayouts[Archetype_RockArchetype].hasBaseComponent.exists = true; archetypeLayouts[Archetype_RockArchetype].hasBaseComponent.init = InitBaseComponent; archetypeLayouts[Archetype_RockArchetype].hasBaseComponent.offset = OffsetOf(RockArchetype, base);  archetypeLayouts[Archetype_RockArchetype].hasStandardImageComponent.exists = true; archetypeLayouts[Archetype_RockArchetype].hasStandardImageComponent.init = InitStandardImageComponent; archetypeLayouts[Archetype_RockArchetype].hasStandardImageComponent.offset = OffsetOf(RockArchetype, image);  archetypeLayouts[Archetype_RockArchetype].hasInteractionComponent.exists = true; archetypeLayouts[Archetype_RockArchetype].hasInteractionComponent.init = InitInteractionComponent; archetypeLayouts[Archetype_RockArchetype].hasInteractionComponent.offset = OffsetOf(RockArchetype, interaction);  archetypeLayouts[Archetype_PlantArchetype].hasBaseComponent.exists = true; archetypeLayouts[Archetype_PlantArchetype].hasBaseComponent.init = InitBaseComponent; archetypeLayouts[Archetype_PlantArchetype].hasBaseComponent.offset = OffsetOf(PlantArchetype, base);  archetypeLayouts[Archetype_PlantArchetype].hasPlantComponent.exists = true; archetypeLayouts[Archetype_PlantArchetype].hasPlantComponent.init = InitPlantComponent; archetypeLayouts[Archetype_PlantArchetype].hasPlantComponent.offset = OffsetOf(PlantArchetype, plant);  archetypeLayouts[Archetype_PlantArchetype].hasStandardImageComponent.exists = true; archetypeLayouts[Archetype_PlantArchetype].hasStandardImageComponent.init = InitStandardImageComponent; archetypeLayouts[Archetype_PlantArchetype].hasStandardImageComponent.offset = OffsetOf(PlantArchetype, image);  archetypeLayouts[Archetype_PlantArchetype].hasInteractionComponent.exists = true; archetypeLayouts[Archetype_PlantArchetype].hasInteractionComponent.init = InitInteractionComponent; archetypeLayouts[Archetype_PlantArchetype].hasInteractionComponent.offset = OffsetOf(PlantArchetype, interaction);  archetypeLayouts[Archetype_GrassArchetype].hasBaseComponent.exists = true; archetypeLayouts[Archetype_GrassArchetype].hasBaseComponent.init = InitBaseComponent; archetypeLayouts[Archetype_GrassArchetype].hasBaseComponent.offset = OffsetOf(GrassArchetype, base);  archetypeLayouts[Archetype_GrassArchetype].hasGrassComponent.exists = true; archetypeLayouts[Archetype_GrassArchetype].hasGrassComponent.init = InitGrassComponent; archetypeLayouts[Archetype_GrassArchetype].hasGrassComponent.offset = OffsetOf(GrassArchetype, grass);  archetypeLayouts[Archetype_GrassArchetype].hasMagicQuadComponent.exists = true; archetypeLayouts[Archetype_GrassArchetype].hasMagicQuadComponent.init = InitMagicQuadComponent; archetypeLayouts[Archetype_GrassArchetype].hasMagicQuadComponent.offset = OffsetOf(GrassArchetype, image);  archetypeLayouts[Archetype_ObjectArchetype].hasBaseComponent.exists = true; archetypeLayouts[Archetype_ObjectArchetype].hasBaseComponent.init = InitBaseComponent; archetypeLayouts[Archetype_ObjectArchetype].hasBaseComponent.offset = OffsetOf(ObjectArchetype, base);  archetypeLayouts[Archetype_ObjectArchetype].hasLayoutComponent.exists = true; archetypeLayouts[Archetype_ObjectArchetype].hasLayoutComponent.init = InitLayoutComponent; archetypeLayouts[Archetype_ObjectArchetype].hasLayoutComponent.offset = OffsetOf(ObjectArchetype, layout);  archetypeLayouts[Archetype_ObjectArchetype].hasInteractionComponent.exists = true; archetypeLayouts[Archetype_ObjectArchetype].hasInteractionComponent.init = InitInteractionComponent; archetypeLayouts[Archetype_ObjectArchetype].hasInteractionComponent.offset = OffsetOf(ObjectArchetype, interaction);  archetypeLayouts[Archetype_ContainerArchetype].hasBaseComponent.exists = true; archetypeLayouts[Archetype_ContainerArchetype].hasBaseComponent.init = InitBaseComponent; archetypeLayouts[Archetype_ContainerArchetype].hasBaseComponent.offset = OffsetOf(ContainerArchetype, base);  archetypeLayouts[Archetype_ContainerArchetype].hasLayoutComponent.exists = true; archetypeLayouts[Archetype_ContainerArchetype].hasLayoutComponent.init = InitLayoutComponent; archetypeLayouts[Archetype_ContainerArchetype].hasLayoutComponent.offset = OffsetOf(ContainerArchetype, layout);  archetypeLayouts[Archetype_ContainerArchetype].hasContainerMappingComponent.exists = true; archetypeLayouts[Archetype_ContainerArchetype].hasContainerMappingComponent.init = InitContainerMappingComponent; archetypeLayouts[Archetype_ContainerArchetype].hasContainerMappingComponent.offset = OffsetOf(ContainerArchetype, container);  archetypeLayouts[Archetype_ContainerArchetype].hasInteractionComponent.exists = true; archetypeLayouts[Archetype_ContainerArchetype].hasInteractionComponent.init = InitInteractionComponent; archetypeLayouts[Archetype_ContainerArchetype].hasInteractionComponent.offset = OffsetOf(ContainerArchetype, interaction);  archetypeLayouts[Archetype_PortalArchetype].hasBaseComponent.exists = true; archetypeLayouts[Archetype_PortalArchetype].hasBaseComponent.init = InitBaseComponent; archetypeLayouts[Archetype_PortalArchetype].hasBaseComponent.offset = OffsetOf(PortalArchetype, base);  archetypeLayouts[Archetype_PortalArchetype].hasFrameByFrameAnimationComponent.exists = true; archetypeLayouts[Archetype_PortalArchetype].hasFrameByFrameAnimationComponent.init = InitFrameByFrameAnimationComponent; archetypeLayouts[Archetype_PortalArchetype].hasFrameByFrameAnimationComponent.offset = OffsetOf(PortalArchetype, animation);  archetypeLayouts[Archetype_ProjectileArchetype].hasBaseComponent.exists = true; archetypeLayouts[Archetype_ProjectileArchetype].hasBaseComponent.init = InitBaseComponent; archetypeLayouts[Archetype_ProjectileArchetype].hasBaseComponent.offset = OffsetOf(ProjectileArchetype, base);  archetypeLayouts[Archetype_ProjectileArchetype].hasStandardImageComponent.exists = true; archetypeLayouts[Archetype_ProjectileArchetype].hasStandardImageComponent.init = InitStandardImageComponent; archetypeLayouts[Archetype_ProjectileArchetype].hasStandardImageComponent.offset = OffsetOf(ProjectileArchetype, image);  archetypeLayouts[Archetype_BoltArchetype].hasBaseComponent.exists = true; archetypeLayouts[Archetype_BoltArchetype].hasBaseComponent.init = InitBaseComponent; archetypeLayouts[Archetype_BoltArchetype].hasBaseComponent.offset = OffsetOf(BoltArchetype, base);  archetypeLayouts[Archetype_BoltArchetype].hasBoltComponent.exists = true; archetypeLayouts[Archetype_BoltArchetype].hasBoltComponent.init = InitBoltComponent; archetypeLayouts[Archetype_BoltArchetype].hasBoltComponent.offset = OffsetOf(BoltArchetype, bolt);  
;
