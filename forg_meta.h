#pragma once

enum MetaType
{
    MetaType_None,
    MetaType_u32,
    MetaType_u8,
    MetaType_u64,
    MetaType_i32,
    MetaType_Vec2,
    MetaType_Vec3,
    MetaType_r32,
    MetaType_b32
};

enum MemberMetaFlags
{
    MetaFlag_FreeList = (1 << 0),
    MetaFlag_Hashed = (1 << 1),
};

struct MemberDefinition
{
    u32 flags;
    MetaType type;
    char* name;
    u32 offset;
};

struct MetaFlag
{
	char* name;
	u32 value;
};
