inline SoundContainer* AddSoundEvent(char* eventName)
{
    Assert(taxTable_->eventCount < ArrayCount(taxTable_->events));
    
    SoundEvent* event = taxTable_->events + taxTable_->eventCount++;
    event->eventNameHash = StringHash(eventName);
    SoundContainer* result = &event->rootContainer;
    
    event->rootContainer.soundCount = 0;
    event->rootContainer.firstSound = 0;
    
    event->rootContainer.containerCount = 0;
    event->rootContainer.firstChildContainer = 0;
    
    event->rootContainer.labelCount = 0;
    return result;
}

inline LabeledSound* AddSoundToContainer(SoundContainer* container, char* soundType, char* soundName, r32 delay, r32 decibelOffset, r32 pitch, r32 toleranceDistance, r32 distanceFalloffCoeff)
{
    ++container->soundCount;
    
    // TODO(Leonardo): make it's own pool here!
    LabeledSound* sound = PushStruct(&taxTable_->pool_, LabeledSound);
    
    sound->typeHash = StringHash(soundType);
    sound->nameHash = StringHash(soundName);
    
    sound->delay = delay;
    sound->decibelOffset = decibelOffset;
    sound->pitch = pitch;
    sound->toleranceDistance = toleranceDistance;
    sound->distanceFalloffCoeff = distanceFalloffCoeff;
    
    FREELIST_INSERT(sound, container->firstSound);
    
    return sound;
}

inline SoundContainer* AddChildContainer(SoundContainer* container)
{
    ++container->containerCount;
    
    // TODO(Leonardo): make this use a special pool
    SoundContainer* newContainer = PushStruct(&taxTable_->pool_, SoundContainer);
    
    newContainer->soundCount = 0;
    newContainer->firstSound = 0;
    
    newContainer->containerCount = 0;
    newContainer->firstChildContainer = 0;
    
    newContainer->labelCount = 0;
    
    FREELIST_INSERT(newContainer, container->firstChildContainer);
    
    return newContainer;
}

inline void SetLabel(SoundLabel* label, char* labelName, char* labelValue)
{
    r32 value = labelValue ? ToR32(labelValue) : 0;
    u64 hash = StringHash(labelName);
    
    label->hashID = hash;
    label->value = value;
}

inline void AddSoundLabel(SoundContainer* container, char* labelName, char* labelValue)
{
    Assert(container->labelCount < ArrayCount(container->labels));
    SoundLabel* label = container->labels + container->labelCount++;
    SetLabel(label, labelName, labelValue);
}

inline void AddSoundLabel(LabeledSound* sound, char* labelName, char* labelValue)
{
    Assert(sound->labelCount < ArrayCount(sound->labels));
    SoundLabel* label = sound->labels + sound->labelCount++;
    SetLabel(label, labelName, labelValue);
}

inline void AddSoundAndChildContainersRecursively(SoundContainer* rootContainer, EditorElement* root)
{
    char* type = GetValue(root, "type");
    if(type)
    {
        rootContainer->type = (SoundContainerType) GetValuePreprocessor(SoundContainerType, type);
    }
    else
    {
        InvalidCodePath;
    }
    
	EditorElement* containerLabels = GetList(root, "labels");
	while(containerLabels)
	{
        char* labelName = GetValue(containerLabels, "name");
        char* labelValue = GetValue(containerLabels, "value");
        
        AddSoundLabel(rootContainer, labelName, labelValue);
		containerLabels = containerLabels->next;
	}
    
    EditorElement* sounds = GetList(root, "sounds");
    while(sounds)
    {
        char* soundType = GetValue(sounds, "soundType");
        char* soundName = GetValue(sounds, "sound");
        
        r32 delay = 0;
        r32 decibelOffset = 0;
        r32 pitch = 1;
        r32 toleranceDistance = 1;
        r32 distanceFalloffCoeff = 1;
        EditorElement* params = GetList(sounds, "params");
        while(params)
        {
            if(StrEqual(params->name, "delay"))
            {
                delay = ToR32(GetValue(params, "value"), 0);
            }
            else if(StrEqual(params->name, "decibelOffset"))
            {
                decibelOffset = ToR32(GetValue(params, "value"), 0);
            }
            else if(StrEqual(params->name, "pitch"))
            {
                pitch = ToR32(GetValue(params, "value"), 1);
            }
            else if(StrEqual(params->name, "toleranceDistance"))
            {
                toleranceDistance = ToR32(GetValue(params, "value"), 1);
            }
            else if(StrEqual(params->name, "distanceFalloffCoeff"))
            {
                distanceFalloffCoeff = ToR32(GetValue(params, "value"), 1);
            }
            
            params = params->next;
        }
        
        if(soundType && soundName)
        {
            LabeledSound* sound = AddSoundToContainer(rootContainer, soundType, soundName, delay, decibelOffset, pitch, toleranceDistance, distanceFalloffCoeff);
			EditorElement* soundLabels = GetList(sounds, "labels");
			while(soundLabels)
			{
                char* labelName = GetValue(soundLabels, "name");
                char* labelValue = GetValue(soundLabels, "value");
                
                AddSoundLabel(sound, labelName, labelValue);
				soundLabels = soundLabels->next;
			}
        }
        sounds = sounds->next;
    }
    
    EditorElement* childs = GetList(root, "childs");
    while(childs)
    {
        SoundContainer* childContainer = AddChildContainer(rootContainer);
        AddSoundAndChildContainersRecursively(childContainer, childs);
        
        childs = childs->next;
    }
}
internal void ImportSoundEventTab(EditorElement* root)
{
    taxTable_->eventCount = 0;
    EditorElement* events = root->firstInList;
    while(events)
    {
        char* eventName = events->name;
        SoundContainer* rootContainer = AddSoundEvent(eventName);
        
        AddSoundAndChildContainersRecursively(rootContainer, events);
        events = events->next;
    }
}