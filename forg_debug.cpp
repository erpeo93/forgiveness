#include "forg_debug.h"

inline void BeginDebugStatistics(DebugStatistic* stats)
{
    stats->min = R32_MAX;
    stats->max = R32_MIN;
    stats->avg = 0;
    stats->count = 0;
    stats->sum = 0;
}

inline void AccumDebugStatistics(DebugStatistic* stats, r64 value)
{
    ++stats->count;
    
    stats->min = Min(value, stats->min);
    stats->max = Max(value, stats->max);
    
    stats->sum += value;
}

inline void EndDebugStatistics(DebugStatistic* stats)
{
    if(stats->count)
    {
        stats->avg = stats->sum / (r64) stats->count;
    }
}


internal void FreeFrame(DebugState* debugState, DebugCollationState* collation, u32 frameOrdinal)
{
    DebugFrame* frame = collation->frames + frameOrdinal;
    for(u32 hashIndex = 0; hashIndex < ArrayCount(collation->elements); ++hashIndex)
    {
        for(DebugElement* element = collation->elements[hashIndex]; element; element = element->nextInHash)
        {
            for(DebugProfileNode* toFree = element->nodes[frameOrdinal]; toFree;)
            {
                DebugProfileNode* next = toFree->next;
                FREELIST_DEALLOC(toFree, debugState->firstFreeProfileNode);
                toFree = next;
            }
            element->nodes[frameOrdinal] = 0;
        }
        
    }
    FREELIST_FREE(frame->firstEntry, DebugEntry, debugState->firstFreeEntry);
    ZeroStruct(*frame);
}

internal void IncrementFrameOrdinal(u32* ordinal)
{
    *ordinal = (*ordinal + 1) % DEBUG_FRAME_COUNT;
}

internal void FreeOldestFrame(DebugState* debugState, DebugCollationState* collation)
{
    FreeFrame(debugState, collation, collation->oldestFrameOrdinal);
    if(collation->oldestFrameOrdinal == collation->mostRecentFrameOrdinal)
    {
        IncrementFrameOrdinal(&collation->mostRecentFrameOrdinal);
    }
    IncrementFrameOrdinal(&collation->oldestFrameOrdinal);
}

internal void InitFrame(DebugCollationState* collation, u64 beginClock, DebugFrame* result)
{
    result->frameIndex = collation->totalFrameCount++;
    result->beginClock = beginClock;
}

inline DebugFrame* GetCollationFrame(DebugCollationState* collation)
{
    DebugFrame* result = collation->frames + collation->collatingFrameOrdinal;
    return result;
}

inline DebugFrame* GetViewingFrame(DebugCollationState* collation)
{
    DebugFrame* result = collation->frames + collation->viewingFrameOrdinal;
    return result;
}

internal void DrawFrameSlider(EditorLayout* layout, DebugCollationState* collation, Rect2 bounds)
{
    PushRect(layout->group, FlatTransform(), bounds, V4(0.0f, 0.0f, 0.0f, 0.5f));
    
    u32 frameCount = ArrayCount(collation->frames);
    r32 barWidth = GetDim(bounds).x / (r32) frameCount;
    r32 atX = bounds.min.x;
    
    r32 thisMinY = bounds.min.y;
    r32 thisMaxY = bounds.max.y;
    for(u32 frameIndex = 0; (frameIndex < frameCount); ++frameIndex)
    {
        Rect2 regionRect = RectMinMax(V2(atX, thisMinY), V2(atX + barWidth, thisMaxY));
        
        b32 highlight = false;
        Vec4 color = V4(0.5f, 0.5f, 0.5f, 1.0f);
        if(frameIndex == collation->viewingFrameOrdinal)
        {
            highlight = true;
            color = V4(1.0f, 1.0f, 0.0f, 1.0f);
        }
        
        if(frameIndex == collation->mostRecentFrameOrdinal)
        {
            highlight = true;
            color = V4(0.0f, 1.0f, 0.0f, 1.0f);
        }
        
        if(frameIndex == collation->collatingFrameOrdinal)
        {
            highlight = true;
            color = V4(1.0f, 0.0f, 0.0f, 1.0f);
        }
        
        if(frameIndex == collation->oldestFrameOrdinal)
        {
            highlight = true;
            color = V4(0.0f, 0.5f, 0.0f, 1.0f);
        }
        
        if(highlight)
        {
            PushRect(layout->group, FlatTransform(), regionRect, color);
        }
        else
        {
            PushRectOutline(layout->group, FlatTransform(), regionRect, color, 2.0f);
        }
        
        if(PointInRect(regionRect, layout->mouseP))
        {
            char buff[256];
            FormatString(buff, sizeof(buff), "%u", frameIndex);
            AddTooltip(layout, buff);
            
            if(UIDown(layout->context, mouseLeft))
            {
                collation->viewingFrameOrdinal = frameIndex;
            }
        }
        atX += barWidth;
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

internal DebugProfileNode* GetViewingNode(DebugCollationState* collation, DebugElement* element)
{
    DebugProfileNode* result = element->nodes[collation->viewingFrameOrdinal];
    return result;
}

internal u64 GetTotalDuration(DebugProfileNode* n)
{
    u64 result = 0;
    for(DebugProfileNode* node = n; node; node = node->next)
    {
        result += node->duration;
    }
    
    return result;
}

internal void DrawProfileBars(EditorLayout* layout, DebugCollationState* collation, Rect2 bounds, DebugElement* rootElement, r32 laneStride, r32 laneHeight, u32 depthRemaining)
{
    DebugProfileNode* rootNode = GetViewingNode(collation, rootElement);
    if(rootNode)
    {
        r32 frameSpan = (r32) (rootNode->duration);
        r32 pixelSpan = GetDim(bounds).x;
        
        r32 zOffset = -20.0f * (r32) depthRemaining;
        
        r32 scale = 0.0f;
        if(frameSpan > 0)
        {
            scale = pixelSpan / frameSpan;
        }
        
        for(DebugProfileNode* node = rootNode->firstChild; node; node = node->nextSameParent)
        {
            DebugElement* element = node->element;
            
            Vec3 color = debugColors[U32FromPointer(element->GUID) % ArrayCount(debugColors)];
            u32 laneIndex = node->threadOrdinal;
            r32 laneY = bounds.max.y - laneStride * laneIndex;
            r32 thisMinX = bounds.min.x + scale * (node->parentRelativeClock);
            r32 thisMaxX = thisMinX + scale * (r32) (node->duration);
            
            Rect2 regionRect = RectMinMax(V2(thisMinX, laneY - laneHeight), V2(thisMaxX, laneY));
            PushRectOutline(layout->group, FlatTransform(), regionRect, V4(0.0f, 0.0f, 0.0f, 1.0f), 2.0f);
            PushRect(layout->group, FlatTransform(), regionRect, V4(color, 1.0f));
            
            if(PointInRect(regionRect, layout->mouseP))
            {
                char buff[256];
                FormatString(buff, sizeof(buff), "%s: %ucy ", element->name, (u32) node->duration);
                AddTooltip(layout, buff);
                
                if(UIPressed(layout->context, mouseLeft))
                {
                    collation->currentRootElement = element;
                }
            }
            
            if(depthRemaining > 0)
            {
                DrawProfileBars(layout, collation, regionRect, node->element, 0, laneHeight * 0.5f, depthRemaining - 1);
            }
        }
    }
}

internal void DrawProfiler(EditorLayout* layout, DebugCollationState* collation, Rect2 bounds)
{
    u32 laneCount = collation->threadCount;
    r32 laneHeight = 0.0f;
    
    if(laneCount)
    {
        laneHeight = GetDim(bounds).y / (r32) laneCount;
    }
    
    DebugElement* rootElement = collation->currentRootElement;
    DrawProfileBars(layout, collation, bounds, rootElement, laneHeight, laneHeight, 0);
}

internal void DrawFrameBars(EditorLayout* layout, DebugCollationState* collation, Rect2 bounds)
{
    u32 frameCount = DEBUG_FRAME_COUNT;
    if(frameCount > 0)
    {
        r32 barWidth = GetDim(bounds).x / (r32) frameCount;
        r32 atX = bounds.min.x;
        
        DebugElement* rootElement = collation->currentRootElement;
        if(rootElement)
        {
            for(u32 frameIndex = 0; frameIndex < frameCount; ++frameIndex)
            {
                DebugProfileNode* rootNode = rootElement->nodes[frameIndex];
                if(rootNode)
                {
                    r32 frameSpan = (r32) (rootNode->duration);
                    r32 pixelSpan = GetDim(bounds).y;
                    r32 scale = 0.0f;
                    if(frameSpan > 0)
                    {
                        scale = pixelSpan / frameSpan;
                    }
                    
                    b32 highlight = (frameIndex == collation->viewingFrameOrdinal);
                    r32 highDim = highlight ? 1.0f : 0.7f;
                    
                    for(DebugProfileNode* node = rootNode->firstChild; node; node = node->nextSameParent)
                    {
                        DebugElement* element = node->element;
                        
                        Vec3 color = debugColors[U32FromPointer(element->GUID) % ArrayCount(debugColors)];
                        r32 thisMinY = bounds.min.y + scale * (node->parentRelativeClock);
                        r32 thisMaxY = thisMinY + scale * (r32) (node->duration);
                        
                        Rect2 regionRect = RectMinMax(V2(atX, thisMinY), V2(atX + barWidth, thisMaxY));
                        PushRectOutline(layout->group, FlatTransform(), regionRect, V4(0.0f, 0.0f, 0.0f, 1.0f), 2.0f);
                        PushRect(layout->group, FlatTransform(), regionRect, V4(color, highDim));
                        
                        if(PointInRect(regionRect, layout->mouseP))
                        {
                            char buff[256];
                            FormatString(buff, sizeof(buff), "%ucy %s %s", (u32) node->duration, element->GUID, element->name);
                            AddTooltip(layout, buff);
                            
                            if(UIPressed(layout->context, mouseLeft))
                            {
                                collation->currentRootElement = element;
                            }
                        }
                    }
                    
                    atX += barWidth;
                }
            }
        }
    }
}

struct DebugClockEntry
{
    DebugElement* element;
    DebugStatistic stats;
};

internal void DrawTopClockList(EditorLayout* layout, DebugCollationState* collation, Rect2 bounds, MemoryPool* pool)
{
    
    DebugFrame* frame = GetViewingFrame(collation);
    if(frame->rootProfileNode)
    {
        TempMemory temp = BeginTemporaryMemory(pool);
        DebugClockEntry* entries = PushArray(temp.pool, DebugClockEntry, frame->entryCount, NoClear());
        SortEntry* sortA = PushArray(temp.pool, SortEntry, frame->entryCount, NoClear());
        SortEntry* sortB = PushArray(temp.pool, SortEntry, frame->entryCount, NoClear());
        
        u32 index = 0;
        
        r64 totalTime = (r64) frame->rootProfileNode->duration;
        for(DebugEntry* debugEntry = frame->firstEntry; debugEntry; debugEntry = debugEntry->next, index++)
        {
            SortEntry* sort = sortA + index;
            DebugClockEntry* entry = entries + index;
            
            DebugElement* element = debugEntry->element;
            entry->element = element;
            
            BeginDebugStatistics(&entry->stats);
            
            for(DebugProfileNode* node = GetViewingNode(collation, element); node; node = node->next)
            {
                AccumDebugStatistics(&entry->stats, (r64) (node->duration - node->durationOfChildren));
            }
            
            EndDebugStatistics(&entry->stats);
            //totalTime += (r32) entry->stats.sum;
            
            sort->sortKey = (r32) -entry->stats.sum;
            sort->index = index;
        }
        
        RadixSort(sortA, frame->entryCount, sortB);
        
        r64 PC = 0;
        if(totalTime > 0)
        {
            PC = 100.0f / totalTime;
        }
        
        Vec2 at = V2(bounds.min.x, bounds.max.y) - V2(0, GetBaseline(layout));
        for(index = 0; index < frame->entryCount; ++ index)
        {
            DebugClockEntry* entry = entries + sortA[index].index;
            DebugStatistic* stats = &entry->stats;
            DebugElement* element = entry->element;
            
            if(at.y < bounds.min.y)
            {
                break;
            }
            else
            {
                char buff[1024];
                FormatString(buff, sizeof(buff), "%012ucy %3.2f%% %4d %s", (u32) stats->sum, PC * stats->sum, stats->count, element->name);
                TextLineAt(layout, buff, at);
                
                at.y -= RawHeight(layout);
            }
        }
        EndTemporaryMemory(temp);
    }
}


internal DebugThread* GetDebugThread(DebugState* debugState, DebugCollationState* collation, u32 threadID)
{
    DebugThread* result = 0;
    
    for(DebugThread* thread = collation->firstThread;
        thread; thread = thread->next)
    {
        if(thread->ID == threadID)
        {
            result = thread;
            break;
        }
    }
    
    if(!result)
    {
        FREELIST_ALLOC(result, debugState->firstFreeThread, PushStruct(&debugState->debugPool, DebugThread));
        
        result->ID = threadID;
        result->currentOpenCodeBlock = 0;
        result->laneIndex = collation->threadCount++;
        
        result->next = collation->firstThread;
        collation->firstThread = result;
    }
    
    return result;
}

internal DebugElement* GetElement(DebugState* debugState, DebugCollationState* collation, char* GUID, char* name)
{
    u32 hashIndex = StringHash(name) % ArrayCount(collation->elements);
    DebugElement* result = 0;
    for(DebugElement* element = collation->elements[hashIndex];
        element;
        element = element->nextInHash)
    {
        if(StrEqual(element->GUID, GUID))
        {
            result = element;
            break;
        }
    }
    
    if(!result)
    {
        result = PushStruct(&debugState->debugPool, DebugElement);
        result->GUID = PushString(&debugState->debugPool, GUID);
        result->name = PushString(&debugState->debugPool, name);
        
        result->nextInHash = collation->elements[hashIndex];
        collation->elements[hashIndex] = result;
    }
    return result;
}

internal DebugProfileNode* StoreEvent(DebugState* debugState, DebugCollationState* collation, DebugEvent* event, b32 addEntry)
{
    DebugFrame* collationFrame = GetCollationFrame(collation);
    
    DebugElement* element = GetElement(debugState, collation, event->GUID, event->name);
    
    DebugProfileNode* result = 0;
    FREELIST_ALLOC(result, debugState->firstFreeProfileNode, PushStruct(&debugState->debugPool, DebugProfileNode));
    ZeroStruct(*result);
    result->element = element;
    
    
    
    if(addEntry && !element->nodes[collation->collatingFrameOrdinal])
    {
        DebugEntry* entry = 0;
        FREELIST_ALLOC(entry, debugState->firstFreeEntry, PushStruct(&debugState->debugPool, DebugEntry));
        entry->element = element;
        
        ++collationFrame->entryCount;
        FREELIST_INSERT(entry, collationFrame->firstEntry);
    }
    
    
    
    result->next = element->nodes[collation->collatingFrameOrdinal];
    element->nodes[collation->collatingFrameOrdinal] = result;
    
    return result;
}

internal void CollateDebugEvent(DebugState* debugState, DebugCollationState* collation, DebugEvent* event)
{
    DebugFrame* collationFrame = GetCollationFrame(collation);
    if(event->type == DebugType_frameMarker)
    {
        collationFrame->endClock = event->clock;
        if(collationFrame->rootProfileNode)
        {
            collationFrame->rootProfileNode->duration = collationFrame->endClock - collationFrame->beginClock;
        }
        
        collationFrame->secondsElapsed = event->Value_r32;
        r32 clockRange = (r32) (collationFrame->endClock - collationFrame->beginClock);
        
        ++collation->totalFrameCount;
        
        if(collation->paused)
        {
            FreeFrame(debugState, collation, collation->collatingFrameOrdinal);
        }
        else
        {
            collation->mostRecentFrameOrdinal = collation->collatingFrameOrdinal;
            IncrementFrameOrdinal(&collation->collatingFrameOrdinal);
            if(collation->collatingFrameOrdinal == collation->oldestFrameOrdinal)
            {
                FreeOldestFrame(debugState, collation);
            }
            collationFrame = GetCollationFrame(collation);
        }
        InitFrame(collation, event->clock, collationFrame);
    }
    else
    {
        if(debugState->profiling)
        {
            Assert(collationFrame);
            u32 frameIndex = collation->totalFrameCount - 1;
            DebugThread* thread = GetDebugThread(debugState, collation, event->threadID);
            u64 relativeClock = event->clock - collationFrame->beginClock;
            
            switch(event->type)
            {
                case DebugType_beginCodeBlock:
                {
                    DebugProfileNode* parentNode = collationFrame->rootProfileNode;
                    u64 clockBasis = collationFrame->beginClock;
                    if(thread->currentOpenCodeBlock)
                    {
                        parentNode = thread->currentOpenCodeBlock->currentNode;
                        clockBasis = thread->currentOpenCodeBlock->beginClock;
                    }
                    else if(!parentNode)
                    {
                        DebugEvent nullEvent = {};
                        nullEvent.GUID = "unprofiled";
                        nullEvent.name = "unprofiled";
                        parentNode =  StoreEvent(debugState, collation, &nullEvent, false);
                        clockBasis = collationFrame->beginClock;
                        collationFrame->rootProfileNode = parentNode;
                        
                        if(!collation->currentRootElement)
                        {
                            collation->currentRootElement = parentNode->element;
                        }
                    }
                    
                    DebugProfileNode* node = StoreEvent(debugState, collation, event, true);
                    node->parentRelativeClock = event->clock - clockBasis;
                    node->threadOrdinal = (u16) thread->laneIndex;
                    node->coreIndex = event->coreIndex;
                    
                    node->nextSameParent = parentNode->firstChild;
                    parentNode->firstChild = node;
                    
                    OpenDebugBlock* debugBlock = 0;
                    FREELIST_ALLOC(debugBlock, debugState->firstFreeBlock, PushStruct(&debugState->debugPool, OpenDebugBlock));
                    debugBlock->beginClock = event->clock;
                    debugBlock->parent = thread->currentOpenCodeBlock;
                    debugBlock->currentNode = node;
                    thread->currentOpenCodeBlock = debugBlock;
                    
                } break;
                
                case DebugType_endCodeBlock:
                {
                    Assert(thread->ID == event->threadID);
                    OpenDebugBlock* matchingBlock = thread->currentOpenCodeBlock;
                    if(matchingBlock)
                    {
                        DebugProfileNode* node = matchingBlock->currentNode;
                        node->duration = event->clock - matchingBlock->beginClock;
                        
                        OpenDebugBlock* toFree = matchingBlock;
                        thread->currentOpenCodeBlock = toFree->parent;
                        FREELIST_DEALLOC(toFree, debugState->firstFreeBlock);
                        
                        if(thread->currentOpenCodeBlock)
                        {
                            DebugProfileNode* parentNode = thread->currentOpenCodeBlock->currentNode;
                            parentNode->durationOfChildren += node->duration;
                        }
                    }
                } break;
                
                InvalidDefaultCase;
            }
        }
    }
}


internal void DEBUGOverlay(EditorLayout* layout)
{
    DebugState* debugState = debugGlobalMemory->debugState;
    DebugCollationState* collation = debugState->showServerProfiling ? &debugState->serverState : &debugState->clientState;
    
    if(!collation->paused)
    {
        collation->viewingFrameOrdinal = collation->mostRecentFrameOrdinal;
    }
    
    Edit_b32(layout, "profiling", &debugState->profiling, 0, {});
    
    DebugFrame* mostRecentFrame = collation->frames + collation->viewingFrameOrdinal;
    NextRaw(layout);
    EditorTextDraw(layout, V4(1, 1, 1, 1), 0, "%2.2fms", mostRecentFrame->secondsElapsed * 1000.0f);
    Vec4 debugButtonColor = V4(0.2f, 0.5f, 1.0f, 1.0f);
    NextRaw(layout);
    if(StandardEditorButton(layout, "Client Profiling", auID(debugState, "client"), debugButtonColor))
    {
        debugState->showServerProfiling = false;
    }
    
    if(StandardEditorButton(layout, "Server Profiling", auID(debugState, "server"), debugButtonColor))
    {
        debugState->showServerProfiling = true;
    }
    
    if(debugState->profiling)
    {
        NextRaw(layout);
        if(StandardEditorButton(layout, "Pause", auID(debugState, "pause"), debugButtonColor))
        {
            collation->paused = !collation->paused;
        }
        
        if(StandardEditorButton(layout, "Capture Server Frame", auID(debugState, "capture server frame"), debugButtonColor))
        {
            SendOrderedMessage(CaptureFrame);
        }
        
        if(StandardEditorButton(layout, "Root", auID(debugState, "root"), debugButtonColor))
        {
            DebugFrame* frame = GetViewingFrame(collation);
            collation->currentRootElement = frame->rootProfileNode->element;
        }
        
        if(StandardEditorButton(layout, "Oldest", auID(debugState, "oldest"), debugButtonColor))
        {
            collation->viewingFrameOrdinal = collation->oldestFrameOrdinal;
        }
        
        if(StandardEditorButton(layout, "Most Recent", auID(debugState, "most recent"), debugButtonColor))
        {
            collation->viewingFrameOrdinal = collation->mostRecentFrameOrdinal;
        }
        
        NextRaw(layout);
        if(StandardEditorButton(layout, "Threads", auID(debugState, "threads"), debugButtonColor))
        {
            debugState->profilerType = Profiler_Threads;
        }
        
        if(StandardEditorButton(layout, "Frames", auID(debugState, "frames"), debugButtonColor))
        {
            debugState->profilerType = Profiler_Frames;
        }
        
        if(StandardEditorButton(layout, "Top Clock", auID(debugState, "top clock"), debugButtonColor))
        {
            debugState->profilerType = Profiler_TopList;
        }
        
        NextRaw(layout);
        
        Rect2 elementBounds = EditorElementBounds(layout, debugState->frameSliderDim);
        
        DrawFrameSlider(layout, collation, elementBounds);
        VerticalAdvance(layout, GetDim(elementBounds).y);
        
        Vec2 resizableP = V2(elementBounds.max.x, elementBounds.min.y);
        EditorResize(layout, resizableP, auID(debugState, "resize frame slider"), &debugState->frameSliderDim);
        
        if(collation->currentRootElement)
        {
            NextRaw(layout);
            Rect2 profilerBounds = EditorElementBounds(layout, debugState->profilerDim);
            PushRect(layout->group, FlatTransform(), profilerBounds, V4(0, 0, 0, 0.7f));
            switch(debugState->profilerType)
            {
                case Profiler_Threads:
                {
                    DrawProfiler(layout, collation, profilerBounds);
                } break;
                
                case Profiler_Frames:
                {
                    DrawFrameBars(layout, collation, profilerBounds);
                } break;
                
                case Profiler_TopList:
                {
                    DrawTopClockList(layout, collation, profilerBounds, &debugState->debugPool);
                } break;
            }
            
            VerticalAdvance(layout, GetDim(profilerBounds).y);
            resizableP = V2(profilerBounds.max.x, profilerBounds.min.y);
            
            EditorResize(layout, resizableP, auID(debugState, "resize profiler"), &debugState->profilerDim);
        }
        
    }
}

//void name()
extern "C" GAME_FRAME_END(GameDEBUGFrameEnd)
{
    if(!debugGlobalMemory->debugState)
    {
        DebugState* debugState = BootstrapPushStruct(DebugState, debugPool);
        debugGlobalMemory->debugState = debugState;
        
        debugState->frameSliderDim = V2(1700, 30);
        debugState->profilerDim = V2(1700, 900);
        
        debugState->clientState.collatingFrameOrdinal = 1;
        debugState->serverState.collatingFrameOrdinal = 1;
    }
    
    FlipTableResult flip = FlipDebugTable(globalDebugTable);
    
    DebugState* debugState = debugGlobalMemory->debugState;
    DebugCollationState* collation = &debugState->clientState;
    for(u32 eventIndex = 0; eventIndex < flip.eventCount; eventIndex++)
    {
        DebugEvent* event = flip.eventArray + eventIndex;
        CollateDebugEvent(debugState, collation, event);
    }
}
