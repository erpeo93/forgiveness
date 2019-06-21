
inline void Spawn(Effect* effect, char* name)
{
    TaxonomySlot* spawnSlot = NORUNTIMEGetTaxonomySlotByName(taxTable_, name);
    if(spawnSlot)
    {
        if(effect)
        {
            effect->data.taxonomy = spawnSlot->taxonomy;
        }
    }
    else
    {
        EditorErrorLog(name);
    }
}

internal void ParseEffect(EditorElement* element, Effect* dest)
{
    
    char* action = GetValue(element, "action");
    char* effectName = GetValue(element, "effectID");
    char* minIntensity = GetValue(element, "minIntensity");
    char* maxIntensity = GetValue(element, "maxIntensity");
    
    
    dest->triggerAction = (EntityAction) GetValuePreprocessor(EntityAction, action);
    dest->ID = (EffectIdentifier) GetValuePreprocessor(EffectIdentifier, effectName);
    dest->minIntensity = ToR32(minIntensity, 1.0f);
    dest->maxIntensity = ToR32(maxIntensity, 1.0f);
    
    
    char* timer = GetValue(element, "timer");
    if(timer)
    {
        dest->targetTimer = ToR32(timer);
    }
    
    EditorElement* flagList = GetList(element, "flags");
    while(flagList)
    {
        char* flag = GetValue(flagList, "name");
        u32 flagValue = GetFlagPreprocessor(EffectFlags, flag);
        dest->flags |= flagValue;
        
        flagList = flagList->next;
    }
    
    EditorElement* range = GetStruct(element, "range");
    char* rangeType = GetValue(range, "rangeType");
    char* rangeValue = GetValue(range, "radious");
    dest->range.type = (EffectTargetRangeType) GetValuePreprocessor(EffectTargetRangeType, rangeType);
    dest->range.radious = ToR32(rangeValue, 0.0f);
    
    dest->data = {};
    EditorElement* params = GetList(element, "effectParams");
    while(params)
    {
        char* paramName = GetValue(params, "effectParamName");
        if(StrEqual(paramName, "velocity"))
        {
            dest->data.speed = ToV3(GetElement(params, "velocity"));
        }
        else if(StrEqual(paramName, "offset"))
        {
            dest->data.offset = ToV3(GetElement(params, "offset"));
        }
        else if(StrEqual(paramName, "offsetV"))
        {
            dest->data.offsetV = ToV3(GetElement(params, "offsetV"));
        }
        else if(StrEqual(paramName, "taxonomyName"))
        {
            char* taxonomy = GetValue(params, "taxonomyName");
            Spawn(dest, taxonomy);
        }
        else if(StrEqual(paramName, "timeToLive"))
        {
            dest->data.timeToLive = ToR32(GetValue(params, "timeToLive"));
        }
        else if(StrEqual(paramName, "spawnCount"))
        {
            dest->data.spawnCount = ToU32(GetValue(params, "spawnCount"));
        }
        
        
        params = params->next;
    }
}
