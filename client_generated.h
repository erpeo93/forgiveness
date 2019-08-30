MemberDefinition memberDefinitionOfRect2[] = 
 {
{0, MetaType_Vec2, "min", (u32) (&((Rect2*)0)->min)}, 
{0, MetaType_Vec2, "max", (u32) (&((Rect2*)0)->max)}, 
};

MemberDefinition memberDefinitionOfRect3[] = 
 {
{0, MetaType_Vec3, "min", (u32) (&((Rect3*)0)->min)}, 
{0, MetaType_Vec3, "max", (u32) (&((Rect3*)0)->max)}, 
};

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

char* MetaTable_EditorWidgetType[] = 
 {
"None",
"TaxonomyTree",
"EditingTaxonomyTabs",
"Animation",
"SoundDatabase",
"GeneralButtons",
"SoundEvents",
"Components",
"Misc",
"ColorPicker",
"GroundParams",
"Debug3DModels",
"WidgetSelection",
"Count",
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

#define META_HANDLE_ADD_TO_DEFINITION_HASH()\
AddToDefinitionHash(definitionHash, Rect3, memberDefinitionOfRect3);\
AddToDefinitionHash(definitionHash, Rect2, memberDefinitionOfRect2);

