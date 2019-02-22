#pragma once
enum ExpressionValueType
{
    ExpressionValue_r32,
    ExpressionValue_Vec3,
};

struct ExpressionValue
{
    ExpressionValueType type;
    union
    {
        r32 value_r32;
        Vec3 value_Vec3;
    };
    
    b32 quit;
};

#define ExpressionValueFunction(t)\
inline ExpressionValue ExpressionVal(t value)\
{\
    ExpressionValue result = {};\
    result.type = ExpressionValue_##t;\
    result.value_##t = value;\
    return result;\
}

ExpressionValueFunction(r32);
ExpressionValueFunction(Vec3);


struct ConsiderationParams
{
    u32 paramCount;
    ExpressionValue params[4];
};

struct ExpressionContext
{
    b32 resultSet;
    
    struct SimRegion* region;
    struct SimEntity* self;
    struct SimEntity* target;
    Object* object;
    
    u32 targetCount;
    SimEntity* targets[32];
    
    ExpressionValue result;
    ConsiderationParams params;
};

struct ResponseCurve
{
    u32 placeHolder;
};

struct Consideration
{
    char* expression;
    
    r32 bookEndMin;
    r32 bookEndMax;
    
    ResponseCurve curve;
    ConsiderationParams params;
};
