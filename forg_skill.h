#pragma once

struct Skill
{
    GameEffect effect;
};

#define MAX_ACTIVE_SKILLS 6
struct SkillComponent
{
    Skill activeSkills[MAX_ACTIVE_SKILLS];
};