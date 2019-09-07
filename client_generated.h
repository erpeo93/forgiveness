char* MetaTable_EntityAction[] = 
 {
"None",
"Idle",
"Move",
"Protecting",
"Rolling",
"Attack",
"Drag",
"Cast",
"Eat",
"Drink",
"Pick",
"Equip",
"Open",
"Craft",
"Die",
"Count",
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

char* MetaTable_AnimationEffectType[] = 
 {
"None",
"ChangeColor",
"SpawnParticles",
"Light",
"Bolt",
};

MetaFlag MetaFlags_AnimationEffectFlags[] = 
 {
{"ActionStart", (1 << 1)},
{"ActionCompleted", (1 << 2)},
{"DeleteWhenActionChanges", (1 << 3)},
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

FieldDefinition fieldDefinitionOfGameAssetType[] = 
 {
{0, MetaType_u16, "u16", "type", (u32) (&((GameAssetType*)0)->type), {}, sizeof(u16),"invalid",0, 0}, 
{0, MetaType_u16, "u16", "subtype", (u32) (&((GameAssetType*)0)->subtype), {}, sizeof(u16),"invalid",0, 0}, 
};

FieldDefinition fieldDefinitionOfGroundColorationArrayTest[] = 
 {
{0, MetaType_u32, "u32", "p1", (u32) (&((GroundColorationArrayTest*)0)->p1), {}, sizeof(u32),"invalid",0, 0}, 
{0, MetaType_u32, "u32", "p2", (u32) (&((GroundColorationArrayTest*)0)->p2), {}, sizeof(u32),"invalid",0, 0}, 
{0, MetaType_GameProperty, "GameProperty", "property", (u32) (&((GroundColorationArrayTest*)0)->property), {}, sizeof(GameProperty),"invalid",0, 0}, 
};

FieldDefinition fieldDefinitionOfground_coloration[] = 
 {
{0, MetaType_Vec4, "Vec4", "color", (u32) (&((ground_coloration*)0)->color), {}, sizeof(Vec4),"invalid",0, 0}, 
{0, MetaType_ArrayCounter, "ArrayCounter", "testCounter", (u32) (&((ground_coloration*)0)->testCounter), {}, sizeof(ArrayCounter),"invalid","a1", (u32)(&((ground_coloration*)0)->a1)}, 
{MetaFlag_Pointer, MetaType_GroundColorationArrayTest, "GroundColorationArrayTest", "a1", (u32) (&((ground_coloration*)0)->a1), {}, sizeof(GroundColorationArrayTest),"invalid",0, 0}, 
{0, MetaType_GameAssetType, "GameAssetType", "asset", (u32) (&((ground_coloration*)0)->asset), {}, sizeof(GameAssetType),"invalid",0, 0, "type"}, 
{0, MetaType_ArrayCounter, "ArrayCounter", "propertyCount", (u32) (&((ground_coloration*)0)->propertyCount), {}, sizeof(ArrayCounter),"invalid","properties", (u32)(&((ground_coloration*)0)->properties)}, 
{MetaFlag_Pointer, MetaType_GameProperty, "GameProperty", "properties", (u32) (&((ground_coloration*)0)->properties), {}, sizeof(GameProperty),"invalid",0, 0}, 
};

char* MetaProperties_Test[] = 
 {
"Value0",
"Value1",
};

char* MetaProperties_test2[] = 
 {
"bla",
"damaged",
};

#define META_HANDLE_ADD_TO_DEFINITION_HASH()\
AddToMetaDefinitions(ground_coloration, fieldDefinitionOfground_coloration);\
AddToMetaDefinitions(GroundColorationArrayTest, fieldDefinitionOfGroundColorationArrayTest);\
AddToMetaDefinitions(GameAssetType, fieldDefinitionOfGameAssetType);

#define META_PROPERTIES_ADD()\
AddToMetaProperties(test2, MetaProperties_test2);\
AddToMetaProperties(Test, MetaProperties_Test);

enum Propertys
{
Property_Invalid,
Property_test2,
Property_Test,
Property_Count,
};
#define META_ASSET_PROPERTIES_STRINGS()\
meta_propertiesString[Property_test2 - 1] = "test2";\
meta_propertiesString[Property_Test - 1] = "Test";\

#define META_DEFAULT_VALUES_CPP_SUCKS()\
fieldDefinitionOfGroundColorationArrayTest[0].def.def_u32 =2;fieldDefinitionOfGroundColorationArrayTest[1].def.def_u32 =3;fieldDefinitionOfground_coloration[0].def.def_Vec4 =V4(1, 0, 1, 1);fieldDefinitionOfground_coloration[3].def.def_GameAssetType ={AssetType_Font, AssetFont_debug};
;
