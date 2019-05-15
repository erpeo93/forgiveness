#pragma once

enum ForgASTOperation
{
    ASTOperation_None,
    ASTOperation_Sum,
    ASTOperation_Mul,
    ASTOperation_Sub,
    ASTOperation_Div,
    ASTOperation_Assign,
};


enum ASTNodeValueType
{
    ASTValue_None,
    ASTValue_R32,
    ASTValue_ContextualR32,
};

enum ASTContextualType
{
    ASTContextual_Actor,
    ASTContextual_Target,
};

struct ASTContextualAttribute
{
    ASTContextualType type;
    u32 offset;
};

struct ASTNodeValue
{
    ASTNodeValueType type;
    
    union
    {
        r32 value_R32;
        ASTContextualAttribute contextual_R32;
    };
};

struct ForgASTNode
{
    ForgASTOperation operation;
    
    ASTNodeValue value;
    u8 operandCount;
    ForgASTNode* operands[2];
    
    union
    {
        ForgASTNode* next;
        ForgASTNode* nextFree;
    };
};

struct ForgAST
{
    ForgASTNode* root;
};

