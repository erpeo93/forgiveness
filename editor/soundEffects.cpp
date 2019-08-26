inline TaxonomySound* AddSoundEffect(TaxonomySlot* slot, char* animationName, char* threesoldIn, char* eventName)
{
    r32 threesold = ToR32(threesoldIn);
    u64 animationHash = StringHash(animationName);
    u64 eventHash = StringHash(eventName);
    
    TaxonomySound* dest = PushStruct(&currentSlot_->pool, TaxonomySound);
    
    dest->animationNameHash = animationHash;
    dest->threesold = threesold;
    dest->eventNameHash = eventHash;
    
    FREELIST_INSERT(dest, slot->firstSound);
    
    return dest;
}


internal void ImportSoundEffectsTab(TaxonomySlot* slot, EditorElement* root)
{
    
    EditorElement* effects = root->firstInList;
    while(effects)
    {
        char* animationName = GetValue(effects, "animationName");
        char* time = GetValue(effects, "time");
        char* event = GetValue(effects, "name");
        
        
        TaxonomySound* sound = AddSoundEffect(slot, animationName, time, event);
        
        
#if 0            
        EditorElement* labels = GetList(effects, "labels");
        while(labels)
        {
            char* labelName = GetValue(labels, "name");
            char* labelValue = GetValue(labels, "value");
            
            AddLabel(sound, labelName, labelValue);
            
            labels = labels->next;
        }
#endif
        
        effects = effects->next;
    }
}