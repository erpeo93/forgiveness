inline void AddParam_(ConsiderationParams* params, ExpressionValue value)
{
    Assert(params->paramCount < ArrayCount(params->params));
    params->params[params->paramCount++] = value;
}

inline ExpressionValue Divide(ExpressionValue o1, ExpressionValue o2)
{
    ExpressionValue result = {};
    Assert(o1.type == o2.type);
    switch(o1.type)
    {
        case ExpressionValue_r32:
        {
            result = ExpressionVal(R32_MAX);
            if(o2.value_r32 != 0.0f)
            {
                result = ExpressionVal(o1.value_r32 / o2.value_r32);
            }
        } break;
        
        case ExpressionValue_Vec3:
        {
            InvalidCodePath;
        }
    }
    
    return result;
}


inline ExpressionValue Mul(ExpressionValue o1, ExpressionValue o2)
{
    ExpressionValue result = {};
    Assert(o1.type == o2.type);
    switch(o1.type)
    {
        case ExpressionValue_r32:
        {
            result = ExpressionVal(o1.value_r32 * o2.value_r32);
        } break;
        
        case ExpressionValue_Vec3:
        {
            result = ExpressionVal(Hadamart(o1.value_Vec3, o2.value_Vec3));
        } break;
    }
    
    return result;
}

inline ExpressionValue Sum(ExpressionValue o1, ExpressionValue o2)
{
    ExpressionValue result = {};
    Assert(o1.type == o2.type);
    switch(o1.type)
    {
        case ExpressionValue_r32:
        {
            result = ExpressionVal(o1.value_r32 + o2.value_r32);
        } break;
        
        case ExpressionValue_Vec3:
        {
            result = ExpressionVal(o1.value_Vec3 + o2.value_Vec3);
        }
    }
    
    return result;
}


inline ExpressionValue Subtract(ExpressionValue o1, ExpressionValue o2)
{
    ExpressionValue result = {};
    Assert(o1.type == o2.type);
    switch(o1.type)
    {
        case ExpressionValue_r32:
        {
            result = ExpressionVal(o1.value_r32 - o2.value_r32);
        } break;
        
        case ExpressionValue_Vec3:
        {
            result = ExpressionVal(o1.value_Vec3 - o2.value_Vec3);
        }
    }
    
    return result;
}


inline ExpressionValue LengthSq(ExpressionValue o1)
{
    ExpressionValue result = {};
    switch(o1.type)
    {
        case ExpressionValue_r32:
        {
            InvalidCodePath;
        } break;
        
        case ExpressionValue_Vec3:
        {
            result = ExpressionVal(LengthSq(o1.value_Vec3));
        }
    }
    
    return result;
}

#define SLOWGetRuntimeOffsetOf(type, name,...) SLOWGetRuntimeOffsetOf_((MemberDefinition*) &(memberDefinitionOf##type), ArrayCount(memberDefinitionOf##type), name, __VA_ARGS__) 

internal MemberDefinition SLOWGetRuntimeOffsetOf_(MemberDefinition* definition, u32 memberCount, char* name, u32 nameLength = 0)
{
    MemberDefinition result = {};
    
    if(!nameLength)
    {
        nameLength = StrLen(name);
    }
    for(u32 attrIndex = 0; attrIndex < memberCount; ++attrIndex)
    {
        MemberDefinition* member = definition + attrIndex;
        char* test = member->name;
        if(!test[0])
        {
            break;
        }
        
        if(StrEqual(nameLength, name, test))
        {
            result = *member;
            break;
        }
        
    }
    
    return result;
}

inline ExpressionValue GetCreatureAttribute(void* ptr, char* attrName, u32 attrLength)
{
    ExpressionValue result = {};
    MemberDefinition member = SLOWGetRuntimeOffsetOf(CreatureComponent, attrName, attrLength);
    u8* valuePtr = (u8*) ptr + member.offset;
    
    switch(member.type)
    {
        case MetaType_r32:
        {
            result = ExpressionVal(*((r32*) valuePtr));
        } break;
        
        case MetaType_Vec3:
        {
            result = ExpressionVal(*((Vec3*) valuePtr));
        } break;
        
        InvalidDefaultCase;
    }
    
    return result;
}

inline ExpressionValue GetGenericAttribute(void* ptr, char* attrName, u32 attrLength)
{
    ExpressionValue result = {};
    MemberDefinition member = SLOWGetRuntimeOffsetOf(SimEntity, attrName, attrLength);
    u8* valuePtr = (u8*) ptr + member.offset;
    
    switch(member.type)
    {
        case MetaType_r32:
        {
            result = ExpressionVal(*((r32*) valuePtr));
        } break;
        
        case MetaType_Vec3:
        {
            result = ExpressionVal(*((Vec3*) valuePtr));
        } break;
        
        InvalidDefaultCase;
    }
    
    return result;
}

inline ExpressionValue Evaluate(ExpressionContext* context, Tokenizer* tokenizer)
{
    ExpressionValue result = {};
    EatAllWhiteSpace(tokenizer);
    
    b32 parsing = true;
    while(parsing)
    {
        Token t = GetToken(tokenizer);
        if(t.type == Token_Number)
        {
            result = ExpressionVal(R32FromChar(t.text));
        }
        else if(t.type == Token_SemiColon)
        {
        }
        else if(t.type == Token_EndOfFile)
        {
            parsing = false;
        }
        else if(t.type == Token_Identifier)
        {
            if(TokenEquals(t, "SetResult"))
            {
                context->resultSet = true;
                
                RequiresToken(tokenizer, OpenParen);
                result = Evaluate(context, tokenizer);
                RequiresToken(tokenizer, CloseParen);
                context->result = result;
            }
            else if(TokenEquals(t, "Not"))
            {
                RequiresToken(tokenizer, OpenParen);
                ExpressionValue o1 = Evaluate(context, tokenizer);
                
                Assert(o1.type == ExpressionValue_r32);
                b32 boolean = (b32) o1.value_r32;
                result = ExpressionVal((r32) !boolean);
                RequiresToken(tokenizer, CloseParen);
            }
            else if(TokenEquals(t, "GtEq"))
            {
                RequiresToken(tokenizer, OpenParen);
                ExpressionValue o1 = Evaluate(context, tokenizer);
                RequiresToken(tokenizer, Comma);
                ExpressionValue o2 = Evaluate(context, tokenizer);
                
                Assert(o1.type == ExpressionValue_r32);
                Assert(o2.type == ExpressionValue_r32);
                
                b32 bigger = (o1.value_r32 >= o2.value_r32);
                result = ExpressionVal((r32)bigger);
                RequiresToken(tokenizer, CloseParen);
            }
            else if(TokenEquals(t, "Square"))
            {
                RequiresToken(tokenizer, OpenParen);
                ExpressionValue o1 = Evaluate(context, tokenizer);
                
                result = Mul(o1, o1);
                RequiresToken(tokenizer, CloseParen);
            }
            else if(TokenEquals(t, "sum"))
            {
                RequiresToken(tokenizer, OpenParen);
                ExpressionValue o1 = Evaluate(context, tokenizer);
                RequiresToken(tokenizer, Comma);
                ExpressionValue o2 = Evaluate(context, tokenizer);
                
                result = Sum(o1, o2);
                RequiresToken(tokenizer, CloseParen);
            }
            else if(TokenEquals(t, "result"))
            {
                result = context->result;
            }
            else if(TokenEquals(t, "Diff"))
            {
                RequiresToken(tokenizer, OpenParen);
                ExpressionValue o1 = Evaluate(context, tokenizer);
                RequiresToken(tokenizer, Comma);
                ExpressionValue o2 = Evaluate(context, tokenizer);
                
                result = Subtract(o1, o2);
                RequiresToken(tokenizer, CloseParen);
            }
            else if(TokenEquals(t, "LengthSq"))
            {
                RequiresToken(tokenizer, OpenParen);
                ExpressionValue o1 = Evaluate(context, tokenizer);
                result = LengthSq(o1);
                RequiresToken(tokenizer, CloseParen);
            }
            else if(TokenEquals(t, "params"))
            {
                RequiresToken(tokenizer, OpenBracket);
                Token number = GetToken(tokenizer);
                Assert(number.type = Token_Number);
                
                u32 paramIndex = atoi(number.text);
                Assert(paramIndex < context->params.paramCount);
                result = context->params.params[paramIndex];
                RequiresToken(tokenizer, CloseBracket);
            }
            else if(TokenEquals(t, "Div"))
            {
                RequiresToken(tokenizer, OpenParen);
                ExpressionValue o1 = Evaluate(context, tokenizer);
                RequiresToken(tokenizer, Comma);
                ExpressionValue o2 = Evaluate(context, tokenizer);
                
                result = Divide(o1, o2);
                RequiresToken(tokenizer, CloseParen);
            }
            else if(TokenEquals(t, "targetGen"))
            {
                RequiresToken(tokenizer, OpenParen);
                Token attributeName = GetToken(tokenizer);
                
                result = GetGenericAttribute(context->target, attributeName.text, attributeName.textLength);
                
                RequiresToken(tokenizer, CloseParen);
            }
            else if(TokenEquals(t, "target"))
            {
                RequiresToken(tokenizer, OpenParen);
                Token attributeName = GetToken(tokenizer);
                
                result = GetCreatureAttribute(context->target, attributeName.text, attributeName.textLength);
                
                RequiresToken(tokenizer, CloseParen);
            }
            else if(TokenEquals(t, "selfGen"))
            {
                RequiresToken(tokenizer, OpenParen);
                Token attributeName = GetToken(tokenizer);
                
                result = GetGenericAttribute(context->self, attributeName.text, attributeName.textLength);
                RequiresToken(tokenizer, CloseParen);
            }
            else if(TokenEquals(t, "self"))
            {
                RequiresToken(tokenizer, OpenParen);
                Token attributeName = GetToken(tokenizer);
                
                result = GetCreatureAttribute(context->self, attributeName.text, attributeName.textLength);
                RequiresToken(tokenizer, CloseParen);
            }
            else if(TokenEquals(t, "endLoop"))
            {
                result.quit = true;
            }
            else if(TokenEquals(t, "foreach"))
            {
                RequiresToken(tokenizer, OpenParen);
                Token loop = GetToken(tokenizer);
                RequiresToken(tokenizer, CloseParen);
                RequiresToken(tokenizer, OpenBraces);
                
                if(loop.type == Token_Identifier)
                {
                    if(TokenEquals(loop, "target"))
                    {
                        for(u32 targetIndex = 0; targetIndex < context->targetCount; ++targetIndex)
                        {
                            context->target = context->targets[targetIndex];
                            result = Evaluate(context, tokenizer);
                            if(result.quit)
                            {
                                break;
                            }
                        }
                    }
                    else
                    {
                        InvalidCodePath;
                    }
                }
                else
                {
                    InvalidCodePath;
                }
                
                RequiresToken(tokenizer, CloseBraces);
            }
            else if(TokenEquals(t, "if"))
            {
                RequiresToken(tokenizer, OpenParen);
                ExpressionValue boolean = Evaluate(context, tokenizer);
                RequiresToken(tokenizer, CloseParen);
                RequiresToken(tokenizer, OpenBraces);
                
                Assert(boolean.type == ExpressionValue_r32);
                Assert(boolean.value_r32 == 0.0f || boolean.value_r32 == 1.0f);
                
                if(boolean.value_r32 == 1.0f)
                {
                    result = Evaluate(context, tokenizer);
                }
                else
                {
                    u32 currentLevel = 1;
                    while(currentLevel > 0)
                    {
                        Token probe = GetToken(tokenizer);
                        if(probe.type == Token_OpenBraces)
                        {
                            ++currentLevel;
                        }
                        else if(probe.type == Token_CloseBraces)
                        {
                            --currentLevel;
                        }
                    }
                }
            }
            else if(TokenEquals(t, "Combine"))
            {
                
                InvalidCodePath;
#if 0
                RequiresToken(tokenizer, OpenParen);
                
                u32 currentMax = ?;
                u32 maxScore = 0;
                
                for(everyTile)
                {
                    u32 currentScore = 0;
                    for(everyInfluenceMap)
                    {
                        u32 value = value;
                        if(Min)
                        {
                            value = 255 - value;
                        }
                        
                        currentScore += value;
                    }
                    
                    if(currentScore > maxScore)
                    {
                        currentMax = tile;
                        maxScore = currentScore;
                    }
                }
#endif
                
            }
            else if(TokenEquals(t, "Average"))
            {
                InvalidCodePath;
            }
            else
            {
                TaxonomySlot* slot = NORUNTIMEGetTaxonomySlotByName(context->region->taxTable, t.text, t.textLength);
                
#if 0                
                if(slot->InfluenceMap)
                {
                    if(!AlreadyPresent(behavior, slot->influenceMap))
                    {
                        Insert(behavior, influenceMap);
                    }
                    GetInfluenceMap(context, slot->taxonomy);
                    ???;
                }
                else
#endif
                
                {
                    if(slot->considerationDefinition)
                    {
                        char* considerationExpression = slot->considerationDefinition->expression;
                        ConsiderationParams oldParams = context->params;
                        context->params.paramCount = 0;
                        
                        RequiresToken(tokenizer, OpenParen);
                        
                        b32 parsingParams = true;
                        while(parsingParams)
                        {
                            Token probe = GetToken(tokenizer);
                            switch(probe.type)
                            {
                                case Token_CloseParen:
                                {
                                    parsingParams = false;
                                } break;
                                
                                case Token_Comma:
                                {
                                    
                                } break;
                                
                                case Token_Number:
                                {
                                    r32 value = R32FromChar(probe.text);
                                    AddParam_(&context->params, ExpressionVal(value));
                                } break;
                                
                                case Token_Identifier:
                                {
                                    if(TokenEquals(probe, "params"))
                                    {
                                        RequiresToken(tokenizer, OpenBracket);
                                        
                                        Token number = GetToken(tokenizer);
                                        Assert(number.type == Token_Number);
                                        u32 paramIndex = atoi(number.text);
                                        
                                        Assert(paramIndex < oldParams.paramCount);
                                        AddParam_(&context->params, oldParams.params[paramIndex]);
                                        RequiresToken(tokenizer, CloseBracket);
                                    }
                                    else
                                    {
                                        InvalidCodePath;
                                    }
                                } break;
                                
                                default:
                                {
                                    tokenizer->at -= probe.textLength;
                                } break;
                            }
                        }
                        
                        Tokenizer expression = {};
                        expression.at = considerationExpression;
                        result = Evaluate(context, &expression);
                        context->params = oldParams;
                    }
                    else
                    {
                    }
                }
            }
        }
        else
        {
            tokenizer->at -= t.textLength;
            parsing = false;
        }
    }
    
    return result;
}

inline r32 EvalResponseCurve(ResponseCurve* curve, r32 value)
{
    Assert(Normalized(value));
    r32 result = value;
    Assert(Normalized(result));
    
    return result;
}

inline r32 EvaluateConsideration(ExpressionContext* context, Consideration* consideration)
{
    context->resultSet = 0;
    
    
    r32 expressionValue = 0;
#if 0
    u64 considerationHash = ??;
    u32 bucketIndex = considerationHash & (ArrayCount() - 1);
    
    CachedExpression* cached = ??;
    if(cached->taxonomy == consideration->taxonomy)
    {
        result = cached->value;
    }
    else
#endif
    {
        Tokenizer expression = {};
        expression.at = consideration->expression;
        ExpressionValue expressionResult = Evaluate(context, &expression);
        if(context->resultSet)
        {
            expressionResult = context->result;
        }
        
        Assert(expressionResult.type == ExpressionValue_r32);
        expressionValue = expressionResult.value_r32;
        //Cache(considerationHash, result);
    }
    
    r32 expressionNormal = Clamp01MapToRange(consideration->bookEndMin, expressionValue, consideration->bookEndMax);
    r32 result = EvalResponseCurve(&consideration->curve, expressionNormal);
    return result;
}
