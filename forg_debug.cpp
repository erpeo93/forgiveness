#include "forg_debug.h"
#include "forg_debug_ui.cpp"

inline void BeginDebugStatistics( DebugStatistic* stats )
{
    stats->min = R32_MAX;
    stats->max = R32_MIN;
    stats->avg = 0;
    stats->count = 0;
    stats->sum = 0;
}

inline void AccumDebugStatistics( DebugStatistic* stats, r64 value )
{
    ++stats->count;
    
    stats->min = Min( value, stats->min );
    stats->max = Max( value, stats->max );
    
    stats->sum += value;
}

inline void EndDebugStatistics( DebugStatistic* stats )
{
    if( stats->count )
    {
        stats->avg = stats->sum / ( r64 ) stats->count;
    }
}

inline DebugState* DEBUGGetState()
{
    DebugState* debugState = debugGlobalMemory->debugState;
    return debugState;
}

struct DebugParsedName
{
    u32 hashValue = 0;
    u32 fileNameCount = 0;
    u32 lineNumber = 0;
    u32 nameLength;
    char* name;
};

inline DebugParsedName DebugParseName( char* GUID, char* properName )
{
    DebugParsedName result = {};
    u32 pipeCount = 0;
    
    for( char* scan = GUID; *scan; ++scan )
    {
        if( *scan == '|' )
        {
            if( pipeCount == 0 )
            {
                result.fileNameCount = ( u32 ) ( scan - GUID );
                result.lineNumber = I32FromChar( scan + 1 );
            }
            else if( pipeCount == 1 )
            {
                
            }
            ++pipeCount;
        }
        
        result.hashValue = 65599 * result.hashValue + *scan;
    }
    
#if 0
    result.nameLength = ( u32 ) ( scan - GUID ) - result.nameStartsAt;
    result.name = GUID + result.nameStartsAt;
#else
    result.nameLength = StrLen( properName );
    result.name = properName;
#endif
    
    return result;
}

inline DebugElement* GetElementFromGUID( DebugCollationState* collation, u32 hashIndex, char* GUID )
{
    DebugElement* result = 0;
    for( DebugElement* element = collation->elements[hashIndex];
        element;
        element = element->nextInHash )
    {
        if( StrEqual( element->GUID, GUID ) )
        {
            result = element;
            break;
        }
    }
    
    return result;
}

inline DebugElement* GetElementFromGUID( DebugCollationState* collation, char* GUID )
{
    DebugElement* result = 0;
    if( GUID )
    {
        DebugParsedName parsedName = DebugParseName( GUID, 0 );
        u32 hashIndex = parsedName.hashValue % ArrayCount( collation->elements );
        result = GetElementFromGUID( collation, hashIndex, GUID );
        
    }
    return result;
}


enum PrintVarFlags
{
    PrintVarFlags_prettybools = ( 1 << 1 ),
    PrintVarFlags_floatF = ( 1 << 2 ),
    PrintVarFlags_carriageReturn = ( 1 << 3 ),
    PrintVarFlags_nullTerminated = ( 1 << 4 ),
    PrintVarFlags_printGroups = ( 1 << 5 ),
    PrintVarFlags_showEntireGUID = ( 1 << 6 ),
};


internal memory_index EventToText( char* buff, char* end, DebugEvent* event, u32 flags )
{
    char* at = buff;
    memory_index result = 0;
    
    if( event->name )
    {
        char* eventName = ( flags & PrintVarFlags_showEntireGUID ) ? event->GUID : event->name;
        switch( event->type )
        {
            case DebugType_b32:
            {
                if( flags & PrintVarFlags_prettybools )
                {
                    at += FormatString( at, end - at, "%s: %s", eventName, event->Value_b32 ? "true" : "false" );
                }
                else
                {
                    at += FormatString( at, end - at, "%s %d", eventName, event->Value_b32 );
                }
            } break;
            
            case DebugType_i32:
            {
                at += FormatString( at, end - at, "%s %d", eventName, event->Value_i32 );
            } break;
            
            case DebugType_u32:
            {
                at += FormatString( at, end - at, "%s %u", eventName, event->Value_u32 );
            } break;
            
            case DebugType_r32:
            {
                at += FormatString( at, end - at, "%s %f", eventName, event->Value_r32 );
                
                if( flags & PrintVarFlags_floatF )
                {
                    *at++ = 'f';
                }
            } break;
            
            case DebugType_Vec2:
            {
                at += FormatString( at, end - at, "V2( %f, %f )", event->Value_Vec2.x, event->Value_Vec2.y );
            } break;
            
            case DebugType_Vec3:
            {
                at += FormatString( at, end - at, "V3( %f, %f, %f )", event->Value_Vec3.x, event->Value_Vec3.y, event->Value_Vec3.z );
            } break;
            
            case DebugType_Vec4:
            {
                at += FormatString( at, end - at, "V4( %f, %f, %f, %f )", event->Value_Vec4.x, event->Value_Vec4.y, event->Value_Vec4.z, event->Value_Vec4.w );
            } break;
            
            case DebugType_Rect2:
            {
                at += FormatString( at, end - at, "Rect2( %f, %f -> %f, %f )", event->Value_Rect2.min.x, event->Value_Rect2.min.y,
                                   event->Value_Rect2.max.x, event->Value_Rect2.max.y );
            } break;
            
            case DebugType_Rect3:
            {
                at += FormatString( at, end - at, "Rect3( %f, %f, %f -> %f, %f, %f )", event->Value_Rect3.min.x, event->Value_Rect3.min.y,
                                   event->Value_Rect3.min.z,
                                   event->Value_Rect3.max.x, event->Value_Rect3.max.y,
                                   event->Value_Rect3.max.z );
            } break;
            
            case DebugType_beginDataBlock:
            {
                if( flags & PrintVarFlags_printGroups )
                {
                    char* toPrint = event->GUID;
                    for( char* scan = toPrint; *scan; ++scan )
                    {
                        if( scan[0] == '/' && scan[1] != '/' )
                        {
                            toPrint = scan + 1;
                        }
                    }
                    at += FormatString( at, end - at, "%s:", toPrint );
                }
            } break;
            
            default:
            {
                at += FormatString( at, end - at, "UNHANDLED %s", event->GUID );
            } break;
        }
        
        if( flags & PrintVarFlags_carriageReturn )
        {
            *at++ = '\n';
        }
        
        if( flags & PrintVarFlags_nullTerminated )
        {
            *at++ = 0;
        }
        
        result = at - buff;
    }
    
    return result;
}

internal u64 GetTotalClock( DebugElementFrame* frame )
{
    u64 result = 0;
    for( DebugStoredEvent* event = frame->oldestEvent; event; event = event->next )
    {
        result += event->profileNode.duration;
    }
    
    return result;
}

inline DebugID GetIDFromLink( DebugTree* tree, DebugEventLink* link )
{
    DebugID result = {};
    result.value[0] = tree;
    result.value[1] = link;
    return result;
}

inline DebugID GetIDFromEvent( DebugTree* tree, DebugStoredEvent* event )
{
    DebugID result = {};
    result.value[0] = tree;
    result.value[1] = event;
    return result;
}

inline DebugID GetIDFromGUID( DebugTree* tree, char* GUID )
{
    DebugID result = {};
    result.value[0] = tree;
    result.value[1] = GUID;
    return result;
}

inline DebugView* GetViewFor( DebugState* debugState, DebugCollationState* collation, DebugID ID )
{
    DebugView* result = 0;
    
    u32 hashIndex = ( ( U32FromPointer( ID.value[0] ) >> 2 ) + ( U32FromPointer( ID.value[1] ) >> 2 ) ) % ArrayCount( collation->viewHash );
    DebugView** hashSlot = collation->viewHash + hashIndex;
    
    for( DebugView* search = *hashSlot; search; search = search->nextInHash )
    {
        if( DebugIDAreEqual( ID, search->ID ) )
        {
            result = search;
            break;
        }
    }
    
    if( !result )
    {
        result = PushStruct( &debugState->debugPool, DebugView );
        result->ID = ID;
        result->type = DebugView_unknown;
        result->nextInHash = *hashSlot;
        *hashSlot = result;
        
    }
    return result;
}

internal void DrawFrameSlider( DebugState* debugState, DebugCollationState* collation, DebugID sliderID, Rect2 bounds, Vec2 mouseP, DebugElement* rootElement )
{
    u32 frameCount = ArrayCount( rootElement->frames );
    if( frameCount > 0 )
    {
        PushRect( &debugState->renderGroup, FlatTransform(), bounds, V4( 0.0f, 0.0f, 0.0f, 0.5f ) );
        r32 barWidth = GetDim( bounds ).x / ( r32 ) frameCount;
        r32 atX = bounds.min.x;
        
        r32 thisMinY = bounds.min.y;
        r32 thisMaxY = bounds.max.y;
        for( u32 frameIndex = 0; ( frameIndex < frameCount ); ++frameIndex )
        {
            Rect2 regionRect = RectMinMax( V2( atX, thisMinY ), V2( atX + barWidth, thisMaxY ) );
            
            b32 highlight = false;
            Vec4 color = V4( 0.5f, 0.5f, 0.5f, 1.0f );
            if( frameIndex == collation->viewingFrameOrdinal )
            {
                highlight = true;
                color = V4( 1.0f, 1.0f, 0.0f, 1.0f );
            }
            
            if( frameIndex == collation->mostRecentFrameOrdinal )
            {
                highlight = true;
                color = V4( 0.0f, 1.0f, 0.0f, 1.0f );
            }
            
            if( frameIndex == collation->collatingFrameOrdinal )
            {
                highlight = true;
                color = V4( 1.0f, 0.0f, 0.0f, 1.0f );
            }
            
            if( frameIndex == collation->oldestFrameOrdinal )
            {
                highlight = true;
                color = V4( 0.0f, 0.5f, 0.0f, 1.0f );
            }
            
            if( highlight )
            {
                PushRect( &debugState->renderGroup, FlatTransform(), regionRect, color );
            }
            else
            {
                PushRectOutline( &debugState->renderGroup, FlatTransform(), regionRect, color, 2.0f );
            }
            
            if( PointInRect( regionRect, mouseP ) )
            {
                char buff[256];
                FormatString( buff, sizeof( buff ), "%u", frameIndex );
                AddTooltip( debugState, buff );
                
                DebugView* view = GetViewFor( debugState, collation, sliderID );
                collation->nextHotInteraction = SetUInt32Interaction( sliderID, &collation->viewingFrameOrdinal, frameIndex );
            }
            atX += barWidth;
        }
    }
}


global_variable Vec3 debugColors[] = 
{
    { 1.0f, 0.0f, 0.0f },
    { 0.0f, 1.0f, 0.0f },
    { 0.0f, 0.0f, 1.0f },
    { 1.0f, 1.0f, 0.0f },
    { 0.0f, 1.0f, 1.0f },
    { 1.0f, 0.0f, 1.0f },
    { 1.0f, 0.5f, 0.0f },
    { 0.0f, 0.5f, 1.0f },
    { 1.0f, 0.0f, 0.5f },
    { 0.5f, 0.0f, 0.5f },
    { 1.0f, 1.0f, 0.5f },
};

internal void DrawProfileBars( DebugState* debugState, DebugCollationState* collation, DebugID graphID, Rect2 bounds, Vec2 mouseP, DebugProfileNode* rootNode, r32 laneStride, r32 laneHeight, u32 depthRemaining )
{
    RenderGroup* renderGroup = &debugState->renderGroup;
    
    r32 frameSpan = ( r32 ) ( rootNode->duration );
    r32 pixelSpan = GetDim( bounds ).x;
    
    r32 zOffset = -20.0f * ( r32 ) depthRemaining;
    
    r32 scale = 0.0f;
    if( frameSpan > 0 )
    {
        scale = pixelSpan / frameSpan;
    }
    
    for( DebugStoredEvent* stored = rootNode->firstChild; stored; stored = stored->profileNode.nextSameParent )
    {
        DebugProfileNode* node = &stored->profileNode;
        DebugElement* element = node->element;
        
        Vec3 color = debugColors[U32FromPointer( element->GUID ) % ArrayCount( debugColors )];
        u32 laneIndex = node->threadOrdinal;
        r32 laneY = bounds.max.y - laneStride * laneIndex;
        r32 thisMinX = bounds.min.x + scale * ( node->parentRelativeClock );
        r32 thisMaxX = thisMinX + scale * ( r32 ) ( node->duration );
        
        Rect2 regionRect = RectMinMax( V2( thisMinX, laneY - laneHeight ), V2( thisMaxX, laneY ) );
        PushRectOutline( renderGroup, FlatTransform(), regionRect, V4( 0.0f, 0.0f, 0.0f, 1.0f ), 2.0f );
        PushRect( renderGroup, FlatTransform(), regionRect, V4( color, 1.0f ) );
        
        if( PointInRect( regionRect, mouseP ) )
        {
            char buff[256];
            FormatString( buff, sizeof( buff ), "%s: %ucy ", element->name, ( u32 ) node->duration );
            
            AddTooltip( debugState, buff );
            
            DebugView* view = GetViewFor( debugState, collation, graphID );
            collation->nextHotInteraction = SetPointerInteraction( graphID, ( void** ) &view->profileGraph.GUID, element->GUID );
        }
        
        if( depthRemaining > 0 )
        {
            DrawProfileBars( debugState, collation, graphID, regionRect, mouseP, node, 0, laneHeight * 0.5f, depthRemaining - 1 );
        }
    }
}

internal void DrawProfiler( DebugState* debugState, DebugCollationState* collation, DebugID graphID, Rect2 bounds, Vec2 mouseP, DebugElement* rootElement )
{
    u32 laneCount = collation->frameBarLaneCount;
    r32 laneHeight = 0.0f;
    
    if( laneCount )
    {
        laneHeight = GetDim( bounds ).y / ( r32 ) laneCount;
    }
    
    DebugElementFrame* rootFrame = rootElement->frames + collation->viewingFrameOrdinal;
    u64 totalClocks = GetTotalClock( rootFrame );
    r32 nextX = bounds.min.x;
    u64 relativeClock = 0;
    
    for( DebugStoredEvent* event = rootFrame->oldestEvent; event; event = event->next )
    {
        DebugProfileNode* node = &event->profileNode;
        Rect2 eventRect = bounds;
        
        relativeClock += node->duration;
        r32 t = ( r32 ) ( ( r64 ) relativeClock / ( r64 ) totalClocks );
        eventRect.min.x = nextX;
        eventRect.max.x = ( 1.0f - t ) * bounds.min.x + t * bounds.max.x;
        nextX = eventRect.max.x;
        
        DrawProfileBars( debugState, collation, graphID, eventRect, mouseP, node, laneHeight, laneHeight, 1 );
    }
}

internal void DrawFrameBars( DebugState* debugState, DebugCollationState* collation, DebugID graphID, Rect2 bounds, Vec2 mouseP, DebugElement* rootElement )
{
    u32 frameCount = ArrayCount( rootElement->frames );
    if( frameCount > 0 )
    {
        r32 barWidth = GetDim( bounds ).x / ( r32 ) frameCount;
        r32 atX = bounds.min.x;
        
        for( u32 frameIndex = 0; ( frameIndex < frameCount ); ++frameIndex )
        {
            DebugStoredEvent* rootEvent = rootElement->frames[frameIndex].mostRecentEvent;
            if( rootEvent )
            {
                DebugProfileNode* rootNode = &rootEvent->profileNode;
                r32 frameSpan = ( r32 ) ( rootNode->duration );
                r32 pixelSpan = GetDim( bounds ).y;
                r32 scale = 0.0f;
                if( frameSpan > 0 )
                {
                    scale = pixelSpan / frameSpan;
                }
                
                b32 highlight = ( frameIndex == collation->viewingFrameOrdinal );
                r32 highDim = highlight ? 1.0f : 0.1f;
                
                for( DebugStoredEvent* stored = rootNode->firstChild; stored; stored = stored->profileNode.nextSameParent )
                {
                    DebugProfileNode* node = &stored->profileNode;
                    DebugElement* element = node->element;
                    
                    Vec3 color = debugColors[U32FromPointer( element->GUID ) % ArrayCount( debugColors )];
                    r32 thisMinY = bounds.min.y + scale * ( node->parentRelativeClock );
                    r32 thisMaxY = thisMinY + scale * ( r32 ) ( node->duration );
                    
                    Rect2 regionRect = RectMinMax( V2( atX, thisMinY ), V2( atX + barWidth, thisMaxY ) );
                    //PushRectOutline( &debugState->renderGroup, FlatTransform(), regionRect, V4( 0.0f, 0.0f, 0.0f, 1.0f ), 2.0f );
                    //PushRect( &debugState->renderGroup, FlatTransform(), regionRect, V4( color, highDim ) );
                    
                    if( PointInRect( regionRect, mouseP ) )
                    {
                        char buff[256];
                        FormatString( buff, sizeof( buff ), "%ucy %s %s", ( u32 ) node->duration, element->GUID, element->name );
                        AddTooltip( debugState, buff );
                        
                        DebugView* view = GetViewFor( debugState, collation, graphID );
                        collation->nextHotInteraction = SetPointerInteraction( graphID, ( void** ) &view->profileGraph.GUID, element->GUID );
                    }
                }
                
                atX += barWidth;
            }
        }
    }
}

struct DebugClockEntry
{
    DebugElement* element;
    DebugStatistic stats;
};

internal void DrawTopClockList( DebugState* debugState, DebugCollationState* collation, DebugID graphID, Rect2 bounds, Vec2 mouseP, DebugElement* rootElement )
{
    u32 linkCount = 0;
    for( DebugEventLink* link = GetSentinel( collation->profileGroup )->next;
        link != GetSentinel( collation->profileGroup );
        link = link->next )
    {
        ++linkCount;
    }
    
    TempMemory temp = BeginTemporaryMemory( &debugState->debugPool );
    DebugClockEntry* entries = PushArray( temp.pool, DebugClockEntry, linkCount, NoClear() );
    SortEntry* sortA = PushArray( temp.pool, SortEntry, linkCount, NoClear() );
    SortEntry* sortB = PushArray( temp.pool, SortEntry, linkCount, NoClear() );
    
    u32 index = 0;
    r32 totalTime = 0;
    for( DebugEventLink* link = GetSentinel( collation->profileGroup )->next;
        link != GetSentinel( collation->profileGroup );
        link = link->next, ++index )
    {
        SortEntry* sort = sortA + index;
        DebugClockEntry* entry = entries + index;
        
        Assert( !HasChildren( link ) );
        entry->element = link->element;
        DebugElement* element = entry->element;
        
        BeginDebugStatistics( &entry->stats );
        
        for( DebugStoredEvent* event = element->frames[collation->viewingFrameOrdinal].oldestEvent;
            event;
            event = event->next )
        {
            u64 clocksWithChildren = event->profileNode.duration;
            u64 clocksWithoutChildren = clocksWithChildren - event->profileNode.durationOfChildren; 
            AccumDebugStatistics( &entry->stats, ( r64 ) clocksWithoutChildren );
        }
        
        EndDebugStatistics( &entry->stats );
        totalTime += ( r32 ) entry->stats.sum;
        
        sort->sortKey = ( r32 ) -entry->stats.sum;
        sort->index = index;
    }
    
    RadixSort( sortA, linkCount, sortB );
    
    r64 PC = 0;
    if( totalTime > 0 )
    {
        PC = 100.0f / totalTime;
    }
    
    Vec2 at = V2( bounds.min.x, bounds.max.y ) - V2( 0, GetBaseline( debugState ) );
    for( index = 0; index < linkCount; ++ index )
    {
        DebugClockEntry* entry = entries + sortA[index].index;
        DebugStatistic* stats = &entry->stats;
        DebugElement* element = entry->element;
        
        if( at.y < bounds.min.y )
        {
            break;
        }
        else
        {
            char buff[1024];
            FormatString( buff, sizeof( buff ), "%012ucy %3.2f%% %4d %s", ( u32 ) stats->sum, PC * stats->sum, stats->count, element->name );
            TextLineAt( debugState, buff, at );
            
            at.y -= GetLineAdvance( debugState );
        }
    }
    EndTemporaryMemory( temp );
}

internal void RewriteConfigFile( DebugState* debugState )
{
#if 0
    char buff[4096];
    char* end = buff + ArrayCount( buff );
    char* at = buff;
    
    u32 depth = 0;
    DebugEventIterator stack[64];
    stack[depth].link = debugState->rootGroup->group.next;
    stack[depth].sentinel = &debugState->rootGroup->group;
    ++depth;
    
    while( depth > 0 )
    {
        DebugEventIterator* iter = stack + ( depth - 1 );
        if( iter->link == iter->sentinel )
        {
            --depth;
        }
        else
        {
            DebugEvent* event = iter->link->event;
            iter->link = iter->link->next;
            
            if( ShouldBeWritten( event->type ) )
            {
                at += PrintOutConfigVar( at, end, event, PrintVarFlags_prefix | 
                                        PrintVarFlags_floatF | 
                                        PrintVarFlags_carriageReturn );
            }
            
            if( event->type == DebugType_VarGroup )
            {
                iter = stack + depth;
                iter->link = event->group.next;
                iter->sentinel = &event->group;
                ++depth;
            }
            
        }
    }
    
    platformAPI.WriteFile( "../code/forg_config.h", buff, ( u32 ) ( at - buff ) );
    
    if( !debugState->compiling )
    {
        debugState->compiling = true;
        debugState->compiler =platformAPI.ExecuteSystemCommand( "..\\code", "c:\\windows\\system32\\cmd.exe", "/C buildclient.bat" );
    }
#endif
}

inline DebugInteraction ElementInteraction( DebugInteractionType type, DebugID ID, DebugElement* element )
{
    DebugInteraction result = {};
    result.ID = ID;
    result.type = type;
    result.element = element;
    
    return result;
}

inline DebugInteraction DebugIDInteraction( DebugInteractionType type, DebugID ID )
{
    DebugInteraction result = {};
    result.ID = ID;
    result.type = type;
    return result;
}

inline DebugInteraction DebugLinkInteraction( DebugInteractionType type, DebugEventLink* link )
{
    DebugInteraction result = {};
    result.link = link;
    result.type = type;
    return result;
}

inline b32 IsSelected( DebugCollationState* collation, DebugID ID )
{
    b32 result = false;
    for( u32 selectedIndex = 0; 
        selectedIndex < collation->countSelectedIDs;
        ++selectedIndex )
    {
        if( DebugIDAreEqual( collation->selectedID[selectedIndex], ID ) )
        {
            result = true;
            break;
        }
    }
    return result;
}

internal void ClearSelection( DebugCollationState* collation )
{
    collation->countSelectedIDs = 0;
}

internal void AddToSelection( DebugCollationState* collation, DebugID ID )
{
    if( !IsSelected( collation, ID ) )
    {
        if( collation->countSelectedIDs < ArrayCount( collation->selectedID ) )
        {
            collation->selectedID[collation->countSelectedIDs++] = collation->hotInteraction.ID;
        }
    }
}

internal void DEBUG_HIT( DebugID ID )
{
    DebugState* debugState = DEBUGGetState();
    DebugCollationState* collation = &debugState->clientState;
    if( debugState )
    {
        collation->nextHotInteraction = DebugIDInteraction( DebugInteraction_select, ID );
        if( !DebugIDAreEqual( ID, collation->editingID ) )
        {
            collation->editingID = {};
        }
        
    }
}

internal b32 DEBUG_HIGHLIGHTED( DebugID ID, Vec4* color )
{
    DebugState* debugState = DEBUGGetState();
    DebugCollationState* collation = &debugState->clientState;
    b32 result = false;
    if( debugState )
    {
        if( IsSelected( collation, ID ) )
        {
            *color = V4( 0.0f, 1.0f, 1.0f, 1.0f );
            result = true;
        }
        else if( DebugIDAreEqual( ID, collation->editingID ) )
        {
            *color = V4( 0.0f, 1.0f, 0.0f, 1.0f );
            result = true;
        }
        else
        {
            if( DebugIDAreEqual( collation->hotInteraction.ID, ID ) )
            {
                *color = V4( 1.0f, 1.0f, 0.0f, 1.0f );
                result = true;
            }
        }
    }
    
    return result;
}

internal b32 DEBUG_EDITING( DebugID ID )
{
    DebugState* debugState = DEBUGGetState();
    DebugCollationState* collation = &debugState->clientState;
    b32 result = false;
    if( debugState )
    {
        result = DebugIDAreEqual( ID, collation->editingID );
        
    }
    return result;
}

internal b32 DEBUG_REQUESTED( DebugID ID )
{
    DebugState* debugState = DEBUGGetState();
    DebugCollationState* collation = &debugState->clientState;
    b32 result = false;
    if( debugState )
    {
        result = IsSelected( collation, ID ) ||
            DebugIDAreEqual( collation->hotInteraction.ID, ID );
        
    }
    return result;
}

internal void DEBUG_TEXT( char* text, Vec2 screenP, Vec4 color )
{
    DebugState* debugState = DEBUGGetState();
    if( debugState )
    {
        if( debugState->countPositionedText < ArrayCount( debugState->positionedText ) )
        {
            u32 textIndex = debugState->countPositionedText++;
            PositionedText* positionedText = debugState->positionedText + textIndex;
            
            Copy( StrLen( text ), positionedText->text, text );
            positionedText->text[StrLen(text)] = 0;
            positionedText->screenP = screenP;
            positionedText->color = color;
        }
    }
}

internal void DEBUG_WORLD_MOUSEP( Vec2 P )
{
    DebugState* debugState = DEBUGGetState();
    if( debugState )
    {
        debugState->mousePOffsetFromCenter = P;
    }
}

internal void DEBUG_RESET_EDITING()
{
    DebugState* debugState = DEBUGGetState();
    DebugCollationState* collation = &debugState->clientState;
    if( debugState )
    {
        collation->editingID = {};
        globalHoverEntity = 0;
    }
}

inline DebugElement* GetElementFromGUID( DebugCollationState* collation, char* GUID );
internal void DebugDrawElement( Layout* layout, DebugTree* tree, DebugElement* inElement, DebugID ID, u32 frameOrdinal )
{
    DebugState* debugState = layout->debugState;
    DebugCollationState* collation = layout->collation;
    
    RenderGroup* renderGroup = &debugState->renderGroup;
    
    DebugInteraction itemInteraction = ElementInteraction( DebugInteraction_autoModified, ID, inElement );
    b32 isHot = InteractionsAreEqual( itemInteraction, collation->hotInteraction );
    Vec4 color = isHot ? V4( 1.0f, 1.0f, 0.0f, 1.0f ) : V4( 1.0f, 1.0f, 1.0f, 1.0f );
    
    DebugStoredEvent* storedEvent = inElement->frames[collation->viewingFrameOrdinal].oldestEvent;
    DebugView* view = GetViewFor( debugState, collation, ID );
    ObjectTransform noTransform = FlatTransform();
    switch( inElement->type )
    {
        case DebugType_BitmapId:
        {
            DebugEvent* event = storedEvent ? &storedEvent->event : 0;
            Bitmap* bitmap = 0;
            if( event )
            {
                bitmap = GetBitmap( renderGroup->assets, event->Value_BitmapId, renderGroup->generationID );
                if( bitmap )
                {
                    BitmapDim dim = GetBitmapDim( bitmap, V3( 0, 0, 0 ), V3( 1, 0, 0 ), V3( 0, 1, 0 ), view->block.dim.y );
                    view->block.dim.x = dim.size.x;
                }
                
                LayoutElement element = BeginRectElement( layout, &view->block.dim );
                MakeElementSizable( layout, &element );
                EndElement( &element );
                
                PushBitmap( renderGroup, noTransform, event->Value_BitmapId, V3( element.bounds.min, 0 ), view->block.dim.y, V2( 1.0f, 1.0f ), V4( 1.0f, 1.0f, 1.0f, 1.0f ), V4(-1, -1, -1, -1));
            }
        } break;
        
        case DebugType_ThreadIntervalGraph:
        case DebugType_FrameBarGraph:
        case DebugType_TopClockList:
        {
            DebugViewProfileGraph* graph = &view->profileGraph;
            
            BeginRow( layout );
            ActionButton( layout, "Root", SetPointerInteraction( ID, ( void** ) &graph->GUID, 0 ) );
            BooleanButton( layout, "Threads", ( inElement->type == DebugType_ThreadIntervalGraph ), SetUInt32Interaction( ID, ( u32* ) &inElement->type, DebugType_ThreadIntervalGraph ) );
            BooleanButton( layout, "Frames", ( inElement->type == DebugType_FrameBarGraph ), SetUInt32Interaction( ID, ( u32* ) &inElement->type, DebugType_FrameBarGraph ) );
            BooleanButton( layout, "TopClock", ( inElement->type == DebugType_TopClockList ), SetUInt32Interaction( ID, ( u32* ) &inElement->type, DebugType_TopClockList ) );
            EndRow( layout );
            
            LayoutElement layoutElement = BeginRectElement( layout, &graph->block.dim );
            if( graph->block.dim.x == 0 && graph->block.dim.y == 0 )
            {
                graph->block.dim.x = 1200;
                graph->block.dim.y = 200;
            }
            
            MakeElementSizable( layout, &layoutElement );
            EndElement( &layoutElement );
            PushRect( &debugState->renderGroup, DefaultOverDrawTransform( -300.0f ), layoutElement.bounds, V4( 0.0f, 0.0f, 0.0f, 0.7f ) );
            
            TransientClipRect(renderGroup, GetClipRect(&debugState->renderGroup, layoutElement.bounds, 0.0f));
            
            DebugStoredEvent* rootNode = 0;
            u32 viewingFrameOrdinal = collation->viewingFrameOrdinal;
            DebugElement* viewingElement = GetElementFromGUID( collation, view->profileGraph.GUID );
            if( !viewingElement )
            {
                viewingElement = collation->rootProfileElement;
            }
            
            switch( inElement->type )
            {
                case DebugType_ThreadIntervalGraph:
                {
                    DrawProfiler( debugState, collation, ID, layoutElement.bounds, layout->mouseP, viewingElement );
                } break;
                
                case DebugType_FrameBarGraph:
                {
                    DrawFrameBars( debugState, collation, ID, layoutElement.bounds, layout->mouseP, viewingElement );
                } break;
                
                case DebugType_TopClockList:
                {
                    DrawTopClockList( debugState, collation, ID, layoutElement.bounds, layout->mouseP, viewingElement );
                } break;
            }
        } break;
        
        case DebugType_FrameSlider:
        {
            Vec2* dim = &view->block.dim;
            if( dim->x == 0 && dim->y == 0 )
            {
                dim->x = 1800;
                dim->y = 32;
            }
            
            LayoutElement layoutElement = BeginRectElement( layout, dim );
            MakeElementSizable( layout, &layoutElement );
            EndElement( &layoutElement );
            
            BeginRow( layout );
            BooleanButton( layout, "Pause", collation->paused, SetUInt32Interaction( ID, ( u32* ) &collation->paused, !collation->paused ) );
            ActionButton( layout, "Oldest", SetUInt32Interaction( ID, &collation->viewingFrameOrdinal, collation->oldestFrameOrdinal ) );
            ActionButton( layout, "Most recent", SetUInt32Interaction( ID, &collation->viewingFrameOrdinal, collation->mostRecentFrameOrdinal ) );
            EndRow( layout );
            
            DrawFrameSlider( debugState, collation, ID, layoutElement.bounds, layout->mouseP, inElement );
        } break;
        
        case DebugType_LastFrameInfo:
        {
            DebugFrame* mostRecentFrame = collation->frames + collation->viewingFrameOrdinal;
            char buff[256];
            FormatString( buff, sizeof( buff ), " last frame time: %.02f %de %dp %dd\n", mostRecentFrame->secondsElapsed * 1000.0f, mostRecentFrame->storedEventCount, mostRecentFrame->profileBlockCount, mostRecentFrame->dataBlockCount );
            BasicTextElement( layout, buff, itemInteraction );
        } break;
        
        default:
        {
            DebugEvent nullEvent = {};
            nullEvent.GUID = inElement->GUID;
            nullEvent.type = ( u8 ) inElement->type;
            DebugEvent* event = storedEvent ? &storedEvent->event : &nullEvent;
            char buff[256];
            EventToText( buff, buff + sizeof( buff ), event, PrintVarFlags_prettybools |
                        PrintVarFlags_carriageReturn | PrintVarFlags_nullTerminated |
                        PrintVarFlags_printGroups );
            BasicTextElement( layout, buff, itemInteraction );
        } break;
    }
}


internal void DrawTreeLink( DebugState* debugState, DebugCollationState* collation, Layout* layout, DebugTree* tree, DebugEventLink* link )
{
    u32 frameOrdinal = collation->viewingFrameOrdinal;
    if( HasChildren( link ) )
    {
        DebugID ID = GetIDFromLink( tree, link );
        DebugView* view = GetViewFor( debugState, collation, ID );
        DebugInteraction itemInteraction = DebugIDInteraction( DebugInteraction_toggleExpansion, ID );
        if( debugState->ALTUI )
        {
            itemInteraction = DebugLinkInteraction( DebugInteraction_tear, link );
        }
        
        char* buff = link->name;
        
        Rect2 textBounds = TextSize( debugState, buff );
        Vec2 dim = V2( GetDim( textBounds ).x, layout->lineAdvance );
        
        LayoutElement element = BeginRectElement( layout, &dim );
        DefaultInteraction( &element, itemInteraction );
        EndElement( &element );
        
        b32 isHot = InteractionsAreEqual( itemInteraction, collation->hotInteraction );
        Vec4 color = isHot ? V4( 1.0f, 1.0f, 0.0f, 1.0f ) : V4( 1.0f, 1.0f, 1.0f, 1.0f );
        TextLineAt( debugState, buff, V2( element.bounds.min.x, element.bounds.max.y - GetBaseline( debugState ) ), color );
        
        if( view->collapsible.expandedAlways )
        {
            if(layout->depth == 1)
            {
                int a = 5;
            }
            ++layout->depth;
            for( DebugEventLink* subLink = link->firstChild; subLink != GetSentinel( link ); subLink = subLink->next )
            {
                DrawTreeLink( debugState, collation, layout, tree, subLink );
            }
            --layout->depth;
        }
    }
    else
    {
        if( link->element )
        {
            DebugID ID = GetIDFromLink( tree, link );
            DebugDrawElement( layout, tree, link->element, ID, frameOrdinal );
        }
    }
}

internal void DrawTrees( DebugState* debugState, DebugCollationState* collation, PlatformInput* input, Vec2 mouseP )
{
    RenderGroup* renderGroup = &debugState->renderGroup;
    for( DebugTree* tree = collation->sentinelTree.next;
        tree!= &collation->sentinelTree;
        tree = tree->next )
    {
        Layout layout = BeginLayout( debugState, collation, mouseP, tree->P + debugState->layoutOffset );
        
        
        DebugEventLink* rootGroup = tree->root; 
        if( rootGroup )
        {
            DrawTreeLink( debugState, collation, &layout, tree, rootGroup );
        }
        
        DebugInteraction moveInteraction = {};
        moveInteraction.type = DebugInteraction_move;
        moveInteraction.P = &tree->P;
        
        Rect2 sizeBounds = RectCenterDim( tree->P - V2( 10.0f, 10.0f ), V2( 4.0f, 4.0f ) );
        PushRect(renderGroup, DefaultOverDrawTransform(), sizeBounds, InteractionIsHot( collation, moveInteraction ) ? V4( 1.0f, 1.0f, 0.0f, 1.0f ) : V4( 1.0f, 1.0f, 1.0f, 1.0f ) ); 
        
        if( PointInRect( sizeBounds, mouseP ) )
        {
            collation->nextHotInteraction = moveInteraction;
        }
        
        EndLayout( &layout );
    }
    
}


DebugEventLink* CreateElementLink( DebugState* debugState, char* name, u32 nameLength );
DebugEventLink* CloneElementGroup( DebugState* debugState, DebugEventLink* first );
DebugTree* AddTree( DebugState* debugState, DebugCollationState* collation, DebugEventLink* group, Vec2 AtP );

internal void BeginInteract( DebugState* debugState, DebugCollationState* collation, PlatformInput* input, Vec2 mouseP )
{
    u32 frameOrdinal = collation->mostRecentFrameOrdinal;
    
    if( collation->hotInteraction.type )
    {
        if( collation->hotInteraction.type == DebugInteraction_autoModified )
        {
            switch( collation->hotInteraction.element->frames[frameOrdinal].mostRecentEvent->event.type )
            {
                case DebugType_b32:
                {
                    collation->hotInteraction.type = DebugInteraction_toggle;
                } break;
                
                case DebugType_r32:
                {
                    collation->hotInteraction.type = DebugInteraction_drag;
                } break;
                
                case DebugType_beginDataBlock:
                {
                    collation->hotInteraction.type = DebugInteraction_toggle;
                } break;
            }
        }
        
        if( debugState->ALTUI )
        {
            collation->hotInteraction.type = DebugInteraction_tear;
        }
        switch( collation->hotInteraction.type )
        {
            case DebugInteraction_tear:
            {
                DebugEventLink* newGroup = CloneElementGroup( debugState, collation->hotInteraction.link );
                DebugTree* tree =  AddTree( debugState, collation, newGroup, mouseP );
                collation->hotInteraction.type = DebugInteraction_move;
                collation->hotInteraction.P = &tree->P;
            } break;
            
            case DebugInteraction_select:
            {
                if( !input->shiftDown )
                {
                    ClearSelection( collation );
                }
                
                if( input->ctrlDown )
                {
                    ClearSelection( collation );
                    collation->editingID = collation->hotInteraction.ID;
                }
                else
                {
                    collation->editingID = {};
                    globalHoverEntity = 0;
                    AddToSelection( collation, collation->hotInteraction.ID );
                }
            } break;
        }
        collation->interaction = collation->hotInteraction;
    }
    else
    {
        collation->interaction.type = DebugInteraction_NOP;
    }
}

internal DebugElement* GetElementFromEvent( DebugState* debugState, DebugCollationState* collation, DebugEvent* event, DebugEventLink* parent, u32 op, b32 isServer );
inline void DEBUGMarkEditedEvent( DebugState* debugState, DebugCollationState* collation, DebugEvent* event )
{
    if( event )
    {
        globalDebugTable->editEvent = *event;
        b32 ignored = false;
        DebugElement* element = GetElementFromEvent( debugState, collation, event, 0, DebugElementOp_AddToGroup | DebugElementOp_CreateHierarchy, ignored );
        globalDebugTable->editEvent.GUID = element->originalGUID;
    }
}

internal void EndInteract( DebugState* debugState, DebugCollationState* collation, PlatformInput* input, Vec2 mouseP )
{
    u32 frameOrdinal = collation->mostRecentFrameOrdinal;
    switch( collation->interaction.type )
    {
        case DebugInteraction_toggleExpansion:
        {
            DebugView* view = GetViewFor( debugState, collation, collation->interaction.ID );
            view->collapsible.expandedAlways = !view->collapsible.expandedAlways;
        } break;
        
        case DebugInteraction_toggle:
        {
            DebugEvent* event = collation->hotInteraction.element ? &collation->hotInteraction.element->frames[frameOrdinal].mostRecentEvent->event : 0;
            switch( event->type )
            {
                case DebugType_b32:
                {
                    event->Value_b32 = !event->Value_b32;
                } break;
            }
            DEBUGMarkEditedEvent( debugState, collation, event );
        } break;
        
        case DebugInteraction_setUInt32:
        {
            *( u32* ) collation->interaction.target = collation->interaction.UInt32;
        } break;
        
        case DebugInteraction_setPointer:
        {
            *( void** ) collation->interaction.target = collation->interaction.pointer;
        } break;
    }
    
    collation->interaction.type = DebugInteraction_none;
    collation->interaction.generic = 0;
}

internal void DEBUGInteract( DebugState* debugState, DebugCollationState* collation, PlatformInput* input, Vec2 mouseP )
{
    Vec2 deltaMouseP = mouseP - debugState->lastMouseP;
    Vec2* P = collation->interaction.P;
    
    u32 frameOrdinal = collation->mostRecentFrameOrdinal;
    switch( collation->interaction.type )
    {
        case DebugInteraction_drag:
        {
            DebugEvent* event = collation->hotInteraction.element ? &collation->hotInteraction.element->frames[frameOrdinal].mostRecentEvent->event : 0;
            switch( event->type )
            {
                case DebugType_r32:
                {
                    r32 finalValue = event->Value_r32 += 0.1f * deltaMouseP.y;
                    if( input->ctrlDown )
                    {
                        finalValue = ( r32 ) ( RoundReal32ToI32( finalValue ) );
                    }
                    event->Value_r32 = finalValue;
                } break;
            }
            DEBUGMarkEditedEvent( debugState, collation, event );
            
        } break;
        
        case DebugInteraction_tear:
        {
            *P = mouseP;
        } break;
        
        case DebugInteraction_move:
        {
            *P += deltaMouseP;
        } break;
        
        case DebugInteraction_resize:
        {
            *P += V2( deltaMouseP.x, -deltaMouseP.y );
        } break;
        
        case DebugInteraction_none:
        {
            collation->hotInteraction = collation->nextHotInteraction;
            if(Clicked(&input->mouseLeft, 5))
            {
                BeginInteract( debugState, collation, input, mouseP );
                EndInteract( debugState, collation, input, mouseP );
            }
            else if( IsDown( &input->mouseLeft ) )
            {
                BeginInteract( debugState, collation, input, mouseP );
            }
        } break;
    }
    
    if( collation->interaction.type && !IsDown( &input->mouseLeft ) )
    {
        EndInteract( debugState, collation, input, mouseP );
    }
}

global_variable DebugEvent globalEditEvent_;
DebugEvent* DEBUGGlobalEditEvent = &globalEditEvent_;

internal DebugThread* GetDebugThread( DebugState* debugState, DebugCollationState* collation, u32 threadID )
{
    DebugThread* result = 0;
    
    for( DebugThread* thread = collation->firstThread;
        thread; thread = thread->next )
    {
        if( thread->ID == threadID )
        {
            result = thread;
            break;
        }
    }
    
    if( !result )
    {
        FREELIST_ALLOC( result, debugState->firstFreeThread, PushStruct( &debugState->debugPool, DebugThread ) );
        
        result->ID = threadID;
        result->firstOpenCodeBlock = 0;
        result->firstOpenDataBlock = 0;
        result->laneIndex = collation->frameBarLaneCount++;
        
        result->next = collation->firstThread;
        collation->firstThread = result;
    }
    
    return result;
}

inline OpenDebugBlock* AllocateDebugBlock( DebugState* debugState, DebugElement* element, u32 frameIndex, DebugEvent* event, OpenDebugBlock** openingBlock )
{
    OpenDebugBlock* result = 0;
    FREELIST_ALLOC( result, debugState->firstFreeBlock, PushStruct( &debugState->debugPool, OpenDebugBlock ) );
    result->startingFrameIndex = frameIndex;
    result->beginClock = event->clock;
    result->element = element;
    result->parent = *openingBlock;
    *openingBlock = result;
    
    return result;
}

inline void DeallocateDebugBlock( DebugState* debugState, OpenDebugBlock** firstBlock )
{
    OpenDebugBlock* toFree = *firstBlock;
    *firstBlock = toFree->parent;
    
    toFree->nextFree = debugState->firstFreeBlock;
    debugState->firstFreeBlock = toFree;
}

inline b32 DebugEventsMatch( DebugEvent* A, DebugEvent* B )
{
    b32 result = ( A->threadID == B->threadID );
    return result;
}

inline DebugTree* AddTree( DebugState* debugState, DebugCollationState* collation, DebugEventLink* group, Vec2 AtP )
{
    DebugTree* tree = PushStruct( &debugState->debugPool, DebugTree );
    tree->root = group;
    tree->P = AtP;
    DLLIST_INSERT( &collation->sentinelTree, tree );
    
    return tree;
}


inline DebugEventLink* CreateElementLink( DebugState* debugState, char* name, u32 nameLength )
{
    DebugEventLink* link = PushStruct( &debugState->debugPool, DebugEventLink );
    
    DLLIST_INIT( GetSentinel( link ) );
    link->next = link->prev = 0;
    link->name = nameLength ? PushAndNullTerminate( &debugState->debugPool, name, nameLength ) : 0;
    link->element = 0;
    
    return link;
}

inline DebugEventLink* AddElementToGroup( DebugState* debugState, DebugEventLink* parent, DebugElement* element )
{
    DebugEventLink* link = CreateElementLink( debugState, 0, 0 );
    if( parent )
    {
        DLLIST_INSERT_AS_LAST( GetSentinel( parent ), link );
        link->element = element;
    }
    return link;
}

inline DebugEventLink* AddLinkToGroup( DebugState* debugState, DebugEventLink* parent, DebugEventLink* link )
{
    DLLIST_INSERT_AS_LAST( GetSentinel( parent ), link );
    return link;
}

inline DebugEventLink* CloneElementLink( DebugState* debugState, DebugEventLink* destGroup, DebugEventLink* source )
{
    DebugEventLink* dest = AddElementToGroup( debugState, destGroup, source->element );
    dest->name = source->name;
    if( HasChildren( source ) )
    {
        for( DebugEventLink* child = source->firstChild; child != GetSentinel( source ); child = child->next )
        {
            CloneElementLink( debugState, dest, child );
        }
    }
    
    return dest;
}


inline DebugEventLink* CloneElementGroup( DebugState* debugState, DebugEventLink* source )
{
    DebugEventLink* result = CloneElementLink( debugState, 0, source );
    return result;
}

internal DebugEventLink* GetOrCreateGroupWithName( DebugState* debugState, DebugEventLink* parent, char* name, u32 nameLength )
{
    DebugEventLink* result = 0;
    for( DebugEventLink* link = parent->firstChild;
        link != GetSentinel( parent );
        link = link->next )
    {
        if( StrEqual( nameLength, name, link->name ) )
        {
            result = link;
        }
    }
    
    if( !result )
    {
        result = CreateElementLink( debugState, name, nameLength );
        AddLinkToGroup( debugState, parent, result );
    }
    
    return result;
}

internal DebugEventLink* GetGroupForName( DebugState* debugState, DebugEventLink* parent, char* eventName, b32 alwaysCreate )
{
    DebugEventLink* result = parent;
    
    char* firstSeparator = 0;
    char* scan = eventName;
    for( ; *scan; ++scan )
    {
        if( *scan == '/' )
        {
            firstSeparator = scan;
            break;
        }
    }
    
    if( firstSeparator || alwaysCreate )
    {
        u32 nameLength = 0;
        if( firstSeparator )
        {
            nameLength = ( u32 ) ( firstSeparator - eventName );
        }
        else
        {
            nameLength = ( u32 ) ( scan - eventName );
        }
        result = GetOrCreateGroupWithName( debugState, parent, eventName, nameLength );
        if( firstSeparator )
        {
            result = GetGroupForName( debugState, result, firstSeparator + 1, alwaysCreate );
        }
    }
    
    return result;
}

internal void FreeFrame( DebugState* debugState, DebugCollationState* collation, u32 frameOrdinal )
{
    Assert( frameOrdinal < DEBUG_FRAME_COUNT );
    u32 freedEventCount = 0;
    for( u32 hashIndex = 0; hashIndex < ArrayCount( collation->elements ); ++hashIndex )
    {
        for( DebugElement* element = collation->elements[hashIndex]; element; element = element->nextInHash )
        {
            DebugElementFrame* elementFrame = element->frames + frameOrdinal;
            while( elementFrame->oldestEvent )
            {
                DebugStoredEvent* freeEvent = elementFrame->oldestEvent;
                elementFrame->oldestEvent = freeEvent->next;
                FREELIST_DEALLOC( freeEvent, debugState->firstFreeStoredEvent );
                
                ++freedEventCount;
            }
            
            ZeroStruct( *elementFrame );
        }
    }
    
    DebugFrame* frame = collation->frames + frameOrdinal;
    Assert( freedEventCount == frame->storedEventCount );
    
    ZeroStruct( *frame );
}

internal void IncrementFrameOrdinal( u32* ordinal )
{
    *ordinal = ( *ordinal + 1 ) % DEBUG_FRAME_COUNT;
}

internal void FreeOldestFrame( DebugState* debugState, DebugCollationState* collation )
{
    FreeFrame( debugState, collation, collation->oldestFrameOrdinal );
    if( collation->oldestFrameOrdinal == collation->mostRecentFrameOrdinal )
    {
        IncrementFrameOrdinal( &collation->mostRecentFrameOrdinal );
    }
    
    IncrementFrameOrdinal( &collation->oldestFrameOrdinal );
}

internal void InitFrame( DebugCollationState* collation, u64 beginClock, DebugFrame* result )
{
    result->frameIndex = collation->totalFrameCount++;
    result->frameBarScale = 1.0f;
    result->beginClock = beginClock;
}

inline DebugFrame* GetCollationFrame( DebugCollationState* collation )
{
    DebugFrame* result = collation->frames + collation->collatingFrameOrdinal;
    return result;
}

internal DebugStoredEvent* StoreEvent( DebugState* debugState, DebugCollationState* collation, DebugElement* element, DebugEvent* event )
{
    DebugStoredEvent* result = 0;
    while( !result )
    {
        result = debugState->firstFreeStoredEvent;
        if( result )
        {
            debugState->firstFreeStoredEvent = result->next;
        }
        else
        {
#if 0
            if( HasRoomFor( &debugState->perFramePool, sizeof( DebugStoredEvent ) ) )
            {
                result = PushStruct( &debugState->perFramePool, DebugStoredEvent );
            }
            else
            {
                FreeOldestFrame( debugState );
            }
#else
            result = PushStruct( &debugState->perFramePool, DebugStoredEvent );
#endif
        }
    }
    
    DebugFrame* collationFrame = GetCollationFrame( collation );
    result->next = 0;
    result->event = *event;
    result->frameIndex = collationFrame->frameIndex;
    ++collationFrame->storedEventCount;
    
    DebugElementFrame* frame = element->frames + collation->collatingFrameOrdinal;
    if( frame->mostRecentEvent )
    {
        frame->mostRecentEvent = frame->mostRecentEvent->next = result;
    }
    else
    {
        frame->oldestEvent = frame->mostRecentEvent = result;
    }
    
    return result;
}

internal DebugElement* GetElementFromEvent( DebugState* debugState, DebugCollationState* collation, DebugEvent* event, DebugEventLink* parent, u32 op, b32 isServer )
{
    if( !parent )
    {
        parent = collation->rootGroup;
    }
    
    DebugParsedName parsedName = DebugParseName( event->GUID, event->name );
    u32 hashIndex = parsedName.hashValue % ArrayCount( collation->elements );
    DebugElement* result = GetElementFromGUID( collation, hashIndex, event->GUID );
    if( !result )
    {
        result = PushStruct( &debugState->debugPool, DebugElement );
        
        result->type = ( DebugType ) event->type;
        result->originalGUID = event->GUID;
        
        
#if 0        
        if( isServer )
        {
            AddString( "server" );
        }
        else
        {
            AddString( "client" );
        }
#endif
        
        result->GUID = PushString( &debugState->debugPool, event->GUID );
        result->fileNameCount = parsedName.fileNameCount;
        result->name = PushString( &debugState->debugPool, parsedName.name );
        
        result->nextInHash = collation->elements[hashIndex];
        collation->elements[hashIndex] = result;
        
        DebugEventLink* parentGroup = parent;
        if( op & DebugElementOp_CreateHierarchy )
        {
            parentGroup = GetGroupForName( debugState, parent, GetName( result ), false );
        }
        
        if( op & DebugElementOp_AddToGroup )
        {
            AddElementToGroup( debugState, parentGroup, result );
        }
        
    }
    
    return result;
}

internal void CollateDebugEvents( DebugState* debugState, DebugCollationState* collation, u32 eventCount, DebugEvent* eventArray, b32 isServer )
{
    for( u32 eventIndex = 0; eventIndex < eventCount; eventIndex++ )
    {
        DebugFrame* collationFrame = GetCollationFrame( collation );
        DebugEvent* event = eventArray + eventIndex;
        
        if( event->type == DebugType_frameMarker )
        {
            collationFrame->endClock = event->clock;
            if( collationFrame->rootProfileNode )
            {
                collationFrame->rootProfileNode->profileNode.duration = collationFrame->endClock - collationFrame->beginClock;
            }
            
            collationFrame->secondsElapsed = event->Value_r32;
            r32 clockRange = ( r32 ) ( collationFrame->endClock - collationFrame->beginClock );
            
            ++collation->totalFrameCount;
            
            if( collation->paused )
            {
                FreeFrame( debugState, collation, collation->collatingFrameOrdinal );
            }
            else
            {
                collation->mostRecentFrameOrdinal = collation->collatingFrameOrdinal;
                IncrementFrameOrdinal( &collation->collatingFrameOrdinal );
                if( collation->collatingFrameOrdinal == collation->oldestFrameOrdinal )
                {
                    FreeOldestFrame( debugState, collation );
                }
                collationFrame = GetCollationFrame( collation );
            }
            InitFrame( collation, event->clock, collationFrame );
        }
        else
        {
            Assert( collationFrame );
            u32 frameIndex = collation->totalFrameCount - 1;
            DebugThread* thread = GetDebugThread( debugState, collation, event->threadID );
            u64 relativeClock = event->clock - collationFrame->beginClock;
            
            DebugEventLink* defaultParentGroup = collation->rootGroup;
            if( thread->firstOpenDataBlock )
            {
                defaultParentGroup = thread->firstOpenDataBlock->group;
            }
            switch( event->type )
            {
                case DebugType_beginCodeBlock:
                {
                    ++collationFrame->profileBlockCount;
                    DebugElement* element = GetElementFromEvent( debugState, collation, event, collation->profileGroup, DebugElementOp_AddToGroup, isServer );
                    
                    DebugStoredEvent* parentEvent = collationFrame->rootProfileNode;
                    u64 clockBasis = collationFrame->beginClock;
                    if( thread->firstOpenCodeBlock )
                    {
                        parentEvent = thread->firstOpenCodeBlock->node;
                        clockBasis = thread->firstOpenCodeBlock->beginClock;
                    }
                    else if( !parentEvent )
                    {
                        DebugEvent nullEvent = {};
                        parentEvent =  StoreEvent( debugState, collation, collation->rootProfileElement, &nullEvent );
                        DebugProfileNode* node = &parentEvent->profileNode;
                        node->duration = 0;
                        node->element = 0;
                        node->firstChild = 0;
                        node->nextSameParent = 0;
                        node->parentRelativeClock = 0;
                        node->duration = 0;
                        node->durationOfChildren = 0;
                        node->threadOrdinal = 0;
                        node->coreIndex = 0;
                        node->nextSameParent = 0;
                        
                        clockBasis = collationFrame->beginClock;
                        collationFrame->rootProfileNode = parentEvent;
                    }
                    
                    DebugStoredEvent* storedEvent = StoreEvent( debugState, collation, element, event );
                    DebugProfileNode* node = &storedEvent->profileNode;
                    node->element = element;
                    node->firstChild = 0;
                    node->nextSameParent = 0;
                    node->parentRelativeClock = event->clock - clockBasis;
                    node->duration = 0;
                    node->durationOfChildren = 0;
                    node->threadOrdinal = ( u16 ) thread->laneIndex;
                    node->coreIndex = event->coreIndex;
                    node->nextSameParent = parentEvent->profileNode.firstChild;
                    
                    parentEvent->profileNode.firstChild = storedEvent;
                    
                    OpenDebugBlock* debugBlock = AllocateDebugBlock( debugState, element, frameIndex, event, &thread->firstOpenCodeBlock );
                    debugBlock->node = storedEvent;
                } break;
                
                case DebugType_endCodeBlock:
                {
                    if( thread->firstOpenCodeBlock )
                    {
                        OpenDebugBlock* matchingBlock = thread->firstOpenCodeBlock;
                        Assert( thread->ID == event->threadID );
                        
                        DebugProfileNode* node = &matchingBlock->node->profileNode;
                        node->duration = event->clock - matchingBlock->beginClock;
                        
                        DeallocateDebugBlock( debugState, &thread->firstOpenCodeBlock );
                        if( thread->firstOpenCodeBlock )
                        {
                            DebugProfileNode* parentNode = &thread->firstOpenCodeBlock->node->profileNode;
                            parentNode->durationOfChildren += node->duration;
                        }
                    }
                } break;
                
                case DebugType_beginDataBlock:
                {
                    ++collationFrame->dataBlockCount;
                    DebugParsedName parsedName = DebugParseName( event->GUID, event->name );
                    DebugEventLink* group = GetGroupForName( debugState, defaultParentGroup, parsedName.name, true );
                    OpenDebugBlock* debugBlock = AllocateDebugBlock( debugState, 0, frameIndex, event, &thread->firstOpenDataBlock );
                    debugBlock->group = group;
                } break;
                
                case DebugType_endDataBlock:
                {
                    if( thread->firstOpenDataBlock )
                    {
                        OpenDebugBlock* matchingBlock = thread->firstOpenDataBlock;
                        Assert( thread->ID == event->threadID );
                        DeallocateDebugBlock( debugState, &thread->firstOpenDataBlock );
                    }
                } break;
                
                default:
                {
                    DebugElement* element = GetElementFromEvent( debugState, collation, event, defaultParentGroup, DebugElementOp_AddToGroup | DebugElementOp_CreateHierarchy, isServer );
                    element->originalGUID = event->GUID;
                    StoreEvent( debugState, collation, element, event );
                } break;
            }
        }
    }
}

internal void DumpStruct( DebugState* debugState, void* structPtr, MemberDefinition* members, u32 memberCount, u32 indentLevel = 0 )
{
    NotImplemented;
    
#if 0    
    for( u32 memberIndex = 0; memberIndex < memberCount;
        ++memberIndex )
    {
        char textBufferBase[4096];
        textBufferBase[0] = 0;
        
        char* textBuffer = textBufferBase;
        
        for( u32 indentIndex = 0; indentIndex < indentLevel; ++indentIndex )
        {
            *textBuffer++ = ' ';
            *textBuffer++ = ' ';
            *textBuffer++ = ' ';
            *textBuffer++ = ' ';
        }
        
        size_t textBufferLeft = sizeof( textBufferBase ) - ( textBuffer - textBufferBase );
        
        MemberDefinition* member = members + memberIndex;
        void* memberPtr = ( ( u8* ) structPtr ) + member->offset;
        
        if( member->flags & MetaFlag_Pointer )
        {
            memberPtr = *( void** ) memberPtr;
        }
        
        if( memberPtr )
        {
            switch( member->type )
            {
                case MetaType_u32:
                {
                    FormatString( textBuffer, textBufferLeft, "%s: %u", member->name, 
                                 *( u32* ) memberPtr );
                } break;
                
                case MetaType_i32:
                {
                    FormatString( textBuffer, textBufferLeft, "%s: %d", member->name, 
                                 *( i32* ) memberPtr );
                } break;
                
                case MetaType_b32:
                {
                    FormatString( textBuffer, textBufferLeft, "%s: %d", member->name, 
                                 *( b32* ) memberPtr );
                } break;
                
                case MetaType_r32:
                {
                    FormatString( textBuffer, textBufferLeft, "%s: %f", member->name, 
                                 *( r32* ) memberPtr );
                } break;
                
                case MetaType_Vec2:
                {
                    FormatString( textBuffer, textBufferLeft, "%s: {%f, %f}", member->name,
                                 ( r32 ) ( ( Vec2* ) memberPtr )->x, ( r32 ) ( ( Vec2* ) memberPtr )->y );
                } break;
                
                case MetaType_Vec3:
                {
                    FormatString( textBuffer, textBufferLeft, "%s: {%f, %f, %f}", 
                                 member->name,
                                 ( r32 ) ( ( Vec3* ) memberPtr )->x, 
                                 ( r32 ) ( ( Vec3* ) memberPtr )->y,
                                 ( r32 ) ( ( Vec3* ) memberPtr )->z );
                } break;
                
                //META_HANDLE_TYPE_DUMP( memberPtr, indentLevel + 1 );
            }
        }
        
        if( textBuffer[0] )
        {
            //DEBUGTextLine( debugState, textBufferBase );
        }
        
    }
#endif
    
}

internal void DEBUGOverlay( DebugState* debugState, DebugCollationState* collation, PlatformInput* input, Vec2 mouseP )
{
    DebugPlatformMemoryStats stats = collation->memStats;
    DebugFrame* mostRecentFrame = collation->frames + collation->viewingFrameOrdinal;
    FormatString( collation->rootInfo, 256, "%2.2fms %de %dp %dd memory: %d blocks, used %lu out of %lu", mostRecentFrame->secondsElapsed * 1000.0f, mostRecentFrame->storedEventCount, mostRecentFrame->profileBlockCount, mostRecentFrame->dataBlockCount,
                 stats.blockCount, stats.totalUsed, stats.totalSize );
    
    PushRect( &debugState->renderGroup, FlatTransform(), RectCenterDim( mouseP, V2( 4.0f, 4.0f ) ), V4( 1.0f, 0.0f, 0.0f, 1.0f ) );
    
    debugState->layoutOffset += V2( 0, -input->mouseWheelOffset * 10.0f );
    RenderGroup* renderGroup = &debugState->renderGroup;
    debugState->ALTUI = IsDown( &input->mouseRight );
    
    for( u32 textIndex = 0; textIndex < debugState->countPositionedText; ++textIndex )
    {
        PositionedText* text = debugState->positionedText + textIndex;
        Rect2 rect = TextSize( debugState, text->text );
        Vec2 finalP = text->screenP - 0.5f * GetDim( rect );
        TextLineAt(debugState, text->text, finalP, text->color, 1.0f);
    }
    
    debugState->mouseTextLayout = BeginLayout( debugState, collation, mouseP, mouseP );
    DrawTrees( debugState, collation, input, mouseP );
    DEBUGInteract( debugState, collation, input, mouseP );
    collation->nextHotInteraction = {};
    
    EndLayout( &debugState->mouseTextLayout );
    
    DebugEvent* event = &globalDebugTable->editEvent;
    if( event->pointer )
    {
        globalDebugTable->pointerToIgnore = event->pointer;
        globalDebugTable->overNetworkEdit[0] = event->overNetwork[0];
        globalDebugTable->overNetworkEdit[1] = event->overNetwork[1];
        SendEditingEvent( event );
    }
    
    
#if 0    
    SimEntity test = {};
    test.type = 2;
    test.flags = 1000;
    test.ignoredAction = 3;
    test.velocity = V3( 1.0f, 2.0f, 3.0f );
    test.acceleration = V3( 10.0f, 20.0f, 30.0f );
    
    SimRegion testRegion = {};
#endif
    
    //DumpStruct( &test, memberDefinitionOfSimEntity, ArrayCount( memberDefinitionOfSimEntity ) );
    //DumpStruct( &testRegion, memberDefinitionOfSimRegion, ArrayCount( memberDefinitionOfSimRegion ) );
    
    //PushCube( &debugState->renderGroup, V3( 0, 0, 0 ), 100.0f, 100.0f, V4( 1,0,0,1) );
}

#define InitializeCollationState( debugState, collation, rootP ) InitializeCollationState_( debugState, collation, rootP, "root_" #collation, "profile_" #collation, "root_profiler_" #collation )
internal void InitializeCollationState_( DebugState* debugState, DebugCollationState* collation, Vec2 rootP, char* rootName, char* profileGroupName, char* rootProfilerName )
{
    collation->rootGroup = CreateElementLink( debugState, rootName, StrLen( rootName ) );
    collation->rootInfo = ( char* ) PushSize( &debugState->debugPool, 256 );
    collation->rootGroup->name = collation->rootInfo;
    
    collation->profileGroup = CreateElementLink( debugState, profileGroupName, StrLen( profileGroupName ) );
    
    collation->sentinelTree.root = 0;
    collation->sentinelTree.next = &collation->sentinelTree;
    collation->sentinelTree.prev = &collation->sentinelTree;
    collation->collatingFrameOrdinal = 1;
    
    DebugEvent rootProfileEvent = {};
    //rootProfileEvent.GUID = DEBUG_NAME( "root_profiler" );
    rootProfileEvent.GUID = rootProfilerName;
    rootProfileEvent.name = rootProfilerName;
    collation->rootProfileElement = GetElementFromEvent( debugState, collation, &rootProfileEvent, 0, 0, true );
    
    AddTree( debugState, collation, collation->rootGroup, rootP );
}

internal DebugState* DebugInit( GameRenderCommands* commands )
{
    DebugState* debugState = BootstrapPushStruct( DebugState, debugPool );
    ZeroStruct( debugState->perFramePool );
    
    DebugCollationState* clientCollation = &debugState->clientState;
    DebugCollationState* serverCollation = &debugState->serverState;
    
    InitializeCollationState( debugState, clientCollation, V2( -0.5f * commands->settings.width + 0.5f, 0.5f * commands->settings.height ) );
    InitializeCollationState( debugState, serverCollation, V2( 0.5f, 0.5f * commands->settings.height ) );
    
    return debugState;
}

internal void DEBUGReset( Assets* assets, GameRenderCommands* commands, u32 mainGenerationID )
{
    if( !debugGlobalMemory->debugState )
    {
        debugGlobalMemory->debugState = DebugInit( commands );
    }
    
    DebugState* debugState = debugGlobalMemory->debugState;
    debugState->renderGroup = BeginRenderGroup( assets, commands, mainGenerationID );
    
    r32 width = (r32) commands->settings.width;
    r32 height = (r32) commands->settings.height;
    
    SetCameraTransform(&debugState->renderGroup, Camera_Orthographic, 0.0f, V3(2.0f / width, 0.0f, 0.0f), V3(0.0f, 2.0f / width, 0.0f), V3( 0, 0, 1));
    
    debugState->fontScale = 1.0f;
    
    TagVector matchVector = {};
    TagVector weightVector = {};
    weightVector.E[Tag_fontType] = 1.0f;
    matchVector.E[Tag_fontType] = ( r32 ) Font_debug;
    
    debugState->fontId = GetMatchingFont( assets, Asset_font, &matchVector, &weightVector );
    debugState->debugFont = PushFont( &debugState->renderGroup, debugState->fontId );
    debugState->debugFontInfo = GetFontInfo( debugState->renderGroup.assets, debugState->fontId );
    
}

//void name( PlatformMemory* memory, PlatformInput* input, GameRenderCommands* commands )
extern "C" GAME_FRAME_END( GameDEBUGFrameEnd )
{
    ZeroStruct( globalDebugTable->editEvent );
    globalDebugTable->currentEventArrayIndex = !globalDebugTable->currentEventArrayIndex;
    
    u64 arrayIndex_eventIndex = AtomicExchangeU64( &globalDebugTable->eventArrayIndex_EventIndex, ( ( u64 ) globalDebugTable->currentEventArrayIndex << 32 ) );
    
    u32 eventArrayIndex = arrayIndex_eventIndex >> 32;
    Assert( eventArrayIndex <= 1 );
    u32 eventCount = arrayIndex_eventIndex & 0xffffffff;
    
    Assets* assets = DEBUGGetGameAssets( memory );
    DEBUGReset( assets, commands, DEBUGGetMainGenerationID( memory ) );
    
    DebugState* debugState = debugGlobalMemory->debugState;
    
    DebugCollationState* clientCollation = &debugState->clientState;
    clientCollation->memStats = platformAPI.DEBUGMemoryStats();
    
    DebugCollationState* serverCollation = &debugState->serverState;
    serverCollation->memStats = globalDebugTable->serverStats;
    
    if( !clientCollation->paused )
    {
        clientCollation->viewingFrameOrdinal = clientCollation->mostRecentFrameOrdinal;
    }
    CollateDebugEvents( debugState, clientCollation, eventCount, globalDebugTable->eventArray[eventArrayIndex], false );
    
    
    if( !serverCollation->paused )
    {
        serverCollation->viewingFrameOrdinal = serverCollation->mostRecentFrameOrdinal;
    }
    
    u32 currentIndex = globalDebugTable->currentServerEventArrayIndex;
    u32 serverEventCount = globalDebugTable->serverEventCount[currentIndex];
    if( globalDebugTable->serverFinished )
    {
        //Clear( &globalDebugTable->tempPool );
        globalDebugTable->currentServerEventArrayIndex = ( currentIndex + 1 ) % 2;
        globalDebugTable->serverEventCount[globalDebugTable->currentServerEventArrayIndex] = 0;
        CollateDebugEvents( debugState, serverCollation, serverEventCount, globalDebugTable->serverEvents[currentIndex], true );
        globalDebugTable->serverFinished = false;
    }
    
    Vec2 mouseP = Unproject( &debugState->renderGroup, &debugState->renderGroup.gameCamera, V2( input->mouseX, input->mouseY ), 0 ).xy;
    DEBUGOverlay(debugState, serverCollation, input, mouseP); 
    DEBUGOverlay(debugState, clientCollation, input, mouseP); 
    
    debugState->lastMouseP = mouseP;
    
    EndRenderGroup( &debugState->renderGroup );
    debugState->countPositionedText = 0;
}
