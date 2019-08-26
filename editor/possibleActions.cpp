inline PossibleAction* BeginPossibleAction(TaxonomySlot* slot, char* action, char* distance)
{
    PossibleAction* possibleAction = PushStruct(&currentSlot_->pool, PossibleAction);
    u8 actionInt = SafeTruncateToU8(GetValuePreprocessor(EntityAction, action));
    possibleAction->action = (EntityAction) actionInt;
    possibleAction->distance = ToR32(distance, 1.0f);
    possibleAction->flags = 0;
    
    possibleAction->next = slot->firstPossibleAction;
    slot->firstPossibleAction = possibleAction;
    
    return possibleAction;
}

inline void AddActionFlag(PossibleAction* possibleAction, char* flagName)
{
    u32 flag = GetFlagPreprocessor(CanDoActionFlags, flagName);
    possibleAction->flags |= flag;
}


inline void AddActor(PossibleAction* action, char* name, char* requiredTime)
{
    TaxonomySlot* target = NORUNTIMEGetTaxonomySlotByName(taxTable_, name);
    if(target)
    {
        TaxonomyNode* node = AddToTaxonomyTree(&action->tree, target);
        node->data.action.requiredTime = ToR32(requiredTime, 1.0f);
    }
    else
    {
        EditorErrorLog(name);
    }
}


internal void ImportPossibleActionsTab(TaxonomySlot* slot, EditorElement* root)
{
    EditorElement* actions = root->firstInList;
    while(actions)
    {
        char* action = GetValue(actions, "action");
        char* distance = GetValue(actions, "distance");
        
        PossibleAction* possibleAction = BeginPossibleAction(slot, action, distance);
        
        EditorElement* flags = GetList(actions, "flags");
        while(flags)
        {
            char* flagName = GetValue(flags, "name");
            AddActionFlag(possibleAction, flagName);
            
            flags = flags->next;
        }
        
        EditorElement* actors = GetList(actions, "actors");
        while(actors)
        {
            char* actorName = GetValue(actors, "taxonomyName");
            char* requiredTime = GetValue(actors, "time");
            AddActor(possibleAction, actorName, requiredTime);
            
            actors = actors->next;
        }
        
        actions = actions->next;
    }
}