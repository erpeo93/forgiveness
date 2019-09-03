#pragma once

enum MetaType
{
    MetaType_None,
    MetaType_u8,
    MetaType_i8,
    MetaType_u16,
    MetaType_i16,
    MetaType_u32,
    MetaType_i32,
    MetaType_u64,
    MetaType_i64,
    MetaType_Vec2,
    MetaType_Vec3,
    MetaType_Vec4,
    MetaType_r32,
    MetaType_b32,
    
    MetaType_GroundColorationArrayTest
};

enum MemberMetaFlags
{
    MetaFlag_Pointer = (1 << 0),
    MetaFlag_Hashed = (1 << 1),
};

union DefaultMemberValue
{
    u8 def_u8;
    i8 def_i8;
    u16 def_u16;
    i16 def_i16;
    u32 def_u32;
    i32 def_i32;
    u64 def_u64;
    i64 def_i64;
    r32 def_r32;
    b32 def_b32;
    Vec2 def_Vec2;
    Vec3 def_Vec3;
    Vec4 def_Vec4;
};

struct MemberDefinition
{
    u32 flags;
    MetaType type;
    char typeName[64];
    char* name;
    u32 offset;
    DefaultMemberValue def;
    u32 size;
};

struct StructDefinition
{
    char name[64];
    u32 size;
    u32 memberCount;
    MemberDefinition* members;
};

struct MetaFlag
{
	char* name;
	u32 value;
};

MemberDefinition memberDefinitionOfVec2[] = 
{
    {0, MetaType_r32, "r32", "x", (u32) (&((Vec2*)0)->x)}, 
    {0, MetaType_r32, "r32", "y", (u32) (&((Vec2*)0)->y)}, 
};

MemberDefinition memberDefinitionOfVec3[] = 
{
    {0, MetaType_r32, "r32", "x", (u32) (&((Vec3*)0)->x)}, 
    {0, MetaType_r32, "r32", "y", (u32) (&((Vec3*)0)->y)}, 
    {0, MetaType_r32, "r32", "z", (u32) (&((Vec3*)0)->z)}, 
    
    {0, MetaType_r32, "r32", "r", (u32) (&((Vec3*)0)->x)}, 
    {0, MetaType_r32, "r32", "g", (u32) (&((Vec3*)0)->y)}, 
    {0, MetaType_r32, "r32", "b", (u32) (&((Vec3*)0)->z)}, 
};


MemberDefinition memberDefinitionOfVec4[] = 
{
    {0, MetaType_r32, "r32", "x", (u32) (&((Vec4*)0)->x)}, 
    {0, MetaType_r32, "r32", "y", (u32) (&((Vec4*)0)->y)}, 
    {0, MetaType_r32, "r32", "z", (u32) (&((Vec4*)0)->z)}, 
    {0, MetaType_r32, "r32", "w", (u32) (&((Vec4*)0)->w)}, 
    
    {0, MetaType_r32, "r32", "r", (u32) (&((Vec4*)0)->x)}, 
    {0, MetaType_r32, "r32", "g", (u32) (&((Vec4*)0)->y)}, 
    {0, MetaType_r32, "r32", "b", (u32) (&((Vec4*)0)->z)}, 
    {0, MetaType_r32, "r32", "a", (u32) (&((Vec4*)0)->w)}, 
};
