#ifdef INTROSPECTION
MemberDefinition memberDefinitionOfCreatureComponent[] = 
 {
{ MetaType_EquipmentSlot, "equipment", (u32) ( &(( CreatureComponent* )0)->equipment ) }, 
{ MetaType_Brain, "brain", (u32) ( &(( CreatureComponent* )0)->brain ) }, 
{ MetaType_r32, "skillCooldown", (u32) ( &(( CreatureComponent* )0)->skillCooldown ) }, 
{ MetaType_r32, "strength", (u32) ( &(( CreatureComponent* )0)->strength ) }, 
{ MetaType_r32, "lifePoints", (u32) ( &(( CreatureComponent* )0)->lifePoints ) }, 
{ MetaType_r32, "maxLifePoints", (u32) ( &(( CreatureComponent* )0)->maxLifePoints ) }, 
{ MetaType_u64, "openedContainerID", (u32) ( &(( CreatureComponent* )0)->openedContainerID ) }, 
{ MetaType_Vec3, "customTargetP", (u32) ( &(( CreatureComponent* )0)->customTargetP ) }, 
{ MetaType_SimEntity, "draggingEntity", (u32) ( &(( CreatureComponent* )0)->draggingEntity ) }, 
{ MetaType_i32, "activeSkillIndex", (u32) ( &(( CreatureComponent* )0)->activeSkillIndex ) }, 
{ MetaType_u32, "skillCount", (u32) ( &(( CreatureComponent* )0)->skillCount ) }, 
{ MetaType_SkillSlot, "skills", (u32) ( &(( CreatureComponent* )0)->skills ) }, 
{ MetaType_u32, "recipeCount", (u32) ( &(( CreatureComponent* )0)->recipeCount ) }, 
{ MetaType_Recipe, "recipes", (u32) ( &(( CreatureComponent* )0)->recipes ) }, 
{ MetaType_SkillSlot, "passiveSkills", (u32) ( &(( CreatureComponent* )0)->passiveSkills ) }, 
{ MetaType_PassiveSkillEffects, "passiveSkillEffects", (u32) ( &(( CreatureComponent* )0)->passiveSkillEffects ) }, 
{ MetaType_EssenceSlot, "essences", (u32) ( &(( CreatureComponent* )0)->essences ) }, 
{ MetaType_u8, "startedAction", (u32) ( &(( CreatureComponent* )0)->startedAction ) }, 
{ MetaType_u64, "startedActionTarget", (u32) ( &(( CreatureComponent* )0)->startedActionTarget ) }, 
{ MetaType_u8, "completedAction", (u32) ( &(( CreatureComponent* )0)->completedAction ) }, 
{ MetaType_u64, "completedActionTarget", (u32) ( &(( CreatureComponent* )0)->completedActionTarget ) }, 
{ MetaType_u32, "nextFree", (u32) ( &(( CreatureComponent* )0)->nextFree ) }, 
};
#endif
#ifdef INTROSPECTION
MemberDefinition memberDefinitionOfSimEntity[] = 
 {
{ MetaType_u32, "taxonomy", (u32) ( &(( SimEntity* )0)->taxonomy ) }, 
{ MetaType_u32, "flags", (u32) ( &(( SimEntity* )0)->flags ) }, 
{ MetaType_u64, "identifier", (u32) ( &(( SimEntity* )0)->identifier ) }, 
{ MetaType_u32, "playerID", (u32) ( &(( SimEntity* )0)->playerID ) }, 
{ MetaType_Vec3, "P", (u32) ( &(( SimEntity* )0)->P ) }, 
{ MetaType_Vec3, "velocity", (u32) ( &(( SimEntity* )0)->velocity ) }, 
{ MetaType_Vec3, "acceleration", (u32) ( &(( SimEntity* )0)->acceleration ) }, 
{ MetaType_EntityAction, "action", (u32) ( &(( SimEntity* )0)->action ) }, 
{ MetaType_r32, "actionTime", (u32) ( &(( SimEntity* )0)->actionTime ) }, 
{ MetaType_u64, "targetID", (u32) ( &(( SimEntity* )0)->targetID ) }, 
{ MetaType_r32, "distanceToTravel", (u32) ( &(( SimEntity* )0)->distanceToTravel ) }, 
{ MetaType_ForgBoundType, "boundType", (u32) ( &(( SimEntity* )0)->boundType ) }, 
{ MetaType_Rect3, "bounds", (u32) ( &(( SimEntity* )0)->bounds ) }, 
{ MetaType_r32, "quantity", (u32) ( &(( SimEntity* )0)->quantity ) }, 
{ MetaType_r32, "status", (u32) ( &(( SimEntity* )0)->status ) }, 
{ MetaType_u32, "recipeTaxonomy", (u32) ( &(( SimEntity* )0)->recipeTaxonomy ) }, 
{ MetaType_GenerationData, "gen", (u32) ( &(( SimEntity* )0)->gen ) }, 
{ MetaType_u64, "ownerID", (u32) ( &(( SimEntity* )0)->ownerID ) }, 
{ MetaType_u32, "IDs", (u32) ( &(( SimEntity* )0)->IDs ) }, 
};
#endif
#ifdef INTROSPECTION
MemberDefinition memberDefinitionOfRect2[] = 
 {
{ MetaType_Vec2, "min", (u32) ( &(( Rect2* )0)->min ) }, 
{ MetaType_Vec2, "max", (u32) ( &(( Rect2* )0)->max ) }, 
};
#endif
#ifdef INTROSPECTION
MemberDefinition memberDefinitionOfRect3[] = 
 {
{ MetaType_Vec3, "min", (u32) ( &(( Rect3* )0)->min ) }, 
{ MetaType_Vec3, "max", (u32) ( &(( Rect3* )0)->max ) }, 
};
#endif
char* MetaTable_EntityAction[] = 
 {
"None",
"Move",
"Examine",
"Attack",
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
{"Target",
(1 << 1)},
{"ResetAfterTimer",
(1 << 2)},
{"SendToClientOnTrigger",
(1 << 3)},
{"SendToClientOnDelete",
(1 << 4)},
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
MetaFlag MetaFlags_CanDoActionFlags[] = 
 {
{"Own",
(1 << 1)},
{"EquipmentSlot",
(1 << 2)},
{"Empty",
(1 << 3)},
};
char* MetaTable_ObjectState[] = 
 {
"Default",
"Ground",
"Open",
"GroundOpen",
"Count",
};
char* MetaTable_SoundContainerType[] = 
 {
"Random",
"Labeled",
"Sequence",
};
char* MetaTable_Material[] = 
 {
"None",
"Standard",
"Special",
"Count",
};
char* MetaTable_TagId[] = 
 {
"none",
"direction",
"skeletonFirstHalf",
"skeletonSecondHalf",
"skinFirstHalf",
"skinSecondHalf",
"codepoint",
"shotIndex",
"layerIndex",
"fontType",
"dimX",
"dimY",
"Material",
"count",
};
char* MetaTable_AssetTypeId[] = 
 {
"none",
"music",
"font",
"glyph",
"openingCutscene",
"rig",
"standing",
"moving",
"attacking",
"eating",
"casting",
"AnimationLast",
"leaf",
"trunk",
"waterRipple",
"emptySpace",
"scrollUI",
"UISphere",
"UISphereBounds",
"BookPage",
"BookElement",
"Bookmark",
"Icon",
"count",
};
MetaFlag MetaFlags_EditorRole[] = 
 {
{"SoundDesigner",
(1 << 1)},
{"Composer",
(1 << 2)},
{"GameDesigner",
(1 << 3)},
{"Writer",
(1 << 4)},
{"Animator",
(1 << 5)},
{"Artist",
(1 << 6)},
{"WebDeveloper",
(1 << 7)},
};
char* MetaTable_GroundViewMode[] = 
 {
"Voronoi",
"Tile",
"Chunk",
};
char* MetaTable_TilePointsLayout[] = 
 {
"Random",
"StraightLine",
"Pound",
};
char* MetaTable_ForgDayPhase[] = 
 {
"Day",
"Sunrise",
"Morning",
"Sunset",
"Dusk",
"Night",
};
#define META_HANDLE_TYPE_DUMP( MemberPtr, indentLevel )case MetaType_Rect3:{DEBUGTextLine( member->name ); DumpStruct( memberPtr, memberDefinitionOfRect3, ArrayCount(memberDefinitionOfRect3 ), indentLevel );} break;\
case MetaType_Rect2:{DEBUGTextLine( member->name ); DumpStruct( memberPtr, memberDefinitionOfRect2, ArrayCount(memberDefinitionOfRect2 ), indentLevel );} break;\
case MetaType_SimEntity:{DEBUGTextLine( member->name ); DumpStruct( memberPtr, memberDefinitionOfSimEntity, ArrayCount(memberDefinitionOfSimEntity ), indentLevel );} break;\
case MetaType_CreatureComponent:{DEBUGTextLine( member->name ); DumpStruct( memberPtr, memberDefinitionOfCreatureComponent, ArrayCount(memberDefinitionOfCreatureComponent ), indentLevel );} break;

