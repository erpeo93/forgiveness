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
    EditorElem_AlwaysEditable = (1 << 6)
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

enum EditorTabType
{
    Tab_Bounds,
    Tab_PlayerMappings,
    Tab_Components,
    Tab_Effects,
    
    Tab_Count,
};

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

struct EditorWidget
{
    u32 buttonCount;
    
    Vec2 P;
    Vec2 dataOffset;
    EditorLayout layout;
    
    b32 expanded;
    
    char name[32];
    EditorElement* root;
};

