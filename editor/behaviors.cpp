internal void ImportBehaviorsTab(TaxonomySlot* slot, EditorElement* root)
{
    slot->hasBrain = true;
}


inline AIAction* AddSpecialAction(AIStateMachine* sm, EntitySpecialAction action)
{
    AIAction* result = 0;
    if(sm->actionCount < ArrayCount(sm->actions))
    {
        result = sm->actions + sm->actionCount++; 
        result->command.type = AIAction_SpecialAction;
        result->command.data.specialAction = action;
        result->transitionCount = 0;
    }
    
    return result;
}

inline AIAction* AddAction(AIStateMachine* sm, EntityAction action, u32 taxonomy)
{
    AIAction* result = 0;
    if(sm->actionCount < ArrayCount(sm->actions))
    {
        result = sm->actions + sm->actionCount++; 
        result->command.type = AIAction_StandardAction;
        result->command.data.standardAction = action;
        result->command.data.conceptTaxonomy = taxonomy;
        result->transitionCount = 0;
    }
    
    return result;
}

inline AIStateMachineTransition* AddTransition(AIStateMachine* sm, AIAction* a1, AIAction* a2)
{
    AIStateMachineTransition* result = 0;
    if(a1->transitionCount < ArrayCount(a1->transitions))
    {
        result = a1->transitions + a1->transitionCount++;
        result->destActionIndex = (u32) (a2 - sm->actions);
    }
    
    return result;
}

inline AICondition* AddCondition(AIStateMachineTransition* transition, AIConditionType type, b32 negated = false)
{
    AICondition* result = 0;
    if(transition->conditionCount < ArrayCount(transition->conditions))
    {
        result = transition->conditions + transition->conditionCount++;
        result->type = type;
        result->negated = negated;
    }
    
    return result;
}

inline void InitDefaultStateMachine()
{
    AIStateMachine* sm = &taxTable_->testStateMachine;
    
    TaxonomySlot* testSlot = NORUNTIMEGetTaxonomySlotByName(taxTable_, "centaur");
    u32 hostileTaxonomy = testSlot->taxonomy;
    
    AIAction* action = AddSpecialAction(sm, SpecialAction_MoveLeft);
    AIAction* action2 = AddSpecialAction(sm, SpecialAction_MoveRight);
    AIAction* action3 = AddAction(sm, Action_Attack, hostileTaxonomy);
    
    AIStateMachineTransition* transition = AddTransition(sm, action, action2);
    AICondition* condition = AddCondition(transition, AICondition_DoingActionFor);
    condition->data.time = 2.0f;
    
    AIStateMachineTransition* transition2 = AddTransition(sm, action2, action);
    AICondition* condition2 = AddCondition(transition2, AICondition_DoingActionFor);
    condition2->data.time = 2.0f;
    
    AIStateMachineTransition* transition3 = AddTransition(sm, action, action3);
    AICondition* condition3 = AddCondition(transition3, AICondition_OnSight);
    condition3->data.taxonomy = hostileTaxonomy;
    
    AIStateMachineTransition* transition4 = AddTransition(sm, action3, action);
    AICondition* condition4 = AddCondition(transition4, AICondition_OnSight, true);
    condition4->data.taxonomy = hostileTaxonomy;                                          
}
