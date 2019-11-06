#pragma once

struct Skill
{
    r32 targetTime;
    GameEffect effect;
};

#define MAX_ACTIVE_SKILLS 4
struct SkillComponent
{
    Skill activeSkills[MAX_ACTIVE_SKILLS];
};

struct SkillMapping
{
    char name[32];
    b32 targetSkill;
    Vec4 color;
};

struct SkillMappingComponent
{
    SkillMapping mappings[MAX_ACTIVE_SKILLS];
};

introspection() struct SkillDefinition
{
    GameEffect effect;
    AssetLabel name;
    b32 targetSkill;
    r32 targetTime MetaDefault("1.0f");
    Vec4 color MetaDefault("V4(1, 0, 0, 1)");
};