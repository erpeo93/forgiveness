#pragma once

struct DebugState;
struct DebugCollationState;
enum DebugInteractionType
{
    DebugInteraction_none,
    DebugInteraction_NOP,
    DebugInteraction_drag,
    DebugInteraction_toggle,
    DebugInteraction_tear,
    DebugInteraction_move,
    DebugInteraction_resize,
    DebugInteraction_select,
    DebugInteraction_toggleExpansion,
    DebugInteraction_setUInt32,
    DebugInteraction_setPointer,
    
    DebugInteraction_autoModified,
};

struct DebugInteraction
{
    DebugInteractionType type;
    DebugID ID;
    
    void* target;
    union
    {
        void* generic;
        u32 UInt32;
        void* pointer;
        DebugType debugType;
        DebugTree* tree;
        Vec2* P;
        DebugElement* element;
        DebugEventLink* link;
    };
    
};

struct Layout
{
};

struct LayoutElement
{
    Layout* layout;
    Vec2* dim;
    Vec2* size;
    DebugInteraction interaction;
    
    Rect2 bounds;
};

