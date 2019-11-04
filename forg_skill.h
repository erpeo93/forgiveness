#pragma once

struct Skill
{
    GameEffect effect;
};

struct SkillComponent
{
    Skill activeSkills[6];
};