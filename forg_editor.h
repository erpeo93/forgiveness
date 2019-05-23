#pragma once

enum EditorElementType
{
	EditorElement_String,
	EditorElement_Unsigned,
	EditorElement_Signed,
	EditorElement_Real,
	
    EditorElement_List,
	EditorElement_Text,
	EditorElement_Struct,
    EditorElement_Taxonomy,
    EditorElement_EmptyTaxonomy,
    
    EditorElement_Animation,
    EditorElement_ColorPicker,
    
    EditorElement_GroundParams,
    EditorElement_Count,
};

enum EditorElementFlags
{
    EditorElem_Pasted = (1 << 1),
    EditorElem_Deleted = (1 << 2),
    EditorElem_Editable = (1 << 3),
    EditorElem_Expanded = (1 << 4),
    EditorElem_CantBeDeleted = (1 << 5),
    EditorElem_AtLeastOneInList = (1 << 6),
    EditorElem_AlwaysEditable = (1 << 7),
    EditorElem_LabelsEditable = (1 << 8),
    EditorElem_PlaySoundButton = (1 << 9),
    EditorElem_PlayEventSoundButton = (1 << 10),
    EditorElem_EquipInAnimationButton = (1 << 11),
    EditorElem_ShowLabelBitmapButton = (1 << 12),
    EditorElem_PlayEventButton = (1 << 13),
    EditorElem_PlayContainerButton = (1 << 14),
    EditorElem_RecursiveEmpty = (1 << 15),
    EditorElem_DontRender = (1 << 16),
    EditorElem_Union = (1 << 17),
};

struct EditorUnionLayout
{
    char unionName[32];
    char discriminatorName[32];
};

struct EditorTextBlock
{
    char text[KiloBytes(100)];
    
    union
    {
        EditorTextBlock* next;
        EditorTextBlock* nextFree;
    };
};

struct EditorElement
{
	char name[32];
    u32 versionNumber;
    u32 flags;
	u32 type;
    union
	{
		char value[32];
        EditorTextBlock* text;
        
        struct
        {
            EditorElement* firstInList;
            char labelName[16];
            char elementName[16];
            EditorElement* emptyElement;
		};
        
        EditorElement* firstValue;
        u32 parentTaxonomy;
        
        struct
        {
            EditorElement* firstChild;
            u32 taxonomy;
        };
        
        struct
        {
            b32 scrolling;
            r32 relativeMouseX;
            r32 norm;
            
            b32 scrolling2;
            Vec2 relativeMouseP;
            r32 distanceBlack;
            r32 distanceWhite;
            r32 distanceColor;
            
            b32 zooming;
        };
	};
    
    
    
	union
	{
		EditorElement* next;
		EditorElement* nextFree;
	};
	
};

struct EditorElementParents
{
    EditorElement* grandParents[16]; // NOTE(Leonardo): grandParents[0] is the grandParent
    EditorElement* father;
};

inline b32 IsSet(EditorElement* element, u32 flags)
{
    b32 result = element->flags & flags;
    return result;
}

inline void ClearFlags(EditorElement* element, u32 flags)
{
    element->flags &= ~flags;
}

inline void AddFlags(EditorElement* element, u32 flags)
{
    element->flags |= flags;
}

#define MAX_DATA_FILE_NAME_LENGTH 64
struct DataFileArrived
{
    char name[MAX_DATA_FILE_NAME_LENGTH];
    
    u32 chunkSize;
    u32 fileSize;
    u32 runningFileSize;
    u8* data;
    
    DataFileArrived* next;
};

struct EditorTabStack
{
    u32 counter;
    EditorElement* result;
    struct EditorElement* stack[16];
    u32 previousElementType;
};


struct EditorLayout
{
    r32 lineThickness;
    r32 lineSegmentLength;
    r32 lineSpacing;
    r32 padding;
    
    r32 squareDim;
    r32 fontScale;
    Vec2 P;
    r32 additionalZBias;
    
    r32 nameValueDistance;
    r32 childStandardHeight;
};

printFlags(noPrefix) enum EditorRole
{
    EditorRole_SoundDesigner = (1 << 1),
    EditorRole_Composer = (1 << 2),
    EditorRole_GameDesigner = (1 << 3),
    EditorRole_Writer = (1 << 4),
    EditorRole_Animator = (1 << 5),
    EditorRole_Artist = (1 << 6),
    EditorRole_WebDeveloper = (1 << 7),
};

enum EditorWidgetType
{
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
    
    EditorWidget_Count,
};

struct WidgetPermanent
{
    r32 fontSize;
    Vec2 P;
    Vec2 resizeP;
    
    b32 expanded;
};

struct EditorWidget
{
    i32 changeCount;
    
    b32 needsToBeReloaded;
    u32 necessaryRole;
    
    WidgetPermanent permanent;
    r32 dataOffsetY;
    
    EditorLayout layout;
    
    b32 moving;
    i32 movingClipWidth;
    i32 movingClipHeight;
    
    i32 oldClipWidth;
    i32 oldClipHeight;
    
    i32 maxDataY;
    i32 minDataY;
    
    char fileName[32];
    char name[32];
    EditorElement* root;
};

