inline void AddAnimationEffect(AnimationEffects* effects, AnimationEffect effect, u32 flags, u32 triggerAction, char* triggerEffect, r32 timer, r32 fadeTime, char* pieceName)
{
    AnimationEffect* dest = PushStruct(&currentSlot_->pool, AnimationEffect);
    *dest = effect;
    
    dest->flags = flags;
    dest->triggerAction = triggerAction;
    dest->timer = timer;
    dest->fadeTime = fadeTime;
    
    if(pieceName)
    {
        if(StrEqual(pieceName, "all"))
        {
            dest->stringHashID = 0xffffffffffffffff;
        }
        else if(StrEqual(pieceName, "none"))
        {
            dest->stringHashID = 0;        
        }
        else
        {
            dest->stringHashID = StringHash(pieceName);
        }
    }
    else
    {
        dest->stringHashID = 0;
    }
    
    
    
    if(!StrEqual(triggerEffect, "invalid"))
    {
        TaxonomySlot* triggerSlot = NORUNTIMEGetTaxonomySlotByName(taxTable_, triggerEffect);
        if(triggerSlot)
        {
            dest->triggerEffectTaxonomy = triggerSlot->taxonomy;
        }
        else
        {
            EditorErrorLog(triggerEffect);
        }   
    }
    
    FREELIST_INSERT(dest, effects->firstAnimationEffect);
}


inline AnimationEffect ColorationEffect(Vec4 color)
{
    AnimationEffect result = {};
    result.type = AnimationEffect_ChangeColor;
    result.color = color;
    return result;
}

inline AnimationEffect SpawnParticlesEffect(char* particleEffectName)
{
    AnimationEffect result = {};
    result.type = AnimationEffect_SpawnParticles;
    
    TaxonomySlot* effectSlot = NORUNTIMEGetTaxonomySlotByName(taxTable_, particleEffectName);
    if(effectSlot)
    {
        result.particleEffectTaxonomy = effectSlot->taxonomy;
    }
    else
    {
        EditorErrorLog(particleEffectName);
    }
    
    return result;
}

inline AnimationEffect LightEffect(Vec3 color, r32 intensity)
{
    AnimationEffect result = {};
    result.type = AnimationEffect_Light;
    
    result.lightColor = color;
    result.lightIntensity = intensity;
    
    return result;
}

inline AnimationEffect BoltEffect(char* boltEffectName, r32 timer)
{
    AnimationEffect result = {};
    result.type = AnimationEffect_Bolt;
    result.boltTargetTimer = timer;
    
    TaxonomySlot* effectSlot = NORUNTIMEGetTaxonomySlotByName(taxTable_, boltEffectName);
    
    if(effectSlot)
    {
        result.boltTaxonomy = effectSlot->taxonomy;
    }
    else
    {
        EditorErrorLog(boltEffectName);
    }
    
    return result;
}


internal void ImportAnimationEffectsTab(AnimationEffects* effects, EditorElement* root)
{
    EditorElement* eff = root->firstInList;
    while(eff)
    {
        AnimationEffectType type = (AnimationEffectType) GetValuePreprocessor(AnimationEffectType, GetValue(eff, "animationEffectType"));
        AnimationEffect effect = {};
        u32 flags = 0;
        
        EditorElement* flagElem = GetList(eff, "flags");
        
        while(flagElem)
        {
            char* flagName = GetValue(flagElem, "effectFlag");
            u32 flag = GetFlagPreprocessor(AnimationEffectFlags, flagName);
            flags |= flag;
            
            flagElem = flagElem->next;
        }
        
        
        EntityAction action = (EntityAction) GetValuePreprocessor(EntityAction, GetValue(eff, "action"));
        char* triggerEffect = GetValue(eff, "triggerEffect");
        char* pieceName = GetValue(eff, "animationPieceName");
        r32 timer = ElemR32(eff, "timer");
        r32 fadeTime = ElemR32(eff, "fadeInTimer");
        
        switch(type)
        {
            case AnimationEffect_ChangeColor:
            {
                Vec4 color = ColorV4(eff, "color");
                effect = ColorationEffect(color);       
            } break;
            
            case AnimationEffect_SpawnParticles:
            {
                char* effectName = GetValue(eff, "particleEffectName");
                effect = SpawnParticlesEffect(effectName);       
            } break;
            
            case AnimationEffect_Light:
            {
                Vec4 color = ColorV4(eff, "color");
                r32 intensity = ElemR32(eff, "lightIntensity");
                effect = LightEffect(color.rgb, intensity);     
            } break;
            
            case AnimationEffect_Bolt:
            {
                char* boltName = GetValue(eff, "boltName");
                r32 boltTimer = ElemR32(eff, "boltTimer");
                effect = BoltEffect(boltName, boltTimer);
            } break;
        }
        
        if(effect.type)
        {
            AddAnimationEffect(effects, effect, flags, action, triggerEffect, timer, fadeTime, pieceName);
        }
        
        eff = eff->next;
    }
}