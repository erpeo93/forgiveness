#pragma once

union BucketHashInfo
{
    u64 data[4];
};

struct RuleQuery
{
    BucketHashInfo hash;
    void* pointers[4];
};

inline void* GetPtr( RuleQuery query, u8 pointerIndex )
{
    void* result = query.pointers[pointerIndex];
    Assert( result );
    return result;
}

struct RuleParameter
{
    b32 is64Bit;
    u32 sourceOffset;
    u32 destOffset;
};

struct BucketParam
{
    RuleParameter base;
    u8 sourcePointerIndex;
    u8 destPointerIndex;
    r32 min;
    r32 max;
};

struct BucketOption
{
    u32 paramCount;
    BucketParam params[4];
    void* dataPointer;
};

struct RuleBucket
{
    BucketHashInfo hashInfo;
    u32 optionCount;
    BucketOption options[16];
    
    RuleBucket* nextInHash;
};

struct RuleDatabase
{
    //bindings?;
    RuleBucket* buckets[1024];
};


