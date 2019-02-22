inline void CopyBasedOnParameter( void* sourcePtr, void* destPtr, RuleParameter* parameter )
{
    u8* dest = ( u8* ) destPtr + parameter->destOffset;
    u8* source = ( u8* ) sourcePtr + parameter->sourceOffset;
    if( parameter->is64Bit )
    {
        *( u64* ) dest = * ( u64* ) source;
    }
    else
    {
        *( u32* ) dest = * ( u32* ) source;
    }
}

inline b32 CheckBasedOnParameter( void* sourcePtr, void* destPtr, RuleParameter parameter )
{
    b32 result = false;
    
    u8* dest = ( u8* ) destPtr + parameter.destOffset;
    u8* source = ( u8* ) sourcePtr + parameter.sourceOffset;
    if( parameter.is64Bit )
    {
        result = ( *( u64* ) dest == * ( u64* ) source );
    }
    else
    {
        result = ( *( u32* ) dest == * ( u32* ) source );
    }
    
    return result;
}

inline b32 CheckRange( void* sourcePtr, u32 offset, r32 min, r32 max )
{
    r32* source = ( r32* ) ( ( u8* ) sourcePtr + offset );
    r32 value = *source;
    b32 result = ( min <= value && value <= max );
    return result;
}

inline b32 OptionMatches( BucketOption* option, RuleQuery query )
{
    b32 result = true;
    for( u32 paramIndex = 0; paramIndex < option->paramCount; ++paramIndex )
    {
        BucketParam* param = option->params + paramIndex;
        void* source = GetPtr( query, param->sourcePointerIndex );
        
        if( param->destPointerIndex )
        {
            void* dest = GetPtr( query, param->destPointerIndex );
            if( !CheckBasedOnParameter( source, dest, param->base ) )
            {
                result = false;
                break;
            }
        }
        else
        {
            result = CheckRange( source, param->base.sourceOffset, param->min, param->max );
        }
    }
    
    return result;
}

inline u32 ComputeBucketIndex( BucketHashInfo info )
{
    u64 result = 0;
    for( u32 dataIndex = 0; dataIndex < ArrayCount( info.data ); ++dataIndex )
    {
        result += info.data[dataIndex];
    }
    return ( u32 ) result;
}

inline b32 InfoEquals( BucketHashInfo i1, BucketHashInfo i2 )
{
    b32 result = true;
    for( u32 dataIndex = 0; dataIndex < ArrayCount( i1.data ); ++dataIndex )
    {
        if( i1.data[dataIndex] != i2.data[dataIndex] )
        {
            result = false;
            break;
        }
    }
    
    return result;
}

inline RuleBucket* GetBucket( RuleDatabase* database, BucketHashInfo info )
{
    RuleBucket* result = 0;
    
    u32 index = ComputeBucketIndex( info ) & ( ArrayCount( database->buckets ) - 1 );
    RuleBucket* test = database->buckets[index];
    while( test )
    {
        if( InfoEquals( info, test->hashInfo ) )
        {
            result = test;
        }
        
        test = test->nextInHash;
        
    }
    
    return result;
}

inline RuleBucket* AddBucket( RuleDatabase* database, BucketHashInfo info, MemoryPool* pool )
{
    u32 index = ComputeBucketIndex( info ) & ( ArrayCount( database->buckets ) - 1 );
    
    RuleBucket* test = database->buckets[index];
    while( test )
    {
        Assert( !InfoEquals( test->hashInfo, info ) );
        test = test->nextInHash;
    }
    
    RuleBucket* result = PushStruct( pool, RuleBucket );
    result->hashInfo = info;
    
    result->nextInHash = database->buckets[index];
    database->buckets[index] = result;
    
    return result;
}

internal void* PickBestOption( RuleDatabase* database, RuleQuery query )
{
    void* result = 0;
    
    RuleBucket* rightBucket = GetBucket( database, query.hash );
    if( rightBucket )
    {
        for( u32 optionIndex = 0; optionIndex < rightBucket->optionCount; ++optionIndex )
        {
            BucketOption* option = rightBucket->options + optionIndex;
            if( OptionMatches( option, query ) )
            {
                result = option->dataPointer;
                break;
            }
        }
    }
    return result;
}

inline BucketOption* AddOption( RuleBucket* bucket )
{
    BucketOption* result = 0;
    Assert( bucket->optionCount < ArrayCount( bucket->options ) );
    result = bucket->options + bucket->optionCount++;
    return result;
}

internal void SortOptions( RuleBucket* bucket )
{
    for( u32 bucketIndex = 0; bucketIndex < bucket->optionCount; ++bucketIndex )
    {
        BucketOption* toChange = bucket->options + bucketIndex;
        BucketOption* max = toChange;
        for( u32 bucketIndex2 = bucketIndex + 1; bucketIndex2 < bucket->optionCount; ++bucketIndex2 )
        {
            BucketOption* toTest = bucket->options + bucketIndex2;
            if( toTest->paramCount > max->paramCount )
            {
                max = toTest;
            }
        }
        
        BucketOption temp = *toChange;
        *toChange = *max;
        *max = temp;
    }
}

inline void RequireParam( BucketOption* option, u32 pointerIndex, u32 offset, r32 min, r32 max )
{
    Assert( pointerIndex < 0xff );
    Assert( option->paramCount < ArrayCount( option->params ) );
    BucketParam* param = option->params + option->paramCount++;
    
    param->base.is64Bit = false;
    param->base.sourceOffset = offset;
    
    param->sourcePointerIndex = ( u8 ) pointerIndex;
    param->destPointerIndex = 0;
    param->min = min;
    param->max = max;
}