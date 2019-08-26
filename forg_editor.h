#pragma once


#define EditorRole_Everyone 0xffffffff
printFlags(noPrefix) enum EditorRole
{
    EditorRole_SoundDesigner = (1 << 1),
    EditorRole_Composer = (1 << 2),
    EditorRole_GameDesigner = (1 << 3),
    EditorRole_Writer = (1 << 4),
    EditorRole_Animator = (1 << 5),
    EditorRole_Artist = (1 << 6),
    EditorRole_WebDeveloper = (1 << 7),
    EditorRole_3DModeller = (1 << 8),
};

printTable(noPrefix) enum EditorWidgetType
{
    EditorWidget_None,
    EditorWidget_TaxonomyTree,
    EditorWidget_EditingTaxonomyTabs,
    EditorWidget_Animation,
    EditorWidget_SoundDatabase,
    EditorWidget_GeneralButtons,
    EditorWidget_SoundEvents,
    EditorWidget_Components,
    EditorWidget_Misc,
    EditorWidget_ColorPicker,
    EditorWidget_GroundParams,
    EditorWidget_Debug3DModels,
    EditorWidget_WidgetSelection,
    
    EditorWidget_Count,
};

struct WidgetPermanent
{
    r32 fontSize;
    Vec2 P;
    Vec2 resizeP;
};

struct EditorWidget
{
    i32 changeCount;
    
    b32 needsToBeReloaded;
    u32 necessaryRole;
    
    WidgetPermanent permanent;
    r32 dataOffsetY;
    
    b32 moving;
    i32 movingClipWidth;
    i32 movingClipHeight;
    
    i32 oldClipWidth;
    i32 oldClipHeight;
    
    i32 maxDataY;
    i32 minDataY;
    
    char fileName[32];
    char name[32];
};

