#pragma once


enum UIBookMode
{
    UIBook_Recipes,
    UIBook_Skills,
    UIBook_Count
};

enum UIBookmarkPosition
{
    UIBookmark_RightUp,
    UIBookmark_RightSide,
    UIBookmark_LeftUp,
    UIBookmark_LeftSide,
    
    UIBookmark_Count
};

enum UIBookmarkConditionType
{
    UICondition_Mode,
    UICondition_Name,
    UICondition_Rarity
};

struct UIBookmarkCondition
{
    UIBookmarkConditionType type;
    union
    {
        UIBookMode mode;
    };
};

struct UIBookmark
{
    b32 active;
    UIBookmarkPosition position;
    UIBookmarkCondition condition;
};


enum BookElementType
{
    Book_Recipe,
    Book_RecipeCategory,
    Book_Skill,
    Book_SkillCategory
};

struct BookElement
{
    BookElementType type;
    
    b32 hot;
    r32 securityTimer;
    u32 taxonomy;
    union
    {
        GenerationData gen;
        struct
        {
            u32 skillLevel;
            r32 skillPower;
        };
        
        b32 unlocked;
    };
};

struct BookElementsBlock
{
    u32 elementCount;
    BookElement elements[16];
    
    union
    {
        BookElementsBlock* next;
        BookElementsBlock* nextFree;
    };
};

struct BookReference
{
    BookElement* element;
    
    union
    {
        BookReference* next;
        BookReference* nextFree;
    };
};

struct BookMode
{
    u32 rootTaxonomy;
    u32 filterTaxonomy;
    u32 filteredElementCount;
    BookReference* filteredElements;
    
    r32 targetSecurityTimer;
    u32 currentElementBookIndex;
    
    BookElementsBlock* elements;
};


