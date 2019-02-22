#pragma once

enum ForgTokenType
{
    Token_Unknown,
    Token_Identifier,
    Token_OpenParen,
    Token_CloseParen,
    Token_Asterisk,
    Token_Pound,
    Token_OpenBracket,
    Token_CloseBracket,
    Token_OpenBraces,
    Token_CloseBraces,
    Token_SemiColon,
    Token_EndOfFile,
    Token_String,
    Token_EqualSign,
    Token_Number,
    Token_Comma,
    Token_Colon,
};

struct Token
{
    ForgTokenType type;
    
    int textLength;
    char* text;
};

struct Tokenizer
{
    char* at;
};


