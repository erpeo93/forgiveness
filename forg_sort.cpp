inline void Swap(SortEntry* e1, SortEntry* e2)
{
    SortEntry temp = *e1;
    *e1 = *e2;
    *e2 = temp;
}

internal void MergeSort(SortEntry* first, u32 count, SortEntry* temp)
{
    Assert( count );
    if( count == 1 )
    {
        
    }
    else if( count == 2 )
    {
        SortEntry* e1 = first;
        SortEntry* e2 = e1 + 1;
        if( e1->sortKey > e2->sortKey )
        {
            Swap( e1, e2 );
        }
    }
    else
    {
        u32 half0 = count / 2;
        u32 half1 = count - half0;
        
        SortEntry* inHalf0 = first;
        SortEntry* inHalf1 = first + half0;
        
        SortEntry* end = first + count;
        
        MergeSort( inHalf0, half0, temp );
        MergeSort( inHalf1, half1, temp + half0 );
        
        SortEntry* readHalf0 = inHalf0;
        SortEntry* readHalf1 = inHalf1;
        
        SortEntry* out = temp;
        for( u32 entryIndex = 0; entryIndex < count; ++entryIndex )
        {
            if( readHalf0 == inHalf1 )
            {
                *out++ = *readHalf1++;
            }
            else if( readHalf1 == end )
            {
                *out++ = *readHalf0++;
            }
            else if( readHalf0->sortKey <= readHalf1->sortKey )
            {
                *out++ = *readHalf0++;
            }
            else
            {
                *out++ = *readHalf1++;
            }
        }
        
        Assert( out == ( temp + count ) );
        Assert( readHalf0 = inHalf1 );
        Assert( readHalf1 == end );
        
        for( u32 index = 0; index < count; ++ index )
        {
            first[index] = temp[index];
        }
    }
    
    
#if FORGIVENESS_INTERNAL
    for( u32 entryIndex = 0; entryIndex < count - 1; ++entryIndex )
    {
        SortEntry* e1 = first + entryIndex;
        SortEntry* e2 = e1 + 1;
        Assert( e1->sortKey <= e2->sortKey );
    }
#endif
}

internal void BubbleSort( SortEntry* first, u32 count )
{
    b32 somethingSwapped = true;
    while( somethingSwapped )
    {
        somethingSwapped = false;
        for( u32 entryIndex = 0; entryIndex < count - 1; ++entryIndex )
        {
            SortEntry* e1 = first + entryIndex;
            SortEntry* e2 = e1 + 1;
            
            if( e1->sortKey > e2->sortKey )
            {
                Swap( e1, e2 );
                somethingSwapped = true;
            }
        }
    }
}

inline u32 SortKeyToU32(r32 sortKey)
{
    u32 result = *( u32* ) &sortKey;
    if(result & 0x80000000)
    {
        result = ~result;
    }
    else
    {
        result |= 0x80000000;
    }
    return result;
}

internal void RadixSort(SortEntry* first, u32 count, SortEntry* temp, b32 reverse = false)
{
    SortEntry* source = first;
    SortEntry* dest = temp;
    
    for(u32 byteIndex = 0; byteIndex < 32; byteIndex += 8)
    {
        u32 sortKeyOffset[256] = {};
        for(u32 index = 0; index < count; ++index)
        {
            u32 radixValue = SortKeyToU32(first[index].sortKey);
            u32 radixPiece = (radixValue >> (byteIndex)) & 0xff;
            ++sortKeyOffset[radixPiece];
        }
        
        u32 total = 0;
        i32 starting = 0;
        i32 maximum = ArrayCount(sortKeyOffset);
        i32 offset = 1;
        
        if(reverse)
        {
            starting = ArrayCount(sortKeyOffset) - 1;
            maximum = -1;
            offset = -1;
        }
        
        for(i32 sortKeyIndex = starting; sortKeyIndex != maximum; sortKeyIndex += offset)
        {
            u32 countPartial = sortKeyOffset[sortKeyIndex];
            sortKeyOffset[sortKeyIndex] = total;
            total += countPartial;
        }
        
        Assert(total == count);
        for(u32 index = 0; index < count; ++index)
        {
            u32 radixValue = SortKeyToU32(source[index].sortKey);
            u32 radixPiece = ( radixValue >> ( byteIndex ) ) & 0xff;
            dest[sortKeyOffset[radixPiece]++] = source[index];
        }
        
        SortEntry* swapTemp = dest;
        dest = source;
        source = swapTemp;
    }
}

