#pragma once

enum EditorElementType
{
	EditorElement_String,
	EditorElement_Unsigned,
	EditorElement_Signed,
	EditorElement_Real,
	
    EditorElement_List,
	EditorElement_Struct,
    EditorElement_Taxonomy,
    EditorElement_EmptyTaxonomy,
    
    EditorElement_Animation,
    EditorElement_Count,
};

enum EditorElementFlags
{
    EditorElem_Pasted = (1 << 1),
    EditorElem_Deleted = (1 << 2),
    EditorElem_Editable = (1 << 3),
    EditorElem_Expanded = (1 << 4),
    EditorElem_CantBeDeleted = (1 << 5),
    EditorElem_AlwaysEditable = (1 << 6),
    EditorElem_LabelsEditable = (1 << 7),
    EditorElem_PlaySoundButton = (1 << 8),
    EditorElem_PlayEventSoundButton = (1 << 9),
    EditorElem_PlayEventButton = (1 << 10),
    EditorElem_RecursiveEmpty = (1 << 11),
    EditorElem_DontRender = (1 << 12),
};

struct EditorElement
{
	char name[32];
    u32 flags;
	u32 type;
    union
	{
		char value[32];
        
        struct
        {
            EditorElement* firstInList;
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
	};
    
	union
	{
		EditorElement* next;
		EditorElement* nextFree;
	};
	
};

struct EditorElementParents
{
    EditorElement* grandParents[8]; // NOTE(Leonardo): grandParents[0] is the grandParent
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
    r32 fontScale;
    Vec2 P;
    r32 additionalZBias;
    
    r32 nameValueDistance;
    r32 childStandardHeight;
};

enum EditorRole
{
    EditorRole_SoundDesigner = (1 << 1),
    EditorRole_GameDesigner = (1 << 2),
    
    EditorRole_Everyone = 0xffffffff,
};

enum EditorWidgetType
{
    EditorWidget_TaxonomyTree,
    EditorWidget_EditingTaxonomyTabs,
    EditorWidget_Animation,
    EditorWidget_SoundDatabase,
    EditorWidget_GeneralButtons,
    EditorWidget_SoundEvents,
    
    EditorWidget_Count,
};

struct WidgetPermanent
{
    Vec2 P;
    b32 expanded;
};

struct EditorWidget
{
    i32 changeCount;
    
    b32 needsToBeReloaded;
    u32 necessaryRole;
    
    WidgetPermanent permanent;
    
    EditorLayout layout;
    
    
    char name[32];
    EditorElement* root;
};

