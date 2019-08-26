#pragma once

#define MAX_PASSIVE_SKILLS_ACTIVE 6
struct PassiveSkillEffects
{
    u8 effectCount;
    Effect effects[8];
};

struct SkillSlot
{
    u32 taxonomy;
    u32 level;
};
